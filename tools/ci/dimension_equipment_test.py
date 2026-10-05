#!/usr/bin/env python3
"""Run EM equipment scripts in the native VM and inspect actual autocast bonuses.

Linux/WSL with existing map support objects is required. The fixture supplies
equipment, learned skills and elemental presence; no server or network is started.
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

ROOT = Path(__file__).resolve().parents[2]


def run(build, before=None, db_root=None):
    build = build.resolve()
    build.mkdir(parents=True, exist_ok=True)
    database_root = db_root.resolve() if db_root else ROOT
    items = {}
    for row in renewal_records(database_root, 'db/item_db.yml'):
        if row['Id'] in (540079, 540080, 540114, 400536, 570062, 580061, 400540, 620037, 400531):
            items.setdefault(row['Id'], {}).update(row)
    combos = list(renewal_records(database_root, 'db/item_combos.yml'))
    if before:
        # Replay captured production preimages through the same VM and assertions.
        original_items = yaml.load((before / 'db/re/item_db_equip.yml').read_text(), Loader=yaml.CSafeLoader)['Body']
        for row in original_items:
            if row['Id'] in items:
                items[row['Id']]['Script'] = row.get('Script', '')
        combos = yaml.load((before / 'db/re/item_combos.yml').read_text(), Loader=yaml.CSafeLoader)['Body']
    profiles = []
    for weapon, trigger in ((540079, 'EM_DIAMOND_STORM'), (540080, 'EM_TERRA_DRIVE'),
                            (540114, 'EM_DIAMOND_STORM')):
        names = {items[weapon]['AegisName'], items[400536]['AegisName']}
        scripts = [{'script': items[weapon]['Script'], 'slot': 'weapon'}]
        scripts += [{'script': row['Script'], 'slot': 'combo'} for row in combos
                    if any(set(c['Combo']) <= names for c in row.get('Combos', []))]
        profiles.append({'weapon': weapon, 'trigger': trigger, 'scripts': scripts})
    other_profiles = []
    for weapon, crown, kind in ((570062, 400540, 'performer'), (580061, 400540, 'performer'),
                                (620037, 400531, 'meister')):
        names = {items[weapon]['AegisName'], items[crown]['AegisName']}
        scripts = [{'script': row['Script']} for row in combos
                   if any(set(c['Combo']) <= names for c in row.get('Combos', []))]
        if len(scripts) != 1:
            raise AssertionError(f'Expected one active crown combo for {weapon}')
        other_profiles.append({'weapon': weapon, 'crown': crown, 'kind': kind, 'scripts': scripts})
    (build / 'input.json').write_text(json.dumps({'items': items, 'profiles': profiles, 'other_profiles': other_profiles}))
    names = {s for p in profiles + other_profiles for entry in p['scripts']
             for s in re.findall(r'"([A-Z][A-Z0-9_]+)"', entry['script'])}
    skills = [r for r in renewal_records(database_root, 'db/skill_db.yml') if r.get('Name') in names]
    (build / 'skills.yml').write_text(yaml.safe_dump({'Body': skills}, sort_keys=False))
    prefix = (ROOT / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    prefix = prefix.replace('BIOSPHERE TEST FAIL', 'DIMENSION TEST FAIL')
    for name in ('setreg', 'readreg', 'registry', 'named_registry', 'setstr', 'readstr'):
        prefix = re.sub(r'^extern "C"[^\n]*\b' + name + r'\([^\n]*\n', '', prefix, flags=re.M)
    rune = (ROOT / 'tools/ci/rune_tablet_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    combined = build / 'combined_dimension_equipment.cpp'
    combined.write_text(prefix + '\n' + rune + '\n' + (ROOT / 'tools/ci/dimension_equipment_test.cpp').read_text())
    sources = [ROOT / 'src/map' / (n + '.cpp') for n in ('pc', 'script', 'itemdb', 'clif')]
    sources += [ROOT / 'src/common/malloc.cpp', combined]
    san = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + san
    flags += ['-I' + str(ROOT / p) for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
              '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')] + ['-I/usr/include/mysql']
    headers = hashlib.sha256(b''.join(p.read_bytes() for p in sorted((ROOT / 'src').rglob('*.hpp')))).digest()

    def compile_one(source):
        target = build / (source.stem + '.o')
        digest = hashlib.sha256(source.read_bytes() + repr(flags).encode() + headers).hexdigest()
        receipt = target.with_suffix('.sha')
        if not target.exists() or not receipt.exists() or receipt.read_text() != digest:
            print('Compile ' + source.name, flush=True)
            subprocess.run(flags + ['-c', str(source), '-o', str(target)], cwd=ROOT, check=True)
            receipt.write_text(digest)
        return target

    with ThreadPoolExecutor(max_workers=2) as pool:
        fresh = list(pool.map(compile_one, sources))
    excluded = {p.name for p in fresh}
    support = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
    libs = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    wrappers = [w for w in WRAPPERS if not any(x in w for x in ('pc_setreg', 'pc_readreg'))]
    wrappers += ['_Z9map_id2bli']
    exe = build / 'dimension_equipment_test'
    subprocess.run(['g++'] + san + ['-o', str(exe)] + [str(p) for p in fresh + support + libs]
                   + ['-Wl,--wrap=' + w for w in wrappers]
                   + ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'], cwd=ROOT, check=True)
    subprocess.run([str(exe), str(build)], cwd=ROOT, check=True, timeout=60)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=ROOT.parent / 'dimension-equipment-proof')
    parser.add_argument('--before', type=Path, help='Replay captured original database files; expected to fail')
    parser.add_argument('--db-root', type=Path, help='Read effective databases from a captured candidate root')
    args = parser.parse_args()
    run(args.build_dir, args.before, args.db_root)
