#!/usr/bin/env python3
"""Compile real storage cleanup functions with ASan/UBSan and boundary doubles.

This verifies command behavior and native deletion, not SQL persistence. Run on
Linux (including WSL) with GCC; all generated files live in a temporary directory.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    return source[start:source.index('\n}\n', start) + 3]


def run(commands=None):
    if commands is None:
        commands = (ROOT / 'src/map/atcommand.cpp').read_text(encoding='utf-8')
    storage = (ROOT / 'src/map/storage.cpp').read_text(encoding='utf-8')
    functions = [function(storage, 'int32 storage_delitem('),
                 function(storage, 'bool storage_guild_delitem('),
                 function(commands, 'ACMD_FUNC(clearstorage)'),
                 function(commands, 'ACMD_FUNC(cleargstorage)')]
    with tempfile.TemporaryDirectory(prefix='pn-storage-native-audit-') as directory:
        build = Path(directory)
        (build / 'storage_native_audit_functions.inc').write_text(
            '\n'.join(functions), encoding='utf-8')
        binary = build / 'storage-native-audit'
        subprocess.run([
            'g++', '-std=c++17', '-O1', '-g', '-DPACKETVER=20260219',
            '-fsanitize=address,undefined', '-fno-sanitize-recover=all',
            '-fno-omit-frame-pointer', '-I' + str(ROOT / 'src'), '-I' + str(build),
            str(ROOT / 'tools/ci/storage_native_audit_test.cpp'), '-o', str(binary),
        ], check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == '__main__':
    run()
