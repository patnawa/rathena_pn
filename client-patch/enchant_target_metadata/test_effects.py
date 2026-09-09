# ============================================================================
#  PN  /  CLIENT TOOLING
#  test_effects.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/enchant_target_metadata/test_effects.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Focused renderer regressions; does not alter active files."""
import copy
import json
from pathlib import Path
import sys
import subprocess
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/ci'))
from effects import amount, description, effect_lines, effective, fragment, provenance, statements
from run_native_bonus_vm_test import validate_output


class EffectsTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.package = Path(__file__).resolve().parent
        cls.manifest = json.loads((cls.package / 'manifest.json').read_text())
        cls.rows = {row['id']: row for row in cls.manifest['items']}
        cls.items, cls.combos, cls.skills = effective(ROOT, set(cls.rows))

    def test_entire_committed_fragment_matches_fresh_effective_server(self):
        self.assertEqual(fragment(self.items, self.rows, self.combos, self.skills),
                         (self.package / 'SystemEN/itemInfo_EnchantTargets.lua').read_text())
        self.assertEqual(provenance(set(self.rows), self.items, self.combos, self.skills),
                         self.manifest['effect_provenance'])
        self.assertEqual(len(self.combos), 15)

    def test_every_basic_scalar_changes_derived_description(self):
        original = self.items[401055]
        before = description(original, self.combos, self.items, self.skills)
        for field in ('Defense', 'Slots', 'ArmorLevel', 'EquipLevelMin', 'Weight'):
            item = copy.deepcopy(original)
            item[field] += 1
            self.assertNotEqual(before, description(item, self.combos, self.items, self.skills), field)
        for field in ('Refineable', 'Gradable'):
            item = copy.deepcopy(original)
            item[field] = False
            with self.assertRaises(AssertionError):
                description(item, self.combos, self.items, self.skills)

    def test_weight_default_and_spirit_handler_actual_class_filter(self):
        text = '\n'.join(description(self.items[401060], self.combos, self.items, self.skills))
        self.assertIn('Weight: 0', text)
        self.assertIn('Summoner family; no class-tier filter', text)
        for item_id in set(self.rows) - {401055, 401056, 401057, 401060}:
            self.assertIn('(fourth class)', '\n'.join(description(self.items[item_id], self.combos, self.items, self.skills)))

    def test_cardinal_alias_is_not_double_described(self):
        text = '\n'.join(effect_lines(self.items[401118]['Script'], self.skills))
        self.assertEqual(text.count('Arbitrium damage +5% per 4 refine levels.'), 1)
        self.assertNotIn('Arbitrium Attack', text)
        with self.assertRaises(AssertionError):
            effect_lines('bonus2 bSkillAtk,"CD_ARBITRIUM_ATK",5;', self.skills)

    def test_hyper_only_effective_parent_key_is_accepted(self):
        script = self.combos[(401117, 500134)]
        self.assertIn('Chain Lightning damage +2% per combined', '\n'.join(effect_lines(script, self.skills)))
        with self.assertRaises(AssertionError):
            effect_lines(script.replace('bSkillAtk,"WL_CHAINLIGHTNING",', 'bSkillAtk,"WL_CHAINLIGHTNING_ATK",'), self.skills)

    def test_cumulative_nested_thresholds_and_floor_units(self):
        events = statements('if (.@r>=7) {\nif (.@r>=9) {\nbonus bPAtk,5;\n}\n}\n')
        self.assertEqual(events, [((('refine', 9),), ['bPAtk', '5'])])
        self.assertEqual(amount('4*(.@r/2)', '%'), '+4% per 2 refine levels')
        self.assertEqual(amount('-500', ' sec', 1000), '-0.5 sec')

    def test_fail_closed_on_unknown_statements_expressions_conditions(self):
        for script in ('bonus bUnknown,5;', 'heal 1,2;', 'bonus bPow,readparam(bStr);',
                       'if (BaseLevel>=240) {\nbonus bPow,5;\n}', 'if (.@r>=7) {\nbonus bPow,5;'):
            with self.assertRaises(AssertionError, msg=script):
                effect_lines(script, self.skills)

    def test_race_exclusion_must_be_exact(self):
        script = self.items[401055]['Script']
        for altered in (script.replace('RC_Player_Doram', 'RC_DemiHuman'),
                        script.replace('RC_Player_Human,-3', 'RC_Player_Human,-2')):
            with self.assertRaises(AssertionError):
                effect_lines(altered, self.skills)

    def test_autocast_gate_and_probability_are_not_silently_changed(self):
        script = self.combos[(401119, 560086)]
        text = '\n'.join(effect_lines(script, self.skills))
        for expected in ('combined crown/right-hand weapon refine at least 24', 'crown Grade A',
                         'right-hand weapon Grade A', 'Tiger Cannon learned at Lv. 10',
                         '100% trigger chance to autocast Lv. 10 Tiger Cannon'):
            self.assertIn(expected, text)
        with self.assertRaises(AssertionError):
            effect_lines(script.replace(',10,1000;', ',10,999;'), self.skills)

    def test_native_zero_exit_warnings_and_missing_clean_teardown_are_rejected(self):
        clean = 'Memory manager: No memory leaks found.\nNATIVE_CROWN_BONUS_VM_OK executions=44205\n'
        validate_output(subprocess.CompletedProcess([], 0, clean, ''))
        for warning in ('[Warning]: allocator problem', '[Error]: bad free',
                        'Memory manager: freed invalid pointer', 'AddressSanitizer: fault',
                        'runtime error: misaligned pointer', 'double free'):
            with self.assertRaises(AssertionError, msg=warning):
                validate_output(subprocess.CompletedProcess([], 0, clean, warning))
        with self.assertRaises(AssertionError):
            validate_output(subprocess.CompletedProcess([], 0, 'NATIVE_CROWN_BONUS_VM_OK executions=44205', ''))


if __name__ == '__main__':
    unittest.main()
