#!/usr/bin/env python3
"""Native mixed-clock reproduction and exact single-snapshot helper proof.

By default the proposed helper is an in-memory change, not a runtime repair.
--source-mode fixed requires that exact repair in the current full NPC source.
No server connections or shared-object rebuilds. Retained sanitizer objects
are accepted only through their exact previous source/byte binding.
"""
import argparse,hashlib,json,pathlib,re,subprocess,sys
from biosphere_crown_transaction_test import WRAPPERS
from finalbattle_reward_callback_audit import body

ROOT=pathlib.Path(__file__).resolve().parents[2]
NPC='npc/custom/episode21/MysteriousGhostShip.txt'
OLD_SHA='b6baf2dfdb5d2bc47a67a4a8f31b002eb46fbec2a9f66de963047c18e250b70d'
RETAINED_BUILD_SHA='077f5fd1c2ab24909c3d959b8f6292a9ec491290401d4a985b98938948dce0a8'
RETAINED_RECEIPT_SHA='bce8640ee1cd73b9fc91c9076f523762fd5223b99443059b0a0cec784514c9f1'
EXPECTED_COUNTS=(173268,117,45,867714)
BROAD_GATE='tools/ci/biosphere_callback_closure_audit.py'
OLD_NPC_PIN=b'9aeeaab191be38b0d36f07c0a55a14b7fae6894f48c8d4e4e5d1d3cbeff330a7'
NEW_NPC_PIN=b'bb7fb7a83501e786251566b1bb4480b521fa5dc91cffd36fd40550dcdc5a046b'
PREFIX='tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER='tools/ci/episode21_daily_clock_test.cpp'
RUNNER='tools/ci/episode21_daily_clock_test.py'
OLD_LINE='\t.@reset = .@now - (gettime(DT_HOUR) * 3600 + gettime(DT_MINUTE) * 60 + gettime(DT_SECOND)) + 14400;\n'
NEW_LINES=('\t// Derive every clock component from the same captured instant.\n'
 '\t.@clock = atoi(gettimestr("%H%M%S",7,.@now));\n'
 '\t.@reset = .@now - ((.@clock / 10000) * 3600 + ((.@clock / 100) % 100) * 60 + (.@clock % 100)) + 14400;\n')
def sha(data):return hashlib.sha256(data).hexdigest()
def require(ok,message):
    if not ok:raise AssertionError(message)
def verify_frozen(pins,read=None):
    if read is None:read=lambda path:pathlib.Path(path).read_bytes()
    require(pins=={path:sha(read(path)) for path in pins},'Frozen input, executable or source bytes changed')
def verify_retained_source(path,data,pin):
    if path==BROAD_GATE:
        require(data.count(NEW_NPC_PIN)==2 and OLD_NPC_PIN not in data,
                'Only both exact reviewed broad-gate NPC pin replacements are accepted')
        data=data.replace(NEW_NPC_PIN,OLD_NPC_PIN)
    require(sha(data)==pin,'Retained source drift: '+path)
def check_output(result):
    result.check_returncode()
    output=re.sub(r'\x1b\[[0-9;]*[A-Za-z]','',result.stdout+'\n'+result.stderr)
    require(not result.stderr,'Native clock stderr must be empty')
    require(output.count('Memory manager: No memory leaks found.')==1,'Explicit clean allocator teardown')
    require(not re.search(r'\[(?:Error|Warning|Fatal)\]|runtime error:|AddressSanitizer|UndefinedBehaviorSanitizer|(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)',output,re.I),'Unexpected diagnostic')
    match=re.search(r'EP21_DAILY_CLOCK_OK fixed_cases=(\d+) stable_controls=(\d+) original_defects=(\d+) assertions=(\d+)',output)
    require(match and output.count('EP21_DAILY_CLOCK_OK')==1,'Exact completion marker')
    counts=tuple(map(int,match.groups()));require(counts==EXPECTED_COUNTS,'Exact complete native matrix required')
    return counts
def controls():
    good='Memory manager: No memory leaks found.\nEP21_DAILY_CLOCK_OK fixed_cases=173268 stable_controls=117 original_defects=45 assertions=867714\n'
    check_output(subprocess.CompletedProcess([],0,good,''))
    bad=[good.replace('Memory manager: No memory leaks found.',''),good.replace('173268','173267'),good+good]
    bad += [good+message for message in ('[Error]: unexpected','[Warning]: unexpected','runtime error: overflow','AddressSanitizer: failed','Memory manager: invalid pointer','double free')]
    for output in bad:
        try:check_output(subprocess.CompletedProcess([],0,output,''))
        except AssertionError:pass
        else:raise AssertionError('Zero-exit incomplete/diagnostic control accepted')
    blobs={'source':b'source','executable':b'executable','fixture':b'fixture','support':b'support'}
    pins={name:sha(value) for name,value in blobs.items()};verify_frozen(pins,blobs.__getitem__)
    for name in blobs:
        changed={**blobs,name:blobs[name]+b'changed'}
        try:verify_frozen(pins,changed.__getitem__)
        except AssertionError:pass
        else:raise AssertionError('Frozen artifact mutation accepted')
    old=b'prefix '+OLD_NPC_PIN+b' middle '+OLD_NPC_PIN+b' suffix'
    current=old.replace(OLD_NPC_PIN,NEW_NPC_PIN);pin=sha(old)
    verify_retained_source(BROAD_GATE,current,pin)
    gate_bad=(current+b'x',current.replace(NEW_NPC_PIN,OLD_NPC_PIN,1),
              current.replace(b'suffix',NEW_NPC_PIN+b' suffix'),old)
    for changed in gate_bad:
        try:verify_retained_source(BROAD_GATE,changed,pin)
        except AssertionError:pass
        else:raise AssertionError('Unreviewed retained gate drift accepted')
    print('DAILY_CLOCK_GUARDS_OK: 9 output, 4 artifact and 4 retained-gate refusals',flush=True)
def native(build,retained,mode):
    build=build.resolve();retained=retained.resolve()
    require(build!=ROOT and ROOT not in build.parents and not build.exists(),'Fresh build outside repository required')
    raw=(ROOT/NPC).read_bytes();text=raw.replace(b'\r\n',b'\n').decode()
    if mode=='fixed':
        require(text.count(NEW_LINES)==1 and OLD_LINE not in text,'Only exact reviewed snapshot repair accepted')
        original=text.replace(NEW_LINES,OLD_LINE,1)
    else:original=text
    require(sha(original.encode())==OLD_SHA,'Entire genuine baseline changed')
    require(original.count(OLD_LINE)==1,'Unique original clock expression')
    candidate=original.replace(OLD_LINE,NEW_LINES,1)
    require(sha((retained/'build.json').read_bytes())==RETAINED_BUILD_SHA,'Exact trusted retained producer binding required')
    require(sha((retained/'receipt.json').read_bytes())==RETAINED_RECEIPT_SHA,'Exact accepted retained producer receipt required')
    binding=json.loads((retained/'build.json').read_text())
    for path,pin in binding['sources'].items():verify_retained_source(path,(ROOT/path).read_bytes(),pin)
    for path,pin in binding['link_inputs_sha256'].items():require(sha(pathlib.Path(path).read_bytes())==pin,'Retained link-input drift: '+path)
    paths=list(binding['sources'])+[NPC,'src/map/date.cpp',DRIVER,RUNNER,'tools/ci/finalbattle_reward_callback_audit.py']
    hashes={path:sha((ROOT/path).read_bytes()) for path in paths}
    require(hashes[NPC]==sha(raw),'NPC source changed between initial and compiled-source snapshots')
    build.mkdir()
    (build/'before.txt').write_text(body(original,'EP21_DailyKey'))
    (build/'after.txt').write_text(body(candidate,'EP21_DailyKey'))
    prefix=(ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]
    combined=build/'combined_daily_clock.cpp';combined.write_text(prefix+'\n'+(ROOT/DRIVER).read_text())
    fixture_pins={str(path):sha(path.read_bytes()) for path in (combined,build/'before.txt',build/'after.txt')}
    san=['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags=['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
    flags+=['-I'+path for path in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
    fresh=[]
    for source in [ROOT/'src/map/date.cpp',combined]:
        obj=build/(source.stem+'.o');print('Fresh sanitizer compile '+str(source),flush=True)
        subprocess.run(flags+['-c',str(source),'-o',str(obj)],cwd=ROOT,check=True);fresh.append(obj)
    retained_inputs=[pathlib.Path(path) for path in binding['link_inputs_sha256'] if pathlib.Path(path).name not in ('date.o','combined_document_test.o')]
    require(len(retained_inputs)==len(binding['link_inputs_sha256'])-2,'Exact retained date/old-driver replacement')
    inputs=fresh+retained_inputs;input_hashes={str(path):sha(path.read_bytes()) for path in inputs}
    exe=build/'episode21_daily_clock_test'
    command=['g++']+san+['-o',str(exe)]+[str(path) for path in inputs]+['-Wl,--wrap='+name for name in (*WRAPPERS,'time')]
    subprocess.run(command+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'],cwd=ROOT,check=True)
    require(input_hashes=={str(path):sha(path.read_bytes()) for path in inputs},'Link inputs changed')
    frozen={**input_hashes,str(exe):sha(exe.read_bytes()),str(retained/'build.json'):RETAINED_BUILD_SHA,
      str(retained/'receipt.json'):RETAINED_RECEIPT_SHA}
    frozen.update(fixture_pins)
    verify_frozen(frozen)
    result=subprocess.run([str(exe),str(build)],cwd=ROOT,text=True,capture_output=True,timeout=180)
    (build/'stdout.txt').write_text(result.stdout);(build/'stderr.txt').write_text(result.stderr)
    print(result.stdout,flush=True);print(result.stderr,file=sys.stderr,flush=True);counts=check_output(result)
    require(hashes=={path:sha((ROOT/path).read_bytes()) for path in paths},'Source changed during proof')
    verify_frozen(frozen)
    receipt={'result':'FIXED_SOURCE_NATIVE_PASS' if mode=='fixed' else 'ORIGINAL_REPRODUCED_AND_PROPOSED_SNAPSHOT_TESTED',
      'counts':dict(zip(('fixed_cases','stable_controls','original_defects','assertions'),counts)),
      'runtime_raw_sha256':sha(raw),'original_normalized_sha256':OLD_SHA,'candidate_normalized_sha256':sha(candidate.encode()),
      'sources':hashes,'linked_inputs':input_hashes,'executable_sha256':frozen[str(exe)],'frozen_artifacts':frozen,
      'fixture_sha256':{path.name:sha(path.read_bytes()) for path in [combined,build/'before.txt',build/'after.txt']},
      'outputs':{stream:sha((build/(stream+'.txt')).read_bytes()) for stream in ('stdout','stderr')},
      'asan_ubsan':True,'network':'kernel denied','boundary':'actual VM/time/format/atoi/date functions; deterministic time syscall boundary, transient result/world/transport doubles; fixed-offset zones only, no DST-policy repair or live claim'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
    print(receipt['result']+' '+sha((build/'receipt.json').read_bytes()),flush=True)
if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build-dir',type=pathlib.Path,required=True)
    parser.add_argument('--retained-document-build',type=pathlib.Path,required=True)
    parser.add_argument('--source-mode',choices=('original','fixed'),default='original')
    args=parser.parse_args();controls();native(args.native_build_dir,args.retained_document_build,args.source_mode)
