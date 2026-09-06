// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "glacialmonolith.hpp"

#include <config/core.hpp>

#include "map/status.hpp"
#include "map/unit.hpp"

const skill_unit* druid_find_active_monolith(const block_list* src) {
	if (src == nullptr || src->prev == nullptr || status_isdead(*src))
		return nullptr;

	const unit_data* ud = unit_bl2ud(src);
	if (ud == nullptr)
		return nullptr;

	for (const auto& group : ud->skillunits) {
		if (group == nullptr || group->skill_id != AT_GLACIER_MONOLITH ||
			group->src_id != src->id || group->map != src->m || group->unit == nullptr)
			continue;

		for (int32 i = 0; i < group->unit_count; ++i) {
			const skill_unit* unit = &group->unit[i];
			if (unit->alive && unit->prev != nullptr && unit->m == src->m && unit->range >= 0 &&
				distance_xy(src->x, src->y, unit->x, unit->y) <= unit->range)
				return unit;
		}
	}
	return nullptr;
}

SkillGlacialMonolith::SkillGlacialMonolith() : SkillImplRecursiveDamageSplash(AT_GLACIER_MONOLITH) {
}

void SkillGlacialMonolith::calculateSkillRatio(const Damage*, const block_list* src, const block_list*, uint16 skill_lv, int32& skillratio, int32 mflag) const {
	skillratio += -100 + 7100 + 300 * (skill_lv - 1);

	if (const status_change* sc = status_get_sc(src); sc != nullptr && sc->hasSCE(SC_TRUTH_OF_ICE)) {
		const status_data* sstatus = status_get_status_data(*src);

		skillratio += 10 * sstatus->spl;
	}

	// Unlike what the description indicates, the BaseLevel modifier is not part of the condition on SC_TRUTH_OF_ICE
	RE_LVL_DMOD(100);
}

void SkillGlacialMonolith::castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const {
	// Damage
	SkillImplRecursiveDamageSplash::castendPos2(src, x, y, skill_lv, tick, flag);

	// Place the UNT which give SC_GLACIER_SHEILD to the caster in range
	skill_unitsetting(src, getSkillId(), skill_lv, x, y, 0);
}
