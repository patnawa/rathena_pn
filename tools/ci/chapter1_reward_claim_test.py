#!/usr/bin/env python3
# GPL-3.0-or-later. See LICENSE.
"""Chapter 1 reward claims through actual NPC helpers, VM and native inventory.

Transport, registries, achievements and quest-info callbacks are explicit doubles.
Fresh sanitizer compilation covers script.cpp, pc.cpp and the allocator. No SQL,
production world or client UI is started; registry crash durability is not proved.
"""
import argparse
import hashlib
import re
from pathlib import Path
import subprocess
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]

def prepare(build):
    build.mkdir(parents=True,exist_ok=True)
    records={}
    for row in renewal_records(ROOT,'db/item_db.yml'):records.setdefault(row['Id'],{}).update(row)
    for id in (1001972,1001973,1001974):
        row=records[id]
        assert row['Type']=='Etc' and not any(row.get(key) for key in ('Stack','Flags','Script','EquipScript','UnEquipScript')),f'Plain reward assumptions changed: {id}'
    story=(ROOT/'npc/custom/chapter1/CH1.c').read_text()
    calls=re.findall(r'callfunc\s+"F_CH1_GiveReward",\s*(\d+),\s*(\d+)\s*;',story)
    assert len(calls)==story.count('"F_CH1_GiveReward"')==40,'Chapter 1 reward call sites changed; audit the new routes'
    assert all(int(id) in (1001972,1001973,1001974) and 1<=int(amount)<=30000 for id,amount in calls),'Unsupported story reward output'
    print(f'CHAPTER1_REWARD_INPUTS callsites={len(calls)} materials=3',flush=True)
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[records[i] for i in (1001972,1001973,1001974,501)]},sort_keys=False))
    (build/'RewardClaims.txt').write_text((ROOT/'npc/custom/chapter1/RewardClaims.txt').read_text())
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    old='nums[key]=value;return true;}'
    assert old in prefix
    prefix=prefix.replace(old,'nums[key]=value;script_array_update(&attached->regs,key,value==0);return true;}',1)
    prefix=prefix.replace('BIOSPHERE TEST FAIL:','CHAPTER1 REWARD TEST FAIL:')
    (build/'driver.cpp').write_text(prefix+(ROOT/'tools/ci/chapter1_reward_claim_test.cpp').read_text())

def run(build,prepare_only=False):
    prepare(build)
    if prepare_only:return
    flags=['-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fno-omit-frame-pointer',
           '-fsanitize=address,undefined','-fno-sanitize-recover=all']
    includes=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    fresh=[]
    for source in (ROOT/'src/map/script.cpp',ROOT/'src/map/pc.cpp',ROOT/'src/common/malloc.cpp',build/'driver.cpp'):
        print(f'Fresh compile {source} sha256={hashlib.sha256(source.read_bytes()).hexdigest()}',flush=True)
        obj=build/(source.stem+'.o');subprocess.run(['g++']+flags+includes+['-c',str(source),'-o',str(obj)],cwd=ROOT,check=True);fresh.append(obj)
    objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in ('script.o','pc.o'))
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    assert objects and all(p.is_file() for p in libs),'Linux native support objects required'
    wrappers=(*WRAPPERS,'_Z9map_id2bli','_Z14pc_setregistryP16map_session_datall','_Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb')
    subprocess.run(['g++']+flags+['-o',str(build/'reward-test')]+list(map(str,fresh+objects+libs))+['-Wl,--wrap='+name for name in wrappers]+
                   ['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(build/'reward-test'),str(build)],cwd=ROOT,capture_output=True,text=True,timeout=60)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
    output=result.stdout+result.stderr
    assert 'CHAPTER1_REWARD_NATIVE_OK' in output and 'Memory manager: No memory leaks found.' in output
    assert output.count("Script command 'getitem' returned failure.")==3
    assert output.count('buildin_getitem: Failed to add the item to player.')==3
    clean=re.sub(r"\[Warning\]: Script command 'getitem' returned failure\.\n",'',output)
    clean=clean.replace('buildin_getitem: Failed to add the item to player.','')
    assert not re.search(r'\[(Error|Warning)\]|AddressSanitizer|runtime error:',clean,re.I)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build-dir',type=Path);parser.add_argument('--prepare-only',action='store_true');args=parser.parse_args()
    if args.build_dir:run(args.build_dir,args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='pn-ch1-reward-') as temp:run(Path(temp),args.prepare_only)
