#ifndef PN_MAIL_COMMIT_HPP
#define PN_MAIL_COMMIT_HPP
#include "mail_protocol.hpp"
#pragma pack(push,1)
struct pn_mail_commit {
    uint16_t packet=0x3095,length=656;
    pn_mail::Request request{};
    int64_t wallet_before=0;
    uint32_t fee_percent=0,reserved=0;
};
struct pn_mail_ack {
    uint16_t packet=0x3895;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,request_id=0;
    uint32_t result=pn_mail::Pending;
};
#pragma pack(pop)
static_assert(sizeof(pn_mail_commit)==656,"mail commit ABI");
static_assert(sizeof(pn_mail_ack)==38,"mail ack ABI");
#endif
