#!/usr/bin/env python3
# GPL-3.0-or-later. See LICENSE.
"""Actual Alice/Bioresearch reward NPCs with native grants, quests and instance registers.

World map names/time, transport, registries and achievement/quest-info callbacks
are declared doubles. Actual group selection and certainty/weekly helpers run.
This is a synchronous claim/retry test, not SQL crash recovery or client UI.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS
ROOT=Path(__file__).resolve().parents[2]

def prepare(build):
    build.mkdir(parents=True,exist_ok=True);records={}
    for r in renewal_records(ROOT,'db/item_db.yml'):records.setdefault(r['Id'],{}).update(r)
    groups=[g for g in renewal_records(ROOT,'db/item_group_db.yml') if g.get('Group')=='BIO_W_BOX']
    assert len(groups)==1 and len(groups[0]['SubGroups'])==1 and groups[0]['SubGroups'][0]['SubGroup']==6
    names={r['AegisName']:r for r in records.values()};weapons=[names[r['Item']]['Id'] for r in groups[0]['SubGroups'][0]['List']]
    assert len(weapons)==len(set(weapons))==39
    for id in (1001074,1001082,25786,25787,102571,*weapons):
        row=records[id];flags=row.get('Flags',{})
        assert not row.get('Stack') and not flags.get('GUID') and not flags.get('AutoEquip'),f'Output capacity assumptions changed: {id}'
        assert row['Type']==('Weapon' if id in weapons else 'Usable' if id==102571 else 'Etc')
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[records[i] for i in (501,1001074,1001082,25786,25787,102571,*weapons)]},sort_keys=False))
    (build/'groups.yml').write_text(yaml.safe_dump({'Body':groups},sort_keys=False))
    (build/'weapons.inc').write_text('static const int WEAPONS[]={'+','.join(map(str,weapons))+'};\n')
    for name,file in [('Alice.txt','AliceTwistedMadness.txt'),('Bio.txt','BioresearchLaboratory.txt')]:
        (build/name).write_text((ROOT/'npc/custom/instances'/file).read_text())
    sources=['npc/custom/main_office/weekly_practice.txt','npc/custom/main_office/reward_progress.txt']
    if (ROOT/'npc/custom/instances/clear_reward_delivery.txt').is_file():sources.append('npc/custom/instances/clear_reward_delivery.txt')
    (build/'functions.txt').write_text('\n'.join((ROOT/p).read_text() for p in sources))
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    old='extern "C" npc_data* npc_lookup(int32){return nullptr;}'
    assert old in prefix;prefix=prefix.replace(old,'npc_data* instance_fixture_npc(int32);\nextern "C" npc_data* npc_lookup(int32 id){return instance_fixture_npc(id);}',1)
    old='nums[key]=value;return true;}';assert old in prefix
    prefix=prefix.replace(old,'nums[key]=value;script_array_update(&attached->regs,key,value==0);return true;}',1).replace('BIOSPHERE TEST FAIL:','INSTANCE REWARD TEST FAIL:')
    (build/'driver.cpp').write_text('#include <algorithm>\n'+prefix+(ROOT/'tools/ci/instance_reward_claim_test.cpp').read_text())
    print(f'INSTANCE_REWARD_INPUTS materials=5 weapons={len(weapons)}',flush=True)

def run(build,prepare_only=False,route=None):
    prepare(build)
    if prepare_only:return
    flags=['-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fno-omit-frame-pointer','-fsanitize=address,undefined','-fno-sanitize-recover=all']
    includes=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql','-I'+str(build)]
    fresh=[]
    for source in (ROOT/'src/map/script.cpp',ROOT/'src/map/pc.cpp',ROOT/'src/map/quest.cpp',ROOT/'src/common/malloc.cpp',build/'driver.cpp'):
        print(f'Fresh compile {source} sha256={hashlib.sha256(source.read_bytes()).hexdigest()}',flush=True)
        obj=build/(source.stem+'.o');subprocess.run(['g++']+flags+includes+['-c',str(source),'-o',str(obj)],cwd=ROOT,check=True);fresh.append(obj)
    objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in ('script.o','pc.o','quest.o'))
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers=(*WRAPPERS,'_Z9map_id2bli','_Z14pc_setregistryP16map_session_datall','_Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb',
              '_Z14clif_quest_addPK16map_session_dataPK5quest','_Z17clif_quest_deletePK16map_session_datai','_Z24clif_quest_update_statusPK16map_session_dataib','_Z27clif_quest_update_objectivePK16map_session_dataPK5quest')
    subprocess.run(['g++']+flags+['-o',str(build/'reward-test')]+list(map(str,fresh+objects+libs))+['-Wl,--wrap='+n for n in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(build/'reward-test'),str(build)]+([route] if route else []),cwd=ROOT,capture_output=True,text=True,timeout=60)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode();output=result.stdout+result.stderr
    assert 'INSTANCE_REWARD_NATIVE_OK' in output and 'Memory manager: No memory leaks found.' in output
    assert output.count("Script command 'getitem' returned failure.")==8 and output.count('buildin_getitem: Failed to add the item to player.')==8
    clean=re.sub(r"\[Warning\]: Script command 'getitem' returned failure\.\n",'',output).replace('buildin_getitem: Failed to add the item to player.','')
    assert not re.search(r'\[(Error|Warning)\]|AddressSanitizer|runtime error:',clean,re.I)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build-dir',type=Path);parser.add_argument('--prepare-only',action='store_true');parser.add_argument('--route',choices=['alice','bio']);args=parser.parse_args()
    if args.build_dir:run(args.build_dir,args.prepare_only,args.route)
    else:
        with tempfile.TemporaryDirectory(prefix='pn-instance-reward-') as temp:run(Path(temp),args.prepare_only,args.route)
