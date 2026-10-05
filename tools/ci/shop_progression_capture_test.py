"""Exercise shop achievement capture through the real achievement engine/VM.

Requires Linux map link objects; recompiles achievement, pc and script against
current headers. Player lookup and packets are explicit doubles. SQL, purchase
atomicity, autoequip and shop EXP are outside this test; networking is denied.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

import yaml

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from audit_item_acquisition import records, overlay


def run(build):
    build = build.resolve()
    build.mkdir(parents=True, exist_ok=True)
    effective = {}
    for _, row in records('db/achievement_db.yml'):
        overlay(effective.setdefault(row['Id'], {}), row)
    selected = []
    condition_calls = set()
    for row in effective.values():
        if row.get('Group') in ('Get_Item', 'Get_Zeny', 'Goal_Achieve'):
            row = dict(row)
            condition = str(row.get('Condition', ''))
            condition_calls.update(re.findall(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(', condition))
            assert ';' not in condition and '{' not in condition and '}' not in condition, \
                f'purchase achievement condition is not a read-only expression: {row.get("Id")}'
            # Claiming rewards is not part of completion. Keep titles; item
            # reward metadata/scripts are outside this focused native fixture.
            row.pop('Rewards', None)
            selected.append(row)
    assert sum(r.get('Group') == 'Get_Item' for r in selected) == 7
    assert sum(r.get('Group') == 'Get_Zeny' for r in selected) == 6
    assert sum(r.get('Group') == 'Goal_Achieve' for r in selected) == 20
    assert condition_calls <= {'readparam'}, \
        'purchase achievement capture requires a read-only condition allowlist: ' + repr(sorted(condition_calls))
    selected += [
        {'Id': 990001, 'Group': 'Get_Item', 'Name': 'VM context fixture',
         'Condition': 'ARG0 >= 100 && readparam(bStr) >= 90 && Zeny == 7654321',
         'Score': 1, 'Rewards': {'TitleId': 1023}},
        {'Id': 990002, 'Group': 'Spend_Zeny', 'Name': 'Deferred counter fixture',
         'Targets': [{'Id': 0, 'Count': 100}], 'Condition': 'true', 'Score': 1},
        {'Id': 990003, 'Group': 'Spend_Zeny', 'Name': 'Mutation policy fixture',
         'Targets': [{'Id': 0, 'Count': 100}], 'Condition': 'true', 'Score': 1},
        {'Id': 990004, 'Group': 'Taming', 'Name': 'Durable taming fixture',
         'Targets': [{'Id': 0, 'Count': 1}], 'Score': 1},
    ]
    (build / 'achievements.yml').write_text(yaml.safe_dump({'Body': selected}, sort_keys=False))
    levels = list(row for _, row in records('db/achievement_level_db.yml'))
    (build / 'levels.yml').write_text(yaml.safe_dump({'Body': levels}, sort_keys=False))

    sources = [ROOT / ('src/map/' + name + '.cpp') for name in ('achievement', 'pc', 'script')]
    sources += [ROOT / 'src/common/malloc.cpp', Path(__file__).with_suffix('.cpp')]
    includes = ['src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include']
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing']
    flags += sanitizer + ['-I' + str(ROOT / p) for p in includes] + ['-I/usr/include/mysql']
    headers = hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT / 'src').rglob('*.hpp')))).hexdigest()

    def compile_one(source):
        target = build / (source.stem + '.o')
        digest = hashlib.sha256(source.read_bytes() + repr(flags).encode() + headers.encode()).hexdigest()
        stamp = target.with_suffix('.sha')
        if not target.exists() or not stamp.exists() or stamp.read_text() != digest:
            print('Compile ' + str(source), flush=True)
            subprocess.run(flags + ['-c', str(source), '-o', str(target)], cwd=ROOT, check=True)
            stamp.write_text(digest)
        return target

    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh = list(pool.map(compile_one, sources))
    excluded = {p.name for p in fresh}
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
    assert objects, 'Build Linux map objects first'
    libraries = [ROOT / p for p in ('src/common/obj/common.a',
                                   '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    wrappers = ['main', '_Z9map_id2sdi', '_Z13map_charid2sdi', '_Z9map_id2ndi',
                '_Z11mapreg_initv', '_Z12mapreg_finalv', '_Z17npc_event_dequeueP16map_session_datab',
                '_Z9ShowErrorPKcz', '_Z23clif_achievement_updateP16map_session_dataPK11achievementi',
                '_Z25clif_achievement_list_allP16map_session_data', '_Z7set_eofi']
    binary = build / 'shop-progression-capture'
    bound = sources + objects + libraries + [Path(__file__), build / 'achievements.yml', build / 'levels.yml']
    def identity():
        return {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in bound}
    hashes = identity()
    report = {'passed': False, 'inputs_sha256': hashes, 'headers_sha256': headers,
              'scope': __doc__, 'network': 'kernel denied'}
    (build / 'receipt.json').write_text(json.dumps(report, indent=2))
    subprocess.run(['g++'] + sanitizer + ['-o', str(binary)] + [str(p) for p in fresh + objects + libraries] +
                   ['-Wl,--wrap=' + w for w in wrappers] +
                   ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm'],
                   cwd=ROOT, check=True)
    result = subprocess.run([str(binary), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=120)
    (build / 'native.log').write_text(result.stdout + result.stderr)
    print(result.stdout, end='')
    print(result.stderr, end='')
    result.check_returncode()
    assert 'SHOP_PROGRESSION_CAPTURE_OK' in result.stdout
    assert 'Memory manager: No memory leaks found.' in result.stdout + result.stderr
    assert identity() == hashes, 'Capture test inputs changed during validation'
    report['passed'] = True
    report['log_sha256'] = hashlib.sha256((result.stdout + result.stderr).encode()).hexdigest()
    (build / 'receipt.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='shop-progression-capture-') as directory:
            run(Path(directory))
