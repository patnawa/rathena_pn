#!/usr/bin/env python3
"""Fail-closed pin of the reviewed Biosphere crown callback closure.

This is a change detector for a human-reviewed current-content proof, NOT a
script sandbox or an arbitrary-script semantic verifier. No files are written.
The normal audit needs PyYAML; --verify-manifest needs only the Python stdlib.
Live persisted bonus_script text is deliberately outside this source proof.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
DATABASES = {
    'db/item_db.yml': ('Id', 'ITEM_DB'),
    'db/item_combos.yml': (None, 'COMBO_DB'),
    'db/item_randomopt_db.yml': ('Id', 'RANDOM_OPTION_DB'),
    'db/status.yml': ('Status', 'STATUS_DB'),
    'db/pet_db.yml': ('Mob', 'PET_DB'),
}
ENGINE_TREES = ('src/map', 'src/common', 'src/config')
ENGINE_SUFFIXES = {'.cpp', '.hpp', '.h'}
NPC_ROOT = 'npc/re/scripts_main.conf'
SCRIPT_FIELDS = ('Script', 'EquipScript', 'UnEquipScript', 'SupportScript')
SCOPE_TYPES = {'Weapon', 'Armor', 'ShadowGear', 'Card', 'Ammo'}
REVIEW = '2026-09-06 current valid Biosphere 19-crown callbacks; not arbitrary persisted code'

# SHA256 of canonical JSON for each COMPLETE manifest section. Manual review is
# required before changing these. There is intentionally no update/accept flag.
PINS = {
    'engine': '213aa0d386ab10a268a666095ce2aa2ab9355da8ab44aded401fe61c9f220c2f',
    'databases': '393c5f74fd82616b74a5667818cb4e5443d683a9ce693af1f3fd8d03f22df331',
    'npcs': 'bb7fb7a83501e786251566b1bb4480b521fa5dc91cffd36fd40550dcdc5a046b',
    'closure': '79513f7035bb459077a3eeae053c0db1f1df5c49c758abbfeab76a513a7f228c',
}

# Explicit, separately reviewed preserved live sources. Never selected after a
# default checksum failure. Source equivalence is NOT running-binary provenance:
# the preserved suicidebombing.cpp delta was not part of the deployed build.
PROFILES = {
    'live-20260906': {
        'files': {
            'db/import/item_db.yml': '20946df000e0fba0710843bfd676dc895818e96fac6a0f3e2296e04cb39a287b',
            'db/import/pet_db.yml': '8c70d5f5b70b905082f3431fc0990ea76c736fb3c4b18713e899764d5297c00a',
            'src/map/skills/npc/suicidebombing.cpp': '80e5ebd92e5eec5e26eec2f3bfb62cb61f894cd8c94bbed097545b7ee7ec4164',
        },
        'pins': {
            'engine': '01a5af6f37959bd526b25cdbe47e48fabef1cc7ace0950570a6b96e369ae925a',
            'databases': 'a9a970c11e8c7e30bc3be916237b9e106a6358906c4ad3e537b970a2485a1088',
            'npcs': 'bb7fb7a83501e786251566b1bb4480b521fa5dc91cffd36fd40550dcdc5a046b',
            'closure': 'b516348011c7faab98fc5023cb3dd453f5ff67e767a72c37410bb9d8599c0d86',
        },
    },
}


def selected_pins(profile=None):
    if profile is None:
        return PINS
    if profile not in PROFILES:
        raise ValueError(f'Unknown callback closure profile: {profile!r}')
    return PROFILES[profile]['pins']


def digest(value):
    return hashlib.sha256(value.encode('utf-8', errors='surrogateescape')).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(',', ':'))


class Reader:
    """Strict in-root reader. Overrides exist only for in-memory negative tests."""
    def __init__(self, root, overrides=None):
        self.root = Path(root).resolve()
        self.overrides = overrides or {}
        self.cache = {}
        self.paths = {}

    def path(self, relative):
        if relative in self.paths:
            return self.paths[relative]
        if not isinstance(relative, str) or not relative or '\\' in relative:
            raise ValueError(f'Invalid source path: {relative!r}')
        path = (self.root / relative).resolve()
        try:
            path.relative_to(self.root)
        except ValueError as exc:
            raise ValueError(f'Source escapes root: {relative}') from exc
        if Path(relative).is_absolute():
            raise ValueError(f'Absolute source path: {relative}')
        self.paths[relative] = path
        return path

    def text(self, relative):
        path = self.path(relative)
        if relative in self.overrides:
            value = self.overrides[relative]
            if value is None:
                raise ValueError(f'Missing required source: {relative}')
            return value.replace('\r\n', '\n').replace('\r', '\n')
        if relative in self.cache:
            return self.cache[relative]
        try:
            # Some enabled legacy NPCs contain non-UTF-8 display text. Preserve
            # those bytes exactly with surrogateescape; never replace/discard.
            value = path.read_bytes().replace(b'\r\n', b'\n').replace(b'\r', b'\n').decode('utf-8', errors='surrogateescape')
            self.cache[relative] = value
            return value
        except (OSError, UnicodeError) as exc:
            raise ValueError(f'Cannot read required source {relative}: {exc}') from exc


def engine_paths(root):
    paths = []
    for tree in ENGINE_TREES:
        base = Path(root) / tree
        if not base.is_dir():
            raise ValueError(f'Missing engine source tree: {tree}')
        paths.extend(p.relative_to(root).as_posix() for p in base.rglob('*')
                     if p.is_file() and p.suffix in ENGINE_SUFFIXES)
    return sorted(paths)


def _yaml_load(text, name):
    try:
        import yaml
    except ImportError as exc:
        raise ValueError('Full audit requires PyYAML; host may use --verify-manifest.') from exc
    try:
        value = yaml.load(text, Loader=getattr(yaml, 'CSafeLoader', yaml.SafeLoader))
    except yaml.YAMLError as exc:
        raise ValueError(f'Invalid YAML in {name}: {exc}') from exc
    if not isinstance(value, dict):
        raise ValueError(f'Expected database mapping: {name}')
    return value


def database_graph(reader):
    files, graph, records = {}, [], {}
    for root, (_, expected_type) in DATABASES.items():
        active = set()
        ordered = []

        def visit(relative):
            resolved = reader.path(relative)
            if resolved in active:
                raise ValueError(f'Database import cycle: {relative}')
            active.add(resolved)
            text = reader.text(relative)
            files[relative] = digest(text)
            data = _yaml_load(text, relative)
            if data.get('Header', {}).get('Type') != expected_type:
                raise ValueError(f'Wrong database type: {relative}')
            body = data.get('Body') or []
            if not isinstance(body, list) or any(not isinstance(r, dict) for r in body):
                raise ValueError(f'Invalid database body: {relative}')
            ordered.extend(copy.deepcopy(body))
            imports = data.get('Footer', {}).get('Imports', [])
            if not isinstance(imports, list):
                raise ValueError(f'Invalid imports: {relative}')
            for index, entry in enumerate(imports):
                if not isinstance(entry, dict) or set(entry) - {'Path', 'Mode'}:
                    raise ValueError(f'Unsupported import declaration: {relative}')
                target, mode = entry.get('Path'), entry.get('Mode', 'Renewal')
                if mode not in ('Renewal', 'Prerenewal'):
                    raise ValueError(f'Unsupported database mode: {mode}')
                reader.path(target)
                selected = mode == 'Renewal'
                graph.append([root, relative, index, target, mode, selected])
                if selected:
                    visit(target)
            active.remove(resolved)

        visit(root)
        records[root] = ordered
    summary = {root: {'records': len(rows), 'ordered_records_sha256': digest(canonical(rows))}
               for root, rows in records.items()}
    return {'roots': list(DATABASES), 'files': files, 'graph': graph,
            'file_count': len(files), 'databases': summary}, records


def npc_graph(reader):
    configs, scripts, graph, active = {}, {}, [], set()

    def visit(relative):
        resolved = reader.path(relative)
        if resolved in active:
            raise ValueError(f'NPC import cycle: {relative}')
        active.add(resolved)
        text = reader.text(relative)
        configs[relative] = digest(text)
        for line_number, line in enumerate(text.splitlines(), 1):
            line = line.split('//', 1)[0].strip()
            if not line or line.startswith('#'):
                continue
            match = re.fullmatch(r'(import|npc)\s*:\s*(\S+)\s*', line)
            if not match:
                raise ValueError(f'Unsupported NPC include syntax: {relative}:{line_number}: {line}')
            kind, target = match.groups()
            reader.path(target)
            graph.append([relative, line_number, kind, target])
            if kind == 'import':
                visit(target)
            else:
                scripts[target] = digest(reader.text(target))
        active.remove(resolved)

    visit(NPC_ROOT)
    return {'root': NPC_ROOT, 'configs': configs, 'scripts': scripts, 'graph': graph,
            'config_count': len(configs), 'script_count': len(scripts)}


def scalar_overlay(rows, key):
    """Native scalar Script inheritance view; not a general DB merge emulator.

    Full ordered records and all raw files are independently pinned, including
    nested flags, card positions, classes and membership merge semantics.
    """
    effective = {}
    for row in rows:
        if key not in row:
            raise ValueError(f'Missing database identity field {key}')
        effective.setdefault(row[key], {}).update(row)
    return effective


def closure_inventory(records, reader, npcs):
    items = scalar_overlay(records['db/item_db.yml'], 'Id')
    all_item_scripts = [[i, field, row[field]] for i, row in sorted(items.items())
                        for field in SCRIPT_FIELDS if row.get(field)]
    scoped = [[i, field, row[field]] for i, row in sorted(items.items())
              if row.get('Type') in SCOPE_TYPES for field in SCRIPT_FIELDS[:3] if row.get(field)]
    pools = {'equipment_card_ammo': scoped}
    for root, name in [('db/item_combos.yml', 'combos'),
                       ('db/item_randomopt_db.yml', 'options'),
                       ('db/status.yml', 'statuses'), ('db/pet_db.yml', 'pets')]:
        rows = records[root]
        key = DATABASES[root][0]
        selected = list(scalar_overlay(rows, key).values()) if key else rows
        pools[name] = [[n, field, row[field]] for n, row in enumerate(selected)
                       for field in SCRIPT_FIELDS if row.get(field)]

    # Literal extraction is traversal accounting only. Exact source/record pins,
    # not these patterns, carry the previously completed semantic review.
    producers = []
    marker = re.compile(r'\b(?:bonus_script|(?:pet)?autobonus[23]?|SC_ITEMSCRIPT)\b')
    for ident, field, text in all_item_scripts:
        if marker.search(text):
            producers.append(['item', ident, field, text])
    for name in ('combos', 'options', 'statuses', 'pets'):
        for ident, field, text in pools[name]:
            if marker.search(text):
                producers.append([name, ident, field, text])
    for name in sorted(npcs['scripts']):
        text = reader.text(name)
        if marker.search(text):
            producers.append(['npc', name, 'source', text])
    literal = re.compile(r'\b((?:pet)?autobonus[23]?|bonus_script)\s*\(?\s*("(?:\\.|[^"\\])*")')
    bodies = [[*p[:3], m[1], m[2]] for p in producers for m in literal.finditer(p[3])]
    body_counts = {}
    for body in bodies:
        body_counts[body[3]] = body_counts.get(body[3], 0) + 1
    return {
        'effective_items': len(items),
        'effective_scoped_items': sum(row.get('Type') in SCOPE_TYPES for row in items.values()),
        'all_item_script_fields': len(all_item_scripts),
        'all_item_scripts_sha256': digest(canonical(all_item_scripts)),
        'pools': {name: {'script_fields': len(rows), 'sha256': digest(canonical(rows))}
                  for name, rows in pools.items()},
        'producer_records': len(producers), 'producer_sha256': digest(canonical(producers)),
        'literal_first_body_counts': body_counts, 'literal_first_body_sha256': digest(canonical(bodies)),
        'sc_itemscript_producers': [[*p[:3], p[3]] for p in producers if 'SC_ITEMSCRIPT' in p[3]],
    }


def collect(root=ROOT, _overrides=None):
    """Collect evidence only. Call validate(), not collect(), to accept a tree."""
    reader = Reader(root, _overrides)
    paths = engine_paths(reader.root)
    return _collect(reader, paths)


def _collect(reader, paths):
    engine = {'trees': list(ENGINE_TREES), 'suffixes': sorted(ENGINE_SUFFIXES),
              'file_count': len(paths), 'files': {p: digest(reader.text(p)) for p in paths}}
    databases, records = database_graph(reader)
    npcs = npc_graph(reader)
    closure = closure_inventory(records, reader, npcs)
    return {'schema': 1, 'review': REVIEW, 'normalization': 'CRLF/CR -> LF only; all other source bytes preserved',
            'engine': engine, 'databases': databases, 'npcs': npcs, 'closure': closure,
            'live_persisted_text_verified': False}


def _check_manifest(manifest, profile=None):
    pins = selected_pins(profile)
    if manifest.get('schema') != 1 or manifest.get('review') != REVIEW:
        raise ValueError('Unknown callback closure review/schema')
    if manifest.get('live_persisted_text_verified') is not False:
        raise ValueError('Static manifest must not claim live persistence verification')
    failures = []
    if profile is not None:
        files = {}
        for section in (manifest.get('engine', {}).get('files', {}),
                        manifest.get('databases', {}).get('files', {}),
                        manifest.get('npcs', {}).get('configs', {}),
                        manifest.get('npcs', {}).get('scripts', {})):
            files.update(section)
        for path, expected in PROFILES[profile]['files'].items():
            if files.get(path) != expected:
                failures.append(f'{profile} requires exact reviewed file: {path}')
    for section, expected in pins.items():
        actual = digest(canonical(manifest.get(section)))
        if actual != expected:
            failures.append(f'{section}: expected {expected}, actual {actual}')
    if failures:
        raise ValueError('Callback closure changed; human re-review required:\n' + '\n'.join(failures))
    return manifest


def validate(root=ROOT, *, profile=None):
    """Return reviewed manifest or raise ValueError; profile is explicit only.

    A named profile traverses and recomputes the entire closure, not just its
    three preserved differences. See the dated manual re-review in the doc.
    """
    selected_pins(profile)
    return _check_manifest(collect(root), profile)


def verify_manifest(root, manifest, *, profile=None):
    """Stdlib-only host equivalence check against an already pinned manifest."""
    _check_manifest(manifest, profile)
    reader = Reader(root)
    if engine_paths(reader.root) != sorted(manifest['engine']['files']):
        raise ValueError('Engine source membership changed')
    files = {}
    for section in (manifest['engine']['files'], manifest['databases']['files'],
                    manifest['npcs']['configs'], manifest['npcs']['scripts']):
        for path, sha in section.items():
            if path in files and files[path] != sha:
                raise ValueError(f'Inconsistent manifest file: {path}')
            files[path] = sha
    failures = [path for path, expected in sorted(files.items()) if digest(reader.text(path)) != expected]
    if failures:
        raise ValueError('Host callback source/content differs: ' + ', '.join(failures))
    return {'verified_files': len(files), 'pins': dict(selected_pins(profile)), 'live_persisted_text_verified': False}


def negative_controls(root=ROOT, *, profile=None, _overrides=None):
    """Inject deliberate changes in memory; never alter the user's checkout."""
    reader = Reader(root, _overrides)
    paths = engine_paths(reader.root)
    baseline = _check_manifest(_collect(reader, paths), profile)
    changes = {
        'engine implementation': ('src/map/pc.cpp', '\n// deliberate callback change\n'),
        'other equipped item Script': ('db/re/item_db_equip.yml', 'delitem 1001555,180;'),
        'card Script': ('db/re/item_db_etc.yml', 'delitem 1001555,180;'),
        'combo Script': ('db/re/item_combos.yml', 'delitem 1001555,180;'),
        'random option Script': ('db/re/item_randomopt_db.yml', 'delitem 1001555,180;'),
        'status Script': ('db/re/status.yml', 'ep17_2_main = 0;'),
        'pet Script': ('db/re/pet_db.yml', 'delitem 1001555,180;'),
        'persisted bonus provider': ('db/re/item_db_usable.yml', 'bonus_script "delitem 1001555,180;",60;'),
        'enabled NPC provider': ('npc/other/Global_Functions.txt', '\nfunction\tscript\tInjectedClosureProvider\t{ bonus_script "delitem 1001555,180;",60; return; }\n'),
    }
    results = []

    def reject(label, overrides):
        injected = Reader(root, {**reader.overrides, **overrides})
        # Reuse the captured, reviewed source snapshot for fast isolated cases.
        # Changed files take precedence over this cache; there are no writes.
        injected.cache, injected.paths = reader.cache, reader.paths
        try:
            _check_manifest(_collect(injected, paths), profile)
        except ValueError:
            results.append(label)
        else:
            raise AssertionError(f'Negative control was accepted: {label}')

    for label, (path, replacement) in changes.items():
        original = reader.text(path)
        if path.endswith('.yml'):
            data = _yaml_load(original, path)
            # Select a real script-bearing record, then replace its executable
            # contents. Do not mistake a changed comment for a semantic test.
            record = next(r for r in data['Body'] if r.get('Script'))
            record['Script'] = replacement + '\n'
            import yaml
            changed = yaml.safe_dump(data, sort_keys=False)
        else:
            changed = original + replacement
        reject(label, {path: changed})
    equip_text = reader.text('db/re/item_db_equip.yml')
    for field in ('EquipScript', 'UnEquipScript'):
        data = _yaml_load(equip_text, 'db/re/item_db_equip.yml')
        data['Body'][0][field] = 'delitem 1001555,180;\n'
        import yaml
        reject(f'base {field}', {'db/re/item_db_equip.yml': yaml.safe_dump(data, sort_keys=False)})
    reject('missing imported database', {'db/re/item_db_equip.yml': None})
    config = reader.text(NPC_ROOT)
    reject('new enabled NPC import', {NPC_ROOT: config + '\nnpc: npc/custom/closure_negative.txt\n',
                                     'npc/custom/closure_negative.txt': '-\tscript\tInjected\t-1,{ end; }\n'})
    root_db = reader.text('db/item_db.yml')
    reject('new Renewal database import', {
        'db/item_db.yml': root_db + '  - Path: db/import/closure_negative.yml\n    Mode: Renewal\n',
        'db/import/closure_negative.yml': 'Header:\n  Type: ITEM_DB\n  Version: 3\nBody:\n  - Id: 5013\n    Script: |\n      delitem 1001555,180;\n',
    })
    reject('NPC include cycle', {NPC_ROOT: config + '\nimport: npc/re/scripts_main.conf\n'})
    reject('NPC include escapes root', {NPC_ROOT: config + '\nnpc: ../outside.txt\n'})
    tampered = copy.deepcopy(baseline)
    tampered['engine']['files'].pop(next(iter(tampered['engine']['files'])))
    try:
        verify_manifest(root, tampered, profile=profile)
    except ValueError:
        results.append('tampered host manifest')
    else:
        raise AssertionError('Tampered manifest accepted')
    if profile is not None:
        for path in PROFILES[profile]['files']:
            reject(f'changed exact profile file: {path}', {path: reader.text(path) + '\n'})
        try:
            _check_manifest(baseline)
        except ValueError:
            results.append('named profile rejected by default gate')
        else:
            raise AssertionError('Named profile silently accepted by default gate')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--profile', choices=sorted(PROFILES), help='explicit separately reviewed live source profile; never inferred')
    parser.add_argument('--manifest', action='store_true', help='emit the validated per-file JSON manifest')
    parser.add_argument('--verify-manifest', type=Path, help='stdlib-only verification of pinned manifest against root')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    try:
        if args.negative_controls:
            result = {'negative_controls_rejected': negative_controls(args.root, profile=args.profile),
                      'pins': selected_pins(args.profile)}
        elif args.verify_manifest:
            result = verify_manifest(args.root, json.loads(args.verify_manifest.read_text(encoding='utf-8')), profile=args.profile)
        else:
            result = validate(args.root, profile=args.profile)
        if args.manifest or args.verify_manifest or args.negative_controls:
            print(json.dumps(result, sort_keys=True, indent=2))
        else:
            print('PASS: reviewed callback source/content/include closure unchanged; live persisted text NOT verified')
            print(json.dumps({'pins': selected_pins(args.profile), 'closure': result['closure']}, sort_keys=True))
    except (ValueError, OSError, AssertionError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
