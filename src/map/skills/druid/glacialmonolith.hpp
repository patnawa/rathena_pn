// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#pragma once

#include "../skill_impl.hpp"

// Resolve the caster's currently live Monolith and enforce its actual area.
// A lingering Glacier Shield status alone is not proof that the unit exists.
const skill_unit* druid_find_active_monolith(const block_list* src);

class SkillGlacialMonolith : public SkillImplRecursiveDamageSplash {
public:
	SkillGlacialMonolith();

	void calculateSkillRatio(const Damage*, const block_list* src, const block_list*, uint16 skill_lv, int32& base_skillratio, int32 mflag) const override;
	void castendPos2(block_list* src, int32 x, int32 y, uint16 skill_lv, t_tick tick, int32& flag) const override;
};
