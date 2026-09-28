// Internal map/character paired economic commit. Upgrade both processes together.
#ifndef PN_PAIR_COMMIT_HPP
#define PN_PAIR_COMMIT_HPP
#include <common/mmo.hpp>
#include <cstdint>
namespace pn_pair {
enum Kind : uint32_t { Trade=1, Vending=2, Buying=3 };
constexpr size_t listing_capacity=20;
constexpr size_t item_capacity=MAX_INVENTORY>MAX_CART?MAX_INVENTORY:MAX_CART;
#pragma pack(push,1)
struct Side {
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0;
    int64_t wallet_before=0,wallet_after=0,bank_before=0,bank_after=0;
    item items[item_capacity]{};
};
// cart_id is the inventory row id for vending and the item id for buying stores.
struct Listing {uint32_t cart_id=0,amount=0;int64_t price=0;};
struct Commit {
    uint16_t packet=0x3096,length=0;
    uint32_t kind=Trade;
    uint64_t sequence=0;
    int64_t fee=0;
    Side side[2]{};
    uint32_t vending_id=0,listing_count=0;
    int64_t buying_budget=0;
    Listing listings[listing_capacity]{};
};
struct Ack {
    uint16_t packet=0x3896;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,sequence=0;
    uint32_t committed=0;
};
#pragma pack(pop)
static_assert(sizeof(Commit)<65536,"Paired inventory packet exceeds inter-server frame; split protocol before increasing capacity");
static_assert(sizeof(Ack)==38,"Paired commit acknowledgement ABI");
// Sum up to four nonnegative int64 balances without overflow, represented as
// a two-limb unsigned integer. No floating point or compiler-specific integer.
struct Total {uint64_t low=0,high=0;void add(uint64_t n){uint64_t previous=low;low+=n;if(low<previous)++high;}};
inline bool conserved(const Commit& c) {
    if ((c.kind!=Trade && c.kind!=Vending && c.kind!=Buying) || c.fee<0 || (c.kind!=Vending && c.fee) || c.buying_budget<0) return false;
    Total before,after;
    for(const auto& s:c.side) {
        if(s.wallet_before<0 || s.wallet_after<0 || s.bank_before<0 || s.bank_after<0)return false;
        before.add(s.wallet_before);before.add(s.bank_before);
        after.add(s.wallet_after);after.add(s.bank_after);
    }
    after.add(c.fee);
    if(c.kind!=Vending && (c.side[0].bank_before!=c.side[0].bank_after || c.side[1].bank_before!=c.side[1].bank_after))return false;
    return before.low==after.low && before.high==after.high;
}
}
#endif
