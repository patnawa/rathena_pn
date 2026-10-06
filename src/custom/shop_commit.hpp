// Immutable map/character NPC stock purchase. Upgrade both services together.
#ifndef PN_SHOP_COMMIT_HPP
#define PN_SHOP_COMMIT_HPP
#include <common/mmo.hpp>
#include <custom/pet_entitlement.hpp>
#include <custom/shop_progression.hpp>
#include <custom/zeny_arithmetic.hpp>
#include <cstdint>
#include <cstring>
#include <limits>
namespace pn_shop {
enum Kind : uint32_t { Market=1, Barter=2, Sale=3, Asset=4, PetClaim=5, ItemUse=6, MailSend=7 };
constexpr uint16_t protocol_version=3;
enum Outcome : uint32_t { Retry=0, Committed=1, Rejected=2, ProgressionStale=3 };
enum Response : uint16_t { Automatic=0, BarterResponse=1, ShopResponse=2, MarketResponse=3, CashNpcResponse=4, CashButtonResponse=5 };
constexpr int32_t cash_pending=254; // Internal only: never send as a client result.
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
struct PetChange {
    uint64_t claim_id=0; // Zero issues an entitlement; nonzero claims one.
    int16_t inventory_index=-1; // Only claims install an egg into inventory.
    pn_pet::Output output{};
};
struct PetRetirement { uint32_t pet_id=0,egg_id=0; };
enum PointScope : uint8_t { NoPoint=0, CharacterPoint=1, AccountPoint=2, SessionPoint=3, GlobalPoint=4 };
struct PointDebit { uint8_t scope=NoPoint; char key[32]{}; int64_t before=0,after=0; };
// MailSend has no NPC stock rows. Its tagged envelope occupies the same wire
// region, preserving historical receipt bytes and the v3 Commit frame size.
struct MailEnvelope {
    char recipient[NAME_LENGTH]{}, title[MAIL_TITLE_LENGTH]{}, body[MAIL_BODY_LENGTH]{};
    int64_t zeny=0;
    uint32_t fee_percent=0,attachment_price=0;
    item attachments[MAIL_MAX_ITEM]{};
};
static_assert(sizeof(MailEnvelope)<=sizeof(Stock)*stock_capacity,"Mail envelope exceeds stock region");
struct Commit {
    uint16_t packet=0x3098,length=0;
    uint32_t kind=Market,account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,sequence=0;
    int64_t wallet_before=0,wallet_after=0;
    int64_t cash_before=0,cash_after=0,kafra_before=0,kafra_after=0;
    uint32_t counter_before=0,counter_after=0;
    uint32_t stock_count=0;
    uint16_t version=protocol_version,pet_count=0,response=Automatic,pet_retire_count=0;
    item items[MAX_INVENTORY]{};
    union { Stock stocks[stock_capacity]{}; MailEnvelope outgoing; };
    PetChange pets[MAX_INVENTORY]{};
    PetRetirement retired_pets[MAX_INVENTORY]{};
    PointDebit point{};
    uint32_t mail_id=0;
    int64_t mail_zeny=0;
    item mail_items[MAIL_MAX_ITEM]{};
    uint32_t progression_size=0; // Exact companion frame length; zero means no durable progression change.
};
struct Ack {
    uint16_t packet=0x3898;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,sequence=0;
    uint32_t outcome=Retry;
};
#pragma pack(pop)
static_assert(sizeof(Commit)<65536,"NPC stock purchase exceeds inter-server frame");
static_assert(sizeof(Commit)==59958,"Update durable shop payload migrations when Commit changes");
static_assert(sizeof(Ack)==38,"NPC stock acknowledgement ABI");
inline bool valid(const Commit& r) {
    if(r.packet!=0x3098 || r.length!=sizeof(r) || r.version!=protocol_version || r.kind<Market || r.kind>MailSend ||
       !r.account_id || !r.char_id || !(r.nonce_hi|r.nonce_lo) || !r.sequence ||
       (r.kind<=Sale && !r.stock_count) || r.stock_count>stock_capacity || r.counter_after<r.counter_before ||
       r.pet_count>MAX_INVENTORY || r.pet_retire_count>MAX_INVENTORY || r.response>CashButtonResponse ||
       r.progression_size>UINT16_MAX || (r.progression_size && r.progression_size<sizeof(pn_shop_progression::Header)) || (r.kind>=Asset && r.stock_count) ||
       r.wallet_before<0 || r.wallet_after<0 || (!r.mail_id && r.kind!=ItemUse && r.wallet_after>r.wallet_before) ||
       r.cash_before<0 || r.cash_before>INT32_MAX || r.cash_after<0 || r.cash_after>INT32_MAX || (r.kind!=ItemUse && r.cash_after>r.cash_before) ||
       r.kafra_before<0 || r.kafra_before>INT32_MAX || r.kafra_after<0 || r.kafra_after>INT32_MAX || (r.kind!=ItemUse && r.kafra_after>r.kafra_before))
        return false;
    const auto& point=r.point;
    if(point.scope) {
        if(r.kind!=Asset || r.mail_id || point.scope>GlobalPoint || !point.key[0] ||
           !std::memchr(point.key,0,sizeof(point.key)) || point.before<1 || point.after<0 || point.after>=point.before ||
           r.cash_before!=r.cash_after || r.kafra_before!=r.kafra_after || r.wallet_before!=r.wallet_after)return false;
        const size_t length=std::strlen(point.key);
        if(point.key[length-1]=='$' || point.key[0]=='$' || point.key[0]=='.' || point.key[0]=='\'')return false;
        if(point.scope==CharacterPoint && (point.key[0]=='#' || point.key[0]=='@'))return false;
        if(point.scope==AccountPoint && (point.key[0]!='#' || point.key[1]=='#' || length<2 ||
           !std::strcmp(point.key,"#CASHPOINTS") || !std::strcmp(point.key,"#KAFRAPOINTS")))return false;
        if(point.scope==SessionPoint && (point.key[0]!='@' || length<2))return false;
        if(point.scope==GlobalPoint && (point.key[0]!='#' || point.key[1]!='#' || length<3))return false;
    }else if(point.key[0] || point.before || point.after)return false;
    if(r.mail_id) {
        if(r.kind!=Asset || r.mail_id>INT32_MAX || r.mail_zeny<0 ||
           r.mail_zeny>INT64_MAX-r.wallet_before || r.wallet_after!=r.wallet_before+r.mail_zeny ||
           r.cash_before!=r.cash_after || r.kafra_before!=r.kafra_after || r.pet_retire_count)return false;
        bool any=false;
        for(const auto& it:r.mail_items) {
            if(it.nameid) {if(it.amount<1 || it.amount>MAX_AMOUNT || it.id || it.equip || it.equipSwitch || it.expire_time)return false;any=true;}
            else if(it.amount)return false;
        }
        if(!any && !r.mail_zeny)return false;
    }else{
        if(r.mail_zeny)return false;
        for(const auto& it:r.mail_items)if(it.nameid || it.amount)return false;
    }
    if(r.kind==MailSend) {
        const auto& m=r.outgoing;
        if(r.mail_id || r.pet_count || r.pet_retire_count || r.point.scope || r.response ||
           r.counter_after!=r.counter_before || !m.recipient[0] || !m.title[0] ||
           !std::memchr(m.recipient,0,sizeof(m.recipient)) || !std::memchr(m.title,0,sizeof(m.title)) ||
           !std::memchr(m.body,0,sizeof(m.body)))return false;
        uint32_t count=0;
        for(const auto& it:m.attachments) {
            if(!it.nameid){if(it.amount)return false;continue;}
            if(it.amount<1 || it.amount>MAX_AMOUNT || it.id || it.equip || it.equipSwitch || it.expire_time)return false;
            ++count;
        }
        int64_t total=0;
        if(!pn_zeny::fee_total(m.zeny,m.fee_percent,static_cast<int64_t>(count)*m.attachment_price,total) ||
           total>r.wallet_before || r.wallet_after!=r.wallet_before-total)return false;
    }
    if(r.kind!=Sale && r.kind!=Asset && r.kind!=ItemUse && (r.cash_after!=r.cash_before || r.kafra_after!=r.kafra_before))return false;
    if(r.kind==Sale && r.wallet_after!=r.wallet_before)return false;
    if(r.kind==PetClaim && (!r.pet_count || r.pet_retire_count || r.wallet_after!=r.wallet_before || r.counter_after!=r.counter_before))return false;
    for(const auto& it:r.items)if((it.nameid && (it.amount<1 || it.amount>MAX_AMOUNT)) || (!it.nameid && it.amount))return false;
    for(uint16_t i=0;i<r.pet_retire_count;++i) {
        const auto& retired=r.retired_pets[i];
        if(!retired.pet_id || retired.pet_id>INT32_MAX || !retired.egg_id)return false;
        for(uint16_t j=0;j<i;++j)if(r.retired_pets[j].pet_id==retired.pet_id)return false;
        for(const auto& it:r.items)if(it.nameid && it.card[0]==pn_pet::egg_marker &&
            it.card[1]==(retired.pet_id&0xffff) && it.card[2]==(retired.pet_id>>16))return false;
    }
    for(uint16_t i=0;i<r.pet_count;++i) {
        const auto& change=r.pets[i];
        if(!change.claim_id) {
            pn_pet::Key key{};key.account_id=r.account_id;key.char_id=r.char_id;
            key.nonce_hi=r.nonce_hi;key.nonce_lo=r.nonce_lo;key.sequence=r.sequence;key.ordinal=i;
            if(r.kind==PetClaim || change.inventory_index!=-1 || !pn_pet::valid(key,change.output))return false;
        }else{
            const auto& egg=change.output.egg;
            if(r.kind!=PetClaim || change.inventory_index<0 || change.inventory_index>=MAX_INVENTORY ||
               change.output.version!=pn_pet::output_version || egg.amount!=1 || !egg.nameid ||
               egg.id || egg.equip || egg.equipSwitch || egg.card[0]!=pn_pet::egg_marker ||
               !(egg.card[1]|egg.card[2]) || egg.card[1]>UINT16_MAX || egg.card[2]>INT16_MAX ||
               memcmp(&egg,&r.items[change.inventory_index],sizeof(egg)))return false;
            unsigned occurrences=0;
            for(const auto& it:r.items)if(it.nameid && it.card[0]==pn_pet::egg_marker &&
                it.card[1]==egg.card[1] && it.card[2]==egg.card[2])++occurrences;
            if(occurrences!=1)return false;
            for(uint16_t j=0;j<i;++j)if(r.pets[j].claim_id==change.claim_id ||
                r.pets[j].inventory_index==change.inventory_index)return false;
        }
    }
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
