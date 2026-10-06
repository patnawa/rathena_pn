#!/usr/bin/env python3
# GPL-3.0-or-later. See LICENSE.
"""Actual inventory/cart functions, stockall command and quest callbacks.

Client transport, actor/NPC lookup, logging/objective notifications are explicit
boundaries. Native quest conditions and inventory mutations run unchanged.
A mandatory syscall filter denies network access; no database or player data.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
from pathlib import Path
import re
import subprocess
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS
ROOT=Path(__file__).resolve().parents[2]

def run(build,case=None):
    build=build.resolve();build.mkdir(parents=True,exist_ok=True)
    prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    for name in ('setreg','readreg','registry','named_registry','setstr','readstr'):
        prefix=re.sub(r'^extern "C"[^\n]*\b'+name+r'\([^\n]*\n','',prefix,flags=re.M)
    prefix=prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");','')
    prefix=prefix.replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}',
        'npc_data* quest_npc=nullptr; extern "C" npc_data* npc_lookup(int32){return quest_npc;}')
    prefix=prefix.replace('extern "C" void quest(map_session_data*){}',
        'extern "C" void real_quest(map_session_data*) asm("__real__Z17pc_show_questinfoP16map_session_data"); extern "C" void quest(map_session_data* sd){real_quest(sd);}')
    prefix=prefix.replace('BIOSPHERE TEST FAIL:','CART TRANSFER TEST FAIL:')
    rune=(ROOT/'tools/ci/rune_tablet_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
    from storage_native_audit_test import function
    stockall=function((ROOT/'src/map/atcommand.cpp').read_text(),'ACMD_FUNC(stockall)').replace('ACMD_FUNC(stockall)','int32 audit_stockall(const int32 fd,map_session_data* sd,const char* command,const char* message)')
    combined=build/'driver.cpp';combined.write_text(prefix+'\n'+rune+'\n'+(ROOT/'tools/ci/cart_transfer_native_test.cpp').read_text()+'\n'+stockall)
    records={}
    for row in renewal_records(ROOT,'db/item_db.yml'):records.setdefault(row['Id'],{}).update(row)
    (build/'items.yml').write_text(yaml.safe_dump({'Body':[records[i] for i in (501,502,909,400999,4365)]},sort_keys=False))
    sources=[ROOT/'src/map'/f'{n}.cpp' for n in ('pc','script','itemdb','clif','storage','quest')]+[ROOT/'src/common/malloc.cpp',combined]
    san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san+['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
    headers=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').rglob('*.hpp')))).hexdigest()
    includes=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src/custom').glob('*.inc')))).hexdigest()
    def compile_one(source):
        target=build/(source.stem+'.o');sha=hashlib.sha256(source.read_bytes()+repr(flags).encode()+headers.encode()+includes.encode()).hexdigest();receipt=target.with_suffix('.sha')
        if not target.exists() or not receipt.exists() or receipt.read_text()!=sha:
            print(f'Fresh compile {source} sha256={hashlib.sha256(source.read_bytes()).hexdigest()}',flush=True)
            subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True);receipt.write_text(sha)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool:fresh=list(pool.map(compile_one,sources))
    excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    wrappers=[w for w in WRAPPERS if not any(x in w for x in ('pc_setreg','pc_readreg'))]+['_Z9map_id2bli','_Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args', '_Z27achievement_check_conditionP11script_codeP16map_session_data', '_Z11map_msg_txtPK16map_session_datai', '_Z25pc_can_give_bounded_itemsPK16map_session_data', '_Z17clif_cart_additemPK16map_session_dataii', '_Z17clif_cart_delitemRK16map_session_dataii', '_Z21clif_cart_additem_ackRK16map_session_data21e_ack_additem_to_cart', '_Z16clif_rental_timePK16map_session_dataji', '_Z19clif_rental_expiredPK16map_session_dataij']
    exe=build/'cart-transfer-test'
    subprocess.run(['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    result=subprocess.run([str(exe),str(build)]+([case] if case else []),cwd=ROOT,capture_output=True,text=True,timeout=60)
    print(result.stdout,end='');print(result.stderr,end='');result.check_returncode()
    assert 'CART_TRANSFER_NATIVE_OK' in result.stdout and 'No memory leaks found.' in result.stdout+result.stderr
    assert not re.search(r'\[(Error|Warning)\]|AddressSanitizer|runtime error:',result.stdout+result.stderr,re.I)

if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path,default=Path('/tmp/pn-cart-transfer-native-test'))
    p.add_argument('--case',choices=['callback','pending_deposit','pending_stockall','capacity','negative','oversized','bulk_callback'])
    a=p.parse_args();run(a.build_dir,a.case)
