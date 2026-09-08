#!/usr/bin/env python3
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
    needed={4001,4700}
    def values(value):
        if isinstance(value,int) and value>=500: needed.add(value)
        elif isinstance(value,list):
            for v in value: values(v)
        elif isinstance(value,dict):
            for v in value.values(): values(v)
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
        target=build/(source.stem+'.o');signature=hashlib.sha256(source.read_bytes()+repr(flags).encode()+headerhash.encode()).hexdigest()
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
