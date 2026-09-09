# ============================================================================
#  PN  /  CLIENT TOOLING
#  effects.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/enchant_target_metadata/effects.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Fail-closed clean-room renderer for the twelve reviewed crown Scripts.

This is a description renderer, not a replacement for rAthena's script VM.
Only the exact reviewed expression/condition/bonus vocabulary is accepted.
"""
import hashlib
import json
import re
import textwrap

from audit_enchant_upgrades import renewal_records


def require(value, message):
    if not value:
        raise AssertionError(message)


def sha(text):
    return hashlib.sha256(text.encode('utf-8')).hexdigest()


def effective(root, target_ids):
    items = {}
    for row in renewal_records(root, 'db/item_db.yml'):
        item = items.setdefault(row['Id'], {})
        # Relevant overlays use field updates; preserve existing map members as
        # the native parser does for Jobs, Classes and Locations.
        for key, value in row.items():
            if isinstance(value, dict) and isinstance(item.get(key), dict):
                item[key].update(value)
            else:
                item[key] = value
    names = {row['AegisName']: i for i, row in items.items()}
    combos = {}
    for row in renewal_records(root, 'db/item_combos.yml'):
        groups = row.get('Combos', [])
        if any(any(name not in names for name in group['Combo']) for group in groups):
            continue  # Native parser refuses this whole row on unresolved item.
        for group in groups:
            ids = tuple(sorted(names[name] for name in group['Combo']))
            if not set(ids) & target_ids:
                continue
            if row.get('Clear', False):
                combos.pop(ids, None)
            elif 'Script' in row:
                combos[ids] = row['Script']
    skills = {}
    for row in renewal_records(root, 'db/skill_db.yml'):
        skills.setdefault(row['Id'], {}).update(row)
    return items, combos, {row['Name']: row for row in skills.values()}


def statements(script):
    stack, result = [], []
    assignments = {
        '.@g = getenchantgrade();', '.@r = getrefine();',
        '.@sum = getequiprefinerycnt(EQI_HEAD_TOP)+getequiprefinerycnt(EQI_HAND_R);',
    }
    for raw in script.splitlines():
        line = raw.strip()
        if not line or line in assignments:
            continue
        match = re.fullmatch(r'if \((.*)\) \{', line)
        if match:
            stack.append(match[1].split(' && '))
        elif line == '}':
            require(stack, 'Unbalanced closing condition')
            stack.pop()
        else:
            match = re.fullmatch(r'(bonus(?:2|4)?) (.*);', line)
            require(match, 'Unreviewed Script statement: ' + line)
            args = [part.strip() for part in match[2].split(',')]
            require(len(args) == {'bonus': 2, 'bonus2': 3, 'bonus4': 5}[match[1]],
                    'Unexpected bonus arity: ' + line)
            conditions = {}
            for condition in sum(stack, []):
                key, value = condition_value(condition)
                conditions[key] = max(value, conditions.get(key, 0))
            result.append((tuple(sorted(conditions.items())), args))
    require(not stack, 'Unclosed condition')
    return result


def condition_value(condition):
    for expression, key in ((r'\.@r>=(\d+)', 'refine'), (r'\.@sum>=(\d+)', 'sum')):
        match = re.fullmatch(expression, condition)
        if match:
            return key, int(match[1])
    match = re.fullmatch(r'(\.@g|getenchantgrade\((EQI_HEAD_TOP|EQI_HAND_R)\))>=ENCHANTGRADE_([DCBA])', condition)
    if match:
        return {'EQI_HEAD_TOP': 'crown_grade', 'EQI_HAND_R': 'weapon_grade', None: 'grade'}[match[2]], 'DCBA'.index(match[3]) + 1
    match = re.fullmatch(r'getskilllv\("([A-Z0-9_]+)"\)>=(\d+)', condition)
    require(match, 'Unreviewed condition: ' + condition)
    return 'skill:' + match[1], int(match[2])


def amount(expression, unit='', divisor=1):
    match = re.fullmatch(r'(-?\d+)', expression)
    if match:
        return f'{int(match[1]) / divisor:+g}{unit}'
    match = re.fullmatch(r'(?:(-?\d+)\*)?\(\.@r/(\d+)\)', expression)
    if match:
        return f'{int(match[1] or 1) / divisor:+g}{unit} per {match[2]} refine levels'
    match = re.fullmatch(r'\.@sum\*(\d+)', expression)
    require(match, 'Unreviewed amount: ' + expression)
    return f'+{int(match[1]) / divisor:g}{unit} per combined crown/right-hand weapon refine level'


BONUS = {
    'bPow': ('POW', ''), 'bCon': ('CON', ''), 'bSpl': ('SPL', ''),
    'bCritAtkRate': ('Critical-damage bonus', '%'), 'bBaseAtk': ('ATK', ''),
    'bMatk': ('MATK', ''), 'bShortAtkRate': ('Melee physical damage', '%'),
    'bLongAtkRate': ('Ranged physical damage', '%'), 'bCRate': ('C.RATE', ''),
    'bAtkRate': ('ATK', '%'), 'bMatkRate': ('MATK', '%'),
    'bPAtk': ('P.ATK', ''), 'bSMatk': ('S.MATK', ''), 'bCritical': ('CRIT', ''),
    'bVariableCastrate': ('Variable cast time', '%'), 'bDelayrate': ('After-cast delay', '%'),
    'bUseSPrate': ('SP consumption', '%'), 'bHit': ('HIT', ''),
    'bNonCritAtkRate': ('Non-critical physical damage', '%'), 'bMaxHPrate': ('Max HP', '%'),
    'bPerfectHitAddRate': ('Perfect-hit chance', ' percentage points'),
}


def skill_name(skills, name):
    name = name.strip('"')
    require(name in skills and skills[name].get('Description'), 'Missing skill: ' + name)
    return skills[name]['Description']


def condition_text(conditions, skills):
    parts = []
    for key, value in conditions:
        if key == 'refine':
            parts.append(f'Refine +{value} or higher')
        elif key == 'sum':
            parts.append(f'combined crown/right-hand weapon refine at least {value}')
        elif key in ('grade', 'crown_grade', 'weapon_grade'):
            label = {'grade': 'Grade', 'crown_grade': 'crown Grade', 'weapon_grade': 'right-hand weapon Grade'}[key]
            parts.append(label + ' ' + 'DCBA'[value - 1] + (' or higher' if value < 4 else ''))
        elif key.startswith('skill:'):
            require(value <= skills[key[6:]]['MaxLevel'], 'Impossible learned-skill condition')
            parts.append(skill_name(skills, key[6:]) + f' learned at Lv. {value} or higher')
        else:
            raise AssertionError('Unhandled condition key: ' + key)
    return '; '.join(parts) if parts else 'Unconditional bonuses'


def effect_lines(script, skills):
    events = statements(script)
    output, last_conditions, index = [], None, 0
    while index < len(events):
        conditions, args = events[index]
        index += 1
        command, *values = args
        # This raw dummy-key entry is unreachable, while the equal parent-key
        # entry already covers both queries. Render the effective bonus once.
        if command == 'bSkillAtk' and values[0] == '"CD_ARBITRIUM_ATK"':
            require((conditions, ['bSkillAtk', '"CD_ARBITRIUM"', values[1]]) in events,
                    'Arbitrium duplicate no longer has its effective parent entry')
            continue
        require(not (command == 'bSkillAtk' and values[0] == '"WL_CHAINLIGHTNING_ATK"'),
                'Unreachable Chain Lightning dummy bonus: require reviewed server fix')
        if conditions != last_conditions:
            output += ['^0055AA' + condition_text(conditions, skills) + ':^000000']
            last_conditions = conditions
        if command in BONUS:
            require(len(values) == 1, 'Bonus arity drift')
            label, unit = BONUS[command]
            line = label + ' ' + amount(values[0], unit) + '.'
        elif command == 'bFixedCast':
            line = 'Fixed cast time ' + amount(values[0], ' sec', 1000) + '.'
        elif command in ('bSkillAtk', 'bSkillCooldown'):
            line = skill_name(skills, values[0])
            line += (' damage ' + amount(values[1], '%') if command == 'bSkillAtk'
                     else ' cooldown ' + amount(values[1], ' sec', 1000)) + '.'
        elif command == 'bMagicAtkEle':
            require(re.fullmatch(r'Ele_(All|Holy|Neutral|Ghost|Wind|Fire|Earth|Water|Poison|Dark|Undead)', values[0]),
                    'Unreviewed attack element')
            label = 'All-element' if values[0] == 'Ele_All' else values[0][4:] + '-element'
            line = label + ' magic damage ' + amount(values[1], '%') + '.'
        elif command in ('bAddSize', 'bMagicAddSize', 'bAddEle', 'bMagicAddEle'):
            dimension = 'sizes' if command.endswith('Size') else 'target elements'
            require(values[0] == ('Size_All' if dimension == 'sizes' else 'Ele_All'), 'Unreviewed target filter')
            label = 'Magic' if command.startswith('bMagic') else 'Physical'
            line = label + ' damage against all ' + dimension + ' ' + amount(values[1], '%') + '.'
        elif command in ('bAddRace', 'bMagicAddRace'):
            require(values[0] == 'RC_All' and not values[1].startswith('-'), 'Unreviewed race bonus')
            # The following two exact cancellations must be present; never
            # silently drop a negative race effect or broaden it to players.
            for race in ('RC_Player_Human', 'RC_Player_Doram'):
                require(index < len(events) and events[index] ==
                        (conditions, [command, race, '-' + values[1]]), 'Race exclusion drift')
                index += 1
            label = 'Magic' if command.startswith('bMagic') else 'Physical'
            line = label + ' damage against non-player races ' + amount(values[1], '%') + '.'
        elif command == 'bAutoSpellOnSkill':
            trigger, cast, level, chance = values
            require(chance == '1000' and 0 < int(level) <= skills[cast.strip('"')]['MaxLevel'],
                    'Unreviewed autocast level/chance')
            line = ('Using ' + skill_name(skills, trigger) + ': 100% trigger chance to autocast Lv. '
                    + level + ' ' + skill_name(skills, cast) + '.')
        else:
            raise AssertionError('Unreviewed bonus operation: ' + command)
        output.append(line)
    return output


JOB_LABELS = {
    'Crusader': 'Imperial Guard', 'Rogue': 'Abyss Chaser', 'Blacksmith': 'Meister',
    'Hunter': 'Windhawk', 'SuperNovice': 'Hyper Novice', 'Priest': 'Cardinal',
    'Monk': 'Inquisitor', 'StarGladiator': 'Sky Emperor',
}


def description(item, combos, items, skills):
    require(item.get('Type') == 'Armor' and item.get('Locations') == {'Head_Top': True}, 'Unsupported equipment category')
    require(item.get('Refineable') is True and item.get('Gradable') is True, 'Refine/grade capability drift')
    require(not any(k in item for k in ('EquipLevelMax', 'Sex', 'SubType', 'EquipScript', 'UnEquipScript')),
            'Additional equipment restrictions/effects require review')
    lines = ['Effects follow this server. Refine steps round down.',
             'All met refine/grade conditions stack.'] + effect_lines(item['Script'], skills)
    related = [(ids, script) for ids, script in combos.items() if item['Id'] in ids]
    for ids, script in sorted(related):
        require(len(ids) == 2, 'Unreviewed multi-part combo')
        partner_id = next(i for i in ids if i != item['Id'])
        partner = items[partner_id]
        lines += ['_______________________', '^0055AASet: with ' + partner['Name'] + f' (ID {partner_id})^000000']
        lines += effect_lines(script, skills)
    combined_script = item['Script'] + ''.join(script for _, script in related)
    if 'bCritAtkRate' in combined_script:
        lines += ['Critical-damage bonuses apply at half their rate to critical skills in Renewal.']
    if 'bNonCritAtkRate' in combined_script:
        lines += ['Non-critical bonus excludes skills flagged to ignore it.']
    if 'bAutoSpellOnSkill' in combined_script:
        lines += ['Autocasts remain subject to server skill, target and equipment restrictions.']
    jobs, classes = item.get('Jobs'), item.get('Classes')
    if jobs is None:
        require(classes is None, 'Unreviewed class-only restriction')
        job_line = 'All jobs (subject to required level)'
    elif jobs == {'Spirit_Handler': True}:
        require(classes is None, 'Spirit Handler class-tier filter changed')
        job_line = 'Summoner family; no class-tier filter'
    else:
        require(len(jobs) == 1 and next(iter(jobs)) in JOB_LABELS and next(iter(jobs.values())) is True
                and classes == {'Fourth': True}, 'Unreviewed job/class restriction')
        job_line = JOB_LABELS[next(iter(jobs))] + ' (fourth class)'
    lines += ['_______________________', 'Type: Upper Headgear', 'Armor Level: ' + str(item['ArmorLevel']),
              'DEF: ' + str(item['Defense']), 'Required Level: ' + str(item['EquipLevelMin']),
              'Card Slots: ' + str(item['Slots']), f"Weight: {item.get('Weight', 0) / 10:g}",
              'Refineable: Yes', 'Gradable: Yes', 'Equip: ' + job_line]
    # Keep authored lines readable on the legacy narrow item-info panel.
    wrapped = []
    for line in lines:
        colored = line.startswith('^0055AA')
        plain = line[7:-7] if colored else line
        parts = textwrap.wrap(plain, width=64, break_long_words=False, break_on_hyphens=False)
        wrapped += ['^0055AA' + part + '^000000' if colored else part for part in parts]
    return wrapped


def provenance(target_ids, items, combos, skills):
    selected_scripts = [items[i]['Script'] for i in sorted(target_ids)] + list(combos.values())
    used_skills = sorted(set(re.findall(r'"([A-Z][A-Z0-9_]+)"', '\n'.join(selected_scripts))))
    return {
        'item_scripts': {str(i): sha(items[i]['Script']) for i in sorted(target_ids)},
        'combo_scripts': [{'ids': list(ids), 'sha256': sha(script)} for ids, script in sorted(combos.items())],
        'combo_partners': [{key: items[i].get(key) for key in ('Id', 'AegisName', 'Name', 'Locations')}
                           for i in sorted({i for ids in combos for i in ids} - target_ids)],
        'skills': {name: {key: skills[name].get(key) for key in ('Id', 'Name', 'Description', 'MaxLevel')}
                   for name in used_skills},
    }


def fragment(items, rows, combos, skills):
    lines = ['-- Clean-room descriptions derived from effective server item and combo Scripts.',
             '-- Exact twelve-record allowlist; no copied third-party effect descriptions.',
             '-- Existing reference-backed art; generic unidentified icon EpisodClear20.',
             '-- Optional additive import only; no active loader is modified.',
             'tbl_enchanttargets = {']
    for item_id, row in sorted(rows.items()):
        item = items[item_id]
        lines += [f'  [{item_id}] = {{',
                  '    unidentifiedDisplayName = "Unidentified Headgear",',
                  '    unidentifiedResourceName = "EpisodClear20",',
                  '    unidentifiedDescriptionName = { "Identify this headgear to view its name." },',
                  '    identifiedDisplayName = ' + json.dumps(item['Name']) + ',',
                  '    identifiedResourceName = ' + json.dumps(row['resource']) + ',',
                  '    identifiedDescriptionName = {']
        lines += ['      ' + json.dumps(line) + ',' for line in description(item, combos, items, skills)]
        lines += ['    },', f"    slotCount = {item['Slots']}, ClassNum = {item['View']}, costume = false", '  },']
    return '\n'.join(lines + ['}', ''])
