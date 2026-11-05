"""Execute the arcade's actual functions and console events in the native VM.

Timers, positions, clicks, instance services, NPC visuals and achievements are
explicit doubles. Parser, control flow and registries execute natively. Static
checks bind the room geometry to the map cache and the client rows to the server.
"""
import argparse
from pathlib import Path
import re
import struct
import subprocess
import zlib

import yaml

import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to, npc_body

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'npc/custom/main_office/minigame_arcade.txt'


def map_cells(name):
    data = (ROOT / 'db/map_cache.dat').read_bytes()
    _, count = struct.unpack_from('<IH', data, 0)
    offset = 8
    for _ in range(count):
        raw, width, height, length = struct.unpack_from('<12shhi', data, offset)
        offset += 20
        if raw.split(b'\0', 1)[0].decode() == name:
            return width, height, zlib.decompress(data[offset:offset + length])
        offset += length
    raise AssertionError('Map is missing from the cache: ' + name)


def static_checks(source):
    width, height, cells = map_cells('guild_vs5')
    walk = lambda x, y: cells[x + y * width] in (0, 3)
    seen = {(61, 48)}
    queue = [(61, 48)]
    while queue:
        x, y = queue.pop()
        for point in ((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)):
            if point not in seen and 0 <= point[0] < width and 0 <= point[1] < height and walk(*point):
                seen.add(point)
                queue.append(point)
    entry = yaml.safe_load((ROOT / 'db/import/quality_lab_instance_db.yml').read_text())['Body']
    arcade = [row for row in entry if row['Name'] == 'PN Minigame Arcade']
    assert len(arcade) == 1 and arcade[0]['Enter']['Map'] == 'guild_vs5' and arcade[0]['NoMapFlag']
    assert (arcade[0]['Enter']['X'], arcade[0]['Enter']['Y']) in seen, 'Entry must be reachable'
    assert arcade[0]['TimeLimit'] >= 1800, 'A room must fit several games'
    points = [tuple(map(int, row)) for row in re.findall(r'^guild_vs5,(\d+),(\d+),4\tscript\t(?:Red|Blue|Green|Yellow) Pad', source, re.M)]
    assert len(points) == 4 and all(point in seen for point in points), 'Pads must be reachable'
    xs = re.search(r'setarray \.@cx\[0\],([\d,]+);', source)[1].split(',')
    ys = re.search(r'setarray \.@cy\[0\],([\d,]+);', source)[1].split(',')
    props = list(zip(map(int, xs), map(int, ys)))
    assert len(props) == 12 == len(set(props)) and all(point in seen for point in props), 'Props must be reachable'
    for sign_x, sign_y in ((-1, 1), (1, 1), (-1, -1), (1, -1)):
        quadrant = [(61 + sign_x * a, 48 + sign_y * b) for a in range(4, 13) for b in range(4, 13)]
        assert sum(point in seen for point in quadrant) >= 70, 'Each pad needs open floor'
    rows = yaml.safe_load((ROOT / 'db/import/arcade_achievement_db.yml').read_text())['Body']
    ids = {row['Id'] for row in rows}
    assert all(not row.get('Score') and 'Rewards' not in row and 'Group' not in row for row in rows), 'Arcade rows must award nothing'
    called = {300001, 300002, 300003, 300004, 300005} | {300000 + g * 10 + t for g in range(1, 5) for t in range(1, 4)}
    assert ids == called, 'Achievement rows must match what the script completes'
    client = (ROOT / 'client-patch/pn_arcade/arcade_achievements.lua').read_text()
    assert {int(value) for value in re.findall(r'pn_arcade\((\d+),', client)} == ids, 'Client rows must match the server'
    names = {row['Id']: row['Name'] for row in rows}
    for ident, title in re.findall(r'pn_arcade\((\d+), "([^"]+)"', client):
        assert names[int(ident)] == title, 'Client title differs: ' + title
    tiers = re.search(r'setarray \.@tier\[0\],([\d, ]+);', source)[1].replace(' ', '').split(',')
    for game, label in enumerate(('Echo Steps', 'Dice Pads', 'Floor is Lava', 'Poring Search')):
        for tier in (1, 2, 3):
            text = re.search(r'pn_arcade\(%d, "[^"]+", "([^"]+)"\)' % (300000 + (game + 1) * 10 + tier), client)[1]
            assert tiers[game * 4 + tier] in text and label in text, 'Client tier text differs: ' + text
    paying = r'^\s*(?:monster|areamonster|getitem\w*|getexp\w*|getnameditem|rentitem|Zeny|#\w+|\$\w+)\b'
    assert not re.search(paying, source, re.M) and 'Zeny+=' not in source and 'Zeny=' not in source, 'Arcade must not spawn or pay'
    # The script ternary evaluates both branches, so a builtin that fails on the
    # base map must never sit inside one. A fixture startup found this.
    assert not re.search(r'\?\s*instance_npcname|:\s*instance_npcname', source), 'instance_npcname inside a ternary'
    conf = (ROOT / 'npc/scripts_custom.conf').read_text()
    assert conf.count('npc: npc/custom/main_office/minigame_arcade.txt') == 1
    assert 'db/import/arcade_achievement_db.yml' in (ROOT / 'db/achievement_db.yml').read_text()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('/tmp/pn-minigame-arcade-test'))
    args = parser.parse_args()
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    source = SCRIPT.read_text()
    static_checks(source)
    rows = []
    for match in re.finditer(r'function\s+script\s+(\w+)\s*\{', source):
        start = match.end() - 1
        rows.append((match[1], source[start:scan_to(source, start, '{', '}') + 1], True))
    console = npc_body(SCRIPT, 'PN Arcade Console')
    rows.append(('console', '{' + console + '}', False))
    for label in re.findall(r'(?m)^(On\w+):', console):
        rows.append((label, '{goto ' + label + ';' + console + '}', False))
    for index, (name, body, _) in enumerate(rows):
        (build / (str(index) + '.txt')).write_text(body)
    includes = '\n'.join('load("%s","%s",%s);' % (name, index, 'true' if function else 'false') for index, (name, _, function) in enumerate(rows))
    prefix = '#include <tuple>\n#include <algorithm>\n' + native.CPP.split('extern "C" int __wrap_main', 1)[0]
    prefix = prefix.replace('#include "episode_cases.inc"', '')
    # Character and instance arrays must stay distinct in the registry adapter.
    prefix = prefix.replace('get_str(script_getvarid(reg))', 'regkey(reg)')
    prefix = prefix.replace('namespace {', 'namespace {\nstd::string regkey(int64 reg){return std::string(get_str(script_getvarid(reg)))+(script_getvaridx(reg)?"["+std::to_string(script_getvaridx(reg))+"]":"");}\n', 1)
    prefix = prefix.replace('registries[sd->id][regkey(reg)] = val; return true;', 'registries[sd->id][regkey(reg)] = val; script_array_update(&sd->regs,reg,val==0); return true;')
    world, main_code = (ROOT / 'tools/ci/minigame_arcade_test.cpp').read_text().split('// MAIN\n', 1)
    prefix = prefix.replace('int32 world(script_state* st) {', world + '\nint32 world(script_state* st) {')
    prefix = prefix.replace('    if (command == "instance_mapname"', '    if (arcade_world(st,command)) return SCRIPT_CMD_SUCCESS;\n    if (command == "instance_mapname"', 1)
    prefix = prefix.replace('"getexp", "callfunc"};', '"getexp","gettimetick","select","instance_live_info","instance_info","strcharinfo","getmapxy","getmapflag","isloggedin","readparam","achievementcomplete","setnpcdisplay","movenpc","getmonsterinfo","specialeffect","specialeffect2","stopnpctimer","initnpctimer","setnpctimer","setmapflag","setmapflagnosave","instance_create","instance_enter","instance_destroy","dispbottom"};')
    driver = build / 'driver.cpp'
    driver.write_text(prefix + main_code.replace('// LOAD', includes))
    objects = list((ROOT / 'src/map/obj').rglob('*.o'))
    if not objects:
        raise SystemExit('Build current map objects first')
    libs = [ROOT / path for path in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    wrappers = (*native.WRAPPERS, '_Z13map_charid2sdi', '_Z13mapreg_setregll', '_Z14mapreg_readregl')
    flags = ['g++', '-std=c++17', '-O0', '-g', '-fsanitize=address,undefined', '-fno-sanitize-recover=all', '-DPACKETVER=20260219']
    flags += ['-I' + str(ROOT / path) for path in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
    binary = build / 'test'
    command = flags + ['-I/usr/include/mysql', str(driver)] + [str(path) for path in objects + libs]
    command += ['-Wl,--wrap=' + name for name in wrappers]
    command += ['-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', str(binary)]
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(binary), str(build)], cwd=ROOT, text=True, capture_output=True, timeout=120)
    print(result.stdout + result.stderr, end='')
    result.check_returncode()
    assert 'MINIGAME_ARCADE_OK' in result.stdout
    assert 'Memory manager: No memory leaks found.' in result.stdout
    assert '[Error]' not in result.stdout + result.stderr


if __name__ == '__main__':
    main()
