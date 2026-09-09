#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_enchant_upgrades.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_enchant_upgrades.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compare active client upgrade recipes with effective Renewal server imports.

Requires PyYAML. --emit-perfect prints an import; it never modifies files.
Client paths are supplied explicitly because GRF precedence is deployment-specific.
"""
import argparse
import copy
import hashlib
import json
import re
import subprocess
from collections import defaultdict
from pathlib import Path
import yaml
from lua51_literal_table import literal_tables


def renewal_records(root, relative):
    """Yield records in database import order, rejecting cycles."""
    active = set()

    def read(relative):
        path = (root / relative).resolve()
        if not path.exists():
            return
        if path in active:
            raise ValueError(f'Import cycle: {relative}')
        active.add(path)
        data = yaml.load(path.read_text(encoding='utf-8'), Loader=getattr(yaml, 'CSafeLoader', yaml.SafeLoader)) or {}
        yield from data.get('Body') or []
        for entry in data.get('Footer', {}).get('Imports', []):
            if entry.get('Mode', 'Renewal') == 'Renewal':
                yield from read(entry['Path'])
        active.remove(path)
    yield from read(relative)


class ClientItemNames:
    """Resolve localized client names by numeric ID, not by translated labels."""

    def __init__(self, path, root):
        data = path.read_bytes()
        self.sha256 = hashlib.sha256(data).hexdigest()
        self.client = literal_tables(data)['ItemDBNameTbl']
        if not all(isinstance(k, str) and type(v) is int and v > 0 for k, v in self.client.items()):
            raise ValueError('ItemDBNameTbl must map names to positive integer IDs')
        self.server = {}
        for record in renewal_records(root, 'db/item_db.yml'):
            if 'AegisName' in record:
                self.server[record['Id']] = record['AegisName']
        self.aliases, self.unresolved = {}, set()
        self.unresolved_details = {}

    def __call__(self, name):
        item_id = self.client.get(name)
        if item_id not in self.server:
            self.unresolved.add(name)
            self.unresolved_details[name] = {
                'reason': 'client_name_missing' if item_id is None else 'server_item_missing',
                'client_id': item_id,
            }
            return name
        result = self.server[item_id]
        if result != name:
            self.aliases[name] = {'Id': item_id, 'AegisName': result}
        return result


def split_args(text):
    return [part.strip() for part in re.split(r',(?=(?:[^\"]*\"[^\"]*\")*[^\"]*$)(?![^{}]*\})', text)]


def item_name(value, resolve=None):
    # The Korean client retains localized Aegis names for the six stat cards.
    name = json.loads(value)
    if resolve is not None:
        return resolve(name)
    for korean, aegis in [('힘', 'Strength'), ('인트', 'Inteligence'),
                          ('덱', 'Dexterity'), ('어질', 'Agility'),
                          ('바탈', 'Vitality'), ('럭', 'Luck')]:
        if re.fullmatch(korean + r'\d+', name):
            return aegis + name[len(korean):]
    return name


def client_recipes(path, resolve=None):
    ordinary, perfect = {}, {}
    for line in path.read_text(encoding='cp949').splitlines():
        line = line.strip()
        match = re.match(r'^Table\[(\d+)\]\.Slot\[(\d+)\]:(AddUpgradeEnchant|AddPerfectUpgradeEnchant|SetRandomUpgradeRequire|AddRandomUpgradeEnchant)\((.*)\)$', line)
        if not match:
            if re.match(r'^Table\[\d+\]\.Slot\[\d+\]:(AddUpgradeEnchant|AddPerfectUpgradeEnchant|SetRandomUpgradeRequire|AddRandomUpgradeEnchant)\(', line):
                raise ValueError(f'Unsupported upgrade declaration: {line[:120]}')
            continue
        group, slot, method, args = match.groups()
        args = split_args(args)
        source = item_name(args[0], resolve)
        key = (int(group), int(slot), source)
        if method == 'AddRandomUpgradeEnchant':
            ordinary[key]['RandomUpgrades'].append({'Upgrade': item_name(args[1], resolve), 'Chance': int(args[2])})
            continue
        if method == 'SetRandomUpgradeRequire':
            recipe = {'Enchant': source, 'RandomUpgrades': []}
            offset = 1
        else:
            recipe = {'Enchant': source, 'Upgrade': item_name(args[1], resolve)}
            offset = 2
        recipe['Price'] = int(args[offset])
        materials = []
        for arg in args[offset + 1:]:
            material = re.fullmatch(r'\{\s*"([^"\\]+)"\s*,\s*(\d+)\s*\}', arg)
            if not material:
                raise ValueError(f'Unsupported material syntax: {arg}')
            materials.append({'Material': resolve(material[1]) if resolve else material[1], 'Amount': int(material[2])})
        if materials:
            recipe['Materials'] = materials
        destination = perfect if method == 'AddPerfectUpgradeEnchant' else ordinary
        if destination is perfect:
            key += (recipe['Upgrade'],)
        if key in destination:
            raise ValueError(f'Duplicate recipe {key}')
        destination[key] = recipe
    for key, recipe in ordinary.items():
        if 'RandomUpgrades' in recipe and sum(x['Chance'] for x in recipe['RandomUpgrades']) != 100000:
            raise ValueError(f'Invalid client random probabilities: {key}')
    return ordinary, perfect


def merge_recipe(previous, recipe, perfect=False):
    """Mirror successful ItemEnchantDatabase overlay semantics, not list replacement."""
    result = copy.deepcopy(previous)
    if 'Upgrade' in recipe and 'RandomUpgrades' in recipe:
        raise ValueError('Upgrade and RandomUpgrades are mutually exclusive')
    if perfect and ('Upgrade' not in recipe or 'RandomUpgrades' in recipe):
        raise ValueError('Perfect upgrades require a deterministic target')
    if not previous and 'Upgrade' not in recipe and 'RandomUpgrades' not in recipe:
        raise ValueError('New upgrades require an outcome')
    if 'RandomUpgrades' in recipe:
        outcomes = recipe['RandomUpgrades']
        if (not outcomes or len({x['Upgrade'] for x in outcomes}) != len(outcomes)
                or any(not 0 < x['Chance'] <= 100000 for x in outcomes)
                or sum(x['Chance'] for x in outcomes) != 100000):
            raise ValueError('Invalid random upgrade outcomes')
    if 'Upgrade' in recipe:
        result.pop('RandomUpgrades', None)
    if 'RandomUpgrades' in recipe:
        result.pop('Upgrade', None)
    result.update(copy.deepcopy({k: v for k, v in recipe.items() if k != 'Materials'}))
    if 'Price' in result:
        price = result['Price']
        if not 0 <= price <= 4294967295 or (perfect and price > 2147483647):
            raise ValueError('Invalid upgrade Zeny price')
        result['Price'] = min(price, 2147483647)  # MAX_ZENY
    materials = {x['Material']: x.get('Amount', 1) for x in previous.get('Materials', [])}
    for entry in recipe.get('Materials') or []:
        name = entry['Material']
        amount = entry.get('Amount', materials.get(name, 1))
        if not 0 <= amount <= 65535:
            raise ValueError(f'Material amount is outside uint16: {name}')
        if amount:
            materials[name] = min(amount, 30000)  # MAX_AMOUNT
        else:
            materials.pop(name, None)
    if materials or 'Materials' in previous or 'Materials' in recipe:
        result['Materials'] = [{'Material': k, 'Amount': v} for k, v in materials.items()]
    return result


def server_recipes(root):
    groups, ordinary, perfect = set(), {}, {}
    for record in renewal_records(root, 'db/item_enchant.yml'):
        group = record['Id']
        groups.add(group)
        for slot in record.get('Slots') or []:
            for section, dest in [('Upgrades', ordinary), ('PerfectUpgrades', perfect)]:
                for recipe in slot.get(section) or []:
                    key = (group, slot['Slot'], recipe['Enchant'])
                    if section == 'PerfectUpgrades':
                        key += (recipe['Upgrade'],)
                    dest[key] = merge_recipe(dest.get(key, {}), recipe, section == 'PerfectUpgrades')
    return groups, ordinary, perfect


def canonical(recipe):
    return (recipe.get('Upgrade'), recipe.get('Price', 0),
            sorted((x['Material'], x.get('Amount', 1)) for x in recipe.get('Materials', [])),
            sorted((x['Upgrade'], x['Chance']) for x in recipe.get('RandomUpgrades', [])))


def workshop_contents(root, ordinary, requested_groups, baseline_head=False):
    """Replace only known restored workshop upgrade blocks; preserve other data."""
    class Dumper(yaml.SafeDumper):
        def increase_indent(self, flow=False, indentless=False):
            return super().increase_indent(flow, False)
    files = {'db/import/grademk_service_enchants.yml': set(range(7, 14)) | set(range(117, 125)) | {142},
             'db/import/item_enchant.yml': {132, 163}}
    contents = {}
    for relative, groups in files.items():
        groups &= requested_groups
        before = (subprocess.check_output(['git', 'show', 'HEAD:' + relative], cwd=root).decode('utf-8')
                  if baseline_head else (root / relative).read_text(encoding='utf-8'))
        lines, output = before.splitlines(keepends=True), []
        last_slot, current_group, in_slots = {}, None, False
        for position, line in enumerate(lines):
            match = re.match(r'^  - Id: (\d+)', line)
            if match:
                current_group = int(match[1])
                in_slots = False
            if re.match(r'^    [A-Za-z]', line):
                in_slots = line.strip() == 'Slots:'
            match = re.match(r'^      - Slot: (\d+)', line)
            if match and in_slots:
                last_slot[(current_group, int(match[1]))] = position
        group, i, in_slots = None, 0, False
        while i < len(lines):
            group_match = re.match(r'^  - Id: (\d+)', lines[i])
            if group_match:
                group = int(group_match[1])
                in_slots = False
            if re.match(r'^    [A-Za-z]', lines[i]):
                in_slots = lines[i].strip() == 'Slots:'
            slot_match = re.match(r'^      - Slot: (\d+)', lines[i])
            if group not in groups or not slot_match or not in_slots:
                output.append(lines[i])
                i += 1
                continue
            slot = int(slot_match[1])
            end = i + 1
            while end < len(lines) and not re.match(r'^(?:  - Id:|      - Slot:|    [A-Za-z])', lines[end]):
                end += 1
            block = lines[i:end]
            kept, j = [], 0
            while j < len(block):
                if block[j].strip() == 'Upgrades:':
                    j += 1
                    while j < len(block) and (not block[j].strip() or len(block[j]) - len(block[j].lstrip()) > 8):
                        j += 1
                else:
                    kept.append(block[j])
                    j += 1
            recipes = [r for (g, s, source), r in ordinary.items() if g == group and s == slot
                       and i == last_slot[(group, slot)]]
            # Retain existing order/format when the recipe is already exact;
            # do not produce noisy probability-order-only changes.
            existing = yaml.safe_load(''.join(block))[0].get('Upgrades', [])
            desired = {r['Enchant']: r for r in recipes}
            recipes = []
            for previous in existing:
                current = desired.pop(previous['Enchant'], None)
                if current is not None:
                    recipes.append(previous if canonical(previous) == canonical(current) else current)
                elif group == 132:
                    # The custom Biosphere service retains two additional jewel
                    # families absent from this client's group 132 table.
                    recipes.append(previous)
            recipes.extend(desired.values())
            trailing = []
            while kept and (not kept[-1].strip() or kept[-1].lstrip().startswith('#')):
                trailing.insert(0, kept.pop())
            if recipes:
                section = yaml.dump({'Upgrades': recipes}, Dumper=Dumper, sort_keys=False, width=120)
                kept += ['        ' + line + '\n' for line in section.splitlines()]
            kept.extend(trailing)
            output.extend(kept)
            i = end
        after = ''.join(output)
        if relative.endswith('grademk_service_enchants.yml'):
            after = after.replace('# Guaranteed recipes use Upgrade; weighted recipes use RandomUpgrades.',
                                  '# Ordinary upgrades only; guaranteed upgrades are in perfect_item_enchant.yml.')
        if after != before:
            contents[relative] = after
    return contents


def verify_preserved(root, ref):
    def without_upgrades(value):
        if isinstance(value, list):
            return [without_upgrades(x) for x in value]
        if isinstance(value, dict):
            return {k: without_upgrades(v) for k, v in value.items() if k != 'Upgrades'}
        return value
    for relative in ['db/import/grademk_service_enchants.yml', 'db/import/item_enchant.yml']:
        before = yaml.safe_load(subprocess.check_output(['git', 'show', ref + ':' + relative], cwd=root))
        after = yaml.safe_load((root / relative).read_text(encoding='utf-8'))
        if without_upgrades(before) != without_upgrades(after):
            raise ValueError(f'Non-upgrade fields changed in {relative}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client', type=Path)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--emit-perfect', action='store_true')
    parser.add_argument('--repair-workshop-content', action='store_true')
    parser.add_argument('--baseline-head', action='store_true', help='Use committed baseline for a reviewed migration')
    parser.add_argument('--compare-non-upgrades-ref', help='Prove unrelated workshop database fields match this git ref')
    parser.add_argument('--groups', help='Comma-separated group IDs to inspect (default: all active groups)')
    parser.add_argument('--details', action='store_true')
    parser.add_argument('--client-item-names', type=Path, help='Active ItemDBNameTbl.lub: resolve localized names by item ID')
    args = parser.parse_args()
    resolve = ClientItemNames(args.client_item_names, args.root) if args.client_item_names else None
    ordinary, perfect = client_recipes(args.client, resolve)
    groups, server_ordinary, server_perfect = server_recipes(args.root)
    if args.compare_non_upgrades_ref:
        verify_preserved(args.root, args.compare_non_upgrades_ref)
    if args.groups:
        groups &= {int(x) for x in args.groups.split(',')}
    if args.repair_workshop_content:
        print(json.dumps(workshop_contents(args.root, ordinary, groups, args.baseline_head)))
        return
    if args.emit_perfect:
        grouped = defaultdict(lambda: defaultdict(list))
        for (group, slot, source, target), recipe in perfect.items():
            if group in groups:
                grouped[group][slot].append(recipe)
        body = [{'Id': group, 'Slots': [{'Slot': slot, 'PerfectUpgrades': recipes}
                for slot, recipes in sorted(slots.items())]} for group, slots in sorted(grouped.items())]
        class Dumper(yaml.SafeDumper):
            def increase_indent(self, flow=False, indentless=False):
                return super().increase_indent(flow, False)
        print('# Guaranteed upgrade recipes from the active client. Kept separate from ordinary/random upgrades.')
        print(yaml.dump({'Header': {'Type': 'ITEM_ENCHANT_DB', 'Version': 1}, 'Body': body},
                       Dumper=Dumper, sort_keys=False, width=120), end='')
        return
    issues = []
    for label, client, server in [('ordinary', ordinary, server_ordinary), ('perfect', perfect, server_perfect)]:
        for key, recipe in client.items():
            if key[0] not in groups:
                continue
            if key not in server:
                issue = [label, list(key), 'missing']
            elif canonical(recipe) != canonical(server[key]):
                issue = [label, list(key), 'recipe mismatch']
            else:
                continue
            if args.details:
                issue.append({'client': recipe, 'server': server.get(key)})
            issues.append(issue)
    missing_groups = sorted({k[0] for k in ordinary.keys() | perfect.keys()} - set(groups))
    print(json.dumps({'active_server_groups': len(groups),
                      'missing_server_recipe_groups': missing_groups,
                      'client_ordinary_recipes_in_missing_groups': sum(k[0] not in groups for k in ordinary),
                      'client_perfect_recipes_in_missing_groups': sum(k[0] not in groups for k in perfect),
                      'item_name_resolution': None if resolve is None else {
                          'table_sha256': resolve.sha256,
                          'localized_aliases_resolved': len(resolve.aliases),
                          'unresolved_names': sorted(resolve.unresolved),
                          'unresolved_details': resolve.unresolved_details},
                      'client_ordinary_recipes_in_active_groups': sum(k[0] in groups for k in ordinary),
                      'client_perfect_recipes_in_active_groups': sum(k[0] in groups for k in perfect),
                      'server_only_ordinary_recipes_in_active_groups': sum(k[0] in groups and k not in ordinary for k in server_ordinary),
                      'issue_count': len(issues),
                      'issues_by_group': {str(group): sum(x[1][0] == group for x in issues) for group in sorted({x[1][0] for x in issues})},
                      'issues': issues if args.details else issues[:12]}, indent=2, ensure_ascii=False))
    raise SystemExit(bool(issues or missing_groups) or bool(resolve and resolve.unresolved))


if __name__ == '__main__':
    main()
