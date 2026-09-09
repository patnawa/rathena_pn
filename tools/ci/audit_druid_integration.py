#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_druid_integration.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_druid_integration.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check Druid engine/database wiring; not an in-game combat certification.

Requires PyYAML. Optional --compare-ref verifies only job-mask additions changed
pre-existing equipment records relative to the supplied pre-integration commit.
"""
import argparse
import copy
import json
import re
import subprocess
from pathlib import Path

import yaml

from audit_enchant_upgrades import renewal_records


JOBS = ('Druid', 'Baby_Druid', 'Karnos', 'Baby_Karnos', 'Alitea')
PASSIVES = {
    'DR_BEASTY_NOSE', 'DR_SHARPE_EYES', 'DR_NATURE_LOGIC',
    'KR_WOLF_INSTINCT', 'KR_RAPTORIAL_INSTINCT', 'KR_EARTH_BUD',
    'KR_NATURE_VIGOUR', 'AT_SIXTH_SENSE', 'AT_NATURE_AID',
}
ENHANCED = {
    'KR_THUNDERING_FOCUS_S': 'KR_THUNDERING_FOCUS',
    'KR_THUNDERING_ORB_S': 'KR_THUNDERING_ORB',
    'KR_THUNDERING_CALL_S': 'KR_THUNDERING_CALL',
    'AT_QUILL_SPEAR_S': 'AT_QUILL_SPEAR',
    'AT_ROARING_PIERCER_S': 'AT_ROARING_PIERCER',
    'AT_ROARING_CHARGE_S': 'AT_ROARING_CHARGE',
}


def alias_function(source):
    """Extract the actual side-effect-free native switch, never reimplement it."""
    start = source.index('uint16 skill_dummy2skill_id(uint16 skill_id) {')
    opening = source.index('{', start)
    depth = 1
    for pos in range(opening + 1, len(source)):
        depth += (source[pos] == '{') - (source[pos] == '}')
        if depth == 0:
            return source[start:pos + 1]
    raise ValueError('Unterminated skill_dummy2skill_id definition')


def emit_alias_test(root):
    source = alias_function(read(root, 'src/map/skill.cpp'))
    print('#include <cassert>\n#include <cstdio>\n#include "map/skill.hpp"')
    print(source)
    print('int main() {')
    controls = {'NPC_LOCKON_LASER_ATK': 'NPC_LOCKON_LASER',
                'AG_CRIMSON_ARROW_ATK': 'AG_CRIMSON_ARROW',
                'AB_DUPLELIGHT_MAGIC': 'AB_DUPLELIGHT',
                'EM_ELEMENTAL_BUSTER_FIRE': 'EM_ELEMENTAL_BUSTER'}
    for variant, base in (ENHANCED | controls).items():
        print(f'assert(skill_dummy2skill_id({variant}) == {base});')
    records = keyed(root, 'db/skill_db.yml', 'Id')
    for item_id in range(6524, 6608):
        name = records[item_id]['Name']
        print(f'assert(skill_dummy2skill_id({name}) == {ENHANCED.get(name, name)});')
    print('std::puts("88 native Druid/legacy skill alias checks passed");\n}')


def read(root, relative):
    return (root / relative).read_text(encoding='utf-8')


def keyed(root, relative, key):
    records = {}
    for row in renewal_records(root, relative):
        records.setdefault(row[key], {}).update(row)
    return records


def factory_errors(factory, constructors, skills):
    """Require each concrete factory result to carry its requested skill ID."""
    errors = []
    cases = set()
    pattern = r'((?:\s*case\s+\w+\s*:)+)\s*return std::make_unique<(\w+)>\(([^;]*)\);'
    for match in re.finditer(pattern, factory):
        names = re.findall(r'case\s+(\w+)\s*:', match[1])
        implementation = match[2]
        for name in names:
            cases.add(name)
            if implementation == 'StatusSkillImpl':
                if not skills.get(name, {}).get('Status'):
                    errors.append(f'{name}: generic status implementation lacks Status database field')
            elif constructors.get(implementation) != name:
                errors.append(f'{name}: {implementation} carries {constructors.get(implementation)}')
    for name in skills:
        if name not in PASSIVES and name != 'AT_FLIP_FLAP_TARGET' and name not in cases:
            errors.append(f'{name}: missing factory implementation')
    return errors, cases


def audit(root, compare_ref=None):
    errors = []
    all_skills = keyed(root, 'db/skill_db.yml', 'Id')
    druid = {row['Name']: row for item_id, row in all_skills.items() if 6524 <= item_id <= 6607}
    if sorted(row['Id'] for row in druid.values()) != list(range(6524, 6608)):
        errors.append('Druid skill IDs must cover 6524..6607 exactly once')
    enum = read(root, 'src/map/skill.hpp')
    match = re.search(r'\bDR_WEREWOLF\s*=\s*6524,(.*?)\bHLIF_HEAL\s*=', enum, re.S)
    enum_names = ['DR_WEREWOLF'] + re.findall(r'\b((?:DR|KR|AT)_\w+)\s*,', match[1]) if match else []
    for item_id, name in enumerate(enum_names, 6524):
        if name not in druid or druid[name]['Id'] != item_id:
            errors.append(f'{name}: enum/database skill ID mismatch at {item_id}')
    if len(enum_names) != 84:
        errors.append(f'Expected 84 Druid skill enum entries, found {len(enum_names)}')
    limit = int(re.search(r'#define MAX_SKILL\s+(\d+)', read(root, 'src/common/mmo.hpp'))[1])
    if len(all_skills) > limit:
        errors.append(f'{len(all_skills)} skills exceed MAX_SKILL {limit}')

    constructors = {}
    for path in (root / 'src/map/skills/druid').glob('*.cpp'):
        for match in re.finditer(r'(Skill\w+)::\1\(\)\s*:\s*\w+\(((?:DR|KR|AT)_\w+)\)', path.read_text()):
            constructors[match[1]] = match[2]
    factory = read(root, 'src/map/skills/druid/skill_factory_druid.cpp')
    factory_problems, active = factory_errors(factory, constructors, druid)
    errors.extend(factory_problems)
    status_source = read(root, 'src/map/status.cpp')
    for name in PASSIVES:
        if not re.search(r'pc_checkskill\(sd,\s*' + name + r'\)', status_source):
            errors.append(f'{name}: missing passive calculation hook')

    pc_source = read(root, 'src/map/pc.cpp')
    change = pc_source[pc_source.index('bool pc_jobchange('):]
    class_assignment = change.index('sd->status.class_ = job;')
    for status in ('SC_WEREWOLF', 'SC_WERERAPTOR'):
        cleanup = f'status_change_end(sd, {status});'
        if cleanup not in change[:class_assignment]:
            errors.append(f'{status}: job change must end transformation before replacing the class')
    if '( ( class_ & MAPID_SECONDMASK ) == MAPID_KARNOS && !pc_is_trait_job(class_) )' not in pc_source:
        errors.append('Alitea must not use the lower-stage extended stat cap')

    aliases = alias_function(read(root, 'src/map/skill.cpp'))
    for variant, base in ENHANCED.items():
        if not re.search(r'case\s+' + variant + r'\s*:\s*return\s+' + base + r'\s*;', aliases):
            errors.append(f'{variant}: missing equipment-bonus alias to {base}')
    if 'bonus2 bSkillAtk, "AT_QUILL_SPEAR", 30;' not in read(root, 'db/re/status.yml'):
        errors.append('Apex Phase Quill Spear bonus must use the normalized base skill')

    statuses = keyed(root, 'db/status.yml', 'Status')
    for name, skill in druid.items():
        required = list(skill.get('Requires', {}).get('Status', {}))
        if skill.get('Status'):
            required.append(skill['Status'])
        for status in required:
            if status not in statuses:
                errors.append(f'{name}: missing status {status}')

    trees = keyed(root, 'db/skill_tree.yml', 'Job')
    def inherited(job, visiting=None):
        visiting = set() if visiting is None else visiting
        if job in visiting:
            raise ValueError(f'Skill tree inheritance cycle at {job}')
        visiting.add(job)
        tree = trees[job]
        result = {}
        for parent, enabled in tree.get('Inherit', {}).items():
            if enabled:
                result.update(inherited(parent, visiting))
        result.update({row['Name']: row for row in tree.get('Tree', [])})
        visiting.remove(job)
        return result
    for job in JOBS:
        tree = inherited(job)
        for name, skill in tree.items():
            if name not in druid:
                continue
            if skill['MaxLevel'] > druid[name]['MaxLevel']:
                errors.append(f'{job}/{name}: tree exceeds skill database max level')
            for requirement in skill.get('Requires', []):
                required = tree.get(requirement['Name'])
                if required is None or required['MaxLevel'] < requirement['Level']:
                    errors.append(f'{job}/{name}: unreachable prerequisite {requirement}')

    caps = {job: {} for job in JOBS}
    for row in renewal_records(root, 'db/job_stats.yml'):
        for job in JOBS:
            if row.get('Jobs', {}).get(job):
                for field in ('MaxBaseLevel', 'MaxJobLevel'):
                    if field in row:
                        caps[job][field] = row[field]
    expected = ((99, 70), (99, 70), (200, 70), (200, 70), (275, 60))
    for job, pair in zip(JOBS, expected):
        actual = caps[job]
        if (actual.get('MaxBaseLevel'), actual.get('MaxJobLevel')) != pair:
            errors.append(f'{job}: unexpected level caps {actual}; expected {pair}')

    equipment_count = None
    if compare_ref:
        before_text = subprocess.check_output(['git', 'show', f'{compare_ref}:db/re/item_db_equip.yml'], cwd=root, text=True)
        before = {r['Id']: r for r in yaml.load(before_text, Loader=yaml.CSafeLoader)['Body']}
        after = keyed(root, 'db/re/item_db_equip.yml', 'Id')
        equipment_count = 0
        if before.keys() != after.keys():
            errors.append('Equipment ID set changed during job-mask integration')
        for item_id in before.keys() & after.keys():
            original, modified = before[item_id], after[item_id]
            if original == modified:
                continue
            equipment_count += 1
            without_jobs = copy.deepcopy(modified)
            jobs = without_jobs.get('Jobs', {})
            for job in ('Druid', 'Karnos'):
                if job in jobs and job not in original.get('Jobs', {}):
                    if jobs.pop(job, None) is not True:
                        errors.append(f'Equipment {item_id}: missing {job} compatibility flag')
            if without_jobs != original:
                errors.append(f'Equipment {item_id}: unrelated fields changed')
        if equipment_count != 205:
            errors.append(f'Expected 205 expanded equipment masks, found {equipment_count}')
    return {'skills': len(druid), 'active_factory_skills': len(active), 'passives': len(PASSIVES),
            'effective_skill_count': len(all_skills), 'skill_capacity': limit,
            'job_caps': caps, 'equipment_masks': equipment_count, 'errors': errors}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--compare-ref')
    parser.add_argument('--emit-alias-test', action='store_true', help='Print native test using the actual extracted alias function')
    args = parser.parse_args()
    if args.emit_alias_test:
        emit_alias_test(args.root)
        return
    result = audit(args.root, args.compare_ref)
    print(json.dumps(result, indent=2))
    raise SystemExit(bool(result['errors']))


if __name__ == '__main__':
    main()
