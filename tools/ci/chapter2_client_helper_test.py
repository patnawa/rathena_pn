#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  chapter2_client_helper_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/chapter2_client_helper_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Execute the real client enchant helper with original, draft and fixed lists.

Requires the matching native Win32 Lua 5.1 runtime. No client files are written.
C callbacks record registration requests, not a rendered/native game window.
"""
import argparse
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from audit_enchant_upgrades import renewal_records
from audit_initial_enchants import server_configuration


ROOT = Path(__file__).resolve().parents[2]
CLIENT_ROOT = ROOT.parent.parent
SIBLING = ROOT.parent
ORIGINAL_LIST = SIBLING / 'audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub'
ORIGINAL_NAMES = SIBLING / 'audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
HELPER = SIBLING / 'audit-client-enchants-20260906/new/data/luafiles514/lua files/Enchant/EnchantList_f.lub'
RUNTIME = SIBLING / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe'
CAUTION = 'Choose carefully: enchant reset is unavailable for this equipment.'
CONSTANTS = {'MAX_SLOT_NUM': 4, 'MAX_REFINE_LEVEL': 20, 'MAX_GRADE_LEVEL': 7, 'MAX_MATERIAL_NUM': 8}
REFERENCE_EXE_SHA256 = '33d4d9af476b8d24b5954d38d121b2bbe93044681b2945bb9b235fd9b25990cb'


LUA_FIXTURE = r'''
assert(_VERSION == "Lua 5.1", "matching Lua 5.1 runtime required")
-- No process launching or shell calls are part of this resource-only harness.
os.execute = nil
io.popen = nil
local function json_string(value)
    return '"' .. string.gsub(value, '[%z\1-\31\\"]', function(c)
        return string.format('\\u%04x', string.byte(c))
    end) .. '"'
end
local function encode(value)
    local kind = type(value)
    if kind == "nil" then return "null" end
    if kind == "boolean" or kind == "number" then return tostring(value) end
    if kind == "string" then return json_string(value) end
    assert(kind == "table", "unsupported recorder value " .. kind)
    local count, maximum, isarray = 0, 0, true
    for key in pairs(value) do
        count = count + 1
        if type(key) ~= "number" or key < 1 or key ~= math.floor(key) then isarray = false
        else maximum = math.max(maximum, key) end
    end
    local parts = {}
    if isarray and count > 0 and count == maximum then
        for index = 1, maximum do parts[#parts + 1] = encode(value[index]) end
        return "[" .. table.concat(parts, ",") .. "]"
    end
    local keys = {}
    for key in pairs(value) do keys[#keys + 1] = key end
    table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
    for _, key in ipairs(keys) do parts[#parts + 1] = json_string(tostring(key)) .. ":" .. encode(value[key]) end
    return "{" .. table.concat(parts, ",") .. "}"
end
local function encode_args(...)
    local args = {}
    for i = 1, select('#', ...) do args[i] = encode(select(i, ...)) end
    return '[' .. table.concat(args, ',') .. ']'
end
local diagnostics, calls, slot_counts, missing_slots = {}, {}, {}, {}
MessageBox = function(message) diagnostics[#diagnostics + 1] = message end
-- The active loader executes its actual base/custom/override merge rules. Its
-- native main()/AddItem registration is not needed to inspect merged slotCount.
dofile(ITEMINFO_LOADER or "SystemEN/itemInfo.lua")
assert(type(tbl) == "table", "active itemInfo table missing")
local client_items = tbl
dofile(NAMES_PATH)
assert(type(ItemDB_To_ItemID) == "function", "original lookup function missing")
local function item_id(name)
    local id = ItemDB_To_ItemID(name)
    assert(type(id) == "number" and id > 0, "unresolved client identity " .. tostring(name))
    return id
end
local metadata_missing = {}
for _, name in ipairs(LIST_TARGETS) do
    local id = item_id(name)
    if not client_items[id] or type(client_items[id].slotCount) ~= "number" then
        metadata_missing[#metadata_missing + 1] = {name=name, id=id}
    end
end
C_GetSlotCount = function(name)
    local id = item_id(name)
    local info = client_items[id]
    if not info or type(info.slotCount) ~= "number" then
        missing_slots[#missing_slots + 1] = {name=name, id=id}
        error("Missing authoritative client slotCount: " .. name .. " / " .. id)
    end
    assert(info.slotCount >= 0 and info.slotCount <= MAX_SLOT_NUM, "invalid client slotCount")
    slot_counts[name] = {id=id, slots=info.slotCount}
    return info.slotCount
end
local callbacks = {
    "C_SetSlotOrder", "C_AddTargetItem", "C_SetCondition", "C_ApproveRandomOption",
    "C_SetReset", "C_SetCaution", "C_SetRequire", "C_SetSuccessRate", "C_SetGradeBonus",
    "C_SetEnchant", "C_AddPerfectEnchant", "C_AddUpgradeEnchant",
    "C_SetRandomUpgradeRequire", "C_AddRandomUpgradeEnchant", "C_AddPerfectUpgradeEnchant"
}
for _, callback in ipairs(callbacks) do
    local name = callback
    _G[name] = function(...)
        local raw = {...}
        local entry = {func=name, args_json=encode_args(...)}
        if name == "C_AddTargetItem" then entry.item_id = item_id(raw[2]) end
        if name == "C_AddPerfectEnchant" then
            entry.item_id = item_id(raw[3])
            entry.material_ids = {}
            for material, amount in pairs(raw[5]) do entry.material_ids[item_id(material)] = amount end
        end
        calls[#calls + 1] = entry
        return true, "recorded"
    end
end
IS_CLIENT = true
dofile(HELPER_PATH)
local function load_list()
    if not FRAGMENT then return dofile(LIST_PATH) end
    local input = assert(io.open(LIST_PATH, "rb"))
    local data = input:read("*a")
    input:close()
    -- Python verifies that the complete original prefix remains byte-exact.
    -- Execute the unchanged appended declarations in isolation, not a repaired
    -- copy of the incomplete original client. This is explicitly a scoped test.
    return assert(loadstring(data:sub(ORIGINAL_BYTES + 1), LIST_PATH .. ":Chapter2append"))()
end
local loaded, load_error = pcall(load_list)
local report = {loaded=loaded, load_error=load_error, slot_counts=slot_counts,
                missing_slots=missing_slots, metadata_missing=metadata_missing}
if not loaded then
    -- Register only already complete groups before the strict missing-metadata
    -- stop. Do not delete or fill the incomplete group, and do not claim that
    -- full LoadAllData ran successfully.
    calls = {}
    report.partial_groups = {}
    for group, info in pairs(Table) do
        if info.SlotOrder and info.TargetItemTbl and info.Condition and
           info.bApproveRandomOpt ~= nil and info.Reset and info.CautionMsg then
            local ok, message = GetEnchantInfo(group)
            assert(ok, message)
            report.partial_groups[#report.partial_groups + 1] = group
        end
    end
    report.partial_calls = calls
end
if loaded then
    report.check_ok, report.check_message = CheckFile()
    report.load_failed = LoadFailed
    report.direct = {}
    for group = 167, 171 do
        calls = {}
        local ok, message = GetEnchantInfo(group)
        report.direct[group] = {ok=ok, message=message, calls=calls}
    end
    calls = {}
    report.all_ok, report.all_message = LoadAllData()
    report.calls = calls
    report.group_count = 0
    for _ in pairs(Table) do report.group_count = report.group_count + 1 end
    -- Probe the real helper's duplicate contract independently after the actual
    -- list/registration report has been captured. No production Table is edited.
    local old_table, old_global, old_failed = Table, GlobalTargetItemTbl, LoadFailed
    Table, GlobalTargetItemTbl, LoadFailed = {}, {}, false
    local probe_name = next(slot_counts)
    for _, group in ipairs({900001, 900002}) do
        Table[group] = CreateEnchantInfo()
        Table[group]:SetSlotOrder(3)
    end
    local before = #diagnostics
    Table[900001]:AddTargetItem(probe_name)
    Table[900002]:AddTargetItem_Duplicate(probe_name)
    report.duplicate_cross_group_ok = not LoadFailed and #diagnostics == before
    Table[900002]:AddTargetItem_Duplicate(probe_name)
    report.duplicate_same_group_rejected = LoadFailed and #diagnostics == before + 1
    report.duplicate_probe_message = table.remove(diagnostics)
    Table, GlobalTargetItemTbl, LoadFailed = old_table, old_global, old_failed
end
report.diagnostics = diagnostics
print(encode(report))
'''


def require(ok, message):
    if not ok:
        raise AssertionError(message)


def windows_path(path):
    if os.name == 'nt':
        return str(path.resolve()).replace('\\', '/')
    return subprocess.check_output(['wslpath', '-w', str(path.resolve())], text=True).strip().replace('\\', '/')


def invoke(runtime, names, enchant_list, fragment=False, iteminfo_loader=None):
    values = '\n'.join(f'{name} = {value}' for name, value in CONSTANTS.items())
    for name, path in [('NAMES_PATH', names), ('LIST_PATH', enchant_list), ('HELPER_PATH', HELPER)]:
        values += '\n' + name + ' = ' + json.dumps(windows_path(path))
    if iteminfo_loader is not None:
        values += '\nITEMINFO_LOADER = ' + json.dumps(windows_path(iteminfo_loader))
    original = ORIGINAL_LIST.read_bytes()
    data = enchant_list.read_bytes()
    if fragment:
        require(data.startswith(original), 'Patched list must preserve the entire original prefix')
        data = data[len(original):]
    targets = sorted(set(re.findall(r':AddTargetItem(?:_Duplicate)?\("([^"]+)"', data.decode('cp949'))))
    values += '\nLIST_TARGETS = {' + ','.join(json.dumps(name, ensure_ascii=False) for name in targets) + '}'
    values += '\nFRAGMENT = ' + str(fragment).lower() + '\nORIGINAL_BYTES = ' + str(len(original))
    result = subprocess.run([str(runtime.resolve()), '-'], cwd=CLIENT_ROOT,
                            input=(values + '\n' + LUA_FIXTURE).encode('cp949'),
                            capture_output=True, timeout=90)
    require(result.returncode == 0, result.stderr.decode('cp949', errors='replace'))
    report = json.loads(result.stdout.decode('cp949'))
    for call in report.get('calls', []):
        call['args'] = json.loads(call.pop('args_json'))
    for call in report.get('partial_calls', []):
        call['args'] = json.loads(call.pop('args_json'))
    for group in report.get('direct', {}).values():
        for call in group['calls'] if isinstance(group['calls'], list) else []:
            call['args'] = json.loads(call.pop('args_json'))
    return report


def frozen(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, default=RUNTIME)
    parser.add_argument('--draft', type=Path, default=SIBLING / 'chapter2-native-candidate-20260906')
    parser.add_argument('--fixed', type=Path, default=SIBLING / 'chapter2-native-verified-20260906')
    parser.add_argument('--iteminfo-loader', type=Path,
                        help='Explicit historical loader checkpoint; default is the active client loader')
    args = parser.parse_args()
    expected_hashes = {
        ORIGINAL_LIST: '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d',
        ORIGINAL_NAMES: '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496',
        HELPER: 'bfee2e2ade0437fbb3a101e93e2e1a81986c3a484e4e96b9ec7fe10d52ac75cb',
        CLIENT_ROOT / 'Ragexe_Server_20250604.exe': REFERENCE_EXE_SHA256,
    }
    for path, expected in expected_hashes.items():
        require(hashlib.sha256(path.read_bytes()).hexdigest() == expected, 'Source drift: ' + str(path))
    mmo = (ROOT / 'src/common/mmo.hpp').read_text()
    require(re.search(r'^#define MAX_SLOTS 4\s*$', mmo, re.M), 'Server MAX_SLOTS differs from reference client fixture')
    version = subprocess.run([str(args.lua.resolve()), '-v'], capture_output=True, check=True)
    require(b'Lua 5.1' in version.stdout + version.stderr, 'Matching native Lua 5.1 required')
    reports = {'original': invoke(args.lua, ORIGINAL_NAMES, ORIGINAL_LIST,
                                  iteminfo_loader=args.iteminfo_loader)}
    for name, directory in [('draft', args.draft), ('fixed', args.fixed)]:
        base = directory / 'data/luafiles514/lua files'
        reports[name] = invoke(args.lua, base / 'ItemDBNameTbl.lub', base / 'Enchant/EnchantList.lub',
                               iteminfo_loader=args.iteminfo_loader)
    original = reports['original']
    require(not original['loaded'] and len(original['metadata_missing']) == 43,
            'Original metadata baseline changed; review and update the scoped coverage checkpoint')
    for name, report in reports.items():
        require(not report['loaded'] and 'Missing authoritative client slotCount: Solid_Whinger / 510189' in report['load_error'],
                name + ' must report the same strict baseline failure')
        require(report['metadata_missing'] == original['metadata_missing'], name + ' changes missing target metadata')
        require(Counter(map(frozen, report['partial_calls'])) == Counter(map(frozen, original['partial_calls'])),
                name + ' changes complete prefix-group registrations')
    fragments = {}
    for name, directory in [('draft', args.draft), ('fixed', args.fixed)]:
        base = directory / 'data/luafiles514/lua files'
        fragments[name] = invoke(args.lua, base / 'ItemDBNameTbl.lub', base / 'Enchant/EnchantList.lub',
                                 fragment=True, iteminfo_loader=args.iteminfo_loader)
        report = fragments[name]
        require(report['loaded'] and not report['metadata_missing'], name + ' appended groups lack authoritative slot metadata')
        require(report['all_ok'], name + ' scoped LoadAllData failed')
        require(report['group_count'] == 5, 'Expected exactly five isolated appended groups')
        require(report['duplicate_cross_group_ok'] and report['duplicate_same_group_rejected'], 'Helper duplicate contract mismatch')
    draft, fixed = fragments['draft'], fragments['fixed']
    for group in range(167, 172):
        require(draft['direct'][str(group)]['ok'] and fixed['direct'][str(group)]['ok'], 'Patched GetEnchantInfo failed')
    require(not fixed['diagnostics'] and fixed['check_ok'], 'Fixed appended groups fail actual CheckFile')
    added = Counter(draft['diagnostics'])
    require(sum(added.values()) == 5 and all('Caution message does not exist.' in text for text in added),
            'Draft does not reproduce exactly five missing-caution errors')
    require(not draft['check_ok'], 'Actual CheckFile must reject missing draft cautions')
    groups = server_configuration(ROOT)
    ids = {r['AegisName']: r['Id'] for r in renewal_records(ROOT, 'db/item_db.yml') if 'AegisName' in r}
    count = 0
    for group in range(167, 172):
        callbacks = [c for c in fixed['calls'] if c['args'][0] == group]
        direct = fixed['direct'][str(group)]['calls']
        require(Counter(map(frozen, callbacks)) == Counter(map(frozen, direct)), 'GetEnchantInfo/LoadAllData differ')
        by_function = {}
        for call in callbacks:
            by_function.setdefault(call['func'], []).append(call)
        for scalar in ('C_SetSlotOrder', 'C_SetCondition', 'C_ApproveRandomOption', 'C_SetReset', 'C_SetCaution'):
            require(len(by_function.get(scalar, [])) == 1, 'Expected exactly one ' + scalar + ' callback')
        expected = groups[group]
        require(by_function['C_SetSlotOrder'][0]['args'] == [group, expected['Order']], 'Slot order mismatch')
        require(by_function['C_SetCondition'][0]['args'] == [group, expected['MinimumRefine'], expected['MinimumEnchantgrade']], 'Condition mismatch')
        require(by_function['C_ApproveRandomOption'][0]['args'] == [group, expected['AllowRandomOptions']], 'Random-option mismatch')
        require(by_function['C_SetReset'][0]['args'] == [group, False, 0, 0, {}], 'Reset mismatch')
        require(by_function['C_SetCaution'][0]['args'] == [group, CAUTION], 'Caution mismatch')
        targets = by_function['C_AddTargetItem']
        require(len(targets) == len(expected['Targets']), 'Target callback count mismatch')
        require({c['args'][1] for c in targets} == expected['Targets'], 'Target names mismatch')
        require({c['item_id'] for c in targets} == {ids[n] for n in expected['Targets']}, 'Target IDs mismatch')
        actual_recipes = {}
        for call in by_function['C_AddPerfectEnchant']:
            _, slot, name, price, materials = call['args']
            key = (slot, name)
            require(key not in actual_recipes, 'Duplicate perfect callback')
            actual_recipes[key] = {'Price': price, 'Materials': materials}
            require(call['item_id'] == ids[name], 'Enchant ID mismatch')
            require(call['material_ids'] == {str(ids[m]): amount for m, amount in materials.items()}, 'Material ID/amount mismatch')
            count += 1
        desired = {(slot, name): recipe for slot, data in expected['Slots'].items() for name, recipe in data['Perfect'].items()}
        require(actual_recipes == desired, 'Exact recipe payload mismatch')
        require(set(by_function) == {'C_SetSlotOrder', 'C_AddTargetItem', 'C_SetCondition', 'C_ApproveRandomOption',
                                     'C_SetReset', 'C_SetCaution', 'C_AddPerfectEnchant'}, 'Unexpected new callback type')
    require(count == 58, 'Expected exactly 58 Chapter 2 recipe callbacks')
    print(json.dumps({
        'result': 'PASS', 'runtime': str(args.lua), 'constants': CONSTANTS,
        'checked_iteminfo_loader': str((args.iteminfo_loader or CLIENT_ROOT / 'SystemEN/itemInfo.lua').resolve()),
        'loader_is_active': args.iteminfo_loader is None or args.iteminfo_loader.resolve() == (CLIENT_ROOT / 'SystemEN/itemInfo.lua').resolve(),
        'constants_scope': 'Disassembled supplied unprotected 2025 reference executable; not protected 2026 runtime proof',
        'full_original_and_patched_load': 'BLOCKED identically by original missing target metadata; no zero fallback',
        'original_missing_target_metadata': original['metadata_missing'],
        'draft_scoped_caution_errors': list(added.elements()), 'fixed_scoped_checkfile_ok': True,
        'unchanged_complete_prefix_groups': sorted(original['partial_groups']),
        'unchanged_original_prefix_callback_count': len(original['partial_calls']),
        'new_targets': 22, 'new_perfect_recipe_callbacks': count,
        'known_original_target_slot_counts': len(original['slot_counts']),
        'known_fixed_target_slot_counts': len(fixed['slot_counts']),
        'get_enchant_info_and_load_all_data_equal': True,
        'cross_group_duplicate_allowed_same_group_duplicate_rejected': True,
    }, indent=2, ensure_ascii=False))


if __name__ == '__main__':
    main()
