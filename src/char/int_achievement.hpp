// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INT_ACHIEVEMENT_HPP
#define INT_ACHIEVEMENT_HPP

#include <common/cbasetypes.hpp>

int32 inter_achievement_parse_frommap(int32 fd);
struct achievement;
bool mapif_achievement_save_rows(uint32 char_id, const achievement* rows, int32 count);
enum class AchievementCompareReplaceResult : uint8 { Error, Conflict, Applied };
AchievementCompareReplaceResult mapif_achievement_compare_replace_locked(uint32 char_id,
	const achievement* before, int32 before_count,
	const achievement* after, int32 after_count);

#endif /* INT_ACHIEVEMENT_HPP */
