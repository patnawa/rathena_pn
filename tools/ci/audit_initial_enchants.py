#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_initial_enchants.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_initial_enchants.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compare initial enchants, eligibility and resets with explicitly supplied client data.

Reads literal client declarations without executing Lua. This models successful
native database overlays; it is not a client interaction or charging test.
"""
import argparse
import json
import re
from collections import Counter
from pathlib import Path

from audit_enchant_upgrades import ClientItemNames, renewal_records, split_args


def materials_overlay(previous, entries):
    result = dict(previous)
    for entry in entries or []:
        name = entry['Material']
        amount = entry.get('Amount', result.get(name, 1))
        if not 0 <= amount <= 65535:
            raise ValueError(f'Invalid material amount: {entry}')
        if amount:
            result[name] = min(amount, 30000)
        else:
            result.pop(name, None)
    return result


def requirement_overlay(previous, record):
    return {'Price': min(record.get('Price', previous.get('Price', 0)), 2147483647),
            'Materials': materials_overlay(previous.get('Materials', {}), record.get('Materials'))}


def client_requirements(args, offset, resolve):
    result = {'Price': int(args[offset]), 'Materials': {}}
    for arg in args[offset + 1:]:
        match = re.fullmatch(r'\{\s*"([^"\\]+)"\s*,\s*(\d+)\s*\}', arg)
        if not match:
            raise ValueError(f'Unsupported material syntax: {arg}')
        result['Materials'][resolve(match[1])] = int(match[2])
    return result


def blank_slot():
    return {'Price': 0, 'Materials': {}, 'Chance': 100000, 'Bonus': {}, 'Enchants': {}, 'Perfect': {}}


def blank_group():
    return {'Targets': set(), 'Order': [], 'MinimumRefine': 0, 'MinimumEnchantgrade': 0,
            'AllowRandomOptions': True, 'Reset': {'Enabled': False, 'Chance': 0, 'Price': 0, 'Materials': {}},
            'Slots': {}}


def client_configuration(path, resolve):
    groups = {}
    for line in path.read_text(encoding='cp949').splitlines():
        line = line.strip()
        match = re.fullmatch(r'Table\[(\d+)\] = CreateEnchantInfo\(\)', line)
        if match:
            groups[int(match[1])] = blank_group()
            continue
        match = re.fullmatch(r'Table\[(\d+)\](?:\.Slot\[(\d+)\])?:(\w+)\((.*)\)', line)
        if not match:
            if line.startswith('Table['):
                raise ValueError(f'Unsupported client declaration: {line[:120]}')
            continue
        group_id, slot, method, args = match.groups()
        if method in {'SetCaution', 'AddUpgradeEnchant', 'AddPerfectUpgradeEnchant',
                      'SetRandomUpgradeRequire', 'AddRandomUpgradeEnchant'}:
            continue
        group = groups[int(group_id)]
        args = split_args(args)
        if slot is None:
            if method == 'SetSlotOrder':
                group['Order'] = [int(x) for x in args]
            elif method in {'AddTargetItem', 'AddTargetItem_Duplicate'}:
                group['Targets'].add(resolve(json.loads(args[0])))
            elif method == 'SetCondition':
                group['MinimumRefine'], group['MinimumEnchantgrade'] = map(int, args)
            elif method == 'ApproveRandomOption':
                group['AllowRandomOptions'] = json.loads(args[0])
            elif method == 'SetReset':
                group['Reset'] = {'Enabled': json.loads(args[0]), 'Chance': int(args[1]),
                                  **client_requirements(args, 2, resolve)}
            else:
                raise ValueError(f'Unsupported group method: {method}')
            continue
        config = group['Slots'].setdefault(int(slot), blank_slot())
        if method == 'SetRequire':
            config.update(client_requirements(args, 0, resolve))
        elif method == 'SetSuccessRate':
            config['Chance'] = int(args[0])
        elif method == 'SetGradeBonus':
            config['Bonus'][int(args[0])] = int(args[1])
        elif method == 'SetEnchant':
            config['Enchants'].setdefault(int(args[0]), {})[resolve(json.loads(args[1]))] = int(args[2])
        elif method == 'AddPerfectEnchant':
            config['Perfect'][resolve(json.loads(args[0]))] = client_requirements(args, 1, resolve)
        else:
            raise ValueError(f'Unsupported slot method: {method}')
    return groups


def server_configuration(root):
    groups = {}
    for record in renewal_records(root, 'db/item_enchant.yml'):
        group = groups.setdefault(record['Id'], blank_group())
        for name, enabled in record.get('TargetItems', {}).items():
            if enabled:
                group['Targets'].add(name)
            else:
                group['Targets'].discard(name)
        for field in ['MinimumRefine', 'MinimumEnchantgrade', 'AllowRandomOptions']:
            if field in record:
                group[field] = record[field]
        if 'Order' in record:
            group['Order'] = [x['Slot'] for x in record['Order']]
        if 'Reset' in record:
            reset = group['Reset']
            reset.update(requirement_overlay(reset, record['Reset']))
            reset['Chance'] = record['Reset'].get('Chance', reset['Chance'])
            reset['Enabled'] = reset['Chance'] > 0
        for slot in record.get('Slots', []):
            config = group['Slots'].setdefault(slot['Slot'], blank_slot())
            config.update(requirement_overlay(config, slot))
            config['Chance'] = slot.get('Chance', config['Chance'])
            config['Bonus'].update({x['Enchantgrade']: x['Chance'] for x in slot.get('EnchantgradeBonus', [])})
            for grade in slot.get('Enchants', []):
                outcomes = config['Enchants'].setdefault(grade['Enchantgrade'], {})
                for item in grade.get('Items', []):
                    outcomes[item['Item']] = item.get('Chance', outcomes.get(item['Item'], 0))
            for recipe in slot.get('PerfectEnchants', []):
                config['Perfect'][recipe['Item']] = requirement_overlay(config['Perfect'].get(recipe['Item'], {}), recipe)
    return groups


def compare(client, server):
    issues = []

    def check(kind, key, expected, actual):
        if expected != actual:
            issues.append({'kind': kind, 'key': key, 'client': expected, 'server': actual})

    for group_id in sorted(client.keys() - server.keys()):
        group = client[group_id]
        check('missing-server-group', [group_id], {
            'targets': sorted(group['Targets']),
            'normal_grade_tables': sum(len(s['Enchants']) for s in group['Slots'].values()),
            'perfect_initial_recipes': sum(len(s['Perfect']) for s in group['Slots'].values()),
        }, None)
    for group_id in sorted(client.keys() & server.keys()):
        expected, actual = client[group_id], server[group_id]
        check('targets', [group_id], sorted(expected['Targets']), sorted(actual['Targets']))
        for field in ['Order', 'MinimumRefine', 'MinimumEnchantgrade', 'AllowRandomOptions']:
            check(field, [group_id], expected[field], actual[field])
        check('reset-enabled', [group_id], expected['Reset']['Enabled'], actual['Reset']['Enabled'])
        if expected['Reset']['Enabled']:
            check('reset', [group_id], expected['Reset'], actual['Reset'])
        for slot, config in expected['Slots'].items():
            other = actual['Slots'].get(slot, blank_slot())
            if config['Enchants']:
                for field in ['Price', 'Materials', 'Chance']:
                    check('normal-' + field, [group_id, slot], config[field], other[field])
                for grade, outcomes in config['Enchants'].items():
                    check('normal-bonus', [group_id, slot, grade], config['Bonus'].get(grade, 0), other['Bonus'].get(grade, 0))
                    check('normal-outcomes', [group_id, slot, grade],
                          {k: v for k, v in outcomes.items() if v},
                          {k: v for k, v in other['Enchants'].get(grade, {}).items() if v})
            for item, recipe in config['Perfect'].items():
                check('perfect-requirements', [group_id, slot, item], recipe, other['Perfect'].get(item))
    return issues


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client', type=Path)
    parser.add_argument('--client-item-names', type=Path, required=True)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--groups', help='Comma-separated group IDs')
    parser.add_argument('--details', action='store_true')
    parser.add_argument('--emit-probability-fixture', action='store_true', help='Print effective server weights for exhaustive C++ testing')
    args = parser.parse_args()
    resolve = ClientItemNames(args.client_item_names, args.root)
    client, server = client_configuration(args.client, resolve), server_configuration(args.root)
    if args.groups:
        wanted = {int(x) for x in args.groups.split(',')}
        client = {g: c for g, c in client.items() if g in wanted}
        server = {g: c for g, c in server.items() if g in wanted}
    if args.emit_probability_fixture:
        ids = {name: item_id for item_id, name in resolve.server.items()}
        tables = [(g, s, grade, outcomes) for g, config in server.items() for s, slot in config['Slots'].items()
                  for grade, outcomes in slot['Enchants'].items()]
        print(len(tables))
        for group, slot, grade, outcomes in tables:
            print(group, slot, grade, len(outcomes))
            for name, chance in outcomes.items():
                print(ids[name], chance)
        return
    issues = compare(client, server)
    invalid_weights = [[g, s, grade, sum(outcomes.values())] for g, config in server.items()
                       for s, slot in config['Slots'].items() for grade, outcomes in slot['Enchants'].items()
                       if sum(outcomes.values()) != 100000]
    invalid_client_weights = [[g, s, grade, sum(outcomes.values())] for g, config in client.items()
                              for s, slot in config['Slots'].items() for grade, outcomes in slot['Enchants'].items()
                              if sum(outcomes.values()) != 100000]
    print(json.dumps({'compared_groups': len(client.keys() & server.keys()), 'issues': issues if args.details else issues[:4],
                      'client_groups': len(client), 'server_groups': len(server),
                      'missing_server_groups': sorted(client.keys() - server.keys()),
                      'server_only_groups': sorted(server.keys() - client.keys()),
                      'client_perfect_initial_recipes_all_groups': sum(len(s['Perfect']) for c in client.values() for s in c['Slots'].values()),
                      'client_normal_grade_tables': sum(len(s['Enchants']) for g, c in client.items() if g in server for s in c['Slots'].values()),
                      'client_perfect_initial_recipes': sum(len(s['Perfect']) for g, c in client.items() if g in server for s in c['Slots'].values()),
                      'server_normal_grade_tables': sum(len(s['Enchants']) for c in server.values() for s in c['Slots'].values()),
                      'issue_count': len(issues), 'issues_by_kind': dict(Counter(x['kind'] for x in issues)),
                      'issues_by_group': dict(Counter(x['key'][0] for x in issues)),
                      'invalid_server_probability_totals': invalid_weights,
                      'invalid_client_probability_totals': invalid_client_weights,
                      'unresolved_names': sorted(resolve.unresolved),
                      'unresolved_details': resolve.unresolved_details}, ensure_ascii=False, indent=2))
    raise SystemExit(bool(issues or invalid_weights or invalid_client_weights or resolve.unresolved))


if __name__ == '__main__':
    main()
