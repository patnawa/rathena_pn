// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
#ifndef INVENTORY_ENCHANT_HPP
#define INVENTORY_ENCHANT_HPP

#include <array>
#include <common/mmo.hpp>

// Construct a replacement value without deleting or recreating the inventory
// record. Keep this pure so metadata preservation and stale-selection rejection
// can be exercised without a running map-server or a player's inventory.
inline bool inventory_enchant_candidate(const item& current, t_itemid expected_id,
	uint64 expected_unique_id, const std::array<t_itemid, MAX_SLOTS>& expected_cards,
	int32 intrinsic_slots, int32 slot, t_itemid enchant_id, item& result) {
	if (current.nameid != expected_id || current.unique_id != expected_unique_id ||
		current.amount != 1 || current.equip != 0 || !current.identify ||
		intrinsic_slots < 0 || intrinsic_slots > MAX_SLOTS || slot < intrinsic_slots ||
		slot < 0 || slot >= MAX_SLOTS || enchant_id == 0) {
		return false;
	}
	for (int32 i = 0; i < MAX_SLOTS; ++i) {
		if (current.card[i] != expected_cards[i]) {
			return false;
		}
	}
	result = current;
	result.card[slot] = enchant_id;
	return true;
}

#endif
