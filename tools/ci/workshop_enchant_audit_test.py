#!/usr/bin/env python3
# Copyright (C) 2026 PN Development Team. GPL-3.0-or-later; see LICENSE.
"""Actual Workshop NPC VM routes, Star of Spell transactions and menu capacity.

Fresh script/allocator/driver compile against Linux map support objects. World,
transport and register persistence are explicit doubles; inventory mutation,
material deletion and script execution are real. No server or SQL is started.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile
import yaml

from audit_enchant_upgrades import renewal_records
from audit_initial_enchants import server_configuration
from biosphere_crown_transaction_test import WRAPPERS

ROOT = Path(__file__).resolve().parents[2]


def routes():
    result = []
    def add(answers, group=0):
        result.append((answers, group))
    direct = {2:63,6:25,7:21,8:20,9:15,11:64,13:103,14:33,15:22,16:165,18:32,19:150,20:128,23:23}
    families = {1:[26,27,28,29,30,31],3:[147,148,149],
        4:[65,66,67,106,107,108,109,110,111,114,115,116,125,126,127,129,130,131,134,135,136],
        5:[1,2,3,4,5,137,138,139,140,141],10:list(range(70,89)),12:list(range(7,13)),
        21:[16,17,18,19,57,58,59],22:[52,53,54,55,60,61,62,99,100,101,102]}
    for choice, group in direct.items(): add([choice],group)
    for choice, groups in families.items():
        for sub, group in enumerate(groups,1): add([choice,sub],group)
        add([choice,len(groups)+1,25]); add([choice,255])
    add([17,1],13); add([17,3,25]); add([17,255])
    for choice in range(1,6): add([24,1,choice],[63,64,147,148,165][choice-1])
    add([24,1,6]); add([24,1,255])
    add([24,2,1],15); add([24,2,2]); add([24,2,255]); add([24,3])
    for choice in (1,2): add([24,4,choice],162+choice)
    add([24,4,3]); add([24,4,255])
    for tier in (1,2):
        for element in range(1,5): add([24,5,tier,element],(15 if tier==1 else 51)+element)
        add([24,5,tier,5]); add([24,5,tier,255])
    add([24,5,3]); add([24,5,255])
    for choice in range(1,8): add([24,6,choice],6+choice)
    add([24,6,9]); add([24,6,255])
    add([24,7,1],128); add([24,7,2]); add([24,7,3],166)
    for choice in range(1,20): add([24,7,4,choice],69+choice)
    add([24,7,4,20]); add([24,7,4,255]); add([24,7,255])
    for category in (1,2):
        for season in range(1,5): add([24,8,category,season],114+category+season*2)
        add([24,8,category,5]); add([24,8,category,255])
    add([24,8,3],142); add([24,8,4]); add([24,8,255])
    add([24,9]); add([24,255]); add([25]); add([255])
    return result


def prepare(build):
    build.mkdir(parents=True,exist_ok=True)
    for source, target in [('npc/custom/grademk_equipment_enchants.txt','equipment.txt'),
                           ('npc/custom/grademk_services.txt','services.txt'),
                           ('npc/other/Global_Functions.txt','functions.txt')]:
        (build/target).write_text((ROOT/source).read_text())
    groups = server_configuration(ROOT)
    route_rows = routes()
    needed = {group for _,group in route_rows if group}
    for group in needed:
        assert group in groups and groups[group]['Targets'] and groups[group]['Slots'], f'Unavailable group {group}'
    records = {}
    for row in renewal_records(ROOT,'db/item_db.yml'): records.setdefault(row['Id'],{}).update(row)
    item_ids = {490136,310709,310710,310711,1002139,1002137,1002143,1002144,1002145,1002146,
                410233,5918,410004,410232}
    assert item_ids <= records.keys(), 'Transaction identity missing'
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[records[id] for id in sorted(item_ids)]},sort_keys=False))
    reps = [r for r in renewal_records(ROOT,'db/reputation.yml') if r['Id']==9]
    assert len(reps)==1
    (build/'reputation.yml').write_text(yaml.safe_dump({'Body':reps},sort_keys=False))
    (build/'routes.inc').write_text('static const std::vector<Route> ROUTES = {\n'+
        ',\n'.join('{{'+','.join(map(str,path))+'},'+str(group)+'}' for path,group in route_rows)+'\n};\n'+
        'static const uint64 GROUPS[] = {'+','.join(map(str,sorted(needed)))+'};\n')
    prefix = (ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    old='nums[key]=value;return true;}'
    assert old in prefix
    prefix=prefix.replace(old,'nums[key]=value;script_array_update(&attached->regs,key,value==0);return true;}',1)
    prefix=prefix.replace('"BIOSPHERE TEST FAIL:', '"WORKSHOP TEST FAIL:')
    old='extern "C" void menu(map_session_data&,uint32,const char* text){menu_text.emplace_back(text);}'
    assert old in prefix
    prefix=prefix.replace(old,'''extern "C" void menu(map_session_data&,uint32,const char* text){
        check(std::strlen(text)<2047,"NPC menu stays below the client truncation limit");
        check(1+std::count(text,text+std::strlen(text),':')<=254,"NPC choices fit the client byte index");
        menu_text.emplace_back(text);
    }''')
    (build/'driver.cpp').write_text('#include <algorithm>\n#include <set>\n'+prefix+
        (ROOT/'tools/ci/workshop_enchant_audit_test.cpp').read_text())
    print(f'WORKSHOP_INPUTS routes={len(route_rows)} groups={len(needed)}',flush=True)


def run(build, prepare_only=False):
    prepare(build)
    if prepare_only: return
    if not prepare_only:
        flags=['-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing',
               '-fno-omit-frame-pointer','-fsanitize=address,undefined','-fno-sanitize-recover=all']
        includes=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
                  '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql','-I'+str(build)]
        fresh=[]
        for source in (ROOT/'src/map/script.cpp',ROOT/'src/common/malloc.cpp',build/'driver.cpp'):
            print(f'Fresh compile {source} sha256={hashlib.sha256(source.read_bytes()).hexdigest()}',flush=True)
            obj=build/(source.stem+'.o')
            subprocess.run(['g++']+flags+includes+['-c',str(source),'-o',str(obj)],cwd=ROOT,check=True)
            fresh.append(obj)
        objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name!='script.o')
        libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        assert objects and all(p.is_file() for p in libs),'Linux native support objects required'
        wrappers=(*WRAPPERS,'_Z18clif_specialeffectPK10block_listi11send_target','_Z9map_id2bli')
        subprocess.run(['g++']+flags+['-o',str(build/'workshop-test')]+list(map(str,fresh+objects+libs))+
            ['-Wl,--wrap='+name for name in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(build/'workshop-test'),str(build)],cwd=ROOT,capture_output=True,text=True,timeout=60)
    print(result.stdout,end='');print(result.stderr,end='')
    result.check_returncode()
    output=result.stdout+result.stderr
    assert 'WORKSHOP_ENCHANT_NATIVE_OK' in output and 'Memory manager: No memory leaks found.' in output
    assert not re.search(r'\[(Error|Warning)\]|AddressSanitizer|runtime error:',output,re.I)


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--prepare-only',action='store_true')
    args=parser.parse_args()
    if args.build_dir: run(args.build_dir.resolve(),args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='workshop-enchant-') as tmp: run(Path(tmp),args.prepare_only)
