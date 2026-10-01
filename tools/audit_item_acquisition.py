"""Trace typed source acquisition paths, including nested item groups.

This classifies QA priorities. Conditions, dynamic expressions and rendered
fallbacks still require runtime validation; a source path is not a playthrough.
"""
from collections import defaultdict, deque
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

WORK=Path(__file__).resolve().parent
REPO=WORK.parent
import yaml
hashes={}


def read(name):
    path=REPO/name
    raw=path.read_bytes();hashes[name]=hashlib.sha256(raw).hexdigest()
    return raw.decode('utf-8-sig',errors='replace')
def records(name,visiting=None):
    visiting=set() if visiting is None else visiting
    if not (REPO/name).exists():return
    if name in visiting:raise ValueError('Import cycle: '+name)
    visiting.add(name)
    doc=yaml.load(read(name),Loader=getattr(yaml,'CSafeLoader',yaml.SafeLoader)) or {}
    for row in doc.get('Body',[]) or []:yield name,row
    for entry in doc.get('Footer',{}).get('Imports',[]):
        if entry.get('Mode','Renewal')=='Renewal':yield from records(entry['Path'],visiting)
    visiting.remove(name)
def overlay(target,row):
    for key,value in row.items():
        if isinstance(value,dict) and isinstance(target.get(key),dict):overlay(target[key],value)
        else:target[key]=value
def strip_comments(text):
    return re.sub(r'"(?:\\.|[^"\\])*"|/\*.*?\*/|//[^\r\n]*',lambda m:m[0] if m[0].startswith('"') else re.sub(r'[^\r\n]',' ',m[0]),text,flags=re.S)


def grant_calls(text):
    """Locate real grant commands and their complete first argument.

    Quoted dialogue is skipped, and nested expressions stay unresolved rather
    than being mistaken for their first literal. This is lexical inventory,
    not evaluation of the scripting language.
    """
    clean=strip_comments(text)
    pattern=r'"(?:\\.|[^"\\])*"|\b(?:getitem(?:bound)?[234]?|rentitem[234]?|getgroupitem|getrandgroupitem)\b'
    end=0
    for match in re.finditer(pattern,clean):
        if match.start()<end or match[0].startswith('"'):continue
        start=match.end()
        while start<len(clean) and clean[start].isspace():start+=1
        # Both command(arg,...) and command (expression),... occur in scripts.
        wrapped=start<len(clean) and clean[start]=='('
        if wrapped:start+=1
        depth=0;quoted=False;escaped=False;pos=start
        while pos<len(clean):
            char=clean[pos]
            if quoted:
                if escaped:escaped=False
                elif char=='\\':escaped=True
                elif char=='"':quoted=False
            elif char=='"':quoted=True
            elif char in '([':depth+=1
            elif char in ')]':
                if depth==0:break
                depth-=1
            elif depth==0 and char in ',;':break
            pos+=1
        expression=clean[start:pos].strip()
        # A parenthesized first subexpression may be followed by operators.
        # Preserve it as a single unresolved expression through the comma.
        if wrapped and pos<len(clean) and clean[pos]==')':
            tail=pos+1
            while tail<len(clean) and clean[tail].isspace():tail+=1
            if tail<len(clean) and clean[tail] in '+-*/%&|^?:<>=!':
                while tail<len(clean) and clean[tail] not in ',;':tail+=1
                expression=clean[start-1:tail].strip();pos=tail
        end=pos
        yield {'command':match[0],'expression':expression,
               'line':clean.count('\n',0,match.start())+1}


def main():
    global REPO
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root',type=Path,default=REPO)
    parser.add_argument('--metadata',type=Path,required=True)
    parser.add_argument('--triage',type=Path,required=True)
    parser.add_argument('--held',type=Path,help='Optional prior snapshot; not a fresh database inventory')
    parser.add_argument('--report',type=Path,required=True)
    args=parser.parse_args()
    REPO=args.root
    hashes.clear()
    items={};item_sources={}
    for source,row in records('db/item_db.yml'):
        overlay(items.setdefault(row['Id'],{}),row);item_sources[row['Id']]=source
    names={row['AegisName']:item for item,row in items.items() if 'AegisName' in row}
    def item_id(token):
        token=token.strip()
        if not re.fullmatch(r'"[^"\\\r\n]+"|[A-Za-z_]\w*|\d+',token):return None
        token=token.strip('"')
        value=int(token) if token.isdigit() else names.get(token)
        return value if value in items else None
    graph=defaultdict(list);roots={};dynamic=[]
    def edge(start,end,kind,source,line=None):
        if end is not None:graph[start].append((end,{'kind':kind,'source':source,'line':line}))
    def root(node,kind,source,line=None):
        if node is not None:roots.setdefault(node,{'kind':kind,'source':source,'line':line})
    def inode(value):return None if value is None else 'item:'+str(value)
    def grants(text,start,source):
        for call in grant_calls(text):
            command,expression,line=call['command'],call['expression'],call['line']
            group=command in ('getgroupitem','getrandgroupitem')
            if group:
                literal=re.fullmatch(r'IG_([A-Za-z0-9_]+)',expression,re.I)
                node='group:'+literal[1].upper() if literal else None
            else:node=inode(item_id(expression))
            if node is None:dynamic.append({'source':source,**call,**({'owner':start} if start else {})})
            elif start is None:root(node,command if group else 'literal '+command,source,line)
            else:edge(start,node,command if group else 'item script '+command,source,line)
    active=set();configs=set();called_shops=set()
    def include(name):
        if name in configs:return
        configs.add(name)
        for kind,target in re.findall(r'^(npc|import):\s*([^\s]+)',strip_comments(read(name)),re.M):
            if kind=='import':include(target)
            elif (REPO/target).exists():active.add(target)
    include('npc/re/scripts_main.conf')
    for source in sorted(active):
        text=strip_comments(read(source));grants(text,None,source)
        called_shops.update(re.findall(r'\bcallshop\s*\(?\s*"([^"\r\n]+)"',text))
        for line_no,line in enumerate(text.splitlines(),1):
            if re.search(r'\t(?:shop|cashshop|itemshop|pointshop)\t',line):
                for match in re.finditer(r'(?:,|\t)(\d+):(-?\d+)',line):root(inode(item_id(match[1])),'shop listing',source,line_no)
            if '\tmonster\t' in line or '\tboss_monster\t' in line:
                tail=line.split('\t')[-1].split(',')
                if tail and tail[0].strip().isdigit():root('mob:'+tail[0].strip(),'map spawn',source,line_no)
            for match in re.finditer(r'\bmonster\s+(?:"[^"]*"|[^,]+),[^,]+,[^,]+,"[^"]*",\s*(\d+)\s*,',line):
                root('mob:'+match[1],'scripted literal spawn',source,line_no)
    for item,row in items.items():
        # Only use the consumable Script; equipment callbacks are not box opening.
        grants(row.get('Script','') or '',inode(item),item_sources[item])
    groups={}
    for source,row in records('db/item_group_db.yml'):
        group=groups.setdefault(str(row['Group']).upper(),{})
        for sub in row.get('SubGroups',[]) or []:
            if 'Clear' in sub:group.pop(int(sub['Clear']),None);continue
            key=sub.get('SubGroup')
            if key is None or 'List' not in sub:continue
            target=group.setdefault(key,{'algorithm':'SharedPool','entries':{}})
            target['algorithm']=sub.get('Algorithm',target['algorithm'])
            for entry in sub['List']:
                index=entry['Index']
                if 'Clear' in entry:
                    if entry['Clear']:target['entries'].pop(index,None)
                    continue
                # Current native parsing requires Item on an overlay entry too.
                if 'Item' not in entry:continue
                merged=target['entries'].setdefault(index,{})
                merged.update(entry);merged['source']=source
    for name,subs in groups.items():
        for subgroup in subs.values():
            for entry in subgroup['entries'].values():
                if subgroup['algorithm']=='All' or entry.get('Rate',0)>0:
                    edge('group:'+name,inode(item_id(entry['Item'])),'item group output',entry['source'])
    mobs={}
    for source,row in records('db/mob_db.yml'):
        drops=mobs.setdefault(row['Id'],{})
        for field in ('Drops','MvpDrops'):
            for drop in row.get(field,[]) or []:
                drops[(field,drop['Item'])]=(drop.get('Rate',0),source)
    for mob,drops in mobs.items():
        for (_,name),(rate,source) in drops.items():
            if rate>0:edge('mob:'+str(mob),inode(item_id(name)),'declared drop',source)
    # Include statically configured barter outputs and achievement rewards as
    # candidate routes. Conditions/prerequisites still require runtime proof.
    for source,row in records('npc/barters.yml' if (REPO/'npc/barters.yml').exists() else 'npc/custom/barters.yml'):
        if not row.get('Map') and row.get('Name') not in called_shops:continue
        for entry in row.get('Items',[]) or []:
            root(inode(item_id(str(entry.get('Item','')))),'configured barter output',source)
    for source,row in records('db/achievement_db.yml'):
        rewards=row.get('Rewards',{}) or {}
        root(inode(item_id(str(rewards.get('Item','')))),'configured achievement reward',source)
        grants(rewards.get('Script','') or '',None,source)
    held=json.loads(args.held.read_text())['item_ids'] if args.held else []
    for item in held:root(inode(item_id(str(item))),'previously observed held item','supplied held-item snapshot')
    for item in (6024,12781):root(inode(item_id(str(item))),'bank exchange output','src/custom/bank_protocol.hpp')
    # Only current, source-derived catalogs can close a reviewed dynamic line.
    # Other dynamic services remain explicitly unresolved below.
    catalog_path=REPO/'npc/custom/dynamic_reward_catalog.json'
    resolved_dynamic=[]
    if catalog_path.exists():
        from dynamic_reward_catalog import validate as validate_catalog
        catalog=json.loads(catalog_path.read_text())
        validate_catalog(catalog,REPO)
        hashes['npc/custom/dynamic_reward_catalog.json']=hashlib.sha256(catalog_path.read_bytes()).hexdigest()
        covered=set()
        for recipe in catalog['recipes']:
            if recipe['source'] not in active:continue
            covered.add((recipe['source'],recipe['line']))
            for output in recipe['outputs']:
                root(inode(output['item_id']),'catalog '+recipe['family'],recipe['source'],recipe['line'])
        resolved_dynamic=[row for row in dynamic if (row['source'],row['line']) in covered]
        dynamic=[row for row in dynamic if (row['source'],row['line']) not in covered]
    previous={node:(None,detail) for node,detail in roots.items()};queue=deque(previous)
    while queue:
        parent=queue.popleft()
        for child,detail in graph[parent]:
            if child not in previous:previous[child]=(parent,detail);queue.append(child)
    def trace(node):
        path=[]
        while node in previous:
            parent,detail=previous[node];path.append({'node':node,**detail})
            if parent is None:break
            node=parent
        return list(reversed(path))
    triage_path=args.triage
    triage=json.loads(triage_path.read_text());missing=defaultdict(list)
    refs=triage.get('catalog_missing_references',triage.get('lua',{}).get('missing_archive_icons',[]))
    for row in refs:missing[row['item_id']].append(row)
    rows=[{'item_id':item,'name':items.get(item,{}).get('AegisName'),'in_server_database':item in items,'source_path_found':inode(item) in previous,
           'path':trace(inode(item)),'missing':refs} for item,refs in sorted(missing.items())]
    rows.sort(key=lambda row:(not row['source_path_found'],len(row['path']),row['item_id']))
    metadata_path=args.metadata
    metadata_raw=metadata_path.read_bytes()
    metadata_text=metadata_raw.decode('utf-16') if metadata_raw.startswith(b'\xff\xfe') else metadata_raw.decode('utf-8-sig')
    metadata_ids={int(line.split('\t')[0]) for line in metadata_text.splitlines() if line.split('\t')[0].isdigit()}
    no_metadata=[{'item_id':int(node[5:]),'name':items[int(node[5:])].get('AegisName'),'path':trace(node)}
                 for node in previous if node.startswith('item:') and int(node[5:]) not in metadata_ids]
    dynamic_lines={(r['source'],r.get('owner'),r['line']) for r in dynamic}
    report={'summary':{'active_npc_files':len(active),'item_definitions':len(items),'item_groups':len(groups),
        'items_with_source_paths':sum(node.startswith('item:') for node in previous),
        'unresolved_catalog_item_ids':len(rows),'unresolved_ids_with_source_paths':sum(row['source_path_found'] for row in rows),
        'dynamic_grant_lines_not_resolved':len(dynamic_lines),'dynamic_grant_calls_not_resolved':len(dynamic),
        'dynamic_grant_lines_cataloged':len({(r['source'],r['line']) for r in resolved_dynamic}),'items_with_source_paths_without_client_metadata':len(no_metadata)},
        'acquired_items':[{'item_id':int(node[5:]),'name':items[int(node[5:])].get('AegisName'),'path':trace(node)} for node in sorted(previous) if node.startswith('item:')],
        'items':rows,'missing_metadata':no_metadata,'dynamic_grants':dynamic,'cataloged_dynamic_grants':resolved_dynamic,
        'source_sha256':hashes,'metadata_sha256':hashlib.sha256(metadata_raw).hexdigest(),'triage_sha256':hashlib.sha256(triage_path.read_bytes()).hexdigest(),
        'boundary':'Typed source paths, configured barter and achievement outputs prioritize runtime QA; they are not gameplay proof. Conditions, dynamic grants/spawns, prerequisite reachability, loose files and runtime artwork fallbacks remain unverified. Held-item roots, when supplied, are an older snapshot.'}
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['summary'],indent=2))
    for row in rows:
        if row['source_path_found']:print(row['item_id'],row['name'],' -> '.join(x['node'] for x in row['path']))


if __name__=='__main__':main()
