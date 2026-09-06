// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
#ifndef ENCHANT_UPGRADE_HPP
#define ENCHANT_UPGRADE_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include <common/mmo.hpp>

struct s_item_enchant_random_upgrade {
	t_itemid item_id;
	uint32 chance;
};

struct s_item_enchant_upgrade {
	t_itemid enchant_item_id;
	t_itemid upgrade_item_id;
	std::vector<s_item_enchant_random_upgrade> random_upgrades;
	uint32 zeny;
	std::unordered_map<t_itemid, uint16> materials;
};

using EnchantUpgradeMap = std::unordered_map<t_itemid, std::shared_ptr<s_item_enchant_upgrade>>;
using PerfectEnchantUpgradeMap = std::unordered_map<t_itemid, EnchantUpgradeMap>;

// Never fall back between request modes: the same source enchant can have
// different costs and outcomes for random and guaranteed upgrades.
inline std::shared_ptr<s_item_enchant_upgrade> select_enchant_upgrade(
	const EnchantUpgradeMap& ordinary, const PerfectEnchantUpgradeMap& perfect,
	t_itemid source, bool perfect_request, t_itemid target) {
	if (perfect_request) {
		if (target == 0) return nullptr;
		const auto sources = perfect.find(source);
		if (sources == perfect.end()) return nullptr;
		const auto recipe = sources->second.find(target);
		return recipe == sources->second.end() ? nullptr : recipe->second;
	}
	if (target != 0) return nullptr;
	const auto recipe = ordinary.find(source);
	return recipe == ordinary.end() ? nullptr : recipe->second;
}

inline t_itemid select_enchant_upgrade_result(const s_item_enchant_upgrade& recipe, uint32 roll) {
	if (recipe.random_upgrades.empty()) return recipe.upgrade_item_id;
	if (roll < 1 || roll > 100000) return 0;
	uint64 cumulative = 0;
	for (const auto& outcome : recipe.random_upgrades) {
		cumulative += outcome.chance;
		if (roll <= cumulative) return outcome.item_id;
	}
	return 0;
}

#endif
