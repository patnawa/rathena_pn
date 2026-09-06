#!/usr/bin/env python3
"""Offline preservation, round-trip, recipe, and refusal tests for Chapter 2.

Pass --luac to additionally validate both generated resources with an independent
compiled Lua 5.1 parser (the supplied ROenglishRE Tools/luac.exe works under WSL).
No active client data is written. Test artifacts stay inside server-work.
"""
import argparse
import copy
import io
import json
import os
import re
import struct
import subprocess
import tempfile
import unittest
import zlib
from pathlib import Path
from unittest.mock import patch

import build_chapter2_native_patch as builder
from audit_enchant_upgrades import ClientItemNames, client_recipes, renewal_records
from audit_initial_enchants import client_configuration, compare
from lua51_literal_table import literal_tables


ROOT = builder.ROOT
SOURCES = {
    'enchant_list': ROOT.parent / 'audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub',
    'item_names': ROOT.parent / 'audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub',
    'helper': ROOT.parent / 'audit-client-enchants-20260906/new/data/luafiles514/lua files/Enchant/EnchantList_f.lub',
}
LUAC = None
LUA = None


def native_argument(executable, path):
    argument = str(path.resolve())
    if os.name != 'nt' and str(executable).lower().endswith('.exe'):
        return subprocess.check_output(['wslpath', '-w', argument], text=True).strip()
    return argument


def independent_grf_reader(data):
    """Read GRF v2 independently of the builder, checking sizes and bounds."""
    source = io.BytesIO(data)
    if source.read(16) != b'Master of Magic\0':
        raise ValueError('GRF signature')
    source.read(14)
    table_offset, seed, raw_count, version = struct.unpack('<IIII', source.read(16))
    if version != 0x200 or raw_count < seed + 7:
        raise ValueError('GRF version/count')
    source.seek(46 + table_offset)
    compressed_size, expanded_size = struct.unpack('<II', source.read(8))
    compressed = source.read(compressed_size)
    if len(compressed) != compressed_size or source.read(1):
        raise ValueError('GRF table bounds')
    table_bytes = zlib.decompress(compressed)
    if len(table_bytes) != expanded_size:
        raise ValueError('GRF table size')
    table, result, occupied = io.BytesIO(table_bytes), {}, []
    for _ in range(raw_count - seed - 7):
        chars = bytearray()
        while True:
            char = table.read(1)
            if not char:
                raise ValueError('GRF name terminator')
            if char == b'\0':
                break
            chars.extend(char)
        name = chars.decode('ascii')
        size, aligned, expanded, kind, offset = struct.unpack('<IIIBI', table.read(17))
        if kind != 1 or aligned != size or offset + aligned > table_offset or name in result:
            raise ValueError('GRF entry schema/bounds')
        if any(offset < end and offset + aligned > start for start, end in occupied):
            raise ValueError('Overlapping GRF entries')
        occupied.append((offset, offset + aligned))
        source.seek(46 + offset)
        result[name] = zlib.decompress(source.read(size))
        if len(result[name]) != expanded:
            raise ValueError('GRF payload size')
    if table.read(1):
        raise ValueError('Extra GRF table entries')
    return result


def execute_lookup(prototype, table, name):
    """Bounded interpreter for the existing lookup's actual Lua instructions.

    This is an additional behavioral check, not a client/Lua-runtime claim.
    Only the opcodes present in this one immutable function are supported.
    """
    registers, messages = {0: name}, []
    globals_ = {'ItemDBNameTbl': table, 'MessageBox': messages.append}
    constants, code, pc = prototype['constants'], prototype['code'], 0

    def rk(operand):
        return constants[operand & 255] if operand & 256 else registers.get(operand)

    for _ in range(100):
        instruction = code[pc]
        pc += 1
        op, a, b, c = instruction & 63, (instruction >> 6) & 255, (instruction >> 23) & 511, (instruction >> 14) & 511
        if op == 0:  # MOVE
            registers[a] = registers[b]
        elif op == 1:  # LOADK
            registers[a] = constants[instruction >> 14]
        elif op == 5:  # GETGLOBAL
            registers[a] = globals_[constants[instruction >> 14]]
        elif op == 6:  # GETTABLE
            registers[a] = registers[b].get(rk(c))
        elif op == 21:  # CONCAT
            registers[a] = ''.join(str(registers[i]) for i in range(b, c + 1))
        elif op == 22:  # JMP
            pc += (instruction >> 14) - 131071
        elif op == 23:  # EQ; skip following instruction when comparison != A.
            if (rk(b) == rk(c)) != bool(a):
                pc += 1
        elif op == 28 and b == 2 and c == 1:  # CALL MessageBox(argument), no results
            if registers[a] != messages.append:
                raise AssertionError('Unexpected callable')
            registers[a](registers[a + 1])
        elif op == 30 and b == 2:  # RETURN one result
            return registers[a], messages
        else:
            raise AssertionError(f'Unexpected lookup opcode {op}')
    raise AssertionError('Lookup instruction bound exceeded')


class Chapter2NativePatchTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        missing = [str(p) for p in SOURCES.values() if not p.is_file()]
        if missing:
            raise unittest.SkipTest('Explicit client inputs unavailable: ' + ', '.join(missing))
        cls.manifest = json.loads(builder.MANIFEST.read_text(encoding='utf-8'))
        cls.original = {k: p.read_bytes() for k, p in SOURCES.items()}
        cls.entries, cls.archive, cls.report = builder.build_payloads(ROOT, **cls.original, manifest=cls.manifest)
        cls.groups = builder.chapter_groups(ROOT, cls.manifest)
        cls.original_table = literal_tables(cls.original['item_names'])['ItemDBNameTbl']
        cls.new_table = literal_tables(cls.entries[builder.NAMES_ENTRY])['ItemDBNameTbl']
        cls.old_chunk = builder.parse_chunk(cls.original['item_names'])
        cls.new_chunk = builder.parse_chunk(cls.entries[builder.NAMES_ENTRY])
        cls.artifacts = tempfile.TemporaryDirectory(prefix='chapter2-native-test-', dir=ROOT.parent)
        cls.addClassCleanup(cls.artifacts.cleanup)
        cls.directory = Path(cls.artifacts.name)
        cls.list_path, cls.names_path = cls.directory / 'EnchantList.lub', cls.directory / 'ItemDBNameTbl.lub'
        cls.list_path.write_bytes(cls.entries[builder.LIST_ENTRY])
        cls.names_path.write_bytes(cls.entries[builder.NAMES_ENTRY])

    def test_exact_two_entry_independent_grf_round_trip(self):
        self.assertEqual(independent_grf_reader(self.archive), self.entries)
        self.assertEqual(len(self.entries), 2)

    def test_deterministic_grf_and_repeated_build(self):
        entries, archive, report = builder.build_payloads(ROOT, **self.original, manifest=self.manifest)
        self.assertEqual((entries, archive, report), (self.entries, self.archive, self.report))
        self.assertEqual(builder.make_grf(dict(reversed(list(entries.items())))), archive)

    def test_all_original_and_added_item_mappings(self):
        self.assertEqual(len(self.new_table), 5292)
        self.assertEqual({k: self.new_table[k] for k in self.original_table}, self.original_table)
        self.assertEqual({k: v for k, v in self.new_table.items() if k not in self.original_table},
                         self.manifest['aliases'])

    def test_original_function_instructions_constants_and_debug_preserved(self):
        old, new = self.old_chunk, self.new_chunk
        at, added = len(old['code']) - 3, 84 * 3
        self.assertEqual(new['code'][:at] + new['code'][at + added:], old['code'])
        self.assertEqual(new['lines'][:at] + new['lines'][at + added:], old['lines'])
        self.assertEqual(new['lines'][at:at + added], [0] * added)
        self.assertEqual(new['constant_bytes'][:len(old['constant_bytes'])], old['constant_bytes'])
        self.assertEqual(new['child_bytes'], old['child_bytes'])
        self.assertEqual(new['children'][0]['raw'], old['children'][0]['raw'])
        self.assertEqual(new['prefix'], old['prefix'])
        self.assertEqual(new['debug_tail'], old['debug_tail'])

    def test_actual_lookup_bytecode_all_names_and_missing_behavior(self):
        function = self.new_chunk['children'][0]
        for name, expected in self.new_table.items():
            with self.subTest(name=name):
                self.assertEqual(execute_lookup(function, self.new_table, name), (expected, []))
        unknown = '__chapter2_unmapped_probe__'
        before = execute_lookup(self.old_chunk['children'][0], self.original_table, unknown)
        after = execute_lookup(function, self.new_table, unknown)
        self.assertEqual(after, before)
        self.assertEqual(after[0], 0)
        self.assertEqual(len(after[1]), 1)
        self.assertIn(unknown, after[1][0])

    def test_original_list_bytes_and_all_initial_models_unchanged(self):
        self.assertTrue(self.entries[builder.LIST_ENTRY].startswith(self.original['enchant_list']))
        old = client_configuration(SOURCES['enchant_list'], lambda name: name)
        new = client_configuration(self.list_path, lambda name: name)
        self.assertEqual(len(old), 159)
        self.assertEqual(len(new), 164)
        self.assertEqual({g: new[g] for g in old}, old)
        self.assertEqual(set(new) - set(old), set(range(167, 172)))

    def test_required_client_caution_metadata(self):
        self.assertEqual(self.manifest['client_only_metadata'], {'caution': builder.CAUTION})
        self.assertEqual(self.report['client_only_metadata'], self.manifest['client_only_metadata'])
        suffix = self.entries[builder.LIST_ENTRY][len(self.original['enchant_list']):].decode('ascii')
        for group in range(167, 172):
            self.assertEqual(suffix.count(f'Table[{group}]:SetCaution({json.dumps(builder.CAUTION)})'), 1)

    def test_separate_material_fragment_matches_server_identity_type_and_weight(self):
        path = ROOT / 'client-patch/chapter2_native/SystemEN/itemInfo_Chapter2Materials.lua'
        fragment = path.read_text(encoding='utf-8')
        parts = re.split(r'^\s*\[(\d+)\]\s*=\s*\{', fragment, flags=re.M)
        records = {int(parts[i]): parts[i + 1] for i in range(1, len(parts), 2)}
        self.assertEqual(set(records), {1002700, 1002751, 1002752, 1002753})
        server = {}
        for record in renewal_records(ROOT, 'db/item_db.yml'):
            server.setdefault(record['Id'], {}).update(record)
        for item_id, text in records.items():
            item = server[item_id]
            self.assertEqual(text.count('DisplayName = ' + json.dumps(item['Name'])), 2)
            self.assertEqual(text.count('ResourceName = "EpisodClear20"'), 2)
            self.assertIn('"Type: ' + item['Type'] + '"', text)
            self.assertIn('"Weight: ' + format(item.get('Weight', 0) / 10, 'g') + '"', text)
            self.assertIn('slotCount = 0, ClassNum = 0, costume = false', text)
        if LUAC is not None:
            result = subprocess.run([str(LUAC.resolve()), '-p', native_argument(LUAC, path)], capture_output=True)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode(errors='replace'))

    def test_exact_server_eligibility_prices_materials_and_all_58_recipes(self):
        resolve = ClientItemNames(self.names_path, ROOT)
        parsed = client_configuration(self.list_path, resolve)
        chapter = {g: parsed[g] for g in range(167, 172)}
        self.assertEqual(chapter, self.groups)  # Both directions: no omitted or extra keys.
        self.assertEqual(compare(chapter, self.groups), [])
        self.assertEqual(sum(len(g['Targets']) for g in chapter.values()), 22)
        self.assertEqual(sum(len(s['Perfect']) for g in chapter.values() for s in g['Slots'].values()), 58)
        self.assertFalse(set(resolve.unresolved) & builder.recipe_names(self.groups))

    def test_original_upgrade_and_probability_tables_unchanged(self):
        self.assertEqual(client_recipes(self.list_path), client_recipes(SOURCES['enchant_list']))
        old = client_configuration(SOURCES['enchant_list'], lambda name: name)
        new = client_configuration(self.list_path, lambda name: name)
        totals = []
        for group_id, group in old.items():
            for slot, config in group['Slots'].items():
                self.assertEqual(new[group_id]['Slots'][slot]['Enchants'], config['Enchants'])
                for outcomes in config['Enchants'].values():
                    totals.append(sum(outcomes.values()))
        self.assertTrue(totals)
        self.assertEqual(set(totals), {100000})

    def test_source_hash_drift_refused_for_each_resource(self):
        for key in self.original:
            with self.subTest(resource=key), self.assertRaisesRegex(ValueError, 'source hash mismatch'):
                inputs = dict(self.original)
                inputs[key] += b'\0'
                builder.build_payloads(ROOT, **inputs, manifest=self.manifest)

    def test_effective_server_drift_refused(self):
        manifest = copy.deepcopy(self.manifest)
        manifest['server_groups_sha256'] = '0' * 64
        with self.assertRaisesRegex(ValueError, 'configuration drift'):
            builder.chapter_groups(ROOT, manifest)

    def test_unknown_raw_server_feature_refused(self):
        records = [copy.deepcopy(r) for r in renewal_records(ROOT, 'db/item_enchant.yml') if 167 <= r['Id'] <= 171]
        records[0]['Slots'][0]['UpgradeEnchants'] = []
        with patch.object(builder, 'renewal_records', return_value=iter(records)):
            with self.assertRaisesRegex(ValueError, 'Unsupported Chapter 2 slot feature'):
                builder.chapter_groups(ROOT, self.manifest)

    def test_name_id_and_manifest_conflicts_refused(self):
        for aliases in [{'Ch1_Mana_Ring': 999}, {'not_an_existing_name': 1001996}, {'a': 999, 'b': 999}]:
            with self.subTest(aliases=aliases), self.assertRaises(ValueError):
                builder.append_name_aliases(self.original['item_names'], aliases)
        manifest = copy.deepcopy(self.manifest)
        manifest['aliases']['Ch2_Flame_Coin'] = 1002701
        with self.assertRaisesRegex(ValueError, 'Server identity drift'):
            builder.build_payloads(ROOT, **self.original, manifest=manifest)

    def test_duplicate_groups_and_bad_lua_schema_refused(self):
        with self.assertRaisesRegex(ValueError, 'already present'):
            builder.append_enchants(self.entries[builder.LIST_ENTRY], self.groups,
                                    {**self.manifest['existing_required_names'], **self.manifest['aliases']})
        with self.assertRaises(ValueError):
            builder.append_name_aliases(b'plaintext is not a compiled lookup', {'a': 999})
        for data in [self.original['item_names'][:-1], self.original['item_names'] + b'extra']:
            with self.subTest(bytes=len(data)), self.assertRaises(ValueError):
                builder.append_name_aliases(data, {'a': 999})

    def test_output_guard_and_no_extra_grf_entries(self):
        for directory in [ROOT.parent.parent / 'chapter2_native.grf', ROOT, ROOT.parent, self.directory]:
            with self.subTest(path=str(directory)), self.assertRaises(ValueError):
                builder.output_directory(directory, ROOT)
        with self.assertRaises(ValueError):
            builder.output_directory(ROOT.parent.parent / 'active-client-dir', ROOT.parent.parent / 'fake-root')
        with self.assertRaisesRegex(ValueError, 'exactly two'):
            builder.make_grf({**self.entries, 'DATA.INI': b'forbidden'})

    def test_independent_compiled_lua51_parser_when_supplied(self):
        if LUAC is None:
            self.skipTest('Pass --luac for independent compiled Lua 5.1 syntax/bytecode validation')
        executable = str(LUAC.resolve())
        version = subprocess.run([executable, '-v'], capture_output=True, check=True)
        self.assertIn(b'Lua 5.1', version.stdout + version.stderr)
        for path in [self.list_path, self.names_path, SOURCES['helper']]:
            argument = native_argument(executable, path)
            result = subprocess.run([executable, '-p', argument], capture_output=True)
            self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode(errors='replace'))

    def test_real_lua51_executes_original_lookup_old_new_and_unknown(self):
        if LUA is None:
            self.skipTest('Pass --lua for direct original 32-bit bytecode execution in Lua 5.1')
        executable = str(LUA.resolve())
        version = subprocess.run([executable, '-v'], capture_output=True, check=True)
        self.assertIn(b'Lua 5.1', version.stdout + version.stderr)
        aliases = '\n'.join(f'  [{json.dumps(k)}] = {v},' for k, v in sorted(self.manifest['aliases'].items()))
        script = '''
local messages = {}
MessageBox = function(text) messages[#messages + 1] = text end
dofile(arg[1])
local old, oldCount = {}, 0
for name, id in pairs(ItemDBNameTbl) do
  old[name] = id
  oldCount = oldCount + 1
  assert(ItemDB_To_ItemID(name) == id, name)
end
assert(oldCount == 5208)
local unknown = "__chapter2_native_unknown__"
assert(ItemDB_To_ItemID(unknown) == 0)
assert(#messages == 1)
local originalMessage = messages[1]
local expected = {
''' + aliases + '''
}
for name in pairs(expected) do assert(ItemDBNameTbl[name] == nil, name) end
dofile(arg[2]) -- Load the actual patched bytes; no conversion/recompilation.
for name, id in pairs(old) do
  assert(ItemDBNameTbl[name] == id, name)
  assert(ItemDB_To_ItemID(name) == id, name)
end
local added = 0
for name, id in pairs(expected) do
  added = added + 1
  assert(ItemDB_To_ItemID(name) == id, name)
end
local count = 0
for name in pairs(ItemDBNameTbl) do
  count = count + 1
  assert(old[name] ~= nil or expected[name] ~= nil, name)
end
assert(added == 84 and count == 5292 and #messages == 1)
assert(ItemDB_To_ItemID(unknown) == 0)
assert(#messages == 2 and messages[2] == originalMessage)
print("LUA51_NATIVE_LOOKUP_OK original=5208 added=84 total=5292 unknown=preserved")
'''
        path = self.directory / 'native_lookup_test.lua'
        path.write_text(script, encoding='ascii')
        env = {k: v for k, v in os.environ.items() if k not in {'LUA_INIT', 'LUA_PATH', 'LUA_CPATH'}}
        result = subprocess.run([executable, native_argument(executable, path),
                                 native_argument(executable, SOURCES['item_names']),
                                 native_argument(executable, self.names_path)], capture_output=True, env=env)
        self.assertEqual(result.returncode, 0, (result.stdout + result.stderr).decode(errors='replace'))
        self.assertIn(b'LUA51_NATIVE_LOOKUP_OK original=5208 added=84 total=5292 unknown=preserved', result.stdout)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--enchant-list', type=Path, default=SOURCES['enchant_list'])
    parser.add_argument('--item-names', type=Path, default=SOURCES['item_names'])
    parser.add_argument('--helper', type=Path, default=SOURCES['helper'])
    parser.add_argument('--luac', type=Path)
    parser.add_argument('--lua', type=Path)
    args, remaining = parser.parse_known_args()
    SOURCES = {k: getattr(args, k) for k in SOURCES}
    LUAC = args.luac
    LUA = args.lua
    unittest.main(argv=[__file__, *remaining])
