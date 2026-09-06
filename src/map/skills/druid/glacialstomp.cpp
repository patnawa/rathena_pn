// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#include "glacialstomp.hpp"

#include <config/core.hpp>

#include "map/clif.hpp"
#include "map/pc.hpp"
#include "map/status.hpp"
#include "map/unit.hpp"

#include "glacialnova.hpp"
#include "glacialmonolith.hpp"

SkillGlacialStomp::SkillGlacialStomp() : SkillImplRecursiveDamageSplash(AT_GLACIER_STOMP) {
}

void SkillGlacialStomp::calculateSkillRatio(const Damage*, const block_list* src, const block_list*, uint16 skill_lv, int32& skillratio, int32 mflag) const {
	skillratio += -100 + 6400 + 500 * (skill_lv - 1);

	if (const status_change* sc = status_get_sc(src); sc != nullptr && sc->hasSCE(SC_TRUTH_OF_ICE)) {
		const status_data* sstatus = status_get_status_data(*src);

		skillratio += 4 * sstatus->spl;
	}

	// Unlike what the description indicates, the BaseLevel modifier is not part of the condition on SC_TRUTH_OF_ICE
	RE_LVL_DMOD(100);
}

void SkillGlacialStomp::castendNoDamageId(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32& flag) const {
	status_change* sc = status_get_sc(src);

	if (sc == nullptr) {
		if (map_session_data* sd = BL_CAST(BL_PC, src); sd != nullptr) {
			clif_skill_fail(*sd, getSkillId(), USESKILL_FAIL);
		}
		return;
	}

	if (!sc->hasSCE(SC_GLACIER_SHEILD)) {
		if (map_session_data* sd = BL_CAST(BL_PC, src); sd != nullptr) {
			clif_skill_fail(*sd, getSkillId(), USESKILL_FAIL);
		}
		return;
	}

	const skill_unit* monolith = druid_find_active_monolith(src);
	if (monolith == nullptr) {
		if (map_session_data* sd = BL_CAST(BL_PC, src); sd != nullptr) {
			clif_skill_fail(*sd, getSkillId(), USESKILL_FAIL);
		}
		return;
	}

	// TODO : the player should be teleported to one cell from the center
	const int16 monolith_x = monolith->x;
	const int16 monolith_y = monolith->y;
	if (!unit_movepos(src, monolith_x, monolith_y, 2, true)) {
		if (map_session_data* sd = BL_CAST(BL_PC, src); sd != nullptr) {
			clif_skill_fail(*sd, getSkillId(), USESKILL_FAIL);
		}
		return;
	}

	clif_fixpos(*src);
	clif_skill_nodamage(src, *target, getSkillId(), skill_lv);

	this->castendDamageId(src, target, skill_lv, tick, flag);
}

void SkillGlacialStomp::splashSearch(block_list* src, block_list* target, uint16 skill_lv, t_tick tick, int32 flag) const {
	SkillImplRecursiveDamageSplash::splashSearch(src, target, skill_lv, tick, flag);

	SkillGlacialNova skillnova;
	skillnova.castendPos2(src, 0, 0, 1, tick, flag);
}
