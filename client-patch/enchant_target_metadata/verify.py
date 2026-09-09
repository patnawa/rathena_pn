#!/usr/bin/env python3
"""Verify only the twelve reviewed target metadata additions; never install.

Uses effective Renewal records, pinned user-supplied reference data, read-only
GRF indexing, and actual Win32 Lua 5.1 execution of the active itemInfo merger.
"""
import argparse
import configparser
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
PACKAGE = Path(__file__).resolve().parent
CLIENT = ROOT.parent.parent
sys.path.insert(0, str(ROOT / 'tools/ci'))
from effects import effective, fragment, provenance


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def windows_path(path):
    value = str(path.resolve())
    if os.name != 'nt':
        value = subprocess.check_output(['wslpath', '-w', value], text=True).strip()
    return value.replace('\\', '/')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-system', type=Path, required=True,
                        help='Directory containing the pinned reference resource table')
    parser.add_argument('--reference-items', type=Path, required=True,
                        help='Pinned reference item-properties Lua file (any filename)')
    parser.add_argument('--lua', type=Path,
                        default=ROOT.parent / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe')
    parser.add_argument('--grf-reader', type=Path, default=ROOT / 'tools/grf_v3_extract/grf_v3_extract.exe')
    parser.add_argument('--before-loader', type=Path,
                        help='Historical itemInfo loader; candidate default is the current active loader')
    parser.add_argument('--loader', type=Path,
                        help='Loader to verify; default active. Explicit archived checkpoints are labeled non-active')
    parser.add_argument('--installed', action='store_true',
                        help='Compare actual active loader with an explicit backed-up pre-install loader')
    args = parser.parse_args()
    active_loader = CLIENT / 'SystemEN/itemInfo.lua'
    checked_loader = (args.loader or active_loader).resolve()
    require(checked_loader.is_file(), 'Selected checked loader does not exist')
    require(not args.installed or args.before_loader is not None,
            '--installed requires an explicit --before-loader backup')
    before_loader = (args.before_loader or checked_loader).resolve()
    require(before_loader.is_file(), 'Selected before-loader does not exist')
    if args.installed:
        require(before_loader != checked_loader, 'Installed baseline must be a separate backup')
        current_source = checked_loader.read_text(encoding='utf-8')
        # Only the two new import-array lines may differ. This preserves old
        # source, all import order, options and the final override merge.
        for value in ('itemInfo_EnchantTargets.lua', 'enchanttargets'):
            current_source, count = re.subn(r'^\s*["\']' + re.escape(value) +
                r'["\'],[^\n]*\n', '', current_source, flags=re.M)
            require(count == 1, 'Installed loader must contain exactly one new file/table pair')
        require(current_source == before_loader.read_text(encoding='utf-8'),
                'Installed loader changed beyond the two allowed import lines')
    manifest = json.loads((PACKAGE / 'manifest.json').read_text(encoding='utf-8'))
    require(manifest['schema'] == 2, 'Manifest schema changed')
    expected_ids = {401055, 401056, 401057, 401058, 401059, 401060,
                    401115, 401116, 401117, 401118, 401119, 401120}
    items = {row['id']: row for row in manifest['items']}
    require(set(items) == expected_ids and len(manifest['items']) == 12, 'Reviewed item scope changed')
    sources = {
        'reference_item_info': args.reference_items,
        'reference_resource_table': args.reference_system / 'itemInfo_EN_db.lua',
        'original_enchant_list': ROOT.parent / 'audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub',
        'original_item_names': ROOT.parent / 'audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub',
        'active_enchant_helper': ROOT.parent / 'audit-client-enchants-20260906/new/data/luafiles514/lua files/Enchant/EnchantList_f.lub',
    }
    for name, path in sources.items():
        require(digest(path) == manifest['source_hashes'][name], 'Source drift: ' + str(path))
    server, combos, skills = effective(ROOT, expected_ids)
    require(expected_ids <= set(server), 'Required server identity missing; no placeholders permitted')
    require(len(combos) == 15, 'Reviewed combo scope changed')
    require(provenance(expected_ids, server, combos, skills) == manifest['effect_provenance'],
            'Effective item/combo Script, partner or skill provenance changed')
    generated = fragment(server, items, combos, skills)
    require((PACKAGE / 'SystemEN/itemInfo_EnchantTargets.lua').read_text(encoding='ascii') == generated,
            'Fragment descriptions/scalars do not exactly match effective server-derived rendering')
    fields = {'aegis': 'AegisName', 'name': 'Name', 'slots': 'Slots', 'view': 'View',
              'defense': 'Defense', 'armor_level': 'ArmorLevel',
              'minimum_level': 'EquipLevelMin', 'weight': 'Weight',
              'jobs': 'Jobs', 'classes': 'Classes', 'refineable': 'Refineable', 'gradable': 'Gradable'}
    # Only selected ASCII literal fields are read here; no lossy decoding of
    # Korean descriptions or copying of full third-party effect text occurs.
    resources = sources['reference_resource_table'].read_bytes().decode('latin1')
    for item_id, row in items.items():
        actual = server[item_id]
        require(actual.get('Type') == 'Armor' and actual.get('Locations') == {'Head_Top': True},
                'Server equipment type/location drift')
        for field, server_field in fields.items():
            require(actual.get(server_field) == row[field], f'Server {item_id} {server_field} drift')
        block = re.search(r'^\s*\[' + str(item_id) + r'\] = \{(.*?)^\s*\},', resources, re.M | re.S)
        require(block is not None, 'Missing reference resource record')
        resource = re.search(r'identifiedResourceName\s*=\s*"([^"]+)"', block[1])
        require(resource and resource[1] == row['resource'], 'Reference resource mapping drift')

    # Index active archives in precedence order; do not extract or alter files.
    ini = configparser.ConfigParser()
    ini.read(CLIENT / 'DATA.INI', encoding='utf-8-sig')
    wanted = {row['resource'].lower() for row in items.values()} | {manifest['unidentified_resource'].lower()}
    pattern = r'(?i)(?:^|[\\/])(?:' + '|'.join(sorted(wanted)) + r')\.(?:bmp|spr|act)$'
    assets = {}
    for _, archive in sorted(ini['Data'].items(), key=lambda pair: int(pair[0])):
        result = subprocess.run([str(args.grf_reader.resolve()), windows_path(CLIENT / archive), pattern],
                                capture_output=True, check=True)
        for line in result.stdout.decode('utf-8', errors='replace').splitlines():
            path = line.split('\t', 1)[0].lower()
            if '\t' not in line:
                continue
            basename = path.rsplit('\\', 1)[-1]
            stem, extension = basename.rsplit('.', 1)
            if extension in ('spr', 'act'):
                role = extension
            elif '\\item\\' in path:
                role = 'item'
            elif '\\collection\\' in path:
                role = 'collection'
            else:
                continue
            assets.setdefault((stem, role), archive)
    required_assets = {(name, role) for name in wanted for role in ('item', 'collection', 'spr', 'act')}
    require(set(assets) == required_assets,
            'Missing/unexpected reference assets: ' + repr(sorted(required_assets ^ set(assets))))

    targets = sorted(set(re.findall(r':AddTargetItem(?:_Duplicate)?\("([^"]+)"',
                                    sources['original_enchant_list'].read_text(encoding='cp949'))))
    values = '\n'.join(name + '=' + json.dumps(windows_path(path)) for name, path in {
        'REFERENCE_PATH': sources['reference_item_info'], 'NAMES_PATH': sources['original_item_names'],
        'HELPER_PATH': sources['active_enchant_helper'],
        'FRAGMENT_PATH': PACKAGE / 'SystemEN/itemInfo_EnchantTargets.lua',
        'BEFORE_LOADER': before_loader, 'CHECKED_LOADER': checked_loader,
    }.items())
    values += '\nINSTALLED=' + str(args.installed).lower()
    values += '\nEXPECTED={' + ','.join('[' + str(i) + ']={name=' + json.dumps(r['name']) +
        ',aegis=' + json.dumps(r['aegis']) + ',reference_name=' + json.dumps(r['reference_name']) +
        ',slots=1,view=' + str(r['view']) + ',resource=' + json.dumps(r['resource']) + '}'
        for i, r in sorted(items.items())) + '}\n'
    values += 'TARGETS={' + ','.join(json.dumps(name, ensure_ascii=False) for name in targets) + '}\n'
    fixture = r'''
assert(_VERSION == "Lua 5.1")
os.execute, io.popen = nil, nil
MessageBox = function(message) error(message) end
local environment = {}
local load_reference = assert(loadfile(REFERENCE_PATH))
setfenv(load_reference, environment)
load_reference()
for id, expected in pairs(EXPECTED) do
  local reference = assert(environment.itemInfo[id])
  assert(reference.slots == expected.slots and reference.view == expected.view)
  assert(reference.name == expected.reference_name)
end
environment, load_reference = nil, nil
collectgarbage("collect")
local function clone(value, seen)
  if type(value) ~= "table" then return value end
  seen = seen or {}
  if seen[value] then return seen[value] end
  local result = {}
  seen[value] = result
  for k, v in pairs(value) do result[clone(k, seen)] = clone(v, seen) end
  return setmetatable(result, getmetatable(value))
end
local function equal(a, b, seen)
  if type(a) ~= type(b) then return false end
  if type(a) ~= "table" then return a == b end
  if getmetatable(a) ~= getmetatable(b) then return false end
  seen = seen or {}
  if seen[a] then return seen[a] == b end
  seen[a] = b
  for k, v in pairs(a) do if not equal(v, b[k], seen) then return false end end
  for k in pairs(b) do if a[k] == nil then return false end end
  return true
end
-- Negative check proves this is a deep-value test, not entry identity alone.
local probe = {name="old", lines={"first", "second"}, slots=1}
local snapshot = clone(probe)
assert(equal(probe, snapshot))
probe.lines[2] = "mutated"
assert(not equal(probe, snapshot))
dofile(BEFORE_LOADER) -- Explicitly selected real pre-install loader.
dofile(NAMES_PATH) -- Actual original compiled name-to-ID lookup.
local before = clone(tbl)
local before_files, before_tables = clone(ImportFiles), clone(ImportTables)
assert(#before_files == #before_tables)
for _, file in ipairs(before_files) do assert(file ~= "itemInfo_EnchantTargets.lua") end
for _, name in ipairs(before_tables) do assert(name ~= "enchanttargets") end
local function missing()
  local result = {}
  for _, name in ipairs(TARGETS) do
    local id = ItemDB_To_ItemID(name)
    assert(id > 0)
    if not tbl[id] or type(tbl[id].slotCount) ~= "number" then result[id] = name end
  end
  return result
end
local missing_before = missing()
for id, expected in pairs(EXPECTED) do
  assert(ItemDB_To_ItemID(expected.aegis) == id)
  assert(tbl[id] == nil, "Existing metadata must not be overwritten: " .. id)
  assert(missing_before[id] == expected.aegis)
end
local new_environment = {}
local load_fragment = assert(loadfile(FRAGMENT_PATH))
setfenv(load_fragment, new_environment)
load_fragment()
local expected_fragment = assert(new_environment.tbl_enchanttargets)
local count = 0
for id, entry in pairs(expected_fragment) do
  count = count + 1
  local expected = assert(EXPECTED[id], "Unexpected new identity")
  assert(entry.slotCount == expected.slots and entry.ClassNum == expected.view)
  assert(entry.identifiedDisplayName == expected.name)
  assert(entry.identifiedResourceName == expected.resource)
  assert(entry.unidentifiedResourceName == "EpisodClear20")
  assert(entry.unidentifiedDisplayName == "Unidentified Headgear")
  assert(entry.costume == false)
end
assert(count == 12)
if INSTALLED then
  dofile(CHECKED_LOADER) -- Explicitly selected real loader, not a simulated import.
  assert(#ImportFiles == #before_files + 1 and #ImportTables == #before_tables + 1)
  local old_index, new_count = 1, 0
  for index, file in ipairs(ImportFiles) do
    if file == "itemInfo_EnchantTargets.lua" then
      assert(ImportTables[index] == "enchanttargets")
      new_count = new_count + 1
    else
      assert(file == before_files[old_index] and ImportTables[index] == before_tables[old_index])
      old_index = old_index + 1
    end
  end
  assert(new_count == 1 and old_index == #before_files + 1)
else
  F_itemInfoMerge(expected_fragment) -- Actual additive non-overwriting merger.
  assert(equal(ImportFiles, before_files) and equal(ImportTables, before_tables))
end
for id, entry in pairs(before) do assert(equal(tbl[id], entry), "Existing metadata field changed: " .. id) end
local added = 0
for id, entry in pairs(tbl) do
  if before[id] == nil then
    added = added + 1
    assert(expected_fragment[id] and equal(entry, expected_fragment[id]), "Unexpected/different new record: " .. id)
  end
end
assert(added == 12)
for id, entry in pairs(expected_fragment) do assert(equal(tbl[id], entry), "Expected complete new record missing: " .. id) end
local missing_after, before_count, after_count = missing(), 0, 0
for id in pairs(missing_before) do
  before_count = before_count + 1
  assert(missing_after[id] ~= nil or EXPECTED[id] ~= nil)
end
for id, name in pairs(missing_after) do
  after_count = after_count + 1
  assert(missing_before[id] == name and EXPECTED[id] == nil)
end
assert(before_count == 43 and after_count == 31)
MAX_SLOT_NUM, MAX_REFINE_LEVEL, MAX_GRADE_LEVEL, MAX_MATERIAL_NUM = 4, 20, 7, 8
C_GetSlotCount = function(name)
  local id = ItemDB_To_ItemID(name)
  assert(EXPECTED[id], "Only reviewed targets belong in this scoped check")
  assert(tbl[id] and type(tbl[id].slotCount) == "number")
  return tbl[id].slotCount -- No zero or inferred fallback.
end
dofile(HELPER_PATH)
Table[165] = CreateEnchantInfo()
Table[165]:SetSlotOrder(3) -- Exact original group-165 order.
for _, expected in pairs(EXPECTED) do Table[165]:AddTargetItem(expected.aegis) end
assert(not LoadFailed and #Table[165].TargetItemTbl == 12)
print("NATIVE_METADATA_OK before=43 after=31 added=12 slots=1 old_fields=deep_preserved scoped_helper_targets=12")
'''
    tracked = [CLIENT / 'DATA.INI', active_loader, checked_loader, before_loader, *sources.values()]
    hashes_before = {str(path): digest(path) for path in tracked}
    env = {k: v for k, v in os.environ.items() if k not in {'LUA_INIT', 'LUA_PATH', 'LUA_CPATH'}}
    result = subprocess.run([str(args.lua.resolve()), '-'], cwd=CLIENT,
                            input=(values + fixture).encode('cp949'), capture_output=True, env=env, timeout=60)
    require(result.returncode == 0, result.stderr.decode('cp949', errors='replace'))
    require(b'NATIVE_METADATA_OK before=43 after=31 added=12 slots=1 old_fields=deep_preserved scoped_helper_targets=12' in
            result.stdout, 'Native metadata verification did not complete')
    require(hashes_before == {str(path): digest(path) for path in tracked}, 'Read-only input changed during verification')
    executed_loader = checked_loader if args.installed else before_loader
    print(json.dumps({'result': 'PASS', 'installed': args.installed, 'targets': 12, 'slot_count': 1,
                      'before_loader': windows_path(before_loader), 'active_loader': windows_path(active_loader),
                      'checked_loader': windows_path(executed_loader),
                      'loader_is_active': executed_loader == active_loader.resolve(),
                      'before_missing': 43, 'after_missing': 31, 'original_metadata_preserved': True,
                      'deep_snapshot_all_old_fields': True, 'all_descriptions_match_effective_server': True,
                      'item_scripts_checked': 12, 'combo_scripts_checked': len(combos),
                      'actual_lua_runtime': windows_path(args.lua), 'scoped_helper_targets_checked': 12,
                      'verified_asset_entries': len(assets), 'asset_archives': sorted(set(assets.values())),
                      'fragment_sha256': digest(PACKAGE / 'SystemEN/itemInfo_EnchantTargets.lua'),
                      'source_hashes': manifest['source_hashes']}, indent=2))


if __name__ == '__main__':
    main()
