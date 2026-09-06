// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// Compile against the real shared job, skill, and status definitions.
#include <cassert>
#include <cstdio>
#include <iterator>

#include "common/mmo.hpp"
#include "map/map.hpp"
#include "map/pc.hpp"
#include "map/skill.hpp"
#include "map/status.hpp"

static_assert(JOB_DRUID == 4351 && JOB_BABY_DRUID == 4352);
static_assert(JOB_KARNOS == 4353 && JOB_BABY_KARNOS == 4354 && JOB_ALITEA == 4355);
static_assert(MAPID_DRUID == 18);
static_assert((MAPID_KARNOS & MAPID_FIRSTMASK) == MAPID_DRUID);
static_assert((MAPID_ALITEA & MAPID_SECONDMASK) == MAPID_KARNOS);
// Expanded trait jobs use their own lineage bits, not the primary fourth bit.
static_assert(pc_is_trait_job(MAPID_ALITEA));
static_assert(!pc_is_trait_job(MAPID_KARNOS));
static_assert(pc_is_upper_expanded_first(MAPID_KARNOS));
static_assert((MAPID_ALITEA & MAPID_SECONDMASK) == (MAPID_KARNOS & MAPID_SECONDMASK));
static_assert((MAPID_BABY_DRUID & ~JOBL_BABY) == MAPID_DRUID);
static_assert((MAPID_BABY_KARNOS & ~JOBL_BABY) == MAPID_KARNOS);
static_assert(DR_WEREWOLF == 6524 && KR_NASTY_SLASH == 6549);
static_assert(AT_SIXTH_SENSE == 6575 && AT_NATURE_HARMONY == 6607);
static_assert(AT_ROARING_PIERCER == 6597 && AT_ROARING_PIERCER_S == 6598);
static_assert(UNT_ICE_PILLAR == 313 && UNT_GLACIAL_MONOLITH == 314);
static_assert(EFST_WEREWOLF == 1675 && EFST_WERERAPTOR == 1680);
static_assert(EFST_BLOCK == 1688 && EFST_NATURE_LOGIC == 1690);
static_assert(EFST_TASK_FURIOS_STORM == 1722);
// Existing custom status IDs must remain before the appended Druid statuses.
static_assert(SC_WEREWOLF == SC_CONTENTS_38 + 1);
static_assert(SC_FERAL_CLAW < SC_MAX);

int main() {
    mmo_charstatus character{};
    assert(std::size(character.skill) == MAX_SKILL);
    assert(MAX_SKILL >= 1733);
    std::puts("Druid native job/skill/status identities and shared skill capacity passed");
}
