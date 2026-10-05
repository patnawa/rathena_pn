#pragma once
#include "../wide_zeny/bank_client.hpp"
#include "market_protocol.hpp"
constexpr UINT MARKET_RESULT=WM_APP+71;
struct MarketResult {pn_market::Reply state;LONG generation=0;bool connected=false;};
bool market_submit(HWND panel,pn_market::Request request);
void market_open(HWND owner,const pn_bank::Reply& bank);
void market_forget_session();
