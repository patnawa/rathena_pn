#!/usr/bin/env python3
"""Actual isolated script-VM regressions for three enabled Episode 21 scripts.

NPC bodies are extracted without rewriting. Fresh script.cpp, quest.cpp and
malloc.cpp execute native control flow, dialogue suspension, instance registers,
and quest mutations. World movement, spawn/event delivery, rewards, visibility,
access helpers and outbound UI are explicit doubles; no live server or SQL.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

import yaml
from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
FILES = ('GimliInfiltration.txt', 'MysteriousGhostShip.txt', 'BlackHairedBeast.txt')
PRE_FIX_HASHES = {
    'GimliInfiltration.txt': '04ebc9fcc713b20ebaa778f47c1840182525cf6fd97505ca9d04c6386f221edf',
    'MysteriousGhostShip.txt': '32036b3d303e75f6b3f34474ed9c5fa416387b5a6687f7d73bd43e8e7a788e0a',
    'BlackHairedBeast.txt': 'cde09b114a9eda4c2f055298155ef4a4233b7169b6a8c97245f738091e6ce152',
}
WRAPPERS = (
    'main', '_Z9map_id2sdi', '_Z9map_id2ndi', '_Z11mapreg_initv', '_Z12mapreg_finalv',
    '_Z17npc_event_dequeueP16map_session_datab', '_Z9ShowErrorPKcz',
    '_Z14clif_scriptmesRK16map_session_datajPKc',
    '_Z15clif_scriptnextRK16map_session_dataj', '_Z16clif_scriptcloseRK16map_session_dataj',
    '_Z15pc_readregistryPK16map_session_datal', '_Z14pc_setregistryP16map_session_datall',
    '_Z14clif_quest_addPK16map_session_dataPK5quest',
    '_Z17clif_quest_deletePK16map_session_datai',
    '_Z24clif_quest_update_statusPK16map_session_dataib',
    '_Z27clif_quest_update_objectivePK16map_session_dataPK5quest',
    '_Z17pc_show_questinfoP16map_session_data',
)

CPP = r'''
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
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
#include "common/timer.hpp"
#include "map/battle.hpp"
#include "map/clif.hpp"
#include "map/instance.hpp"
#include "map/npc.hpp"
#include "map/pc.hpp"
#include "map/quest.hpp"
#include "map/script.hpp"
script_data* push_val2(script_stack*, c_op, int64, reg_db*);
script_data* push_str(script_stack*, c_op, char*);
#define push_val(stack,type,val) push_val2(stack,type,val,nullptr)

// This exact public-linkage table layout is checked against script.cpp by the
// Python runner. Only explicitly listed world/service entries are rebound before
// do_init_script copies their pointers; parser/control/quest entries stay native.
struct script_function {
    int32 (*func)(struct script_state *st);
    const char *name;
    const char *arg;
    const char *deprecated;
};
extern script_function buildin_func[];
struct Case { const char* name; const char* path; const char* var; int stage; bool changes; };
#include "episode_cases.inc"
namespace {
constexpr int32 NPC = 99000003;
unsigned checks = 0, failures = 0, errors = 0, cases = 0;
std::string current, fixture_dir;
std::map<std::string, script_code*> codes;
std::vector<std::unique_ptr<map_session_data>> players;
map_session_data* attached = nullptr;
npc_data npc{};
std::set<std::string> disabled, enabled;
std::vector<std::string> events;
struct Move { int32 id; std::string map; int x, y; };
std::vector<Move> moves;
unsigned spawned = 0;
std::map<int32, std::map<std::string, int64>> registries;
std::map<int32, std::map<int, int64>> items;
std::map<int32, int64> reputation;
std::map<int32, std::pair<int64,int64>> experience;
bool capacity = true;
void check(bool value, const std::string& message) {
    ++checks;
    if (!value) { ++failures; std::fprintf(stderr, "EP21 FAIL [%s]: %s\n", current.c_str(), message.c_str()); }
}
void boundary(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "EP21 BOUNDARY ERROR: %s\n", message); std::exit(2); }
}
void deny_network() {
    sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_socket, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_connect, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_bind, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_listen, 0, 1),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog filter{static_cast<unsigned short>(std::size(rules)), rules};
    boundary(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no-new-privileges");
    boundary(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &filter) == 0, "network denial installed");
    check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "socket denied");
    check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "connect denied");
    check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "bind denied");
    check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "listen denied");
}
int64 stage(const char* name) { return i64db_i64get(instances.at(1)->regs.vars, add_str(name)); }
void stage(const char* name, int value) { i64db_i64put(instances.at(1)->regs.vars, add_str(name), value); }
void reset() {
    for (auto& player : players) {
        boundary(!player->st, "no abandoned dialogue between cases");
        if (player->quest_log) aFree(player->quest_log);
        player->quest_log = nullptr; player->num_quests = player->avail_quests = 0;
    }
    script_free_vars(instances.at(1)->regs.vars);
    instances.at(1)->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    disabled.clear(); enabled.clear(); events.clear(); moves.clear();
    registries.clear(); items.clear(); reputation.clear(); experience.clear();
    spawned = 0; capacity = true; ++cases;
}
int32 world(script_state* st) {
    const std::string command = script_getfuncname(st);
    if (command == "instance_mapname" || command == "instance_npcname") {
        script_pushstrcopy(st, script_getstr(st, 2));
    } else if (command == "instance_id") script_pushint(st, 1);
    else if (command == "strnpcinfo") {
        const auto name = current.substr(0,current.find("::"));
        script_pushstrcopy(st, name.c_str());
    }
    else if (command == "disablenpc" || command == "enablenpc") {
        const std::string target = script_hasdata(st, 2) ? script_getstr(st, 2) : current;
        if (command == "disablenpc") disabled.insert(target);
        else { enabled.insert(target); disabled.erase(target); }
    } else if (command == "warp" || command == "instance_warpall") {
        const std::string map = script_getstr(st, 2);
        int x = script_getnum(st, 3), y = script_getnum(st, 4);
        if (command == "warp") moves.push_back({st->rid, map, x, y});
        else for (const auto& player : players) moves.push_back({player->id, map, x, y});
    } else if (command == "donpcevent") events.emplace_back(script_getstr(st, 2));
    else if (command == "monster") spawned += script_getnum(st, 7);
    else if (command == "areamonster") spawned += script_getnum(st, 9);
    else if (command == "mapannounce" || command == "questinfo") { }
    else if (command == "checkweight") script_pushint(st, capacity);
    else if (command == "getitem") items[st->rid][script_getnum(st, 2)] += script_getnum(st, 3);
    else if (command == "getexp") {
        experience[st->rid].first += script_getnum64(st, 2);
        experience[st->rid].second += script_getnum64(st, 3);
    } else if (command == "callfunc") {
        const std::string function = script_getstr(st, 2);
        if (function == "EP21_GhostShipUnlocked") script_pushint(st, 1);
        else if (function == "EP21_MainComplete") script_pushint(st, 0);
        else if (function == "EP21_AddReputation") {
            reputation[st->rid] += script_getnum(st, 3); script_pushint(st, reputation[st->rid]);
        } else boundary(false, "unrecognized callfunc boundary");
    } else boundary(false, "unrecognized world builtin");
    return SCRIPT_CMD_SUCCESS;
}
void install_world_doubles() {
    std::set<std::string> names = {"instance_mapname", "instance_npcname", "instance_id", "strnpcinfo",
        "disablenpc", "enablenpc", "warp", "instance_warpall", "donpcevent", "monster", "areamonster",
        "mapannounce", "questinfo", "checkweight", "getitem", "getexp", "callfunc"};
    for (int i = 0; buildin_func[i].func; ++i)
        if (names.erase(buildin_func[i].name)) buildin_func[i].func = world;
    boundary(names.empty(), "all explicit world doubles matched native builtin entries");
}
void invoke(const std::string& name, unsigned player = 0, bool force = false) {
    current = name; attached = players.at(player).get();
    boundary(!attached->st, "player must be idle before invocation");
    if (!force && disabled.count(name)) return;
    run_script(codes.at(name), 0, attached->id, NPC);
}
void acknowledge(unsigned index) {
    attached = players.at(index).get();
    boundary(attached->st != nullptr, "dialogue acknowledgement has suspended script");
    const auto state = attached->st->state;
    boundary(state == STOP || state == CLOSE, "only actual Next/close acknowledgement states allowed");
    attached->st->state = state == CLOSE ? END : RUN;
    run_script_main(attached->st);
}
void to_close2(unsigned index) {
    auto* player = players.at(index).get();
    unsigned limit = 0;
    while (player->st && player->st->mes_active) {
        boundary(++limit < 10, "bounded real Next dialogue"); acknowledge(index);
    }
    boundary(player->st && player->st->state == STOP && !player->st->mes_active,
             "production body reaches actual close2 suspension");
}
void finish(const std::string& name, unsigned index = 0) {
    invoke(name, index);
    unsigned limit = 0;
    while (players.at(index)->st) { boundary(++limit < 15, "bounded dialogue"); acknowledge(index); }
}
void seed_quest(unsigned player, int id, bool completed = false) {
    boundary(quest_add(players.at(player).get(), id) == 0, "native fixture quest add");
    if (completed) boundary(quest_update_status(players.at(player).get(), id, Q_COMPLETE) == 0, "native fixture quest completion");
}
int q(unsigned player, int id) { return quest_check(players.at(player).get(), id, HAVEQUEST); }
}
extern "C" map_session_data* ep_lookup(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* ep_lookup(int32 id) { for (auto& p : players) if (p->id == id) return p.get(); return nullptr; }
extern "C" npc_data* ep_npc(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* ep_npc(int32 id) { return id == NPC ? &npc : nullptr; }
extern "C" void ep_reg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void ep_reg_init() { }
extern "C" void ep_reg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void ep_reg_final() { }
extern "C" int32 ep_dequeue(map_session_data*, bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 ep_dequeue(map_session_data*, bool) { return 0; }
extern "C" int64 ep_read(const map_session_data*, int64) asm("__wrap__Z15pc_readregistryPK16map_session_datal");
extern "C" int64 ep_read(const map_session_data* sd, int64 reg) { return registries[sd->id][get_str(script_getvarid(reg))]; }
extern "C" bool ep_write(map_session_data*, int64, int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool ep_write(map_session_data* sd, int64 reg, int64 val) { registries[sd->id][get_str(script_getvarid(reg))] = val; return true; }
extern "C" void ep_mes(const map_session_data&, uint32, const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void ep_mes(const map_session_data&, uint32, const char*) { }
extern "C" void ep_next(const map_session_data&, uint32) asm("__wrap__Z15clif_scriptnextRK16map_session_dataj");
extern "C" void ep_next(const map_session_data&, uint32) { }
extern "C" void ep_close(const map_session_data&, uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void ep_close(const map_session_data&, uint32) { }
extern "C" void ep_qadd(const map_session_data*, const struct quest*) asm("__wrap__Z14clif_quest_addPK16map_session_dataPK5quest");
extern "C" void ep_qadd(const map_session_data*, const struct quest*) { }
extern "C" void ep_qdel(const map_session_data*, int32) asm("__wrap__Z17clif_quest_deletePK16map_session_datai");
extern "C" void ep_qdel(const map_session_data*, int32) { }
extern "C" void ep_qstatus(const map_session_data*, int32, bool) asm("__wrap__Z24clif_quest_update_statusPK16map_session_dataib");
extern "C" void ep_qstatus(const map_session_data*, int32, bool) { }
extern "C" void ep_qobject(const map_session_data*, const struct quest*) asm("__wrap__Z27clif_quest_update_objectivePK16map_session_dataPK5quest");
extern "C" void ep_qobject(const map_session_data*, const struct quest*) { }
extern "C" void ep_qinfo(map_session_data*) asm("__wrap__Z17pc_show_questinfoP16map_session_data");
extern "C" void ep_qinfo(map_session_data*) { }
extern "C" void ep_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void ep_error(const char* format, ...) {
    ++errors; va_list args; va_start(args, format); std::vfprintf(stderr, format, args); va_end(args);
}
extern "C" int __wrap_main(int argc, char** argv) {
    boundary(argc == 2, "fixture directory supplied"); fixture_dir = argv[1];
    deny_network(); static char name[] = "episode21-encounter-flow-test"; SERVER_NAME = name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings = 0; battle_config.atcommand_disable_npc = 0;
    npc.id = NPC; npc.type = BL_NPC; npc.instance_id = 1;
    instances[1] = std::make_shared<s_instance_data>();
    instances.at(1)->state = INSTANCE_BUSY;
    instances.at(1)->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    for (int i = 0; i < 2; ++i) {
        auto p = std::make_unique<map_session_data>();
        p->id = 99000010 + i; p->type = BL_PC; p->status.account_id = p->id;
        p->status.char_id = p->id; p->fd = 0; p->state.ignoretimeout = true;
        p->npc_idle_timer = INVALID_TIMER; players.emplace_back(std::move(p));
    }
    for (int id : quest_ids) { auto entry = std::make_shared<s_quest_db>(); entry->id = id; quest_db.put(id, entry); }
    for (const auto& test : source_cases) {
        std::ifstream file(fixture_dir + "/" + test.path);
        boundary(file.good(), "source-extracted production body exists");
        const std::string source{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        auto* code = parse_script(source.c_str(), test.name, 1, 0);
        boundary(code != nullptr, "actual NPC body parses"); codes[test.name] = code;
    }
    boundary(errors == 0, "production bodies parse without errors");
    // Every shared dialogue transition: two real VM states suspend at close2.
    // The first progresses, another member may advance further, then the stale
    // continuation must neither rewind shared state nor issue world effects.
    for (const auto& test : source_cases) if (test.var[0] && test.changes) {
        reset(); current = test.name; stage(test.var, test.stage);
        invoke(test.name, 0); to_close2(0); invoke(test.name, 1); to_close2(1);
        check(stage(test.var) == test.stage, "close2 truly suspends before shared stage change");
        acknowledge(0); boundary(!players[0]->st, "first continuation ends");
        check(stage(test.var) > test.stage, "first continuation advances shared stage");
        int progressed = stage(test.var) + 1; stage(test.var, progressed);
        const auto before = std::make_tuple(events.size(), moves.size(), spawned, enabled, disabled);
        acknowledge(1); boundary(!players[1]->st, "stale continuation ends");
        check(stage(test.var) == progressed, "stale continuation does not rewind stage");
        check(before == std::make_tuple(events.size(), moves.size(), spawned, enabled, disabled), "stale continuation has no world effects");
    }
    // Four Gimli dialogues start waves without changing their shared stage.
    // A continuation held until after combat must not issue a stale event.
    for (const auto& test : source_cases) if (test.var[0] && !test.changes && std::string(test.name).find("#ep21gimli_") != std::string::npos) {
        reset(); stage(test.var,test.stage);
        invoke(test.name,0); to_close2(0); invoke(test.name,1); to_close2(1);
        acknowledge(0); check(events.size() == 1, "first same-stage dialogue requests one wave");
        stage(test.var,test.stage+1); acknowledge(1);
        check(events.size() == 1, "late same-stage dialogue cannot request a stale wave");
    }
    const struct { const char* name; int stage; unsigned count; } waves[] = {
        {"#EP21_Gimli_Control::OnBelieverWave",3,6}, {"#EP21_Gimli_Control::OnGuardWave",6,6},
        {"#EP21_Gimli_Control::OnInnerWave",9,7}, {"#EP21_Gimli_Control::OnSearchWave",14,5},
        {"#EP21_Gimli_Control::OnTanWave",17,4}, {"#EP21_Gimli_Control::OnFinalWave1",20,8},
    };
    for (const auto& wave : waves) {
        reset(); stage("'gimli_stage", wave.stage - 1); finish(wave.name);
        check(spawned == 0, "wave cannot start before its stage");
        stage("'gimli_stage", wave.stage); finish(wave.name);
        check(spawned == wave.count, "first event issues exact original wave count");
        finish(wave.name); check(spawned == wave.count, "repeated same-stage event cannot duplicate wave");
    }
    const struct { const char* name; int unlock; int x,y; unsigned events; } portals[] = {
        {"#EP21_GS_UpperPortal",7,47,27,1}, {"#EP21_GS_MiddlePortal",10,185,101,1},
        {"#EP21_GS_CaptainPortal",16,303,29,0}, {"#EP21_GS_CaptainExit",21,86,306,0},
    };
    for (const auto& portal : portals) {
        reset(); stage("'stage", portal.unlock - 1); finish(portal.name);
        check(moves.empty() && events.empty(), "closed portal cannot skip progression");
        stage("'stage", portal.unlock); finish(portal.name, 0);
        check(stage("'stage") == portal.unlock + 1, "first crossing initializes next stage once");
        check(!disabled.count(portal.name), "passage remains enabled for party and reconnects");
        check(events.size() == portal.events, "first crossing issues exact wave event count");
        for (int state = portal.unlock + 1; state <= 22; ++state) {
            stage("'stage", state); auto n = moves.size(); auto e = events.size();
            finish(portal.name, 1);
            check(moves.size() == n + 1, "another member or re-entry can cross open passage");
            if (moves.size() > n) {
                const auto& move = moves.back();
                check(move.id == players[1]->id && move.map == "1@wtgs" && move.x == portal.x && move.y == portal.y,
                      "crossing preserves original map and arrival coordinates");
            }
            check(stage("'stage") == state && events.size() == e, "repeat crossing neither rewinds nor respawns");
        }
    }
    const std::string tris = "Tris#ep21_finale";
    reset(); seed_quest(0,16818); finish(tris);
    check(q(0,16818) == Q_COMPLETE && q(0,23249) == Q_ACTIVE, "first report completes 16818 and starts repair");
    boundary(quest_change(players[0].get(),23249,23253) == 0, "native test advances through completed repair");
    finish(tris);
    check(q(0,23253) == -1 && q(0,23254) == Q_ACTIVE && q(0,23249) == -1, "completed 16818 no longer intercepts repaired tablet report");
    check(items[players[0]->id][1001618] == 50 && reputation[players[0]->id] == 50,
          "unchanged report reward issues exactly once");
    finish(tris);
    check(items[players[0]->id][1001618] == 50 && reputation[players[0]->id] == 50, "report retry cannot duplicate reward");
    for (int report : {23255,18345}) {
        reset(); seed_quest(0,16818,true); seed_quest(0,report); registries[players[0]->id]["EP21_ReportMask"] = 7;
        finish(tris); check(q(0, report == 23255 ? 18343 : 18346) == Q_ACTIVE,
                            "later campaign report advances despite completed 16818");
        check(q(0,23249) == -1, "later report does not restart tablet repair");
    }
    reset(); seed_quest(0,16818,true); finish(tris);
    check(q(0,23249) == Q_ACTIVE, "legacy completed Ghost Ship without later quests can start repair");
    reset(); seed_quest(0,16818,true); seed_quest(0,23253); capacity = false; finish(tris);
    check(q(0,23253) == Q_ACTIVE && q(0,23254) == -1 && items.empty() && reputation.empty(), "capacity rejection preserves report and reward");
    // Existing per-character finish guards remain usable by both members and
    // remain idempotent. No reward changes are part of these instance exits.
    const struct { const char* name; const char* var; int from,to; } rewards[] = {
        {"Tan#ep21gimli_finish","'gimli_stage",17764,17765},
        {"Maristella#ep21gs_finish","'stage",16816,16817},
        {"Maristella#ep21gs_finish","'stage",16821,16822},
    };
    for (const auto& reward : rewards) {
        reset(); stage(reward.var,22);
        for (unsigned p = 0; p < 2; ++p) { seed_quest(p,reward.from); finish(reward.name,p); check(q(p,reward.to) == Q_ACTIVE && q(p,reward.from) == -1, "each eligible member records its own completion"); }
        for (unsigned p = 0; p < 2; ++p) finish(reward.name,p);
        check(!disabled.count(reward.name) && items.empty() && reputation.empty(), "finish remains available without duplicate reward");
    }
    check(errors == 0, "no parser or native quest errors");
    for (auto& entry : codes) script_free_code(entry.second);
    reset(); players.clear(); attached = nullptr;
    script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("EP21_NATIVE_RESULT cases=%u assertions=%u failures=%u errors=%u\n",cases-1,checks,failures,errors);
    return failures || errors ? 1 : 0;
}
'''


def before_fix(filename, source):
    """Reverse only this pass in memory, requiring an exact recorded pre-fix hash.

    No Git operation or worktree rewrite is needed for the negative regression.
    A future unrelated source change fails this historical-checkpoint option.
    """
    source = re.sub(r"(\tclose2;\n)(?:\t// (?:Another member|close2 yields:)[^\n]*\n)?\tif \('(?:gimli_stage|stage) != \d+\) end;\n", r'\1', source)
    if filename == 'GimliInfiltration.txt':
        source = re.sub(r"\t'gimli_wave_stage = \d+;\n", '', source)
        source = re.sub(r"(if \('gimli_stage != \d+) \|\| 'gimli_wave_stage == \d+(\) end;)", r'\1\2', source)
    elif filename == 'MysteriousGhostShip.txt':
        for name, unlock, action, x, y in (
            ('UpperPortal', 7, 'donpcevent instance_npcname("#EP21_GhostShip_Control")+"::OnMiddle";',47,27),
            ('MiddlePortal',10, 'donpcevent instance_npcname("#EP21_GhostShip_Control")+"::OnLower1";',185,101),
            ('CaptainPortal',16, 'enablenpc instance_npcname("Ghost Ship Captain#ep21gs_battle");',303,29),
            ('CaptainExit',21, 'enablenpc instance_npcname("Maristella#ep21gs_finish");',86,306),
        ):
            match = re.search(r'(?m)^[^\n]*\tscript\(DISABLED\)\t#EP21_GS_' + name + r'\t[^\n]*\{', source)
            start = match.end() - 1
            stop = scan_to(source, start, '{', '}') + 1
            original = "{\n\tif ('stage != %d) end;\n\t'stage = %d;\n\tdisablenpc();\n\t%s\n\twarp instance_mapname(\"1@wtgs\"),%d,%d;\n\tend;\n}" % (unlock,unlock+1,action,x,y)
            source = source[:start] + original + source[stop:]
    else:
        source = source.replace(
            '\t// Completed quests remain in the log. Only the active arrival report\n'
            '\t// should intercept the later tablet, eyewitness, and serpent reports.\n'
            '\tif (isbegin_quest(16818) == 1) {\n\t\tcompletequest 16818;',
            '\tif (isbegin_quest(16818)) {\n\t\tif (isbegin_quest(16818) == 1)\n\t\t\tcompletequest 16818;')
    possibilities = [source.encode('utf-8'), source.replace('\n', '\r\n').encode('utf-8')]
    if not any(hashlib.sha256(value).hexdigest() == PRE_FIX_HASHES[filename] for value in possibilities):
        raise AssertionError('Reconstructed pre-fix source does not match the recorded original hash: ' + filename)
    return source


def fixtures(build, pre_fix=False):
    result, sources = [], {}
    enabled = (ROOT / 'npc/scripts_custom.conf').read_text()
    for filename in FILES:
        path = ROOT / 'npc/custom/episode21' / filename
        source = path.read_text(encoding='utf-8')
        if len(re.findall(r'(?m)^npc:\s*npc/custom/episode21/' + re.escape(filename) + r'\s*$', enabled)) != 1:
            raise AssertionError('Expected one active script import: ' + filename)
        original = before_fix(filename, source)
        # Prove this pass did not change encounter composition, movements, entry
        # policy calls, or the numerical reward/capacity commands.
        protected = r'(?m)^\s*(?:monster|areamonster|warp|instance_warpall|instance_create|instance_enter|getitem|getexp|callfunc "EP21_AddReputation"|if \(!checkweight)[^\n]*'
        if [line.strip() for line in re.findall(protected, source)] != [line.strip() for line in re.findall(protected, original)]:
            raise AssertionError('Existing spawn, travel, entry, or reward declarations changed: ' + filename)
        sources[filename] = hashlib.sha256(path.read_bytes()).hexdigest()
        if pre_fix:
            source = original
            sources[filename] = PRE_FIX_HASHES[filename]
        for match in re.finditer(r'(?m)^[^\n]*\tscript(?:\(DISABLED\))?\t([^\t]+)\t[^\n]*\{', source):
            name = match[1]
            start = match.end() - 1
            body = source[start:scan_to(source, start, '{', '}') + 1]
            guard = re.match(r"\{\s*if \(('(?:gimli_stage|stage)) != (\d+)\) end;", body)
            changing = bool(guard and 'close2;' in body and re.search(re.escape(guard[1]) + r'\s*=\s*\d+', body))
            if changing or guard and 'close2;' in body and 'donpcevent' in body or name in ('Tris#ep21_finale', 'Tan#ep21gimli_finish', 'Maristella#ep21gs_finish') or name.startswith('#EP21_GS_') and 'Portal' in name or name == '#EP21_GS_CaptainExit':
                result.append((name, body, guard[1] if guard else '', int(guard[2]) if guard else 0, changing))
            if name == '#EP21_Gimli_Control':
                for label in re.finditer(r'(?m)^(On(?:BelieverWave|GuardWave|InnerWave|SearchWave|TanWave|FinalWave1)):\s*\n', body):
                    next_label = re.search(r'(?m)^\w+:', body[label.end():])
                    end = label.end() + next_label.start() if next_label else len(body) - 1
                    result.append((name + '::' + label[1], '{\n' + body[label.end():end] + '\n}', '', 0, False))
    if len({entry[0] for entry in result}) != len(result):
        raise AssertionError('Duplicate selected fixture name')
    if sum(entry[4] for entry in result) != 21:
        raise AssertionError('Expected 21 existing shared dialogue transitions; audit changed source before updating: ' + repr([(entry[0], entry[4]) for entry in result]))
    records = yaml.safe_load((ROOT / 'db/import/quest_db.yml').read_text())['Body']
    known = {record['Id'] for record in records}
    qids = {16818, 23249, 23253, 23254, 23255, 18343, 18345, 18346,17764,17765,16816,16817,16821,16822}
    if not qids <= known:
        raise AssertionError('A tested quest is absent from the actual import')
    header = ['static Case source_cases[] = {']
    for index, (name, body, var, stage, changing) in enumerate(result):
        path = f'body_{index}.script'
        (build / path).write_text(body, encoding='utf-8')
        header.append('{' + ','.join((json.dumps(name), json.dumps(path), json.dumps(var), str(stage), str(changing).lower())) + '},')
    header += ['};', 'static int quest_ids[] = {' + ','.join(map(str, sorted(qids))) + '};']
    (build / 'episode_cases.inc').write_text('\n'.join(header), encoding='utf-8')
    print(json.dumps({'production_sha256': sources, 'selected_bodies': len(result), 'native_dialogue_race_cases': 21}, indent=2), flush=True)
    return hashlib.sha256((build / 'episode_cases.inc').read_bytes()).hexdigest()


def run(build, reuse, pre_fix=False, prepare_only=False):
    shape = fixtures(build, pre_fix)
    if prepare_only:
        return
    # The only copied core declaration is checked byte-for-byte modulo whitespace.
    core = (ROOT / 'src/map/script.cpp').read_text()
    declaration = re.search(r'typedef struct script_function\s*\{([^}]+)\}', core)[1]
    expected = 'int32 (*func)(struct script_state *st); const char *name; const char *arg; const char *deprecated;'
    if re.sub(r'\s+', '', declaration) != re.sub(r'\s+', '', expected):
        raise AssertionError('Native builtin-table layout changed; review harness declaration')
    (build / 'episode21_driver.cpp').write_text(CPP, encoding='utf-8')
    fresh = [ROOT / 'src/map/script.cpp', ROOT / 'src/map/quest.cpp', ROOT / 'src/common/malloc.cpp', build / 'episode21_driver.cpp']
    fingerprint = {str(path): hashlib.sha256(path.read_bytes()).hexdigest() for path in fresh}
    fingerprint['fixture_metadata'] = shape
    stamp, executable = build / 'compiled.json', build / 'episode21_encounter_flow_test'
    if reuse:
        if not executable.is_file() or json.loads(stamp.read_text()) != fingerprint:
            raise AssertionError('Cannot reuse build: source or fixture metadata changed')
    else:
        objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in ('script.o', 'quest.o'))
        libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        if not objects or any(not path.is_file() for path in libraries):
            raise SystemExit('Build the local Linux map-server first; required support objects are missing')
        includes = ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql')
        sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219', '-fno-strict-aliasing', '-fno-omit-frame-pointer'] + sanitizer + ['-I' + p for p in includes]
        compiled = []
        for source in fresh:
            target = build / (source.stem + '.o')
            print('Compiling fresh ' + str(source), flush=True)
            subprocess.run(flags + ['-c', str(source), '-o', str(target)], cwd=ROOT, check=True)
            compiled.append(target)
        command = ['g++'] + sanitizer + ['-o', str(executable)] + [str(p) for p in compiled + objects + libraries]
        command += ['-Wl,--wrap=' + name for name in WRAPPERS]
        command += ['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm']
        subprocess.run(command, cwd=ROOT, check=True)
        stamp.write_text(json.dumps(fingerprint, indent=2))
    completed = subprocess.run([str(executable), str(build)], cwd=ROOT, capture_output=True, text=True, timeout=60)
    print(completed.stdout, end='', flush=True)
    print(completed.stderr, end='', flush=True)
    if 'EP21_NATIVE_RESULT ' not in completed.stdout:
        raise AssertionError('Native postconditions not reached; this is not an ordinary regression failure')
    completed.check_returncode()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir', type=Path, help='Keep generated artifacts in this Linux directory')
    parser.add_argument('--reuse-build', action='store_true', help='Reuse only if all native source hashes and fixture metadata are unchanged')
    parser.add_argument('--pre-fix', action='store_true', help='In-memory reconstruction must match all three recorded pre-fix source hashes; expected regression failure')
    parser.add_argument('--prepare-only', action='store_true', help='Only write source-extracted temporary build fixtures')
    args = parser.parse_args()
    if args.reuse_build and not args.build_dir:
        parser.error('--reuse-build requires --build-dir')
    if args.build_dir:
        directory = args.build_dir.resolve()
        directory.mkdir(parents=True, exist_ok=True)
        run(directory, args.reuse_build, args.pre_fix, args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='rathena-ep21-flow-') as temporary:
            run(Path(temporary), False, args.pre_fix, args.prepare_only)
