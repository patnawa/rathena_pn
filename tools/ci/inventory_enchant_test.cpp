// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  inventory_enchant_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/inventory_enchant_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Standalone behavioral tests for the helper used by modifyinventoryenchant.
// g++ -std=c++17 -Isrc tools/ci/inventory_enchant_test.cpp -o /tmp/inventory_enchant_test
#include <cstring>
#include <iostream>
#include <limits>
#include "map/inventory_enchant.hpp"

static unsigned checks = 0;
static void require(bool value, const char* message) {
	++checks;
	if (!value) {
		std::cerr << "FAIL: " << message << '\n';
		std::exit(1);
	}
}

int main() {
	item original{};
	original.id = 9182;
	original.nameid = 490136;
	original.amount = 1;
	original.identify = 1;
	original.refine = 20;
	original.attribute = 1;
	original.card[0] = 4001;
	original.card[1] = 0;
	original.card[2] = 310709;
	original.card[3] = 310710;
	original.expire_time = 1900000000;
	original.favorite = 1;
	original.bound = 4;
	original.unique_id = std::numeric_limits<uint64>::max();
	original.equipSwitch = EQP_ACC_L;
	original.enchantgrade = 4;
	for (int i = 0; i < MAX_ITEM_RDM_OPT; ++i) {
		original.option[i].id = i + 1;
		original.option[i].value = -100 + i;
		original.option[i].param = i + 10;
	}
	const std::array<t_itemid, MAX_SLOTS> cards{4001, 0, 310709, 310710};
	for (int slot : {2, 3}) {
		item result{};
		require(inventory_enchant_candidate(original, 490136, original.unique_id,
			cards, 1, slot, cards[slot] + 1, result), "valid enchant accepted");
		item expected = original;
		expected.card[slot]++;
		require(std::memcmp(&expected, &result, sizeof(item)) == 0,
			"every byte except the selected enchant is preserved");
		require(original.card[slot] == cards[slot], "input record not mutated by validation");
	}
	auto reject = [&](const item& current, t_itemid id, uint64 uid,
		const std::array<t_itemid, MAX_SLOTS>& snapshot, int slots, int slot, t_itemid target) {
		item result;
		std::memset(&result, 0xa5, sizeof(result));
		const item sentinel = result;
		require(!inventory_enchant_candidate(current, id, uid, snapshot, slots, slot, target, result),
			"invalid/stale selection rejected");
		require(std::memcmp(&result, &sentinel, sizeof(result)) == 0,
			"rejected candidate leaves result unchanged");
	};
	reject(original, 490137, original.unique_id, cards, 1, 2, 310710);
	reject(original, 490136, original.unique_id - 1, cards, 1, 2, 310710);
	for (int i = 0; i < MAX_SLOTS; ++i) {
		auto stale = cards;
		stale[i]++;
		reject(original, 490136, original.unique_id, stale, 1, 2, 310710);
	}
	for (int slot : {-1, 0, MAX_SLOTS, 100})
		reject(original, 490136, original.unique_id, cards, 1, slot, 310710);
	for (int slots : {-1, MAX_SLOTS + 1})
		reject(original, 490136, original.unique_id, cards, slots, 2, 310710);
	reject(original, 490136, original.unique_id, cards, 1, 2, 0);
	for (int amount : {0, 2}) {
		item changed = original;
		changed.amount = amount;
		reject(changed, 490136, original.unique_id, cards, 1, 2, 310710);
	}
	item changed = original;
	changed.equip = EQP_ACC_L;
	reject(changed, 490136, original.unique_id, cards, 1, 2, 310710);
	changed = original;
	changed.identify = 0;
	reject(changed, 490136, original.unique_id, cards, 1, 2, 310710);
	std::cout << "PASS: " << checks << " inventory enchant behavioral checks\n";
}
