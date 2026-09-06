#!/usr/bin/env python3
"""Real old/current pc.cpp, switch registration/deletion, and Ellie exchange VM."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import yaml
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT=Path(__file__).resolve().parents[2]
BASE='50717df9e48a6a762303a05e5d92dcc65092b8bc'
OLD_NORMALIZED_SHA='416f501be40a1346e9d07ae54a3b4e59dbf8a7780a1dfde3729217d3c9aef250'
FIXED_NORMALIZED_SHA='660c0bcc0e9efeaafc65c862fb08d97de8d1dd2648548eeb1714919f247ec70f'
NPC='npc/custom/varmundt_biosphere_quests.txt'
ACCESS='npc/custom/varmundt_biosphere.txt'
PREFIX='tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER='tools/ci/equipswitch_deletion_test.cpp'
def require(ok,message):
    if not ok: raise AssertionError(message)
def sha(data):return hashlib.sha256(data).hexdigest()
def normalized(data):return data.replace(b'\r\n',b'\n').replace(b'\r',b'\n')

def check_pc_delta(old,current):
    require(sha(normalized(current))==FIXED_NORMALIZED_SHA,'Reviewed normalized pc.cpp changed')
    require(sha(normalized(old))==OLD_NORMALIZED_SHA,'Immutable old source must match independently recorded normalized SHA256')
    expected=normalized(old).replace(b'if(n < 0 || sd->inventory.u.items_inventory[n].nameid',b'if(n < 0 || n >= MAX_INVENTORY || sd->inventory.u.items_inventory[n].nameid',1)
    expected=expected.replace(b'\tif( sd->inventory.u.items_inventory[n].amount <= 0 ){\n',b'\tif( sd->inventory.u.items_inventory[n].amount <= 0 ){\n\t\t// Remove every switch-cache reference while the old item mask still\n\t\t// exists. A replacement in this cell must not inherit its registration.\n\t\tpc_equipswitch_remove(sd, n);\n',1)
    require(normalized(current)==expected,'Only approved native bounds and depletion cleanup delta allowed')

def newline_controls(old,current):
    before=normalized(old);after=normalized(current)
    forms=lambda data:(data,data.replace(b'\n',b'\r\n'))
    for old_form in forms(before):
        for new_form in forms(after):check_pc_delta(old_form,new_form)
    for old_form in forms(before):
        for new_form in forms(after):
            try:check_pc_delta(old_form,new_form.replace(b'n >= MAX_INVENTORY',b'n > MAX_INVENTORY',1))
            except AssertionError:pass
            else:raise AssertionError('Non-newline native change accepted')
    print('EQUIPSWITCH_NEWLINE_CONTROLS_OK: 4 LF/CRLF combinations accepted; 4 semantic mutations rejected',flush=True)

def validate():
    from biosphere_callback_closure_audit import validate as callback_validate
    manifest=callback_validate(ROOT)
    old=subprocess.check_output(['git','show',f'{BASE}:src/map/pc.cpp'],cwd=ROOT)
    current=(ROOT/'src/map/pc.cpp').read_bytes()
    check_pc_delta(old,current)
    newline_controls(old,current)
    for p in (NPC,ACCESS):require(normalized((ROOT/p).read_bytes())==normalized(subprocess.check_output(['git','show',f'{BASE}:{p}'],cwd=ROOT)),'Unchanged NPC/access source required')
    ids={450199,480144,470107,490297,1151,2224,1000640,1000641,1000642,1000643,1001182,1001180,1001178,
         450201,480145,470108,450200,480146,470109,450203,480148,470111,450202,480147,470110,490299,490300,490301}
    records={}
    for row in renewal_records(ROOT,'db/item_db.yml'):
        if row['Id'] in ids:records.setdefault(row['Id'],{}).update(row)
    require(set(records)==ids,'All exact Ellie and registration fixture identities exist')
    print('EQUIPSWITCH_STATIC_OK: exact two native changes; all NPC/recipe bytes unchanged; mandatory gate PASS',flush=True)
    return old,current,[records[i] for i in sorted(records)],manifest

def check_clean(result,marker):
    result.check_returncode();text=re.sub(r'\x1b\[[0-9;]*m','',result.stdout+'\n'+result.stderr)
    require(text.count(marker)==1,'Missing unique native completion marker')
    require(text.count('Memory manager: No memory leaks found.')==1,'Clean allocator teardown required')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|\[(?:Error|Warning)\]|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)',text,re.I),'Unexpected sanitizer/allocator/native diagnostic')
    return text

def check_old_oob(result,build,old):
    require(result.returncode!=0,'Old out-of-range process must fail')
    stderr=result.stderr.replace('\r\n','\n')
    marker=re.fullmatch(r'EXPECTED_OLD_OOB_BEGIN pc_delitem index=(\d+) MAX_INVENTORY=(\d+)',stderr.splitlines()[0] if stderr else '')
    require(marker and int(marker[1])>0 and marker[1]==marker[2] and stderr.count('EXPECTED_OLD_OOB_BEGIN')==1,'Unique exact native MAX_INVENTORY request marker required')
    source=normalized(old).decode();start=source.index('char pc_delitem(')
    statement=source.index('\tif(n < 0 || sd->inventory.u.items_inventory[n].nameid',start)
    line=source.count('\n',0,statement)+1
    diagnostic=re.escape(str(build/'pc_old.cpp'))+rf':{line}:\d+: runtime error: index {marker[1]} out of bounds for type \'item \[{marker[2]}\]\''
    require(len(stderr.splitlines())==2 and re.fullmatch(diagnostic,stderr.splitlines()[1]),'Only exact old pc_delitem item-array bounds diagnostic accepted')
    require(not re.search(r'\[(?:Error|Warning)\]|AddressSanitizer|runtime error:|Memory manager:',result.stdout,re.I),'No unrelated early failure or teardown in expected-abort child')

def output_negative_controls():
    marker='EQUIPSWITCH_FIXED_OK';base=marker+' cases=1 assertions=1\n[Info]: Memory manager: No memory leaks found.\n'
    check_clean(subprocess.CompletedProcess([],0,base,''),marker)
    samples=[base.replace('Memory manager: No memory leaks found.',''),*(base+'\n'+bad for bad in
             ('[Error]: unexpected','[Warning]: unexpected','AddressSanitizer: fault','runtime error: overflow','Memory manager: invalid pointer','double free'))]
    for text in samples:
        try:check_clean(subprocess.CompletedProcess([],0,text,''),marker)
        except AssertionError:pass
        else:raise AssertionError('Zero-exit diagnostic control incorrectly accepted')
    print(f'EQUIPSWITCH_OUTPUT_NEGATIVES_OK: {len(samples)} zero-exit diagnostics rejected',flush=True)

def native(build,inputs):
    build=build.resolve();require(build!=ROOT and ROOT not in build.parents,'Artifacts must be outside repository');build.mkdir(parents=True,exist_ok=True)
    old,current,items,manifest=inputs
    (build/'pc_old.cpp').write_bytes(old)
    (build/'npc.txt').write_bytes((ROOT/NPC).read_bytes());(build/'access.txt').write_bytes((ROOT/ACCESS).read_bytes())
    (build/'items.yml').write_text(yaml.safe_dump({'Body':items},sort_keys=False))
    combined=build/'combined.cpp';combined.write_text((ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]+'\n'+(ROOT/DRIVER).read_text())
    production=['src/map/pc.cpp','src/map/script.cpp','src/map/itemdb.cpp','src/map/clif.cpp','src/common/malloc.cpp']
    tracked=production+[NPC,ACCESS,PREFIX,DRIVER,'tools/ci/equipswitch_deletion_test.py','tools/ci/biosphere_crown_transaction_test.py']
    hashes={p:sha((ROOT/p).read_bytes()) for p in tracked}
    context={'sources':hashes,'callback_manifest':sha(json.dumps(manifest,sort_keys=True).encode())}
    san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
    flags+=['-I'+p for p in ('src','src/map','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
    cache=json.loads((build/'build.json').read_text()) if (build/'build.json').is_file() else {}
    def compile_one(source):
        target=build/(Path(source).stem+'.o')
        if source in hashes and cache.get('callback_manifest')==context['callback_manifest'] and cache.get('sources',{}).get(source)==hashes[source] and target.is_file():print('Reuse exact-source object '+source,flush=True)
        else:
            print('Fresh compile '+source,flush=True);subprocess.run(flags+['-c',source,'-o',str(target)],cwd=ROOT,check=True)
        return target
    with ThreadPoolExecutor(max_workers=2) as pool:compiled=list(pool.map(compile_one,production+[str(build/'pc_old.cpp'),str(combined)]))
    require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Tracked source changed during compilation')
    exclude={'pc.o','script.o','itemdb.o','clif.o'}
    objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in exclude)
    libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(p.is_file() for p in libs),'Existing native support objects required')
    wrappers=[w for w in WRAPPERS if w not in ('_Z12pc_equipitemP16map_session_datasib','_Z14pc_unequipitemP16map_session_dataii')]
    wrappers+=['_Z20clif_equipswitch_addPK16map_session_datatjh','_Z23clif_equipswitch_removePK16map_session_datatjb','_Z8log_zenyRK16map_session_data15e_log_pick_typeji']
    bins={}
    for mode in ('fixed','old'):
        binary=build/('equipswitch_'+mode);bins[mode]=binary
        fresh=[p for p in compiled if p.name!=('pc_old.o' if mode=='fixed' else 'pc.o')]
        cmd=['g++']+san+['-o',str(binary)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
        subprocess.run(cmd,cwd=ROOT,check=True)
    (build/'build.json').write_text(json.dumps(context,indent=2)+'\n')
    outputs={}
    for mode in ('fixed','old','old-oob'):
        result=subprocess.run([str(bins['old' if mode=='old-oob' else mode]),str(build),mode],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (build/(mode+'.stdout.txt')).write_text(result.stdout);(build/(mode+'.stderr.txt')).write_text(result.stderr)
        print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
        if mode=='old-oob':
            check_old_oob(result,build,old)
        else:outputs[mode]=check_clean(result,'EQUIPSWITCH_FIXED_OK' if mode=='fixed' else 'EQUIPSWITCH_OLD_REPRO_OK')
    from biosphere_callback_closure_audit import validate as callback_validate
    require(callback_validate(ROOT)==manifest,'Callback gate changed during proof')
    require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Tracked source changed during proof')
    require((build/'npc.txt').read_bytes()==(ROOT/NPC).read_bytes() and (build/'access.txt').read_bytes()==(ROOT/ACCESS).read_bytes(),'Actual raw NPC/access fixture bytes must match final checkout')
    counts={m:dict(zip(('cases','assertions'),map(int,re.search(r'cases=(\d+) assertions=(\d+)',t).groups()))) for m,t in outputs.items()}
    receipt={'result':'PASS','counts':counts,'old_pc_sha256':sha(old),'fixed_pc_sha256':sha(current),'build':context,
             'binaries':{m:sha(b.read_bytes()) for m,b in bins.items()},'asan_ubsan':True,'old_upper_bound':'separate expected sanitizer failure',
             'boundary':'Actual registration/eligibility/deletion/add/helper and Ellie VM; switch notification arguments observed, transport/registry/log/world callbacks doubled or absent; no full ba_in01 startup'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-build-dir',type=Path,required=True);args=parser.parse_args()
    output_negative_controls()
    native(args.native_build_dir,validate())
