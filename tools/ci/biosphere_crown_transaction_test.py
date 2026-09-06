#!/usr/bin/env python3
"""Isolated production Biosphere NPC/helper VM regression; never starts a server."""
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
from biosphere_regression_scope import reviewed_depth_baseline

ROOT = Path(__file__).resolve().parents[2]
NPC = 'npc/custom/varmundt_biosphere_depth.txt'
BASE = '5e139a555'
BASE_SHA = 'c0e59e6497f0bebef17ee98784ac92a988d54d16cacf41be188fb774f0456d09'
CROWN_BASE = 'bc55ec95dc341b43f458a71160ecd75b0f33c9d2'
CROWN_BASE_SHA = '790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df'
IDS = list(range(400529, 400547)) + [400999]
WRAPPERS = ('main', '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z11mapreg_initv', '_Z12mapreg_finalv',
    '_Z17npc_event_dequeueP16map_session_datab', '_Z9ShowErrorPKcz',
    '_Z14clif_scriptmesRK16map_session_datajPKc', '_Z15clif_scriptnextRK16map_session_dataj',
    '_Z16clif_scriptcloseRK16map_session_dataj', '_Z15clif_scriptmenuR16map_session_datajPKc',
    '_Z9pc_setregP16map_session_datall', '_Z10pc_readregPK16map_session_datal',
    '_Z12pc_setregstrP16map_session_datalPKc', '_Z13pc_readregstrPK16map_session_datal',
    '_Z15pc_readregistryPK16map_session_datal', '_Z11pc_readreg2PK16map_session_dataPKc', '_Z13mapreg_setregll', '_Z14mapreg_readregl',
    '_Z23clif_enchantwindow_openR16map_session_datam', '_Z19clif_displaymessageiPKc',
    '_Z11log_pick_pcPK16map_session_data15e_log_pick_typeiPK4item',
    '_Z12clif_additemPK16map_session_dataiih', '_Z12clif_delitemRK16map_session_dataiis',
    '_Z17clif_updatestatusR16map_session_data3_sp', '_Z17pc_show_questinfoP16map_session_data',
    '_Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz',
    '_Z17pc_can_trade_itemPK16map_session_datai',
    '_Z14pc_unequipitemP16map_session_dataii', '_Z12pc_equipitemP16map_session_datasib')


def require(ok, message):
    if not ok: raise AssertionError(message)


def sha(data): return hashlib.sha256(data).hexdigest()


def validate():
    from biosphere_callback_closure_audit import validate as validate_callback_closure
    callback_manifest = validate_callback_closure(ROOT)
    original = subprocess.check_output(['git', 'show', f'{BASE}:{NPC}'], cwd=ROOT)
    require(sha(original) == BASE_SHA, 'Pinned pre-fix NPC drift')
    current = (ROOT / NPC).read_bytes()
    old = original.decode()
    new = current.decode().replace('\r\n', '\n').replace('\r', '\n')
    marker = '// Changes exactly one synthetic enchant slot'
    require(reviewed_depth_baseline(old.split(marker)[0].encode()) ==
            reviewed_depth_baseline(new.split('// Crown transaction guard.')[0].encode()),
            'Changes outside approved helper/Abyss Researcher prefix')
    crown_baseline = subprocess.check_output(['git', 'show', f'{CROWN_BASE}:{NPC}'], cwd=ROOT)
    require(sha(crown_baseline) == CROWN_BASE_SHA, 'Pinned deployed crown baseline drift')
    # Other services have separate proofs. Only their exact reviewed fusion and
    # document repairs may differ; preserve every other crown/helper/access byte.
    require(reviewed_depth_baseline(crown_baseline.split(b'L_Convert:\n', 1)[0]) ==
            reviewed_depth_baseline(new.split('L_Convert:\n', 1)[0].encode()),
            'Previously deployed crown/helper/access source changed')
    match = re.search(r'setarray \.@crown_ids\[0\],([^;]+);', new)
    require(match and [int(x) for x in match[1].split(',')] == IDS, 'Exact supported 19-ID mapping changed')
    menu = 'Dragon Knight:Imperial Guard:Meister:Biolo:Shadow Cross:Abyss Chaser:Arch Mage:Elemental Master:Cardinal:Inquisitor:Windhawk:Troubadour / Trouvere:Shinkiro / Shiranui:Night Watch:Sky Emperor:Soul Ascetic:Hyper Novice:Spirit Handler:Cancel'
    require(menu + ':Alitea' in new and menu in old, 'Old menus/cancel must be preserved')
    for pattern in (r'setarray \.@chance\[1\],[^;]+;', r'setarray \.@cost\[1\],[^;]+;',
                    r'setarray \.@base\[0\],[^;]+;', r'setarray \.@stat_id\[0\],[^;]+;',
                    r'setarray \.@stat_weight\[0\],[^;]+;'):
        require(re.findall(pattern, old) == re.findall(pattern, new), 'Custom cost/probability drift')
    items = {}
    for record in renewal_records(ROOT, 'db/item_db.yml'): items.setdefault(record['Id'], {}).update(record)
    required = set(IDS + [1001552,1001553,1001555,1001556,25865,4365,400547])
    for base in [312719,312729,312739,312749,312759,312769,312779,312789,314249,314259]:
        required.update(range(base, base + 10))
    for base in [4700,4710,4740,4750]: required.update(range(base, base + 5))
    require(required <= items.keys(), 'Current effective transaction identity missing')
    for id in IDS:
        require(items[id]['Type'] == 'Armor' and items[id]['Slots'] == 1 and items[id].get('Weight', 0) == 0,
                f'Crown slot/weight/type changed: {id}')
    job_keys = ['Knight','Crusader','Blacksmith','Alchemist','Assassin','Rogue','Wizard','Sage',
                'Priest','Monk','Hunter','BardDancer','KagerouOboro','Rebellion','StarGladiator',
                'SoulLinker','SuperNovice','Spirit_Handler','Alitea']
    for id, job in zip(IDS, job_keys):
        require(items[id]['Jobs'] == {job: True}, f'Positive-fixture job eligibility metadata changed: {id}')
    require(items[400999]['AegisName'] == 'Time_DM_R_Crown_AT', 'Alitea identity mismatch')
    require(items[4365]['Type'] == 'Card' and items[4365]['Locations'].get('Head_Top') is True,
            'Fixture physical card must be legitimate headgear equipment')
    print('BIOSPHERE_STATIC_OK: exact 19 IDs, old Cancel 19, old economy and deployed crown prefix unchanged', flush=True)
    print('Required callback-closure gate PASS', flush=True)
    return original, current, [items[id] for id in sorted(required)], callback_manifest


def output_check(result):
    result.check_returncode()
    output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + '\n' + result.stderr)
    require(output.count('BIOSPHERE_NATIVE_OK') == 1, 'Missing/duplicate native completion marker')
    require(re.search(r'BIOSPHERE_NATIVE_OK cases=4152 assertions=\d+', output),
            'Native case/assertion coverage drift requires explicit review')
    require(output.count('BIOSPHERE_OLD_FAILURE_REPRODUCED: replacement crown modified and charged') == 1,
            'Missing pinned original source behavioral failure')
    require(output.count('Memory manager: No memory leaks found.') == 1, 'Missing clean allocator teardown')
    require(not re.search(r'\[(?:error|warning)\]|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|'
                          r'(?:invalid|double) free|Memory manager:(?! No memory leaks found\.)', output, re.I),
            'Native warning/error/allocator/sanitizer diagnostic')


def native(build, inputs, reuse=False):
    build = build.resolve()
    require(build != ROOT and ROOT not in build.parents, 'Generated artifacts must be outside repository')
    build.mkdir(parents=True, exist_ok=True)
    original, current, records, callback_manifest = inputs
    (build / 'before.txt').write_bytes(original)
    # Git's Windows checkout may use CRLF. Normalize only newline form for
    # source-region extraction; the receipt below retains the actual raw hash.
    (build / 'after.txt').write_bytes(current.replace(b'\r\n', b'\n').replace(b'\r', b'\n'))
    (build / 'items.yml').write_text(yaml.safe_dump({'Body': records}, sort_keys=False))
    reps = list(renewal_records(ROOT, 'db/reputation_db.yml'))
    if not reps:
        reps = list(renewal_records(ROOT, 'db/reputation.yml'))
    relevant = [r for r in reps if r.get('Id') in (6, 9)]
    require(len(relevant) == 2, 'Exact current reputation definitions required')
    (build / 'reputation.yml').write_text(yaml.safe_dump({'Body': relevant}, sort_keys=False))
    sources = ['src/map/pc.cpp', 'src/map/script.cpp', 'src/map/itemdb.cpp',
               'src/map/clif.cpp', 'src/common/malloc.cpp', 'tools/ci/biosphere_crown_transaction_test.cpp']
    tracked = sources + ['tools/ci/biosphere_crown_transaction_test.py', 'tools/ci/biosphere_regression_scope.py']
    hashes = {p: sha((ROOT / p).read_bytes()) for p in tracked}
    executable = build / 'biosphere_crown_transaction_test'
    if reuse:
        previous = json.loads((build / 'build.json').read_text())
        require(previous == hashes, 'Retained binary does not match every fresh production/driver source')
    else:
        sanitize = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
        flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + sanitize
        flags += ['-I' + p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
                  '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]
        def compile_one(source):
            output = build / (Path(source).stem + '.o')
            cache = json.loads((build / 'build.json').read_text()) if (build / 'build.json').is_file() else {}
            if cache.get(source) == hashes[source] and output.is_file():
                print('Reusing retained exact-source sanitizer object ' + source, flush=True)
                return output
            print('Fresh compile ' + source, flush=True)
            subprocess.run(flags + ['-c', source, '-o', str(output)], cwd=ROOT, check=True)
            return output
        with ThreadPoolExecutor(max_workers=2) as pool: fresh = list(pool.map(compile_one, sources))
        require(hashes == {p: sha((ROOT / p).read_bytes()) for p in tracked}, 'Sources changed during compilation')
        excluded = {p.name for p in fresh}
        old_objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
        libraries = [ROOT / p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a',
                                        '3rdparty/rapidyaml/obj/ryml.a')]
        require(old_objects and all(p.is_file() for p in libraries), 'Linux map support objects unavailable')
        command = ['g++'] + sanitize + ['-o', str(executable)] + [str(p) for p in fresh + old_objects + libraries]
        command += ['-Wl,--wrap=' + name for name in WRAPPERS]
        command += ['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
        subprocess.run(command, cwd=ROOT, check=True)
        (build / 'build.json').write_text(json.dumps(hashes, indent=2) + '\n')
    result = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=120)
    print(result.stdout, end=''); print(result.stderr, end='', file=sys.stderr)
    output_check(result)
    require((ROOT / NPC).read_bytes() == current, 'NPC changed during native proof')
    require(hashes == {p: sha((ROOT / p).read_bytes()) for p in tracked}, 'Compiled source changed during native proof')
    from biosphere_callback_closure_audit import validate as validate_callback_closure
    require(validate_callback_closure(ROOT) == callback_manifest, 'Callback content changed during native proof')
    receipt = {'result': 'PASS', 'npc_sha256': sha(current), 'original_npc_sha256': sha(original),
               'native_cases': 4152,
               'native_assertions': int(re.search(r'BIOSPHERE_NATIVE_OK cases=4152 assertions=(\d+)', result.stdout)[1]),
               'stat_buckets': 100000,
               'fresh_sources': hashes, 'network': 'kernel denied', 'asan_ubsan': True,
               'callback_manifest_sha256': sha(json.dumps(callback_manifest, sort_keys=True).encode()),
               'boundary': 'Explicit player/world/UI/registry/equip-status doubles; actual VM and inventory mutation'}
    (build / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-build-dir', type=Path)
    parser.add_argument('--reuse-build', action='store_true', help='Require exact production/driver hashes of retained binary')
    args = parser.parse_args()
    inputs = validate()
    if args.native_build_dir: native(args.native_build_dir, inputs, args.reuse_build)
