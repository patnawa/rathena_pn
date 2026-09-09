# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  shadow_group128_enchant_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/shadow_group128_enchant_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Standalone group-128 overlay tests; this script does not wire the import.

Pass --client and --client-item-names for pinned client equality, and --native
to compile the actual upgrade selector with ASAN/UBSAN and exhaust all draws.
Absent optional inputs are reported as skipped, not counted as validation.
"""
import argparse
import copy
import hashlib
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import yaml
from audit_enchant_upgrades import (ClientItemNames, canonical, client_recipes,
                                    renewal_records, server_recipes)
from audit_initial_enchants import client_configuration, server_configuration


ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / 'db/import/shadow_group128_enchant.yml'
CLIENT = CLIENT_NAMES = None
NATIVE = False
CLIENT_SHA256 = '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d'
NAMES_SHA256 = '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496'
TARGETS = {
    'S_Full_Power_Armor', 'S_Full_Power_Shoes', 'S_Full_Rate_Armor', 'S_Full_Rate_Shoes',
    'S_Focusing_Pendant', 'S_Focusing_Earring', 'S_Stout_Pendant', 'S_Stout_Earring',
    'S_Full_Spell_Armor', 'S_Full_Spell_Shoes', 'S_Centering_Pendant', 'S_Centering_Earring',
    'S_Witty_Pendant', 'S_Witty_Earring',
}
STATS = {'Strength3', 'Dexterity3', 'Inteligence3', 'Agility3', 'Vitality3', 'Luck3'}
CATEGORIES = ('ATK', 'MATK', 'CRI', 'CAST', 'HP', 'SP')


def configurations(records):
    with patch('audit_initial_enchants.renewal_records', return_value=records):
        initial = server_configuration(ROOT)
    with patch('audit_enchant_upgrades.renewal_records', return_value=records):
        upgrades = server_recipes(ROOT)
    return initial, upgrades


def baseline_records():
    """Suppress only our overlay, including if a later reviewed batch wires it."""
    original = Path.read_text

    def without_overlay(path, *args, **kwargs):
        if path.resolve() == OVERLAY.resolve():
            return 'Header:\n  Type: ITEM_ENCHANT_DB\n  Version: 1\nBody: []\n'
        return original(path, *args, **kwargs)

    with patch.object(Path, 'read_text', without_overlay):
        return list(renewal_records(ROOT, 'db/item_enchant.yml'))


NATIVE_SOURCE = r'''
#include <cstdlib>
#include <iostream>
#include <unordered_map>
#include "map/enchant_upgrade.hpp"
static void require(bool ok) { if (!ok) std::exit(1); }
int main() {
    unsigned count;
    require(bool(std::cin >> count));
    require(count == 24);
    unsigned long draws = 0;
    for (unsigned i = 0; i < count; ++i) {
        auto recipe = std::make_shared<s_item_enchant_upgrade>();
        t_itemid first, second, material;
        uint32 first_weight, second_weight;
        uint16 amount;
        require(bool(std::cin >> recipe->enchant_item_id >> material >> amount
            >> first >> first_weight >> second >> second_weight));
        recipe->zeny = 0;
        recipe->materials = {{material, amount}};
        recipe->random_upgrades = {{first, first_weight}, {second, second_weight}};
        require(first != second && first_weight + second_weight == 100000);
        EnchantUpgradeMap ordinary = {{recipe->enchant_item_id, recipe}};
        PerfectEnchantUpgradeMap perfect;
        require(select_enchant_upgrade(ordinary, perfect, recipe->enchant_item_id, false, 0) == recipe);
        for (t_itemid target : {0u, first, second})
            require(!select_enchant_upgrade(ordinary, perfect, recipe->enchant_item_id, true, target));
        require(!select_enchant_upgrade(ordinary, perfect, recipe->enchant_item_id, false, second));
        require(!select_enchant_upgrade(ordinary, perfect, 0, false, 0));
        require(select_enchant_upgrade_result(*recipe, 0) == 0);
        require(select_enchant_upgrade_result(*recipe, 100001) == 0);
        std::unordered_map<t_itemid, uint32> frequency;
        for (uint32 roll = 1; roll <= 100000; ++roll) {
            ++frequency[select_enchant_upgrade_result(*recipe, roll)];
            ++draws;
        }
        require(frequency.size() == 2);
        require(frequency[first] == first_weight && frequency[second] == second_weight);
    }
    require(draws == 2400000);
    std::cout << "PASS: 24 exact native distributions, " << draws << " draws and request-mode isolation\n";
}
'''


class ShadowGroup128Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.overlay = yaml.load(OVERLAY.read_text(), Loader=yaml.CSafeLoader)
        cls.records = baseline_records()
        cls.before, cls.upgrades_before = configurations(cls.records)
        cls.after, cls.upgrades_after = configurations(cls.records + cls.overlay['Body'])
        cls.group = cls.after[128]
        cls.items = {r['AegisName']: r for r in renewal_records(ROOT, 'db/item_db.yml') if 'AegisName' in r}

    def test_standalone_schema_and_no_group_collision(self):
        self.assertEqual(self.overlay['Header'], {'Type': 'ITEM_ENCHANT_DB', 'Version': 1})
        self.assertEqual(len(self.overlay['Body']), 1)
        self.assertEqual(self.overlay['Body'][0]['Id'], 128)
        self.assertNotIn(128, self.before, 'Another active import already defines group 128; review collision')
        record = self.overlay['Body'][0]
        self.assertEqual(set(record), {'Id', 'TargetItems', 'MinimumRefine', 'MinimumEnchantgrade',
                                      'AllowRandomOptions', 'Reset', 'Order', 'Slots'})
        self.assertEqual(len(record['TargetItems']), 14)
        self.assertTrue(all(record['TargetItems'].values()))
        self.assertEqual([s['Slot'] for s in record['Slots']], [3, 2])
        self.assertEqual(set(record['Slots'][0]), {'Slot', 'PerfectEnchants'})
        self.assertEqual(set(record['Slots'][1]), {'Slot', 'PerfectEnchants', 'Upgrades'})

    def test_exact_targets_eligibility_slot_order_and_disabled_reset(self):
        self.assertEqual(self.group['Targets'], TARGETS)
        self.assertEqual(self.group['Order'], [3, 2])
        self.assertEqual(self.group['MinimumRefine'], 0)
        self.assertEqual(self.group['MinimumEnchantgrade'], 0)
        self.assertIs(self.group['AllowRandomOptions'], True)
        self.assertEqual(self.group['Reset'], {'Enabled': False, 'Chance': 0, 'Price': 0, 'Materials': {}})
        for name in TARGETS:
            self.assertEqual(self.items[name]['Type'], 'ShadowGear')
            self.assertLessEqual(self.items[name].get('Slots', 0), 2)

    def test_twelve_exact_initial_recipes(self):
        expected = {3: STATS, 2: {f'Shadow_{category}_1' for category in CATEGORIES}}
        total = 0
        for slot, names in expected.items():
            data = self.group['Slots'][slot]
            self.assertEqual(set(data['Perfect']), names)
            self.assertFalse(data['Enchants'])
            for recipe in data['Perfect'].values():
                self.assertEqual(recipe, {'Price': 0, 'Materials': {'S_Enchant_Essence': 5 if slot == 3 else 7}})
                total += 1
        self.assertEqual(total, 12)

    def test_twenty_four_exact_random_upgrade_recipes(self):
        ordinary = {k: v for k, v in self.upgrades_after[1].items() if k[0] == 128}
        expected_keys = {(128, 2, f'Shadow_{category}_{level}')
                         for category in CATEGORIES for level in range(1, 5)}
        self.assertEqual(set(ordinary), expected_keys)
        self.assertFalse(any(k[0] == 128 for k in self.upgrades_after[2]))
        for (group, slot, name), recipe in ordinary.items():
            category, level = name.rsplit('_', 1)
            level = int(level)
            low_level, low_weight, amount = {1: (1, 30000, 2), 2: (1, 40000, 3),
                                            3: (2, 50000, 5), 4: (3, 60000, 8)}[level]
            expected = {'Enchant': name, 'Price': 0,
                        'Materials': [{'Material': 'S_Enchant_Essence', 'Amount': amount}],
                        'RandomUpgrades': [{'Upgrade': f'{category}_{low_level}', 'Chance': low_weight},
                                           {'Upgrade': f'{category}_{level + 1}', 'Chance': 100000 - low_weight}]}
            self.assertEqual(canonical(recipe), canonical(expected))
            self.assertNotIn('Upgrade', recipe)

    def test_all_dependencies_exist_and_no_placeholder_items_needed(self):
        names = TARGETS | STATS | {'S_Enchant_Essence'} | {
            f'Shadow_{category}_{level}' for category in CATEGORIES for level in range(1, 6)}
        self.assertTrue(names.issubset(self.items))
        ids = [self.items[name]['Id'] for name in names]
        self.assertEqual(len(ids), len(set(ids)))
        self.assertEqual(self.items['S_Enchant_Essence']['Id'], 1001253)
        for name in names - TARGETS - {'S_Enchant_Essence'}:
            self.assertEqual(self.items[name]['Type'], 'Card')
            self.assertEqual(self.items[name]['SubType'], 'Enchant')

    def test_every_other_group_and_upgrade_is_preserved(self):
        restored = copy.deepcopy(self.after)
        del restored[128]
        self.assertEqual(restored, self.before)
        groups, ordinary, perfect = self.upgrades_after
        self.assertEqual(groups - {128}, self.upgrades_before[0])
        self.assertEqual({k: v for k, v in ordinary.items() if k[0] != 128}, self.upgrades_before[1])
        self.assertEqual(perfect, self.upgrades_before[2])

    def test_standalone_overlay_is_semantically_idempotent(self):
        self.assertEqual(configurations(self.records + self.overlay['Body'] * 2),
                         (self.after, self.upgrades_after))
        # Native target parser warns on repeated enabled targets; normal wiring
        # must import this file only once, despite semantic map idempotence.

    def test_exact_pinned_active_client_initial_and_upgrade_equality(self):
        if CLIENT is None:
            self.skipTest('Supply --client and --client-item-names for exact active-client verification')
        self.assertEqual(hashlib.sha256(CLIENT.read_bytes()).hexdigest(), CLIENT_SHA256)
        self.assertEqual(hashlib.sha256(CLIENT_NAMES.read_bytes()).hexdigest(), NAMES_SHA256)
        resolve = ClientItemNames(CLIENT_NAMES, ROOT)
        client = client_configuration(CLIENT, resolve)
        ordinary, perfect = client_recipes(CLIENT, resolve)
        self.assertEqual(self.group, client[128])
        dependencies = TARGETS | STATS | {'S_Enchant_Essence'} | {
            f'Shadow_{category}_{level}' for category in CATEGORIES for level in range(1, 6)}
        self.assertFalse(dependencies & resolve.unresolved)
        expected = {k: canonical(v) for k, v in ordinary.items() if k[0] == 128}
        actual = {k: canonical(v) for k, v in self.upgrades_after[1].items() if k[0] == 128}
        self.assertEqual(actual, expected)
        self.assertFalse(any(k[0] == 128 for k in perfect))

    def test_actual_native_selector_all_2400000_rolls(self):
        if not NATIVE:
            self.skipTest('Supply --native to compile and run the real C++ selector with ASAN/UBSAN')
        compiler = shutil.which('g++')
        self.assertIsNotNone(compiler, '--native requires g++')
        lines = ['24']
        material = self.items['S_Enchant_Essence']['Id']
        for key, recipe in sorted(self.upgrades_after[1].items()):
            if key[0] != 128:
                continue
            fields = [self.items[key[2]]['Id'], material, recipe['Materials'][0]['Amount']]
            for outcome in recipe['RandomUpgrades']:
                fields.extend([self.items[outcome['Upgrade']]['Id'], outcome['Chance']])
            lines.append(' '.join(map(str, fields)))
        with tempfile.TemporaryDirectory(prefix='shadow-group128-') as temporary:
            executable = Path(temporary) / 'native-selector-test'
            subprocess.run([compiler, '-std=c++17', '-O1', '-g', '-fsanitize=address,undefined',
                            '-fno-omit-frame-pointer', '-I', str(ROOT / 'src'), '-x', 'c++', '-',
                            '-o', str(executable)], input=NATIVE_SOURCE, text=True, check=True,
                           capture_output=True, timeout=60)
            result = subprocess.run([str(executable)], input='\n'.join(lines) + '\n', text=True,
                                    check=True, capture_output=True, timeout=60)
            self.assertIn('24 exact native distributions, 2400000 draws', result.stdout)
            print(result.stdout.strip())


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path)
    parser.add_argument('--client-item-names', type=Path)
    parser.add_argument('--native', action='store_true')
    args, remaining = parser.parse_known_args()
    if bool(args.client) != bool(args.client_item_names):
        parser.error('--client and --client-item-names must be supplied together')
    CLIENT, CLIENT_NAMES, NATIVE = args.client, args.client_item_names, args.native
    unittest.main(argv=[sys.argv[0], *remaining])
