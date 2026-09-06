#!/usr/bin/env python3
"""Read-only geometry/dependency checks plus isolated actual-VM menu regression."""
import argparse
from collections import deque
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib

from audit_initial_enchants import server_configuration
from audit_enchant_upgrades import renewal_records
from episode_party_progression_test import scan_to
from run_native_shadow_service_test import WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
NPC = ROOT / 'npc/custom/grademk_equipment_enchants.txt'
GROUPS = (63, 64, 147, 148, 165)


def enabled_scripts():
    visited, result = set(), set()
    def visit(relative):
        if relative in visited:
            return
        visited.add(relative)
        for line in (ROOT / relative).read_text(encoding='utf-8').splitlines():
            match = re.match(r'^\s*(import|npc):\s*([^\s]+)\s*(?://.*)?$', line)
            if not match:
                continue
            kind, path = match.groups()
            if kind == 'import':
                visit(path)
            else:
                if not (ROOT / path).is_file():
                    raise AssertionError(f'Enabled script missing: {path}')
                result.add(path)
    visit('npc/re/scripts_main.conf')
    return result


def verify_geometry():
    occupancy = []
    for relative in enabled_scripts():
        if ROOT / relative == NPC:
            continue
        for number, line in enumerate((ROOT / relative).read_text(encoding='utf-8', errors='replace').splitlines(), 1):
            match = re.match(r'^grademk,(\d+),(\d+),[^\t]*\t(.*)', line)
            if match:
                x, y, declaration = match.groups()
                occupancy.append({'x':int(x), 'y':int(y), 'declaration':declaration, 'file':relative, 'line':number})
    if any((r['x'], r['y']) == (28,184) for r in occupancy):
        raise AssertionError('New equipment counter collides with an existing enabled NPC')
    if not any((r['x'], r['y']) == (34,184) and 'Sratos#sratos' in r['declaration'] for r in occupancy):
        raise AssertionError('Previously rejected Sratos collision needs a fresh coordinate review')
    geometry = None
    for relative in ('db/import/map_cache.dat','db/re/map_cache.dat','db/map_cache.dat'):
        data = (ROOT / relative).read_bytes()
        count, = struct.unpack_from('<H',data,4)
        offset = 8
        for _ in range(count):
            name, width, height, length = struct.unpack_from('<12shhi',data,offset)
            offset += 20
            if not 0 < length <= len(data)-offset:
                raise AssertionError('Invalid map-cache record')
            if name.split(b'\0',1)[0] == b'grademk' and geometry is None:
                cells = zlib.decompress(data[offset:offset+length])
                if len(cells) != width*height:
                    raise AssertionError('Invalid Grademk geometry')
                geometry = (width,height,cells,relative,hashlib.sha256(data).hexdigest())
            offset += length
    if geometry is None:
        raise AssertionError('No effective Grademk geometry')
    width,height,cells,relative,digest = geometry
    def walk(x,y):
        return 0 <= x < width-1 and 0 <= y < height-1 and cells[x+y*width] in (0,2,3,4,6)
    for point in ((28,184),(28,181),(13,172),(38,177)):
        if not walk(*point):
            raise AssertionError(f'Blocked service/approach/entrance cell: {point}')
    occupied = {(r['x'],r['y']) for r in occupancy} | {(28,184)}
    seen, pending = {(13,172)}, deque([(13,172)])
    while pending:
        x,y = pending.popleft()
        for point in ((x-1,y),(x+1,y),(x,y-1),(x,y+1)):
            if point not in seen and point not in occupied and walk(*point):
                seen.add(point)
                pending.append(point)
    if (28,181) not in seen or (38,177) not in seen:
        raise AssertionError('Service approach is disconnected from actual workshop entrance')
    return {'npc':[28,184], 'approach':[28,181], 'entrance':[13,172], 'all_four_cells_gat':
            [cells[x+y*width] for x,y in ((28,184),(28,181),(13,172),(38,177))],
            'cache':relative, 'cache_sha256':digest, 'enabled_grademk_occupancy':occupancy,
            'approach_connected_to_entrance_avoiding_npcs':True}


def prepare(build):
    source = NPC.read_text(encoding='utf-8')
    if source.count('Equipment Enchanter#grademk') != 1 or not re.search(r'^grademk,28,184,4\tscript\tEquipment Enchanter#grademk\t4_M_REPAIR,\{',source,re.M):
        raise AssertionError('Reviewed NPC identity/position changed')
    code = re.sub(r'"(?:\\.|[^"\\])*"|//[^\n]*', '', source)
    if re.search(r'\b(?:getitem\w*|delitem\w*|setquest|changequest|setinstancevar|item_reform|item_enchantui|warp|Zeny|BaseLevel)\b',code):
        raise AssertionError('Entry-only service gained an unrelated grant/charge/gate/movement')
    start = source.index('{',source.index('Equipment Enchanter#grademk'))
    (build / 'body.script').write_text(source[start:scan_to(source,start,'{','}')+1],encoding='utf-8')
    groups = server_configuration(ROOT)
    effective = {}
    for record in renewal_records(ROOT,'db/item_db.yml'):
        effective.setdefault(record['Id'],{}).update(record)
    ids = {record['AegisName']:item_id for item_id,record in effective.items() if 'AegisName' in record}
    counts = {}
    for group_id in GROUPS:
        group = groups.get(group_id)
        if not group or not group['Targets'] or not group['Slots']:
            raise AssertionError(f'Existing backend group not ready: {group_id}')
        missing = group['Targets'] - ids.keys()
        if missing:
            raise AssertionError(f'Unresolved target identities in group {group_id}: {sorted(missing)}')
        counts[group_id] = len(group['Targets'])
    sample = [ids[sorted(groups[group]['Targets'])[0]] for group in (63,165)]
    (build / 'equipment_fixture.inc').write_text('static const uint64 GROUPS[] = {'+','.join(map(str,GROUPS))+'};\n'
        +'static const uint32 SAMPLE_ITEMS[] = {'+','.join(map(str,sample))+'};\n')
    print(json.dumps({'npc_sha256':hashlib.sha256(NPC.read_bytes()).hexdigest(), 'group_target_counts':counts,
                      'inventory_fixture_item_ids':sample, 'geometry':verify_geometry()},indent=2),flush=True)


def run(build,prepare_only=False):
    prepare(build)
    if prepare_only:
        return
    objects = sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name != 'script.o')
    libraries = [ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries):
        raise SystemExit('Build local native map-server support objects first')
    sanitizer = ['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fno-omit-frame-pointer']+sanitizer
    flags += ['-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')]+['-I'+str(build)]
    compiled = []
    for relative in ('src/map/script.cpp','src/common/malloc.cpp','tools/ci/grademk_equipment_service_test.cpp'):
        source = ROOT/relative
        print(f'Fresh compile {relative} SHA256={hashlib.sha256(source.read_bytes()).hexdigest()}',flush=True)
        target = build/(source.stem+'.o')
        subprocess.run(flags+['-c',str(source),'-o',str(target)],cwd=ROOT,check=True)
        compiled.append(target)
    executable = build/'grademk_equipment_service_test'
    command = ['g++']+sanitizer+['-o',str(executable)]+[str(p) for p in compiled+objects+libraries]
    command += ['-Wl,--wrap='+name for name in WRAPPERS]+['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
    subprocess.run(command,cwd=ROOT,check=True)
    result = subprocess.run([str(executable),str(build/'body.script')],cwd=ROOT,capture_output=True,text=True,timeout=60)
    print(result.stdout,end='',flush=True)
    print(result.stderr,end='',flush=True)
    output = re.sub(r'\x1b\[[0-9;]*m','',result.stdout+result.stderr)
    if 'Memory manager: No memory leaks found.' not in output or 'EQUIPMENT_SERVICE_NATIVE_RESULT cases=7 ' not in output:
        raise AssertionError('Native postconditions or explicit leak-free shutdown absent')
    if re.search(r'\[Error\]|\[Warning\]|AddressSanitizer|UndefinedBehaviorSanitizer|LeakSanitizer|runtime error:|Memory manager:.*(?:leak|corrupt|invalid|warning)',
                 output.replace('Memory manager: No memory leaks found.',''),re.I):
        raise AssertionError('Native error/allocator/sanitizer diagnostic')
    for choice in (1,2,3,4,5,6,255):
        if output.count(f'EQUIPMENT_SERVICE_CASE_PASS choice={choice}\n') != 1:
            raise AssertionError(f'Actual VM path did not complete exactly once: {choice}')
    result.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--prepare-only',action='store_true')
    args = parser.parse_args()
    if args.build_dir:
        directory = args.build_dir.resolve()
        directory.mkdir(parents=True,exist_ok=True)
        run(directory,args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-equipment-menu-') as temporary:
            run(Path(temporary),args.prepare_only)
