// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  enchant_upgrade_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/enchant_upgrade_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Behavioral tests for the exact selectors used by both client request handlers.
#include <cstdlib>
#include <iostream>
#include "map/enchant_upgrade.hpp"

static unsigned checks = 0;
static void require(bool ok, const char* message) {
	++checks;
	if (!ok) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

int main() {
	EnchantUpgradeMap ordinary;
	PerfectEnchantUpgradeMap perfect;
	auto random = std::make_shared<s_item_enchant_upgrade>();
	random->enchant_item_id = 600;
	random->zeny = 5000000;
	random->materials = {{100, 10}};
	random->random_upgrades = {{500, 10000}, {600, 20000}, {700, 70000}};
	ordinary[600] = random;
	auto guaranteed = std::make_shared<s_item_enchant_upgrade>();
	guaranteed->enchant_item_id = 600;
	guaranteed->upgrade_item_id = 700;
	guaranteed->zeny = 200000000;
	guaranteed->materials = {{100, 80}, {200, 120}};
	perfect[600][700] = guaranteed;
	auto alternative = std::make_shared<s_item_enchant_upgrade>(*guaranteed);
	alternative->upgrade_item_id = 800;
	alternative->zeny = 300000000;
	perfect[600][800] = alternative;

	require(select_enchant_upgrade(ordinary, perfect, 600, false, 0) == random,
		"ordinary request selects its own costs and weighted outcomes");
	require(select_enchant_upgrade(ordinary, perfect, 600, true, 700) == guaranteed,
		"perfect request selects exact source/target pair despite source overlap");
	require(select_enchant_upgrade(ordinary, perfect, 600, true, 800) == alternative,
		"multiple guaranteed targets retain independent costs");
	for (t_itemid target : {0u, 500u, 600u, 900u})
		require(!select_enchant_upgrade(ordinary, perfect, 600, true, target),
			"unlisted perfect result does not fall back to a random recipe");
	for (bool mode : {false, true})
		require(!select_enchant_upgrade(ordinary, perfect, 999, mode, mode ? 700 : 0),
			"wrong existing enchant rejected");
	require(!select_enchant_upgrade(ordinary, perfect, 600, false, 700),
		"ordinary request cannot select a guaranteed outcome");
	ordinary.clear();
	require(!select_enchant_upgrade(ordinary, perfect, 600, false, 0),
		"ordinary request cannot fall back to perfect-only recipe");

	std::unordered_map<t_itemid, unsigned> frequencies;
	for (uint32 roll = 1; roll <= 100000; ++roll)
		++frequencies[select_enchant_upgrade_result(*random, roll)];
	require(frequencies.size() == 3 && frequencies[500] == 10000 &&
		frequencies[600] == 20000 && frequencies[700] == 70000,
		"all 100000 random draws partition exactly into declared probabilities");
	require(select_enchant_upgrade_result(*random, 0) == 0, "zero roll rejected");
	require(select_enchant_upgrade_result(*random, 100001) == 0, "oversized roll rejected");
	require(select_enchant_upgrade_result(*guaranteed, 0) == 700, "guaranteed target applied");
	require(select_enchant_upgrade_result(*alternative, 0) == 800, "alternate target applied");
	random->random_upgrades = {{700, 99999}};
	require(select_enchant_upgrade_result(*random, 100000) == 0, "uncovered roll fails closed");
	std::cout << "PASS: " << checks << " upgrade selection checks, including all 100000 random rolls\n";
}
