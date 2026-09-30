// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INT_STORAGE_HPP
#define INT_STORAGE_HPP

#include <common/cbasetypes.hpp>

struct s_storage;

void inter_storage_sql_init(void);
void inter_storage_sql_final(void);

bool inter_storage_parse_frommap(int32 fd);
bool pn_global_point_pending(uint32 account_id);

bool guild_storage_tosql(int32 guild_id, struct s_storage *p);

#endif /* INT_STORAGE_HPP */
