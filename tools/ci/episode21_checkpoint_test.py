#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  episode21_checkpoint_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode21_checkpoint_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Native episode finale helper: restore only an admitted entrant to safe stages.

Full production helper, control flow and instance-register reads are native.
Admission result, party ownership and final movement are explicit doubles.
"""
import argparse
import hashlib
from pathlib import Path
import re
import struct
import tempfile
import zlib

import episode21_encounter_flow_test as native
import instance_entry_native_test as setup
from episode_party_progression_test import scan_to, npc_body

ROOT = native.ROOT
BLOCK = r'\t// BEGIN EP21 FINALE ENTRANT CHECKPOINT\n.*?\t// END EP21 FINALE ENTRANT CHECKPOINT\n'
CASES = r'''
    const char* helper = "EP21_EnterInstance";
    const struct { const char* name; const char* var; int state; const char* map; int x,y; } routes[] = {
      {"Secret Altar","'sa_stage",3,"1@ep21a",184,34},
      {"Secret Altar","'sa_stage",4,"1@ep21a",184,34},
      {"Secret Altar","'sa_stage",5,"1@ep21a",184,34},
      {"Secret Altar","'sa_stage",6,"1@ep21a",182,97},
      {"Secret Altar","'sa_stage",7,"1@ep21a",182,97},
      {"Secret Altar","'sa_stage",8,"1@ep21a",126,155},
      {"Final Battle","'fb_stage",1,"1@ep21b",168,30},
      {"Final Battle","'fb_stage",3,"1@ep21b",168,30},
      {"Final Battle","'fb_stage",4,"1@ep21b",168,30},
      {"Final Battle","'fb_stage",10,"1@ep21b",168,170},
      {"Final Battle(Normal)","'fb_stage",10,"1@ep21b",168,170},
      {"Final Battle(Hard)","'fb_stage",20,"1@ep21b",168,170},
      {"Silent Sanctuary","'ss_stage",1,"1@twbs2",96,70},
      {"Silent Sanctuary","'ss_stage",2,"1@twbs2",96,70},
      {"Silent Sanctuary","'ss_stage",3,"1@twbs2",96,70},
    };
    for (const auto& route : routes) {
      reset(); requested_name=route.name; actual_name=route.name; entry_result=0; entries=0;
      stage(route.var,route.state); if (route.state == 6) stage("'sa_inner_open",1);
      finish(helper,1);
      check(entries == 1 && moves.size() == 1, "one admitted entrant gets one checkpoint warp");
      if (moves.size() == 1) check(moves[0].id == players[1]->id && moves[0].map == route.map && moves[0].x == route.x && moves[0].y == route.y, "checkpoint matches unlocked production route and only returning member");
      check(stage(route.var) == route.state && items.empty() && reputation.empty(), "recovery neither advances stage nor grants reward");
      moves.clear(); entry_result=3; finish(helper,1);
      check(moves.empty(), "failed admission never gets checkpoint warp");
      moves.clear(); entries=0; actual_name="Unrelated instance"; finish(helper,1);
      check(entries == 0 && moves.empty(), "another instance owner cannot use checkpoint");
    }
    for (const auto& route : routes) for (int unknown : {0,99}) {
      reset(); requested_name=route.name; actual_name=route.name; entry_result=0;
      stage(route.var,unknown); finish(helper,1);
      check(moves.empty(), "initial and unknown stages keep ordinary entrance");
    }
    reset(); requested_name="Secret Altar"; actual_name=requested_name; entry_result=0;
    stage("'sa_stage",6); finish(helper,1);
    check(moves.size() == 1 && moves[0].x == 184 && moves[0].y == 34, "unopened second altar door keeps entrant on its accessible side");
    finish("Door#ep21_sa_2",0); moves.clear();
    check(stage("'sa_inner_open") == 1, "actual second door records its opened state");
    finish(helper,1);
    check(moves.size() == 1 && moves[0].x == 182 && moves[0].y == 97, "opened second altar door restores entrant beyond it");
    reset(); requested_name="Secret Altar"; actual_name=requested_name; entry_result=0;
    stage("'sa_stage",8); missing_map=true; finish(helper,1);
    check(moves.empty(), "missing cloned map keeps successfully validated entrance");
'''


def fixtures(build, pre_fix=False):
    cells = {}
    for relative in ('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat'):
        path = ROOT / relative
        if not path.exists():
            continue
        data = path.read_bytes()
        offset = 8
        for _ in range(struct.unpack_from('<H', data, 4)[0]):
            name, width, height, size = struct.unpack_from('<12shhi', data, offset)
            offset += 20
            name = name.split(b'\0')[0].decode()
            cells.setdefault(name, (width, height, zlib.decompress(data[offset:offset+size])))
            offset += size
    for name, x, y in [('1@ep21a',184,34), ('1@ep21a',182,97), ('1@ep21a',126,155),
                       ('1@ep21b',168,30), ('1@ep21b',168,170), ('1@twbs2',96,70)]:
        width, height, data = cells[name]
        assert 0 <= x < width and 0 <= y < height and data[y*width+x] in (0,3,6), (name,x,y)
    source = (ROOT / 'npc/custom/episode21/BlackHairedBeast.txt').read_text()
    match = re.search(r'function\s+script\s+EP21_EnterInstance\s*\{', source)
    start = match.end()-1
    body = source[start:scan_to(source, start, '{', '}')+1]
    if pre_fix:
        body, count = re.subn(BLOCK, '', body, flags=re.S)
        assert count == 1
    (build/'checkpoint.script').write_text(body)
    door = '{\n' + npc_body(ROOT / 'npc/custom/episode21/SecretAltar.txt', 'Door#ep21_sa_2') + '\n}'
    (build/'door.script').write_text(door)
    header = 'static Case source_cases[] = {{"EP21_EnterInstance","checkpoint.script","",0,false},{"Door#ep21_sa_2","door.script","",0,false}};\nstatic int quest_ids[] = {0};\n'
    (build/'episode_cases.inc').write_text(header)
    return hashlib.sha256((header+body+door).encode()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--pre-fix', action='store_true')
    parser.add_argument('--prepare-only', action='store_true')
    args = parser.parse_args()
    prefix = native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix = prefix.replace('bool capacity = true;', 'bool capacity = true; int entries=0, entry_result=0; bool missing_map=false; std::string requested_name, actual_name;')
    prefix = prefix.replace('if (command == "instance_mapname"', '''if (command == "getarg") script_pushstrcopy(st,requested_name.c_str());
    else if (command == "is_party_leader") script_pushint(st,1);
    else if (command == "instance_live_info") script_pushstrcopy(st,actual_name.c_str());
    else if (command == "instance_create") script_pushint(st,1);
    else if (command == "instance_enter") { ++entries; script_pushint(st,entry_result); }
    else if (command == "instance_mapname" && missing_map) script_pushstrcopy(st,"");
    else if (command == "instance_mapname"''',1)
    prefix = prefix.replace('"mapannounce", "questinfo", "checkweight", "getitem", "getexp", "callfunc"};', '"mapannounce", "questinfo", "checkweight", "getitem", "getexp", "callfunc", "getarg", "is_party_leader", "instance_live_info", "instance_create", "instance_enter"};',1)
    main_body = setup.MAIN[:setup.MAIN.index('    const std::string gate')]
    main_body += CASES + setup.MAIN[setup.MAIN.index('    for (auto& player : players) boundary'):]
    main_body = main_body.replace('INSTANCE_ENTRY_RESULT', 'EP21_CHECKPOINT_RESULT')
    native.CPP = prefix + main_body
    native.fixtures = fixtures
    def run(directory):
        directory.mkdir(parents=True, exist_ok=True)
        native.run(directory.resolve(), False, args.pre_fix, args.prepare_only, completion_marker='EP21_CHECKPOINT_RESULT ')
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='ep21-checkpoint-') as temp:
            run(Path(temp))


if __name__ == '__main__':
    main()
