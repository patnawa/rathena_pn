// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  garden_legacy_gate_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/garden_legacy_gate_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Actual current NPC parser/visibility/click/unload implementation, not a model.
// Inclusion allows initializing only its private registries, without do_init_npc
// loading unrelated content, market SQL, timers, or a live world. No production
// source is modified. The runner excludes the existing npc.o from the link.
#include "../../src/map/npc.cpp"

#include <cerrno>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace garden_test {
unsigned assertions = 0, failures = 0, errors = 0, cases = 0;
unsigned gate_vm_calls = 0, messages = 0, closes = 0, option_packets = 0, clears = 0, spawns = 0;
std::vector<std::unique_ptr<map_session_data>> players;
std::unordered_map<int32, block_list*> world;
block_list registered_block_marker;
std::vector<script_code*> gate_codes;

void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "GARDEN_BOUNDARY_FAILURE: %s\n", message); std::exit(3); }
}
void check(bool value, const char* message) {
    ++assertions;
    if (!value) { ++failures; std::fprintf(stdout, "GARDEN_EXPECTATION_FAILURE: %s\n", message); }
}
void deny_network() {
    sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_socket, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_connect, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_bind, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_listen, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog program{static_cast<unsigned short>(std::size(rules)), rules};
    require(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no-new-privileges");
    require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "mandatory network denial");
    check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "socket denied");
    check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "connect denied");
    check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "bind denied");
    check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "listen denied");
}
script_code* script_file(const char* path) {
    std::ifstream input(path, std::ios::binary);
    require(input.good(), "read exact extracted script fixture");
    std::string body{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    auto* code = parse_script(body.c_str(), path, 1, 0);
    require(code && !errors, "actual extracted builtin calls compile");
    return code;
}
void complete(map_session_data& player) {
    if (player.st) {
        require(player.st->state == CLOSE, "old/control path stops at original unfinished-quest Close");
        player.st->state = END; // explicit close-ack boundary; no packet handler
        run_script_main(player.st);
    }
    require(!player.st && !player.npc_id, "native VM cleanup detaches player");
}
void blocked_click(npc_data& gate, map_session_data& player, const char* phase) {
    ++cases;
    player.x = gate.x; player.y = gate.y - 1;
    const auto old_calls = gate_vm_calls, old_messages = messages;
    const auto inventory = player.inventory;
    const auto zeny = player.status.zeny;
    const int result = npc_click(&player, &gate); // real native proximity + hide guard + dispatch
    check(result == 1, "gate refuses native click");
    check(gate_vm_calls == old_calls, "legacy body never dispatched");
    check(player.st == nullptr && messages == old_messages, "no legacy dialogue began");
    complete(player);
    check(std::memcmp(&inventory, &player.inventory, sizeof(inventory)) == 0 && player.status.zeny == zeny,
          "complete inventory and Zeny unchanged");
    std::printf("GARDEN_CASE phase=%s gate=%s player=%d disabled=%d hide=%d cloaked=%d click=%d\n",
                phase, gate.exname, player.status.char_id, gate.is_invisible,
                !!(gate.sc.option & OPTION_HIDE), npc_is_cloaked(&gate, &player), result);
}
struct Iterator { std::vector<block_list*> values; size_t position = 0; };
}

extern "C" void garden_reg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void garden_reg_init() {}
extern "C" void garden_reg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void garden_reg_final() {}
extern "C" void garden_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void garden_error(const char* fmt, ...) { ++garden_test::errors; va_list ap; va_start(ap, fmt); std::vfprintf(stderr, fmt, ap); va_end(ap); }
extern "C" block_list* garden_bl(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* garden_bl(int32 id) { auto p = garden_test::world.find(id); return p == garden_test::world.end() ? nullptr : p->second; }
extern "C" bool garden_exists(int32) asm("__wrap__Z15map_blid_existsi");
extern "C" bool garden_exists(int32 id) { return garden_bl(id) != nullptr; }
extern "C" map_session_data* garden_sd(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* garden_sd(int32 id) { auto* p = garden_bl(id); return BL_CAST(BL_PC, p); }
extern "C" npc_data* garden_nd(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* garden_nd(int32 id) { auto* p = garden_bl(id); return BL_CAST(BL_NPC, p); }
extern "C" map_session_data* garden_cid(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* garden_cid(int32 id) { for (auto& p : garden_test::players) if (p->status.char_id == id) return p.get(); return nullptr; }
extern "C" int16 garden_map(const char*) asm("__wrap__Z17map_mapname2mapidPKc");
extern "C" int16 garden_map(const char* name) { return std::strcmp(name, "t_garden") ? -1 : 0; }
extern "C" uint16 garden_index(const char*, const char*) asm("__wrap__Z17mapindex_name2idxPKcS0_");
extern "C" uint16 garden_index(const char* name, const char*) { return garden_map(name) == 0 ? 1000 : 0; }
extern "C" void garden_addid(block_list*) asm("__wrap__Z11map_addiddbP10block_list");
extern "C" void garden_addid(block_list* b) { garden_test::require(garden_test::world.emplace(b->id, b).second, "unique synthetic world ID"); }
extern "C" void garden_delid(block_list*) asm("__wrap__Z11map_deliddbP10block_list");
extern "C" void garden_delid(block_list* b) { garden_test::require(garden_test::world.erase(b->id) == 1, "remove known synthetic world ID"); }
extern "C" bool garden_addnpc(int16, npc_data*) asm("__wrap__Z10map_addnpcsP8npc_data");
extern "C" bool garden_addnpc(int16 m, npc_data* nd) {
    garden_test::require(m == 0 && nd->u.scr.xs == -1 && nd->u.scr.ys == -1, "only actual non-touch Garden gates enter world double");
    map[0].npc[map[0].npc_num++] = nd; garden_addid(nd); return true;
}
extern "C" int32 garden_addblock(block_list*) asm("__wrap__Z12map_addblockP10block_list");
extern "C" int32 garden_addblock(block_list* b) {
    garden_test::require(b->type == BL_NPC && b->m == 0 && !b->prev, "known NPC block registration");
    // Native npc_remove_map uses prev != nullptr as registration evidence.
    // Spatial buckets are out of scope; retain their registration contract.
    b->prev = &garden_test::registered_block_marker; b->next = nullptr; return 0;
}
extern "C" int32 garden_delblock(block_list*) asm("__wrap__Z12map_delblockP10block_list");
extern "C" int32 garden_delblock(block_list* b) {
    garden_test::require(b->type == BL_NPC && b->m == 0 && b->prev == &garden_test::registered_block_marker, "known NPC block removal");
    b->prev = b->next = nullptr; return 0;
}
extern "C" s_mapiterator* garden_iter(e_mapitflags, bl_type) asm("__wrap__Z11mapit_alloc12e_mapitflags7bl_type");
extern "C" s_mapiterator* garden_iter(e_mapitflags flags, bl_type types) {
    garden_test::require(flags == MAPIT_NORMAL, "normal isolated world iterator");
    auto* it = new garden_test::Iterator;
    for (auto& entry : garden_test::world) if (entry.second->type & types) it->values.push_back(entry.second);
    return reinterpret_cast<s_mapiterator*>(it);
}
extern "C" void garden_iter_free(s_mapiterator*) asm("__wrap__Z10mapit_freeP13s_mapiterator");
extern "C" void garden_iter_free(s_mapiterator* p) { delete reinterpret_cast<garden_test::Iterator*>(p); }
extern "C" bool garden_iter_exists(s_mapiterator*) asm("__wrap__Z12mapit_existsP13s_mapiterator");
extern "C" bool garden_iter_exists(s_mapiterator* p) { auto& it = *reinterpret_cast<garden_test::Iterator*>(p); return it.position < it.values.size(); }
extern "C" block_list* garden_iter_first(s_mapiterator*) asm("__wrap__Z11mapit_firstP13s_mapiterator");
extern "C" block_list* garden_iter_first(s_mapiterator* p) { auto& it = *reinterpret_cast<garden_test::Iterator*>(p); it.position = 0; return garden_iter_exists(p) ? it.values[0] : nullptr; }
extern "C" block_list* garden_iter_next(s_mapiterator*) asm("__wrap__Z10mapit_nextP13s_mapiterator");
extern "C" block_list* garden_iter_next(s_mapiterator* p) { auto& it = *reinterpret_cast<garden_test::Iterator*>(p); ++it.position; return garden_iter_exists(p) ? it.values[it.position] : nullptr; }
extern "C" int32 garden_foreach(int32 (*)(block_list*, va_list), int16, int32, ...) asm("__wrap__Z16map_foreachinmapPFiP10block_listP13__va_list_tagEsiz");
extern "C" int32 garden_foreach(int32 (*func)(block_list*, va_list), int16 m, int32 types, ...) {
    garden_test::require(m == 0 && types == BL_PC, "cloak cleanup iterates only the attached fixture players");
    int32 result = 0; va_list ap; va_start(ap, types);
    for (auto& p : garden_test::players) { va_list copy; va_copy(copy, ap); result += func(p.get(), copy); va_end(copy); }
    va_end(ap); return result;
}
extern "C" void garden_foreachnpc(int32 (*)(npc_data*, va_list), ...) asm("__wrap__Z14map_foreachnpcPFiP8npc_dataP13__va_list_tagEz");
extern "C" void garden_foreachnpc(int32 (*func)(npc_data*, va_list), ...) {
    std::vector<npc_data*> copy;
    for (auto& e : garden_test::world) if (e.second->type == BL_NPC) copy.push_back(static_cast<npc_data*>(e.second));
    va_list ap; va_start(ap, func);
    for (auto* nd : copy) { va_list next; va_copy(next, ap); func(nd, next); va_end(next); }
    va_end(ap);
}
extern "C" int32 garden_spawn(const block_list*, bool) asm("__wrap__Z10clif_spawnPK10block_listb");
extern "C" int32 garden_spawn(const block_list* b, bool) { garden_test::require(b && b->type == BL_NPC, "NPC-only spawn request"); ++garden_test::spawns; return 0; }
extern "C" void garden_option(const block_list*, const block_list*) asm("__wrap__Z24clif_changeoption_targetPK10block_listS1_");
extern "C" void garden_option(const block_list* b, const block_list* target) {
    garden_test::require(b && b->type == BL_NPC && (!target || target->type == BL_PC), "NPC option UI recorder");
    ++garden_test::option_packets;
}
extern "C" void garden_clear(const block_list&, clr_type) asm("__wrap__Z19clif_clearunit_areaRK10block_list8clr_type");
extern "C" void garden_clear(const block_list& b, clr_type) { garden_test::require(b.type == BL_NPC, "NPC clear UI recorder"); ++garden_test::clears; }
extern "C" void garden_mes(const map_session_data&, uint32, const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void garden_mes(const map_session_data& sd, uint32 id, const char* text) {
    garden_test::require(garden_sd(sd.id) == &sd && garden_nd(id), "legacy validation message targets real parsed gate/player");
    garden_test::require(std::strstr(text, "You have not completed all the steps required for entry") != nullptr, "old/control body reaches its exact first quest guard only");
    ++garden_test::messages;
}
extern "C" void garden_close(const map_session_data&, uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void garden_close(const map_session_data& sd, uint32 id) { garden_test::require(garden_sd(sd.id) == &sd && garden_nd(id), "known close destination"); ++garden_test::closes; }
extern "C" void garden_real_run(script_code*, int32, int32, int32) asm("__real__Z10run_scriptP11script_codeiii");
extern "C" void garden_run(script_code*, int32, int32, int32) asm("__wrap__Z10run_scriptP11script_codeiii");
extern "C" void garden_run(script_code* code, int32 pos, int32 rid, int32 oid) {
    if (std::find(garden_test::gate_codes.begin(), garden_test::gate_codes.end(), code) != garden_test::gate_codes.end()) ++garden_test::gate_vm_calls;
    garden_real_run(code, pos, rid, oid);
}

extern "C" int __wrap_main(int argc, char** argv) {
    using namespace garden_test;
    require(argc == 4, "gate, reveal, and existing suppression fixtures supplied");
    deny_network(); static char server[] = "garden-legacy-native-test"; SERVER_NAME = server;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    // The exact native registry allocation shapes, without do_init_npc's SQL,
    // content loading, live timers, stylist/barter DBs, or fake NPC creation.
    ev_db = strdb_alloc((DBOptions)(DB_OPT_DUP_KEY | DB_OPT_RELEASE_DATA), EVENT_NAME_LENGTH);
    npcname_db = strdb_alloc(DB_OPT_BASE, NPC_NAME_LENGTH + 1);
    npc_path_db = strdb_alloc((DBOptions)(DB_OPT_BASE | DB_OPT_DUP_KEY | DB_OPT_RELEASE_DATA), 80);
    for (int i = 0; i < MAX_NPC_CLASS; ++i) npc_viewdb[i].look[LOOK_BASE] = i;
    for (int i = MAX_NPC_CLASS2_START; i < MAX_NPC_CLASS2_END; ++i) npc_viewdb2[i - MAX_NPC_CLASS2_START].look[LOOK_BASE] = i;
    std::strcpy(map[0].name, "t_garden"); map[0].m = 0; map[0].index = 1000; map[0].xs = map[0].ys = 400; map[0].users = 2;
    battle_config.atcommand_disable_npc = 0; battle_config.etc_log = 0; battle_config.dynamic_mobs = 0;
    for (int i = 0; i < 2; ++i) {
        auto p = std::make_unique<map_session_data>(); p->id = 99000010 + i; p->type = BL_PC; p->m = 0;
        p->status.account_id = p->id; p->status.char_id = 99000020 + i; p->status.zeny = 7654321;
        p->fd = 0; p->state.ignoretimeout = true; p->npc_idle_timer = p->npc_timer_id = INVALID_TIMER;
        garden_addid(p.get()); players.emplace_back(std::move(p));
    }
    auto* reveal = script_file(argv[2]); auto* timers = script_file(argv[3]);
    int32 previous_ids[2] = {};
    for (int generation = 0; generation < 2; ++generation) {
        require(npc_parsesrcfile(argv[1]) == 1 && !errors, "actual NPC source parser succeeds without diagnostics");
        check(map[0].npc_num == 2 && map[0].npc_num_area == 0, "only two actual non-touch gates registered");
        npc_data* gates[] = {npc_name2id("Dimensional Prison#1"), npc_name2id("Dimensional Prison#2")};
        gate_codes.clear();
        for (int i = 0; i < 2; ++i) {
            require(gates[i] && gates[i]->u.scr.script, "native unique-name lookup and full script body");
            gate_codes.push_back(gates[i]->u.scr.script);
            check(gates[i]->id != previous_ids[i], "reload constructs fresh NPC identity");
            previous_ids[i] = gates[i]->id;
            check(gates[i]->x == (i ? 173 : 158) && gates[i]->y == 235, "original gate coordinate retained");
            check(gates[i]->state == NPCVIEW_DISABLE, "native parser recognizes DISABLED declaration");
            check(gates[i]->is_invisible && (gates[i]->sc.option & OPTION_HIDE), "disabled and hidden immediately after native parse, without timer");
            blocked_click(*gates[i], *players[0], generation ? "reload-before-timer" : "startup-before-timer");
        }
        for (auto& player : players) {
            run_script(reveal, 0, player->id, gates[0]->id);
            require(!player->st && !errors, "actual cloakoffnpcself calls finish without suspension/error");
            for (auto* gate : gates) {
                check(!npc_is_cloaked(gate, player.get()), "native per-character reveal clears cloak presentation");
                check(gate->is_invisible && (gate->sc.option & OPTION_HIDE), "story reveal does not enable gate");
                blocked_click(*gate, *player, "after-player-reveal");
            }
        }
        for (auto* gate : gates) {
            require(npc_enable_target(*gate, 0, NPCVIEW_CLOAKOFF), "native global cloak-off succeeds");
            check(gate->is_invisible && (gate->sc.option & OPTION_HIDE), "global cloak-off does not enable gate");
            blocked_click(*gate, *players[0], "after-global-reveal");
        }
        run_script(timers, 0, 0, gates[0]->id);
        require(!errors, "existing extracted custom suppression calls succeed");
        for (auto* gate : gates) {
            blocked_click(*gate, *players[0], "after-existing-suppression");
            // Positive control: prove our proximity/world/VM doubles do not
            // blanket-reject every NPC click. Explicit enable remains possible.
            require(npc_enable_target(*gate, 0, NPCVIEW_ENABLE), "explicit native enable control");
            auto& player = *players[0]; player.x = gate->x; player.y = gate->y - 1;
            const auto before = gate_vm_calls;
            check(npc_click(&player, gate) == 0 && gate_vm_calls == before + 1 && player.st && player.st->state == CLOSE,
                  "explicit enabled control enters actual original quest-guard dialogue");
            complete(player);
            require(npc_enable_target(*gate, 0, NPCVIEW_DISABLE), "restore disabled state before native unload");
        }
        gate_codes.clear();
        require(npc_unloadfile(argv[1]), "actual file unload succeeds");
        check(!npc_name2id("Dimensional Prison#1") && !npc_name2id("Dimensional Prison#2") && !map[0].npc_num,
              "native unload removes both name registrations and map entries");
        check(!garden_bl(previous_ids[0]) && !garden_bl(previous_ids[1]), "native unload removes old world IDs");
        check(!errors, "no native lifecycle errors");
        // Next iteration reparses only this file: no custom OnInit/timer occurs.
    }
    script_free_code(reveal); script_free_code(timers);
    ev_db->destroy(ev_db, nullptr); npcname_db->destroy(npcname_db, nullptr); npc_path_db->destroy(npc_path_db, nullptr);
    script_event.clear();
    for (auto& p : players) garden_delid(p.get()); players.clear(); require(world.empty(), "private world fully released");
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("GARDEN_NATIVE_RESULT cases=%u assertions=%u failures=%u errors=%u gate_vm_calls=%u\n", cases, assertions, failures, errors, gate_vm_calls);
    std::printf("GARDEN_UI_REQUESTS option=%u clear=%u spawn=%u; packet transport and rendering not executed\n", option_packets, clears, spawns);
    return errors ? 3 : failures ? 2 : 0;
}
