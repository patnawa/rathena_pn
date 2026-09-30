#!/usr/bin/env python3
"""Costume bonuses through native combo discovery and full status recalculation.

Independent expectations are hand-authored from official descriptions. This is
an isolated synthetic player; packets/world persistence are explicit boundaries.
Requires Linux map link objects. Never starts a game server or accesses SQL.
"""
import argparse
import shutil
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]

def prepare(build):
    # Source: official Taiwan Apr7 2022 Range update and Sept22 2026 itemInfo;
    # see doc/costume_taiwan_reference_20260928.md. Values are not parsed scripts.
    cases=[
      dict(name='bare',expected={'short':0,'long':0,'magic_all':0,'fixed':0,'vct':0}),
      dict(name='23rd-alone',garment=[300713],expected={'short':3,'long':3,'magic_all':3,'fixed':-200,'vct':5,'delay':-3}),
      dict(name='22nd-alone',garment=[0,313524],expected={'short':11,'long':11,'magic_all':11}),
      dict(name='anniversary-pair',garment=[300713,313524],expected={'short':14,'long':14,'magic_all':14}),
      dict(name='range-native',upper=[310325],middle=[0,310330],lower=[0,0,310326],expected={'long':15}),
      dict(name='range-legacy',upper=[310325],middle=[0,29048],lower=[0,0,310326],expected={'long':15}),
      dict(name='range-missing-middle',upper=[310325],lower=[0,0,310326],expected={'long':6}),
      dict(name='range-unrelated-expert',upper=[310325],lower=[0,0,310326],armor=[29048],expected={'long':9}),
      dict(name='23rd-range-native',garment=[300713],upper=[310325],middle=[0,310330],lower=[0,0,310326],expected={'long':23}),
      dict(name='23rd-range-legacy',garment=[300713],upper=[310325],middle=[0,29048],lower=[0,0,310326],expected={'long':23}),
      dict(name='23rd-range-unrelated',garment=[300713],upper=[310325],lower=[0,0,310326],armor=[29048],expected={'long':12}),
      dict(name='23rd-delay-set',garment=[300713],upper=[29053],middle=[0,29054],lower=[0,0,29055],expected={'short':6,'long':6,'magic_all':6,'delay':-13}),
      dict(name='23rd-cast-set',garment=[300713],upper=[29156],middle=[0,29157],lower=[0,0,29158],expected={'short':6,'long':6,'magic_all':6,'vct':30}),
    ]
    # Five independently transcribed official Korean boundary examples in
    # doc/costume_source_verification_20260928.md (not the script-derived JSON).
    for level,half in ((0,0),(1,0),(2,1),(9,4),(10,5)):
        cases += [
          dict(name=f'ranger-delay-{level}',upper=[315325],learned={'RA_ARROWSTORM':level},expected={'delay':-half}),
          dict(name=f'ranger-size-{level}',lower=[0,0,315327],learned={'RA_AIMEDBOLT':level},expected={'size_all':3*half}),
          dict(name=f'shinkiro-combo-{level}',garment=[315332],lower=[0,0,315335],learned={'SS_KAGENOMAI':level},expected={'short':3*half,'crit':13+10*half}),
          dict(name=f'nightwatch-combo-{level}',garment=[315059],lower=[0,0,315068],learned={'NW_P_F_I':level},expected={'fixed':-100*half,'size_all':3*half}),
        ]
    for level in (0,1,4,5):
        cases.append(dict(name=f'sorcerer-fixed-{level}',upper=[315329],learned={'SO_PSYCHIC_WAVE':level},expected={'fixed':-100*level}))
    # Official Thai metadata: all types/classes; these are not race bonuses.
    for card,label,extra in ((313738,'str',{'batk_bonus':100}),(313739,'luk',{'crit_bonus':20}),
                            (313740,'dex',{'long':10}),(313741,'int',{'matk_bonus':100}),
                            (313742,'vit',{'hp_rate':120}),(313743,'agi',{'hit':237})):
        cases.append(dict(name='purified-'+label,garment=[card],expected={'delay':-10,'physical_class':10,'magic_class':10,**extra}))
    cases.append(dict(name='purified-ultimate',garment=[313744],expected={'delay':-20,'physical_class':20,'magic_class':20,'long':10,'crit_bonus':20,'matk_bonus':100,'batk_bonus':100,'hp_rate':120,'hit':237}))
    cases.append(dict(name='festa-supreme',upper=[0,314796],expected={'pow':10,'sta':10,'wis':10,'spl':10,'con':10,'crt':10,'delay':-10,'short':10,'long':10,'hp_rate':110,'sp_rate':110,'matk_rate':110,'greed':1}))
    # Separately assert native battle-stage damage for a controlled 10000-point
    # magic hit against an unresistant neutral player target.
    for case in cases:
        # Soul Strike Lv1: 400 ms variable +100 ms fixed cast, 1400 ms
        # delay. Fixture INT/DEX=1; Renewal scale530 gives369 ms variable.
        if case['name']=='bare':case['timing']={'soulstrike_cast':469,'soulstrike_delay':1400}
        if case['name']=='23rd-alone':case['timing']={'soulstrike_cast':351,'soulstrike_delay':1358}
        if case['name']=='23rd-cast-set':case['timing']={'soulstrike_cast':258,'soulstrike_delay':1358}
        if case['name']=='23rd-delay-set':case['timing']={'soulstrike_delay':1218}
        if case['name'].startswith('purified-'):case['timing']={'soulstrike_delay':1120 if case['name']=='purified-ultimate' else 1260}
        if case['name'] in ('bare','23rd-alone','22nd-alone','anniversary-pair','23rd-delay-set','23rd-cast-set'):
            case['expected']['magic_hit']=10000+case['expected']['magic_all']*100
        elif case['name'].startswith('purified-'):
            case['expected']['magic_hit']=12000 if case['name']=='purified-ultimate' else 11000
    # Renewal physical cardfix covers class/size categories; ranged/melee
    # damage bonuses occur later in the attack pipeline and are not asserted
    # as part of this stage.
    for case in cases:
        if case['name'].startswith('purified-'):
            hit=12000 if case['name']=='purified-ultimate' else 11000
            case['expected'].update(weapon_cardfix_hit=hit,projectile_cardfix_hit=hit)
        if case['name'].startswith('ranger-size-'):
            hit=10000+300*(int(case['name'].rsplit('-',1)[1])//2)
            case['expected'].update(weapon_cardfix_hit=hit,projectile_cardfix_hit=hit)
        if case['name'] in ('range-native','range-legacy'):
            case['remove_middle']={'long':6}
        if case['name'] in ('23rd-range-native','23rd-range-legacy'):
            case['remove_middle']={'long':9}
    cases.append(dict(name='purified-ranger-size-stack',garment=[313741],lower=[0,0,315327],
        learned={'RA_AIMEDBOLT':10},expected={'physical_class':10,'size_all':15,
        'weapon_cardfix_hit':12650,'projectile_cardfix_hit':12650}))
    for case in cases:case['kind']='independent'
    # Exhaustive unique-output smoke coverage has no fabricated numeric oracle.
    # Exercise the catalogue's actual position, including effect slot4/Festa2.
    locations={0:('upper',0),1:('middle',1),2:('lower',2),3:('garment',0),4:('garment',1),5:('upper',3),6:('middle',3),7:('lower',3),8:('garment',3),9:('upper',1)}
    catalogue=json.loads((ROOT/'npc/custom/fashion_points/stone_catalogue.json').read_text())
    seen=set()
    for box in catalogue['boxes']:
        location,index=locations[box['category']]
        for pair in box['pairs']:
            card=pair['enchant']
            if card in seen:continue
            seen.add(card)
            cases.append({'name':f'smoke-{card}','kind':'smoke',location:[0]*index+[card],'expected':{}})
    items={}
    for row in renewal_records(ROOT,'db/item_db.yml'):items.setdefault(row['Id'],{}).update(row)
    required={id for case in cases for key in ('upper','middle','lower','garment','armor') for id in case.get(key,[]) if id}
    # Native skill parsing resolves consumption materials even though this
    # fixture never casts a consumable skill. Load their real item records.
    costs={cost['Item'] for skill in renewal_records(ROOT,'db/skill_db.yml') for cost in skill.get('Requires',{}).get('ItemCost',[])}
    costs.update(name for skill in renewal_records(ROOT,'db/skill_db.yml') for name in skill.get('Requires',{}).get('Equipment',{}))
    required.update(id for id,row in items.items() if row.get('AegisName') in costs)
    selected={items[id]['AegisName'] for id in required}
    combos=[]
    for row in renewal_records(ROOT,'db/item_combos.yml'):
        retained=[entry for entry in row.get('Combos',[]) if set(entry['Combo'])<=selected]
        if retained:combos.append({**row,'Combos':retained})
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[items[id] for id in sorted(required)]},sort_keys=False))
    (build/'combos.yml').write_text(yaml.safe_dump({'Body':combos},sort_keys=False))
    (build/'skills.yml').write_text('Body: []\n')
    (build/'cases.json').write_text(json.dumps(cases,indent=2))

def run(build):
    build=build.resolve();build.mkdir(parents=True,exist_ok=True);prepare(build)
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    combined=build/'costume_combat_driver.cpp';combined.write_text(prefix+'\n'+(ROOT/'tools/ci/costume_combat_native_test.cpp').read_text())
    sources=[ROOT/'src/map'/f'{n}.cpp' for n in ('pc','script','itemdb','status','battle')]+[ROOT/'src/common/malloc.cpp',combined]
    san=['-fsanitize='+os.environ.get('COSTUME_COMBAT_SANITIZERS','undefined'),'-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san+['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    headers=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').rglob('*.hpp')))).hexdigest()
    def compile_one(source):
        target=build/(source.stem+'.o');sha=hashlib.sha256(source.read_bytes()+repr(flags).encode()+headers.encode()).hexdigest();stamp=target.with_suffix('.sha')
        if not target.exists() or not stamp.exists() or stamp.read_text()!=sha:
            print('Compile '+str(source),flush=True);subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True);stamp.write_text(sha)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool:fresh=list(pool.map(compile_one,sources))
    excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers=list(WRAPPERS)+['_Z9map_id2bli']
    exe=build/'costume_combat_native_test';subprocess.run(['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(exe),str(build)],cwd=ROOT,capture_output=True,text=True,timeout=120)
    (build/'native.log').write_text(result.stdout+result.stderr)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
    assert 'COSTUME_COMBAT_NATIVE_OK' in result.stdout
    assert not re.search(r'\[(?:Error|Warning)\]|runtime error:',result.stdout+result.stderr)

    # Deliberately corrupt only private fixture inputs. A rejected mutation is
    # harness sensitivity evidence, not evidence of a discovered gameplay bug.
    mutation_results=[]
    def mutation(name, change):
        folder=build/name;folder.mkdir(exist_ok=True)
        for filename in ('items.yml','combos.yml','skills.yml','cases.json'):
            shutil.copyfile(build/filename,folder/filename)
        change(folder)
        bad=subprocess.run([str(exe),str(folder)],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (folder/'native.log').write_text(bad.stdout+bad.stderr)
        assert bad.returncode!=0 and 'independent described bonus reaches native status' in bad.stderr,(name,bad.returncode,bad.stderr[-1000:])
        mutation_results.append({'name':name,'rejected':True,'exit_code':bad.returncode})
    def change_item(folder,id,old,new):
        path=folder/'items.yml';data=yaml.safe_load(path.read_text());row=next(r for r in data['Body'] if r['Id']==id)
        assert old in row['Script'];row['Script']=row['Script'].replace(old,new)
        path.write_text(yaml.safe_dump(data,sort_keys=False))
    mutation('mutant-anniversary-damage',lambda p:change_item(p,300713,'bMagicAtkEle,Ele_All,3','bMagicAtkEle,Ele_All,4'))
    mutation('mutant-skill-threshold',lambda p:change_item(p,315325,'/2','/3'))
    mutation('mutant-purified-category',lambda p:change_item(p,313741,'bMagicAddClass,Class_All','bMagicAddRace,RC_All'))
    def remove_range_combo(folder):
        path=folder/'combos.yml';data=yaml.safe_load(path.read_text())
        selected=next(r for r in data['Body'] if any(set(c['Combo'])=={'Range_Top','Range_Middle','Range_Bottom'} for c in r['Combos']))
        data['Body'].remove(selected);path.write_text(yaml.safe_dump(data,sort_keys=False))
    mutation('mutant-missing-range-combo',remove_range_combo)
    evidence={'independent_cases':sum(c['kind']=='independent' for c in json.loads((build/'cases.json').read_text())),
              'unique_output_smoke_cases':sum(c['kind']=='smoke' for c in json.loads((build/'cases.json').read_text())),
              'native_success':True,'mutations':mutation_results,
              'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources if p.is_relative_to(ROOT)},
              'database_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in (ROOT/'db/import/fashion_stone_expansion_items.yml',ROOT/'db/import/fashion_stone_expansion_combos.yml')},
              'oracle':'hand-authored official descriptions; not script-derived fixture JSON',
              'boundaries':'synthetic inventory/player and lookup, transport/registry/persistence wrappers; no server startup; magic and physical class/size cardfix stages, partial card removal/reinsert, and native cast/delay; not full skill attack simulation or native unequip packet handling'}
    (build/'evidence.json').write_text(json.dumps(evidence,indent=2)+'\n')
    print('COSTUME_COMBAT_MUTATIONS_OK rejected='+str(len(mutation_results)))

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build-dir',type=Path,required=True);args=parser.parse_args();run(args.build_dir)
