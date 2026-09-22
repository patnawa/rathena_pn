"""Read-only closure check for Episode 21 and Chapter 1/2 equipment routes.

Run from a workspace containing this repository and the installed PN-Client.
"""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/ci'))
from audit_enchant_upgrades import renewal_records


def rows(path, key):
    return {row[key]: row for row in renewal_records(ROOT, path)}


items_by_id = rows('db/item_db.yml', 'Id')
items_by_name = {row['AegisName']: row for row in items_by_id.values() if 'AegisName' in row}
enchant_patches = {}
for enchant_row in renewal_records(ROOT, 'db/item_enchant.yml'):
    enchant_patches.setdefault(enchant_row['Id'], []).append(enchant_row)
reforms = rows('db/item_reform.yml', 'Item')
barters = rows('npc/custom/barters.yml', 'Name')
violations = []
count = 0


def check(ok, message):
    global count
    count += 1
    if not ok:
        violations.append(message)


def item(name):
    row = items_by_id.get(name) if isinstance(name, int) else items_by_name.get(name)
    check(row is not None, f'Undefined item: {name}')
    return row


def shop(name):
    check(name in barters, f'Unloaded barter: {name}')
    row = barters.get(name, {})
    for offer in row.get('Items', []):
        item(offer['Item'])
        for cost in offer.get('RequiredItems', []):
            item(cost['Item'])
            check(cost.get('Amount', 0) > 0, f'Nonpositive cost in {name}: {cost}')
    return {offer['Item'] for offer in row.get('Items', [])}


ep21 = shop('barter_ep21_gaebolg_equipment') | shop('barter_ep21_yorscalp_crowns')
ch1 = shop('CH1_STORE_4') | shop('CH1_STORE_5')
chapter2 = (ROOT / 'npc/custom/chapter2/Chapter2.txt').read_text()
match = re.search(r'setarray \.@items\[0\],([\d,]+);\s*\n\s*\.@item=\.@items\[\.@p-1\]', chapter2)
check(match is not None, 'Chapter 2 Azure shop array not found')
ch2_ids = [int(part) for part in match.group(1).split(',')] if match else []
ch2 = {item(i)['AegisName'] for i in ch2_ids if item(i)}
check(len(ch2_ids) == 11 and len(ch2) == 11, f'Chapter 2 Azure shop has {len(ch2_ids)} IDs')
next_match = re.search(r'setarray \.@next\[0\],([\d,]+);', chapter2)
check(next_match is not None, 'Chapter 2 Blaze reform array not found')
blaze_ids = [int(part) for part in next_match.group(1).split(',')] if next_match else []
check(len(blaze_ids) == len(ch2_ids), 'Azure/Blaze reform array length mismatch')
for number in blaze_ids:
    item(number)

groups = {
    'Episode 21 Gaebolg': ([143, 144, 145, 146], {n for n in ep21 if n.startswith('Gaebolg_')}),
    'Episode 21 Yorscalp': ([152, 153, 154, 155, 156], {n for n in items_by_name if n.startswith('Yorscalp_') and items_by_name[n].get('Type') == 'Armor'}),
    'Chapter 1 Entwined': ([160, 161, 162], {n for n in ch1 if n.startswith('Entwined_')}),
    'Chapter 1 Dimension': ([163], {n for n in ch1 if n.startswith('Dimension_')}),
    'Chapter 2 Azure': ([167, 168, 169, 170, 171], ch2),
}
for label, (ids, expected) in groups.items():
    targeted = set()
    for group_id in ids:
        patches = enchant_patches.get(group_id, [])
        check(bool(patches), f'{label}: enchant group {group_id} absent')
        for row in patches:
            for name, enabled in row.get('TargetItems', {}).items():
                if enabled:
                    targeted.add(name)
                else:
                    targeted.discard(name)
                item(name)
            for slot in row.get('Slots', []):
                for offer in slot.get('PerfectEnchants', []):
                    item(offer['Item'])
                    for material in offer.get('Materials', []):
                        item(material['Material'])
    missing = sorted(expected - targeted)
    check(not missing, f'{label}: offered equipment missing from enchant groups: {missing}')
    print(f'{label}: offered={len(expected)} targeted={len(targeted)} missing={len(missing)}')

for stone in ('Yorscalp_Scroll_A', 'Gaebolg_A_Hammer_1', 'Gaebolg_A_Hammer_2', 'Gaebolg_A_Hammer_3'):
    row = reforms.get(stone)
    check(row is not None, f'Missing reform tool: {stone}')
    if row:
        item(stone)
        for entry in row.get('BaseItems', []):
            item(entry['BaseItem'])
            item(entry['ResultItem'])
            for material in entry.get('Materials', []):
                item(material['Material'])

reformed = {entry['ResultItem'] for entry in reforms['Yorscalp_Scroll_A']['BaseItems']}
check(reformed == {n for n in items_by_name if n.startswith('Yorscalp_')
                   and items_by_name[n].get('Type') == 'Armor'} - shop('barter_ep21_yorscalp_crowns'),
      'Gaebolg-to-Yorscalp reform does not produce every non-crown piece')
for old_id, new_id in zip(ch2_ids, blaze_ids):
    old, new = items_by_id[old_id], items_by_id[new_id]
    for field in ('Type', 'Locations', 'Slots'):
        check(old.get(field) == new.get(field),
              f'Azure-to-Blaze {old_id}->{new_id} incompatible {field}')

groups_db = rows('db/item_group_db.yml', 'Group')
hard_box = groups_db['AEGIS_103537']
box_contents = {offer['Item'] for sub in hard_box['SubGroups'] for offer in sub['List']}
check({'Yorscalp_Serpent', 'Yorscalp_Symbol', 'Yorscalp_Spirit'} <= box_contents,
      'Yorscalp material missing from final-battle antiquity')
check('getitem 103537,1;' in (ROOT / 'npc/custom/episode21/FinalBattle.txt').read_text(),
      'Final battle has no antiquity reward')

client_root = next((candidate for candidate in (ROOT.parent.parent, *(parent / 'PN-Client' for parent in ROOT.parents))
                    if (candidate / 'DATA.INI').is_file()), ROOT.parent.parent)
client = client_root / 'SystemEN'
loader = (client / 'itemInfo.lua').read_text(errors='replace')
client_paths = [client / 'LuaFiles514/itemInfo.lua']
client_paths += [client / name for name in re.findall(r'"(itemInfo_[^"/]+\.lua)"', loader)]
known_ids = set()
for path in client_paths:
    check(path.is_file(), f'Loaded client item fragment missing: {path.name}')
    if path.is_file():
        known_ids.update(int(s) for s in re.findall(r'\[(\d+)\]\s*=', path.read_text(errors='replace')))
client_gear = ep21 | ch1 | ch2 | reformed | {items_by_id[n]['AegisName'] for n in blaze_ids if n in items_by_id}
missing_client = sorted((items_by_name[n]['Id'], n) for n in client_gear if n in items_by_name
                        and items_by_name[n]['Id'] not in known_ids)
check(not missing_client, f'Equipment lacks loaded client description: {missing_client}')
print(f'Client metadata: gear={len(client_gear)} missing={len(missing_client)}')

print(json.dumps({'checks': count, 'violations': violations}, indent=2))
sys.exit(bool(violations))
