# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  druid_item_enchant_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_item_enchant_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Druid initial-enchant overlay regressions, with optional active-client comparison.

Run with --client and --client-item-names to execute every test. Without those
external client files, only the repository-contained checks run; client checks
are explicitly skipped. Neither the client nor the repository is modified.
"""
import argparse
import copy
import hashlib
from pathlib import Path
import re
import sys
import unittest
from unittest.mock import patch

import yaml
from audit_enchant_upgrades import ClientItemNames, renewal_records, server_recipes
from audit_initial_enchants import client_configuration, server_configuration
from druid_item_db_test import EXPECTED_IDS


ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / 'db/import/druid_item_enchant.yml'
CLIENT = None
CLIENT_NAMES = None
CLIENT_SHA256 = '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d'
EXPECTED_TARGETS = {
    (1, 1): {f'Wolf_Orb_Skill_{i}' for i in range(52, 55)},
    (26, 1): {f'Ice_F_Orb_Skill_{i}' for i in range(55, 58)},
    (31, 1): {f'Glacier_F_Orb_{i}' for i in range(192, 202)},
    (44, 2): {f'Automatic_Orb{i}' for i in range(99, 102)},
    (44, 1): {f'Automatic_Orb{i}' for i in range(99, 102)},
    (47, 3): {f'Glacier_F_Orb_{i}' for i in range(192, 211)},
    (47, 2): {f'Glacier_F_Orb_{i}' for i in range(192, 211)},
    (137, 1): {f'Wolf_Orb_Skill_{i}' for i in range(52, 55)},
}
# Keep the original six-group / 63-recipe scope explicit. The shared overlay
# now also contains exactly two independently sourced Clock Tower Gear recipes.
GEAR_TARGETS = {(24, 2): {'Gear_AT1', 'Gear_AT2'}}
ALL_EXPECTED_TARGETS = {**EXPECTED_TARGETS, **GEAR_TARGETS}
ALL_EXPECTED_IDS = {**EXPECTED_IDS, 314269: 'Gear_AT2', 314270: 'Gear_AT1'}


def configurations_without_overlay():
    """Use the actual import readers, suppressing only this new file's body."""
    original = Path.read_text

    def without_overlay(path, *args, **kwargs):
        if path.resolve() == OVERLAY.resolve():
            return 'Header:\n  Type: ITEM_ENCHANT_DB\n  Version: 1\nBody: []\n'
        return original(path, *args, **kwargs)

    with patch.object(Path, 'read_text', without_overlay):
        return server_configuration(ROOT), server_recipes(ROOT)


def expected_requirements(group, slot, name):
    if group == 1:
        return {'Price': 10000000, 'Materials': {'Ep18_Amethyst_Fragment': 2500}}
    if group == 137:
        return {'Price': 0, 'Materials': {'Sp_Amethyst_Fragment': 1}}
    if group == 44:
        return {'Price': 0, 'Materials': {'BarMealTicket': 25}}
    if group == 26:
        return {'Price': 0, 'Materials': {name.replace('_Orb_', '_Stone_'): 1}}
    if group == 24:
        return {'Price': 0, 'Materials': {
            'ClockTower_Gear': 150, 'Shadowdecon': 150, 'Zelunium': 150}}
    if group == 47:
        amounts = [10, 15, 25] if slot == 3 else [15, 20, 35]
        return {'Price': 0, 'Materials': {f'EP19_S_F_{i}_Extract': amount
                                         for i, amount in enumerate(amounts, 1)}}
    additional = {
        192: {'Sharpened_Cuspid': 150, 'Posionous_Canine': 150},
        193: {'Claw_Of_Desert_Wolf': 300},
        194: {'Feather': 150, 'Feather_Of_Birds': 150},
        195: {'Bill_Of_Birds': 150, 'Golden_Feather': 150},
        196: {'Blade_Of_Pinwheel': 300},
        197: {'Ice_Piece': 150, 'Ice_Heart': 150},
        198: {'Lantern': 150, 'Brilliant_Jelly': 150},
        199: {'Cloud_Piece': 300},
        200: {'Grit': 300},
        201: {'Starsand_Of_Witch': 150, 'Browny_Root': 150},
    }
    return {'Price': 0, 'Materials': {**{f'Snow_F_Stone{i}': 25 for i in (1, 2, 3)},
                                     **additional[int(name.rsplit('_', 1)[1])]}}


class DruidEnchantTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.overlay = yaml.load(OVERLAY.read_text(), Loader=yaml.CSafeLoader)
        cls.before, cls.upgrades_before = configurations_without_overlay()
        cls.after = server_configuration(ROOT)
        cls.upgrades_after = server_recipes(ROOT)

    def test_one_renewal_import(self):
        root = yaml.load((ROOT / 'db/item_enchant.yml').read_text(), Loader=yaml.CSafeLoader)
        imports = [entry for entry in root['Footer']['Imports']
                   if entry['Path'] == 'db/import/druid_item_enchant.yml']
        self.assertEqual(imports, [{'Path': 'db/import/druid_item_enchant.yml', 'Mode': 'Renewal'}])

    def test_overlay_schema_changes_only_allowed_fields(self):
        self.assertEqual(self.overlay['Header'], {'Type': 'ITEM_ENCHANT_DB', 'Version': 1})
        original_groups = [r for r in self.overlay['Body'] if r['Id'] != 24]
        self.assertEqual(len(original_groups), 6)
        self.assertEqual({r['Id'] for r in original_groups}, {1, 26, 31, 44, 47, 137})
        self.assertEqual(len(self.overlay['Body']), 7)
        self.assertEqual({r['Id'] for r in self.overlay['Body']}, {1, 24, 26, 31, 44, 47, 137})
        actual = {}
        for group in self.overlay['Body']:
            self.assertEqual(set(group), {'Id', 'Slots'})
            for slot in group['Slots']:
                key = (group['Id'], slot['Slot'])
                self.assertNotIn(key, actual)
                allowed = {'Slot', 'PerfectEnchants'}
                if key == (1, 1):
                    allowed.add('Enchants')
                self.assertEqual(set(slot), allowed)
                names = [r['Item'] for r in slot['PerfectEnchants']]
                self.assertEqual(len(names), len(set(names)))
                actual[key] = set(names)
                for recipe in slot['PerfectEnchants']:
                    self.assertEqual(set(recipe), {'Item', 'Price', 'Materials'})
        original_recipes = {key: names for key, names in actual.items() if key[0] != 24}
        self.assertEqual(original_recipes, EXPECTED_TARGETS)
        self.assertEqual(sum(map(len, original_recipes.values())), 63)
        self.assertEqual({key: names for key, names in actual.items() if key[0] == 24}, GEAR_TARGETS)
        self.assertEqual(actual, ALL_EXPECTED_TARGETS)
        self.assertEqual(sum(map(len, actual.values())), 65)

    def test_exact_pinned_costs_and_all_dependencies_exist(self):
        items = {r['AegisName']: r for r in renewal_records(ROOT, 'db/item_db.yml') if 'AegisName' in r}
        for (group, slot), names in ALL_EXPECTED_TARGETS.items():
            for name in names:
                with self.subTest(group=group, slot=slot, item=name):
                    recipe = self.after[group]['Slots'][slot]['Perfect'][name]
                    self.assertEqual(recipe, expected_requirements(group, slot, name))
                    self.assertEqual(items[name]['Type'], 'Card')
                    self.assertEqual(items[name]['SubType'], 'Enchant')
                    for material, amount in recipe['Materials'].items():
                        self.assertIn(material, items)
                        self.assertGreater(amount, 0)
                        self.assertLessEqual(amount, 30000)

    def test_gray_wolf_exact_distribution_and_only_one_grade(self):
        expected = {'Wolf_Orb_R_Reject_1': 1830, 'Wolf_Orb_R_Reject_2': 500,
                    'Wolf_Orb_R_Reject_3': 100, 'Wolf_Orb_Force': 1774,
                    **{f'Wolf_Orb_Skill_{i}': 1774 for i in range(1, 55)}}
        actual = self.after[1]['Slots'][1]['Enchants'][0]
        self.assertEqual(actual, expected)
        self.assertEqual(len(actual), 58)
        self.assertEqual(sum(actual.values()), 100000)
        self.assertTrue(set(self.before[1]['Slots'][1]['Enchants'][0]).issubset(actual))
        group = next(r for r in self.overlay['Body'] if r['Id'] == 1)
        self.assertEqual([g['Enchantgrade'] for g in group['Slots'][0]['Enchants']], [0])

    def test_every_other_initial_enchant_setting_is_unchanged(self):
        restored = copy.deepcopy(self.after)
        for (group, slot), names in ALL_EXPECTED_TARGETS.items():
            for name in names:
                self.assertNotIn(name, self.before[group]['Slots'][slot]['Perfect'])
                del restored[group]['Slots'][slot]['Perfect'][name]
        restored[1]['Slots'][1]['Enchants'][0] = self.before[1]['Slots'][1]['Enchants'][0]
        # Covers every group, target list, slot/order, eligibility, grade bonus,
        # reset, normal price/material and every previous initial recipe.
        self.assertEqual(restored, self.before)

    def test_all_ordinary_and_guaranteed_upgrades_are_unchanged(self):
        self.assertEqual(self.upgrades_after, self.upgrades_before)

    def test_all_effective_probability_tables_still_total_100000(self):
        tables = 0
        for group in self.after.values():
            for slot in group['Slots'].values():
                for outcomes in slot['Enchants'].values():
                    self.assertEqual(sum(outcomes.values()), 100000)
                    self.assertTrue(all(0 <= value <= 100000 for value in outcomes.values()))
                    tables += 1
        self.assertEqual(tables, 339)

    def test_reapplying_overlay_is_idempotent(self):
        records = list(renewal_records(ROOT, 'db/item_enchant.yml'))
        with patch('audit_initial_enchants.renewal_records', return_value=records + self.overlay['Body']):
            self.assertEqual(server_configuration(ROOT), self.after)
        with patch('audit_enchant_upgrades.renewal_records', return_value=records + self.overlay['Body']):
            self.assertEqual(server_recipes(ROOT), self.upgrades_after)

    def test_active_client_recipes_and_distribution_match_exactly(self):
        if CLIENT is None or CLIENT_NAMES is None:
            self.skipTest('Pass --client and --client-item-names for the required active-client comparison')
        self.assertEqual(hashlib.sha256(CLIENT.read_bytes()).hexdigest(), CLIENT_SHA256)
        resolve = ClientItemNames(CLIENT_NAMES, ROOT)
        client = client_configuration(CLIENT, resolve)
        verified = set(ALL_EXPECTED_IDS.values())
        selected = {(group, slot): {name for name in data['Perfect'] if name in verified}
                    for group, cfg in client.items() for slot, data in cfg['Slots'].items()
                    if any(name in verified for name in data['Perfect'])}
        self.assertEqual(selected, ALL_EXPECTED_TARGETS)
        for (group, slot), names in selected.items():
            for name in names:
                with self.subTest(group=group, slot=slot, item=name):
                    expected = client[group]['Slots'][slot]['Perfect'][name]
                    self.assertNotIn(name, resolve.unresolved)
                    self.assertFalse(set(expected['Materials']) & resolve.unresolved)
                    self.assertEqual(self.after[group]['Slots'][slot]['Perfect'][name], expected)
        outcomes = client[1]['Slots'][1]['Enchants'][0]
        self.assertFalse(set(outcomes) & resolve.unresolved)
        self.assertEqual(self.after[1]['Slots'][1]['Enchants'][0], outcomes)

    def test_original_client_names_already_contain_all_verified_identities(self):
        if CLIENT_NAMES is None:
            self.skipTest('Pass --client-item-names to verify existing client identities')
        resolve = ClientItemNames(CLIENT_NAMES, ROOT)
        for item_id, name in ALL_EXPECTED_IDS.items():
            self.assertEqual(resolve.client[name], item_id)
            self.assertEqual(resolve.server[item_id], name)
            self.assertEqual(resolve(name), name)
        self.assertFalse(resolve.unresolved)
        unknown = 'UNVERIFIED_DRUID_ITEM_12345'
        resolve(unknown)
        self.assertIn(unknown, resolve.unresolved)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path)
    parser.add_argument('--client-item-names', type=Path)
    args, remaining = parser.parse_known_args()
    if bool(args.client) != bool(args.client_item_names):
        parser.error('--client and --client-item-names must be supplied together')
    CLIENT, CLIENT_NAMES = args.client, args.client_item_names
    unittest.main(argv=[sys.argv[0], *remaining])
