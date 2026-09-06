// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "glacialnova.hpp"

#include <config/core.hpp>

#include "map/clif.hpp"
#include "map/status.hpp"

#include "glacialmonolith.hpp"

SkillGlacialNova::SkillGlacialNova() : SkillImplRecursiveDamageSplash(AT_GLACIER_NOVA) {
}

void SkillGlacialNova::calculateSkillRatio(const Damage*, const block_list* src, const block_list*, uint16 skill_lv, int32& skillratio, int32 mflag) const {
	const status_data* sstatus = status_get_status_data(*src);

	skillratio += -100 + 15000;

	// SPL and BaseLevel ratio do not depend on SC_TRUTH_OF_ICE
	skillratio += 15 * sstatus->spl;

	RE_LVL_DMOD(100);
}

void SkillGlacialNova::castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const {
	status_change* sc = status_get_sc(src);

	if (sc == nullptr)
		return;

	if (!sc->hasSCE(SC_GLACIER_SHEILD))
		return;

	const skill_unit* monolith = druid_find_active_monolith(src);
	if (monolith == nullptr)
		return;

	const int16 monolith_x = monolith->x;
	const int16 monolith_y = monolith->y;
	clif_skill_poseffect(*src, getSkillId(), skill_lv, monolith_x, monolith_y, tick);
	SkillImplRecursiveDamageSplash::castendPos2(src, monolith_x, monolith_y, skill_lv, tick, flag);
}
