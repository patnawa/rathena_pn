// Durable pet output identity. Map/character integration must preserve this ABI.
#ifndef PN_PET_ENTITLEMENT_HPP
#define PN_PET_ENTITLEMENT_HPP
#include <common/mmo.hpp>
#include <cstdint>
#include <cstring>

namespace pn_pet {
constexpr uint16_t output_version = 1;
constexpr t_itemid egg_marker = 0x0100; // CARD0_PET in map/itemdb.hpp.
#pragma pack(push,1)
struct Key {
    uint32_t account_id=0, char_id=0;
    uint64_t nonce_hi=0, nonce_lo=0, sequence=0;
    uint16_t ordinal=0;
};
struct Output {
    uint16_t version=output_version;
    item egg{};
    int16_t pet_class=0, level=0, intimacy=0, hungry=100;
    char name[NAME_LENGTH]{};
};
#pragma pack(pop)
struct Entitlement {
    uint64_t id=0;
    uint32_t pet_id=0;
    item egg{};
    bool claimed=false;
};
enum class Result { Error, Conflict, Issued, Pending, ClaimedNow, AlreadyClaimed, RetiredNow };

inline bool valid(const Key& key,const Output& output) {
    if(!key.account_id || !key.char_id || !(key.nonce_hi|key.nonce_lo) || !key.sequence ||
       key.ordinal>=MAX_INVENTORY || output.version!=output_version ||
       output.pet_class<=0 || output.level<=0 || output.intimacy<0 || output.intimacy>1000 ||
       output.hungry<0 || output.hungry>100 || !output.name[0] ||
       !std::memchr(output.name,0,sizeof(output.name)))return false;
    const auto& egg=output.egg;
    if(!egg.nameid || egg.amount!=1 || egg.id || egg.equip || egg.equipSwitch ||
       egg.card[0] || egg.card[1] || egg.card[2] || egg.card[3])return false;
    return true;
}

// All calls require an active Sql_BeginTransaction(sql_handle). They never
// begin, commit or roll back. The caller must atomically persist its payment,
// inventory and request receipt, rolling back on Error/Conflict. A replayed
// output must not cause another debit or another inventory installation.
// These helpers are not an authorization boundary: the outer request handler
// must authenticate the current map owner and validate the whole request.
Result issue_locked(const Key&,const Output&,Entitlement&);
Result claim_locked(uint32_t account_id,uint32_t char_id,uint64_t entitlement_id,
                    const item& expected);
Result retire_locked(uint32_t char_id,uint32_t pet_id,uint32_t egg_id);
}
#endif
