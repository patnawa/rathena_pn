# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  audit_initial_enchants_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/audit_initial_enchants_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Synthetic client/database fixtures for initial-enchant comparison."""
import copy
import unittest
from unittest.mock import patch
from audit_initial_enchants import blank_group, blank_slot, client_configuration, compare, materials_overlay, server_configuration


class ClientPath:
    def __init__(self, text):
        self.text = text

    def read_text(self, encoding):
        return self.text


class InitialEnchantTests(unittest.TestCase):
    def test_missing_server_groups_cannot_disappear_from_comparison(self):
        expected = {128: blank_group()}
        expected[128]['Targets'] = {'Armor'}
        expected[128]['Slots'][1] = blank_slot()
        expected[128]['Slots'][1]['Perfect']['A'] = {'Price': 0, 'Materials': {}}
        issues = compare(expected, {999: blank_group()})
        self.assertEqual(issues, [{'kind': 'missing-server-group', 'key': [128],
                                 'client': {'targets': ['Armor'], 'normal_grade_tables': 0,
                                            'perfect_initial_recipes': 1}, 'server': None}])
        self.assertEqual(compare({}, {999: blank_group()}), [])

    def test_client_initial_reset_grade_and_perfect_declarations(self):
        lines = '''Table[1] = CreateEnchantInfo()
Table[1]:SetSlotOrder(3, 2)
Table[1]:AddTargetItem("Armor")
Table[1]:SetCondition(7, 1)
Table[1]:ApproveRandomOption(false)
Table[1]:SetReset(true, 50000, 5, {"Ore", 2})
Table[1].Slot[3]:SetRequire(10, {"Ore", 1})
Table[1].Slot[3]:SetSuccessRate(80000)
Table[1].Slot[3]:SetGradeBonus(1, 20000)
Table[1].Slot[3]:SetEnchant(1, "A", 1)
Table[1].Slot[3]:SetEnchant(1, "B", 99999)
Table[1].Slot[3]:AddPerfectEnchant("B", 100, {"Ore", 20})  \t'''
        group = client_configuration(ClientPath(lines), lambda n: n)[1]
        self.assertEqual(group['Order'], [3, 2])
        self.assertEqual(group['Targets'], {'Armor'})
        self.assertEqual((group['MinimumRefine'], group['MinimumEnchantgrade'], group['AllowRandomOptions']), (7, 1, False))
        self.assertEqual(group['Reset'], {'Enabled': True, 'Chance': 50000, 'Price': 5, 'Materials': {'Ore': 2}})
        self.assertEqual(group['Slots'][3]['Enchants'], {1: {'A': 1, 'B': 99999}})
        self.assertEqual(group['Slots'][3]['Bonus'], {1: 20000})
        self.assertEqual(group['Slots'][3]['Perfect']['B'], {'Price': 100, 'Materials': {'Ore': 20}})

    def test_unrecognized_client_syntax_fails(self):
        for text in ['Table[1] = SomethingElse()', 'Table[1] = CreateEnchantInfo()\nTable[1]:Unknown(1)']:
            with self.subTest(text=text), self.assertRaises(ValueError):
                client_configuration(ClientPath(text), lambda n: n)

    def test_material_overlay_preserves_and_removes(self):
        self.assertEqual(materials_overlay({'A': 3, 'B': 4}, [{'Material': 'A'}, {'Material': 'B', 'Amount': 0}, {'Material': 'C'}]),
                         {'A': 3, 'C': 1})

    def test_server_overlay_defaults_and_zero_probability(self):
        records = [{'Id': 1, 'TargetItems': {'Armor': True}, 'Order': [{'Slot': 3}],
                    'Reset': {'Chance': 50000, 'Price': 9, 'Materials': [{'Material': 'Ore', 'Amount': 2}]},
                    'Slots': [{'Slot': 3, 'EnchantgradeBonus': [{'Enchantgrade': 1, 'Chance': 5000}],
                               'Enchants': [{'Enchantgrade': 1, 'Items': [{'Item': 'A', 'Chance': 100000}]}]}]},
                   {'Id': 1, 'TargetItems': {'Armor': False, 'Robe': True}, 'Order': [{'Slot': 2}],
                    'Reset': {'Chance': 0}, 'Slots': [{'Slot': 3, 'Enchants': [{'Enchantgrade': 1, 'Items': [{'Item': 'A', 'Chance': 0}, {'Item': 'B', 'Chance': 100000}]}]}]}]
        with patch('audit_initial_enchants.renewal_records', return_value=records):
            group = server_configuration(None)[1]
        self.assertEqual(group['Targets'], {'Robe'})
        self.assertEqual(group['Order'], [2])
        self.assertEqual(group['Reset'], {'Enabled': False, 'Chance': 0, 'Price': 9, 'Materials': {'Ore': 2}})
        self.assertEqual(group['Slots'][3]['Chance'], 100000)
        self.assertEqual(group['Slots'][3]['Bonus'], {1: 5000})
        self.assertEqual(group['Slots'][3]['Enchants'][1], {'A': 0, 'B': 100000})

    def test_perfect_materials_merge_instead_of_replace(self):
        records = [{'Id': 1, 'Slots': [{'Slot': 1, 'PerfectEnchants': [{'Item': 'A', 'Price': 50, 'Materials': [{'Material': 'Ore', 'Amount': 5}]}]}]},
                   {'Id': 1, 'Slots': [{'Slot': 1, 'PerfectEnchants': [{'Item': 'A', 'Materials': [{'Material': 'Rune', 'Amount': 2}]}]}]}]
        with patch('audit_initial_enchants.renewal_records', return_value=records):
            config = server_configuration(None)[1]
        self.assertEqual(config['Slots'][1]['Perfect']['A'], {'Price': 50, 'Materials': {'Ore': 5, 'Rune': 2}})

    def test_mismatch_is_reported_at_correct_grade(self):
        expected = {1: blank_group()}
        expected[1]['Slots'][3] = blank_slot()
        expected[1]['Slots'][3]['Enchants'][1] = {'A': 100000}
        actual = copy.deepcopy(expected)
        actual[1]['Slots'][3]['Bonus'][2] = 50000  # another grade must not affect grade 1
        self.assertEqual(compare(expected, actual), [])
        actual[1]['Slots'][3]['Bonus'][1] = 5000
        self.assertEqual([x['kind'] for x in compare(expected, actual)], ['normal-bonus'])
        self.assertEqual(compare(expected, actual)[0]['key'], [1, 3, 1])

    def test_zero_weight_and_disabled_reset_costs_do_not_create_false_differences(self):
        expected = {1: blank_group()}
        expected[1]['Slots'][3] = blank_slot()
        expected[1]['Slots'][3]['Enchants'][0] = {'A': 100000}
        actual = copy.deepcopy(expected)
        actual[1]['Slots'][3]['Enchants'][0]['B'] = 0
        actual[1]['Reset']['Price'] = 999
        self.assertEqual(compare(expected, actual), [])


if __name__ == '__main__':
    unittest.main()
