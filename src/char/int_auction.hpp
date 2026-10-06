// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef INT_AUCTION_HPP
#define INT_AUCTION_HPP

#include <common/cbasetypes.hpp>
#include <vector>
namespace pn_shop {struct Commit;}
struct mail_message;
// -1: SQL error/retry, 0: business rejection, 1: outputs staged in caller transaction.
int pn_auction_apply_locked(const pn_shop::Commit&,std::vector<mail_message>&);
bool pn_auction_reload();

int32 inter_auction_parse_frommap(int32 fd);

int32 inter_auction_sql_init(void);
void inter_auction_sql_final(void);

#endif /* INT_AUCTION_HPP */
