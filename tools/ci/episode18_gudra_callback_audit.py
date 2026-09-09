#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode18_gudra_callback_audit.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode18_gudra_callback_audit.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Fail-closed source/content inventory for the proposed Gudra transaction proof.

Collection alone is UNVALIDATED. Public acceptance requires reviewed section pins
and the existing broad gate; no automatic rebaseline or runtime installation.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import re

import biosphere_callback_closure_audit as broad
import biosphere_material_callback_audit as shared
from biosphere_document_callback_audit import ordered_graph

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/re/quests/quests_18.txt'
ENGINE = ['src/map/' + x for x in ('pc.cpp', 'pc.hpp', 'script.cpp', 'script.hpp',
    'script_constants.hpp', 'itemdb.cpp', 'quest.cpp', 'quest.hpp', 'achievement.cpp',
    'npc.cpp', 'party.cpp', 'clif.cpp', 'status.cpp')]
ENGINE += ['conf/battle/party.conf', 'conf/script_athena.conf', 'conf/import/script_conf.txt']
# Reviewed 2026-09-06 source/definitions/condition inventory; not a live-state pin.
# The broad gate is a separate mandatory prerequisite and may still be pending.
PINS = {
    'engine': 'f4274cb495653cda49466b1d928021fcd7261a82db887b6bbdca00a87891788e',
    'items': 'ed799c5708d011bd5ab47451048dc63328e051595cfe7fdd7dbafede57b638bd',
    # Reviewed after the Episode20/21 OnInit relocation, the exact Gudra-only
    # transaction transform, and the Family supply marker refresh. All
    # wolfvill registration statements/owners/conditions remain unchanged.
    'questinfo': 'cc70ca09d9974c9860b3b258be864ce35fcd29989f838d2d3859943357712409',
    'quests': 'da31d7251d156984084c1ba481160a76d24b4353504068ed4b4e66bca6a9c694',
    'reputation': '89f622169a93a24d291b5d7826c95de0c0555c98a7a8d3d64efaee1617098443',
    'constants': 'e7715a04e1af763607c79caec9817d63d5834afe687e456effa254af0c1409f0',
    'achievements': 'c6b595b58b04446b73cab1a871c60b28c54f56bc5fd086cc4fdfee9d31fd8b23',
}


def questinfo(reader, graph):
    rows, owners, declarations, duplicates, sites, unitwarps = [], [], [], [], [], []
    for path in graph['scripts']:
        text = reader.text(path)
        clean, masked = shared.lexical_views(text)
        for token in re.finditer(r'\b(?:questinfo|questinfo_refresh|unitwarp|UNPC_MAPID|duplicate_dynamic)\b', masked):
            start = clean.rfind('\n', 0, token.start()) + 1
            end = clean.find('\n', token.end())
            statement = clean[start:end if end >= 0 else len(clean)].strip()
            record = [path, clean.count('\n', 0, token.start()) + 1, statement]
            if token.group() in ('UNPC_MAPID', 'duplicate_dynamic'):
                raise ValueError('Unreviewed dynamic NPC map/duplicate producer')
            (unitwarps if token.group() == 'unitwarp' else sites).append(record)
        for match in re.finditer(r'^wolfvill,\d+,\d+,\d+\s+[^\n]+', masked, re.M):
            header = clean[match.start():match.end()]
            fields = header.split('\t')
            if len(fields) < 4:
                raise ValueError('Unreviewed wolfvill declaration: ' + header)
            kind, name = fields[1:3]
            entry = {'path': path, 'header': header, 'npc': name,
                     'line': clean.count('\n', 0, match.start()) + 1}
            declarations.append(entry)
            if kind.startswith('duplicate('):
                if kind not in ('duplicate(dummy_npc)', 'duplicate(dummy_cloaked_npc)',
                                'duplicate(#contest1)', 'duplicate(Half Flower#EP18_R01)'):
                    raise ValueError('Unreviewed wolfvill duplicate parent: ' + kind)
                duplicates.append([path, name, kind])
                continue
            if not re.fullmatch(r'script(?:\([A-Z_|]+\))?', kind):
                if kind not in ('warp', 'warp2', 'shop', 'cashshop', 'trader') or '{' in header:
                    raise ValueError('Unreviewed wolfvill NPC type')
                continue
            start = masked.find('{', match.start(), match.end())
            if start < 0:
                raise ValueError('Missing wolfvill NPC body')
            end = shared.body_end(masked, start)
            local = []
            for token in re.finditer(r'\b(?:questinfo|questinfo_refresh)\b', masked[start:end]):
                absolute = start + token.start()
                stop = masked.find(';', absolute, end)
                statement = clean[absolute:stop + 1]
                literal = re.fullmatch(r'questinfo\s*\(\s*(QTYPE_\w+)\s*,\s*(QMARK_\w+)\s*,\s*"([^"\\\n]*)"\s*\)\s*;', statement)
                if literal is None:
                    raise ValueError('Unreviewed wolfvill registration: ' + statement)
                condition = literal[3]
                calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', condition))
                if calls - {'isbegin_quest', 'checkquest', 'countitem'}:
                    raise ValueError('Unreviewed condition call')
                if re.search(r'(?<![=!<>])=(?!=)|\+\+|--|[;{}]', condition):
                    raise ValueError('Condition mutation or unreviewed syntax')
                local.append({'path': path, 'npc': name, 'statement': statement,
                              'condition': condition, 'owner_order': len(local)})
            if local:
                owners.append({**entry, 'body_sha256': broad.digest(text[start:end]),
                               'registration_count': len(local)})
                rows.extend(local)
    if len(rows) != 72 or len(owners) != 35 or len(duplicates) != 44:
        raise ValueError('Exact 72 conditions/35 owners/44 duplicate declarations changed')
    parent_bodies = []
    for path, declaration in ((NPC, 'script\t#contest1\t'),
                              (NPC, 'script\tHalf Flower#EP18_R01\t'),
                              ('npc/re/other/global_npcs.txt', 'script\tdummy_npc\t'),
                              ('npc/re/other/global_npcs.txt', 'script\tdummy_cloaked_npc\t')):
        clean, masked = shared.lexical_views(reader.text(path))
        # Parent declarations may specify a script visibility modifier.
        name = declaration.split('\t')[1]
        found = list(re.finditer(r'^[^\n]*\tscript(?:\([A-Z_|]+\))?\t' + re.escape(name) + r'\t[^\n]*\{', masked, re.M))
        if len(found) != 1:
            raise ValueError('Unique reviewed duplicate parent missing: ' + name)
        start = masked.index('{', found[0].start()); end = shared.body_end(masked, start)
        if re.search(r'\b(?:questinfo|questinfo_refresh)\b', masked[start:end]):
            raise ValueError('Duplicate parent acquired QuestInfo')
        parent_bodies.append([path, name, broad.digest(reader.text(path)[start:end])])
    return {'map': 'wolfvill', 'registrations': rows, 'owners': owners,
            'declarations': declarations, 'duplicates': duplicates, 'parent_bodies': parent_bodies,
            'registration_and_refresh_sites': sites, 'unitwarp_sites': unitwarps,
            'script_count': graph['script_count'], 'config_count': graph['config_count']}


def _collect(reader):
    _, databases = broad.database_graph(reader)
    items = broad.scalar_overlay(databases['db/item_db.yml'], 'Id')
    statuses = broad.scalar_overlay(databases['db/status.yml'], 'Status')
    selected = [items[i] for i in (1000405, 1000408)]
    for row in selected:
        if row.get('Type') != 'Etc' or row.get('Weight', 0) != 0 or row.get('Buy', 0) or row.get('Sell', 0):
            raise ValueError('Gudra zero-weight/sell Etc premise changed')
        if any(row.get(f) for f in broad.SCRIPT_FIELDS) or row.get('Locations') or row.get('Stack') or set(row.get('Flags', {})) - {'BuyingStore'}:
            raise ValueError('Gudra item callback/stack/behavior changed')
    weight_statuses = [statuses[n] for n in ('Weight50', 'Weight90')]
    for row in weight_statuses:
        if row.get('Script') or row.get('CalcFlags') or set(row.get('Flags', {})) & {'OnTouch', 'UnitMove'}:
            raise ValueError('Weight status callback changed')
    qi = questinfo(reader, broad.npc_graph(reader))
    quests, quest_rows = shared.ordered_database(reader, 'db/quest_db.yml', 'QUEST_DB', 3)
    ids = set(range(16551, 16560)) | {18082}
    ids |= {int(x) for row in qi['registrations'] for x in re.findall(r'\b(?:checkquest|isbegin_quest)\(\s*(\d+)', row['condition'])}
    quests['referenced_ordered_records'] = [row for row in quest_rows if row['Id'] in ids]
    if {r['Id'] for r in quests['referenced_ordered_records']} != ids:
        raise ValueError('Referenced quest missing')
    quests['referenced_ids'] = sorted(ids)
    rep = ordered_graph(reader, 'db/reputation.yml', 'REPUTATION_DB', 'Id')
    record = broad.scalar_overlay(rep['ordered_records'], 'Id')[3]
    if record.get('Variable') != 'RepPointsWolf' or record.get('Minimum') != -5000 or record.get('Maximum') != 5000:
        raise ValueError('Reputation3 contract changed')
    rep['selected_record'] = record
    constants = ordered_graph(reader, 'db/const.yml', 'CONSTANT_DB', 'Name')
    if [r for r in constants['ordered_records'] if r['Name'] == 'REPUTATION_EP18'] != [{'Name': 'REPUTATION_EP18', 'Value': 3}]:
        raise ValueError('Exact unique reputation binding changed')
    ach, rows = shared.ordered_database(reader, 'db/achievement_db.yml', 'ACHIEVEMENT_DB', 2)
    effective = broad.scalar_overlay(rows, 'Id')
    ach['effective_count'] = len(effective)
    ach['get_item_records'] = [r for _, r in sorted(effective.items()) if r.get('Group') == 'Get_Item']
    if [r['Id'] for r in ach['get_item_records']] != list(range(220023, 220030)):
        raise ValueError('Seven original item achievements required')
    for row, threshold in zip(ach['get_item_records'], (100, 1000, 5000, 10000, 50000, 100000, 150000)):
        if row.get('Condition') != f' ARG0 >= {threshold} ':
            raise ValueError('Item achievement condition changed')
    return {'schema': 1, 'acceptance': 'UNVALIDATED_COLLECTION',
            'engine': {p: broad.digest(reader.text(p)) for p in ENGINE},
            'items': {'records': selected, 'weight_statuses': weight_statuses},
            'questinfo': qi, 'quests': quests, 'reputation': rep, 'constants': constants,
            'achievements': ach, 'runtime_mutated': False, 'live_state_verified': False}


SECTIONS = ('engine', 'items', 'questinfo', 'quests', 'reputation', 'constants', 'achievements')


def _check(manifest):
    if set(PINS) != set(SECTIONS):
        raise ValueError('Gudra scope has not yet received reviewed section pins; collection is unvalidated')
    for section in SECTIONS:
        actual = broad.digest(broad.canonical(manifest[section]))
        if actual != PINS[section]:
            raise ValueError(f'Gudra {section} drift; manual review required: {actual}')
    return manifest


def collect_evidence(root=ROOT):
    return _collect(broad.Reader(Path(root)))


def validate(root=ROOT, *, profile=None):
    root = Path(root)
    before = broad.validate(root, profile=profile)
    result = _check(collect_evidence(root))
    if broad.validate(root, profile=profile) != before:
        raise ValueError('Broad content/source changed during scoped collection')
    result['acceptance'] = 'REVIEWED_SOURCE_CONSISTENCY'
    result['broad_manifest_sha256'] = broad.digest(broad.canonical(before))
    return result


def negative_controls(root=ROOT):
    reader = broad.Reader(Path(root))
    baseline = _check(_collect(reader))
    results = []
    def reject(name, changes):
        injected = broad.Reader(reader.root, changes)
        injected.cache, injected.paths = reader.cache, reader.paths
        try:
            _check(_collect(injected))
        except ValueError:
            results.append(name)
        else:
            raise AssertionError('Negative Gudra source control accepted: ' + name)
    import yaml
    path = 'db/re/item_db_etc.yml'
    data = broad._yaml_load(reader.text(path), path)
    import copy
    changed = copy.deepcopy(data)
    next(r for r in changed['Body'] if r['Id'] == 1000408)['Script'] = 'delitem 1000405,20;'
    reject('Note gained material mutation', {path: yaml.safe_dump(changed, sort_keys=False)})
    changed = copy.deepcopy(data)
    next(r for r in changed['Body'] if r['Id'] == 1000408)['Trade']['NoSell'] = True
    reject('Note economy changed', {path: yaml.safe_dump(changed, sort_keys=False)})
    path = 'db/re/achievement_db.yml'
    changed = copy.deepcopy(broad._yaml_load(reader.text(path), path))
    next(r for r in changed['Body'] if r['Id'] == 220023)['Condition'] = 'achievement_condition(1); delitem 1000408,1;'
    reject('achievement condition mutator', {path: yaml.safe_dump(changed, sort_keys=False)})
    first = baseline['questinfo']['registrations'][0]['statement']
    reject('existing wolfvill condition mutator', {NPC: reader.text(NPC).replace(first, first.replace('isbegin_quest(11724) == 1', 'Zeny=0'), 1)})
    extra = '\nwolfvill,1,1,0\tscript\tGudraNegative\t-1,{ end; OnInit: questinfo(QTYPE_QUEST,QMARK_YELLOW,"countitem(1000408)>0"); end; }\n'
    reject('new wolfvill registration', {NPC: reader.text(NPC) + extra})
    parent = 'npc/re/other/global_npcs.txt'
    reject('duplicate parent gained registration', {parent: reader.text(parent).replace('\tend;', '\tquestinfo(QTYPE_QUEST,QMARK_YELLOW,"true"); end;', 1)})
    root_conf = 'npc/re/scripts_main.conf'
    reject('new enabled script include', {root_conf: reader.text(root_conf) + '\nnpc: npc/custom/gudra_negative.txt\n',
            'npc/custom/gudra_negative.txt': '-\tscript\tGudraEmptyNegative\t-1,{end;}\n'})
    path = 'db/achievement_db.yml'
    reject('added achievement import', {path: reader.text(path) + '  - Path: db/import/gudra_negative.yml\n',
            'db/import/gudra_negative.yml': 'Header:\n  Type: ACHIEVEMENT_DB\n  Version: 2\nBody: []\n'})
    reject('native stack matcher changed', {'src/map/pc.cpp': reader.text('src/map/pc.cpp').replace('sd->inventory.u.items_inventory[i].bound == item->bound', 'true /* changed compatibility */', 1)})
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--collect', type=Path, help='Emit explicitly UNVALIDATED evidence; never accepts new hashes')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    result = collect_evidence(args.root) if args.collect else validate(args.root)
    if args.collect:
        args.collect.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
        print('GUDRA_UNVALIDATED_COLLECTION: ' + json.dumps({s: broad.digest(broad.canonical(result[s])) for s in SECTIONS}))
    else:
        print('GUDRA_CALLBACK_GATE_OK: reviewed source consistency; no live-state claim')
    if args.negative_controls:
        print('GUDRA_CALLBACK_NEGATIVES_OK: ' + json.dumps(negative_controls(args.root)))
