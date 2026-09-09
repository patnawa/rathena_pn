#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  client_enchant_reference_runtime_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/client_enchant_reference_runtime_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Full Lua enchant registration with the supplied 2025 client's slot fallback.

The actual Lua resources execute in Lua 5.1. C registration APIs remain recording
doubles. Only the slot callback follows independently inspected reference-client
machine code; this is not execution of that executable or 2026 client UI proof.
The strict missing-metadata regression remains separate and unchanged.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct

import chapter2_client_helper_test as helper


ROOT = Path(__file__).resolve().parents[2]


def require(value, message):
    if not value:
        raise AssertionError(message)


def reference_evidence(path):
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == helper.REFERENCE_EXE_SHA256,
            'Supplied reference executable changed')
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    require(data[pe:pe + 4] == b'PE\0\0', 'PE signature')
    sections, optional_size = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    optional = pe + 24
    require(struct.unpack_from('<H', data, optional)[0] == 0x10b, 'Expected PE32')
    base = struct.unpack_from('<I', data, optional + 28)[0]
    table = optional + optional_size

    def read(va, size):
        rva = va - base
        for index in range(sections):
            offset = table + index * 40
            address, raw_size, raw_offset = struct.unpack_from('<III', data, offset + 12)
            if address <= rva and rva + size <= address + raw_size:
                return data[raw_offset + rva - address:raw_offset + rva - address + size]
        raise AssertionError('VA has no file-backed bytes: ' + hex(va))

    expected = {
        0xfe67e0: b'C_GetSlotCount\0',
        0x644659: bytes.fromhex('68 e0 67 fe 00'),
        0x64468b: bytes.fromhex('68 50 83 64 00'),
        0x64843f: bytes.fromhex('74 68'),
        0x696990: bytes.fromhex('8b 40 30 c3'),
        0x692b0f: bytes.fromhex('b8 d8 de 22 01 74 03 8d 41 14'),
        0x692b46: bytes.fromhex('b9 d8 de 22 01'),
        0x692664: bytes.fromhex('c7 41 30 00 00 00 00'),
    }
    for va, code in expected.items():
        require(read(va, len(code)) == code, 'Inspected slot fallback bytes changed at ' + hex(va))
    for va, target in {0x648436: 0x6866c0, 0x648465: 0x696970,
                       0x696988: 0x692aa0, 0x692b4b: 0x6925e0}.items():
        code = read(va, 5)
        require(code[0] == 0xe8 and va + 5 + struct.unpack_from('<i', code, 1)[0] == target,
                'Inspected slot call target changed at ' + hex(va))
    require(struct.unpack('<d', read(0xfe68a0, 8))[0] == -1.0, 'Unknown-name result changed')
    return {'sha256': helper.REFERENCE_EXE_SHA256, 'callback_va': '0x648350',
            'known_name_missing_item_metadata_slots': 0, 'unresolved_name_result': -1,
            'default_record_va': '0x122ded8', 'slot_field_offset': '0x30'}


REFERENCE_SLOT_CALLBACK = r'''C_GetSlotCount = function(name)
    local id = item_id(name) -- All selected names must still resolve; no name invention.
    local info = client_items[id]
    if not info then
        -- Supplied unprotected 2025 client: known ID missing from item metadata
        -- uses its default item-info record, whose slot-count field is zero.
        -- Record each fallback; do not call this a real zero-slot item definition.
        missing_slots[#missing_slots + 1] = {name=name, id=id}
        return 0
    end
    assert(type(info.slotCount) == "number", "malformed existing slot metadata")
    assert(info.slotCount >= 0 and info.slotCount <= MAX_SLOT_NUM, "invalid client slotCount")
    slot_counts[name] = {id=id, slots=info.slotCount}
    return info.slotCount
end
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, default=helper.RUNTIME)
    parser.add_argument('--fixed', type=Path, default=ROOT.parent / 'chapter2-native-verified-20260906')
    args = parser.parse_args()
    evidence = reference_evidence(helper.CLIENT_ROOT / 'Ragexe_Server_20250604.exe')
    expected_hashes = {
        helper.ORIGINAL_LIST: '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d',
        helper.ORIGINAL_NAMES: '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496',
        helper.HELPER: 'bfee2e2ade0437fbb3a101e93e2e1a81986c3a484e4e96b9ec7fe10d52ac75cb',
    }
    for path, digest in expected_hashes.items():
        require(hashlib.sha256(path.read_bytes()).hexdigest() == digest, 'Original resource drift: ' + str(path))
    fixed_base = args.fixed / 'data/luafiles514/lua files'
    fixed_list, fixed_names = fixed_base / 'Enchant/EnchantList.lub', fixed_base / 'ItemDBNameTbl.lub'
    require(fixed_list.read_bytes().startswith(helper.ORIGINAL_LIST.read_bytes()), 'Original list prefix changed')
    fixture = helper.LUA_FIXTURE
    start, end = fixture.index('C_GetSlotCount = function(name)'), fixture.index('local callbacks = {')
    # Change only the explicit test double, never either actual Lua resource.
    helper.LUA_FIXTURE = fixture[:start] + REFERENCE_SLOT_CALLBACK + fixture[end:]
    try:
        reports = {
            'original': helper.invoke(args.lua, helper.ORIGINAL_NAMES, helper.ORIGINAL_LIST),
            'fixed': helper.invoke(args.lua, fixed_names, fixed_list),
        }
    finally:
        helper.LUA_FIXTURE = fixture
    for name, report in reports.items():
        require(report['loaded'], name + ' list execution failed: ' + str(report.get('load_error')))
        require(report['check_ok'] and not report['load_failed'] and not report['diagnostics'],
                name + ' helper rejects the complete list: ' + repr(report['diagnostics']))
        require(report['all_ok'], name + ' complete LoadAllData failed')
    original, fixed = reports['original'], reports['fixed']
    require(original['group_count'] == 159 and fixed['group_count'] == 164, 'Unexpected full group count')
    require(original['metadata_missing'] == fixed['metadata_missing'], 'Patch changed original missing metadata')
    old_calls = [call for call in fixed['calls'] if call['args'][0] < 167]
    require(Counter(map(helper.frozen, old_calls)) == Counter(map(helper.frozen, original['calls'])),
            'An original full-list callback changed')
    added = [call for call in fixed['calls'] if 167 <= call['args'][0] <= 171]
    require(len(fixed['calls']) == len(old_calls) + len(added), 'Unexpected callback group')
    require(sum(c['func'] == 'C_AddPerfectEnchant' for c in added) == 58, 'New recipe callback count')
    require(sum(c['func'] == 'C_AddTargetItem' for c in added) == 22, 'New target callback count')
    print(json.dumps({'result': 'PASS', 'reference_machine_code': evidence,
                      'actual_lua_groups_original': original['group_count'],
                      'actual_lua_groups_fixed': fixed['group_count'],
                      'full_original_callbacks_preserved': len(old_calls),
                      'full_fixed_callbacks': len(fixed['calls']),
                      'added_callback_counts': dict(Counter(c['func'] for c in added)),
                      'still_missing_metadata': fixed['metadata_missing'],
                      'reference_fallback_queries_original': len(original['missing_slots']),
                      'reference_fallback_queries_fixed': len(fixed['missing_slots']),
                      'scope': 'Actual Lua, 2025-reference slot double, recording C APIs; not native game execution'}, indent=2))


if __name__ == '__main__':
    main()
