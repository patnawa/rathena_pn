#!/usr/bin/env python3
# ============================================================================
#  PN  /  CLIENT TOOLING
#  build.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/druid_missing_crowns/build.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Deterministic new-only data and clean-room client renderer; emits a patch, never installs."""
import argparse
from collections import OrderedDict
import hashlib
import json
from pathlib import Path
import re
import sys
import textwrap
import yaml
from facts import ITEMS, COMBOS, EXTERNAL_COMBOS, UNRESOLVED

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / 'tools/ci'))
from audit_enchant_upgrades import renewal_records

LABELS = {
    'bBaseAtk': ('ATK', ''), 'bMatk': ('MATK', ''), 'bAtkRate': ('ATK', '%'),
    'bMatkRate': ('MATK', '%'), 'bCritical': ('CRIT', ''), 'bHit': ('HIT', ''),
    'bPAtk': ('P.ATK', ''), 'bSMatk': ('S.MATK', ''), 'bCRate': ('C.RATE', ''),
    'bPow': ('POW', ''), 'bSpl': ('SPL', ''), 'bCon': ('CON', ''),
    'bMaxHP': ('Max HP', ''), 'bMaxSP': ('Max SP', ''),
    'bMaxHPrate': ('Max HP', '%'), 'bMaxSPrate': ('Max SP', '%'),
    'bCritAtkRate': ('Critical-damage bonus', '%'), 'bNonCritAtkRate': ('Non-critical physical damage', '%'),
    'bShortAtkRate': ('Melee physical damage', '%'), 'bLongAtkRate': ('Ranged physical damage', '%'),
    'bVariableCastrate': ('Variable cast time', '%'), 'bDelayrate': ('After-cast delay', '%'),
    'bPerfectHitAddRate': ('Perfect-hit chance', ' percentage points'),
}
JOBS = {'Assassin': 'Shadow Cross', 'Wizard': 'Arch Mage', 'Alchemist': 'Biolo',
        'BardDancer': 'Troubadour/Trouvere', 'Alitea': 'Alitea', 'Knight': 'Dragon Knight',
        'Sage': 'Elemental Master', 'KagerouOboro': 'Shinkiro/Shiranui',
        'Rebellion': 'Nightwatch', 'SoulLinker': 'Soul Ascetic'}


def require(value, message):
    if not value:
        raise AssertionError(message)


def groups(effects):
    result = OrderedDict()
    for effect in effects:
        result.setdefault(tuple(sorted(effect['conditions'].items())), []).append(effect)
    return result.items()


def condition(c):
    result = []
    for key, value in c:
        if key == 'refine': result.append(f'.@r>={value}')
        elif key == 'grade': result.append('.@g>=ENCHANTGRADE_' + 'DCBA'[value - 1])
        elif key == 'sum': result.append(f'.@sum>={value}')
        elif key in ('crown_grade', 'weapon_grade'):
            result.append('getenchantgrade(' + ('EQI_HEAD_TOP' if key == 'crown_grade' else 'EQI_HAND_R')
                          + ')>=ENCHANTGRADE_' + 'DCBA'[value - 1])
        elif key == 'learned': result.append(f'getskilllv("{value}")>={dict(c)["learned_level"]}')
        elif key != 'learned_level': raise AssertionError('Unhandled condition ' + key)
    return ' && '.join(result)


def script(effects):
    lines = ['.@r = getrefine();', '.@g = getenchantgrade();',
             '.@sum = getequiprefinerycnt(EQI_HEAD_TOP)+getequiprefinerycnt(EQI_HAND_R);']
    # Avoid unnecessary inventory queries in item-only and unconditional sets.
    text = repr(effects)
    if not any(e['step'] > 0 or 'refine' in e['conditions'] for e in effects): lines.remove(lines[0])
    if not any('grade' in e['conditions'] for e in effects): lines = [x for x in lines if not x.startswith('.@g =')]
    if not any(e['step'] == -1 or 'sum' in e['conditions'] for e in effects): lines = [x for x in lines if not x.startswith('.@sum =')]
    for conditions, events in groups(effects):
        if conditions: lines.append('if (' + condition(conditions) + ') {')
        prefix = '  ' if conditions else ''
        for e in events:
            op, selector = e['op'], e['selector']
            require(op in LABELS or op in ('bSkillAtk', 'bSkillCooldown', 'bMagicAtkEle', 'bMagicAddSize',
                    'bAddSize', 'bMagicAddEle', 'bAddEle', 'bAddRace', 'bAutoSpellOnSkill',
                    'bFixedCast', 'bUnbreakableWeapon'), 'Unsupported operation')
            amount = str(e['value'])
            if e['step'] > 0: amount += f'*(.@r/{e["step"]})'
            elif e['step'] == -1: amount = '.@sum' + (f'*{e["value"]}' if e['value'] != 1 else '')
            if op == 'bUnbreakableWeapon':
                require(e['value'] == 1 and not e['step'], 'Unbreakable value drift')
                line = 'bonus bUnbreakableWeapon;'
            elif op == 'bAutoSpellOnSkill':
                trigger, cast, level = selector.split('|')
                line = f'bonus4 {op},"{trigger}","{cast}",{level},{amount};'
            elif selector:
                selector = json.dumps(selector) if op in ('bSkillAtk', 'bSkillCooldown') else selector
                line = f'bonus2 {op},{selector},{amount};'
            else: line = f'bonus {op},{amount};'
            lines.append(prefix + line)
        if conditions: lines.append('}')
    return '\n'.join(lines) + '\n'


def skills():
    result = {}
    for row in renewal_records(ROOT, 'db/skill_db.yml'):
        result.setdefault(row['Name'], {}).update(row)
    return result


def describe_effects(effects, skill_db):
    def skill(name):
        require(name in skill_db and skill_db[name].get('Description'), 'Missing skill ' + name)
        return skill_db[name]['Description']
    output = []
    for conditions, events in groups(effects):
        labels = []
        for key, value in conditions:
            if key == 'refine': labels.append(f'Refine +{value} or higher')
            elif key == 'sum': labels.append(f'combined crown/right-hand weapon refine at least {value}')
            elif key in ('grade', 'crown_grade', 'weapon_grade'):
                labels.append({'grade': 'Grade', 'crown_grade': 'crown Grade', 'weapon_grade': 'right-hand weapon Grade'}[key]
                              + ' ' + 'DCBA'[value - 1] + (' or higher' if value < 4 else ''))
            elif key == 'learned':
                level = dict(conditions)['learned_level']
                require(level <= skill_db[value]['MaxLevel'], 'Impossible skill threshold')
                labels.append(skill(value) + f' learned at Lv. {level} or higher')
            elif key != 'learned_level': raise AssertionError(key)
        output.append('^0055AA' + ('; '.join(labels) or 'Unconditional bonuses') + ':^000000')
        for index, e in enumerate(events):
            op, selector, value = e['op'], e['selector'], e['value']
            suffix = f' per {e["step"]} refine levels' if e['step'] > 0 else (
                ' per combined crown/right-hand weapon refine level' if e['step'] == -1 else '')
            if op in LABELS:
                label, unit = LABELS[op]; line = f'{label} {value:+g}{unit}{suffix}.'
            elif op == 'bFixedCast': line = f'Fixed cast time {value / 1000:+g} sec{suffix}.'
            elif op in ('bSkillAtk', 'bSkillCooldown'):
                line = skill(selector) + (f' damage {value:+g}%' if op == 'bSkillAtk' else f' cooldown {value / 1000:+g} sec') + suffix + '.'
            elif op == 'bMagicAtkEle':
                line = ('All-element' if selector == 'Ele_All' else selector[4:] + '-element') + f' magic damage {value:+g}%{suffix}.'
            elif op in ('bAddSize', 'bMagicAddSize', 'bAddEle', 'bMagicAddEle'):
                require(selector in ('Size_All', 'Ele_All'), 'Unreviewed target restriction')
                line = ('Magic' if op.startswith('bMagic') else 'Physical') + ' damage against all ' + (
                    'sizes' if selector == 'Size_All' else 'target elements') + f' {value:+g}%{suffix}.'
            elif op == 'bAddRace':
                if selector != 'RC_All':
                    require(selector in ('RC_Player_Human', 'RC_Player_Doram') and value < 0,
                            'Unexpected race exclusion')
                    continue
                require(index + 2 < len(events) and all(events[index + n]['value'] == -value and
                        events[index + n]['selector'] == race for n, race in
                        ((1, 'RC_Player_Human'), (2, 'RC_Player_Doram'))), 'Missing race cancellations')
                line = f'Physical damage against non-player races {value:+g}%{suffix}.'
            elif op == 'bUnbreakableWeapon': line = 'Weapon is indestructible in battle.'
            elif op == 'bAutoSpellOnSkill':
                trigger, cast, level = selector.split('|')
                require(value == 1000 and 0 < int(level) <= skill_db[cast]['MaxLevel'], 'Autocast value drift')
                line = f'Using {skill(trigger)}: 100% trigger chance to autocast Lv. {level} {skill(cast)}.'
            else: raise AssertionError('Unsupported effect ' + op)
            output.append(line)
    return output


def fragment(all_items):
    skill_db = skills()
    output = ['-- Clean-room complete server-effect metadata: 12 crowns + 11 Sky partners.',
              '-- Does not change the active loader. Generic unidentified icon only.', 'tbl_druidmissingcrowns = {']
    for fact in sorted(ITEMS, key=lambda r: r['item']['Id']):
        item = fact['item']; id = item['Id']; name = item['AegisName']
        lines = ['Effects follow this server. Refine steps round down.', 'All met refine/grade conditions stack.']
        lines += describe_effects(fact['effects'], skill_db)
        related = [c for c in COMBOS + EXTERNAL_COMBOS if name in c['names']]
        effects = list(fact['effects'])
        for c in related:
            partner = next(n for n in c['names'] if n != name)
            require(partner in all_items, 'Missing complete set partner ' + partner)
            lines += ['_______________________', '^0055AASet: with ' + all_items[partner]['Name']
                      + f' (ID {all_items[partner]["Id"]})^000000']
            lines += describe_effects(c['effects'], skill_db); effects += c['effects']
        ops = {e['op'] for e in effects}
        if 'bCritAtkRate' in ops: lines += ['Critical-damage bonuses apply at half their rate to critical skills in Renewal.']
        if 'bNonCritAtkRate' in ops: lines += ['Non-critical bonus excludes skills flagged to ignore it.']
        if 'bAutoSpellOnSkill' in ops: lines += ['Autocasts remain subject to server skill, target and equipment restrictions.']
        lines += ['_______________________']
        if item['Type'] == 'Armor':
            lines += ['Type: Upper Headgear', f'Armor Level: {item["ArmorLevel"]}', f'DEF: {item["Defense"]}']
        else:
            lines += [f'Type: {item["SubType"]}', f'Weapon Level: {item["WeaponLevel"]}',
                      f'ATK: {item["Attack"]}', f'MATK: {item.get("MagicAttack", 0)}',
                      'Hands: ' + ('Two' if 'Both_Hand' in item['Locations'] else 'One'),
                      f'Normal-attack reach: {item["Range"]} (project category default)']
        lines += [f'Required Level: {item["EquipLevelMin"]}', f'Card Slots: {item["Slots"]}',
                  f'Weight: {item["Weight"] / 10:g}', 'Refineable: Yes', 'Gradable: Yes',
                  'Equip: ' + JOBS[next(iter(item['Jobs']))] + ' (trait class)']
        if 'Gender' in item: lines += ['Gender: ' + item['Gender']]
        wrapped = []
        for line in lines:
            colored = line.startswith('^0055AA'); plain = line[7:-7] if colored else line
            wrapped += [('^0055AA' + p + '^000000' if colored else p)
                        for p in textwrap.wrap(plain, 64, break_long_words=False, break_on_hyphens=False)]
        category = 'Headgear' if item['Type'] == 'Armor' else 'Weapon'
        output += [f'  [{id}] = {{', f'    unidentifiedDisplayName = "Unidentified {category}",',
                   '    unidentifiedResourceName = "EpisodClear20",',
                   '    unidentifiedDescriptionName = { "Identify this equipment to view its name." },',
                   '    identifiedDisplayName = ' + json.dumps(item['Name']) + ',',
                   '    identifiedResourceName = ' + json.dumps(fact['resource']) + ',',
                   '    identifiedDescriptionName = {']
        output += ['      ' + json.dumps(line) + ',' for line in wrapped]
        output += ['    },', f'    slotCount = {item["Slots"]}, ClassNum = {item.get("View", fact.get("client_view"))}, costume = false', '  },']
    return '\n'.join(output + ['}', ''])


def records():
    return [dict(f['item'], Script=script(f['effects'])) for f in ITEMS]


def yaml_dump(kind, rows):
    class Dumper(yaml.SafeDumper): pass
    Dumper.add_representer(str, lambda d, s: d.represent_scalar('tag:yaml.org,2002:str', s, style='|' if '\n' in s else None))
    return '# Additive reviewed definitions; see doc/druid_missing_crowns_audit.md.\n' + yaml.dump(
        {'Header': {'Type': kind, 'Version': 3 if kind == 'ITEM_DB' else 1}, 'Body': rows},
        Dumper=Dumper, sort_keys=False, width=110, allow_unicode=False)


def outputs():
    merged = {}
    for row in renewal_records(ROOT, 'db/item_db.yml'):
        merged.setdefault(row['Id'], {}).update(row)
    all_items = {r['AegisName']: r for r in merged.values()}
    partner_file = ROOT / 'db/import/druid_missing_weapons.yml'
    require(partner_file.is_file(), 'Coordinated complete weapon overlay is required')
    all_items.update({r['AegisName']: r for r in yaml.safe_load(partner_file.read_text())['Body']})
    all_items.update({r['AegisName']: r for r in records()})
    external_keys = {tuple(sorted(c['names'])) for c in EXTERNAL_COMBOS}
    external_hashes = {}
    for row in yaml.safe_load((ROOT / 'db/import/druid_missing_weapon_combos.yml').read_text())['Body']:
        for group in row['Combos']:
            names = tuple(sorted(group['Combo']))
            if names in external_keys:
                require('|'.join(names) not in external_hashes, 'Duplicate external crown set')
                external_hashes['|'.join(names)] = hashlib.sha256(row['Script'].encode()).hexdigest()
    require(len(external_hashes) == 4, 'Missing coordinated complete crown sets')
    rows = records()
    generated = {
        'db/import/druid_missing_crowns.yml': yaml_dump('ITEM_DB', [r for r in rows if r['Type'] == 'Armor']),
        'db/import/sky_crown_partner_weapons.yml': yaml_dump('ITEM_DB', [r for r in rows if r['Type'] == 'Weapon']),
        'db/import/druid_missing_crown_combos.yml': yaml_dump('COMBO_DB',
            [{'Combos': [{'Combo': c['names']}], 'Script': script(c['effects'])} for c in COMBOS]),
        'client-patch/druid_missing_crowns/SystemEN/itemInfo_DruidMissingCrowns.lua': fragment(all_items),
    }
    generated['client-patch/druid_missing_crowns/manifest.json'] = json.dumps({
        'schema': 1, 'date': '2026-09-06', 'supported_crowns': 12, 'sky_partners': 11,
        'unresolved': UNRESOLVED, 'items': ITEMS, 'combos': COMBOS,
        'external_combos_registered_elsewhere': EXTERNAL_COMBOS,
        'external_combo_script_sha256': external_hashes,
        'output_sha256': {p: hashlib.sha256(v.encode()).hexdigest() for p, v in generated.items()},
        'reach_source': 'Project category defaults; exact upstream records absent at e985006171d2eb320ee512a653f4c83aea3d81b6; official item-specific reach unresolved',
    }, indent=2, sort_keys=True) + '\n'
    return generated


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--patch', action='store_true', help='Emit apply_patch input for missing files only')
    args = parser.parse_args()
    result = outputs()
    if args.patch:
        print('*** Begin Patch')
        for path, content in result.items():
            require(not (ROOT / path).exists(), 'Refusing overwrite: ' + path)
            print('*** Add File: ' + path)
            print('\n'.join('+' + line for line in content.splitlines()))
        print('*** End Patch')
    else:
        for path, content in result.items():
            require((ROOT / path).read_text() == content, 'Generated file drift: ' + path)
        print('MISSING_CROWNS_RENDER_OK items=23 combos=11 unresolved=1')
