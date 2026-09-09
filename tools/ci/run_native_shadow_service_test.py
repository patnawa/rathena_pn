#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  run_native_shadow_service_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/run_native_shadow_service_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Run the production Shadow Gear Enchanter body in an isolated actual script VM.

Fresh script.cpp, malloc.cpp and driver compilation; other existing Linux map
objects satisfy link dependencies only. No server startup, SQL, or live player.
The actual item_enchant builtin ends at an explicit outbound-UI request double.
"""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

from audit_enchant_upgrades import renewal_records
from audit_initial_enchants import server_configuration


ROOT = Path(__file__).resolve().parents[2]
WRAPPERS = (
    'main', '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z11mapreg_initv', '_Z12mapreg_finalv',
    '_Z17npc_event_dequeueP16map_session_datab', '_Z9ShowErrorPKcz',
    '_Z14clif_scriptmesRK16map_session_datajPKc',
    '_Z15clif_scriptnextRK16map_session_dataj',
    '_Z16clif_scriptcloseRK16map_session_dataj',
    '_Z15clif_scriptmenuR16map_session_datajPKc',
    '_Z9pc_setregP16map_session_datall',
    '_Z23clif_enchantwindow_openR16map_session_datam',
    '_Z10pc_payzenyP16map_session_datai15e_log_pick_typej',
    '_Z10pc_delitemP16map_session_dataiiis15e_log_pick_type',
)


def run(build, sanitizer):
    groups = server_configuration(ROOT)
    if 128 not in groups or len(groups[128]['Targets']) != 14:
        raise SystemExit('The effective Renewal imports must contain reviewed group 128 with 14 targets')
    if 166 not in groups or len(groups[166]['Targets']) != 4:
        raise SystemExit('The effective Renewal imports must contain reviewed group 166 with four targets')
    if any(group not in groups for group in range(70, 89)) or sum(len(groups[group]['Targets']) for group in range(70, 89)) != 74:
        raise SystemExit('The effective Renewal imports must contain Master groups 70 through 88 with 74 targets')
    if any(groups[group].get('Reset', {}).get('Enabled', False) for group in (*range(70, 89), 128, 166)):
        raise SystemExit('Shadow service no-reset description no longer matches effective recipes')
    expected = {24872: ('S_Full_Power_Armor', 'ShadowGear'), 1001253: ('S_Enchant_Essence', 'Etc')}
    items = {}
    for record in renewal_records(ROOT, 'db/item_db.yml'):
        if record['Id'] in expected:
            items.setdefault(record['Id'], {}).update(record)
    for item_id, metadata in expected.items():
        actual = (items.get(item_id, {}).get('AegisName'), items.get(item_id, {}).get('Type'))
        if actual != metadata:
            raise SystemExit(f'Synthetic inventory identity drift: {item_id}: {actual} != {metadata}')
    print('Effective groups 70-88/128/166, disabled resets and synthetic inventory identities verified; native fixture tests existence only', flush=True)
    npc = ROOT / 'npc/custom/grademk_services.txt'
    print('Production NPC source SHA256 ' + hashlib.sha256(npc.read_bytes()).hexdigest(), flush=True)
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name != 'script.o')
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                   '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries):
        raise SystemExit('Build the local Linux map-server first; required support objects are missing')
    includes = ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')
    sanitizer_flags = ['-fsanitize=' + sanitizer, '-fno-sanitize-recover=all']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing',
             '-fno-omit-frame-pointer'] + sanitizer_flags + ['-I' + p for p in includes]
    compiled = []
    for source in ('src/map/script.cpp', 'src/common/malloc.cpp', 'tools/ci/native_shadow_service_test.cpp'):
        target = build / (Path(source).stem + '.o')
        print('Compiling current ' + source, flush=True)
        print('SHA256 ' + hashlib.sha256((ROOT / source).read_bytes()).hexdigest(), flush=True)
        subprocess.run(flags + ['-c', source, '-o', str(target)], cwd=ROOT, check=True)
        compiled.append(target)
    executable = build / 'native_shadow_service_test'
    command = ['g++'] + sanitizer_flags + ['-o', str(executable)]
    command.extend(str(p) for p in compiled + objects + libraries)
    command.extend('-Wl,--wrap=' + name for name in WRAPPERS)
    command.extend(['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'])
    print('Linking isolated main and explicit world/UI doubles', flush=True)
    subprocess.run(command, cwd=ROOT, check=True)
    print('Executing actual NPC body with mandatory network denial', flush=True)
    result = subprocess.run([str(executable)], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(result.stdout, end='', flush=True)
    print(result.stderr, end='', flush=True)
    result.check_returncode()
    if re.search(r'AddressSanitizer|runtime error:|invalid.free|\[Error\]|\[Warning\]', result.stdout + result.stderr, re.I):
        raise SystemExit('Native sanitizer/allocator/error diagnostic detected')
    if 'Memory manager: No memory leaks found.' not in result.stdout:
        raise SystemExit('Native allocator did not confirm clean teardown')
    cases = [(2, 0), (1, 0), (3, 0), (255, 0)] + [(4, family) for family in range(1, 21)] + [(4, 255)]
    for main, sub in cases:
        marker = f'SHADOW_SERVICE_CASE_PASS: main={main} sub={sub}\n'
        if result.stdout.count(marker) != 1:
            raise SystemExit('Missing or duplicate actual-VM completion marker for ' + marker.strip())
    if 'PASS actual Shadow Gear Enchanter VM: 25 paths;' not in result.stdout:
        raise SystemExit('Native postconditions did not complete')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitizer', choices=('address', 'undefined', 'address,undefined'),
                        default='address,undefined')
    parser.add_argument('--build-dir', type=Path, help='Keep generated artifacts in this Linux directory')
    args = parser.parse_args()
    if args.build_dir:
        directory = args.build_dir.resolve()
        directory.mkdir(parents=True, exist_ok=True)
        run(directory, args.sanitizer)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-shadow-vm-') as temporary:
            run(Path(temporary), args.sanitizer)
