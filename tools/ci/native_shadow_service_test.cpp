// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// Actual parse_script/run_script dialogue proof. Only player/world lookup,
// transient @menu storage, persistence, and outbound UI/resource services are
// explicit test boundaries. The NPC body and item_enchant builtin are real.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>

#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/database.hpp"
#include "common/core.hpp"
#include "common/db.hpp"
#include "common/malloc.hpp"
#include "common/timer.hpp"
#include "map/battle.hpp"
#include "map/clif.hpp"
#include "map/itemdb.hpp"
#include "map/log.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"

namespace {
constexpr int32 TEST_NPC = 99000003;
map_session_data* attached = nullptr;
unsigned assertions = 0, errors = 0, nexts = 0, closes = 0, menus = 0;
unsigned payment_calls = 0, deletion_calls = 0;
std::vector<std::string> messages;
std::vector<uint64> window_requests;
std::vector<int64> menu_values;

void check(bool ok, const char* message) {
    ++assertions;
    if (!ok) {
        std::fprintf(stderr, "SHADOW VM FAIL: %s\n", message);
        std::exit(1);
    }
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
    sock_fprog program{static_cast<unsigned short>(std::size(rules)), rules};
    check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no-new-privileges installed");
    check(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "network-denial filter installed");
    check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "socket denied");
    check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "connect denied");
    check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "bind denied");
    check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "listen denied");
}

std::string actual_body() {
    std::ifstream input("npc/custom/grademk_services.txt");
    check(input.good(), "read production grademk services source");
    const std::string source{std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    const std::string name = "Shadow Gear Enchanter#grademk";
    const auto marker = source.find(name);
    check(marker != std::string::npos && source.find(name, marker + name.size()) == std::string::npos,
          "exactly one production Shadow Gear Enchanter exists");
    const auto begin = source.find('{', marker);
    check(begin != std::string::npos, "NPC body opens");
    // Track braces while ignoring quoted text and both comment forms. No source
    // statements are rewritten; the complete body, including OnInit, is parsed.
    unsigned depth = 0;
    bool quoted = false, escaped = false, line_comment = false, block_comment = false;
    for (size_t i = begin; i < source.size(); ++i) {
        const char c = source[i], next = i + 1 < source.size() ? source[i + 1] : '\0';
        if (line_comment) { if (c == '\n') line_comment = false; continue; }
        if (block_comment) { if (c == '*' && next == '/') { block_comment = false; ++i; } continue; }
        if (quoted) {
            if (escaped) escaped = false;
            else if (c == '\\') escaped = true;
            else if (c == '"') quoted = false;
            continue;
        }
        if (c == '/' && next == '/') { line_comment = true; ++i; continue; }
        if (c == '/' && next == '*') { block_comment = true; ++i; continue; }
        if (c == '"') { quoted = true; continue; }
        if (c == '{') ++depth;
        if (c == '}' && --depth == 0) return source.substr(begin, i - begin + 1);
    }
    check(false, "production NPC body is balanced");
    return {};
}
}

extern "C" map_session_data* shadow_lookup(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* shadow_lookup(int32 id) { return attached && attached->id == id ? attached : nullptr; }
extern "C" npc_data* shadow_npc(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* shadow_npc(int32) { return nullptr; }
extern "C" void shadow_mapreg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void shadow_mapreg_init() {}
extern "C" void shadow_mapreg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void shadow_mapreg_final() {}
extern "C" int32 shadow_dequeue(map_session_data*, bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 shadow_dequeue(map_session_data*, bool) { return 0; }

extern "C" void shadow_mes(const map_session_data&, uint32, const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void shadow_mes(const map_session_data& sd, uint32 npc, const char* text) {
    check(&sd == attached && npc == TEST_NPC, "dialogue targets attached player and NPC");
    messages.emplace_back(text);
}
extern "C" void shadow_next(const map_session_data&, uint32) asm("__wrap__Z15clif_scriptnextRK16map_session_dataj");
extern "C" void shadow_next(const map_session_data& sd, uint32 npc) {
    check(&sd == attached && npc == TEST_NPC, "Next targets attached player and NPC");
    ++nexts;
}
extern "C" void shadow_close(const map_session_data&, uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void shadow_close(const map_session_data& sd, uint32 npc) {
    check(&sd == attached && npc == TEST_NPC, "Close targets attached player and NPC");
    ++closes;
}
extern "C" void shadow_menu(map_session_data&, uint32, const char*) asm("__wrap__Z15clif_scriptmenuR16map_session_datajPKc");
extern "C" void shadow_menu(map_session_data& sd, uint32 npc, const char* text) {
    check(&sd == attached && npc == TEST_NPC, "menu targets attached player and NPC");
    const char* expected = menus == 0
        ? "Open Shadow Enchant:Cancel:M. Alitea Shadow Enchant:Master class enchants"
        : "Master Weapon / Shield:Dragon Knight:Imperial Guard:Shadow Cross:Abyss Chaser:Cardinal:Inquisitor:Meister:Biolo:Windhawk:Troubadour / Trouvere:Arch Mage:Elemental Master:Night Watch:Spirit Handler:Shinkiro / Shiranui:Sky Emperor:Soul Ascetic:Hyper Novice:Cancel";
    check(std::strcmp(text, expected) == 0, "real select preserves original choices and ordered Master families");
    ++menus;
}
extern "C" bool shadow_setreg(map_session_data*, int64, int64) asm("__wrap__Z9pc_setregP16map_session_datall");
extern "C" bool shadow_setreg(map_session_data* sd, int64 reg, int64 value) {
    check(sd == attached && std::strcmp(get_str(script_getvarid(reg)), "@menu") == 0,
          "only transient select @menu write crosses registry boundary");
    menu_values.push_back(value);
    return true;
}
extern "C" void shadow_open(map_session_data&, uint64) asm("__wrap__Z23clif_enchantwindow_openR16map_session_datam");
extern "C" void shadow_open(map_session_data& sd, uint64 group) {
    check(&sd == attached && (group == 128 || group == 166 || (group >= 70 && group <= 88)), "actual item_enchant builtin requests a supported group for attached player");
    check(closes == 1, "window request occurs only after dialogue close");
    window_requests.push_back(group);
    // Deliberately do not fake packet delivery, weight checks or the UI handler's
    // item_enchant_index assignment. The tested boundary is the actual request.
}
extern "C" char shadow_pay(map_session_data*, int32, e_log_pick_type, uint32) asm("__wrap__Z10pc_payzenyP16map_session_datai15e_log_pick_typej");
extern "C" char shadow_pay(map_session_data*, int32, e_log_pick_type, uint32) { ++payment_calls; return 1; }
extern "C" char shadow_del(map_session_data*, int32, int32, int32, int16, e_log_pick_type) asm("__wrap__Z10pc_delitemP16map_session_dataiiis15e_log_pick_type");
extern "C" char shadow_del(map_session_data*, int32, int32, int32, int16, e_log_pick_type) { ++deletion_calls; return 1; }
extern "C" void shadow_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void shadow_error(const char* format, ...) {
    ++errors;
    va_list arguments;
    va_start(arguments, format);
    std::vfprintf(stderr, format, arguments);
    va_end(arguments);
}

extern "C" int __wrap_main(int, char**) {
    deny_network();
    static char test_server_name[] = "native-shadow-service-test";
    SERVER_NAME = test_server_name;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    check(errors == 0, "isolated script subsystems initialize cleanly");
    auto group = std::make_shared<s_item_enchant>();
    group->id = 128;
    item_enchant_db.put(128, group);
    auto alitea = std::make_shared<s_item_enchant>();
    alitea->id = 166;
    item_enchant_db.put(166, alitea);
    for (uint64 id = 70; id <= 88; ++id) {
        auto master = std::make_shared<s_item_enchant>();
        master->id = id;
        item_enchant_db.put(id, master);
    }
    // Only existence is used by this builtin; full native DB parsing and recipe
    // execution are deliberately not claimed by this minimal container entry.
    const auto source = actual_body();
    script_code* code = parse_script(source.c_str(), "npc/custom/grademk_services.txt:Shadow Gear Enchanter", 1, 0);
    check(code != nullptr, "actual production dialogue parses without rewriting");
    battle_config.atcommand_disable_npc = 0;
    struct TestCase { int choice; int subchoice; };
    std::vector<TestCase> cases{{2,0},{1,0},{3,0},{255,0}};
    for (int family = 1; family <= 20; ++family) cases.push_back({4,family});
    cases.push_back({4,255});
    for (const auto& test : cases) {
        const int choice = test.choice, subchoice = test.subchoice;
        messages.clear(); window_requests.clear(); menu_values.clear();
        nexts = closes = menus = payment_calls = deletion_calls = 0;
        auto player = std::make_unique<map_session_data>();
        attached = player.get();
        player->id = 99000001; player->type = BL_PC;
        player->status.account_id = 99000001; player->status.char_id = 99000002;
        player->status.zeny = 7654321;
        player->fd = 0; player->state.ignoretimeout = true;
        player->npc_idle_timer = INVALID_TIMER;
        player->weight = 100; player->max_weight = 10000;
        auto& equipment = player->inventory.u.items_inventory[0];
        equipment.nameid = 24872; equipment.amount = 1; equipment.identify = 1;
        equipment.refine = 10; equipment.unique_id = UINT64_MAX;
        equipment.bound = 2; equipment.favorite = 1;
        equipment.card[3] = 4702; equipment.card[2] = 311229;
        auto& essence = player->inventory.u.items_inventory[1];
        essence.nameid = 1001253; essence.amount = 30; essence.identify = 1;
        player->inventory.amount = 2;
        const auto before = player->inventory;
        const auto before_zeny = player->status.zeny;
        const auto unchanged = [&]() {
            check(std::memcmp(&before, &player->inventory, sizeof(before)) == 0, "entire inventory remains byte-identical");
            check(player->status.zeny == before_zeny, "zeny unchanged");
            check(payment_calls == 0 && deletion_calls == 0, "no script-side payment or item deletion attempted");
        };
        run_script(code, 0, player->id, TEST_NPC);
        check(player->st && player->st->state == STOP && nexts == 1 && messages.size() == 4, "first real Next suspension");
        unchanged();
        run_script_main(player->st);
        check(player->st && player->st->state == STOP && nexts == 2 && messages.size() == 8, "second real Next suspension");
        check(messages[6].find("may retain or lower") != std::string::npos &&
              messages[6].find("no reset") != std::string::npos, "actual risk and no-reset warning displayed");
        check(messages[7].find("two ordered, selectable enchants") != std::string::npos, "Alitea service explains its separate recipe flow");
        unchanged();
        run_script_main(player->st);
        check(player->st && player->st->state == RERUNLINE && menus == 1 && player->state.menu_or_input,
              "actual select builtin waits for client choice");
        check(window_requests.empty(), "no window opens before selection");
        player->npc_menu = choice;
        run_script_main(player->st);
        if (choice == 4) {
            check(player->st && player->st->state == RERUNLINE && menus == 2 && player->state.menu_or_input,
                  "Master submenu waits for its own client choice");
            check(menu_values == std::vector<int64>{4} && closes == 0 && window_requests.empty(),
                  "opening Master submenu does not close or request native UI");
            unchanged();
            player->npc_menu = subchoice;
            run_script_main(player->st);
        }
        const bool escaped = choice == 255 || subchoice == 255;
        const bool cancelled = choice == 2 || (choice == 4 && subchoice == 20);
        if (escaped) {
            check(player->st == nullptr && closes == 0 && menu_values == (choice == 4 ? std::vector<int64>{4} : std::vector<int64>{}), "menu Escape terminates without close continuation");
        } else {
            check(menu_values == (choice == 4 ? std::vector<int64>{4,subchoice} : std::vector<int64>{choice}), "actual select resolves requested option");
            check(player->st && closes == 1 && window_requests.empty(), "dialogue pauses for close acknowledgement");
            check(player->st->state == (cancelled ? CLOSE : STOP), "Cancel uses close, Open uses close2");
            // This explicit client-ack boundary follows npc_scriptcont's state
            // transition; that world/proximity/packet handler is not executed.
            player->st->state = cancelled ? END : RUN;
            run_script_main(player->st);
            check(player->st == nullptr, "acknowledged dialogue detaches from real VM");
        }
        const bool opened = !escaped && !cancelled;
        check(window_requests.size() == (opened ? 1u : 0u), "only valid Open requests exactly one enchant window");
        if (opened)
            check(window_requests[0] == (choice == 1 ? 128u : choice == 3 ? 166u : 69u + subchoice), "menu selection routes to its exact group");
        check(!player->state.menu_or_input, "menu wait flag cleared");
        check(player->state.item_enchant_index == 0, "UI boundary did not simulate transport/session activation");
        unchanged();
        check(errors == 0, "real dialogue path has no script errors");
        std::printf("SHADOW_SERVICE_CASE_PASS: main=%d sub=%d\n", choice, subchoice);
        attached = nullptr;
    }
    script_free_code(code);
    item_enchant_db.clear();
    group.reset();
    alitea.reset();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("PASS actual Shadow Gear Enchanter VM: %zu paths; %u assertions; no script-side charges; UI requests only\n", cases.size(), assertions);
    return errors ? 1 : 0;
}
