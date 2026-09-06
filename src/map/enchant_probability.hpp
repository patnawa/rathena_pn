// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
#ifndef ENCHANT_PROBABILITY_HPP
#define ENCHANT_PROBABILITY_HPP

#include <memory>
#include <unordered_map>
#include <common/mmo.hpp>

struct s_item_enchant_normal_sub {
	t_itemid item_id;
	uint32 chance;
};

struct s_item_enchant_normal {
	uint16 enchantgrade;
	std::unordered_map<t_itemid, std::shared_ptr<s_item_enchant_normal_sub>> enchants;
};

inline bool enchant_item_state_valid(const item& selected, bool special_card_data) {
	return selected.amount == 1 && selected.identify && !selected.equip &&
		!selected.equipSwitch && !selected.attribute && !special_card_data &&
		selected.enchantgrade <= MAX_ENCHANTGRADE;
}

// SetGradeBonus assigns the bonus for this grade, not a cumulative bonus.
inline uint32 enchant_success_rate(uint32 base, const std::unordered_map<uint16, uint32>& bonuses,
	uint16 grade) {
	if (grade > MAX_ENCHANTGRADE) return 0;
	const auto bonus = bonuses.find(grade);
	const uint64 total = static_cast<uint64>(base) + (bonus == bonuses.end() ? 0 : bonus->second);
	return total > 100000 ? 100000 : static_cast<uint32>(total);
}

// Exactly 100000 equiprobable draws: 0% never succeeds and 100% never fails.
inline bool enchant_roll_succeeds(uint32 chance, uint32 roll) {
	return roll >= 1 && roll <= 100000 && roll <= chance;
}

// Validate the complete distribution before selecting a result. Returning zero
// rejects malformed/empty tables before charging; never fall back to uniform odds.
inline t_itemid select_normal_enchant_result(const s_item_enchant_normal& table, uint32 roll) {
	if (roll < 1 || roll > 100000) return 0;
	uint64 total = 0;
	for (const auto& entry : table.enchants) {
		if (!entry.second || entry.second->item_id == 0 || entry.first != entry.second->item_id ||
			entry.second->chance > 100000) return 0;
		total += entry.second->chance;
	}
	if (total != 100000) return 0;
	uint32 cumulative = 0;
	for (const auto& entry : table.enchants) {
		cumulative += entry.second->chance;
		if (roll <= cumulative) return entry.second->item_id;
	}
	return 0;
}

#endif
