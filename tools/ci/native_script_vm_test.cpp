// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  native_script_vm_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/native_script_vm_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// Isolated real-script-VM proof. Player lookup, network UI, logging, map registry
// persistence, and event dequeue are explicit doubles. The parser, VM, builtin,
// item DB lookup, and inventory mutation are compiled from production code.
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
map_session_data* attached = nullptr;
int errors = 0;
int added = 0;
int removed = 0;
int assertions = 0;
struct LogEntry { int amount; item value; };
std::vector<LogEntry> logged;

void check(bool value, const char* message) {
	++assertions;
	if (!value) {
		std::fprintf(stderr, "VM TEST FAIL: %s\n", message);
		std::exit(1);
	}
}

void deny_network() {
	// Fail closed: socket()/connect()/bind()/listen() are denied even inside
	// libraries, if a future initialization accidentally calls world code.
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
	check(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "install no-new-privileges");
	check(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "install mandatory network-denial filter");
	check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "kernel refuses socket creation");
	check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "kernel refuses connect");
	check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "kernel refuses bind");
	check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "kernel refuses listen");
}
}

extern "C" map_session_data* vm_lookup(int32 id) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* vm_lookup(int32 id) {
	return attached && attached->id == id ? attached : nullptr;
}
extern "C" npc_data* vm_npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* vm_npc_lookup(int32) { return nullptr; }
extern "C" void vm_mapreg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void vm_mapreg_init() {}
extern "C" void vm_mapreg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void vm_mapreg_final() {}
extern "C" int32 vm_dequeue(map_session_data*, bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 vm_dequeue(map_session_data*, bool) { return 0; }
extern "C" void vm_log(const map_session_data*, e_log_pick_type, int32, const item*) asm("__wrap__Z11log_pick_pcPK16map_session_data15e_log_pick_typeiPK4item");
extern "C" void vm_log(const map_session_data* sd, e_log_pick_type type, int32 amount, const item* value) {
	check(sd == attached && type == LOG_TYPE_ENCHANT, "enchant log identifies attached test player");
	logged.push_back({amount, *value});
}
extern "C" void vm_add(const map_session_data*, int32, int32, unsigned char) asm("__wrap__Z12clif_additemPK16map_session_dataiih");
extern "C" void vm_add(const map_session_data* sd, int32 index, int32 amount, unsigned char fail) {
	check(sd == attached && index == 0 && amount == 1 && fail == 0, "inventory add notification arguments");
	++added;
}
extern "C" void vm_remove(const map_session_data&, int32, int32, int16) asm("__wrap__Z12clif_delitemRK16map_session_dataiis");
extern "C" void vm_remove(const map_session_data& sd, int32 index, int32 amount, int16 reason) {
	check(&sd == attached && index == 0 && amount == 1 && reason == 3, "inventory remove notification arguments");
	++removed;
}
extern "C" void vm_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void vm_error(const char* format, ...) {
	++errors;
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	va_end(args);
}

extern "C" int __wrap_main(int, char**) {
	deny_network();
	static char test_server_name[] = "native-script-vm-test";
	SERVER_NAME = test_server_name;
	malloc_init();
	db_init();
	do_init_database();
	timer_init();
	do_init_script();
	check(errors == 0, "script subsystem initializes without parser/DB errors");
	auto player = std::make_unique<map_session_data>();
	attached = player.get();
	player->id = 99000001;
	player->type = BL_PC;
	player->status.account_id = 99000001;
	player->status.char_id = 99000002;
	player->fd = 0;
	player->state.ignoretimeout = true;
	player->npc_idle_timer = INVALID_TIMER;
	battle_config.atcommand_disable_npc = 0;
	auto equipment = std::make_shared<item_data>();
	equipment->nameid = 490136;
	equipment->type = IT_ARMOR;
	equipment->slots = 1;
	item_db.put(equipment->nameid, equipment);
	auto enchant = std::make_shared<item_data>();
	enchant->nameid = 310710;
	enchant->type = IT_CARD;
	enchant->subtype = CARD_ENCHANT;
	item_db.put(enchant->nameid, enchant);
	auto next_enchant = std::make_shared<item_data>(*enchant);
	next_enchant->nameid = 310711;
	item_db.put(next_enchant->nameid, next_enchant);
	auto ordinary_card = std::make_shared<item_data>();
	ordinary_card->nameid = 4001;
	ordinary_card->type = IT_CARD;
	ordinary_card->subtype = CARD_NORMAL;
	item_db.put(ordinary_card->nameid, ordinary_card);
	player->inventory_data[0] = equipment.get();
	item& original = player->inventory.u.items_inventory[0];
	original.nameid = 490136;
	original.amount = 1;
	original.identify = 1;
	original.unique_id = UINT64_MAX;
	original.card[2] = 310709;
	original.refine = 12;
	original.enchantgrade = 3;
	original.bound = 2;
	original.favorite = 1;
	original.expire_time = 2000000000;
	original.option[0].id = 1;
	original.option[0].value = 23;
	const item before = original;
	std::ifstream fixture("npc/test/native_vm_inventory_fixture.script");
	check(fixture.good(), "read disabled isolated VM fixture");
	const std::string source{std::istreambuf_iterator<char>(fixture), std::istreambuf_iterator<char>()};
	script_code* code = parse_script(source.c_str(), "npc/test/native_vm_inventory_fixture.script", 1, 0);
	check(code != nullptr, "actual parser accepts guarded inventory mutation");
	run_script(code, 0, player->id, 0);
	check(errors == 0, "actual script VM returns success");
	check(original.card[2] == 310710, "actual builtin mutates selected enchant");
	item expected = before;
	expected.card[2] = 310710;
	check(std::memcmp(&original, &expected, sizeof(item)) == 0, "all remaining item metadata preserved byte-for-byte");
	check(logged.size() == 2 && logged[0].amount == -1 && logged[1].amount == 1, "old/new log order");
	check(std::memcmp(&logged[0].value, &before, sizeof(item)) == 0, "old log contains unmodified record");
	check(std::memcmp(&logged[1].value, &expected, sizeof(item)) == 0, "new log contains mutated record");
	check(added == 1 && removed == 1, "exactly one inventory refresh pair");
	check(player->st == nullptr, "player detached from completed VM state");
	script_free_code(code);
	attached = nullptr;
	player.reset();
	item_db.clear();
	do_final_script();
	timer_final();
	db_final();
	malloc_final();
	std::printf("PASS actual rAthena script VM: %d assertions; synthetic player/world boundaries, no sockets or SQL\n", assertions);
	return errors ? 1 : 0;
}
