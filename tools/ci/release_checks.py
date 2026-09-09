#!/usr/bin/env python3
"""Fail closed on database syntax, focused regressions, and isolated startup errors."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
TESTS = (
    'release_checks_test.py',
    'scdata_reload_test.py', 'rodex_operation_test.py',
    'hotfix_bonus_regression.py', 'fly_wing_rental_regression.py',
    'aquila_cast_time_test.py',
    'equipment_progression_test.py',
    'equipment_reform_transaction_test.py',
    'refine_transaction_test.py',
    'episode_party_progression_test.py',
    'database_backup_test.py',
    'mob_sql_schema_test.py',
    'client_archive_stack_test.py',
    'instance_access_manifest_test.py',
)


def candidate_digest(root):
    """Include local import overrides as well as tracked candidate inputs."""
    digest = hashlib.sha256()
    files = [root / 'map-server']
    for directory in ('src', 'db', 'npc', 'conf', 'sql-files', 'tools', 'client-patch'):
        files.extend(p for p in (root / directory).rglob('*') if p.is_file()
                     and not any(part in ('obj', '__pycache__') for part in p.parts)
                     and p.suffix not in ('.o', '.a', '.pyc'))
    for path in sorted(files):
        digest.update(path.relative_to(root).as_posix().encode() + b'\0')
        digest.update(hashlib.sha256(path.read_bytes()).digest())
    return digest.hexdigest()


def startup_errors(text):
    text = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', text)
    errors = []
    for line in text.splitlines():
        failure = re.search(r'\[Error\]|\[Fatal[^\]]*\]|AddressSanitizer|runtime error:|Segmentation fault', line, re.I)
        # ShowSQL emits a primary [SQL] severity. Connection announcements
        # instead use [Info]: [SQL]: or [Status]: [SQL]: and are not errors.
        severity = re.search(r'\[(Info|Status|Notice|Warning|Debug|SQL)\]:', line, re.I)
        sql_failure = severity and severity.group(1).lower() == 'sql'
        sql_db_error = re.search(r'\[SQL\]:\s*DB error\b', line, re.I)
        if failure or sql_failure or sql_db_error:
            errors.append(line)
    if "Server is 'ready' and listening" not in text and 'Map Server is now online' not in text:
        errors.append('Map server readiness marker missing')
    return errors


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--phase', choices=['source', 'full'], default='full')
    parser.add_argument('--startup-log', type=Path, help='Output log for the isolated map-server invocation; required for full')
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    if args.phase == 'full' and not args.startup_log:
        parser.error('--startup-log is required for the full release gate')
    if sys.platform != 'linux':
        parser.error('Run on Linux: the native regression tests require GCC and sanitizers')
    import yaml
    checks = []
    report = {'phase': args.phase, 'passed': False, 'checks': checks}
    try:
        if args.phase == 'full':
            report['candidate_sha256'] = candidate_digest(ROOT)
        count = 0
        for path in sorted((ROOT / 'db').rglob('*.yml')):
            with path.open(encoding='utf-8-sig') as stream:
                list(yaml.load_all(stream, Loader=yaml.CSafeLoader if hasattr(yaml, 'CSafeLoader') else yaml.SafeLoader))
            count += 1
        checks.append({'name': 'database_yaml_syntax', 'passed': True, 'files': count})
        tests = TESTS + (('chapter1_protection_test.py', 'instance_entry_native_test.py',
                         'episode21_finale_flow_test.py', 'episode21_checkpoint_test.py',
                         'mob_matk_range_test.py', 'immortal_instance_test.py',
                         'instance_warper_test.py') if args.phase == 'full' else ())
        commands = [(name, [sys.executable, str(ROOT / 'tools/ci' / name)]) for name in tests]
        commands.append(('client_compat_assets', [sys.executable,
                         str(ROOT / 'client-patch/client_compat/validate.py'), '--assets-only']))
        for name, command in commands:
            started = time.monotonic()
            result = subprocess.run(command, cwd=ROOT, timeout=900)
            checks.append({'name': name, 'passed': result.returncode == 0, 'seconds': round(time.monotonic() - started, 2)})
            result.check_returncode()
        if args.phase == 'full':
            args.startup_log.parent.mkdir(parents=True, exist_ok=True)
            with args.startup_log.open('wb') as stream:
                result = subprocess.run([str(ROOT / 'map-server'), '--run-once'], cwd=ROOT,
                                        stdout=stream, stderr=subprocess.STDOUT, timeout=300)
            content = args.startup_log.read_bytes()
            errors = startup_errors(content.decode('utf-8', errors='replace'))
            if result.returncode:
                errors.append(f'map-server exited with status {result.returncode}')
            if candidate_digest(ROOT) != report['candidate_sha256']:
                errors.append('Candidate files changed during validation; rerun on a stable build')
            checks.append({'name': 'isolated_startup', 'passed': not errors, 'sha256': hashlib.sha256(content).hexdigest(), 'errors': errors})
            if errors:
                raise ValueError('\n'.join(errors))
        report['passed'] = True
    except Exception as exc:
        report['error'] = str(exc)
        raise
    finally:
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f"PASS: {args.phase} release checks ({len(checks)} checks). Report: {args.report}")


if __name__ == '__main__':
    main()
