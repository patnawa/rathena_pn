#!/usr/bin/env python3
"""Actual isolated Depth document NPC/input/inventory/reputation/loaded registry proof."""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
from unittest import mock
import yaml

from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS
from biosphere_material_transaction_test import prepare_callbacks

ROOT=Path(__file__).resolve().parents[2]
NPC='npc/custom/varmundt_biosphere_depth.txt'
PREFIX='tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER='tools/ci/biosphere_document_exchange_test.cpp'
OLD_SHA='6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988'
NEW_SHA='3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326'

def require(ok,message):
    if not ok:raise AssertionError(message)

def sha(data):return hashlib.sha256(data).hexdigest()

def replace_once(text,old,new):
    require(text.count(old)==1,'Exact fixture/source adapter drift: '+old[:80])
    return text.replace(old,new,1)

def original(raw):
    current=raw.replace(b'\r\n',b'\n');require(b'\r' not in current and sha(current)==NEW_SHA,'Reviewed LF-normalized candidate changed')
    source=current.decode()
    start=source.index('\tif (input(.@amount,1,.@max) != 0)',source.index('Depth Research Administrator#bio_d1'))
    end=source.index('\t.@gain = .@amount * 3;',start)
    source=source[:start]+'\tinput .@amount,1,.@max;\n\tdelitem 1001289,.@amount * 2;\n'+source[end:]
    source=replace_once(source,'\t// Keep the final partial-credit pair, but never consume redundant pairs.\n\t// The reviewed ba_in01 callbacks do not mutate these payment resources.\n\tdelitem 1001289,.@amount * 2;\n','')
    require(sha(source.encode())==OLD_SHA,'Entire genuine original differs: protected NPC bytes changed')
    return source.encode(),current

def effective(path):
    records={}
    for row in renewal_records(ROOT,path):records.setdefault(row['Id'],{}).update(row)
    return records

def validate(prepare=False):
    from biosphere_document_callback_audit import validate as gate,collect_evidence
    raw=(ROOT/NPC).read_bytes();before,current=original(raw)
    manifest=collect_evidence(ROOT) if prepare else gate(ROOT)
    items=effective('db/item_db.yml');reputations=effective('db/reputation.yml')
    require(items[1001289]==manifest['document']['record'],'Effective document definition matches gate')
    require(reputations[6]==manifest['reputation']['selected_record'],'Effective reputation definition matches gate')
    require(reputations[6]['Variable']=='RepPoints6' and reputations[6]['Minimum']==-5000 and reputations[6]['Maximum']==5000,'Exact reputation identity/bounds')
    print('DOCUMENT_SOURCE_OK: entire genuine original reconstructed; '+('UNVALIDATED compile preparation' if prepare else 'mandatory document gate PASS'),flush=True)
    return raw,before,current,manifest,items

def check_output(result,mode):
    result.check_returncode();text=re.sub(r'\x1b\[[0-9;]*m','',result.stdout+'\n'+result.stderr)
    require(text.count('Memory manager: No memory leaks found.')==1,'Explicit clean allocator teardown required')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)',text,re.I),'Sanitizer/allocator diagnostic')
    marker='DOCUMENT_ORIGINAL_OK' if mode=='original' else 'DOCUMENT_NATIVE_OK'
    require(text.count(marker)==1,'Native coverage marker missing')
    residual=text
    if mode=='original':
        expected='[Error]: buildin_delitem: failed to delete 4 items (AID=99000001 item_id=1001289).'
        warning="[Warning]: Script command 'delitem' returned failure."
        require(text.count(expected)==2 and text.count(warning)==2,'Exactly two original single-debit shortage diagnostics')
        residual=residual.replace(expected,'').replace(warning,'')
    require(not re.search(r'\[(?:Error|Warning)\]|fatal error',residual,re.I),'Unexpected native diagnostic, even at exit zero')
    return text

def output_controls():
    good='DOCUMENT_NATIVE_OK cases=1 assertions=1 nested_checks=1\nMemory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([],0,good,''),'candidate')
    bad=[good.replace('Memory manager: No memory leaks found.','')]+[good+x for x in
        ('[Error]: unexpected','[Warning]: unexpected','AddressSanitizer: fail','runtime error: fail','Memory manager: invalid pointer','double free')]
    for text in bad:
        try:check_output(subprocess.CompletedProcess([],0,text,''),'candidate')
        except AssertionError:pass
        else:raise AssertionError('Zero-exit diagnostic accepted')
    print('DOCUMENT_OUTPUT_GUARD_OK: 7 negative controls',flush=True)

def verify_binding(binding,hashes,exe):
    require(binding['sources']==hashes,'Retained executable source mismatch')
    require(sha(exe.read_bytes())==binding['executable_sha256'],'Retained executable bytes changed')
    require(binding['link_inputs_sha256']=={p:sha(Path(p).read_bytes()) for p in binding['link_inputs_sha256']},'Retained support/scoped object bytes changed')

def artifact_controls():
    # In-memory byte-provider controls; no retained artifact is overwritten.
    exe=Path('/document-fixture-executable');support='/document-fixture-support';sources={'source':'unchanged'}
    data={str(exe):b'executable',support:b'support'}
    binding={'sources':sources,'executable_sha256':sha(data[str(exe)]),'link_inputs_sha256':{support:sha(data[support])}}
    with mock.patch.object(Path,'read_bytes',lambda p:data[str(p)]):
        verify_binding(binding,sources,exe)
        for change in ('source','executable','support'):
            changed_sources={**sources,'source':'changed'} if change=='source' else sources
            key=str(exe) if change=='executable' else support
            if change!='source':data[key]+=b'changed'
            try:verify_binding(binding,changed_sources,exe)
            except AssertionError:pass
            else:raise AssertionError('Changed retained artifact/source accepted')
            if change!='source':data[key]=data[key][:-7]
    print('DOCUMENT_ARTIFACT_GUARD_OK: 3 in-memory source/executable/support controls',flush=True)

def native(build,inputs,prepare=False,reuse=False):
    build=build.resolve();require(build!=ROOT and ROOT not in build.parents,'Artifacts must remain outside repository');build.mkdir(parents=True,exist_ok=True)
    raw,before,current,manifest,items=inputs
    (build/'before.txt').write_bytes(before);(build/'after.txt').write_bytes(current)
    def emit(name,rows):(build/(name+'.yml')).write_text(yaml.safe_dump({'Body':rows},sort_keys=False),encoding='utf-8')
    def item_emit(name,rows):emit(name,rows+[items[1001289]] if name=='items' else rows)
    prepare_callbacks(build,manifest['material'],items,item_emit)
    emit('reputation',manifest['reputation']['ordered_records'])
    prefix=(ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]
    prefix=replace_once(prefix,'extern "C" npc_data* npc_lookup(int32){return nullptr;}','')
    prefix=replace_once(prefix,'extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*){}','')
    prefix=replace_once(prefix,'extern "C" void error(const char* f,...){++errors;va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}',
        'extern "C" void error(const char* f,...){++errors;std::fputs("[Error]: ",stderr);va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}')
    combined=build/'combined_document_test.cpp';combined.write_text(prefix+'\n'+(ROOT/DRIVER).read_text())
    production=['src/map/pc.cpp','src/map/script.cpp','src/map/itemdb.cpp','src/map/clif.cpp','src/map/achievement.cpp','src/map/quest.cpp','src/common/malloc.cpp']
    dependencies=[PREFIX,DRIVER,'tools/ci/biosphere_document_exchange_test.py','tools/ci/biosphere_document_callback_audit.py',
        'tools/ci/biosphere_crown_transaction_test.py','tools/ci/biosphere_material_transaction_test.py','tools/ci/biosphere_material_callback_audit.py',
        'tools/ci/biosphere_callback_closure_audit.py','tools/ci/biosphere_regression_scope.py','tools/ci/audit_enchant_upgrades.py']
    headers=sorted(p.relative_to(ROOT).as_posix() for tree in ('src','3rdparty') for p in (ROOT/tree).rglob('*') if p.is_file() and p.suffix in ('.h','.hpp','.inl','.tcc'))
    tracked=production+dependencies+headers;hashes={p:sha((ROOT/p).read_bytes()) for p in tracked}
    header_sha=sha(json.dumps({p:hashes[p] for p in headers},sort_keys=True).encode());exe=build/'biosphere_document_exchange_test'
    if reuse:
        binding=json.loads((build/'build.json').read_text())
        verify_binding(binding,hashes,exe)
    else:
        san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
        flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
        flags+=['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
        def compile_one(source):
            out=build/(Path(source).stem+'.o');sidecar=out.with_suffix('.source.json')
            signature={'source':str(source),'sha256':sha(Path(source).read_bytes()),'flags':flags,'headers_sha256':header_sha}
            retained=json.loads(sidecar.read_text()) if sidecar.is_file() else {}
            if out.is_file() and {k:v for k,v in retained.items() if k!='object_sha256'}==signature and retained.get('object_sha256')==sha(out.read_bytes()):print('Reuse exact-source/byte sanitizer object '+str(source),flush=True)
            else:
                print('Fresh compile '+str(source),flush=True);subprocess.run(flags+['-c',str(source),'-o',str(out)],cwd=ROOT,check=True)
                require(signature['sha256']==sha(Path(source).read_bytes()),'Source changed during compilation')
                sidecar.write_text(json.dumps({**signature,'object_sha256':sha(out.read_bytes())},indent=2)+'\n')
            return out
        with ThreadPoolExecutor(max_workers=2) as pool:fresh=list(pool.map(compile_one,production+[str(combined)]))
        require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Source changed while compiling')
        excluded={p.name for p in fresh};objects=sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
        libs=[ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        require(objects and all(p.is_file() for p in libs),'Built native support objects required')
        wrappers=[w for w in WRAPPERS if w not in ('_Z17pc_show_questinfoP16map_session_data','_Z15pc_readregistryPK16map_session_datal','_Z11pc_readreg2PK16map_session_dataPKc')]
        wrappers+=['_Z9map_id2bli','_Z16clif_scriptinputR16map_session_dataj','_Z20clif_reputation_typeRK16map_session_datall',
            '_Z27achievement_check_conditionP11script_codeP16map_session_data','_Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor']
        # No pc_setreg2/pc_setregistry/pc_readreg2/pc_readregistry wrapper: the
        # actual loaded persistent registry is part of this native proof.
        command=['g++']+san+['-o',str(exe)]+[str(p) for p in fresh+objects+libs]+['-Wl,--wrap='+w for w in wrappers]
        link_inputs={str(p):sha(p.read_bytes()) for p in fresh+objects+libs}
        subprocess.run(command+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
        require(link_inputs=={p:sha(Path(p).read_bytes()) for p in link_inputs},'Native link input changed while linking')
        binding={'sources':hashes,'executable_sha256':sha(exe.read_bytes()),'link_inputs_sha256':link_inputs}
        (build/'build.json').write_text(json.dumps(binding,indent=2)+'\n')
    if prepare:print('DOCUMENT_BUILD_ONLY: no accepted gate/native PASS claimed',flush=True);return
    outputs={}
    for mode in ('candidate','original'):
        result=subprocess.run([str(exe),str(build),mode],cwd=ROOT,capture_output=True,text=True,timeout=120)
        for stream in ('stdout','stderr'):(build/(mode+'.'+stream+'.txt')).write_text(getattr(result,stream))
        print(result.stdout,end='',flush=True);print(result.stderr,end='',file=sys.stderr,flush=True);outputs[mode]=check_output(result,mode)
    from biosphere_document_callback_audit import validate as gate
    require(gate(ROOT)==manifest,'Mandatory document callback closure changed during proof')
    require(raw==(ROOT/NPC).read_bytes(),'Raw runtime changed during proof');require(hashes=={p:sha((ROOT/p).read_bytes()) for p in tracked},'Compiled dependency changed during proof')
    verify_binding(binding,hashes,exe)
    counts={}
    for mode,text in outputs.items():
        found=re.search(r'DOCUMENT_(?:NATIVE|ORIGINAL)_OK cases=(\d+) assertions=(\d+) nested_checks=(\d+)',text);require(found,'Exact native counts required')
        counts[mode]=dict(zip(('cases','assertions','nested_checks'),map(int,found.groups())))
    require(counts['original']['cases']==20,'All exact original controls executed')
    receipt={'result':'PASS','counts':counts,'gate':'PASS before and after with identical full manifest','callback_manifest_sha256':sha(json.dumps(manifest,sort_keys=True).encode()),
        'runtime_raw_sha256':sha(raw),'original_normalized_sha256':sha(before),'source_hashes':hashes,'executable_sha256':sha(exe.read_bytes()),
        'build_binding_sha256':sha((build/'build.json').read_bytes()),'link_inputs_sha256':binding['link_inputs_sha256'],
        'fixture_sha256':{p.name:sha(p.read_bytes()) for p in sorted(build.iterdir()) if p.suffix=='.yml' or p.name in ('before.txt','after.txt','combined_document_test.cpp')},
        'native_output_sha256':{mode:{stream:sha((build/(mode+'.'+stream+'.txt')).read_bytes()) for stream in ('stdout','stderr')} for mode in outputs},
        'asan_ubsan':True,'network':'kernel denied','registry':'actual loaded native persistent read/write/dirty state; no SQL save or reconnect claim',
        'boundary':'Actual NPC/input/delitem/reputation/QuestInfo; world, reputation notification before packet construction, other transport/logs, transient @ registers and weight status notification are explicit doubles'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n');print('DOCUMENT_PROOF_OK: receipt.json written',flush=True)

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--native-build-dir',type=Path);parser.add_argument('--prepare-only',action='store_true');parser.add_argument('--reuse-build',action='store_true')
    args=parser.parse_args();output_controls();artifact_controls();inputs=validate(args.prepare_only)
    if args.native_build_dir:native(args.native_build_dir,inputs,args.prepare_only,args.reuse_build)
