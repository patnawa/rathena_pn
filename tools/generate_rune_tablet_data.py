#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  generate_rune_tablet_data.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/generate_rune_tablet_data.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Build deterministic Rune Tablet NPC getters from a factual JSON catalog.

Import mode reads published HTML and non-executing client data. Normal mode
needs only the committed catalog; --check rejects generated-file drift.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'tools/ci'))
from audit_enchant_upgrades import renewal_records
from rune_tablet_catalog import read_client, read_reference

SET_ORDER = [
    [8,9,10,11,12,0,1,2,3,4,5,6,7],
    list(range(14,23)), list(range(23,33)),
    [39,40,41,42,43,37,38,44],
    [53,54,55,56,57,48,49,50,51,52], [47,45,46], [35,36,13,34],
]

def amounts(text):
    """Every amount must precede a named item ending in its numeric ID."""
    result = []
    pos = 0
    for m in re.finditer(r'\((\d+)\)', text):
        label = text[pos:m.start()].strip(' ,')
        a = re.match(r'(\d+)T?\s*x?\s+', label)
        if not a:
            raise ValueError(f'Missing amount: {label!r} in {text!r}')
        result.append([int(m[1]), int(a[1])])
        pos = m.end()
    if text[pos:].strip() or not result:
        raise ValueError(f'Unparsed cost: {text}')
    return sorted(result)

def pairs(table):
    return sorted([[v['1'],v['2']] for v in table.values()])

def item_db():
    result = {}
    for r in renewal_records(ROOT, 'db/item_db.yml'):
        result.setdefault(r['Id'], {}).update(r)
    return result

def build(client_path, reference):
    # JSON roundtrip makes the non-executing Lua parser's integer keys uniform.
    c = json.loads(json.dumps(read_client(client_path)))
    t = read_reference(reference)
    costs = c['RuneTable_itemList']
    native_sets = {int(i):(int(tag),s) for tag,rows in c['RuneSettbl_info'].items() for i,s in rows.items()}
    native_pieces = {int(i):s for rows in c['Runetbl_info'].values() for i,s in rows.items()}
    piece_rows = []
    for table in t[7:14]:
        for name, cost in table[2:]:
            price = amounts(cost)
            matches = [i for i,p in native_pieces.items() if pairs(costs[str(p['Rune_ActiveList'])]) == price]
            if len(matches) != 1:
                raise ValueError(f'Piece match {name}: {matches}')
            piece_rows.append(dict(id=matches[0],name=name,cost=price))
    pieces = {p['id']:p for p in piece_rows}
    sets = []
    for table, ids in zip(t[:7],SET_ORDER):
        assert len(table[2:]) == len(ids)
        for row, suffix in zip(table[2:], ids):
            sid = 1260000 + suffix
            tag,s = native_sets[sid]
            cost = amounts(row[2])
            assert cost == pairs(costs[str(s['RuneSetActiveList'])]), (sid,row[0])
            pids = [i for i in s['RuneSet_SlotList'].values() if i]
            assert all(i in pieces for i in pids), (sid,pids)
            grade = str(s['RuneSet_UpGrade_Percentage_table'])
            fail = str(s['RuneSet_UpGrade_Percentage_table_Fail'])
            base = list(c['GradeTable'][grade].values())
            pity = list(c['GradeTable_Fail'][fail].values())
            upgrades = [pairs(costs[str(x)]) for x in s['RuneSet_UpGradeList'].values()]
            rewards = {int(k):[[v,1]] for k,v in c['RuneSettbl_Reward'][str(tag)].get(str(sid),{}).items() if v}
            sets.append(dict(id=sid,key=s['RuneSetRes'],name=row[0].replace('Chaso','Chaos').replace('Set Set','Set'),tag=tag,pieces=pids,activation=cost,description=row[3],base=base,pity=pity,max_level=15 if any(base) else 0,upgrade=upgrades,rewards=rewards,reward_source='native_fallback' if rewards else 'none'))
    # Published reward rows with rowspan omit tablet cells on continuation rows.
    # Match by native reward box IDs to avoid translation/name ambiguities.
    corrections = []
    published_reward_sets=set()
    for table in t[16:22]:
        current = None
        for row in table[2:]:
            if len(row)==3:
                current_name, label, text = row
                ids = [int(x) for x in re.findall(r'\((\d+)\)',text)]
                if ids == [1003349]:
                    ids=[103349];text=text.replace('1003349','103349')
                    corrections.append('Secret Facility activation reward wiki typo 1003349 corrected to native 103349.')
                matches = [s for s in sets if s['rewards'].get(1)==[[ids[0],1]]]
                if len(matches)!=1:raise ValueError(('Reward tablet',row,matches))
                current=matches[0]
                if current['id'] in published_reward_sets:raise ValueError(('Duplicate reward tablet',row))
                published_reward_sets.add(current['id'])
                # Wiki rows bundle earlier native milestones into later claims.
                # They REPLACE the native list; merging would duplicate boxes
                # and currency (notably Secret Facility and Chaos).
                current['rewards']={}
                current['reward_source']='published'
            else:
                label,text=row
            tier = 1 if label=='Activation' else 7 if label=='Completion' else int(label.split()[0])
            rewards=[]
            for segment in text.split(','):
                iid=re.search(r'\((\d+)\)',segment)
                if not iid:raise ValueError(('Reward item',segment))
                qty=re.match(r'\s*(\d+)\s*x\s*',segment)
                rewards.append([int(iid[1]),int(qty[1]) if qty else 1])
            current['rewards'][tier]=rewards
    assert len(published_reward_sets)==52
    corrections.append('Published reward milestones replace, rather than merge with, native rewards for all 52 listed sets; bundled earlier rewards are not paid twice.')
    corrections.append('Greenhouse Administration has no published reward rows; its native client rewards are retained. Four event sets have no defined rewards.')
    # Wiki cost group numbering has one more group than chance numbering.
    # Identify each published 15-level cost matrix by its first native price,
    # then require exact full matrix match (except documented client deltas).
    published=[]
    for table in t[36:51]:
        matrix=[[] for _ in range(15)]
        for row in table[2:]:
            iid=int(re.search(r'\((\d+)\)',row[0])[1])
            values=row[1:]
            if len(values)==1:values*=15  # colspan, Chaos flat price
            assert len(values)==15
            for lv,amount in enumerate(values):
                if amount not in ('-','0',''):
                    matrix[lv].append([iid,int(amount)])
        published.append([sorted(x) for x in matrix])
    for s in sets:
        same=[m for m in published if m==s['upgrade']]
        if not same:
            scores=[sum(a==b for a,b in zip(m,s['upgrade'])) for m in published]
            index=max(range(len(scores)),key=scores.__getitem__)
            if scores[index]<10:raise ValueError(('Upgrade matrix mismatch',s['name'],scores))
            corrections.append(f"{s['name']}: published upgrade cost group {index+1} overrides differing client levels.")
            s['upgrade']=published[index]
    decomp={}
    # Native type tables 11..14 are absent; these four published rows are explicit.
    types={int(k):[[v['1'],v['2'],v['3'],v['4']] for v in vs.values()] for k,vs in c['itemDecomType_tbl'].items()}
    types.update({11:[[1001283,1,1,100000]],12:[[1001283,30,30,100000]],13:[[1001282,6,10,100000],[1001283,1,1,25000]],14:[[1001282,200,330,100000],[1001283,7,30,100000]]})
    for iid,modes in c['itemDecom_tbl'].items():
        decomp[int(iid)]={int(c['itemDecomItemNum_tbl'][mode]):types[group] for mode,group in modes.items()}
    prints=[]
    for row in t[51][2:]:
        base,out=[int(re.findall(r'\((\d+)\)',x)[-1]) for x in row]
        prints.append(dict(id=base,output=out,cost=[[base,1],[1001282,10]]))
    shop=[dict(id=int(re.search(r'\((\d+)\)',r[0])[1]),cost=amounts(r[1])) for r in t[52][1:]]
    db=item_db()
    published_decomp={int(i) for row in t[15][1:] for i in re.findall(r'\((\d+)\)',row[2])}
    excluded=[]
    for iid in list(decomp):
        if iid not in db:
            if iid in published_decomp:raise ValueError(f'Published decomposition item missing: {iid}')
            excluded.append(iid);del decomp[iid]
    corrections.append(f'Unsupported native-only decomposition IDs excluded (absent from effective DB and published whitelist): {sorted(excluded)}.')
    seals=[i for i,r in db.items() if r.get('Type')=='Card' and r.get('AegisName','').startswith('Sealed_')]
    return dict(schema=1,provenance=dict(reference_url='',reference_sha256=hashlib.sha256(Path(reference).read_bytes()).hexdigest(),client_sha256={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(Path(client_path).glob('*.lua'))},notes=corrections+['Native integer odds use denominator 100000; retained when published percentages round or are blank.','Rune and tablet IDs identify persistent registrations; they are not physical items.','Unquantified published reward entries mean one item. Rewards are claimed once per RO login account; PN has no cross-account master-account mapping.','Eight additional Chapter 1 sets present only in client tables are outside published scope.']),sets=sets,pieces=sorted(piece_rows,key=lambda p:p['id']),prints=prints,shop=shop,decomposition=decomp,seals=sorted(seals))

def quote(s):return json.dumps(s,ensure_ascii=True)
def array(name, values):
    if not values:return ''
    return '\tsetarray '+name+'[0],'+','.join(quote(v) if isinstance(v,str) else str(v) for v in values)+';\n'
def function(name, body):return 'function\tscript\t'+name+'\t{\n'+body+'}\n\n'
def clear(*names):return ''.join('\tdeletearray '+n+'[0],getarraysize('+n+');\n' for n in names)
def output_pairs(prefix, pairs_):
    return array('@RT_'+prefix+'Ids',[p[0] for p in pairs_])+array('@RT_'+prefix+'Amounts',[p[1] for p in pairs_])+f'\t@RT_{prefix}Count = {len(pairs_)};\n'

def generate(d):
    out='// Generated by tools/generate_rune_tablet_data.py; edit catalog.json then regenerate.\n// All probabilities use denominator 100000. Getters clear their output arrays.\n\n'
    sets=d['sets']
    out+=function('PN_RT_ListSets',clear('@RT_SetIds','@RT_SetNames$','@RT_SetTags')+array('@RT_SetIds',[s['id'] for s in sets])+array('@RT_SetNames$',[s['name'] for s in sets])+array('@RT_SetTags',[s['tag'] for s in sets])+f'\treturn {len(sets)};\n')
    body=clear('@RT_PieceIds','@RT_Base','@RT_PityStep')+'\t@RT_Name$ = ""; @RT_Description$ = ""; @RT_PieceCount = 0; @RT_MaxLevel = 0;\n\tswitch (getarg(0)) {\n'
    for s in sets:
        body+=f"\tcase {s['id']}:\n\t@RT_Name$ = {quote(s['name'])};\n\t@RT_Description$ = {quote(s['description'])};\n"
        body+=array('@RT_PieceIds',s['pieces'])+array('@RT_Base',s['base'])+array('@RT_PityStep',s['pity'])+f"\t@RT_PieceCount = {len(s['pieces'])}; @RT_MaxLevel = {s['max_level']}; return 1;\n"
    out+=function('PN_RT_LoadSet',body+'\t}\n\treturn 0;\n')
    out+=function('PN_RT_LoadPiece','\t@RT_PieceName$ = "";\n\tswitch (getarg(0)) {\n'+''.join(f"\tcase {p['id']}: @RT_PieceName$ = {quote(p['name'])}; return 1;\n" for p in d['pieces'])+'\t}\n\treturn 0;\n')
    body=clear('@RT_CostIds','@RT_CostAmounts')+'\t@RT_CostCount = 0;\n'
    for kind,records in [('piece',[(p['id'],p['cost']) for p in d['pieces']]),('activate',[(s['id'],s['activation']) for s in sets]),('print',[(p['id'],p['cost']) for p in d['prints']]),('shop',[(p['id'],p['cost']) for p in d['shop']])]:
        body+=f'\tif (getarg(0) == "{kind}") {{\n\tswitch (getarg(1)) {{\n'
        for iid,cost in records:body+=f'\tcase {iid}:\n'+output_pairs('Cost',cost)+'\treturn 1;\n'
        body+='\t}\n\treturn 0;\n\t}\n'
    body+='\tif (getarg(0) == "upgrade") {\n\tswitch (getarg(1)) {\n'
    for s in sets:
        body+=f"\tcase {s['id']}:\n\tswitch (getarg(2,0)) {{\n"
        for lv,cost in enumerate(s['upgrade'],1):body+=f'\tcase {lv}:\n'+output_pairs('Cost',cost)+'\treturn 1;\n'
        body+='\t}\n\treturn 0;\n'
    out+=function('PN_RT_Cost',body+'\t}\n\t}\n\treturn 0;\n')
    body=clear('@RT_RewardIds','@RT_RewardAmounts')+'\t@RT_RewardCount = 0;\n\tswitch (getarg(0)) {\n'
    for s in sets:
        body+=f"\tcase {s['id']}:\n\tswitch (getarg(1)) {{\n"
        for tier in range(1,8):body+=f'\tcase {tier}:\n'+output_pairs('Reward',s['rewards'].get(str(tier),s['rewards'].get(tier,[])))+'\treturn 1;\n'
        body+='\t}\n\treturn 0;\n'
    out+=function('PN_RT_LoadReward',body+'\t}\n\treturn 0;\n')
    out+=function('PN_RT_ListPrints',clear('@RT_PrintIds','@RT_PrintOutputs')+array('@RT_PrintIds',[p['id'] for p in d['prints']])+array('@RT_PrintOutputs',[p['output'] for p in d['prints']])+f"\treturn {len(d['prints'])};\n")
    out+=function('PN_RT_ListShop',clear('@RT_ShopIds')+array('@RT_ShopIds',[p['id'] for p in d['shop']])+f"\treturn {len(d['shop'])};\n")
    out+=function('PN_RT_ListSeals',clear('@RT_SealIds','@RT_SealOutputs')+array('@RT_SealIds',d['seals'])+array('@RT_SealOutputs',[1001594]*len(d['seals']))+f"\treturn {len(d['seals'])};\n")
    # Group identical decomposition recipes to keep generated script compact.
    groups={}
    for iid,recipe in d['decomposition'].items():groups.setdefault(json.dumps(recipe,sort_keys=True),[]).append(int(iid))
    body=clear('@RT_DecompIds','@RT_DecompMin','@RT_DecompMax','@RT_DecompChance')+'\t@RT_DecompCount = 0;\n\tswitch (getarg(0)) {\n'
    for recipe,ids in groups.items():
        body+=''.join(f'\tcase {iid}:\n' for iid in sorted(ids))+'\tswitch (getarg(1)) {\n'
        for batch,rewards in json.loads(recipe).items():
            body+=f'\tcase {batch}:\n'
            for j,suffix in enumerate(['Ids','Min','Max','Chance']):body+=array('@RT_Decomp'+suffix,[r[j] for r in rewards])
            body+=f'\t@RT_DecompCount = {len(rewards)}; return 1;\n'
        body+='\t}\n\treturn 0;\n'
    out+=function('PN_RT_LoadDecomp',body+'\t}\n\treturn 0;\n')
    return out.rstrip()+'\n'

def validate(d):
    assert len(d['sets'])==57
    assert len({p['id'] for p in d['pieces']})==len(d['pieces'])
    needed=set()
    for s in d['sets']:
        assert len(s['base'])==len(s['pity'])==len(s['upgrade'])==15
        assert all(0<=p<=100000 for p in s['base']+s['pity'])
        for cost in [s['activation']]+s['upgrade']+list(s['rewards'].values()):
            for iid,qty in cost:assert qty>0;needed.add(iid)
    for p in d['pieces']+d['prints']+d['shop']:
        for iid,qty in p['cost']:assert qty>0;needed.add(iid)
    needed.update(p['output'] for p in d['prints']);needed.update(p['id'] for p in d['shop']);needed.update(d['seals'])
    for iid,modes in d['decomposition'].items():
        needed.add(int(iid))
        for rewards in modes.values():
            for reward,low,high,chance in rewards:
                assert 0<low<=high and 0<chance<=100000
                needed.add(reward)
    missing=sorted(needed-item_db().keys())
    if missing:raise ValueError(f'Missing effective Renewal item IDs: {missing}')
    return len(needed)

def main():
    p=argparse.ArgumentParser();p.add_argument('--client',type=Path);p.add_argument('--reference',type=Path);p.add_argument('--check',action='store_true');p.add_argument('--allow-missing',action='store_true',help='Import diagnostics only; does not waive deployment validation')
    a=p.parse_args();folder=ROOT/'npc/custom/rune_tablet';folder.mkdir(exist_ok=True)
    if a.client:
        assert a.reference
        d=build(a.client,a.reference)
        (folder/'catalog.json').write_text(json.dumps(d,indent=2,ensure_ascii=True)+'\n',encoding='utf-8')
    else:d=json.loads((folder/'catalog.json').read_text())
    try:n=validate(d)
    except ValueError as e:
        if not a.allow_missing:raise
        print(e);n=0
    generated=generate(d);path=folder/'data.txt'
    if a.check:
        assert path.read_text()==generated,'Generated Rune Tablet data drift'
    else:path.write_text(generated,encoding='utf-8')
    print(f"Rune Tablet: {len(d['sets'])} sets, {len(d['pieces'])} pieces, {len(d['prints'])} imprints, {n} item dependencies validated")

if __name__=='__main__':main()
