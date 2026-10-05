#!/usr/bin/env python3
"""Actual preparation VM/registry with explicit world and durable-submit boundaries.
Requires Linux objects and tools; never connects to a game or SQL server.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
from pathlib import Path
import re
import subprocess
from biosphere_crown_transaction_test import WRAPPERS
ROOT=Path(__file__).resolve().parents[2]
EXTRA=(
 '_Z9map_id2bli','_Z17chrif_isconnectedv','_Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args',
 '_Z17pc_can_give_itemsPK16map_session_data','_Z18pc_get_group_levelPK16map_session_data',
 '_Z22storage_page_availableR16map_session_datai','_Z19storage_batch_beginR16map_session_data',
 '_Z17storage_batch_endR16map_session_dataR9s_storage','_Z18storage_storageaddP16map_session_dataP9s_storageii',
 '_Z13pn_shop_beginR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEERKSt6vectorINS2_5GrantESaIS6_EEPKj',
 '_Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj')
def run(build):
 build=build.resolve();build.mkdir(parents=True,exist_ok=True)
 prefix=(ROOT/'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 for name in ('setreg','readreg','registry','named_registry','setstr','readstr'):
  prefix=re.sub(r'^extern "C"[^\n]*\b'+name+r'\([^\n]*\n','',prefix,flags=re.M)
 prefix=prefix.replace('check(flags==3,"native mutation forces expected unequip flags");','check(flags==1,"preparation respects native unequip restrictions");')
 prefix=prefix.replace('++equips;if(fail_equip)return false;','++equips;if(fail_equip){fail_equip=false;return false;}')
 prefix=prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");','')
 rune=(ROOT/'tools/ci/rune_tablet_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(',1)[0]
 combined=build/'combined_preparation_test.cpp';combined.write_text(prefix+'\n'+rune+'\n'+(ROOT/'tools/ci/preparation_native_test.cpp').read_text())
 sources=[ROOT/'src/map'/f'{n}.cpp' for n in ('pc','script','itemdb','clif')]+[ROOT/'src/common/malloc.cpp',combined]
 san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
 flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san+['-I'+str(ROOT/p) for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
 headers=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src').rglob('*.hpp')))).hexdigest()
 # Included implementation files are a real compiler input too.
 includes=hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT/'src/custom').glob('*.inc')))).hexdigest()
 def compile_one(source):
  target=build/(source.stem+'.o');sha=hashlib.sha256(source.read_bytes()+repr(flags).encode()+headers.encode()+includes.encode()).hexdigest();receipt=target.with_suffix('.sha')
  if not target.exists() or not receipt.exists() or receipt.read_text()!=sha:
   print('Compile '+str(source),flush=True);subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True);receipt.write_text(sha)
  return target
 with ThreadPoolExecutor(max_workers=2) as pool:fresh=list(pool.map(compile_one,sources))
 excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
 libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
 wrappers=[w for w in WRAPPERS if not any(x in w for x in ('pc_setreg','pc_readreg'))]+list(EXTRA)
 zstd='-lzstd' if subprocess.check_output(['g++','-print-file-name=libzstd.so'],text=True).strip()!='libzstd.so' else '-l:libzstd.so.1'
 exe=build/'preparation_test';subprocess.run(['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient',zstd,'-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
 result=subprocess.run([str(exe),str(build)],cwd=ROOT,capture_output=True,text=True,timeout=120);print(result.stdout,end='');print(result.stderr,end='');result.check_returncode();assert 'PREPARATION_NATIVE_OK' in result.stdout
if __name__=='__main__':
 parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--build-dir',type=Path,default=Path('/tmp/pn-preparation-native-test'));run(parser.parse_args().build_dir)
