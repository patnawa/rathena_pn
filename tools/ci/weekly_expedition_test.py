"""Execute the expedition's actual NPC events and records in the native VM.

World spawn, timer delivery, movement and UI are explicit doubles. Instance
registries, character array logic, parser and control flow execute natively.
The fixture denies network access and awards no items or currency.
"""
import argparse
from pathlib import Path
import re
import subprocess

import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to, npc_body

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, default=Path('/tmp/pn-weekly-expedition-test'))
    args = parser.parse_args()
    build = args.build_dir.resolve()
    build.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'npc/custom/main_office/weekly_expedition.txt').read_text()
    rows = []
    for match in re.finditer(r'function\s+script\s+(\w+)\s*\{', source):
        start = match.end() - 1
        rows.append((match[1], source[start:scan_to(source, start, '{', '}') + 1], True))
    console = npc_body(ROOT / 'npc/custom/main_office/weekly_expedition.txt', 'PN Expedition Console')
    rows.append(('console', '{' + console + '}', False))
    for label in re.findall(r'(?m)^(On\w+):', console):
        rows.append((label, '{goto ' + label + ';' + console + '}', False))
    for index, (name, body, _) in enumerate(rows):
        (build / (str(index) + '.txt')).write_text(body)
    includes = '\n'.join('load("%s","%s",%s);' % (name, index, 'true' if function else 'false') for index, (name, _, function) in enumerate(rows))
    prefix = '#include <tuple>\n' + native.CPP.split('extern "C" int __wrap_main', 1)[0]
    prefix = prefix.replace('#include "episode_cases.inc"', '')
    # Character arrays must remain distinct in the explicit registry adapter.
    prefix = prefix.replace('get_str(script_getvarid(reg))', 'regkey(reg)')
    prefix = prefix.replace('namespace {', 'namespace {\nstd::string regkey(int64 reg){return std::string(get_str(script_getvarid(reg)))+(script_getvaridx(reg)?"["+std::to_string(script_getvaridx(reg))+"]":"");}\n', 1)
    prefix = prefix.replace('registries[sd->id][regkey(reg)] = val; return true;', 'registries[sd->id][regkey(reg)] = val; script_array_update(&sd->regs,reg,val==0); return true;')
    extra = (ROOT / 'tools/ci/weekly_expedition_test.cpp').read_text()
    world, main_code = extra.split('// MAIN\n', 1)
    prefix = prefix.replace('int32 world(script_state* st) {', world + '\nint32 world(script_state* st) {')
    prefix = prefix.replace('    if (command == "instance_mapname"', '    if (expedition_world(st,command)) return SCRIPT_CMD_SUCCESS;\n    if (command == "instance_mapname"', 1)
    prefix = prefix.replace('"getexp", "callfunc"};', '"getexp","gettimetick","select","instance_live_info","strcharinfo","getmapxy","getmapflag","unitexists","setunitdata","setnpctimer","initnpctimer","stopnpctimer","instance_create","instance_enter","instance_destroy","setmapflag","setmapflagnosave","killmonsterall"};')
    # Spawn IDs belong to the actual script world, not a fabricated clear count.
    prefix = prefix.replace('else if (command == "monster") spawned += script_getnum(st, 7);', 'else if (command == "monster"){spawned+=script_getnum(st,7);const int gid=++next_gid;live.insert(gid);global_numbers[add_str("$@mobid")]=gid;}')
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
    result = subprocess.run([str(binary), str(build)], cwd=ROOT, text=True, capture_output=True, timeout=90)
    print(result.stdout + result.stderr, end='')
    result.check_returncode()
    assert 'WEEKLY_EXPEDITION_OK' in result.stdout
    assert 'Memory manager: No memory leaks found.' in result.stdout
    assert '[Error]' not in result.stdout + result.stderr


if __name__ == '__main__':
    main()
