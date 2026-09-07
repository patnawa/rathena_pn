#!/usr/bin/env python3
"""Reviewed-content consistency gate for Omega/Ellie material callbacks on ba_in01.

Not a script sandbox, native transaction test, or live-world/client attestation.
Acceptance always requires the independent broad callback gate first. Collection
without that gate is expressly unvalidated evidence and cannot return PASS.
"""
from __future__ import annotations

import argparse
import copy
from functools import lru_cache
import json
from pathlib import Path
import re
import sys

import biosphere_callback_closure_audit as base

ROOT = Path(__file__).resolve().parents[2]
FRAGMENTS = [1000636, 1000637, 1000638, 1000639, 1001181, 1001179, 1001177]
RUNES = [1000640, 1000641, 1000642, 1000643, 1001182, 1001180, 1001178]
ESSENCES = [1001138, 1001139, 1001140, 1001141, 1001185, 1001184, 1001183]
WATERS = [1001186, 1001189, 1001188]
IDS = sorted(set(FRAGMENTS + RUNES + ESSENCES + WATERS + list(range(1001290, 1001322))))
WEIGHT_ONE = set(FRAGMENTS + list(range(1001290, 1001306)))
ENGINE = ['src/map/' + name + '.cpp' for name in
          ('pc', 'script', 'itemdb', 'quest', 'achievement', 'clif', 'status')]
QI_FILES = ('npc/re/quests/quests_17_2.txt', 'npc/custom/episode19/quests_19.txt')
PINS = {
    # Manual 2026-09-06 review; see the adjacent audit document. No rebaseline CLI.
    'engine': '6b1fbe30732d98f9a2ee64e97a16762337be3dd0887a76aafc7617e7c98dfac6',
    'materials': '32da27a2a0b00927814d3c9dacc72bad5c91c8c66caf210caa4bb3a1f37455b4',
    'achievements': '743058032b0e7c9c6940949bf7491f11c862d542f0a8c6507483d4e7a06a9e6c',
    'questinfo': '8b51fa5bd603905182137c4982904fab5a8e7d0cfe87bca72e6bb832151baa3d',
    'quests': '169ab22f64ae4a48e73d713310eef65b2d3da3dbb725c6567db33b654c4aac17',
    'npc_scope': '7ab8a55ccd074c7e03d80f1351115b34c8e00fad29f5092dc9f7607c700c7be2',
}


def recipes():
    result = []
    for fragment, rune, essence in zip(FRAGMENTS, RUNES, ESSENCES):
        result.extend([{'inputs': [[fragment, 10]], 'output': rune, 'zeny': 20000},
                       {'inputs': [[rune, 5]], 'output': essence, 'zeny': 20000}])
    for special, water in zip(ESSENCES[4:], WATERS):
        result.append({'inputs': [[ident, 5] for ident in ESSENCES[:4] + [special]],
                       'output': water, 'zeny': 20000})
    for element in range(8):
        for stage, count, zeny in ((0, 10, 30000), (1, 5, 50000), (2, 10, 100000)):
            ident = 1001290 + element + 8 * stage
            result.append({'inputs': [[ident, count]], 'output': ident + 8, 'zeny': zeny})
    return result


@lru_cache(maxsize=1200)
def lexical_views(text):
    """Position-preserving comment-free and comment/string-free source views.

    This scanner identifies literal registration sites, not script semantics.
    Unknown/unterminated syntax fails closed; source hashes carry manual review.
    """
    clean, masked = list(text), list(text)
    index = 0
    while index < len(text):
        if text.startswith('//', index):
            end = text.find('\n', index)
            if end < 0:
                end = len(text)
            for pos in range(index, end):
                clean[pos] = masked[pos] = ' '
            index = end
        elif text.startswith('/*', index):
            end = text.find('*/', index + 2)
            if end < 0:
                raise ValueError('Unterminated NPC block comment')
            end += 2
            for pos in range(index, end):
                if text[pos] != '\n':
                    clean[pos] = masked[pos] = ' '
            index = end
        elif text[index] == '"':
            end = index + 1
            while end < len(text):
                if text[end] == '\\':
                    end += 2
                elif text[end] == '"':
                    end += 1
                    break
                else:
                    end += 1
            else:
                raise ValueError('Unterminated NPC string')
            for pos in range(index, end):
                if text[pos] != '\n':
                    masked[pos] = ' '
            index = end
        else:
            index += 1
    return ''.join(clean), ''.join(masked)


def body_end(masked, start):
    depth = 0
    for pos in range(start, len(masked)):
        if masked[pos] == '{':
            depth += 1
        elif masked[pos] == '}':
            depth -= 1
            if depth == 0:
                return pos + 1
    raise ValueError('Unterminated NPC body')


def ordered_database(reader, root, db_type, version):
    """Exact ordered selected import graph; unique scoped rows checked by caller."""
    files, graph, rows, active = {}, [], [], set()

    def visit(path):
        resolved = reader.path(path)
        if resolved in active:
            raise ValueError(f'{db_type} import cycle: {path}')
        active.add(resolved)
        text = reader.text(path)
        files[path] = base.digest(text)
        data = base._yaml_load(text, path)
        if set(data) - {'Header', 'Body', 'Footer'} or data.get('Header') != {'Type': db_type, 'Version': version}:
            raise ValueError(f'Unsupported {db_type} schema: {path}')
        body = data.get('Body') or []
        if not isinstance(body, list) or any(not isinstance(row, dict) or type(row.get('Id')) is not int for row in body):
            raise ValueError(f'Invalid {db_type} body: {path}')
        rows.extend(copy.deepcopy(body))
        footer = data.get('Footer') or {}
        if not isinstance(footer, dict) or set(footer) - {'Imports'}:
            raise ValueError(f'Unsupported {db_type} footer: {path}')
        imports = footer.get('Imports') or []
        if not isinstance(imports, list):
            raise ValueError(f'Invalid {db_type} import list: {path}')
        for index, entry in enumerate(imports):
            if not isinstance(entry, dict) or set(entry) - {'Path', 'Mode'}:
                raise ValueError(f'Invalid {db_type} import: {path}')
            target, mode = entry.get('Path'), entry.get('Mode', 'Renewal')
            reader.path(target)
            if mode not in ('Renewal', 'Prerenewal'):
                raise ValueError(f'Invalid {db_type} import mode: {mode}')
            selected = mode == 'Renewal'
            graph.append([path, index, target, mode, selected])
            if selected:
                visit(target)
        active.remove(resolved)

    visit(root)
    return {'root': root, 'files': files, 'graph': graph,
            'ordered_record_count': len(rows),
            'ordered_records_sha256': base.digest(base.canonical(rows))}, rows


def questinfo_inventory(reader, graph):
    declarations, registrations, owners, duplicates = [], [], [], []
    global_sites, unitwarps, dynamic = [], [], []
    # dict insertion order follows the enabled import walk, then source order.
    for path in graph['scripts']:
        text = reader.text(path)
        clean, masked = lexical_views(text)
        for token in re.finditer(r'\b(?:questinfo|questinfo_refresh|showevent|unitwarp|UNPC_MAPID|duplicate_dynamic|duplicate)\b', masked):
            line_start = clean.rfind('\n', 0, token.start()) + 1
            line_end = clean.find('\n', token.end())
            if line_end < 0:
                line_end = len(clean)
            statement = clean[line_start:line_end].strip()
            entry = [path, clean.count('\n', 0, token.start()) + 1, statement]
            name = token.group()
            if name in ('questinfo', 'questinfo_refresh', 'showevent'):
                global_sites.append(entry)
            elif name == 'unitwarp':
                unitwarps.append(entry)
            elif name != 'duplicate' or not re.match(r'^\S+\s+duplicate\(', statement):
                dynamic.append(entry)
        for match in re.finditer(r'^ba_in01,\d+,\d+,\d+\s+[^\n]+', masked, re.M):
            header = clean[match.start():match.end()].rstrip()
            fields = header.split('\t')
            if len(fields) < 4:
                raise ValueError(f'Unsupported ba_in01 declaration: {path}: {header}')
            kind, name = fields[1:3]
            entry = {'path': path, 'line': clean.count('\n', 0, match.start()) + 1,
                     'header': header, 'npc': name, 'kind': kind}
            declarations.append(entry)
            if kind.startswith('duplicate('):
                parent = re.fullmatch(r'duplicate\((dummy_npc|dummy_cloaked_npc)\)', kind)
                if not parent:
                    raise ValueError(f'Unreviewed ba_in01 duplicate parent: {kind}')
                duplicates.append([name, parent[1]])
                continue
            if not re.fullmatch(r'script(?:\([A-Z_|]+\))?', kind):
                # A warp/shop cannot carry a script body or QuestInfo list.
                if kind not in ('warp', 'warp2', 'shop', 'cashshop', 'trader') or '{' in header:
                    raise ValueError(f'Unreviewed ba_in01 NPC type: {kind}')
                continue
            start = masked.find('{', match.start(), match.end())
            if start < 0:
                raise ValueError(f'Missing ba_in01 script body: {name}')
            end = body_end(masked, start)
            body = clean[start:end]
            masked_body = masked[start:end]
            local = []
            for token in re.finditer(r'\b(?:questinfo|questinfo_refresh|showevent)\b', masked_body):
                absolute = start + token.start()
                statement_end = masked.find(';', absolute, end)
                if statement_end < 0:
                    raise ValueError(f'Unterminated map registration: {name}')
                statement = clean[absolute:statement_end + 1]
                literal = re.fullmatch(r'questinfo\s*\(\s*(QTYPE_\w+)\s*,\s*(QMARK_\w+)\s*,\s*"([^"\\\n]*)"\s*\)\s*;', statement)
                if not literal:
                    raise ValueError(f'Unreviewed map registration syntax: {name}: {statement}')
                icon, color, condition = literal.groups()
                calls = set(re.findall(r'\b([A-Za-z_]\w*)\s*\(', condition))
                if calls - {'isbegin_quest', 'checkquest', 'countitem'}:
                    raise ValueError(f'Unreviewed QuestInfo condition calls: {name}')
                row = {'path': path, 'line': clean.count('\n', 0, absolute) + 1,
                       'npc': name, 'header': header, 'icon': icon, 'color': color,
                       'condition': condition, 'statement': statement, 'owner_order': len(local)}
                local.append(row)
                registrations.append(row)
            if local:
                # Whole owner body pins control flow around OnInit registrations.
                owners.append({**entry, 'body_sha256': base.digest(text[start:end]),
                               'registration_count': len(local)})
    if dynamic:
        raise ValueError(f'Runtime NPC duplication/map relocation requires review: {dynamic[:2]}')
    if len(registrations) != 33 or len(owners) != 19 or {row['path'] for row in owners} != set(QI_FILES):
        raise ValueError(f'Exact 33 registrations/19 owners changed: {len(registrations)}/{len(owners)}')
    dummy_path = 'npc/re/other/global_npcs.txt'
    if dummy_path not in graph['scripts']:
        raise ValueError('Required duplicate parent source is disabled')
    dummy_clean, dummy_masked = lexical_views(reader.text(dummy_path))
    parents = []
    for name in ('dummy_npc', 'dummy_cloaked_npc'):
        match = re.search(r'^-\tscript(?:\([A-Z]+\))?\t' + name + r'\t[^\n]*\{', dummy_masked, re.M)
        if not match:
            raise ValueError(f'Missing duplicate parent: {name}')
        start = dummy_masked.index('{', match.start())
        end = body_end(dummy_masked, start)
        body = dummy_clean[start:end]
        if re.sub(r'\s+', '', body) != '{end;}':
            raise ValueError(f'Duplicate parent acquired callbacks: {name}')
        parents.append([dummy_path, name, body])
    qi = {'map': 'ba_in01', 'registrations': registrations, 'owners': owners,
          'registration_count': len(registrations), 'owner_count': len(owners)}
    scope = {'declarations': declarations, 'duplicates': duplicates, 'duplicate_parents': parents,
             'registration_sites_all_enabled_scripts': global_sites,
             'unitwarp_statements': unitwarps, 'runtime_duplicate_or_map_setters': dynamic,
             'enabled_import_graph': graph['graph'], 'config_files': graph['configs'],
             'enabled_script_count': graph['script_count'], 'include_config_count': graph['config_count']}
    return qi, scope


def _collect(reader):
    _, records = base.database_graph(reader)
    items = base.scalar_overlay(records['db/item_db.yml'], 'Id')
    statuses = base.scalar_overlay(records['db/status.yml'], 'Status')
    if len(IDS) != 56 or len(WEIGHT_ONE) != 23 or not set(IDS) <= items.keys():
        raise ValueError('Exact 56 material identities are required')
    selected = [items[ident] for ident in IDS]
    for row in selected:
        ident = row['Id']
        if row.get('Type') != 'Etc' or row.get('Weight') != (1 if ident in WEIGHT_ONE else 10):
            raise ValueError(f'Material type/weight changed: {ident}')
        if any(row.get(field) for field in base.SCRIPT_FIELDS) or row.get('Locations') or row.get('Stack'):
            raise ValueError(f'Material scripts/locations/stack changed: {ident}')
        if row.get('Buy', 0) or row.get('Sell', 0) or set(row.get('Flags', {})) - {'BuyingStore', 'DropEffect'}:
            raise ValueError(f'Material value/behavioral flags changed: {ident}')
    weight_statuses = [statuses[name] for name in ('Weight50', 'Weight90')]
    for row in weight_statuses:
        if row.get('Script') or row.get('CalcFlags') or set(row.get('Flags', {})) & {'OnTouch', 'UnitMove'}:
            raise ValueError(f'Weight notification callback changed: {row["Status"]}')
    qi, scope = questinfo_inventory(reader, base.npc_graph(reader))
    achievements, achievement_rows = ordered_database(reader, 'db/achievement_db.yml', 'ACHIEVEMENT_DB', 2)
    effective = base.scalar_overlay(achievement_rows, 'Id')
    get_item = [row for ident, row in sorted(effective.items()) if row.get('Group') == 'Get_Item']
    if [row['Id'] for row in get_item] != list(range(220023, 220030)):
        raise ValueError('Exact seven Get_Item achievements required')
    for row, threshold in zip(get_item, (100, 1000, 5000, 10000, 50000, 100000, 150000)):
        if row.get('Condition') != f' ARG0 >= {threshold} ':
            raise ValueError(f'Get_Item condition changed: {row["Id"]}')
    achievements.update({'effective_record_count': len(effective), 'get_item_records': get_item,
                         'goal_achieve_records': [row for _, row in sorted(effective.items()) if row.get('Group') == 'Goal_Achieve']})
    referenced = sorted({int(value) for row in qi['registrations'] for value in
                         re.findall(r'\b(?:checkquest|isbegin_quest)\(\s*(\d+)', row['condition'])})
    quests, quest_rows = ordered_database(reader, 'db/quest_db.yml', 'QUEST_DB', 3)
    # Do not pretend scalar_overlay emulates nested native quest-objective merge.
    # The only current repeated scoped rows are title-only 18119/18120 overrides.
    # Return ordered rows too, so a fixture can invoke native parseBodyNode on all.
    selected_quests = [row for row in quest_rows if row['Id'] in referenced]
    if sorted({row['Id'] for row in selected_quests}) != referenced:
        raise ValueError('Missing referenced quest')
    for ident in referenced:
        occurrences = [row for row in selected_quests if row['Id'] == ident]
        if len(occurrences) != 1 and not (ident in (18119, 18120) and len(occurrences) == 2
                                        and all(set(row) == {'Id', 'Title'} for row in occurrences)):
            raise ValueError(f'Overlaid referenced quest requires native merge review: {ident}')
    quests['referenced_ordered_records'] = selected_quests
    quests['referenced_records'] = list(base.scalar_overlay(selected_quests, 'Id').values())
    return {'schema': 1, 'acceptance': 'UNVALIDATED_COLLECTION',
            'engine': {'files': {path: base.digest(reader.text(path)) for path in ENGINE}},
            'materials': {'records': selected, 'recipes': recipes(), 'weight_counts': {'1': 23, '10': 33},
                          'weight_notification_statuses': weight_statuses,
                          'output_maximum': 30000, 'payment_chunk_maximum': 30000},
            'achievements': achievements, 'questinfo': qi, 'quests': quests, 'npc_scope': scope,
            'live_world_verified': False, 'deployed_binary_verified': False, 'graphical_client_verified': False}


def _check(manifest):
    fields = {'schema', 'acceptance', 'live_world_verified', 'deployed_binary_verified', 'graphical_client_verified'} | set(PINS)
    if set(manifest) != fields or manifest.get('schema') != 1 or manifest.get('acceptance') != 'UNVALIDATED_COLLECTION' or any(
            manifest.get(name) is not False for name in ('live_world_verified', 'deployed_binary_verified', 'graphical_client_verified')):
        raise ValueError('Invalid static material evidence schema/provenance')
    for section, expected in PINS.items():
        actual = base.digest(base.canonical(manifest.get(section)))
        if actual != expected:
            raise ValueError(f'Material callback {section} changed; manual review required: expected {expected}, actual {actual}')
    return manifest


def collect_evidence(root=ROOT):
    """Unvalidated source inventory only; no broad gate, pins, or acceptance claim."""
    return _collect(base.Reader(root))


def validate(root=ROOT, *, profile=None):
    """Mandatory broad validation followed by independently reviewed scoped pins."""
    broad = base.validate(root, profile=profile)
    reader = base.Reader(root)
    result = _check(_collect(reader))
    expected = {**broad['databases']['files'], **broad['npcs']['configs'], **broad['npcs']['scripts'],
                **broad['engine']['files'], **result['achievements']['files'], **result['quests']['files']}
    for path, text in reader.cache.items():
        if base.digest(text) != expected.get(path):
            raise ValueError(f'Source changed between broad/scoped reads: {path}')
    # The additional ordered achievement/quest trees are reread too: they are
    # outside the broad gate, and cannot silently change during collection.
    fresh = base.Reader(root)
    for section in ('engine', 'achievements', 'quests'):
        for path, expected_hash in result[section]['files'].items():
            if base.digest(fresh.text(path)) != expected_hash:
                raise ValueError(f'Scoped source changed during validation: {path}')
    base.verify_manifest(root, broad, profile=profile)
    result['broad_manifest_sha256'] = base.digest(base.canonical(broad))
    result['profile'] = profile
    result['acceptance'] = 'REVIEWED_CONTENT_MATCH'
    return result


def negative_controls(root=ROOT, *, profile=None):
    """Reject deliberate in-memory changes; never modify reviewed source files."""
    accepted = validate(root, profile=profile)  # Deliberately unconditional.
    reader = base.Reader(root)
    baseline = _check(_collect(reader))
    results = []

    def reject(label, changes):
        injected = base.Reader(root, changes)
        injected.cache, injected.paths = reader.cache, reader.paths
        try:
            _check(_collect(injected))
        except ValueError:
            results.append(label)
        else:
            raise AssertionError(f'Negative material callback control accepted: {label}')

    import yaml

    def changed_row(path, key, ident, change):
        data = base._yaml_load(reader.text(path), path)
        row = next(row for row in data['Body'] if row.get(key) == ident)
        change(row)
        return {path: yaml.safe_dump(data, sort_keys=False)}

    def item_change(ident, change):
        paths = [path for path in reader.cache if path.endswith('.yml')
                 and re.search(r'\n  - Id: ' + str(ident) + r'\b', reader.text(path))]
        if len(paths) != 1:
            raise AssertionError(f'Negative-control material definition not unique: {ident}')
        return changed_row(paths[0], 'Id', ident, change)

    pc = 'src/map/pc.cpp'
    reject('native inventory callback removed', {
        pc: reader.text(pc).replace('\tpc_show_questinfo(sd);', '\t/* injected removal */', 1)})
    reject('native-weight-one material changed', item_change(1001290, lambda row: row.update(Weight=10)))
    reject('native-weight-ten material changed', item_change(1001314, lambda row: row.update(Weight=1)))
    reject('material script added', item_change(1001314, lambda row: row.update(Script='Zeny = 0;')))
    reject('output GUID flag added', item_change(1001314, lambda row: row.setdefault('Flags', {}).update(UniqueId=True)))
    reject('output sell value changed', item_change(1001314, lambda row: row.update(Sell=100)))
    reject('weight-status script added', changed_row('db/re/status.yml', 'Status', 'Weight50', lambda row: row.update(Script='Zeny = 0;')))
    first = baseline['questinfo']['registrations'][0]
    quest_path = first['path']
    quest_source = reader.text(quest_path)
    reject('literal QuestInfo registration removed', {quest_path: quest_source.replace(first['statement'], ';', 1)})
    reject('QuestInfo expression assigned Zeny', {
        quest_path: quest_source.replace(first['statement'], first['statement'].replace(first['condition'], '(Zeny = 0)'), 1)})
    reject('QuestInfo registration conditionally skipped', {
        quest_path: quest_source.replace(first['statement'], 'if (0) ' + first['statement'], 1)})
    reject('nonliteral QuestInfo condition', {
        quest_path: quest_source.replace(first['statement'], first['statement'].replace('"' + first['condition'] + '"', '.unreviewed$'), 1)})
    rookie = [row for row in baseline['questinfo']['registrations'] if row['npc'] == 'Rookie#jh5']
    if len(rookie) != 2:
        raise AssertionError('Expected two Rookie registrations')
    swapped = quest_source.replace(rookie[0]['statement'], '__MATERIAL_QI_SWAP__', 1)
    swapped = swapped.replace(rookie[1]['statement'], rookie[0]['statement'], 1).replace('__MATERIAL_QI_SWAP__', rookie[1]['statement'], 1)
    reject('QuestInfo registration order swapped', {quest_path: swapped})
    dummy = 'npc/re/other/global_npcs.txt'
    reject('duplicate parent gained registration', {
        dummy: reader.text(dummy).replace('\tend;', '\tquestinfo(QTYPE_QUEST,QMARK_YELLOW,"Zeny=0"); end;', 1)})
    reject('runtime NPC map relocation added', {
        quest_path: quest_source + '\nfunction\tscript\tMaterialNegative\t{ setunitdata 1,UNPC_MAPID,1; end; }\n'})
    reject('new enabled QuestInfo NPC', {
        base.NPC_ROOT: reader.text(base.NPC_ROOT) + '\nnpc: npc/custom/material_negative.txt\n',
        'npc/custom/material_negative.txt': 'ba_in01,1,1,0\tscript\tMaterialNegative\t-1,{ end; OnInit: questinfo(QTYPE_QUEST,QMARK_YELLOW,"Zeny=0"); end; }\n'})
    achievement = 'db/re/achievement_db.yml'
    reject('Get_Item condition mutates payment', changed_row(achievement, 'Id', 220023, lambda row: row.update(Condition='achievement_condition(1); Zeny = 0;')))
    reject('Goal_Achieve condition mutates payment', changed_row(achievement, 'Id', baseline['achievements']['goal_achieve_records'][0]['Id'], lambda row: row.update(Condition='Zeny = 0;')))
    reject('referenced hunting objective changed', changed_row('db/re/quest_db.yml', 'Id', 5893, lambda row: row['Targets'][0].update(Count=1)))
    reject('missing selected achievement import', {'db/import/achievement_db.yml': None})
    ach_root = 'db/achievement_db.yml'
    reject('added selected achievement import', {
        ach_root: reader.text(ach_root) + '  - Path: db/import/material_negative.yml\n',
        'db/import/material_negative.yml': 'Header:\n  Type: ACHIEVEMENT_DB\n  Version: 2\nBody:\n  - Id: 220023\n    Condition: "Zeny = 0;"\n'})
    reject('achievement import cycle', {ach_root: reader.text(ach_root) + '  - Path: db/achievement_db.yml\n'})
    reject('achievement import escapes root', {ach_root: reader.text(ach_root) + '  - Path: ../outside.yml\n'})
    reject('Renewal achievement import disabled', {
        ach_root: reader.text(ach_root).replace('Mode: Renewal', 'Mode: Prerenewal', 1)})
    reject('new quest import', {
        'db/quest_db.yml': reader.text('db/quest_db.yml') + '  - Path: db/import/material_negative.yml\n',
        'db/import/material_negative.yml': 'Header:\n  Type: QUEST_DB\n  Version: 3\nBody:\n  - Id: 5893\n    Title: Injected\n'})
    tampered = copy.deepcopy(baseline)
    tampered['questinfo']['registrations'][0]['condition'] = 'Zeny=0'
    try:
        _check(tampered)
    except ValueError:
        results.append('tampered collected manifest')
    else:
        raise AssertionError('Tampered manifest accepted')
    tampered = copy.deepcopy(baseline)
    tampered['unreviewed_acceptance'] = True
    try:
        _check(tampered)
    except ValueError:
        results.append('unexpected evidence schema field')
    else:
        raise AssertionError('Unknown evidence schema accepted')

    # An explicit failing broad validator must prevent even collecting scope.
    # This tests ordering, not a replacement acceptance policy.
    from unittest.mock import patch
    for requested in (None, 'live-20260906'):
        with patch.object(base, 'validate', side_effect=ValueError('required broad failure')) as broad_mock:
            with patch.object(sys.modules[__name__], '_collect', side_effect=AssertionError('scope entered before broad acceptance')):
                try:
                    validate(root, profile=requested)
                except ValueError as exc:
                    if str(exc) != 'required broad failure':
                        raise
                else:
                    raise AssertionError('Broad rejection bypassed')
            broad_mock.assert_called_once_with(root, profile=requested)
        results.append(f'mandatory broad gate/profile forwarding: {requested}')
    try:
        validate(root, profile='unreviewed-profile')
    except ValueError as exc:
        if 'Unknown callback closure profile' not in str(exc):
            raise
        results.append('unknown profile rejected')
    else:
        raise AssertionError('Unknown profile accepted')
    if validate(root, profile=profile) != accepted:
        raise AssertionError('Source changed during negative controls')
    return results


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=ROOT)
    parser.add_argument('--profile', choices=sorted(base.PROFILES))
    parser.add_argument('--collect-only', action='store_true')
    parser.add_argument('--manifest', action='store_true')
    parser.add_argument('--negative-controls', action='store_true')
    args = parser.parse_args()
    if args.collect_only and (args.negative_controls or args.profile):
        parser.error('--collect-only cannot claim a profile or run acceptance controls')
    try:
        result = collect_evidence(args.root) if args.collect_only else validate(args.root, profile=args.profile)
        if args.collect_only:
            print('UNVALIDATED COLLECTION: broad gate and reviewed pins were NOT accepted.', file=sys.stderr)
        if args.negative_controls:
            print(json.dumps({'negative_controls_rejected': negative_controls(args.root, profile=args.profile),
                              'pins': PINS, 'profile': args.profile}, sort_keys=True, indent=2))
        elif args.manifest or args.collect_only:
            print(json.dumps(result, sort_keys=True, indent=2))
        else:
            print('PASS: mandatory broad gate and reviewed ba_in01 material callback evidence unchanged')
            print(json.dumps({'pins': PINS, 'broad_manifest_sha256': result['broad_manifest_sha256'], 'profile': result['profile']}, sort_keys=True))
    except (ValueError, OSError, AssertionError) as exc:
        print(f'FAIL: {exc}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
