// Independent PN market protocol. Native game packet layouts are unchanged.
#ifndef PN_MARKET_PROTOCOL_HPP
#define PN_MARKET_PROTOCOL_HPP
#include <cstdint>
#include <limits>
namespace pn_market {
constexpr uint32_t magic = 0x314b4d50; // PMK1
constexpr uint16_t version = 1;
constexpr uint32_t page_size = 20;
enum Kind : uint32_t { Vending, Buying };
enum Action : uint32_t { InspectOwn, ListShop, Search, SetPrice, SetBudget, Purchase, Sell, Publish, Close };
enum Result : uint32_t { Ok, Unauthorized, Invalid, Busy, Missing, Stale, Funds, Capacity, Unavailable, Saving };
enum Flags : uint32_t { Draft = 1, Published = 2 };
#pragma pack(push, 1)
struct alignas(8) Request {
    uint32_t magic_value = magic;
    uint16_t protocol = version, length = 128;
    uint32_t account_id = 0, char_id = 0, login_id1 = 0, login_id2 = 0;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint32_t action = InspectOwn, kind = Vending, index = 0, quantity = 0;
    uint32_t target_account = 0, shop_id = 0, cursor = 0, item_id = 0;
    uint64_t revision = 0;
    int64_t expected_price = 0, budget = 0, min_price = 0, max_price = 0;
    uint64_t item_unique_id = 0;
};
struct Option { int16_t id = 0, value = 0; int8_t param = 0; };
struct alignas(8) Entry {
    uint32_t index = 0, item_id = 0, quantity = 0;
    uint16_t refine = 0, grade = 0;
    int64_t price = 0;
    uint64_t unique_id = 0;
    char name[48] = {};
    uint32_t cards[4] = {};
    Option options[5] = {};
    uint8_t reserved[7] = {};
};
struct alignas(8) Reply {
    uint32_t magic_value = magic;
    uint16_t protocol = version, length = 2760;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint32_t result = Unauthorized, flags = 0, count = 0, next_cursor = 0, has_more = 0;
    uint32_t kind = Vending, owner_account = 0, owner_char = 0, shop_id = 0;
    uint64_t revision = 0;
    int64_t wallet = 0, budget = 0;
    char title[80] = {}, owner_name[24] = {};
    uint32_t reserved = 0;
    Entry entries[page_size] = {};
};
#pragma pack(pop)
static_assert(sizeof(Request) == 128, "Market request ABI");
static_assert(sizeof(Entry) == 128, "Market listing ABI");
static_assert(sizeof(Reply) == 2760, "Market reply ABI");
inline bool valid_reply(const Reply& reply) {
    if (reply.magic_value != magic || reply.protocol != version || reply.length != sizeof(Reply) ||
        reply.result > Saving || reply.flags > Published || reply.count > page_size ||
        reply.has_more > 1 || reply.kind > Buying || reply.wallet < 0 || reply.budget < 0 || reply.reserved) return false;
    for (uint32_t i=0;i<reply.count;++i) {
        const auto& row=reply.entries[i];
        if (!row.item_id || !row.quantity || row.price < 0) return false;
        for (auto byte : row.reserved) if (byte) return false;
    }
    return true;
}
// SetPrice: expected_price is the current quote; budget carries the NEW price.
// SetBudget: budget is the new total buying limit. Mutations bind shop_id/revision.
// Search returns one shop at a time, sorted by account id; next_cursor advances
// to the next owner. ListShop/InspectOwn page listings using cursor as slot offset.
// Native-created shops start Draft; no purchase is possible until Publish.
}
#endif
