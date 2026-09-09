#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  gimli_checkpoint_reentry_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/gimli_checkpoint_reentry_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Native Gimli entrant-only checkpoint recovery regression.

Fresh script.cpp/instance.cpp/map.cpp/malloc.cpp execute the source-extracted full entry helper,
instance ownership/readiness validation, instance-variable references, cloned-map
resolution and warp builtin. Player/party/map-index lookup, final pc_setpos, map
creation and outbound UI are explicit isolated boundaries. No server/SQL/network.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import tempfile
import zlib

from audit_enchant_upgrades import renewal_records
from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'npc/custom/episode21/GimliInfiltration.txt'
BEFORE_HASH = 'f0c554a83ae877ea0fcd8e76cddda5fb987b7363c47fec26874c6cb499245ff6'
BLOCK = r'\t// BEGIN GIMLI ENTRANT CHECKPOINT\n.*?\t// END GIMLI ENTRANT CHECKPOINT\n'
WRAPPERS = (
    'main', '_Z9map_id2sdi', '_Z13map_charid2sdi', '_Z9map_id2ndi',
    '_Z12party_searchi', '_Z17mapindex_name2idxPKcS0_', '_Z18map_mapindex2mapidt', '_Z17map_mapname2mapidPKc',
    '_Z9pc_setposP16map_session_datatii8clr_type',
    '_Z15instance_createiPKc15e_instance_mode',
    '_Z14instance_enterP16map_session_dataiPKcss',
    '_Z11mapreg_initv', '_Z12mapreg_finalv', '_Z17npc_event_dequeueP16map_session_datab',
    '_Z14clif_scriptmesRK16map_session_datajPKc', '_Z16clif_scriptcloseRK16map_session_dataj',
    '_Z9ShowErrorPKcz',
)

CPP = r'''
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <tuple>
#include <vector>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <unistd.h>
#include "common/core.hpp"
#include "common/database.hpp"
#include "common/db.hpp"
#include "common/malloc.hpp"
#include "common/mapindex.hpp"
#include "common/timer.hpp"
#include "map/battle.hpp"
#include "map/clif.hpp"
#include "map/instance.hpp"
#include "map/npc.hpp"
#include "map/party.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"
struct Route { int stage; int map; int x,y; };
#include "gimli_routes.inc"
int32 map_readfromcache(struct map_data*,const char*,size_t,char*,size_t);
namespace {
constexpr int32 NPC = 99000003, TEST_PARTY = 99000004, INSTANCE = 42;
unsigned assertions = 0, failures = 0, errors = 0, cases = 0;
unsigned entered = 0, created = 0, closes = 0, suspended_closes = 0;
int missing_map = -1, creation_result = -4;
bool fail_position = false;
std::string current;
npc_data entrance{};
party_data group{};
std::vector<std::unique_ptr<map_session_data>> players;
struct Move { uint32 character; int map,x,y; bool success; };
std::vector<Move> moves;
script_code* helper = nullptr;
void check(bool ok, const char* message) {
    ++assertions;
    if (!ok) { ++failures; std::fprintf(stderr,"GIMLI FAIL [%s]: %s\n",current.c_str(),message); }
}
void boundary(bool ok, const char* message) {
    if (!ok) { std::fprintf(stderr,"GIMLI BOUNDARY ERROR: %s\n",message); std::exit(2); }
}
void deny_network() {
    sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data,nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,__NR_socket,0,1),
        BPF_STMT(BPF_RET | BPF_K,SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,__NR_connect,0,1),
        BPF_STMT(BPF_RET | BPF_K,SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,__NR_bind,0,1),
        BPF_STMT(BPF_RET | BPF_K,SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K,__NR_listen,0,1),
        BPF_STMT(BPF_RET | BPF_K,SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K,SECCOMP_RET_ALLOW),
    };
    sock_fprog filter{static_cast<unsigned short>(std::size(rules)),rules};
    boundary(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0) == 0,"no-new-privileges");
    boundary(prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&filter) == 0,"network-denial filter");
    check(syscall(SYS_socket,AF_INET,SOCK_STREAM,0) == -1 && errno == EPERM,"socket denied");
    check(syscall(SYS_connect,0,nullptr,0) == -1 && errno == EPERM,"connect denied");
    check(syscall(SYS_bind,0,nullptr,0) == -1 && errno == EPERM,"bind denied");
    check(syscall(SYS_listen,0,1) == -1 && errno == EPERM,"listen denied");
}
int64 instance_var(int id, const char* name) {
    return i64db_i64get(instances.at(id)->regs.vars,add_str(name));
}
void instance_var(int id, const char* name, int64 value) {
    i64db_i64put(instances.at(id)->regs.vars,add_str(name),value);
}
auto position(const map_session_data& player) { return std::make_tuple(player.m,player.mapindex,player.x,player.y); }
void reset(int stage = 8) {
    ++cases; entered = created = closes = suspended_closes = 0; moves.clear();
    missing_map = -1; fail_position = false; creation_result = -4;
    group = party_data{}; group.party.party_id = TEST_PARTY; group.instance_id = INSTANCE;
    auto instance = instances.at(INSTANCE);
    instance->id = 147; instance->state = INSTANCE_BUSY; instance->mode = IM_PARTY; instance->owner_id = TEST_PARTY;
    script_free_vars(instance->regs.vars); instance->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    instance_var(INSTANCE,"'gimli_stage",stage);
    instance_var(INSTANCE,"'gimli_wave_stage",991);
    for (unsigned i = 0; i < players.size(); ++i) {
        auto& player = *players[i]; boundary(!player.st,"previous dialogue completed");
        player.status.party_id = TEST_PARTY; player.status.zeny = 7654321;
        player.m = i ? 4 : 0; player.mapindex = map[player.m].index; player.x = i ? 77 : 20; player.y = i ? 88 : 30;
        group.data[i].sd = &player; group.party.member[i].leader = i == 0;
    }
}
void execute(unsigned actor = 0) {
    auto& player = *players.at(actor);
    run_script(helper,0,player.id,NPC);
    if (player.st) {
        boundary(player.st->state == CLOSE,"failure path uses native close suspension");
        ++suspended_closes;
        player.st->state = END; run_script_main(player.st);
    }
    boundary(!player.st,"helper terminates/detaches");
}
void unchanged(int stage) {
    check(instance_var(INSTANCE,"'gimli_stage") == stage,"shared encounter stage unchanged");
    check(instance_var(INSTANCE,"'gimli_wave_stage") == 991,"wave latch unchanged");
    check(instance_var(43,"'gimli_stage") == 19001,"other instance registers untouched");
    check(players[0]->status.zeny == 7654321 && players[1]->status.zeny == 7654321,"no zeny mutation");
}
}
extern "C" map_session_data* lookup(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* lookup(int32 id) { for (auto& p:players) if (p->id == id) return p.get(); return nullptr; }
extern "C" map_session_data* char_lookup(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* char_lookup(int32 id) { for (auto& p:players) if (p->status.char_id == id) return p.get(); return nullptr; }
extern "C" npc_data* npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* npc_lookup(int32 id) { return id == NPC ? &entrance : nullptr; }
extern "C" party_data* party_lookup(int32) asm("__wrap__Z12party_searchi");
extern "C" party_data* party_lookup(int32 id) { return id == TEST_PARTY ? &group : nullptr; }
extern "C" uint16 name_index(const char*,const char*) asm("__wrap__Z17mapindex_name2idxPKcS0_");
extern "C" uint16 name_index(const char* name,const char*) {
    for (int i=0;i<5;++i) if (i != missing_map && std::strcmp(name,map[i].name) == 0) return map[i].index;
    return 0;
}
extern "C" int16 index_map(uint16) asm("__wrap__Z18map_mapindex2mapidt");
extern "C" int16 index_map(uint16 index) { for (int i=0;i<5;++i) if (i != missing_map && map[i].index == index) return i; return -1; }
extern "C" int16 name_map(const char*) asm("__wrap__Z17map_mapname2mapidPKc");
extern "C" int16 name_map(const char* name) { return index_map(name_index(name,"fixture")); }
extern "C" e_setpos place(map_session_data*,uint16,int32,int32,clr_type) asm("__wrap__Z9pc_setposP16map_session_datatii8clr_type");
extern "C" e_setpos place(map_session_data* player,uint16 index,int32 x,int32 y,clr_type type) {
    const int m = index_map(index);
    boundary(player && m >= 0 && type == CLR_OUTSIGHT,"native movement request uses known map and player");
    check(map_getcell(m,x,y,CELL_CHKPASS),"every requested destination passes native effective-cache collision check");
    const bool success = !(fail_position && moves.empty());
    moves.push_back({player->status.char_id,m,x,y,success});
    if (!success) return SETPOS_MAPINDEX;
    player->m = m; player->mapindex = index; player->x = x; player->y = y;
    return SETPOS_OK;
}
extern "C" e_instance_enter actual_enter(map_session_data*,int32,const char*,int16,int16) asm("__real__Z14instance_enterP16map_session_dataiPKcss");
extern "C" e_instance_enter observed_enter(map_session_data*,int32,const char*,int16,int16) asm("__wrap__Z14instance_enterP16map_session_dataiPKcss");
extern "C" e_instance_enter observed_enter(map_session_data* sd,int32 id,const char* name,int16 x,int16 y) {
    ++entered; check(id == INSTANCE && std::strcmp(name,"Gimli Infiltration") == 0 && x == -1 && y == -1,
                     "full helper calls actual entry with explicit instance and original database coordinates");
    return actual_enter(sd,id,name,x,y);
}
extern "C" int32 create(int32,const char*,e_instance_mode) asm("__wrap__Z15instance_createiPKc15e_instance_mode");
extern "C" int32 create(int32 owner,const char* name,e_instance_mode mode) {
    ++created; boundary(owner == TEST_PARTY && std::strcmp(name,"Gimli Infiltration") == 0 && mode == IM_PARTY,"creation boundary arguments unchanged");
    if (creation_result > 0) { group.instance_id = INSTANCE; instance_var(INSTANCE,"'gimli_stage",0); }
    return creation_result;
}
extern "C" void reg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void reg_init() {}
extern "C" void reg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void reg_final() {}
extern "C" int32 dequeue(map_session_data*,bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 dequeue(map_session_data*,bool) { return 0; }
extern "C" void mes(const map_session_data&,uint32,const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void mes(const map_session_data&,uint32,const char*) {}
extern "C" void close_ui(const map_session_data&,uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void close_ui(const map_session_data&,uint32) { ++closes; }
extern "C" void error(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error(const char* format,...) { ++errors; va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args); }
extern "C" int __wrap_main(int argc,char** argv) {
    boundary(argc == 2,"source fixture path supplied"); deny_network();
    static char server[] = "gimli-checkpoint-test"; SERVER_NAME = server;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    battle_config.atcommand_disable_npc = 0;
    entrance.id = NPC; entrance.type = BL_NPC; entrance.m = 0; entrance.instance_id = 0;
    for (int i=0;i<5;++i) { map[i].index = 1000+i; map[i].m = i; map[i].instance_id = 0; }
    std::strcpy(map[0].name,"luna_sf2"); std::strcpy(map[1].name,"1@mdtem"); std::strcpy(map[2].name,"2@mdtem");
    // Exact selected original records in one-record aligned envelopes. The
    // existing whole-cache parser has a separate variable-offset alignment UB;
    // no sanitizer suppression or synthetic geometry is used here.
    const std::string helper_path(argv[1]);
    const auto directory = helper_path.substr(0,helper_path.find_last_of('/')+1);
    for (int m=1;m<=2;++m) {
        std::ifstream cache(directory+"map_"+std::to_string(m)+".cache",std::ios::binary);
        boundary(cache.good(),"exact selected cache record available");
        std::vector<char> bytes{std::istreambuf_iterator<char>(cache),std::istreambuf_iterator<char>()};
        char decoded[MAX_MAP_SIZE];
        boundary(map_readfromcache(&map[m],bytes.data(),bytes.size(),decoded,sizeof(decoded)) == 1,"native selected-record cache decoding succeeded");
    }
    boundary(map[1].cell && map[2].cell,"both actual source map geometries decoded");
    instance_generate_mapname(1,INSTANCE,map[3].name); instance_generate_mapname(2,INSTANCE,map[4].name);
    map[3].instance_id = map[4].instance_id = INSTANCE; map[3].instance_src_map = 1; map[4].instance_src_map = 2;
    for (int m=3;m<=4;++m) { map[m].cell = map[m-2].cell; map[m].xs = map[m-2].xs; map[m].ys = map[m-2].ys; }
    for (const auto& point:checkpoints) {
        check(map_getcell(point.map,point.x,point.y,CELL_CHKPASS),"exact source checkpoint passes native cache collision check");
        std::printf("GIMLI_NATIVE_CELL map=%s x=%d y=%d gat=%d pass=%d\n",map[point.map].name,point.x,point.y,
                    map_getcell(point.map,point.x,point.y,CELL_GETTYPE),map_getcell(point.map,point.x,point.y,CELL_CHKPASS));
    }
    auto record = std::make_shared<s_instance_db>(); record->id = 147; record->name = "Gimli Infiltration";
    record->enter.map = 1; record->enter.x = ENTRANCE_X; record->enter.y = ENTRANCE_Y; record->maplist = {2};
    instance_db.put(147,record);
    auto other = std::make_shared<s_instance_db>(); other->id = 149; other->name = "Final Battle"; instance_db.put(149,other);
    for (int id : {INSTANCE,43}) { auto live = std::make_shared<s_instance_data>(); live->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA); instances[id] = live; }
    instances.at(INSTANCE)->map = {{3,1},{4,2}}; instance_var(43,"'gimli_stage",19001);
    for (int i=0;i<2;++i) {
        auto player = std::make_unique<map_session_data>(); player->id = 99000010+i; player->type = BL_PC;
        player->status.account_id = player->id; player->status.char_id = 99000020+i;
        player->fd = 0; player->state.ignoretimeout = true; player->npc_idle_timer = INVALID_TIMER;
        players.emplace_back(std::move(player));
    }
    std::ifstream input(argv[1]); boundary(input.good(),"source-extracted helper exists");
    const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
    helper = parse_script(source.c_str(),"GimliInfiltration.txt:EP21_EnterGimli",1,0);
    boundary(helper != nullptr && errors == 0,"actual complete entry helper parses");
    for (const auto& route:routes) for (unsigned actor=0;actor<2;++actor) {
        reset(route.stage); current = "stage="+std::to_string(route.stage)+" actor="+std::to_string(actor);
        const auto other_position = position(*players[1-actor]); const auto inventory = players[actor]->inventory;
        execute(actor);
        check(entered == 1 && created == 0 && suspended_closes == 0 && closes == 1,"existing owner/member enters once and native end closes UI without suspending an error dialogue");
        check(moves.size() == (route.map == 0 ? 1u : 2u),"only database entry and optional entrant checkpoint move requested");
        boundary(!moves.empty(),"valid native entry reaches position boundary");
        check(moves.front().map == 3 && moves.front().x == ENTRANCE_X && moves.front().y == ENTRANCE_Y,"actual instance_enter first resolves database entrance");
        const auto& last = moves.back();
        check(last.map == (route.map ? route.map+2 : 3) && last.x == route.x && last.y == route.y,"entrant reaches exact last existing unlocked destination or entrance fallback");
        for (const auto& move:moves) check(move.character == players[actor]->status.char_id,"every movement targets only selected entrant");
        check(position(*players[1-actor]) == other_position,"other party member is never relocated");
        check(std::memcmp(&inventory,&players[actor]->inventory,sizeof(inventory)) == 0,"entrant inventory unchanged");
        unchanged(route.stage);
    }
    // All failure paths execute the same full helper and native entry validator.
    for (const std::string failure : {"no-party","member-without-instance","creation-failed","wrong-instance","not-ready","wrong-owner","entrance-map-missing","entrance-move-failed"}) {
        reset(); current = failure; unsigned actor = failure == "member-without-instance" ? 1 : 0;
        if (failure == "no-party") players[actor]->status.party_id = 0;
        else if (failure == "member-without-instance" || failure == "creation-failed") group.instance_id = 0;
        else if (failure == "wrong-instance") instances.at(INSTANCE)->id = 149;
        else if (failure == "not-ready") instances.at(INSTANCE)->state = INSTANCE_IDLE;
        else if (failure == "wrong-owner") instances.at(INSTANCE)->owner_id = TEST_PARTY+1;
        else if (failure == "entrance-map-missing") missing_map = 3;
        else if (failure == "entrance-move-failed") fail_position = true;
        auto first = position(*players[0]), second = position(*players[1]); execute(actor);
        check(closes == 1 && suspended_closes == 1,"rejection suspends original failure dialogue with native close");
        check(entered == (failure == "not-ready" || failure == "wrong-owner" || failure == "entrance-map-missing" || failure == "entrance-move-failed" ? 1u : 0u),"failure reaches only the correct native entry boundary");
        check(created == (failure == "creation-failed" ? 1u : 0u),"creation policy unchanged");
        check(moves.size() == (failure == "entrance-move-failed" ? 1u : 0u),"failed entry never attempts checkpoint routing");
        check(position(*players[0]) == first && position(*players[1]) == second,"failure leaves both players in place");
        unchanged(8);
    }
    reset(0); current = "new-instance-leader"; group.instance_id = 0; creation_result = INSTANCE; execute();
    check(created == 1 && entered == 1 && moves.size() == 1 && suspended_closes == 0 && closes == 1,"leader creation success retains entrance stage zero"); unchanged(0);
    reset(8); current = "checkpoint-map-unavailable"; missing_map = 4; execute();
    check(moves.size() == 1 && moves[0].map == 3 && suspended_closes == 0 && closes == 1,"empty native instance_mapname result retains successful entrance"); unchanged(8);
    check(errors == 0,"no native parser/instance errors");
    script_free_code(helper); players.clear();
    for (auto& item:instances) script_free_vars(item.second->regs.vars);
    instances.clear(); instance_db.clear(); record.reset(); other.reset();
    for (int m=1;m<=2;++m) { aFree(map[m].cell); map[m].cell = map[m+2].cell = nullptr; }
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("GIMLI_NATIVE_RESULT cases=%u assertions=%u failures=%u errors=%u\n",cases,assertions,failures,errors);
    return failures || errors ? 1 : 0;
}
'''


def verify_cells(points, build):
    """Independently decode first matching records; native code repeats the check."""
    maps = {}
    for relative in ('db/import/map_cache.dat', 'db/re/map_cache.dat', 'db/map_cache.dat'):
        path = ROOT / relative
        if not path.is_file():
            continue
        data = path.read_bytes()
        # The native reader uses map_count/record lengths, not the historical
        # file_size field (the existing base cache has a stale value).
        _, count = struct.unpack_from('<IH', data)
        offset = 8
        for _ in range(count):
            name, width, height, length = struct.unpack_from('<12shhi', data, offset)
            offset += 20
            if length < 6 or offset + length > len(data):
                raise AssertionError(f'Invalid map-cache record: {relative}')
            name = name.split(b'\0', 1)[0].decode('ascii')
            if name in ('1@mdtem', '2@mdtem') and name not in maps:
                cells = zlib.decompress(data[offset:offset+length])
                if width <= 0 or height <= 0 or len(cells) != width * height:
                    raise AssertionError(f'Invalid decoded map geometry: {name}')
                record = data[offset-20:offset+length]
                maps[name] = (width, height, cells, relative, hashlib.sha256(data).hexdigest(), record)
            offset += length
        if offset != len(data):
            raise AssertionError(f'Map-cache trailing data: {relative}')
    report = []
    for name, (_, _, _, _, _, record) in maps.items():
        # Native cache records are not individually padded/aligned. Preserve
        # every selected record byte, but isolate it at offset 8 so fresh UBSan
        # can test decoding/collision without the unrelated whole-file UB.
        (build / f'map_{name[0]}.cache').write_bytes(struct.pack('<IHH', 8+len(record), 1, 0)+record)
    for name, x, y in points:
        width, height, cells, relative, digest, record = maps[name]
        # Native map_getcellp deliberately rejects the last row and column.
        if not (0 <= x < width-1 and 0 <= y < height-1):
            raise AssertionError(f'Out-of-bounds native checkpoint: {name},{x},{y}')
        gat = cells[x+y*width]
        if gat not in (0, 2, 3, 4, 6):  # map_gat2cell walkable types
            raise AssertionError(f'Blocked checkpoint: {name},{x},{y}; GAT={gat}')
        report.append({'map':name, 'x':x, 'y':y, 'gat':gat, 'dimensions':[width,height],
                       'cache':relative, 'cache_sha256':digest, 'exact_record_sha256':hashlib.sha256(record).hexdigest()})
    return report


def prepare(build, pre_fix):
    source = SOURCE.read_text(encoding='utf-8')
    matches = re.findall(BLOCK, source, re.S)
    if len(matches) != 1:
        raise AssertionError('Expected exactly one isolated entrant checkpoint block')
    before = re.sub(BLOCK, '', source, flags=re.S)
    possible = (before.encode(), before.replace('\n', '\r\n').encode())
    if not any(hashlib.sha256(value).hexdigest() == BEFORE_HASH for value in possible):
        raise AssertionError('Code outside the checkpoint block changed from the reviewed current checkpoint')
    assigned = {int(value) for value in re.findall(r"'gimli_stage\s*=\s*(\d+);", before)}
    if assigned != set(range(23)) - {13, 16, 19}:
        raise AssertionError('Authoritative reachable stage assignments changed')
    milestones = {}
    for match in re.finditer(r'(?m)^[^\n]*\tscript(?:\(DISABLED\))?\t[^\n]*\{', before):
        start = match.end() - 1
        body = before[start:scan_to(before, start, '{', '}') + 1]
        travel = re.findall(r'instance_warpall "([12]@mdtem)",(\d+),(\d+),instance_id\(\);', body)
        if travel:
            changes = re.findall(r"'gimli_stage\s*=\s*(\d+);", body)
            if len(travel) != 1 or len(changes) != 1:
                raise AssertionError('Checkpoint source NPC requires a fresh route audit')
            milestones[int(changes[0])] = (travel[0][0], int(travel[0][1]), int(travel[0][2]))
    expected = {2:('1@mdtem',170,154),5:('1@mdtem',121,36),8:('2@mdtem',140,125),
                11:('2@mdtem',60,125),14:('1@mdtem',80,80),17:('2@mdtem',60,65),20:('1@mdtem',252,67)}
    if milestones != expected:
        raise AssertionError('Existing group relocation definitions changed')
    instances = {}
    for record in renewal_records(ROOT, 'db/instance_db.yml'):
        instances.setdefault(record['Id'], {}).update(record)
    instance = instances[147]
    if instance['Name'] != 'Gimli Infiltration' or instance['Enter'] != {'Map':'1@mdtem','X':266,'Y':174} or not instance.get('AdditionalMaps',{}).get('2@mdtem'):
        raise AssertionError('Effective instance identity/maps/entrance differ')
    points = [('1@mdtem', 266, 174)] + list(milestones.values())
    geometry = verify_cells(points, build)
    # Production route ranges must never introduce a combat/global write or a
    # collective warp. Other original code is preserved by the whole-file hash.
    if re.search(r"(?:instance_warpall|donpcevent|monster|getitem|changequest|setinstancevar|(?<!getinstancevar\()'gimli_stage\s*=)", matches[0]):
        raise AssertionError('Checkpoint block contains a forbidden shared/reward side effect')
    selected = before if pre_fix else source
    match = re.search(r'function\tscript\tEP21_EnterGimli\s*\{', selected)
    start = match.end() - 1
    helper = selected[start:scan_to(selected, start, '{', '}') + 1]
    (build / 'entry_helper.script').write_text(helper, encoding='utf-8')
    headers = ['static const int ENTRANCE_X=266, ENTRANCE_Y=174;', 'static Route routes[] = {']
    # Include every 0..22 value, all real stages, three internal holes, and four
    # other out-of-range values. Unknown states always use the database entrance.
    for stage in [-999, -1] + list(range(23)) + [23, 99, 2147483647]:
        earlier = [value for value in milestones if value <= stage]
        if stage in assigned and earlier:
            name, x, y = milestones[max(earlier)]
            m = int(name[0])
        else:
            m, x, y = 0, 266, 174
        headers.append(f'{{{stage},{m},{x},{y}}},')
    headers.append('};')
    headers.append('static Route checkpoints[] = {')
    headers.extend(f'{{0,{int(name[0])},{x},{y}}},' for name, x, y in points)
    headers.append('};')
    (build / 'gimli_routes.inc').write_text('\n'.join(headers))
    print(json.dumps({'source_sha256': BEFORE_HASH if pre_fix else hashlib.sha256(SOURCE.read_bytes()).hexdigest(),
                      'source_mode':'hash-exact previous helper' if pre_fix else 'current actual helper',
                      'reachable_stages':sorted(assigned),'source_derived_checkpoints':milestones,
                      'effective_cache_cells':geometry,
                      'outside_checkpoint_block_unchanged':True}, indent=2), flush=True)


def run(build, pre_fix, prepare_only=False):
    prepare(build, pre_fix)
    if prepare_only:
        return
    driver = build / 'gimli_driver.cpp'
    driver.write_text(CPP)
    fresh = [ROOT / 'src/map/script.cpp', ROOT / 'src/map/instance.cpp', ROOT / 'src/map/map.cpp', ROOT / 'src/common/malloc.cpp', driver]
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in ('script.o', 'instance.o', 'map.o'))
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries):
        raise SystemExit('Build the local Linux map-server first; native support objects are required')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    includes = ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql')
    flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219','-fno-strict-aliasing','-fno-omit-frame-pointer'] + sanitizer + ['-I'+p for p in includes]
    compiled = []
    for path in fresh:
        target = build / (path.stem + '.o')
        print('Fresh compile ' + str(path) + ' SHA256=' + hashlib.sha256(path.read_bytes()).hexdigest(), flush=True)
        subprocess.run(flags + ['-c',str(path),'-o',str(target)], cwd=ROOT, check=True)
        compiled.append(target)
    executable = build / 'gimli_checkpoint_reentry_test'
    command = ['g++'] + sanitizer + ['-o',str(executable)] + [str(p) for p in compiled + objects + libraries]
    command += ['-Wl,--wrap='+name for name in WRAPPERS]
    command += ['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm']
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(executable),str(build / 'entry_helper.script')], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(result.stdout, end='', flush=True)
    print(result.stderr, end='', flush=True)
    if 'GIMLI_NATIVE_RESULT ' not in result.stdout:
        raise AssertionError('Native postconditions not reached; this is not an ordinary regression failure')
    combined = re.sub(r'\x1b\[[0-9;]*m', '', result.stdout + result.stderr)
    if 'Memory manager: No memory leaks found.' not in combined:
        raise AssertionError('Native allocator did not explicitly confirm leak-free shutdown')
    if re.search(r'\[Error\]|\[Warning\]|AddressSanitizer|UndefinedBehaviorSanitizer|LeakSanitizer|runtime error:|Memory manager:.*(?:leak|corrupt|invalid|warning)',
                 combined.replace('Memory manager: No memory leaks found.', ''), re.I):
        raise AssertionError('Native error, sanitizer or allocator diagnostic detected')
    result.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--pre-fix',action='store_true',help='Use previous helper after exact whole-file hash verification; expected regression failure')
    parser.add_argument('--prepare-only',action='store_true',help='Only extract a source fixture and authoritative expected destinations')
    args = parser.parse_args()
    if args.build_dir:
        directory = args.build_dir.resolve()
        directory.mkdir(parents=True, exist_ok=True)
        run(directory,args.pre_fix,args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-gimli-checkpoint-') as temporary:
            run(Path(temporary),args.pre_fix,args.prepare_only)
