// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  enchant_probability_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/enchant_probability_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Exercise the exact normal-enchant/reset helpers used by clif.cpp.
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include "map/enchant_probability.hpp"

static unsigned checks = 0;
static void require(bool ok, const char* message) {
	++checks;
	if (!ok) { std::cerr << "FAIL: " << message << '\n'; std::exit(1); }
}

static s_item_enchant_normal distribution(std::initializer_list<std::pair<t_itemid, uint32>> outcomes) {
	s_item_enchant_normal result{};
	for (const auto& entry : outcomes)
		result.enchants[entry.first] = std::make_shared<s_item_enchant_normal_sub>(s_item_enchant_normal_sub{entry.first, entry.second});
	return result;
}

static int test_effective_database() {
	unsigned table_count;
	require(static_cast<bool>(std::cin >> table_count) && table_count > 0, "database fixture has tables");
	for (unsigned i = 0; i < table_count; ++i) {
		uint64 group;
		uint16 slot, grade;
		unsigned outcomes;
		require(static_cast<bool>(std::cin >> group >> slot >> grade >> outcomes) && outcomes > 0, "complete table header");
		s_item_enchant_normal table{};
		table.enchantgrade = grade;
		std::unordered_map<t_itemid, unsigned> expected, actual;
		for (unsigned n = 0; n < outcomes; ++n) {
			t_itemid id;
			uint32 chance;
			require(static_cast<bool>(std::cin >> id >> chance) && !table.enchants.count(id), "complete unique outcome");
			table.enchants[id] = std::make_shared<s_item_enchant_normal_sub>(s_item_enchant_normal_sub{id, chance});
			if (chance) expected[id] = chance;
		}
		for (uint32 roll = 1; roll <= 100000; ++roll) ++actual[select_normal_enchant_result(table, roll)];
		if (actual != expected) {
			std::cerr << "Database distribution mismatch: group " << group << ", slot " << slot << ", grade " << grade << '\n';
			return 1;
		}
	}
	std::cin >> std::ws;
	require(std::cin.peek() == std::char_traits<char>::eof(), "no trailing fixture data");
	std::cout << "PASS: all " << table_count << " effective database tables, " << static_cast<uint64>(table_count) * 100000
		<< " exhaustive draws with exact declared frequencies\n";
	return 0;
}

int main(int argc, char** argv) {
	if (argc == 2 && std::string(argv[1]) == "--database") return test_effective_database();
	require(argc == 1, "unknown test argument rejected");
	auto table = distribution({{100, 0}, {200, 1}, {300, 9999}, {400, 90000}});
	std::unordered_map<t_itemid, unsigned> counts;
	for (uint32 roll = 1; roll <= 100000; ++roll) ++counts[select_normal_enchant_result(table, roll)];
	require(counts.size() == 3 && counts[200] == 1 && counts[300] == 9999 && counts[400] == 90000,
		"every draw follows exact declared weights, including one-in-100000 rarity");
	require(counts[100] == 0 && counts[0] == 0, "zero-weight outcome and empty result are never selected");
	require(select_normal_enchant_result(table, 0) == 0, "zero draw rejected");
	require(select_normal_enchant_result(table, 100001) == 0, "oversized draw rejected");
	require(select_normal_enchant_result(distribution({{100, 100000}}), 100000) == 100, "single outcome is guaranteed");
	for (auto invalid : {distribution({}), distribution({{100, 0}}), distribution({{100, 99999}}),
		distribution({{100, 50001}, {200, 50000}}), distribution({{0, 100000}}), distribution({{100, 100001}})})
		require(select_normal_enchant_result(invalid, 1) == 0, "invalid total or item rejects even first draw");
	table.enchants[200] = nullptr;
	require(select_normal_enchant_result(table, 1) == 0, "null outcome rejected");
	table = distribution({{100, 100000}});
	table.enchants[100]->item_id = 101;
	require(select_normal_enchant_result(table, 1) == 0, "inconsistent item identity rejected");

	const std::unordered_map<uint16, uint32> bonuses = {{1, 10000}, {2, 20000}, {3, 30000}, {4, 40000}};
	for (uint16 grade = 0; grade <= 4; ++grade)
		require(enchant_success_rate(50000, bonuses, grade) == 50000u + grade * 10000u,
			"only current grade's bonus is added");
	require(enchant_success_rate(90000, bonuses, 4) == 100000, "bonus saturates at 100 percent");
	require(enchant_success_rate(std::numeric_limits<uint32>::max(), bonuses, 4) == 100000, "addition cannot wrap");
	require(enchant_success_rate(100000, bonuses, 255) == 0, "invalid grade rejected");
	for (uint32 rate : {0u, 1u, 10000u, 50000u, 99999u, 100000u}) {
		unsigned successes = 0;
		for (uint32 roll = 1; roll <= 100000; ++roll) successes += enchant_roll_succeeds(rate, roll);
		require(successes == rate, "success and reset rolls have exact endpoints and probabilities");
	}
	require(!enchant_roll_succeeds(100000, 0) && !enchant_roll_succeeds(100000, 100001), "invalid success rolls rejected");

	item selected{};
	selected.amount = 1; selected.identify = 1;
	require(enchant_item_state_valid(selected, false), "ordinary identified inventory item accepted");
	require(!enchant_item_state_valid(selected, true), "creator/forge/pet card metadata protected");
	for (int amount : {0, 2, -1}) {
		selected.amount = amount;
		require(!enchant_item_state_valid(selected, false), "empty or stacked target rejected");
	}
	selected.amount = 1; selected.identify = 0;
	require(!enchant_item_state_valid(selected, false), "unidentified target rejected");
	selected.identify = 1; selected.equip = 1;
	require(!enchant_item_state_valid(selected, false), "equipped target rejected");
	selected.equip = 0; selected.equipSwitch = 1;
	require(!enchant_item_state_valid(selected, false), "equip-switch target rejected");
	selected.equipSwitch = 0; selected.attribute = 1;
	require(!enchant_item_state_valid(selected, false), "broken target rejected");
	selected.attribute = 0; selected.enchantgrade = 255;
	require(!enchant_item_state_valid(selected, false), "invalid item grade rejected");
	std::cout << "PASS: " << checks << " enchant probability/state checks, including exhaustive 100000-point draws\n";
}
