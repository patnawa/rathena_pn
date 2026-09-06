"""Druid item identity/import and compiled effect-boundary regressions.

Requires Python 3, PyYAML and g++. Item scripts use a deliberately restricted
expression subset translated to C++ for arithmetic tests; this is not the full
rAthena script VM or an attached-player equipment test.
"""
import collections
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

import yaml
from audit_enchant_upgrades import renewal_records
from audit_combat_bindings_test import braced_block


ROOT = Path(__file__).resolve().parents[2]
ITEM_PATH = 'db/import/druid_item_db.yml'
EXPECTED_IDS = {
    **{314271 + i: f'Automatic_Orb{99 + i}' for i in range(3)},
    **{314274 + i: f'Wolf_Orb_Skill_{52 + i}' for i in range(3)},
    **{314277 + i: f'Glacier_F_Orb_{192 + i}' for i in range(19)},
    **{314296 + i: f'Ice_F_Orb_Skill_{55 + i}' for i in range(3)},
    **{1002350 + i: f'Ice_F_Stone_Skill_{55 + i}' for i in range(3)},
}
FAMILY_SKILLS = [
    ['KR_DOUBLE_SLASH', 'KR_CHOP_CHOP'],
    ['KR_SHARPEN_HAIL', 'KR_SHARPEN_GUST'],
    ['KR_ICE_SPLASH', 'KR_THUNDERING_ORB', 'KR_EARTH_STAMP'],
]
ICE_EXTRA = [
    ['AT_ALPHA_CLAW', 'AT_FRENZY_FANG'],
    ['AT_PINION_SHOT', 'AT_QUILL_SPEAR'],
    ['AT_GLACIER_SHARD', 'AT_ROARING_PIERCER', 'AT_TERRA_HARVEST'],
]
GLACIER_SKILLS = [
    ['KR_CHOP_CHOP'], ['KR_DOUBLE_SLASH'], ['KR_SHARPEN_GUST'],
    ['KR_SHARPEN_HAIL'], ['KR_FEATHER_SPRINKLE'], ['KR_ICE_SPLASH'],
    ['KR_THUNDERING_ORB'], ['KR_THUNDERING_FOCUS'], ['KR_EARTH_DRILL'],
    ['KR_EARTH_STAMP'], ['AT_GLACIER_NOVA'], ['AT_GLACIER_SHARD'],
    ['AT_ROARING_PIERCER'], ['AT_TERRA_WAVE'], ['AT_TERRA_HARVEST'],
    ['AT_QUILL_SPEAR'], ['AT_PINION_SHOT'], ['AT_FRENZY_FANG'],
    ['AT_PRIMAL_CLAW', 'AT_FERAL_CLAW', 'AT_ALPHA_CLAW'],
]
# Independent expected totals at every refine level 0..20.
AUTOMATIC = [15, 15, 15, 15, 15, 15, 15, 15, 15, 18, 18, 25, 25, 25, 25, 25, 25, 25, 25, 25, 25]
WOLF = [15, 15, 15, 15, 15, 15, 15, 15, 15, 30, 30, 45, 45, 45, 45, 45, 45, 45, 45, 45, 45]
GLACIER_KARNOS = [20, 20, 20, 30, 30, 30, 40, 40, 40, 70, 70, 90, 100, 100, 100, 110, 110, 110, 120, 120, 120]
GLACIER_ALITEA = [10, 10, 10, 10, 15, 15, 15, 15, 20, 30, 30, 30, 35, 35, 35, 35, 40, 40, 40, 40, 45]
ICE = [15, 15, 15, 15, 15, 15, 15, 30, 30, 45, 45, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60]
ENHANCED = {
    'KR_THUNDERING_FOCUS_S': 'KR_THUNDERING_FOCUS',
    'KR_THUNDERING_ORB_S': 'KR_THUNDERING_ORB',
    'KR_THUNDERING_CALL_S': 'KR_THUNDERING_CALL',
    'AT_QUILL_SPEAR_S': 'AT_QUILL_SPEAR',
    'AT_ROARING_PIERCER_S': 'AT_ROARING_PIERCER',
    'AT_ROARING_CHARGE_S': 'AT_ROARING_CHARGE',
}


def translate_script(script):
    """Translate only this import's checked arithmetic/skill-bonus subset."""
    lines = []
    for line in script.splitlines():
        line = line.strip()
        if not line:
            continue
        bonus = re.fullmatch(r'bonus2 bSkillAtk,"([A-Z0-9_]+)",(\.@b|15);', line)
        if bonus:
            amount = 'b' if bonus[2] == '.@b' else bonus[2]
            lines.append(f'bonus[{json.dumps(bonus[1])}] += {amount};')
        elif line == '.@r = getrefine();':
            lines.append('int r = refine;')
        elif re.fullmatch(r'\.@b = [0-9 +*/().@r]+;', line):
            lines.append(line.replace('.@b', 'int b').replace('.@r', 'r'))
        elif re.fullmatch(r'if \(\.@r >= (7|9|11)\) \.@b \+= (3|7|10|15|20);', line):
            lines.append(line.replace('.@r', 'r').replace('.@b', 'b'))
        elif line in ('if (.@r >= 11) {', '}'):
            lines.append(line.replace('.@r', 'r'))
        else:
            raise ValueError(f'Unsupported item script statement: {line}')
    return '\n'.join(lines)


def expected_bonus(item_id, refine):
    extra = []
    if 314271 <= item_id <= 314273:
        skills, amount = FAMILY_SKILLS[item_id - 314271], AUTOMATIC[refine]
    elif 314274 <= item_id <= 314276:
        skills, amount = FAMILY_SKILLS[item_id - 314274], WOLF[refine]
    elif 314277 <= item_id <= 314295:
        skills = GLACIER_SKILLS[item_id - 314277]
        amount = (GLACIER_KARNOS if item_id <= 314286 else GLACIER_ALITEA)[refine]
    else:
        skills, amount = FAMILY_SKILLS[item_id - 314296], ICE[refine]
        if refine >= 11:
            extra = ICE_EXTRA[item_id - 314296]
    return {**dict.fromkeys(skills, amount), **dict.fromkeys(extra, 15)}


class DruidItemTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = yaml.load((ROOT / ITEM_PATH).read_text(), Loader=yaml.CSafeLoader)
        cls.items = {record['Id']: record for record in cls.data['Body']}

    def test_exact_31_id_name_mappings(self):
        self.assertEqual(len(self.data['Body']), 31)
        self.assertEqual({i: r['AegisName'] for i, r in self.items.items()}, EXPECTED_IDS)

    def test_import_is_renewal_only_and_does_not_override_existing_items(self):
        root = yaml.load((ROOT / 'db/item_db.yml').read_text(), Loader=yaml.CSafeLoader)
        imports = [entry for entry in root['Footer']['Imports'] if entry['Path'] == ITEM_PATH]
        self.assertEqual(imports, [{'Path': ITEM_PATH, 'Mode': 'Renewal'}])
        records = list(renewal_records(ROOT, 'db/item_db.yml'))
        counts = collections.Counter(record['Id'] for record in records)
        names = collections.Counter(record.get('AegisName') for record in records)
        for item_id, name in EXPECTED_IDS.items():
            with self.subTest(item_id=item_id):
                self.assertEqual(counts[item_id], 1)
                self.assertEqual(names[name], 1)

    def test_28_enchants_and_three_nonusable_materials_without_new_economy_rules(self):
        for item_id, record in self.items.items():
            with self.subTest(item_id=item_id):
                self.assertFalse(set(record) & {'Buy', 'Sell', 'Weight', 'Trade', 'Flags', 'Locations'})
                if item_id < 1000000:
                    self.assertEqual((record['Type'], record['SubType']), ('Card', 'Enchant'))
                    self.assertTrue(record['Script'].startswith('.@r = getrefine();'))
                else:
                    self.assertEqual(record['Type'], 'Etc')
                    self.assertNotIn('Script', record)
                    self.assertNotIn('SubType', record)

    def test_all_referenced_skills_are_loaded(self):
        loaded = {r['Name'] for r in renewal_records(ROOT, 'db/skill_db.yml') if 'Name' in r}
        for record in self.items.values():
            for skill in re.findall(r'bonus2 bSkillAtk,"([A-Z0-9_]+)"', record.get('Script', '')):
                with self.subTest(item=record['Id'], skill=skill):
                    self.assertIn(skill, loaded)
        for enhanced, parent in ENHANCED.items():
            self.assertIn(enhanced, loaded)
            self.assertIn(parent, loaded)

    def test_client_metadata_matches_server_names(self):
        path = ROOT / 'client-patch/druid_items/SystemEN/itemInfo_DruidItems.lua'
        text = path.read_text(encoding='ascii')
        pairs = re.findall(r'^add\((\d+), ("[^"\n]+"), \{', text, re.M)
        self.assertEqual(len(pairs), 31)
        self.assertEqual({int(i): json.loads(name) for i, name in pairs},
                         {i: item['Name'] for i, item in self.items.items()})
        self.assertEqual(text.count('ResourceName = "EpisodClear20"'), 2)
        self.assertIn('tbl_druiditems = items', text)

    def test_588_refine_cases_and_enhanced_bonus_canonicalization(self):
        source = '#include <cstdint>\n#include <map>\n#include <string>\n#include <iostream>\n#include <cassert>\n'
        source += 'using Bonus = std::map<std::string,int>;\nusing uint16 = uint16_t;\n'
        for item_id, record in self.items.items():
            if 'Script' in record:
                source += f'Bonus item_{item_id}(int refine) {{ Bonus bonus;\n'
                source += translate_script(record['Script']) + '\nreturn bonus; }\n'
        canonical = braced_block((ROOT / 'src/map/skill.cpp').read_text(), 'uint16 skill_dummy2skill_id(')
        symbols = sorted(set(re.findall(r'(?:case|return) ([A-Z][A-Z0-9_]+)', canonical)))
        source += 'enum Skill { ' + ','.join(symbols) + ' };\n' + canonical + '\nint main() {\n'
        checks = 0
        for item_id in range(314271, 314299):
            for refine in range(21):
                expected = expected_bonus(item_id, refine)
                cpp_map = '{' + ','.join('{' + json.dumps(skill) + ',' + str(value) + '}'
                                         for skill, value in sorted(expected.items())) + '}'
                source += f'{{ Bonus expected = {cpp_map}; assert(item_{item_id}({refine}) == expected); }}\n'
                checks += 1
        for enhanced, parent in ENHANCED.items():
            source += f'assert(skill_dummy2skill_id({enhanced}) == {parent});\n'
            source += f'assert(skill_dummy2skill_id({parent}) == {parent});\n'
        source += f'std::cout << "{checks} item/refine cases and 12 canonicalization checks passed\\n"; }}\n'
        with tempfile.TemporaryDirectory(prefix='rathena-druid-items-') as tmp:
            cpp, binary = Path(tmp) / 'probe.cpp', Path(tmp) / 'probe'
            cpp.write_text(source, encoding='ascii')
            subprocess.run([os.environ.get('CXX', 'g++'), '-std=c++17', '-O1', '-Wall', '-Wextra',
                            '-fsanitize=address,undefined', str(cpp), '-o', str(binary)], check=True)
            output = subprocess.check_output([str(binary)], text=True).strip()
        self.assertEqual(output, '588 item/refine cases and 12 canonicalization checks passed')


if __name__ == '__main__':
    unittest.main()
