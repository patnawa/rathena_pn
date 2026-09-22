#!/usr/bin/env python3
"""Run actual Fashion NPCs in an isolated Linux script VM with network denied.

Fresh script.cpp, allocator and driver; existing map objects satisfy other link
dependencies. UI, registry persistence and inventory deletion are explicit test
boundaries. No map-server startup, account database, or live player is used.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

from biosphere_crown_transaction_test import WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
NPC = ROOT / 'npc/custom/fashion_points/FashionPoints.txt'


def run(build, source):
    prefix = (ROOT / 'tools/ci/biosphere_crown_transaction_test.cpp').read_text()
    prefix = prefix.split('extern "C" int __wrap_main(', 1)[0]
    # The shared fixture stores registry values in a map. Fashion's actual
    # FP_LoadBox uses transient arrays, so mirror array membership as well.
    original = 'extern "C" bool setreg(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}'
    replacement = ('extern "C" bool setreg(map_session_data* sd,int64 key,int64 value){'
                   'nums[key]=value;if(script_getvaridx(key))script_array_update(&sd->regs,key,value==0);return true;}')
    if prefix.count(original) != 1:
        raise AssertionError('Shared registry fixture changed; review array boundary')
    driver = build / 'npc_audit_fashion_driver.cpp'
    driver.write_text(prefix.replace(original, replacement) +
                      (ROOT / 'tools/ci/npc_audit_fashion_test.cpp').read_text())
    sanitize = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-fno-omit-frame-pointer']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing'] + sanitize
    flags += ['-I' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
              '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')]
    sources = [ROOT / 'src/map/script.cpp', ROOT / 'src/common/malloc.cpp', driver]

    def compile_one(path):
        target = build / (path.stem + '.o')
        print('Compiling ' + str(path) + ' SHA256=' + hashlib.sha256(path.read_bytes()).hexdigest(), flush=True)
        subprocess.run(flags + ['-c', str(path), '-o', str(target)], cwd=ROOT, check=True)
        return target

    with ThreadPoolExecutor(max_workers=2) as pool:
        objects = list(pool.map(compile_one, sources))
    support = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name != 'script.o')
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                  '3rdparty/rapidyaml/obj/ryml.a')]
    if not support or any(not path.is_file() for path in libraries):
        raise SystemExit('Build local Linux map-server objects first')
    wrappers = (*WRAPPERS, '_Z14pc_setregistryP16map_session_datall',
                '_Z10pc_delitemP16map_session_dataiiis15e_log_pick_type')
    binary = build / 'npc_audit_fashion_test'
    subprocess.run(['g++'] + sanitize + ['-o', str(binary)] +
                   [str(p) for p in objects + support + libraries] +
                   ['-Wl,--wrap=' + name for name in wrappers] +
                   ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'],
                   cwd=ROOT, check=True)
    result = subprocess.run([str(binary), str(source)], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(result.stdout, end=''); print(result.stderr, end='')
    result.check_returncode()
    output = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + result.stderr)
    if not re.search(r'NPC_AUDIT_FASHION_OK cases=11 assertions=\d+', output):
        raise AssertionError('Missing native case coverage marker')
    if re.search(r'\[(?:Error|Warning)\]|AddressSanitizer|runtime error:|invalid.free', output, re.I):
        raise AssertionError('Native diagnostics must be clean')
    if 'Memory manager: No memory leaks found.' not in output:
        raise AssertionError('Missing clean native allocator teardown')
    # Reintroduce each audited defect independently. Execute the same real VM
    # proof again, so coverage must reject all three broken production variants.
    current = source.read_text()
    mutations = [
        ('gold-menu', 'Exchange Gold Points (1 for 1)', 'Exchange Gold Points 1:1',
         'Gold menu exposes exactly the three routed actions'),
        ('recovery-menu', '"Slot " + (.@slot+1) + " - "', '"Slot " + (.@slot+1) + ": "',
         'exactly one enchant option per recoverable slot'),
        ('costume-debit', 'if (!delitemidx(@inventorylist_idx[.@i],1))',
         'if ((delitemidx(@inventorylist_idx[.@i],1) * 0))',
         'Fashion Points require successful inventory debit'),
    ]
    for name, needle, replacement, expected in mutations:
        if current.count(needle) != 1:
            raise AssertionError('Regression mutation anchor drift: ' + name)
        altered = build / (name + '.txt')
        altered.write_text(current.replace(needle, replacement))
        failed = subprocess.run([str(binary), str(altered)], cwd=ROOT, capture_output=True, text=True, timeout=60)
        if failed.returncode == 0 or expected not in failed.stderr:
            raise AssertionError('Regression sensitivity failed for ' + name + '\n' + failed.stdout + failed.stderr)
        print('REGRESSION_REJECTED: ' + name, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source', type=Path, default=NPC, help='Alternate NPC source for regression sensitivity checks')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix='npc-audit-fashion-') as directory:
        run(Path(directory), args.source.resolve())
