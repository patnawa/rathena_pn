#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  finalbattle_reward_capacity_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/finalbattle_reward_capacity_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Native exact Final Battle array-helper, crystal and achievement regression.

No Git, world startup, remote access, automatic evidence rebaseline, or model of
pc_additem replaces the actual VM/native grants. Artifacts stay outside the repo.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import yaml

import finalbattle_reward_callback_audit as gate
import episode20_21_questinfo_migration as questinfo_migration
from biosphere_crown_transaction_test import WRAPPERS as CROWN_WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
NPC = gate.NPC
ORIGINAL_SHA = '7f8dc969e03879ec90bc11f01e553a5556beb2d931e155061523ee91cd6026cd'
PRE_QI_CURRENT_LF_SHA = '3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1'
CURRENT_LF_SHA = 'd93ccfe36f48cccaf041e15d24c6036cc2fd908afe89103bd5a1dd3924558ace'
PREFIX = 'tools/ci/biosphere_crown_transaction_test.cpp'
DRIVER = 'tools/ci/finalbattle_reward_capacity_test.cpp'
RUNNER = 'tools/ci/finalbattle_reward_capacity_test.py'
PRODUCTION = ['src/map/pc.cpp','src/map/script.cpp','src/map/itemdb.cpp',
              'src/map/clif.cpp','src/map/achievement.cpp','src/common/malloc.cpp']


def require(ok, message):
    if not ok: raise AssertionError(message)


def sha(data): return hashlib.sha256(data).hexdigest()
def normal(data): return data.replace(b'\r\n', b'\n').replace(b'\r', b'\n')


def validate_source(current):
    """Invert only the pinned questinfo relocation before capacity review."""
    require(sha(normal(current)) == CURRENT_LF_SHA, 'Frozen normalized NPC changed')
    phase, source, migrated, _ = questinfo_migration.pair(NPC, current)
    require(phase == 'after' and sha(migrated.encode()) == CURRENT_LF_SHA,
            'Exact reviewed post-migration Final Battle source required')
    require(sha(source.encode()) == PRE_QI_CURRENT_LF_SHA,
            'Questinfo inverse did not reconstruct the exact capacity-fixed source')
    marker = '// Exact capacity for this file'
    require(source.count(marker) == 1, 'Exact helper insertion region')
    start, end = source.index(marker), source.index('jor_raise1,132,323,4')
    old = source[:start] + source[end:]
    call = 'callfunc("EP21_FB_CheckPlainBatch",.@reward_item,.@reward_amount,.@reward_count)'
    require(old.count(call) == 2, 'Only two crystal capacity call sites')
    old = old.replace(call, 'checkweight2(.@reward_item,.@reward_amount)').encode()
    require(sha(old) == ORIGINAL_SHA, 'Exact baseline source not restored by only helper/two call changes')
    helper = gate.body(source, 'EP21_FB_CheckPlainBatch')
    require(helper.count('getinventorylist;') == 1 and 'freeloop' not in source,
            'One snapshot and no script-budget bypass')
    return old, helper, source.encode()


def source_newline_controls(current):
    lf = normal(current)
    crlf = lf.replace(b'\n', b'\r\n')
    require(lf != crlf, 'Positive controls must exercise distinct line endings')
    require(validate_source(lf) == validate_source(crlf), 'LF/CRLF exact source equivalence')
    mutations = [(b'.@slots < 1', b'.@slots < 0'),
                 (b'26473,14568,9000', b'26474,14568,9000')]
    negatives = 0
    for original in (lf, crlf):
        samples = [original+b'// nonnewline content addition\n']
        for before, after in mutations:
            require(before in original, 'Negative source target must exist')
            samples.append(original.replace(before, after, 1))
        for sample in samples:
            try: validate_source(sample)
            except AssertionError: negatives += 1
            else: raise AssertionError('Non-newline source mutation unexpectedly accepted')
    print(f'FINALBATTLE_NEWLINE_CONTROLS_OK: 2 positive / {negatives} content negatives', flush=True)


def validate():
    callbacks = gate.validate(ROOT)
    current = (ROOT/NPC).read_bytes()
    old, helper, pre_qi = validate_source(current)
    source_newline_controls(current)
    reader = gate.base.Reader(ROOT)
    _, rows = gate.base.database_graph(reader)
    records = gate.base.scalar_overlay(rows['db/item_db.yml'], 'Id')
    relevant = [r for r in callbacks['achievements']['ordered_records']
                if r.get('Group') in ('Get_Item', 'Goal_Achieve')]
    require(len(relevant) == 27, 'All seven Get_Item and twenty Goal_Achieve definitions')
    partners = {r.get('Rewards', {}).get('Item') for r in relevant} - {None}
    require(partners == {'Gift_Box'}, 'Only reviewed deferred reward metadata dependency')
    extra = [r for r in records.values() if r.get('AegisName') in partners]
    require(len(extra) == 1 and extra[0]['Id'] == 644, 'Actual Gift_Box definition')
    items = callbacks['outputs']['records'] + extra
    daily = gate.body(reader.text(gate.DAILY), 'EP21_DailyKey')
    print('FINALBATTLE_STATIC_OK: baseline exact except one helper/two checks; required callback gate PASS', flush=True)
    return old, current, pre_qi, helper, items, relevant, callbacks, daily


def check_output(result, mode):
    result.check_returncode()
    text = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout+'\n'+result.stderr)
    require(text.count('Memory manager: No memory leaks found.') == 1, 'Missing clean allocator teardown')
    require(not re.search(r'AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|(?:invalid|double) free|'
                          r'Memory manager:(?! No memory leaks found\.)', text, re.I), 'Sanitizer/allocator diagnostic')
    residual = text
    marker = 'FINALBATTLE_OLD_FAILURES_OK' if mode == 'original' else 'FINALBATTLE_NATIVE_OK'
    require(text.count(marker) == 1, 'Missing/duplicate native completion marker')
    if mode == 'original':
        diagnostic = 'buildin_getitem: Failed to add the item to player.'
        warning = "[Warning]: Script command 'getitem' returned failure."
        require(text.count(diagnostic) == 3 and text.count(warning) == 3,
                'Exactly three pinned original grant failures required')
        residual = residual.replace(diagnostic, '').replace(warning, '')
    require(not re.search(r'\[(?:error|warning)\]|buildin_.*(?:failed|fatal)|infinity loop|fatal error',
                          residual, re.I), 'Unexpected native/script warning or error')
    return text


def output_negative_controls():
    good = 'FINALBATTLE_NATIVE_OK cases=1 assertions=1\n[Info]: Memory manager: No memory leaks found.\n'
    check_output(subprocess.CompletedProcess([], 0, good, ''), 'candidate')
    samples = [good.replace('Memory manager: No memory leaks found.', ''),
               good.replace('FINALBATTLE_NATIVE_OK', ''),
               *(good+'\n'+bad for bad in ('[Error]: unexpected','[Warning]: unexpected',
                 'AddressSanitizer: heap-use-after-free','runtime error: overflow',
                 'Memory manager: invalid pointer','double free','run_script: infinity loop !'))]
    for sample in samples:
        try: check_output(subprocess.CompletedProcess([], 0, sample, ''), 'candidate')
        except AssertionError: pass
        else: raise AssertionError('Zero-exit negative unexpectedly accepted')
    old = good.replace('FINALBATTLE_NATIVE_OK', 'FINALBATTLE_OLD_FAILURES_OK')
    old += ('buildin_getitem: Failed to add the item to player.\n'
            "[Warning]: Script command 'getitem' returned failure.\n") * 3
    check_output(subprocess.CompletedProcess([], 0, old, ''), 'original')
    for sample in (old+'\n[Error]: unrelated', old.replace('Failed to add', 'Failed to give', 1)):
        try: check_output(subprocess.CompletedProcess([], 0, sample, ''), 'original')
        except AssertionError: pass
        else: raise AssertionError('Original-mode negative unexpectedly accepted')
    print(f'FINALBATTLE_OUTPUT_NEGATIVES_OK: {len(samples)+2} zero-exit failures rejected', flush=True)


def native(build, inputs, reuse=False):
    build = build.resolve()
    require(build != ROOT and ROOT not in build.parents, 'Artifacts must be outside repository')
    build.mkdir(parents=True, exist_ok=True)
    old, current, pre_qi, helper, items, achievements, callbacks, daily = inputs
    (build/'before.txt').write_bytes(old)
    (build/'after.txt').write_bytes(normal(current))
    (build/'helper.txt').write_text(helper)
    (build/'daily.txt').write_text(daily)
    for variant, text in (('before', old.decode()), ('after', normal(current).decode())):
        for name, declaration in (('normal','Giant Serpent Crystal#ep21_fb'),
                                  ('hard','Giant Serpent Crystal#ep21_fb_hard')):
            (build/f'{name}-{variant}.txt').write_text(gate.body(text, declaration))
    levels = callbacks['achievements']['levels']['ordered_records']
    for name, rows in [('items',items), ('achievements',achievements), ('levels',levels)]:
        (build/f'{name}.yml').write_text(yaml.safe_dump({'Body':rows}, sort_keys=False))
    prefix = (ROOT/PREFIX).read_text().split('extern "C" int __wrap_main(',1)[0]
    remove = ('extern "C" npc_data* npc_lookup(int32) asm("__wrap__Z9map_id2ndi");\n'
              'extern "C" npc_data* npc_lookup(int32){return nullptr;}\n')
    require(prefix.count(remove) == 1, 'Exact test world-lookup boundary replacement')
    combined = build/'combined_finalbattle_test.cpp'
    combined.write_text(prefix.replace(remove, '')+'\n'+(ROOT/DRIVER).read_text())
    tracked = PRODUCTION + [PREFIX,DRIVER,RUNNER,'tools/ci/biosphere_crown_transaction_test.py',
                            'tools/ci/finalbattle_reward_callback_audit.py',
                            'tools/ci/episode20_21_questinfo_migration.py']
    hashes = {p:sha((ROOT/p).read_bytes()) for p in tracked}
    headers = sorted(p for root in ('src','3rdparty') for p in (ROOT/root).rglob('*')
                     if p.is_file() and p.suffix in ('.h','.hpp'))
    header_hashes = {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in headers}
    excluded = {Path(p).with_suffix('.o').name for p in PRODUCTION}
    objects = sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs = [ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a',
                             '3rdparty/rapidyaml/obj/ryml.a')]
    require(objects and all(p.is_file() for p in libs), 'Native support objects missing')
    support_hashes = {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in objects+libs}
    san = ['-fsanitize=address,undefined','-fno-sanitize-recover=all','-fno-omit-frame-pointer']
    flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing']+san
    flags += ['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
              '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
    context = {'tracked':hashes, 'headers':header_hashes, 'support':support_hashes, 'flags':flags}
    executable = build/'finalbattle_reward_capacity_test'
    if reuse:
        cached = json.loads((build/'build.json').read_text())
        require(cached['context'] == context and cached['binary'] == sha(executable.read_bytes()),
                'Retained binary/source/header/support/flag mismatch')
    else:
        def compile_one(source):
            path = Path(source)
            raw = path.read_bytes() if path.is_absolute() else (ROOT/path).read_bytes()
            key = {'source':sha(raw), 'headers':header_hashes, 'flags':flags}
            out = build/(path.stem+'.o')
            stamp = out.with_suffix('.json')
            cached = json.loads(stamp.read_text()) if stamp.is_file() else {}
            if cached.get('context') == key and out.is_file() and cached.get('object') == sha(out.read_bytes()):
                print('Retain exact-source/header sanitizer object '+str(source), flush=True)
            else:
                print('Fresh compile '+str(source), flush=True)
                subprocess.run(flags+['-c',str(source),'-o',str(out)], cwd=ROOT, check=True)
                stamp.write_text(json.dumps({'context':key,'object':sha(out.read_bytes())},sort_keys=True))
            return out
        with ThreadPoolExecutor(max_workers=2) as pool:
            fresh = list(pool.map(compile_one, PRODUCTION+[str(combined)]))
        wrappers = [w for w in CROWN_WRAPPERS if w not in (
            '_Z17pc_show_questinfoP16map_session_data',
            '_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz')]
        wrappers += ['_Z9map_id2bli','_Z13map_charid2sdi','_Z14pc_setregistryP16map_session_datall',
                     '_Z23clif_achievement_updateP16map_session_dataPK11achievementi']
        command = ['g++']+san+['-o',str(executable)]+[str(p) for p in fresh+objects+libs]
        command += ['-Wl,--wrap='+w for w in wrappers]
        command += ['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
        subprocess.run(command,cwd=ROOT,check=True)
        (build/'build.json').write_text(json.dumps({'context':context,'binary':sha(executable.read_bytes())},indent=2)+'\n')
    outputs = {}
    for mode in ('candidate','original'):
        result = subprocess.run([str(executable),str(build),mode],cwd=ROOT,capture_output=True,text=True,timeout=120)
        (build/f'{mode}.stdout.txt').write_text(result.stdout)
        (build/f'{mode}.stderr.txt').write_text(result.stderr)
        print(result.stdout,end=''); print(result.stderr,end='',file=sys.stderr)
        outputs[mode] = check_output(result,mode)
    require(gate.validate(ROOT) == callbacks, 'Callback evidence changed during proof')
    require((ROOT/NPC).read_bytes() == current, 'Frozen NPC changed during proof')
    require(hashes == {p:sha((ROOT/p).read_bytes()) for p in tracked}, 'Compiled inputs changed during proof')
    require(header_hashes == {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in headers}, 'Headers changed during proof')
    require(support_hashes == {str(p.relative_to(ROOT)):sha(p.read_bytes()) for p in objects+libs}, 'Support objects changed during proof')
    count = re.search(r'FINALBATTLE_NATIVE_OK cases=(\d+) assertions=(\d+)',outputs['candidate'])
    old_count = re.search(r'FINALBATTLE_OLD_FAILURES_OK cases=(\d+) assertions=(\d+)',outputs['original'])
    require(count and old_count, 'Native exact counts missing')
    receipt = {'result':'PASS','npc_sha256':sha(current),'before_sha256':sha(old),
               'pre_questinfo_npc_lf_sha256':sha(pre_qi),
               'npc_lf_sha256':sha(normal(current)),
               'newline_controls':'LF and CRLF accepted; six non-newline content edits rejected',
               'executable_sha256':sha(executable.read_bytes()),'cases':int(count[1]),'assertions':int(count[2]),
               'old_cases':int(old_count[1]),'old_assertions':int(old_count[2]),'asan_ubsan':True,
               'network':'kernel-denied socket/connect/bind/listen', 'compiled_sources':hashes,
               'callback_manifest_sha256':sha(json.dumps(callbacks,sort_keys=True).encode()),
               'script_limits':callbacks['script_limits'],
               'native_boundary':'Actual parser/VM/array references/helper/checkweight2/getitem/pc_additem/achievement conditions/completion/level; explicit registry/world/transport/log doubles',
               'weight_status_boundary':'clif_updatestatus double stops Weight50/Weight90 execution; mandatory source/data gate proves this current scoped branch pure',
               'world_boundary':'Native pc_show_questinfo executes with instance-map qi_npc empty, separately proven by pinned enabled source graph; no world startup',
               'achievement_scope':'Seven Get_Item and twenty Goal_Achieve actual records plus their deferred Gift_Box metadata; all361 records pinned, conditionless other-group recursion reviewed in source',
               'limitations':'No live persisted text, deployed-binary equivalence, crash/durability atomicity, or arbitrary invalid inventories claimed'}
    (build/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--native-build-dir',type=Path)
    p.add_argument('--reuse-build',action='store_true')
    args = p.parse_args()
    require(not args.reuse_build or args.native_build_dir, '--reuse-build requires artifact directory')
    output_negative_controls()
    inputs = validate()
    if args.native_build_dir: native(args.native_build_dir,inputs,args.reuse_build)
