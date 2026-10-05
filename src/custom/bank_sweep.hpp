#ifndef PN_BANK_SWEEP_HPP
#define PN_BANK_SWEEP_HPP
#include "bank_protocol.hpp"
#pragma pack(push,1)
struct pn_bank_sweep {
    uint16_t packet=0x3097,length=44;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,request_id=0;
    int64_t bank_before=0;
};
struct pn_bank_sweep_ack {
    uint16_t packet=0x3897;
    uint32_t account_id=0,char_id=0;
    uint64_t nonce_hi=0,nonce_lo=0,request_id=0;
    uint32_t result=pn_bank::Saving;
    int64_t bank_after=0;
    uint32_t collected=0,skipped=0;
};
#pragma pack(pop)
static_assert(sizeof(pn_bank_sweep)==44,"sweep request ABI");
static_assert(sizeof(pn_bank_sweep_ack)==54,"sweep ack ABI");
#endif
