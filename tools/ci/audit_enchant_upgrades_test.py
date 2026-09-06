"""Synthetic regression tests; no proprietary GRFs or Lua runtime required."""
import copy
import struct
import unittest
from unittest.mock import patch

from audit_enchant_upgrades import ClientItemNames, canonical, client_recipes, merge_recipe, workshop_contents
from lua51_literal_table import literal_tables


def u32(value):
    return struct.pack('<I', value)


def lua_string(value):
    encoded = value.encode('cp949') + b'\0'
    return u32(len(encoded)) + encoded


def abc(op, a=0, b=0, c=0):
    return op | (a << 6) | (c << 14) | (b << 23)


def abx(op, a=0, bx=0):
    return op | (a << 6) | (bx << 14)


def prototype(code, constants=(), children=(), upvalues=0):
    data = u32(0) + bytes(8) + bytes([upvalues, 0, 2, 4])
    data += u32(len(code)) + b''.join(u32(x) for x in code)
    data += u32(len(constants))
    for value in constants:
        data += (b'\4' + lua_string(value) if isinstance(value, str)
                 else b'\3' + struct.pack('<d', value))
    return data + u32(len(children)) + b''.join(children) + bytes(12)


HEADER = b'\x1bLua\x51\0\1\4\4\4\x08\0'
RETURN = abc(30, b=1)
CONSTANTS = ['ItemDBNameTbl', '마력1', 4815, 'Lookup']
TABLE = [abc(10), abc(9, b=257, c=258), abx(7)]


class FakePath:
    def __init__(self, content):
        self.content = content

    def read_bytes(self):
        return self.content

    def read_text(self, encoding):
        return self.content.decode(encoding)


class LuaLiteralTests(unittest.TestCase):
    def test_literal_names_and_ids(self):
        self.assertEqual(literal_tables(HEADER + prototype(TABLE + [RETURN], CONSTANTS)),
                         {'ItemDBNameTbl': {'마력1': 4815}})

    def test_uncalled_function_body_is_not_evaluated(self):
        child = prototype([abc(28), RETURN])  # CALL is deliberately unsupported
        code = TABLE + [abx(36), abx(7, bx=3), RETURN]
        self.assertEqual(literal_tables(HEADER + prototype(code, CONSTANTS, [child])),
                         {'ItemDBNameTbl': {'마력1': 4815}})

    def test_dynamic_top_level_is_rejected(self):
        for opcode in [5, 22, 28, 29]:  # GETGLOBAL, JMP, CALL, TAILCALL
            with self.subTest(opcode=opcode), self.assertRaises(ValueError):
                literal_tables(HEADER + prototype(TABLE + [abc(opcode), RETURN], CONSTANTS))

    def test_function_cannot_replace_literal_table(self):
        with self.assertRaises(ValueError):
            literal_tables(HEADER + prototype(TABLE + [abx(36), abx(7), RETURN],
                                             CONSTANTS, [prototype([RETURN])]))

    def test_upvalue_bindings_are_rejected(self):
        with self.assertRaises(ValueError):
            literal_tables(HEADER + prototype(TABLE + [abx(36), abx(7, bx=3), RETURN],
                                             CONSTANTS, [prototype([RETURN], upvalues=1)]))

    def test_post_declaration_mutation_is_rejected(self):
        with self.assertRaises(ValueError):
            literal_tables(HEADER + prototype(TABLE + [abx(36), abx(7, bx=3), abc(10), RETURN],
                                             CONSTANTS, [prototype([RETURN])]))

    def test_bad_header_truncation_and_trailing_bytes(self):
        data = HEADER + prototype(TABLE + [RETURN], CONSTANTS)
        for invalid in [b'not lua', data[:-1], data + b'\0', data[:8] + b'\x08' + data[9:]]:
            with self.subTest(length=len(invalid)), self.assertRaises(ValueError):
                literal_tables(invalid)

    def test_missing_return_is_rejected(self):
        with self.assertRaises(ValueError):
            literal_tables(HEADER + prototype(TABLE, CONSTANTS))


class RecipeMergeTests(unittest.TestCase):
    def setUp(self):
        self.base = {'Enchant': 'A', 'Upgrade': 'B', 'Price': 100,
                     'Materials': [{'Material': 'Ore', 'Amount': 5}, {'Material': 'Rune', 'Amount': 10}]}

    def test_material_overlay_retains_unmentioned_materials(self):
        result = merge_recipe(self.base, {'Materials': [{'Material': 'Ore', 'Amount': 2}]})
        self.assertEqual(result['Materials'], [{'Material': 'Ore', 'Amount': 2}, {'Material': 'Rune', 'Amount': 10}])
        self.assertEqual(result['Price'], 100)

    def test_zero_removes_material(self):
        self.assertEqual(merge_recipe(self.base, {'Materials': [{'Material': 'Ore', 'Amount': 0}]})['Materials'],
                         [{'Material': 'Rune', 'Amount': 10}])

    def test_omitted_amount_retains_existing_or_defaults_to_one(self):
        result = merge_recipe(self.base, {'Materials': [{'Material': 'Ore'}, {'Material': 'New'}]})
        self.assertEqual(result['Materials'][-1], {'Material': 'New', 'Amount': 1})
        self.assertEqual(result['Materials'][0]['Amount'], 5)

    def test_empty_list_does_not_clear_materials(self):
        self.assertEqual(merge_recipe(self.base, {'Materials': []}), self.base)

    def test_random_and_deterministic_replace_one_another(self):
        random = {'Enchant': 'A', 'RandomUpgrades': [{'Upgrade': 'C', 'Chance': 100000}], 'Price': 0}
        merged = merge_recipe(self.base, random)
        self.assertNotIn('Upgrade', merged)
        self.assertEqual(merged['Price'], 0)
        self.assertEqual(merged['Materials'], self.base['Materials'])
        self.assertNotIn('RandomUpgrades', merge_recipe(merged, {'Upgrade': 'D'}))

    def test_conflicting_or_missing_outcomes_are_rejected(self):
        for recipe in [{'Enchant': 'A'}, {'Upgrade': 'B', 'RandomUpgrades': []}]:
            with self.subTest(recipe=recipe), self.assertRaises(ValueError):
                merge_recipe({}, recipe)

    def test_random_probabilities_and_duplicate_targets(self):
        for outcomes in [[], [{'Upgrade': 'B', 'Chance': 0}], [{'Upgrade': 'B', 'Chance': 90000}],
                         [{'Upgrade': 'B', 'Chance': 50000}, {'Upgrade': 'B', 'Chance': 50000}]]:
            with self.subTest(outcomes=outcomes), self.assertRaises(ValueError):
                merge_recipe({}, {'RandomUpgrades': outcomes})

    def test_price_and_material_caps(self):
        result = merge_recipe(self.base, {'Price': 3000000000, 'Materials': [{'Material': 'Ore', 'Amount': 40000}]})
        self.assertEqual(result['Price'], 2147483647)
        self.assertEqual(result['Materials'][0]['Amount'], 30000)
        with self.assertRaises(ValueError):
            merge_recipe({}, {'Upgrade': 'B', 'Price': 3000000000}, perfect=True)
        with self.assertRaises(ValueError):
            merge_recipe(self.base, {'Materials': [{'Material': 'Ore', 'Amount': 65536}]})

    def test_perfect_mode_rejects_random(self):
        with self.assertRaises(ValueError):
            merge_recipe({}, {'RandomUpgrades': [{'Upgrade': 'B', 'Chance': 100000}]}, perfect=True)

    def test_inputs_are_not_mutated(self):
        before = copy.deepcopy(self.base)
        merge_recipe(self.base, {'Materials': [{'Material': 'Ore', 'Amount': 0}]})
        self.assertEqual(self.base, before)

    def test_material_order_does_not_affect_canonical_recipe(self):
        other = copy.deepcopy(self.base)
        other['Materials'].reverse()
        self.assertEqual(canonical(self.base), canonical(other))


class ClientNameTests(unittest.TestCase):
    def test_alias_uses_numeric_identity_and_flags_unknown_names(self):
        data = HEADER + prototype(TABLE + [RETURN], CONSTANTS)
        with patch('audit_enchant_upgrades.renewal_records', return_value=[{'Id': 4815, 'AegisName': 'Spell1'}]):
            resolve = ClientItemNames(FakePath(data), None)
        self.assertEqual(resolve('마력1'), 'Spell1')
        self.assertEqual(resolve.aliases['마력1']['Id'], 4815)
        self.assertEqual(resolve('Missing'), 'Missing')
        self.assertEqual(resolve.unresolved, {'Missing'})
        self.assertEqual(resolve.unresolved_details['Missing'],
                         {'reason': 'client_name_missing', 'client_id': None})

    def test_known_client_id_missing_on_server_is_not_a_missing_client_name(self):
        data = HEADER + prototype(TABLE + [RETURN], CONSTANTS)
        with patch('audit_enchant_upgrades.renewal_records', return_value=[]):
            resolve = ClientItemNames(FakePath(data), None)
        name = CONSTANTS[1]
        self.assertEqual(resolve(name), name)
        self.assertEqual(resolve.unresolved_details[name],
                         {'reason': 'server_item_missing', 'client_id': 4815})
        self.assertFalse(resolve.aliases)

    def test_client_recipe_resolves_sources_targets_and_materials(self):
        source = '  Table[1].Slot[2]:AddUpgradeEnchant("마력1","마력2",5,{"광석",2})  \t'
        aliases = {'마력1': 'Spell1', '마력2': 'Spell2', '광석': 'Ore'}
        ordinary, perfect = client_recipes(FakePath(source.encode('cp949')), aliases.__getitem__)
        self.assertEqual(ordinary[(1, 2, 'Spell1')], {'Enchant': 'Spell1', 'Upgrade': 'Spell2',
                                                   'Price': 5, 'Materials': [{'Material': 'Ore', 'Amount': 2}]})
        self.assertFalse(perfect)


class MigrationTests(unittest.TestCase):
    def test_custom_recipes_and_section_comments_survive_migration(self):
        source = '''Body:
  - Id: 132
    Slots:
      - Slot: 1
        Upgrades:
          - Enchant: A
            Upgrade: B
            Price: 100
          - Enchant: Custom
            Upgrade: Custom2
            Price: 200
# END CROWN

# NEXT GROUP
  - Id: 163
    TargetItems:
      Shoe: true
'''
        class Root:
            def __truediv__(self, relative):
                return FakePath(source.encode('utf-8'))
        ordinary = {(132, 1, 'A'): {'Enchant': 'A', 'RandomUpgrades': [{'Upgrade': 'B', 'Chance': 100000}], 'Price': 5}}
        result = workshop_contents(Root(), ordinary, {132})['db/import/item_enchant.yml']
        self.assertIn('Upgrade: Custom2\n            Price: 200', result)
        self.assertLess(result.index('RandomUpgrades:'), result.index('# END CROWN'))
        self.assertLess(result.index('# NEXT GROUP'), result.index('  - Id: 163'))
        self.assertTrue(result.endswith('    TargetItems:\n      Shoe: true\n'))

    def test_migration_is_idempotent(self):
        source = '''Body:
  - Id: 132
    Slots:
      - Slot: 1
        Upgrades:
          - Enchant: A
            Upgrade: B
'''
        class Root:
            def __truediv__(self, relative):
                return FakePath(source.encode('utf-8'))
        self.assertEqual(workshop_contents(Root(), {(132, 1, 'A'): {'Enchant': 'A', 'Upgrade': 'B'}}, {132}), {})


if __name__ == '__main__':
    unittest.main()
