#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  biosphere_conversion_callback_audit.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/biosphere_conversion_callback_audit.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Fail-closed current-content gate for the five Biosphere conversions.

Requires the broader reviewed source/NPC/item callback gate, then independently
pins achievement imports, all eleven material definitions and scoped map-entry
evidence. Hash equality carries the manual semantic review, NOT regex matching.
No files are written and no live persistence/binary verification is claimed.
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
IDS = tuple(sorted([6607, 6608, 6755, 25866, *range(1001550, 1001557)]))
OUTPUTS = tuple(range(1001552, 1001557))
NPC_FILES = ('npc/custom/varmundt_biosphere.txt', 'npc/custom/varmundt_biosphere_depth.txt')
RECIPES = [
    [[[1001550, 10]], 1001552, 10000],
    [[[1001551, 10]], 1001553, 10000],
    [[[1001552, 10], [1001553, 10]], 1001554, 20000],
    [[[1001554, 5], [6607, 5]], 1001555, 30000],
    [[[1001555, 5], [6608, 5], [6755, 5], [25866, 3]], 1001556, 50000],
]
PINS = {
    'materials': '917a3c4e66bace9f94479cb4cc135e8c3a167d133f19acc0940481f1d4f630ec',
    'achievements': '893b6aa217ba6c43f8375e00f200a3dc2030e6434037dd37aba5f3252f8c3d8e',
    'npc_scope': '9f0f650cbf9e23baf7a25ea6e62d7769f6aba260204b34092ddc4e058bfe5c38',
}


def achievement_graph(reader):
    files, graph, rows, active = {}, [], [], set()

    def visit(path):
        resolved = reader.path(path)
        if resolved in active:
            raise ValueError(f'Achievement import cycle: {path}')
        active.add(resolved)
        text = reader.text(path)
        files[path] = base.digest(text)
        data = base._yaml_load(text, path)
        if data.get('Header') != {'Type': 'ACHIEVEMENT_DB', 'Version': 2}:
            raise ValueError(f'Unsupported achievement header: {path}')
        body = data.get('Body') or []
        if not isinstance(body, list) or any(not isinstance(row, dict) or 'Id' not in row for row in body):
            raise ValueError(f'Invalid achievement body: {path}')
        rows.extend(copy.deepcopy(body))
        imports = data.get('Footer', {}).get('Imports', [])
        if not isinstance(imports, list):
            raise ValueError(f'Invalid achievement imports: {path}')
        for index, entry in enumerate(imports):
            if not isinstance(entry, dict) or set(entry) - {'Path', 'Mode'}:
                raise ValueError(f'Unsupported achievement import: {path}')
            target, mode = entry.get('Path'), entry.get('Mode', 'Renewal')
            reader.path(target)
            if mode not in ('Renewal', 'Prerenewal'):
                raise ValueError(f'Unsupported achievement mode: {mode}')
            selected = mode == 'Renewal'
            graph.append([path, index, target, mode, selected])
            if selected:
                visit(target)
        active.remove(resolved)

    visit('db/achievement_db.yml')
    effective = base.scalar_overlay(rows, 'Id')
    conditions = [[ident, row.get('Group'), row.get('Condition')]
                  for ident, row in sorted(effective.items())
                  if row.get('Group') in ('Get_Item', 'Goal_Achieve')]
    return {'files': files, 'graph': graph, 'ordered_record_count': len(rows),
            'ordered_records_sha256': base.digest(base.canonical(rows)),
            'effective_records': len(effective), 'condition_inventory': conditions}


def npc_scope(reader, graph):
    # These checks give explicit traversal/change evidence. The broad gate pins
    # every enabled NPC byte, so this is NOT permission to run arbitrary source
    # that happens to evade a keyword search or construct a map name dynamically.
    declarations, mentions, dynamic, unitwarps = [], [], [], []
    for path in sorted(graph['scripts']):
        lines = reader.text(path).splitlines()
        for line in lines:
            code = line.split('//', 1)[0].strip()
            if not code:
                continue
            if 'ba_chess' in code:
                mentions.append([path, code])
            if re.match(r'^ba_chess,\d+,\d+,\d+\s+(?:script|warp|duplicate)', code):
                declarations.append([path, code])
            if re.search(r'(?:^duplicate(?:_dynamic)?\b|[=(,]\s*duplicate(?:_dynamic)?\s*\(|\bUNPC_MAPID\b)', code):
                dynamic.append([path, code])
            if re.search(r'\bunitwarp\b', code):
                unitwarps.append([path, code])
        if path in NPC_FILES and re.search(r'\b(?:questinfo|questinfo_refresh|showevent)\b', reader.text(path)):
            raise ValueError(f'Biosphere map condition registration changed: {path}')
    if not set(NPC_FILES) <= graph['scripts'].keys():
        raise ValueError('Required Biosphere map scripts are not enabled')
    if len(declarations) != 4 or {row[0] for row in declarations} != set(NPC_FILES):
        raise ValueError('Reviewed four ba_chess NPC declarations changed')
    if dynamic:
        raise ValueError('Runtime NPC duplication/map relocation requires new review')
    return {'map': 'ba_chess', 'declarations': declarations, 'map_mentions': mentions,
            'condition_registration_files': list(NPC_FILES), 'condition_registrations': 0,
            'runtime_duplicate_or_map_setters': dynamic, 'unitwarp_statements': unitwarps,
            'enabled_script_count': graph['script_count'], 'include_config_count': graph['config_count']}


def _collect(reader):
    _, records = base.database_graph(reader)
    items = base.scalar_overlay(records['db/item_db.yml'], 'Id')
    statuses = base.scalar_overlay(records['db/status.yml'], 'Status')
    weight_statuses = [statuses[name] for name in ('Weight50', 'Weight90')]
    if not set(IDS) <= items.keys():
        raise ValueError('Required conversion material is missing')
    selected = [items[i] for i in IDS]
    for row in selected:
        if row.get('Type') != 'Etc' or row.get('Weight') != 10:
            raise ValueError(f'Conversion material type/weight changed: {row["Id"]}')
        if any(row.get(field) for field in base.SCRIPT_FIELDS) or row.get('Locations') or row.get('Stack'):
            raise ValueError(f'Conversion material callback/stack/location changed: {row["Id"]}')
        if set(row.get('Flags', {})) - {'BuyingStore', 'DropEffect'}:
            raise ValueError(f'Conversion material behavioral flags changed: {row["Id"]}')
    for ident in OUTPUTS:
        if items[ident].get('Buy', 0) or items[ident].get('Sell', 0):
            raise ValueError(f'Output sell-value achievement premise changed: {ident}')
    return {'schema': 1,
            'materials': {'records': selected, 'recipes': RECIPES, 'outputs': list(OUTPUTS),
                          'weight_notification_statuses': weight_statuses,
                          'output_maximum': 30000, 'payment_chunk_maximum': 30000},
            'achievements': achievement_graph(reader),
            'npc_scope': npc_scope(reader, base.npc_graph(reader)),
            'live_persisted_text_verified': False, 'deployed_binary_verified': False}


def _check(manifest):
    if manifest.get('schema') != 1 or manifest.get('live_persisted_text_verified') is not False or manifest.get('deployed_binary_verified') is not False:
        raise ValueError('Invalid static conversion evidence schema/provenance')
    for section, expected in PINS.items():
        actual = base.digest(base.canonical(manifest.get(section)))
        if actual != expected:
            raise ValueError(f'Conversion {section} changed; manual review required: expected {expected}, actual {actual}')
    return manifest


def validate(root=ROOT, *, profile=None):
    """Mandatory broad source gate plus conversion-specific reviewed evidence."""
    broad = base.validate(root, profile=profile)
    reader = base.Reader(root)
    result = _check(_collect(reader))
    expected = {**broad['databases']['files'], **broad['npcs']['configs'],
                **broad['npcs']['scripts'], **result['achievements']['files']}
    for path, text in reader.cache.items():
        if base.digest(text) != expected.get(path):
            raise ValueError(f'Conversion source changed during broad/scoped validation: {path}')
    result['broad_manifest_sha256'] = base.digest(base.canonical(broad))
    return result


def _negative_controls(reader):
    baseline = _check(_collect(reader))
    results = []

    def reject(label, changes):
        injected = base.Reader(reader.root, changes)
        injected.cache, injected.paths = reader.cache, reader.paths
        try:
            _check(_collect(injected))
        except ValueError:
            results.append(label)
        else:
            raise AssertionError(f'Negative conversion control accepted: {label}')

    import yaml
    path = 'db/re/achievement_db.yml'
    data = base._yaml_load(reader.text(path), path)
    next(row for row in data['Body'] if row['Id'] == 220023)['Condition'] = 'achievement_condition(1); delitem 1001555,180;'
    reject('achievement condition material mutator', {path: yaml.safe_dump(data, sort_keys=False)})
    reject('missing imported achievement database', {'db/import/achievement_db.yml': None})
    reject('new achievement import', {
        'db/achievement_db.yml': reader.text('db/achievement_db.yml') + '  - Path: db/import/conversion_negative.yml\n',
        'db/import/conversion_negative.yml': 'Header:\n  Type: ACHIEVEMENT_DB\n  Version: 2\nBody:\n  - Id: 220023\n    Condition: "achievement_condition(1); delitem 1001555,180;"\n',
    })
    npc = 'npc/custom/varmundt_biosphere_depth.txt'
    reject('new ba_chess QuestInfo registration', {
        npc: reader.text(npc) + '\nba_chess,1,1,0\tscript\tInjectedConversion\t-1,{ end; OnInit: questinfo QTYPE_QUEST,QMARK_YELLOW,"delitem(1001555,180)"; end; }\n',
    })
    reject('new enabled ba_chess NPC source', {
        base.NPC_ROOT: reader.text(base.NPC_ROOT) + '\nnpc: npc/custom/conversion_negative.txt\n',
        'npc/custom/conversion_negative.txt': 'ba_chess,1,1,0\tscript\tInjectedConversion\t-1,{ end; }\n',
    })
    material = next(p for p in reader.cache if p.endswith('.yml') and re.search(r'\n  - Id: 1001555\b', reader.text(p)))
    data = base._yaml_load(reader.text(material), material)
    next(row for row in data['Body'] if row['Id'] == 1001555)['Script'] = 'delitem 1001552,1;'
    reject('material callback mutation', {material: yaml.safe_dump(data, sort_keys=False)})
    data = base._yaml_load(reader.text(material), material)
    next(row for row in data['Body'] if row['Id'] == 1001555).setdefault('Flags', {})['UniqueId'] = True
    reject('material GUID flag mutation', {material: yaml.safe_dump(data, sort_keys=False)})
    status_path = 'db/re/status.yml'
    data = base._yaml_load(reader.text(status_path), status_path)
    next(row for row in data['Body'] if row['Status'] == 'Weight50')['Script'] = 'delitem 1001555,180;'
    reject('weight notification status callback mutation', {status_path: yaml.safe_dump(data, sort_keys=False)})
    tampered = copy.deepcopy(baseline)
    tampered['achievements']['condition_inventory'][0][2] = 'achievement_condition(1);'
    try:
        _check(tampered)
    except ValueError:
        results.append('tampered evidence manifest')
    else:
        raise AssertionError('Tampered conversion evidence accepted')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--profile', choices=sorted(base.PROFILES))
    parser.add_argument('--manifest', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    try:
        manifest = validate(args.root, profile=args.profile)
        result = {'negative_controls_rejected': _negative_controls(base.Reader(args.root)), 'pins': PINS} if args.negative_controls else manifest
        if args.manifest or args.negative_controls:
            print(json.dumps(result, sort_keys=True, indent=2))
        else:
            print('PASS: broad source gate and exact conversion callback/material/map evidence unchanged')
            print(json.dumps({'pins': PINS, 'broad_manifest_sha256': manifest['broad_manifest_sha256']}, sort_keys=True))
    except (ValueError, OSError, AssertionError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
