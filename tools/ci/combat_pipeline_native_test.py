#!/usr/bin/env python3
"""Native final damage, timing and actual costume equip/unequip regression.

Compiles current pc/script/itemdb/status/battle/skill translation units. The
fixture uses actual item/combo databases and pc_equipitem/pc_unequipitem; battle
calculations go through battle_calc_attack rather than stopping at cardfix.
Transport, registries and world lookup are explicit doubles. Damage delivery,
packet authentication, delayed damage and SQL persistence are outside this test.
Linux g++, PyYAML and existing unrelated native map objects are required.
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
from audit_enchant_upgrades import renewal_records
from biosphere_crown_transaction_test import WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
EQUIP_WRAPPERS = {'_Z14pc_unequipitemP16map_session_dataii', '_Z12pc_equipitemP16map_session_datasib'}
EXTRA_WRAPPERS = ('_Z9map_id2bli', '_Z17clif_equipitemackRK16map_session_datahii',
                  '_Z19clif_unequipitemackRK16map_session_datatib',
                  '_Z15clif_changelookP10block_listii', '_Z19clif_skillinfoblockRK16map_session_data')


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(build):
    # Independently described item bonuses are already sourced in
    # doc/costume_taiwan_reference_20260928.md and the established costume
    # regression. Do not derive the expected numeric values from Script fields.
    cases = [
        {'name': 'bare', 'expected': {'magic': 10000, 'melee': 25999, 'melee_rate': 0, 'long_rate': 0,
                                     'cast': 469, 'delay': 1400}},
        {'name': '23rd', 'garment': [300713],
         'expected': {'magic': 10300, 'melee': 26779, 'melee_rate': 3, 'long_rate': 3, 'cast': 351, 'delay': 1358}},
        {'name': '22nd', 'garment': [0, 313524],
         'expected': {'magic': 11100, 'melee': 28859, 'melee_rate': 11, 'long_rate': 11, 'cast': 469, 'delay': 1400}},
        {'name': 'anniversary-pair', 'garment': [300713, 313524],
         'expected': {'magic': 11400, 'melee': 29639, 'melee_rate': 14, 'long_rate': 14, 'cast': 351, 'delay': 1358}},
        {'name': 'range-set', 'upper': [310325], 'middle': [0, 310330], 'lower': [0, 0, 310326],
         'expected': {'magic': 10000, 'melee': 25999, 'melee_rate': 0, 'long_rate': 15, 'cast': 469, 'delay': 1400}},
        {'name': 'anniversary-range', 'garment': [300713], 'upper': [310325],
         'middle': [0, 310330], 'lower': [0, 0, 310326],
         'expected': {'magic': 10300, 'melee': 26779, 'melee_rate': 3, 'long_rate': 23, 'cast': 351, 'delay': 1358}},
    ]
    items = {}
    for row in renewal_records(ROOT, 'db/item_db.yml'):
        items.setdefault(row['Id'], {}).update(row)
    required = {value for test in cases for key in ('upper', 'middle', 'lower', 'garment')
                for value in test.get(key, []) if value}
    # Native skill parsing resolves real consumption/equipment identities even
    # though this controlled calculation does not consume a skill resource.
    costs = {cost['Item'] for row in renewal_records(ROOT, 'db/skill_db.yml')
             for cost in row.get('Requires', {}).get('ItemCost', [])}
    costs.update(name for row in renewal_records(ROOT, 'db/skill_db.yml')
                 for name in row.get('Requires', {}).get('Equipment', {}))
    required.update(value for value, row in items.items() if row.get('AegisName') in costs)
    names = {items[value]['AegisName'] for value in required}
    combos = []
    for row in renewal_records(ROOT, 'db/item_combos.yml'):
        groups = [group for group in row.get('Combos', []) if set(group['Combo']) <= names]
        if groups:
            combos.append({**row, 'Combos': groups})
    (build / 'items.yml').write_text(yaml.safe_dump({'Body': [items[value] for value in sorted(required)]}, sort_keys=False))
    (build / 'combos.yml').write_text(yaml.safe_dump({'Body': combos}, sort_keys=False))
    (build / 'cases.json').write_text(json.dumps(cases, indent=2) + '\n')


def checked_output(result):
    output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + result.stderr)
    result.check_returncode()
    assert output.count('COMBAT_PIPELINE_NATIVE_OK') == 1, 'Missing or repeated completion marker'
    assert output.count('Memory manager: No memory leaks found.') == 1, 'Missing clean allocator teardown'
    assert not re.search(r'\[(?:error|warning)\]|AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:|'
                         r'(?:invalid|double) free', output, re.I), 'Native diagnostics'
    counts = re.search(r'COMBAT_PIPELINE_NATIVE_OK cases=(\d+) assertions=(\d+) equip_acks=(\d+) unequip_acks=(\d+)', output)
    assert counts and (int(counts[1]), int(counts[3]), int(counts[4])) == (6, 624, 600), 'Expected cases/equip transitions missing'
    return dict(zip(('cases', 'assertions', 'equip_acks', 'unequip_acks'), map(int, counts.groups())))


def run(build):
    build = build.resolve()
    assert build != ROOT and ROOT not in build.parents, 'Build artifacts must stay outside repository'
    build.mkdir(parents=True, exist_ok=True)
    prepare(build)
    prefix_path = ROOT / 'tools/ci/biosphere_crown_transaction_test.cpp'
    driver_path = ROOT / 'tools/ci/combat_pipeline_native_test.cpp'
    combined = build / 'combat_pipeline_driver.cpp'
    prefix = prefix_path.read_text().split('extern "C" int __wrap_main(', 1)[0]
    combined.write_text(prefix + '\n' + driver_path.read_text())
    production = ['src/map/pc.cpp', 'src/map/script.cpp', 'src/map/itemdb.cpp',
                  'src/map/status.cpp', 'src/map/battle.cpp', 'src/map/skill.cpp',
                  'src/map/skills/mage/skill_factory_mage.cpp',
                  'src/map/skills/swordman/skill_factory_swordman.cpp',
                  'src/common/malloc.cpp']
    # Retain explicit source and reused-object hashes. A full server build is not
    # implied by freshly compiling the translation units this test exercises.
    inputs = production + [str(prefix_path.relative_to(ROOT)), str(driver_path.relative_to(ROOT)),
                           'tools/ci/combat_pipeline_native_test.py']
    inputs += [str(path.relative_to(ROOT)) for path in sorted((ROOT / 'src').rglob('*.hpp'))]
    # Factories include their per-skill .cpp files directly (unity build).
    # Compile and bind the containing translation units to avoid duplicate
    # definitions from linking standalone FireBolt/Bash objects beside them.
    for directory in ('mage', 'swordman'):
        inputs += [str(path.relative_to(ROOT)) for path in sorted((ROOT / 'src/map/skills' / directory).glob('*.cpp'))]
    inputs += [str(path.relative_to(ROOT)) for path in sorted((ROOT / 'db').rglob('*.yml'))]
    source_hashes = {path: sha(ROOT / path) for path in inputs}
    excluded = {Path(path).stem + '.o' for path in production}
    objects = sorted(path for path in (ROOT / 'src/map/obj').rglob('*.o') if path.name not in excluded)
    libraries = [ROOT / path for path in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                         '3rdparty/rapidyaml/obj/ryml.a')]
    assert objects and all(path.is_file() for path in libraries), 'Native support objects missing'
    support_hashes = {str(path.relative_to(ROOT)): sha(path) for path in objects + libraries}
    san = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + san
    flags += ['-I' + str(ROOT / path) for path in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                                                  '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
    flags += ['-I/usr/include/mysql']
    cache_inputs = hashlib.sha256(json.dumps(source_hashes, sort_keys=True).encode()).hexdigest()
    def compile_one(path):
        output = build / (path.stem + '.o')
        binding = hashlib.sha256(path.read_bytes() + repr(flags).encode() + cache_inputs.encode()).hexdigest()
        stamp = output.with_suffix('.sha')
        if not output.exists() or not stamp.exists() or stamp.read_text() != binding:
            print('Fresh compile ' + str(path), flush=True)
            subprocess.run(flags + ['-c', str(path), '-o', str(output)], cwd=ROOT, check=True)
            stamp.write_text(binding)
        return output
    fresh = [compile_one(combined)]
    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh += list(pool.map(compile_one, [ROOT / path for path in production]))
    assert source_hashes == {path: sha(ROOT / path) for path in inputs}, 'Source changed during compilation'
    executable = build / 'combat_pipeline_native_test'
    wrappers = [name for name in WRAPPERS if name not in EQUIP_WRAPPERS] + list(EXTRA_WRAPPERS)
    subprocess.run(['g++'] + san + ['-o', str(executable)] + [str(path) for path in fresh + objects + libraries]
                   + ['-Wl,--wrap=' + name for name in wrappers]
                   + ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm'],
                   cwd=ROOT, check=True)
    result = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=120)
    (build / 'native.log').write_text(result.stdout + result.stderr)
    print(result.stdout, end=''); print(result.stderr, end='', file=sys.stderr)
    counts = checked_output(result)
    # Deliberately alter only private item data, without changing the oracle.
    # This is harness sensitivity evidence, not a discovered game defect.
    original = (build / 'items.yml').read_text()
    mutations = [('magic', 'bMagicAtkEle,Ele_All,3', 'bMagicAtkEle,Ele_All,4',
                  'final magic damage agrees with independent item bonus'),
                 ('melee', 'bShortAtkRate,3', 'bShortAtkRate,4',
                  'final physical damage agrees with independent item bonus')]
    for name, old, new, message in mutations:
        data = yaml.safe_load(original)
        row = next(row for row in data['Body'] if row['Id'] == 300713)
        assert old in row['Script']
        row['Script'] = row['Script'].replace(old, new)
        (build / 'items.yml').write_text(yaml.safe_dump(data, sort_keys=False))
        try:
            mutant = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=120)
            (build / ('mutation-' + name + '.log')).write_text(mutant.stdout + mutant.stderr)
            assert mutant.returncode != 0 and message in mutant.stderr
        finally:
            (build / 'items.yml').write_text(original)
    receipt = {'result': 'PASS', 'native_counts': counts,
               'fresh_source_hashes': source_hashes, 'reused_support_object_hashes': support_hashes,
               'sanitizers': ['address', 'undefined'], 'network': 'kernel denied',
               'mutations_rejected': ['23rd magic bonus changed from 3% to 4%',
                                      '23rd short attack bonus changed from 3% to 4%'],
               'boundary': 'native equip/combo/status/timing/final damage calculation; delivery, live packets and SQL excluded'}
    (build / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='combat-pipeline-native-') as directory:
            run(Path(directory))
