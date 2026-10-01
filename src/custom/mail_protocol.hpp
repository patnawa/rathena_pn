// Independent full-width Zeny mail composer. GPL-3.0-or-later.
#ifndef PN_MAIL_PROTOCOL_HPP
#define PN_MAIL_PROTOCOL_HPP
#include <cstdint>
#include <limits>
namespace pn_mail {
constexpr uint32_t magic=0x314c5a50;
constexpr uint16_t version=1;
enum Action:uint32_t {Status,Quote,Send};
enum Result:uint32_t {Ok,Pending,Denied,Invalid,Busy,Funds,Stale,Failed};
#pragma pack(push,1)
struct Request {
    uint32_t magic_value=magic;uint16_t protocol=version,length=636;
    uint32_t account_id=0,char_id=0,login_id1=0,login_id2=0;
    uint64_t nonce_hi=0,nonce_lo=0,request_id=0;
    uint32_t action=Status,reserved=0;
    int64_t amount=0,expected_total=0;
    char recipient[24]={},title[40]={},body[500]={};
};
struct Reply {
    uint32_t magic_value=magic;uint16_t protocol=version,length=96;
    uint64_t nonce_hi=0,nonce_lo=0,request_id=0;
    uint32_t result=Denied,char_id=0;
    int64_t wallet=0,amount=0,fee=0,total=0,limit=std::numeric_limits<int64_t>::max();
    uint32_t fee_percent=0,pending=0;
    uint64_t send_sequence=0;
};
#pragma pack(pop)
static_assert(sizeof(Request)==636,"mail request ABI");
static_assert(sizeof(Reply)==96,"mail reply ABI");
}
#endif
