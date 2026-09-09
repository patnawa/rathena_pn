#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  chapter2_gear_client_install_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/chapter2_gear_client_install_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Read-only installed Chapter 2 GRF and six-record itemInfo merge verification.

Compares the exact pre-install loader with the installed loader using real Lua
5.1, preserving a recursive snapshot because the base file reinitializes tbl.
No active file, backup, archive, or extracted artifact is written by this test.
"""
import argparse
import configparser
import hashlib
import json
from pathlib import Path
import re
import subprocess

from audit_enchant_upgrades import renewal_records
from chapter2_client_helper_test import windows_path
from chapter2_native_patch_test import independent_grf_reader


ROOT = Path(__file__).resolve().parents[2]
CLIENT = ROOT.parent.parent
BACKUP = ROOT.parent / 'client-before-chapter2-gear-20260906'
CANDIDATE = ROOT.parent / 'chapter2-native-verified-20260906'
RUNTIME = ROOT.parent / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe'
GRF_HASH = '09a60d6b3da391c7b45160338ccc887357ad836e680b6829426353fbfd160541'
ENTRY_HASHES = {
    r'data\luafiles514\lua files\Enchant\EnchantList.lub':
        '4adcfaa537969e89622540acd302863dd653bb0f5d4a9211c6088f2a4f9fe203',
    r'data\luafiles514\lua files\ItemDBNameTbl.lub':
        '9c1bf7474d9869528b10c9383d3613d2491961b81adc9f0976065cddb043b301',
}
FRAGMENTS = {
    'itemInfo_Chapter2Materials.lua': (
        'chapter2materials', ROOT / 'client-patch/chapter2_native/SystemEN/itemInfo_Chapter2Materials.lua',
        'bcb1cdd1e74111466ad00f6c8f7a7c4b65406da904fbe592deee65c8aa922da7'),
    'itemInfo_DruidGear.lua': (
        'druidgear', ROOT / 'client-patch/druid_gear/SystemEN/itemInfo_DruidGear.lua',
        'be2c7814b76537f8285c37dddffb8bec99ba14ac772dc348b1920d490e5580ab'),
}
EXPECTED_IDS = {1002700, 1002751, 1002752, 1002753, 314269, 314270}


LUA_COMPARE = r'''
assert(_VERSION == "Lua 5.1", "Lua 5.1 required")
os.execute = nil
io.popen = nil
local function snapshot(value)
    if type(value) ~= "table" then return value end
    local result = {}
    for key, child in pairs(value) do result[key] = snapshot(child) end
    return result
end
local checked = 0
local function same(left, right)
    checked = checked + 1
    if type(left) ~= type(right) then return false end
    if type(left) ~= "table" then return left == right end
    for key, value in pairs(left) do if not same(value, right[key]) then return false end end
    for key in pairs(right) do if left[key] == nil then return false end end
    return true
end
local function count(value)
    local result = 0
    for _ in pairs(value) do result = result + 1 end
    return result
end
-- Loading itemInfo defines its native main functions, but this harness never
-- calls them. No AddItem registration callbacks or game window are invoked.
dofile(BACKUP_LOADER)
assert(type(tbl) == "table", "backup loader did not build itemInfo")
local before = snapshot(tbl)
local old_files, old_tables = snapshot(ImportFiles), snapshot(ImportTables)
dofile(ACTIVE_LOADER)
assert(type(tbl) == "table", "active loader did not build itemInfo")
local after = tbl
assert(#ImportFiles == #old_files + 2 and #ImportTables == #old_tables + 2,
       "active import arrays have unexpected size")
for index, name in ipairs(old_files) do assert(ImportFiles[index] == name, "old file import reordered") end
for index, name in ipairs(old_tables) do assert(ImportTables[index] == name, "old table import reordered") end
for id, record in pairs(before) do
    assert(after[id] ~= nil, "previous item disappeared: " .. tostring(id))
    assert(same(record, after[id]), "previous item metadata changed recursively: " .. tostring(id))
end
local additions = {}
for id, record in pairs(after) do if before[id] == nil then additions[id] = record end end
assert(count(additions) == 6, "merged loader did not add exactly six records")
-- Independently execute the pinned reviewed repository fragments, then compare
-- their actual Lua-produced records with the installed merged records.
dofile(MATERIALS_REVIEW)
dofile(GEAR_REVIEW)
local reviewed = {}
for id, record in pairs(tbl_chapter2materials) do reviewed[id] = record end
for id, record in pairs(tbl_druidgear) do assert(not reviewed[id], "reviewed duplicate ID"); reviewed[id] = record end
assert(count(reviewed) == 6, "reviewed fragment ID scope changed")
assert(same(reviewed, additions), "installed new records differ from reviewed metadata")
local ids = {}
for id, record in pairs(additions) do
    assert(EXPECTED[id], "unreviewed added item identity")
    assert(record.slotCount == 0 and record.ClassNum == 0 and record.costume == false,
           "new item slot/class/costume metadata drift")
    assert(record.identifiedResourceName == "EpisodClear20" and
           record.unidentifiedResourceName == "EpisodClear20", "generic resource mismatch")
    ids[#ids + 1] = id
end
table.sort(ids)
print('{"result":"PASS","previous_records":' .. count(before) ..
      ',"active_records":' .. count(after) .. ',"recursive_comparisons":' .. checked ..
      ',"added_ids":[' .. table.concat(ids, ',') .. '],"old_records_unchanged":true}')
'''


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def archive_order(path):
    parser = configparser.ConfigParser(strict=True)
    parser.read_string(path.read_text(encoding='utf-8-sig'))
    require(parser.sections() == ['Data'], 'Unexpected DATA.INI section layout')
    require(all(key.isdigit() for key in parser['Data']), 'Nonnumeric archive priority')
    keys = sorted(map(int, parser['Data']))
    require(keys == list(range(len(keys))), 'Archive priorities are not contiguous')
    return [parser['Data'][str(key)] for key in keys]


def import_arrays(path):
    text = path.read_text(encoding='utf-8-sig')
    result = {}
    for name in ('ImportFiles', 'ImportTables'):
        blocks = re.findall(r'\b' + name + r'\s*=\s*\{([^}]*)\}', text, re.S)
        require(len(blocks) == 1, 'Expected exactly one ' + name + ' array')
        contents = re.sub(r'--[^\n]*', '', blocks[0])
        result[name] = re.findall(r'"([^"\r\n]+)"', contents)
        require(len(result[name]) == len(set(result[name])), 'Duplicate entry in ' + name)
    require(len(result['ImportFiles']) == len(result['ImportTables']), 'File/table pair count mismatch')
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, default=RUNTIME)
    parser.add_argument('--backup', type=Path, default=BACKUP)
    parser.add_argument('--loader', type=Path, default=CLIENT / 'SystemEN/itemInfo.lua',
                        help='Loader to check; explicitly select a retained checkpoint after later client additions')
    args = parser.parse_args()
    backup_ini, backup_loader = args.backup / 'DATA.INI', args.backup / 'SystemEN/itemInfo.lua'
    active_ini, active_loader = CLIENT / 'DATA.INI', args.loader
    require(digest(backup_ini) == '10b3584271cfb8197c61365f643c851e7b332f444d4cd9399febc4a6b1d595dd', 'Backup DATA.INI drift')
    require(digest(backup_loader) == '3bcef815049208711949e10a0bf975c90292f3cfed9d58749c5317b26838c065', 'Backup itemInfo loader drift')
    old_order, new_order = archive_order(backup_ini), archive_order(active_ini)
    require(old_order == ['nebula_upgrade_v2.grf', 'server.grf', 'english.grf', 'new.grf', 'data.grf'], 'Original five-archive order drift')
    require(new_order == ['chapter2_native.grf'] + old_order, 'Installed archive priorities do not preserve originals')
    require(all((CLIENT / name).is_file() for name in new_order), 'A configured archive is missing')
    installed = CLIENT / new_order[0]
    require(digest(installed) == GRF_HASH, 'Installed GRF is not the reviewed corrected artifact')
    entries = independent_grf_reader(installed.read_bytes())
    require(set(entries) == set(ENTRY_HASHES), 'Installed GRF has unexpected resources')
    for name, expected in ENTRY_HASHES.items():
        require(hashlib.sha256(entries[name]).hexdigest() == expected, 'Installed GRF payload drift: ' + name)
        extracted = CANDIDATE / Path(name.replace('\\', '/'))
        require(entries[name] == extracted.read_bytes(), 'Installed payload differs from reviewed extracted artifact: ' + name)
        require(not (CLIENT / Path(name.replace('\\', '/'))).exists(), 'Unexpected loose resource could override the installed GRF: ' + name)
    old_imports, new_imports = import_arrays(backup_loader), import_arrays(active_loader)
    for field in ('ImportFiles', 'ImportTables'):
        require(new_imports[field][:len(old_imports[field])] == old_imports[field], 'Old loader imports changed or reordered')
        require(len(new_imports[field]) == len(old_imports[field]) + 2, 'Unexpected added loader import count')
    expected_pairs = [(file, data[0]) for file, data in FRAGMENTS.items()]
    require(list(zip(new_imports['ImportFiles'], new_imports['ImportTables']))[-2:] == expected_pairs, 'Installed new file/table pair order mismatch')
    active_text = active_loader.read_text(encoding='utf-8-sig')
    removed = set(FRAGMENTS) | {entry[0] for entry in FRAGMENTS.values()}
    restored_lines = [line for line in active_text.splitlines()
                      if not any(re.match(r'\s*"' + re.escape(name) + r'"\s*,', line) for name in removed)]
    require('\n'.join(restored_lines) == '\n'.join(backup_loader.read_text(encoding='utf-8-sig').splitlines()),
            'Loader changed outside the four appended import lines')
    code_lines = [line.strip() for line in active_text.splitlines() if line.strip() and not line.lstrip().startswith('--')]
    require(re.fullmatch(r'F_itemInfoMerge\(tbl_override,\s*true\)\s*(?:--.*)?', code_lines[-1]),
            'Final override merge is no longer last')
    for filename, (_, reviewed, expected) in FRAGMENTS.items():
        require(digest(reviewed) == expected, 'Reviewed fragment drift: ' + filename)
        require((CLIENT / 'SystemEN' / filename).read_bytes() == reviewed.read_bytes(), 'Installed fragment differs: ' + filename)
    items = {}
    for record in renewal_records(ROOT, 'db/item_db.yml'):
        if record['Id'] in EXPECTED_IDS:
            items.setdefault(record['Id'], {}).update(record)
    require(set(items) == EXPECTED_IDS, 'One of six corresponding server items is absent')
    for item_id, record in items.items():
        expected = ('Card', 'Enchant') if item_id in (314269, 314270) else ('Etc', None)
        require((record.get('Type'), record.get('SubType')) == expected, 'Server item type/subtype mismatch')
        require(record.get('Slots', 0) == 0, 'Server material/enchant physical slots mismatch')
    values = {'BACKUP_LOADER': backup_loader, 'ACTIVE_LOADER': active_loader,
              'MATERIALS_REVIEW': FRAGMENTS['itemInfo_Chapter2Materials.lua'][1],
              'GEAR_REVIEW': FRAGMENTS['itemInfo_DruidGear.lua'][1]}
    prelude = '\n'.join(name + ' = ' + json.dumps(windows_path(path)) for name, path in values.items())
    prelude += '\nEXPECTED = {' + ','.join(f'[{item_id}] = true' for item_id in sorted(EXPECTED_IDS)) + '}\n'
    result = subprocess.run([str(args.lua.resolve()), '-'], cwd=CLIENT,
                            input=(prelude + LUA_COMPARE).encode('ascii'), capture_output=True, timeout=90)
    require(result.returncode == 0, result.stderr.decode('cp949', errors='replace'))
    merged = json.loads(result.stdout.decode('ascii'))
    require(set(merged['added_ids']) == EXPECTED_IDS, 'Actual merged item ID scope mismatch')
    print(json.dumps({
        'result': 'PASS', 'installed_grf_sha256': GRF_HASH,
        'independently_extracted_entries': ENTRY_HASHES,
        'archive_order': new_order, 'new_loader_pairs': expected_pairs,
        'loader_unchanged_except_four_added_import_lines': True,
        'two_installed_fragments_equal_reviewed_bytes': True,
        'actual_lua_merged_metadata': merged,
        'active_data_ini_sha256': digest(active_ini), 'checked_loader_sha256': digest(active_loader),
        'checked_loader': str(active_loader.resolve()),
        'loader_is_active': active_loader.resolve() == (CLIENT / 'SystemEN/itemInfo.lua').resolve(),
        'lua_runtime_sha256': digest(args.lua.resolve()),
        'runtime_scope': 'Actual Lua loader/merge only; no native main, game packets, or window proof',
    }, indent=2))


if __name__ == '__main__':
    main()
