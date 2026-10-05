// PN account bank. GPL-3.0-or-later. Shared by the server and Windows client.
#ifndef PN_BANK_PROTOCOL_HPP
#define PN_BANK_PROTOCOL_HPP
#include <cstdint>
#include <algorithm>
#include <limits>

namespace pn_bank {
constexpr uint32_t magic = 0x314b4250; // PBK1, on a separate authenticated socket.
constexpr uint16_t version = 3;
constexpr int64_t bank_limit = INT64_MAX;
constexpr int64_t wallet_limit = INT64_MAX;
constexpr uint32_t open_panel = 1;
constexpr uint32_t item_ids[2] = {6024, 12781};
constexpr uint32_t buy_prices[2] = {501000000, 1002000};
constexpr uint32_t sell_prices[2] = {499000000, 998000};
// Legacy exchange IDs remain reserved so old clients cannot reinterpret requests.
enum Action : uint32_t { Refresh, Deposit, Withdraw, BuyDiamond, SellDiamond, BuyNote, SellNote, TradeSetOffer, TradeLock, TradeCommit, TradeCancel, CollectOffline };
enum Result : uint32_t { Ok, Saving, Unauthorized, Invalid, Busy, Unavailable, Funds, Capacity, Limit, Items, Stale, SaveFailed };
#pragma pack(push, 1)
struct alignas(8) Request {
    uint32_t magic_value = magic;
    uint16_t protocol = version, length = 80;
    uint32_t account_id = 0, char_id = 0, login_id1 = 0, login_id2 = 0;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    int64_t amount = 0;
    uint32_t action = Refresh, reserved = 0;
    uint64_t trade_id = 0, trade_revision = 0;
};
struct alignas(8) Reply {
    uint32_t magic_value = magic;
    uint16_t protocol = version, length = 208;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint32_t result = Unauthorized, flags = 0;
    int64_t bank = 0, wallet = 0, bank_limit = pn_bank::bank_limit, wallet_limit = pn_bank::wallet_limit;
    uint32_t counts[2] = {}, buy[2] = {buy_prices[0], buy_prices[1]}, sell[2] = {sell_prices[0], sell_prices[1]};
    uint32_t max_buy[2] = {}, max_sell[2] = {};
    int64_t max_deposit = 0, max_withdraw = 0;
    uint32_t char_id = 0, reserved = 0;
    uint32_t partner_account_id = 0, partner_char_id = 0;
    char partner_name[24] = {};
    int64_t own_offer = 0, partner_offer = 0;
    uint32_t own_trade_state = 0, partner_trade_state = 0;
    uint64_t trade_id = 0, trade_revision = 0;
};
#pragma pack(pop)
static_assert(sizeof(Request) == 80, "Bank request ABI");
static_assert(sizeof(Reply) == 208, "Bank reply ABI");

inline bool valid_reply(const Reply& state) {
    if (state.magic_value != magic || state.protocol != version || state.length != sizeof(Reply) ||
        (state.flags & ~open_panel) || state.reserved || state.result > SaveFailed ||
        state.bank_limit != bank_limit || state.wallet_limit != wallet_limit ||
        state.bank < 0 || state.wallet < 0 || state.wallet > wallet_limit ||
        state.max_deposit < 0 || state.max_deposit > std::min(state.wallet, bank_limit - state.bank) ||
        state.max_withdraw < 0 || state.max_withdraw > std::min(state.bank, wallet_limit - state.wallet)) return false;
    for (int i = 0; i < 2; ++i)
        if (state.buy[i] != buy_prices[i] || state.sell[i] != sell_prices[i] ||
            state.max_buy[i] > static_cast<uint64_t>(state.bank / buy_prices[i]) ||
            state.max_sell[i] > state.counts[i] ||
            state.max_sell[i] > static_cast<uint64_t>((bank_limit - state.bank) / sell_prices[i])) return false;
    if (state.own_offer < 0 || state.partner_offer < 0 || state.own_trade_state > 2 || state.partner_trade_state > 2) return false;
    if (state.trade_id) {
        if (!state.trade_revision || !state.partner_account_id || !state.partner_char_id || !state.partner_name[0] || state.partner_name[23]) return false;
    } else if (state.trade_revision || state.partner_account_id || state.partner_char_id || state.own_offer || state.partner_offer || state.own_trade_state || state.partner_trade_state) return false;
    return true;
}

struct Plan {
    Result result = Invalid;
    int64_t bank = 0, wallet = 0;
    int item = -1;
    int32_t item_delta = 0;
};

// All arithmetic is checked before any inventory, wallet, or SQL mutation.
inline Plan plan(const Reply& state, uint32_t action, int64_t amount) {
    Plan out; out.bank = state.bank; out.wallet = state.wallet;
    if (state.bank < 0 || state.wallet < 0 || state.wallet > wallet_limit || amount <= 0 || state.trade_id)
        return out;
    if (action == Deposit) {
        if (amount > state.wallet) { out.result = Funds; return out; }
        if (amount > bank_limit - state.bank) { out.result = Limit; return out; }
        out.bank += amount; out.wallet -= amount;
    } else if (action == Withdraw) {
        if (amount > state.bank) { out.result = Funds; return out; }
        if (amount > wallet_limit - state.wallet) { out.result = Limit; return out; }
        out.bank -= amount; out.wallet += amount;
    } else return out;
    out.result = Ok;
    return out;
}
inline const char* message(uint32_t result) {
    switch (result) {
    case Ok: return "Choose a banking action.";
    case Saving: return "Saving transaction. Please wait...";
    case Unauthorized: return "Log in to a character to use the bank.";
    case Invalid: return "Enter a valid whole amount greater than zero.";
    case Busy: return "Finish your current trade, shop, or NPC dialog first.";
    case Unavailable: return "Banking is unavailable here. Please try again later.";
    case Funds: return "You do not have enough zeny for this action.";
    case Capacity: return "There is not enough inventory space or weight capacity.";
    case Limit: return "This amount would exceed the wallet or bank limit.";
    case Items: return "You do not have enough eligible items to sell.";
    case Stale: return "Your session changed. Refresh before trying again.";
    default: return "The transaction is waiting for the character server to save.";
    }
}
}
#endif
