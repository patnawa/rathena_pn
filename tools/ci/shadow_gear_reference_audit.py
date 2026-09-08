#!/usr/bin/env python3
"""Read-only Shadow Gear dependency/recipe coverage audit against saved HTML.

This inventories declarations, not gameplay reachability or script equivalence.
No external descriptions/assets are copied into the output.
"""
import argparse
from collections import defaultdict
import hashlib
import json
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'tools'))
from rune_tablet_catalog import Tree
from generate_rune_tablet_data import item_db
from audit_enchant_upgrades import renewal_records

def active_scripts():
    seen=set();scripts=[]
    def visit(rel):
        if rel in seen:return
        seen.add(rel)
        p=ROOT/rel
        if not p.exists():return
        for line in p.read_text(encoding='utf-8',errors='replace').splitlines():
            m=re.match(r'^\s*(import|npc):\s*(\S+)',line)
            if not m:continue
            if m[1]=='import':visit(m[2])
            else:scripts.append(m[2])
    visit('npc/re/scripts_main.conf')
    return sorted(set(scripts))

def records(relative,key):
    result={}
    for record in renewal_records(ROOT,relative):
        result.setdefault(record[key],{}).update(record)
    return result

def walk_item_refs(value):
    if isinstance(value,dict):
        if isinstance(value.get('Item'),str):yield value['Item']
        for child in value.values():yield from walk_item_refs(child)
    elif isinstance(value,list):
        for child in value:yield from walk_item_refs(child)

def run(reference):
    tree=Tree();raw=reference.read_bytes();tree.feed(raw.decode('utf-8'))
    ids=sorted({int(x) for x in re.findall(r'\((\d+)\)',tree.root.text())})
    names={}
    for a in tree.root.find('a'):
        if 'module=item' in a.attrs.get('href',''):
            m=re.search(r'[?&]id=(\d+)',a.attrs['href'])
            if m:names[int(m[1])]=a.text().strip()
    db=item_db();byname={r['AegisName']:i for i,r in db.items() if 'AegisName' in r}
    gear={i for i in ids if db.get(i,{}).get('Type','').lower()=='shadowgear'}
    upgrades=records('db/laphine_upgrade.yml','Item')
    synthesis=records('db/laphine_synthesis.yml','Item')
    randoms=records('db/item_randomopt_group.yml','Group')
    groups=records('db/item_group_db.yml','Group')
    group_sources=defaultdict(list)
    for key,group in groups.items():
        for name in set(walk_item_refs(group)):group_sources[name].append(key)
    scripts=active_scripts()
    tokens=defaultdict(list)
    for path in scripts:
        p=ROOT/path
        if not p.exists():continue
        text=p.read_text(encoding='utf-8',errors='replace')
        # Evidence only: tokens can be menus, requirements, comments or grants.
        for tok in set(re.findall(r'\b[A-Za-z_][A-Za-z_0-9]*\b|\b\d+\b',text)):
            tokens[tok].append(path)
    containers=defaultdict(list)
    for iid,r in db.items():
        for g in re.findall(r'getgroupitem\s*\(?\s*IG_([A-Za-z_0-9]+)',r.get('Script','')):
            containers[g].append(iid)
    mob_sources=defaultdict(list)
    for iid,m in records('db/mob_db.yml','Id').items():
        for drop in m.get('Drops',[])+m.get('MvpDrops',[]):mob_sources[drop['Item']].append(iid)
    usage=[]
    for iid in ids:
        r=db.get(iid,{})
        script=r.get('Script','')
        name=r.get('AegisName')
        if 'laphine_upgrade' in script:
            recipe=upgrades.get(name)
            target_ids=[] if recipe is None else [byname.get(t['Item']) for t in recipe.get('TargetItems',[])]
            usage.append(dict(id=iid,name=r.get('Name'),aegis=name,system='upgrade',recipe_present=recipe is not None,target_count=len(target_ids),target_ids=target_ids,random_group=None if recipe is None else recipe.get('RandomOptionGroup'),random_group_present=None if not recipe or not recipe.get('RandomOptionGroup') else recipe['RandomOptionGroup'] in randoms,result_refine=None if recipe is None else {k:v for k,v in recipe.items() if 'Refine' in k}))
        if 'laphine_synthesis' in script:
            recipe=synthesis.get(name)
            usage.append(dict(id=iid,name=r.get('Name'),aegis=name,system='synthesis',recipe_present=recipe is not None,recipe=recipe))
    evidence=[]
    for iid in ids:
        r=db.get(iid,{})
        if not r:continue
        name=r['AegisName'];gs=group_sources.get(name,[])
        evidence.append(dict(id=iid,aegis=name,type=r.get('Type'),script_present=bool(r.get('Script')),group_membership=gs,group_container_ids=sorted({j for g in gs for j in containers.get(g,[])}),mob_drop_ids=mob_sources.get(name,[]),active_npc_token_paths=sorted(set(tokens.get(name,[])+tokens.get(str(iid),[])))))
    hammers=[]
    for iid in [23436,23926,23720]:
        r=upgrades.get(db[iid]['AegisName'],{})
        ts={byname.get(x['Item']) for x in r.get('TargetItems',[])}
        hammers.append(dict(id=iid,target_count=len(ts),reference_gear_covered=len(gear & ts),reference_gear_not_targeted=sorted(gear-ts),refine_fields={k:v for k,v in r.items() if 'Refine' in k},random_group=r.get('RandomOptionGroup')))
    enchant_groups=[]
    shadow_names={db[i]['AegisName'] for i,r in db.items() if r.get('Type','').lower()=='shadowgear'}
    for group,record in records('db/item_enchant.yml','Id').items():
        if set(record.get('TargetItems',{})) & shadow_names:
            enchant_groups.append(dict(id=group,target_ids=[byname[n] for n in record['TargetItems']],order=record.get('Order')))
    source_paths=['db/item_db.yml','db/laphine_upgrade.yml','db/re/laphine_upgrade.yml','db/laphine_synthesis.yml','db/re/laphine_synthesis.yml','db/re/item_randomopt_group.yml','db/re/item_group_db.yml','db/import/shadow_group128_enchant.yml','npc/custom/grademk_services.txt','npc/re/scripts_main.conf','npc/scripts_custom.conf','npc/re/merchants/shadow_refiner.txt']
    return dict(schema=1,audited_at='2026-09-08',reference_url='https://wiki.muhro.eu/Shadow_Gear',reference_sha256=hashlib.sha256(raw).hexdigest(),reference_revision=62112,counts=dict(reference_ids=len(ids),reference_gear_present=len(gear),missing_ids=len(set(ids)-db.keys()),active_npc_files=len(scripts)),limitations=['Text token matches are not proof of an acquisition grant or reachable NPC.','Group membership and container IDs are evidence of a definition, not proof that its parent container is obtainable.','Item presence does not certify every skill/bonus/combo effect.','Native recipe availability does not certify client recipe support.'],missing=[dict(id=i,name=names.get(i)) for i in ids if i not in db],item_use_recipes=usage,hammer_and_standard_option_coverage=hammers,native_enchant_groups=enchant_groups,standard_random_options=randoms['SHADOW_RANDOM_MIX'],item_source_evidence=evidence,local_source_sha256={p:hashlib.sha256((ROOT/p).read_bytes()).hexdigest() for p in source_paths if (ROOT/p).exists()})

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('reference',type=Path);p.add_argument('--output',type=Path,default=ROOT/'doc/shadow_gear_reference_coverage_20260908.json');a=p.parse_args();d=run(a.reference);a.output.write_text(json.dumps(d,indent=2)+'\n');print(d['counts']);print('Missing use recipes:',[(r['id'],r['aegis']) for r in d['item_use_recipes'] if not r['recipe_present']]);print('Hammer/option coverage:',[(r['id'],r['reference_gear_covered']) for r in d['hammer_and_standard_option_coverage']])
