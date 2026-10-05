#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  rune_tablet_transaction_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/rune_tablet_transaction_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Run production Rune services in the real isolated Linux script VM.

Requires a built map-server object set. Native pc/script/itemdb are freshly
compiled with ASan/UBSan; explicit world/transport/bonus-refresh boundaries.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]

def run(build):
    build=build.resolve();build.mkdir(parents=True,exist_ok=True)
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    # Use actual loaded numeric/string/account registry and array bookkeeping.
    names=('setreg','readreg','registry','named_registry','setstr','readstr')
    for name in names:
        prefix=re.sub(r'^extern "C"[^\n]*\b'+name+r'\([^\n]*\n','',prefix,flags=re.M)
    combined=build/'combined_rune_test.cpp'
    combined.write_text(prefix+'\n'+(ROOT/'tools/ci/rune_tablet_transaction_test.cpp').read_text())
    catalog=json.loads((ROOT/'npc/custom/rune_tablet/catalog.json').read_text())
    # Exhaustively execute production grant families, using the authoritative
    # recipe JSON as an independent output oracle for the generated script.
    checks=[]
    def exact(outputs):
        expected={i:n for i,n in outputs}
        lines=[f'check(count({i})=={n},"catalog output amount");' for i,n in expected.items()]
        condition=' || '.join(f'it.nameid=={i}' for i in expected) or 'false'
        lines.append('for(const auto& it:sd->inventory.u.items_inventory) if(it.nameid) check('+condition+',"no undeclared output");')
        return ''.join(lines)
    for tablet in catalog['sets']:
        for tier,outputs in tablet['rewards'].items():
            if not outputs: continue
            suffix=tablet['id']-1260000
            setup=f'reg("PNRTPaid",{suffix},1);reg("#PNRTClaims",{suffix},{127^(1<<(int(tier)-1))});'
            setup+=''.join(f'reg("#PNRTPiece",{piece-1263000},1);' for piece in tablet['pieces'])
            checks.append('{auto sd=rune_player();'+setup+f'invoke("callfunc \\"PN_RT_Claim\\",{tablet["id"]};",{{1,1}});'+exact(outputs)+'}')
    for recipe in catalog['prints']:
        checks.append('{auto sd=rune_player();'+f'put(0,{recipe["id"]},1);put(1,1001282,10);weight();invoke("callfunc \\"PN_RT_Print\\";",{{1,1}});'+exact([(recipe['output'],1)])+'}')
    for index,recipe in enumerate(catalog['shop']):
        setup=''.join(f'put({n},{i},{amount});' for n,(i,amount) in enumerate(recipe['cost']))
        checks.append('{auto sd=rune_player();'+setup+f'weight();invoke("callfunc \\"PN_RT_Shop\\";",{{{index+1},1}});'+exact([(recipe['id'],1)])+'}')
    for material,batches in catalog['decomposition'].items():
        for batch,outputs in batches.items():
            lines=f'put(0,{material},{batch});weight();invoke("callfunc \\"PN_RT_Decompose\\";",{{1,{1 if batch=="1" else 2},1}});'
            for iid,lo,hi,chance in outputs:
                lines+=f'check((count({iid})==0 && {chance}<100000) || (count({iid})>={lo} && count({iid})<={hi}),"decomposition range and guaranteed output");'
            condition=' || '.join(f'it.nameid=={row[0]}' for row in outputs) or 'false'
            lines+='for(const auto& it:sd->inventory.u.items_inventory) if(it.nameid) check('+condition+',"no undeclared decomposition output");'
            checks.append('{auto sd=rune_player();'+lines+'}')
    # Separate generated scopes keep ASan compiler analysis bounded: thousands
    # of local unique_ptr lifetimes in one function otherwise compile very slowly.
    generated_cases='\n'.join('[]()'+case+'();' for case in checks)+'\n'
    (build/'rune_catalog_cases.inc').write_text(generated_cases)
    needed={4001,4700}
    def values(value):
        if isinstance(value,int) and value>=500: needed.add(value)
        elif isinstance(value,list):
            for v in value: values(v)
        elif isinstance(value,dict):
            for key,v in value.items():
                if str(key).isdigit() and int(key)>=500: needed.add(int(key))
                values(v)
    values(catalog)
    records={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row['Id'] in needed: records.setdefault(row['Id'],{}).update(row)
    # Preserve actual metadata, remove unrelated use/equip scripts from fixture
    # because transaction proof never uses/consumes a reward container.
    for row in records.values():
        for key in ('Script','EquipScript','UnEquipScript'): row.pop(key,None)
    (build/'items.yml').write_text(yaml.safe_dump({'Body':list(records.values())},sort_keys=False))
    production=[ROOT/'src/map'/f'{name}.cpp' for name in ('pc','script','itemdb','clif')]+[ROOT/'src/common/malloc.cpp']
    san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
    flags+=['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    headerhash=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').rglob('*.hpp')))).hexdigest()
    def compile_one(source):
        dependencies=generated_cases.encode() if source==combined else b''
        target=build/(source.stem+'.o');signature=hashlib.sha256(source.read_bytes()+dependencies+repr(flags).encode()+headerhash.encode()).hexdigest()
        receipt=target.with_suffix('.sha')
        if not target.exists() or not receipt.exists() or receipt.read_text()!=signature:
            print('Compile '+str(source),flush=True);subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True);receipt.write_text(signature)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool: fresh=list(pool.map(compile_one,production+[combined]))
    excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    assert objects and all(p.exists() for p in libs),'Build Linux map-server first'
    wrappers=[w for w in WRAPPERS if not any(x in w for x in ('pc_setreg','pc_readreg'))]+['_Z9map_id2bli']
    exe=build/'rune_tablet_transaction_test'
    subprocess.run(['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(exe),str(build)],cwd=ROOT,capture_output=True,text=True,timeout=120)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
    assert 'RUNE_NATIVE_OK' in result.stdout

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build-dir',type=Path,required=True)
    run(parser.parse_args().build_dir)
