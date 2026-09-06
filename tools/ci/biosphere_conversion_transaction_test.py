#!/usr/bin/env python3
"""Execute exact Biosphere conversion NPC and original failures in a native VM."""
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
from biosphere_crown_transaction_test import WRAPPERS as CROWN_WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/custom/varmundt_biosphere_depth.txt'
BASE = 'bc55ec95dc341b43f458a71160ecd75b0f33c9d2'
BASE_SHA = '790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df'
PREFIX = 'tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER = 'tools/ci/biosphere_conversion_transaction_test.cpp'
RECIPES = [([1001550], [10], 10000, 1001552),
           ([1001551], [10], 10000, 1001553),
           ([1001552,1001553], [10,10], 20000, 1001554),
           ([1001554,6607], [5,5], 30000, 1001555),
           ([1001555,6608,6755,25866], [5,5,5,3], 50000, 1001556)]


def require(ok, message):
    if not ok: raise AssertionError(message)


def sha(data): return hashlib.sha256(data).hexdigest()


def normal(data): return data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')


def validate():
    from biosphere_conversion_callback_audit import validate as callback_validate
    callbacks = callback_validate(ROOT)
    old = subprocess.check_output(['git','show',f'{BASE}:{NPC}'], cwd=ROOT)
    current = (ROOT / NPC).read_bytes()
    require(sha(old) == BASE_SHA, 'Pinned original NPC differs')
    before = old.split(b'L_Convert:\n',1)
    after = normal(current).split(b'L_Convert:\n',1)
    require(len(after) == 2 and before[0] == after[0], 'Crown/helper/prefix bytes changed, except newline form')
    marker = b'\t.@max = Zeny / .@cost;\n'
    require(before[1].split(marker,1)[0] == after[1].split(marker,1)[0], 'Any original recipe/menu field changed')
    require(b'REPUTATION_BIOSPHERE_DEPTH2' not in after[1] and b'S_Access' not in after[1],
            'Conversion must not gain a Depth2 threshold')
    records = {}
    for r in renewal_records(ROOT, 'db/item_db.yml'): records.setdefault(r['Id'],{}).update(r)
    ids = sorted({i for mats,_,_,out in RECIPES for i in mats+[out]})
    require(len(ids) == 11, 'Exact eleven conversion identities')
    for id in ids:
        r = records[id]
        require(r['Type'] == 'Etc' and r['Weight'] == 10 and not r.get('Stack') and
                not any(r.get(k) for k in ('Script','EquipScript','UnEquipScript')) and
                not any(r.get('Flags',{}).get(k) for k in ('AutoEquip','UniqueId')),
                f'Reviewed plain material/output invariant changed: {id}')
    achievements = [r for r in renewal_records(ROOT,'db/achievement_db.yml') if r.get('Group') == 'Get_Item']
    require(len(achievements) == 7, 'Exact current AG_GET_ITEM callback definitions required')
    reps = [r for r in renewal_records(ROOT,'db/reputation.yml') if r.get('Id') in (6,9)]
    require(len(reps) == 2, 'Current reputation metadata required')
    print('CONVERSION_STATIC_OK: five unchanged recipes; preserved crown prefix; required callback gate PASS',flush=True)
    return old, current, [records[i] for i in ids], achievements, reps, callbacks


def check_output(result, mode):
    result.check_returncode()
    text = re.sub(r'\x1b\[[0-9;]*m','',result.stdout+'\n'+result.stderr)
    require(text.count('Memory manager: No memory leaks found.') == 1, 'Missing clean allocator teardown')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|'
                          r'Memory manager:(?! No memory leaks found\.)',text,re.I), 'Sanitizer/allocator diagnostic')
    residual = text
    if mode == 'getter-missing':
        require(text.count('CONVERSION_GETTER_FAILURES_OK') == 1, 'Getter failure proof absent')
        require(text.count("buildin_getinventoryslots: Player with char id '99009999' is not found.") == 1,
                'Exact optional unknown-character diagnostic required')
        require(text.count('buildin_getinventoryslots: fatal error ! player not attached!') == 1,
                'Exact no-attached-player diagnostic required')
        residual = residual.replace("buildin_getinventoryslots: Player with char id '99009999' is not found.", '')
        residual = residual.replace('buildin_getinventoryslots: fatal error ! player not attached!', '')
    elif mode == 'original':
        require(text.count('CONVERSION_OLD_FAILURES_OK') == 1, 'Original failure proof absent')
        require(text.count("script_set_reg: failed to set param 'Zeny' to -1.") == 5,
                'Exact five original negative-Zeny failures required')
        require(text.count('buildin_delitem: failed to delete') == 3, 'Three original later-material failures required')
        warning = "[Warning]: Script command 'delitem' returned failure."
        require(text.count(warning) == 3, 'Exactly three original delitem failure warnings required')
        residual = residual.replace("script_set_reg: failed to set param 'Zeny' to -1.", '').replace(warning, '')
        for amount, item in ((10,1001553),(5,6607),(3,25866)):
            diagnostic = f'buildin_delitem: failed to delete {amount} items (AID=99000001 item_id={item}).'
            require(text.count(diagnostic) == 1, 'Exact original failing material diagnostic required')
            residual = residual.replace(diagnostic, '')
    else:
        require(text.count('CONVERSION_NATIVE_OK') == 1, 'Native completion marker absent')
    require(not re.search(r'\[(?:error|warning)\]|script_set_reg:|buildin_delitem: failed|fatal error',residual,re.I),
            'Unexpected native/script warning or error, including negative child')
    return text


def output_negative_controls():
    base = 'CONVERSION_NATIVE_OK cases=1 assertions=1\n[Info]: Memory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([],0,base,''),'candidate')
    samples = [base.replace('Memory manager: No memory leaks found.',''),
               *(base+'\n'+bad for bad in ('[Error]: unexpected', '[Warning]: unexpected',
                 'AddressSanitizer: heap-use-after-free','runtime error: overflow',
                 'Memory manager: invalid pointer','double free'))]
    for sample in samples:
        try: check_output(subprocess.CompletedProcess([],0,sample,''),'candidate')
        except AssertionError: pass
        else: raise AssertionError('Zero-exit output negative control unexpectedly passed')
    print(f'CONVERSION_OUTPUT_NEGATIVES_OK: {len(samples)} zero-exit diagnostics rejected',flush=True)


def native(build, inputs, reuse=False):
    build = build.resolve()
    require(build != ROOT and ROOT not in build.parents, 'Artifacts must remain outside repository')
    build.mkdir(parents=True,exist_ok=True)
    old,current,items,achievements,reps,callbacks = inputs
    (build/'before.txt').write_bytes(old)
    (build/'after.txt').write_bytes(normal(current))
    for name,rows in [('items',items),('achievements',achievements),('reputation',reps)]:
        (build/(name+'.yml')).write_text(yaml.safe_dump({'Body':rows},sort_keys=False))
    prefix = (ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]
    combined = build/'combined_conversion_test.cpp'
    combined.write_text(prefix+'\n'+(ROOT/DRIVER).read_text())
    production = ['src/map/pc.cpp','src/map/script.cpp','src/map/itemdb.cpp','src/map/clif.cpp',
                  'src/map/achievement.cpp','src/common/malloc.cpp']
    tracked = production + [PREFIX,DRIVER,'tools/ci/biosphere_conversion_transaction_test.py',
                            'tools/ci/biosphere_crown_transaction_test.py']
    hashes = {p:sha((ROOT/p).read_bytes()) for p in tracked}
    executable = build/'biosphere_conversion_transaction_test'
    if reuse:
        require(json.loads((build/'build.json').read_text()) == hashes, 'Retained binary source mismatch')
    else:
        san = ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
        flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
        flags += ['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
                  '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
        cache = json.loads((build/'build.json').read_text()) if (build/'build.json').is_file() else {}
        def compile_one(source):
            out = build/(Path(source).stem+'.o')
            if source in hashes and cache.get(source) == hashes[source] and out.is_file():
                print('Reuse exact-source sanitizer object '+source,flush=True)
            else:
                print('Fresh compile '+str(source),flush=True)
                subprocess.run(flags+['-c',str(source),'-o',str(out)],cwd=ROOT,check=True)
            return out
        with ThreadPoolExecutor(max_workers=2) as pool:
            fresh = list(pool.map(compile_one,production+[str(combined)]))
        require(hashes == {p:sha((ROOT/p).read_bytes()) for p in tracked}, 'Source changed during compile')
        excluded = {p.name for p in fresh}
        objects = sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
        libs = [ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        require(objects and all(p.is_file() for p in libs),'Native support objects missing')
        # pc_show_questinfo and achievement_update_objective execute for real.
        wrappers = [w for w in CROWN_WRAPPERS if w not in (
            '_Z17pc_show_questinfoP16map_session_data',
            '_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz')]
        wrappers += ['_Z16clif_scriptinputR16map_session_dataj',
                     '_Z9map_id2bli',
                     '_Z13map_charid2sdi',
                     '_Z8log_zenyRK16map_session_data15e_log_pick_typeji',
                     '_Z14pc_setregistryP16map_session_datall']
        command = ['g++']+san+['-o',str(executable)]+[str(p) for p in fresh+objects+libs]
        command += ['-Wl,--wrap='+w for w in wrappers]
        command += ['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
        subprocess.run(command,cwd=ROOT,check=True)
        (build/'build.json').write_text(json.dumps(hashes,indent=2)+'\n')
    outputs = {}
    for mode in ('candidate','original','getter-missing'):
        result = subprocess.run([str(executable),str(build),mode],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (build/(mode+'.stdout.txt')).write_text(result.stdout)
        (build/(mode+'.stderr.txt')).write_text(result.stderr)
        print(result.stdout,end='');print(result.stderr,end='',file=sys.stderr)
        outputs[mode] = check_output(result,mode)
    from biosphere_conversion_callback_audit import validate as callback_validate
    require(callback_validate(ROOT) == callbacks,'Callback closure changed during proof')
    require((ROOT/NPC).read_bytes() == current,'NPC changed during proof')
    require(hashes == {p:sha((ROOT/p).read_bytes()) for p in tracked},'Compiled source changed during proof')
    counts = re.search(r'CONVERSION_NATIVE_OK cases=(\d+) assertions=(\d+)',outputs['candidate'])
    require(counts,'Native coverage counts missing')
    receipt = {'result':'PASS','npc_sha256':sha(current),'before_sha256':sha(old),
               'executable_sha256':sha(executable.read_bytes()),
               'cases':int(counts[1]),'assertions':int(counts[2]),'fresh_sources':hashes,
               'asan_ubsan':True,'network':'kernel denied',
               'callback_manifest_sha256':sha(json.dumps(callbacks,sort_keys=True).encode()),
               'original_failures':'Five stale-Zeny and three later-material partial-payment paths, separate process',
               'getter_failures':'Actual builtin unknown-char and no-attached-player return FAILURE and push -1, separate process',
               'weight_status_boundary':'clif_updatestatus transport double stops Weight50/Weight90 execution; required source/data gate proves current scoped purity',
               'native_boundary':'Actual input/getinventoryslots/VM/add/delete/Zeny/AG_GET_ITEM; explicit transport and registry/log persistence doubles'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build-dir',type=Path)
    parser.add_argument('--reuse-build',action='store_true')
    args = parser.parse_args()
    output_negative_controls()
    inputs = validate()
    if args.native_build_dir: native(args.native_build_dir,inputs,args.reuse_build)
