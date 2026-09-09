#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  equipment_progression_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/equipment_progression_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Check effective progression imports, acquisition and matching client recipes.

Requires PyYAML. Runs offline without changing player records or client files.
"""
from pathlib import Path
import unittest

from audit_enchant_upgrades import ClientItemNames, renewal_records
from audit_initial_enchants import client_configuration, compare, server_configuration

ROOT = Path(__file__).resolve().parents[2]


class EquipmentProgression(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.items = {}
        for row in renewal_records(ROOT, 'db/item_db.yml'):
            cls.items.setdefault(row['Id'], {}).update(row)
        cls.names = {r['AegisName']: i for i, r in cls.items.items()}
        cls.enchants = server_configuration(ROOT)

    def test_tuning_opens_native_reform_for_existing_weapon(self):
        tuning = self.items[106250]
        self.assertEqual(tuning['AegisName'], 'Fron_Fix_AT_Axe')
        self.assertEqual(tuning['Type'], 'DelayConsume')
        self.assertEqual(tuning['Script'].strip(), 'item_reform();')
        rows = [r for r in renewal_records(ROOT, 'db/item_reform.yml')
                if r['Item'] == tuning['AegisName']]
        self.assertEqual(len(rows), 1)
        self.assertEqual(len(rows[0]['BaseItems']), 1)
        recipe = rows[0]['BaseItems'][0]
        self.assertEqual(self.names[recipe['BaseItem']], 520054)
        self.assertEqual(self.names[recipe['ResultItem']], 520055)
        self.assertEqual((recipe['MinimumRefine'], recipe['MaximumRefine']), (10, 20))
        self.assertEqual({self.names[x['Material']]: x['Amount'] for x in recipe['Materials']},
                         {1001996: 200, 1002007: 15, 1002008: 10, 1002009: 5, 1002331: 5})
        self.assertTrue(recipe.get('CardsAllowed', True))
        self.assertFalse(recipe.get('ClearSlots', False))
        self.assertFalse(recipe.get('RemoveEnchantgrade', False))
        self.assertEqual(recipe.get('ChangeRefine', 0), 0)
        self.assertNotIn('RandomOptionGroup', recipe)

    def test_both_daily_reward_boxes_can_supply_tuning(self):
        for group in ('AEGIS_105161', 'AEGIS_105162'):
            rows = {}
            for record in renewal_records(ROOT, 'db/item_group_db.yml'):
                if record['Group'] != group:
                    continue
                for sub in record.get('SubGroups', []):
                    self.assertEqual(sub['SubGroup'], 6)
                    for row in sub.get('List', []):
                        rows.setdefault(row['Index'], {}).update(row)
            tuning = [r for r in rows.values() if r['Item'] == 'Fron_Fix_AT_Axe']
            peers = [r for r in rows.values() if r['Item'].startswith('Fron_Fix_')]
            self.assertEqual(len(tuning), 1)
            self.assertEqual(len(peers), 20)
            self.assertEqual({r['Rate'] for r in peers}, {130})
            self.assertTrue(all(r['Item'] in self.names for r in rows.values()))

    def test_perfect_signet_choices_preserve_normal_route(self):
        group = self.enchants[142]
        self.assertEqual(group['Order'], [3, 2])
        self.assertEqual(group['Reset']['Chance'], 100000)
        slot = group['Slots'][2]
        choices = {'Signet_Of_' + s + '1' for s in ('Pow', 'Con', 'Spl', 'Sta', 'Crt', 'Wis')}
        self.assertEqual(set(slot['Perfect']), choices)
        for recipe in slot['Perfect'].values():
            self.assertEqual(recipe['Price'], 40000000)
            self.assertEqual(recipe['Materials'], {
                'Energy_Of_Spring': 200, 'Energy_Of_Summer': 200,
                'Energy_Of_Autumn': 200, 'Energy_Of_Winter': 200,
                'Circul_Of_Life': 900, 'Fruit_Of_Birth': 100,
                'Fruit_Of_Extinction': 100, 'Grace_Of_Spirit': 200})
        self.assertEqual(slot['Price'], 1000000)
        self.assertEqual(slot['Materials'], {'Circul_Of_Life': 10})
        for outcomes in slot['Enchants'].values():
            self.assertEqual(len(outcomes), 20)
            self.assertEqual(sum(outcomes.values()), 100000)
        token = group['Slots'][3]['Perfect']['Token_Of_Life']
        self.assertEqual(token['Price'], 500000)
        self.assertEqual(token['Materials']['Grace_Of_Spirit'], 10)
        self.assertEqual(len(token['Materials']), 8)

    def test_client_and_server_signet_recipes_agree(self):
        folder = ROOT / 'client-patch/enchant_repair/source'
        resolver = ClientItemNames(folder / 'ItemDBNameTbl.lub', ROOT)
        client = client_configuration(folder / 'EnchantList.lub', resolver)
        self.assertEqual(compare({142: client[142]}, {142: self.enchants[142]}), [])
        self.assertEqual(resolver('Fron_Fix_AT_Axe'), 'Fron_Fix_AT_Axe')


if __name__ == '__main__':
    unittest.main()
