#!/usr/bin/env python3
"""Fail-closed source/data evidence for the two Final Battle crystal grants.

Requires the separately reviewed broad callback gate. Hashes retain an explicit
manual review; a keyword scan is not arbitrary-script semantic verification.
No live persistence, compiled-binary equivalence, or crash atomicity is claimed.
"""
from __future__ import annotations

import argparse
import copy
import json
from pathlib import Path
import re
import sys

import biosphere_callback_closure_audit as base

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/custom/episode21/FinalBattle.txt'
DAILY = 'npc/custom/episode21/MysteriousGhostShip.txt'
IDS = tuple(sorted([103512, 1001480, *range(1001653, 1001665),
                    *range(1001034, 1001038), *range(1000812, 1000815)]))
PINS = {
    'outputs': 'a99d5fe79ba2c1fd1293e2679dd733cd6ed9f214b71ca442c312f411e4c23507',
    'achievements': '25abd01b4f4bd15b5c69d303ba9510c0c42ab73c8fd11a7fd9488b87034a2788',
    'npc_scope': '899459d9f77e22228278eb37fc15d5a972d21e568474e84abeaea9f906d32044',
    'script_limits': '0cbf3e59420392021263f8d4672308303919cc0eefe5d16cd799fb73af0a2062',
}


def body(text, name):
    """Extract exact braced source after an unambiguous tab-delimited name."""
    needle = '\t' + name + '\t'
    if text.count(needle) != 1:
        raise ValueError(f'Expected one source declaration: {name}')
    start = text.index('{', text.index(needle))
    depth, quoted, escaped, line, block = 0, False, False, False, False
    i = start
    while i < len(text):
        ch, nxt = text[i], text[i + 1:i + 2]
        if line:
            if ch == '\n': line = False
        elif block:
            if ch == '*' and nxt == '/': block = False; i += 1
        elif quoted:
            if escaped: escaped = False
            elif ch == '\\': escaped = True
            elif ch == '"': quoted = False
        elif ch == '/' and nxt == '/': line = True; i += 1
        elif ch == '/' and nxt == '*': block = True; i += 1
        elif ch == '"': quoted = True
        elif ch == '{': depth += 1
        elif ch == '}':
            depth -= 1
            if depth == 0: return text[start:i + 1]
        i += 1
    raise ValueError(f'Unbalanced source: {name}')


def yaml_graph(reader, root, header, key):
    files, edges, rows, active = {}, [], [], set()

    def visit(path):
        resolved = reader.path(path)
        if resolved in active: raise ValueError(f'Database cycle: {path}')
        active.add(resolved)
        text = reader.text(path)
        files[path] = base.digest(text)
        data = base._yaml_load(text, path)
        if data.get('Header') != header: raise ValueError(f'Wrong header: {path}')
        entries = data.get('Body') or []
        if not isinstance(entries, list) or any(not isinstance(r, dict) or key not in r for r in entries):
            raise ValueError(f'Invalid rows: {path}')
        rows.extend(copy.deepcopy(entries))
        imports = data.get('Footer', {}).get('Imports', [])
        if not isinstance(imports, list): raise ValueError(f'Invalid imports: {path}')
        for index, entry in enumerate(imports):
            if not isinstance(entry, dict) or set(entry) - {'Path', 'Mode'}:
                raise ValueError(f'Unsupported import: {path}')
            target, mode = entry.get('Path'), entry.get('Mode', 'Renewal')
            reader.path(target)
            if mode not in ('Renewal', 'Prerenewal'): raise ValueError('Unknown mode')
            edges.append([path, index, target, mode, mode == 'Renewal'])
            if mode == 'Renewal': visit(target)
        active.remove(resolved)

    visit(root)
    return {'files': files, 'graph': edges, 'ordered_records': rows,
            'record_count': len(rows), 'effective_count': len(base.scalar_overlay(rows, key))}


def npc_scope(reader, graph):
    declarations, mentions, dynamic, unitwarps = [], [], [], []
    for path in sorted(graph['scripts']):
        text = reader.text(path)
        for line in text.splitlines():
            code = line.split('//', 1)[0].strip()
            if not code: continue
            if '1@ep21b' in code: mentions.append([path, code])
            match = re.match(r'^1@ep21b,\d+,\d+,\d+\s+script(?:\([^)]*\))?\s+([^\t]+)\t', code)
            if match:
                name = match[1]
                declarations.append([path, code])
                if re.search(r'\b(?:questinfo|questinfo_refresh|showevent)\b', body(text, name)):
                    raise ValueError('Instance quest-info producer requires new review')
            if re.search(r'(?:^duplicate(?:_dynamic)?\b|[=(,]\s*duplicate(?:_dynamic)?\s*\(|\bUNPC_MAPID\b)', code):
                dynamic.append([path, code])
            if re.search(r'\bunitwarp\b', code): unitwarps.append([path, code])
    if NPC not in graph['scripts'] or DAILY not in graph['scripts']:
        raise ValueError('Required encounter/daily helper source disabled')
    if len(declarations) != 14 or {r[0] for r in declarations} != {NPC} or dynamic:
        raise ValueError('Instance NPC membership/relocation changed')
    return {'map': '1@ep21b', 'declarations': declarations, 'mentions': mentions,
            'runtime_npc_producers': dynamic, 'unitwarps': unitwarps,
            'condition_registrations': 0, 'enabled_scripts': graph['script_count'],
            'include_configs': graph['config_count'], 'npc_sha256': base.digest(reader.text(NPC)),
            'daily_body_sha256': base.digest(body(reader.text(DAILY), 'EP21_DailyKey'))}


def collect(reader):
    _, records = base.database_graph(reader)
    items = base.scalar_overlay(records['db/item_db.yml'], 'Id')
    statuses = base.scalar_overlay(records['db/status.yml'], 'Status')
    selected = [items[i] for i in IDS]
    for row in selected:
        if row.get('Type', 'Etc') not in ('Etc', 'Usable') or row.get('Locations') or row.get('Stack'):
            raise ValueError(f'Output stacking/type contract changed: {row["Id"]}')
        if row.get('EquipScript') or row.get('UnEquipScript'):
            raise ValueError('Output equipment callback changed')
        if set(row.get('Flags', {})) - {'BuyingStore', 'DropEffect', 'Container'}:
            raise ValueError('Output behavioral flag changed')
        if row.get('Script') and not (row['Id'] == 103512 and row['Script'].strip() == 'getgroupitem(IG_YOR_CARD_P_BOX);'):
            raise ValueError('Unexpected output script')
    achievements = yaml_graph(reader, 'db/achievement_db.yml', {'Type': 'ACHIEVEMENT_DB', 'Version': 2}, 'Id')
    effective = base.scalar_overlay(achievements['ordered_records'], 'Id')
    achievements['conditions'] = [[i, r.get('Group'), r.get('Condition')] for i, r in sorted(effective.items())
                                  if r.get('Group') in ('Get_Item', 'Goal_Achieve')]
    achievements['levels'] = yaml_graph(reader, 'db/achievement_level_db.yml',
                                        {'Type': 'ACHIEVEMENT_LEVEL_DB', 'Version': 1}, 'Level')
    config_paths = ('conf/script_athena.conf', 'conf/import/script_conf.txt')
    config_files = {p: base.digest(reader.text(p)) for p in config_paths}
    main = reader.text(config_paths[0])
    imported = reader.text(config_paths[1])
    active = lambda t: [line.split('//', 1)[0].strip() for line in t.splitlines()
                        if line.split('//', 1)[0].strip()]
    if active(imported) or active(main).count('import: conf/import/script_conf.txt') != 1:
        raise ValueError('Script configuration import graph changed')
    if not {'check_cmdcount: 655360', 'check_gotocount: 2048'} <= set(active(main)):
        raise ValueError('Actual script execution budgets changed')
    return {'schema': 1, 'script_limits': {'files': config_files, 'commands': 655360, 'jumps': 2048},
            'outputs': {'records': selected, 'stack_maximum': 30000,
            'weight_statuses': [statuses[k] for k in ('Weight50', 'Weight90')]},
            'achievements': achievements, 'npc_scope': npc_scope(reader, base.npc_graph(reader)),
            'live_persisted_text_verified': False, 'deployed_binary_verified': False}


def check_manifest(manifest):
    if manifest.get('schema') != 1 or manifest.get('live_persisted_text_verified') is not False or manifest.get('deployed_binary_verified') is not False:
        raise ValueError('Invalid evidence provenance')
    for section, expected in PINS.items():
        actual = base.digest(base.canonical(manifest.get(section)))
        if actual != expected:
            raise ValueError(f'Final Battle {section} changed; manual review required: {expected} != {actual}')
    return manifest


def validate(root=ROOT, *, profile=None):
    broad = base.validate(root, profile=profile)
    reader = base.Reader(root)
    result = check_manifest(collect(reader))
    expected = {**broad['databases']['files'], **broad['npcs']['configs'], **broad['npcs']['scripts'],
                **result['achievements']['files'], **result['achievements']['levels']['files'],
                **result['script_limits']['files']}
    for path, text in reader.cache.items():
        if base.digest(text) != expected.get(path): raise ValueError(f'Source changed during validation: {path}')
    result['broad_manifest_sha256'] = base.digest(base.canonical(broad))
    return result


def negative_controls(reader):
    import yaml
    check_manifest(collect(reader))
    results = []

    def reject(label, changes):
        modified = base.Reader(reader.root, changes)
        modified.cache, modified.paths = reader.cache, reader.paths
        try: check_manifest(collect(modified))
        except (ValueError, KeyError): results.append(label)
        else: raise AssertionError(f'Accepted negative: {label}')

    def row_change(path, key, ident, change):
        data = base._yaml_load(reader.text(path), path)
        change(next(row for row in data['Body'] if row[key] == ident))
        return {path: yaml.safe_dump(data, sort_keys=False)}

    reject('Get_Item inventory mutator', row_change('db/re/achievement_db.yml', 'Id', 220023,
            lambda r: r.update(Condition='achievement_condition(1); getitem 1001653,1;')))
    reject('Goal_Achieve inventory mutator', row_change('db/re/achievement_db.yml', 'Id', 240001,
            lambda r: r.update(Condition='achievement_condition(1); delitem 1001653,1;')))
    reject('missing achievement import', {'db/import/achievement_db.yml': None})
    reject('new achievement import', {'db/achievement_db.yml': reader.text('db/achievement_db.yml') +
           '  - Path: db/import/finalbattle_negative.yml\n', 'db/import/finalbattle_negative.yml':
           'Header: {Type: ACHIEVEMENT_DB, Version: 2}\nBody: []\n'})
    reject('achievement level import', {'db/import/achievement_level_db.yml': None})
    for label, change in [('output GUID', lambda r: r.setdefault('Flags', {}).update(UniqueId=True)),
                          ('output stack cap', lambda r: r.update(Stack={'Amount': 3, 'Inventory': True})),
                          ('output callback', lambda r: r.update(EquipScript='getitem 1001653,1;'))]:
        reject(label, row_change('db/re/item_db_etc.yml', 'Id', 1001653, change))
    reject('new output import', {'db/item_db.yml': reader.text('db/item_db.yml') +
           '  - Path: db/import/finalbattle_item_negative.yml\n', 'db/import/finalbattle_item_negative.yml':
           'Header: {Type: ITEM_DB, Version: 3}\nBody:\n  - Id: 1001653\n    Weight: 99\n'})
    reject('Weight50 callback', row_change('db/re/status.yml', 'Status', 'Weight50',
            lambda r: r.update(Script='getitem 1001653,1;')))
    reject('instance quest-info', {NPC: reader.text(NPC).replace("\tif ('fb_stage != 20) end;",
           "\tquestinfo QTYPE_QUEST,QMARK_YELLOW,\"getitem(1001653,1)\";\n\tif ('fb_stage != 20) end;", 1)})
    reject('new enabled instance NPC', {base.NPC_ROOT: reader.text(base.NPC_ROOT) +
           '\nnpc: npc/custom/finalbattle_negative.txt\n', 'npc/custom/finalbattle_negative.txt':
           '1@ep21b,1,1,0\tscript\tInjectedCrystal\t-1,{ end; }\n'})
    reject('roll probability edit', {NPC: reader.text(NPC).replace('26473,14568,9000', '26474,14568,9000', 1)})
    reject('raised script jump budget', {'conf/script_athena.conf': reader.text('conf/script_athena.conf').replace('check_gotocount: 2048', 'check_gotocount: 99999')})
    reject('new script config import', {'conf/import/script_conf.txt': 'import: conf/import/extra_script.txt\n',
                                       'conf/import/extra_script.txt': ''})
    return results


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--root', type=Path, default=ROOT)
    p.add_argument('--profile', choices=sorted(base.PROFILES))
    p.add_argument('--negative-controls', action='store_true')
    p.add_argument('--manifest', action='store_true')
    args = p.parse_args()
    try:
        result = validate(args.root, profile=args.profile)
        if args.negative_controls:
            result = {'rejected': negative_controls(base.Reader(args.root)), 'pins': PINS}
        print(json.dumps(result, indent=2, sort_keys=True) if args.manifest or args.negative_controls else
              'PASS: required broad gate and exact Final Battle outputs/achievement/instance callback closure')
    except (ValueError, OSError, AssertionError, KeyError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
