#!/usr/bin/env python3
# GPL-3.0-or-later. See LICENSE.
"""Actual Confused Boy VM, loaded recipes, native barter/inventory/payment.

Transport, registries, actor/NPC lookup and legacy Zeny setter are explicit
doubles. Quest-info conditions execute the actual VM. No sockets or SQL;
unlimited ordinary exchanges do not exercise durable crash recovery.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import yaml
from alice_test import RECIPES
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS
ROOT = Path(__file__).resolve().parents[2]

def prepare(build, source=None, legacy=False):
    build.mkdir(parents=True, exist_ok=True)
    records = {}
    for row in renewal_records(ROOT, 'db/item_db.yml'):
        records.setdefault(row['Id'], {}).update(row)
    ids = {501}
    for recipe in RECIPES:
        ids.add(recipe['output']); ids.update(map(int, recipe['materials']))
        assert records[recipe['output']]['Type'] == 'Armor'
        assert not records[recipe['output']].get('Flags', {}).get('AutoEquip')
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[records[i] for i in sorted(ids)]}, sort_keys=False))
    source = source or ROOT/'npc/custom/instances/AliceTwistedMadness.txt'
    (build/'Alice.txt').write_text(source.read_text())
    h = ['struct Recipe { int output,zeny; std::map<int,int> costs; };', 'static Recipe recipes[] = {']
    for r in RECIPES:
        h.append('{'+str(r['output'])+','+str(r['zeny'])+',{'+','.join('{'+str(k)+','+str(v)+'}' for k,v in r['materials'].items())+'}},')
    h.append('};'); (build/'recipes.inc').write_text('\n'.join(h))
    if not legacy:
        catalog = yaml.safe_load((ROOT/'npc/custom/instances/alice_barters.yml').read_text())
        assert len(catalog['Body']) == 1 and catalog['Body'][0]['Name'] == 'barter_alice_equipment'
        rows = catalog['Body'][0]['Items']; assert len(rows) == len(RECIPES)
        names = {v['AegisName']: k for k,v in records.items()}
        for index, (row, expected) in enumerate(zip(rows, RECIPES)):
            assert row['Index'] == index and names[row['Item']] == expected['output']
            assert row['Zeny'] == expected['zeny'] and not row.get('Stock', 0)
            assert {str(names[c['Item']]):c['Amount'] for c in row['RequiredItems']} == expected['materials']
            assert [c['Index'] for c in row['RequiredItems']] == list(range(len(expected['materials'])))
        imports = yaml.safe_load((ROOT/'npc/custom/barters.yml').read_text())['Footer']['Imports']
        assert {'Path':'npc/custom/instances/alice_barters.yml'} in imports
        (build/'barters.yml').write_text(yaml.safe_dump(catalog, sort_keys=False))
    prefix = (ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix = prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");', '')
    prefix = prefix.replace('extern "C" void quest(map_session_data*){}',
        'extern "C" void real_quest(map_session_data*) asm("__real__Z17pc_show_questinfoP16map_session_data"); extern "C" void quest(map_session_data* sd){real_quest(sd);}')
    prefix = prefix.replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}',
        'npc_data* quest_npc=nullptr; extern "C" npc_data* npc_lookup(int32){return quest_npc;}')
    prefix = prefix.replace('nums[key]=value;return true;}', 'nums[key]=value;script_array_update(&attached->regs,key,value==0);return true;}',1)
    prefix = prefix.replace('BIOSPHERE TEST FAIL:', 'ALICE EXCHANGE TEST FAIL:')
    npc = (ROOT/'src/map/npc.cpp').read_text()
    barter = npc[npc.index('e_purchase_result npc_barter_purchase('):npc.index('//Atempt to remove an npc')]
    barter = barter.replace('npc_barter_purchase(', 'audit_npc_barter_purchase(',1)
    # Extract exact production functions; only names and the declared quest-info
    # boundary are redirected. The actual native scope still defers callbacks.
    pc = (ROOT/'src/map/pc.cpp').read_text()
    deletion = pc[pc.index('char pc_delitem('):pc.index(' * Attempt to drop an item.')]
    deletion = deletion[:deletion.rfind('/*')].replace('pc_delitem(', 'audit_pc_delitem(',1).replace('pc_show_questinfo(sd)', 'quest(sd)')
    barter = barter.replace('pc_delitem(', 'audit_pc_delitem(')
    planner = (ROOT/'src/custom/shop_map.inc').read_text()
    planner = planner[planner.index('bool pn_shop_plan_inventory('):planner.index('bool pn_shop_begin(')]
    planner = planner.replace('pn_shop_plan_inventory(', 'audit_pn_shop_plan_inventory(',1)
    barter = barter.replace('pn_shop_plan_inventory(', 'audit_pn_shop_plan_inventory(')
    driver = (ROOT/'tools/ci/alice_exchange_transaction_test.cpp').read_text()
    driver = driver.replace('// FUNCTIONS', planner+deletion+barter)
    (build/'driver.cpp').write_text('#include <algorithm>\n'+prefix+driver)
    print('ALICE_EXCHANGE_INPUTS recipes=9 exact_prices=true', flush=True)

def run(build, args):
    prepare(build, args.source, bool(args.legacy_case))
    if args.prepare_only: return
    flags = ['-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fno-omit-frame-pointer','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    includes = ['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql','-I'+str(build)]
    fresh = []
    for path in ('src/map/script.cpp','src/map/pc.cpp','src/map/quest.cpp','src/common/malloc.cpp'):
        source = ROOT/path; obj = build/(source.stem+'.o')
        if args.cached_objects:
            obj = args.cached_objects/obj.name
            assert obj.is_file(), 'Explicit debug object missing'
        else:
            print(f'Fresh compile {source} sha256={hashlib.sha256(source.read_bytes()).hexdigest()}', flush=True)
            subprocess.run(['g++']+flags+includes+['-c',str(source),'-o',str(obj)], check=True, cwd=ROOT)
        fresh.append(obj)
    subprocess.run(['g++']+flags+includes+['-c',str(build/'driver.cpp'),'-o',str(build/'driver.o')], check=True, cwd=ROOT)
    objects = sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in ('script.o','pc.o','quest.o'))
    libs = [ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers = (*WRAPPERS, '_Z11pc_setparamP16map_session_datall', '_Z11npc_name2idPKc', '_Z25clif_barter_extended_openR16map_session_dataR8npc_data')
    subprocess.run(['g++']+flags+['-o',str(build/'exchange-test')]+list(map(str,fresh+[build/'driver.o']+objects+libs))+['-Wl,--wrap='+n for n in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm'], check=True, cwd=ROOT)
    result = subprocess.run([str(build/'exchange-test'),str(build)]+([args.legacy_case] if args.legacy_case else []), cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(result.stdout,end=''); print(result.stderr,end=''); result.check_returncode()
    assert 'ALICE_EXCHANGE_NATIVE_OK' in result.stdout and 'No memory leaks found.' in result.stdout+result.stderr
    assert not re.search(r'\[(Error|Warning)\]|AddressSanitizer|runtime error:', result.stdout+result.stderr,re.I)

if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path); p.add_argument('--source',type=Path)
    p.add_argument('--prepare-only',action='store_true')
    p.add_argument('--cached-objects',type=Path,help='Explicit debug-only objects; final validation compiles fresh')
    p.add_argument('--legacy-case',choices=['slots','callback'])
    args = p.parse_args()
    if args.build_dir: run(args.build_dir.resolve(),args)
    else:
        with tempfile.TemporaryDirectory(prefix='pn-alice-exchange-') as d: run(Path(d),args)
