#!/usr/bin/env python3
"""Bind scoped release evidence to candidate inputs; never promote missing proof.

This validates evidence integrity, not the truth of human observations. Rendered
receipts must come from actual client runs. Configuration is recorded separately
because isolated fixtures necessarily use different database credentials.
"""
import argparse
import hashlib
import json
import math
from pathlib import Path
import sys


SOURCE_DIRS = ('src', '3rdparty', 'db', 'npc', 'sql-files', 'tools', 'client-patch', '.github')
BUILD_FILES = ('configure', 'configure.ac', 'Makefile.in', 'CMakeLists.txt')
BINARIES = ('map-server', 'char-server', 'login-server', 'web-server')
# Operational files are configuration, even though templates live under tools/.
OPERATIONAL_CONFIG = (
    'tools/docker/docker-compose.yml', 'tools/docker/docker-compose.override.yml',
    'tools/docker/asset/char_conf.txt', 'tools/docker/asset/inter_conf.txt',
    'tools/docker/asset/map_conf.txt',
)
# Exact reproducible outputs; keep their builders/assets/source in the identity.
DERIVED_OUTPUTS = (
    'tools/grf_v3_extract/grf_v3_extract.exe',
    'client-patch/client_compat/client_compat.grf',
    'client-patch/enchant_repair/enchant_repair.grf',
)

SCOPES = {
    'market': {'sql': {'market-restore-restart'}},
    'pets': {'sql': {'shop-recovery', 'pet-atomic-payment', 'pet-replay', 'pet-full-inventory',
                    'pet-character-switch', 'pet-restart', 'pet-all-producers'}},
    'release': {'sql': {'shop-recovery'}},
    'barter': {'sql': {'shop-recovery'}},
    'guide': {'rendered': {'onboarding'}},
    'content': {'rendered': {'onboarding', 'encounter-reward', 'party-reentry',
                             'shop-storage', 'client-visuals'}},
    'metrics': {'responsiveness': {'idle-baseline', 'load-baseline', 'process-stall',
                                   'single-buyer', 'four-buyers', 'sql-delayed-purchase'}},
    'lab': {'rendered': {'lab-relog-comparison'}},
    'npc-fixes': {},
    'client-fixes': {'rendered': {'client-visuals'}},
}


def sha256(path):
    digest = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            digest.update(chunk)
    return digest.hexdigest()


def derived_makefile(path):
    # configure writes these from tracked templates; they are build outputs.
    return path.name == 'Makefile' and path.with_name('Makefile.in').is_file()


def inventory(root, directories):
    paths = set()
    for directory in directories:
        paths.update(p for p in (root / directory).rglob('*') if p.is_file()
                     and not any(x in ('obj', 'obj-gen', '__pycache__', '.git') for x in p.relative_to(root).parts)
                     and p.suffix not in ('.o', '.a', '.pyc')
                     and p.relative_to(root).as_posix() not in (*OPERATIONAL_CONFIG, *DERIVED_OUTPUTS)
                     and not (directory in ('src', '3rdparty') and derived_makefile(p)))
    return {p.relative_to(root).as_posix(): sha256(p) for p in sorted(paths)}


def build_inventory(root):
    return {name: sha256(root / name) for name in BUILD_FILES if (root / name).is_file()}


def configuration_inventory(root):
    result = inventory(root, ('conf',))
    result.update({name: sha256(root / name) for name in OPERATIONAL_CONFIG if (root / name).is_file()})
    return result


def binding(root):
    inputs = inventory(root, SOURCE_DIRS)
    inputs.update(build_inventory(root))
    encoded = json.dumps(inputs, sort_keys=True, separators=(',', ':')).encode()
    return {'source_sha256': hashlib.sha256(encoded).hexdigest(),
            'binaries': {name: sha256(root / name) for name in BINARIES if (root / name).is_file()}}


def checked_path(root, relative):
    path = (root / relative).resolve()
    if not relative or Path(relative).is_absolute() or root.resolve() not in path.parents:
        raise ValueError('Evidence path escapes its root: ' + str(relative))
    return path


def verify_files(root, hashes):
    if not isinstance(hashes, dict) or not hashes:
        raise ValueError('Missing evidence file hashes')
    for name, expected in hashes.items():
        if sha256(checked_path(root, name)) != expected:
            raise ValueError('Changed evidence input: ' + name)


def assemble(root, scopes, receipts, stage='candidate'):
    unknown = set(scopes) - SCOPES.keys()
    if unknown or not scopes:
        raise ValueError('Explicit known release scopes required: ' + str(sorted(unknown)))
    if stage not in ('candidate', 'deployed'):
        raise ValueError('Unknown release stage')
    current = binding(root)
    required = {'native': {'full-gate'}}
    for scope in scopes:
        for kind, cases in SCOPES[scope].items():
            required.setdefault(kind, set()).update(cases)
    if stage == 'deployed':
        required['deployment'] = {'backup-restore', 'deployed-hashes', 'health', 'data-invariants', 'git-remote'}
    result = {'schema': 1, 'passed': False, 'stage': stage, 'scopes': sorted(set(scopes)),
              'binding': current, 'configuration_sha256': configuration_inventory(root),
              'requirements': {k: sorted(v) for k, v in required.items()},
              'evidence': {}, 'errors': []}
    for kind, cases in required.items():
        path = receipts.get(kind)
        if path is None:
            result['errors'].append('Missing ' + kind + ' evidence: ' + ', '.join(sorted(cases)))
            continue
        try:
            receipt = json.loads(path.read_text(encoding='utf-8'))
            if receipt.get('passed') is not True:
                raise ValueError('Receipt has not passed')
            if kind == 'deployment' and receipt.get('configuration_sha256') != result['configuration_sha256']:
                raise ValueError('Deployment configuration differs from declared candidate')
            tested = receipt.get('binding', {})
            if tested.get('source_sha256') != current['source_sha256']:
                raise ValueError('Source identity differs from candidate')
            needed = ('map-server',) if kind == 'native' else (('char-server', 'login-server') if 'pets' in scopes else ('char-server',)) if kind == 'sql' else ('map-server', 'char-server')
            for name in needed:
                if not current['binaries'].get(name) or tested.get('binaries', {}).get(name) != current['binaries'][name]:
                    raise ValueError('Missing or different binary: ' + name)
            for name, value in tested.get('binaries', {}).items():
                if current['binaries'].get(name) != value:
                    raise ValueError('Tested binary changed: ' + name)
            completed = set()
            if kind == 'native':
                from release_checks import TESTS, FULL_TESTS
                checks = receipt.get('checks', [])
                if receipt.get('phase') != 'full' or not checks or any(
                        row.get('passed') is not True or row.get('status') != 'passed' for row in checks):
                    raise ValueError('Full native gate is incomplete')
                if not any(row.get('name') == 'isolated_startup' for row in checks):
                    raise ValueError('Isolated startup missing')
                expected = set(TESTS + FULL_TESTS) | {'database_yaml_syntax', 'client_compat_assets', 'isolated_startup'}
                names = [row.get('name') for row in checks]
                if len(names) != len(set(names)) or set(names) != expected:
                    raise ValueError('Gate check inventory differs from current runner')
                for row in checks:
                    if row['name']=='database_yaml_syntax': continue
                    log = Path(row.get('log', ''))
                    if not log.is_absolute():
                        log = path.parent / log
                    log = log.resolve()
                    if path.parent.resolve() not in log.parents or not row.get('sha256') or sha256(log)!=row['sha256']:
                        raise ValueError('Native log missing, changed or outside evidence root: '+row['name'])
                completed.add('full-gate')
            if kind == 'sql' and 'input_sha256' in receipt:
                verify_files(root, receipt['input_sha256'])
                verify_files(path.parent, receipt.get('artifacts'))
                fields = ('runtime', 'character_wire_handler', 'concurrent_final_unit',
                          'committed_restart', 'uncommitted_restart')
                if not all(isinstance(receipt.get(key), str) and receipt[key] for key in fields):
                    raise ValueError('SQL recovery scenarios missing')
                if 'pets' in scopes:
                    pet_fields = ('pet_entitlement_core', 'point_asset_commit', 'point_global_barrier', 'point_global_restart', 'point_login_adapter', 'pet_asset_commit', 'pet_retirement', 'pet_mail_asset', 'pet_floor_asset', 'pet_floor_restart',
                                  'pet_entitlement_restart', 'pet_concurrent_result', 'pet_uncommitted_restart')
                    if not all(isinstance(receipt.get(key), str) and receipt[key] for key in pet_fields):
                        raise ValueError('Production pet SQL scenarios missing')
                completed.add('shop-recovery')
            case_names = [case['name'] for case in receipt.get('cases', [])]
            if len(case_names) != len(set(case_names)):
                raise ValueError('Duplicate evidence cases')
            for case in receipt.get('cases', []):
                if case['name'] in ('full-gate', 'shop-recovery'):
                    raise ValueError('Built-in gates require their original runner reports')
                if case.get('status') != 'passed':
                    continue
                verify_files(path.parent, case.get('artifacts'))
                if kind == 'responsiveness' and case.get('name') in ('idle-baseline', 'load-baseline'):
                    duration = case.get('duration_seconds')
                    if type(duration) not in (int, float) or not math.isfinite(duration) or duration < 1800:
                        raise ValueError('Baseline must cover at least 30 minutes')
                completed.add(case['name'])
            if cases - completed:
                raise ValueError('Missing passed cases: ' + ', '.join(sorted(cases - completed)))
            result['evidence'][kind] = {'path': str(path), 'sha256': sha256(path), 'cases': sorted(completed)}
        except (ValueError, OSError, KeyError, TypeError) as exc:
            result['errors'].append(kind + ': ' + str(exc))
    if binding(root) != current:
        result['errors'].append('Candidate changed while assembling evidence')
    result['passed'] = not result['errors']
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--scope', action='append', choices=sorted(SCOPES), required=True)
    parser.add_argument('--evidence', action='append', default=[], metavar='KIND=REPORT')
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--stage', choices=('candidate', 'deployed'), default='candidate')
    args = parser.parse_args()
    receipts = {}
    for value in args.evidence:
        kind, separator, name = value.partition('=')
        if not separator or kind in receipts:
            parser.error('Evidence must be unique KIND=REPORT arguments')
        receipts[kind] = Path(name).resolve()
    root = args.candidate.resolve()
    output = args.output.resolve()
    if output == root or root in output.parents:
        parser.error('Write the bundle outside the candidate')
    result = assemble(root, args.scope, receipts, args.stage)
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_name(output.name + '.tmp')
    temporary.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    temporary.replace(output)
    print(json.dumps({'passed': result['passed'], 'errors': result['errors'], 'bundle': str(output)}))
    return 0 if result['passed'] else 1


if __name__ == '__main__':
    sys.exit(main())
