#!/usr/bin/env python3
"""Fresh native map-cache parser, allocator and zlib-wrapper regression (Linux/WSL)."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]


def run(build, legacy=False, prepare_only=False, map_source=None):
    sources = {'map_source.cpp': map_source or ROOT / 'src/map/map.cpp',
               'malloc.cpp': ROOT / 'src/common/malloc.cpp',
               'grfio.cpp': ROOT / 'src/common/grfio.cpp',
               'driver.cpp': ROOT / 'tools/ci/map_cache_native_test.cpp'}
    for name, source in sources.items():
        data = source.read_bytes()
        (build / name).write_bytes(data)
        print(f'Fresh snapshot {source} SHA256={hashlib.sha256(data).hexdigest()}', flush=True)
    report = []
    oracle = bytearray()
    total = 0
    for relative in ('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat'):
        data = (ROOT / relative).read_bytes()
        declared, count = struct.unpack_from('<IH', data)
        oracle += struct.pack('<I', count)
        offset = 8
        names = set()
        for _ in range(count):
            name, width, height, length = struct.unpack_from('<12shhi', data, offset)
            offset += 20
            if not 0 < length <= len(data)-offset or b'\0' not in name or name[0] == 0:
                raise AssertionError(f'Invalid reference record in {relative}')
            key = name.split(b'\0', 1)[0]
            if key in names:
                raise AssertionError(f'Duplicate reference name in {relative}: {key!r}')
            names.add(key)
            decoded = zlib.decompress(data[offset:offset+length])
            if width <= 0 or height <= 0 or len(decoded) != width*height or any(gat > 6 for gat in decoded):
                raise AssertionError(f'Invalid reference geometry: {key!r}')
            oracle += name + struct.pack('<hhI', width, height, len(decoded)) + decoded
            offset += length
            total += 1
        if offset != len(data):
            raise AssertionError(f'Unexpected trailing reference bytes: {relative}')
        report.append({'cache':relative, 'records':count, 'actual_size':len(data), 'historical_size':declared,
                       'sha256':hashlib.sha256(data).hexdigest()})
    (build / 'geometry_oracle.bin').write_bytes(oracle)
    print(json.dumps({'cache_records':total, 'cache_provenance':report}, indent=2), flush=True)
    if prepare_only:
        return
    fresh = [build / name for name in ('driver.cpp', 'malloc.cpp', 'grfio.cpp')]
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name != 'map.o')
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries):
        raise SystemExit('Build the local Linux map-server support objects first')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    includes = ('src', 'src/map', 'src/common', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing',
             '-fno-omit-frame-pointer'] + sanitizer + ['-I'+p for p in includes]
    if legacy:
        flags += ['-DMAPCACHE_LEGACY_API']
    compiled = []
    for source in fresh:
        target = source.with_suffix('.o')
        subprocess.run(flags + ['-c', str(source), '-o', str(target)], cwd=ROOT, check=True)
        compiled.append(target)
    executable = build / 'map_cache_native_test'
    command = ['g++'] + sanitizer + ['-o', str(executable)] + [str(p) for p in compiled+objects+libraries]
    command += ['-Wl,--wrap=_Z9ShowErrorPKcz', '-Wl,--wrap=_Z11ShowWarningPKcz',
                '-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm']
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(executable), str(build / 'geometry_oracle.bin')], cwd=ROOT,
                            capture_output=True, text=True, timeout=120)
    print(result.stdout, end='', flush=True)
    print(result.stderr, end='', flush=True)
    combined = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout+result.stderr)
    if 'MAP_CACHE_NATIVE_RESULT ' not in combined or 'Memory manager: No memory leaks found.' not in combined:
        raise AssertionError('Native postconditions/leak-free shutdown absent')
    if re.search(r'\[Error\]|\[Warning\]|AddressSanitizer|UndefinedBehaviorSanitizer|LeakSanitizer|runtime error:|Memory manager:.*(?:leak|corrupt|invalid|warning)',
                 combined.replace('Memory manager: No memory leaks found.', ''), re.I):
        raise AssertionError('Native diagnostic or sanitizer failure')
    result.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--legacy', action='store_true', help='Reproduce old unbounded API failure before source repair')
    parser.add_argument('--map-source', type=Path, help='Explicit read-only map.cpp snapshot for a legacy reproducer; hash is reported')
    parser.add_argument('--prepare-only', action='store_true', help='Snapshot actual native sources and independent reference geometry')
    args = parser.parse_args()
    if args.build_dir:
        target = args.build_dir.resolve()
        target.mkdir(parents=True, exist_ok=True)
        run(target, args.legacy, args.prepare_only, args.map_source.resolve() if args.map_source else None)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-map-cache-') as temporary:
            run(Path(temporary), args.legacy, args.prepare_only, args.map_source.resolve() if args.map_source else None)
