// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// Isolated actual-VM test. No normal server startup, sockets or SQL connection.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
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
#include "map/itemdb.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"
#include "map/skill.hpp"
#include "map/status.hpp"

namespace {
map_session_data* attached = nullptr;
int errors = 0, assertions = 0, executions = 0;
void check(bool value, const char* message) {
  ++assertions;
  if (!value) { std::fprintf(stderr, "BONUS VM FAIL: %s\n", message); std::exit(1); }
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
  check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no new privileges");
  check(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "mandatory network denial");
  check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "socket denied");
  check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "connect denied");
  check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "bind denied");
  check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "listen denied");
}
std::string read(const std::string& path) {
  std::ifstream input(path);
  check(input.good(), "read generated exact source fixture");
  return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
std::unique_ptr<map_session_data> player(int head_id) {
  auto sd = std::make_unique<map_session_data>();
  sd->id = 99000001; sd->type = BL_PC;
  sd->status.account_id = 99000001; sd->status.char_id = 99000002;
  sd->fd = 0; sd->state.ignoretimeout = true; sd->npc_idle_timer = INVALID_TIMER;
  for (auto& index : sd->equip_index) index = -1;
  sd->equip_index[EQI_HEAD_TOP] = 0; sd->equip_index[EQI_HAND_R] = 1;
  sd->inventory.u.items_inventory[0].nameid = head_id;
  sd->inventory.u.items_inventory[1].nameid = 500134;
  current_equip_item_index = 0;
  return sd;
}
void execute(script_code* code, map_session_data& sd) {
  attached = &sd;
  sd.skillatk.clear(); sd.autospell3.clear();
  run_script(code, 0, sd.id, 0);
  ++executions;
  check(errors == 0 && sd.st == nullptr, "actual VM success and clean player detachment");
}
}
extern "C" map_session_data* vm_lookup(int32 id) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* vm_lookup(int32 id) { return attached && attached->id == id ? attached : nullptr; }
extern "C" npc_data* vm_npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* vm_npc_lookup(int32) { return nullptr; }
extern "C" void vm_mapreg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void vm_mapreg_init() {}
extern "C" void vm_mapreg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void vm_mapreg_final() {}
extern "C" int32 vm_dequeue(map_session_data*, bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 vm_dequeue(map_session_data*, bool) { return 0; }
extern "C" void vm_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void vm_error(const char* format, ...) {
  ++errors; va_list args; va_start(args, format); std::vfprintf(stderr, format, args); va_end(args);
}

extern "C" int __wrap_main(int argc, char** argv) {
  deny_network();
  check(argc == 2, "explicit generated fixture directory");
  const std::string directory = argv[1];
  static char name[] = "isolated-crown-bonus-vm";
  SERVER_NAME = name;
  malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
  battle_config.atcommand_disable_npc = 0;
  // Minimal identities/max levels come from the fresh effective skill DB.
  // Production parsing establishes real skill indices; no lookup/bonus doubles.
  const auto yaml = read(directory + "/skills.yml");
  auto tree = ryml::parse_in_arena(ryml::to_csubstr(yaml));
  for (const auto& node : tree.rootref()) check(skill_db.parseBodyNode(node) == 1, "production skill metadata parser");
  skill_db.loadingFinished();
  check(errors == 0, "subsystems and selected skill fixtures initialize");
  const auto cardinal = read(directory + "/cardinal.script");
  const auto hyper_before = read(directory + "/hyper_before.script");
  const auto hyper_after = read(directory + "/hyper_after.script");
  script_code* cd = parse_script(cardinal.c_str(), "actual-cardinal-item", 1, 0);
  script_code* hn_before = parse_script(hyper_before.c_str(), "hyper-combo-before-key-fix", 1, 0);
  script_code* hn_after = parse_script(hyper_after.c_str(), "actual-hyper-combo-after-key-fix", 1, 0);
  check(cd && hn_before && hn_after && errors == 0, "production parser accepts all exact Scripts");
  for (int refine = 0; refine <= 20; ++refine) for (int grade = 0; grade <= 4; ++grade) {
    auto sd = player(401118);
    sd->inventory.u.items_inventory[0].refine = refine;
    sd->inventory.u.items_inventory[0].enchantgrade = grade;
    execute(cd, *sd);
    const int expected = 5 * (refine / 4);
    check(pc_skillatk_bonus(sd.get(), CD_ARBITRIUM) == expected, "Cardinal parent bonus exactly once");
    check(pc_skillatk_bonus(sd.get(), CD_ARBITRIUM_ATK) == expected, "Cardinal dummy query uses same parent bonus");
    check(sd->skillatk.size() == 3, "both raw Arbitrium entries and Framen retained by actual pc_bonus2");
    check(pc_skillatk_bonus(sd.get(), CD_FRAMEN) == expected + (grade >= ENCHANTGRADE_C ? 10 : 0), "Cardinal grade gate");
  }
  auto sd = player(401117);
  const auto chain_index = skill_get_index(WL_CHAINLIGHTNING);
  check(chain_index > 0, "production learned-skill index available");
  sd->status.skill[chain_index].id = WL_CHAINLIGHTNING;
  for (int crown_refine = 0; crown_refine <= 20; ++crown_refine)
  for (int weapon_refine = 0; weapon_refine <= 20; ++weapon_refine)
  for (int crown_grade = 0; crown_grade <= 4; ++crown_grade)
  for (int weapon_grade = 0; weapon_grade <= 4; ++weapon_grade)
  for (int learned : {4, 5}) {
    sd->inventory.u.items_inventory[0].refine = crown_refine;
    sd->inventory.u.items_inventory[1].refine = weapon_refine;
    sd->inventory.u.items_inventory[0].enchantgrade = crown_grade;
    sd->inventory.u.items_inventory[1].enchantgrade = weapon_grade;
    sd->status.skill[chain_index].lv = learned;
    const int sum = crown_refine + weapon_refine;
    const bool gate = sum >= 24 && crown_grade >= ENCHANTGRADE_A && weapon_grade >= ENCHANTGRADE_A;
    for (bool fixed : {false, true}) {
      execute(fixed ? hn_after : hn_before, *sd);
      const int expected = fixed && gate ? 2 * sum : 0;
      check(pc_skillatk_bonus(sd.get(), WL_CHAINLIGHTNING) == expected, "Hyper parent query before/after and all condition boundaries");
      check(pc_skillatk_bonus(sd.get(), WL_CHAINLIGHTNING_ATK) == expected, "Hyper real damage dummy query before/after");
      check(pc_skillatk_bonus(sd.get(), HN_NAPALM_VULCAN_STRIKE) == 35, "unconditional Napalm bonus unchanged");
      check(sd->autospell3.size() == (gate && learned >= 5 ? 1u : 0u), "actual learned-skill autocast gate unchanged");
      if (!sd->autospell3.empty()) {
        const auto& cast = sd->autospell3[0];
        check(cast.id == WL_CHAINLIGHTNING && cast.lv == 5 && cast.rate == 1000 &&
              cast.trigger_skill == HN_NAPALM_VULCAN_STRIKE, "actual autocast identity level rate and trigger unchanged");
      }
    }
  }
  attached = nullptr; sd.reset();
  script_free_code(cd); script_free_code(hn_before); script_free_code(hn_after);
  skill_db.clear(); do_final_script(); timer_final(); db_final(); malloc_final();
  check(executions == 44205, "all 105 Cardinal and 44100 Hyper VM cases executed");
  std::printf("NATIVE_CROWN_BONUS_VM_OK executions=%d assertions=%d cardinal=105 hyper=44100\n", executions, assertions);
  return errors ? 1 : 0;
}
