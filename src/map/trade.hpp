// Copyright (c) rAthena Dev Teams - Licensed under GNU GPL
// For more information, see LICENCE in the main folder

#ifndef TRADE_HPP
#define TRADE_HPP

#include <common/cbasetypes.hpp>
#include <custom/bank_protocol.hpp>

class map_session_data;

enum e_ack_trade_response : uint8 {
	TRADE_ACK_TOOFAR = 0,
	TRADE_ACK_CHARNOTEXIST,
	TRADE_ACK_FAILED,
	TRADE_ACK_ACCEPT,
	TRADE_ACK_CANCEL,
	TRADE_ACK_BUSY
};

void trade_traderequest(map_session_data *sd, map_session_data *target_sd);
void trade_tradeack(map_session_data *sd,int32 type);
void trade_tradeadditem(map_session_data *sd,int16 index,int16 amount);
void trade_tradeaddzeny(map_session_data *sd,int64 amount);
void trade_tradeok(map_session_data *sd, bool wide = false);
void trade_tradecancel(map_session_data *sd);
void trade_tradecommit(map_session_data *sd, bool wide = false);
void trade_pair_completed(map_session_data& a,map_session_data& b);
pn_bank::Result trade_wide_action(map_session_data& sd, const pn_bank::Request& request);

#endif /* TRADE_HPP */
