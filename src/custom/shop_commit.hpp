// Immutable map/character NPC stock purchase. Upgrade both services together.
#ifndef PN_SHOP_COMMIT_HPP
#define PN_SHOP_COMMIT_HPP
#include <common/mmo.hpp>
#include <cstdint>
#include <cstring>
#include <limits>
namespace pn_shop {
enum Kind : uint32_t { Market=1, Barter=2, Sale=3 };
enum Outcome : uint32_t { Retry=0, Committed=1, Rejected=2 };
constexpr size_t stock_capacity=MAX_INVENTORY;
constexpr size_t stock_name_size=50;
#pragma pack(push,1)
struct Stock {
    char name[stock_name_size]{}; // NPC exname; empty for Sale.
    uint32_t key=0; // Market/Sale item id; Barter index (zero is valid).
    int64_t before=0,after=0;
    int64_t sale_start=0,sale_end=0; // Unix seconds; Sale only.
    uint32_t price=0; // Market price, retained when a legacy stock row is absent.
    uint8_t flag=0; // Market flag.
};
struct Commit {
    uint16_t packet=0x3098,length=0;
    uint32_t kind=Market,account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,sequence=0;
    int64_t wallet_before=0,wallet_after=0;
    int64_t cash_before=0,cash_after=0,kafra_before=0,kafra_after=0;
    uint32_t counter_before=0,counter_after=0;
    uint32_t stock_count=0;
    item items[MAX_INVENTORY]{};
    Stock stocks[stock_capacity]{};
};
struct Ack {
    uint16_t packet=0x3898;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,sequence=0;
    uint32_t outcome=Retry;
};
#pragma pack(pop)
static_assert(sizeof(Commit)<65536,"NPC stock purchase exceeds inter-server frame");
static_assert(sizeof(Ack)==38,"NPC stock acknowledgement ABI");
inline bool valid(const Commit& r) {
    if(r.packet!=0x3098 || r.length!=sizeof(r) || r.kind<Market || r.kind>Sale ||
       !r.account_id || !r.char_id || !(r.nonce_hi|r.nonce_lo) || !r.sequence ||
       !r.stock_count || r.stock_count>stock_capacity || r.counter_after<r.counter_before ||
       r.wallet_before<0 || r.wallet_after<0 || r.wallet_after>r.wallet_before ||
       r.cash_before<0 || r.cash_before>INT32_MAX || r.cash_after<0 || r.cash_after>r.cash_before ||
       r.kafra_before<0 || r.kafra_before>INT32_MAX || r.kafra_after<0 || r.kafra_after>r.kafra_before)
        return false;
    if(r.kind!=Sale && (r.cash_after!=r.cash_before || r.kafra_after!=r.kafra_before))return false;
    if(r.kind==Sale && r.wallet_after!=r.wallet_before)return false;
    for(const auto& it:r.items)if((it.nameid && (it.amount<1 || it.amount>MAX_AMOUNT)) || (!it.nameid && it.amount))return false;
    for(uint32_t i=0;i<r.stock_count;++i){
        const auto& s=r.stocks[i];
        if(!std::memchr(s.name,0,sizeof(s.name)) || s.before<1 || s.after<0 || s.after>=s.before ||
           s.before-s.after>MAX_AMOUNT || s.before>INT32_MAX)return false;
        if(r.kind==Sale){if(s.name[0] || !s.key || s.sale_start<0 || s.sale_end<=s.sale_start)return false;}
        else if(!s.name[0] || s.sale_start || s.sale_end || (r.kind==Market && !s.key) ||
                (r.kind==Barter && (s.key>UINT16_MAX || s.before>UINT16_MAX)))return false;
        for(uint32_t j=0;j<i;++j)if(s.key==r.stocks[j].key && !std::strcmp(s.name,r.stocks[j].name))return false;
    }
    return true;
}
}
#endif
