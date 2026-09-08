#!/usr/bin/env python3
"""Read-only audit of the installed Chapter 2 maps and enchant registration.

Unlike the dated installation receipts, this allows subsequent independent
patches. It verifies resource precedence, actual Lua callbacks, exact current
server recipes, and client/server walkability. No game window is exercised.
"""
import configparser
import hashlib
import json
from pathlib import Path
import struct
import sys
import tempfile
import zlib

import chapter2_client_helper_test as helper
from chapter2_native_patch_test import independent_grf_reader
from audit_initial_enchants import server_configuration, client_configuration
from audit_enchant_upgrades import renewal_records, ClientItemNames, client_recipes, server_recipes, canonical

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from build_main_office import cache_records

ROOT = Path(__file__).resolve().parents[2]
CLIENT = ROOT.parent.parent


def audit():
    ini = configparser.ConfigParser()
    ini.read(CLIENT / 'DATA.INI')
    order = [name for key, name in sorted(ini['Data'].items(), key=lambda p: int(p[0]))]
    assert order.count('chapter2_native.grf') == order.count('chapter2_maps.grf') == 1
    # Every higher-priority archive is one of our small independently readable
    # patches. Unknown archives require inspection, never silently ignoring them.
    effective = {}
    stop = max(order.index('chapter2_native.grf'), order.index('chapter2_maps.grf'))
    for archive in order[:stop + 1]:
        for name, raw in independent_grf_reader((CLIENT / archive).read_bytes()).items():
            effective.setdefault(name.replace('\\', '/').lower(), (archive, raw))
    resources = {
        'names': 'data/luafiles514/lua files/itemdbnametbl.lub',
        'list': 'data/luafiles514/lua files/enchant/enchantlist.lub',
    }
    for name in resources.values():
        assert effective[name][0] == 'chapter2_native.grf', ('Unexpected override', name)
        assert not (CLIENT / name).exists(), ('Loose override', name)

    items = {r['AegisName']: r['Id'] for r in renewal_records(ROOT, 'db/item_db.yml') if 'AegisName' in r}
    groups = server_configuration(ROOT)
    with tempfile.TemporaryDirectory(prefix='ch2-current-', dir=ROOT.parent) as tmp:
        temp = Path(tmp)
        names, listing = temp / 'ItemDBNameTbl.lub', temp / 'EnchantList.lub'
        names.write_bytes(effective[resources['names']][1])
        listing.write_bytes(effective[resources['list']][1])
        scoped = helper.invoke(helper.RUNTIME, names, listing, fragment=True)
        full = helper.invoke(helper.RUNTIME, names, listing)
        resolve = ClientItemNames(names, ROOT)
        client_groups = client_configuration(listing, resolve)
        client_upgrades, client_perfect = client_recipes(listing, resolve)
        server_upgrade_data = server_recipes(ROOT)
        for group in [*range(70, 89), 128, 166]:
            # Disabled reset payloads never execute; the client retains unused
            # historical prices which the server deliberately omits.
            if not client_groups[group]['Reset']['Enabled'] and not groups[group]['Reset']['Enabled']:
                client_groups[group]['Reset'] = groups[group]['Reset']
            assert client_groups[group] == groups[group], ('Shadow initial recipe mismatch', group)
            assert {k: canonical(v) for k, v in client_upgrades.items() if k[0] == group} == {
                k: canonical(v) for k, v in server_upgrade_data[1].items() if k[0] == group}, ('Shadow upgrades mismatch', group)
            assert {k: canonical(v) for k, v in client_perfect.items() if k[0] == group} == {
                k: canonical(v) for k, v in server_upgrade_data[2].items() if k[0] == group}, ('Shadow perfect upgrades mismatch', group)
    assert scoped['loaded'] and scoped['all_ok'] and scoped['check_ok']
    assert not scoped['metadata_missing'] and not scoped['diagnostics']
    assert scoped['group_count'] == 5
    recipe_count = target_count = 0
    for group in range(167, 172):
        calls = [c for c in scoped['calls'] if c['args'][0] == group]
        expected = groups[group]
        targets = [c for c in calls if c['func'] == 'C_AddTargetItem']
        assert {c['item_id'] for c in targets} == {items[n] for n in expected['Targets']}
        target_count += len(targets)
        recipes = {(c['args'][1], c['args'][2]): {'Price': c['args'][3], 'Materials': c['args'][4]}
                   for c in calls if c['func'] == 'C_AddPerfectEnchant'}
        desired = {(slot, name): recipe for slot, data in expected['Slots'].items()
                   for name, recipe in data['Perfect'].items()}
        assert recipes == desired, ('Recipe mismatch', group)
        recipe_count += len(recipes)
        for func, args in [('C_SetSlotOrder', [group, expected['Order']]),
                           ('C_SetCondition', [group, expected['MinimumRefine'], expected['MinimumEnchantgrade']]),
                           ('C_ApproveRandomOption', [group, expected['AllowRandomOptions']])]:
            assert [c['args'] for c in calls if c['func'] == func] == [args]
    assert (target_count, recipe_count) == (22, 58)

    cache = {}
    for rel in ['db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat']:
        if (ROOT / rel).exists():
            for name, raw in cache_records(ROOT / rel).items():
                cache.setdefault(name, raw)
    checked = []
    for name, (archive, gat) in effective.items():
        if archive != 'chapter2_maps.grf' or not name.endswith('.gat'):
            continue
        alias = Path(name).stem
        assert not (CLIENT / name).exists(), ('Loose map override', name)
        record = cache[alias]
        w, h = struct.unpack_from('<hh', record, 12)
        assert gat[:6] in (b'GRAT\x01\x02', b'GRAT\x01\x03')
        assert len(gat) == 14 + 20*w*h
        assert struct.unpack_from('<II', gat, 6) == (w, h)
        cells = zlib.decompress(record[20:])
        assert all((cells[i] in (0, 3)) == (struct.unpack_from('<I', gat, 30 + 20*i)[0] in (0, 3))
                   for i in range(w*h)), ('Walkability mismatch', alias)
        for ext in ['.gat', '.gnd', '.rsw']:
            assert effective['data/' + alias + ext][0] == archive
        checked.append(alias)
    assert checked, 'No Chapter 2 maps checked'
    return {'chapter2_result': 'PASS', 'archive_order': order,
            'native_grf_sha256': hashlib.sha256((CLIENT / 'chapter2_native.grf').read_bytes()).hexdigest(),
            'map_walkability_verified': sorted(checked), 'enchant_targets': target_count,
            'exact_enchant_recipes': recipe_count,
            'shadow_groups_current_recipe_equality': [*range(70, 89), 128, 166],
            'full_registry_loaded': full['loaded'],
            'full_registry_missing_metadata': full['metadata_missing'],
            'scope': 'Installed resources and Lua callbacks; not a rendered game or completed quest playthrough.'}


if __name__ == '__main__':
    print(json.dumps(audit(), indent=2))
