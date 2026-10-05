"""Actual Wise Old Woman VM and card commands, with isolated player boundaries.

Requires Linux map objects built from this checkout. Fresh UBSan pc/script
objects bind the transaction test to current sources. No server or SQL starts.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile
import yaml
from biosphere_crown_transaction_test import WRAPPERS
from audit_enchant_upgrades import renewal_records

ROOT = Path(__file__).resolve().parents[2]


def run(out, source):
    items = {}
    for row in renewal_records(ROOT, 'db/item_db.yml'):
        if row['Id'] in {400547, 4365, 1000, 715, 1202}:
            items.setdefault(row['Id'], {}).update(row)
    (out / 'card-items.yml').write_text(yaml.safe_dump({'Header': {'Type': 'ITEM_DB', 'Version': 3}, 'Body': list(items.values())}, sort_keys=False))
    (out / 'card-npc.txt').write_bytes(source.read_bytes())
    (out / 'globals.txt').write_bytes((ROOT / 'npc/other/Global_Functions.txt').read_bytes())
    prefix = (ROOT / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text().split('extern "C" int __wrap_main(', 1)[0]
    prefix = prefix.replace('check(attached->status.zeny==7654321,"no Zeny charge");', '')
    prefix = prefix.replace('extern "C" void quest(map_session_data*){}',
        'std::function<void()> quest_hook; bool native_quest=false; '
        'extern "C" void real_quest(map_session_data*) asm("__real__Z17pc_show_questinfoP16map_session_data"); '
        'extern "C" void quest(map_session_data* sd){if(quest_hook)quest_hook();if(native_quest)real_quest(sd);}')
    prefix = prefix.replace('extern "C" npc_data* npc_lookup(int32){return nullptr;}',
        'npc_data* quest_npc=nullptr; extern "C" npc_data* npc_lookup(int32){return quest_npc;}')
    combined = out / 'card-native.cpp'
    combined.write_text(prefix + (ROOT / 'tools/ci/card_removal_transaction_test.cpp').read_text())
    flags = ['g++', '-std=c++17', '-O0', '-g', '-fsanitize=undefined', '-fno-sanitize-recover=all', '-DPACKETVER=20260219']
    flags += ['-I' + str(ROOT / p) for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
    flags += ['-I/usr/include/mysql']
    fresh = []
    for name in ('pc', 'script'):
        target = out / (name + '.o')
        subprocess.run(flags + ['-c', str(ROOT / 'src/map' / (name + '.cpp')), '-o', str(target)], cwd=ROOT, check=True)
        fresh.append(target)
    objects = [p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in {'pc.o', 'script.o'}]
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    binary = out / 'card-test'
    subprocess.run(flags + [str(combined)] + [str(p) for p in fresh + objects + libraries] + ['-Wl,--wrap=' + x for x in WRAPPERS] + ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary), str(out), 'all'], cwd=ROOT, check=True, timeout=60)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=ROOT / 'npc/custom/card_remover.txt')
    parser.add_argument('--build-dir', type=Path)
    args = parser.parse_args()
    if args.build_dir:
        args.build_dir.mkdir(parents=True, exist_ok=True)
        run(args.build_dir.resolve(), args.source.resolve())
    else:
        with tempfile.TemporaryDirectory(prefix='card-removal-') as directory:
            run(Path(directory), args.source.resolve())
