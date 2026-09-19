// PN account bank. GPL-3.0-or-later. Internal map/character protocol.
#ifndef PN_BANK_COMMIT_HPP
#define PN_BANK_COMMIT_HPP
#include "bank_protocol.hpp"
#pragma pack(push, 1)
struct pn_bank_commit {
    uint16_t packet = 0x308e, length = 0;
    uint32_t account_id = 0, char_id = 0;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint32_t action = 0;
    int64_t amount = 0, bank_before = 0, wallet_before = 0;
    int64_t bank_after = 0, wallet_after = 0;
    int64_t reserve_before = 0, reserve_after = 0;
    uint32_t reserve_item[2] = {}, reserve_quantity[2] = {};
};
struct pn_bank_ack {
    uint16_t packet = 0x388e;
    uint32_t account_id = 0, char_id = 0;
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint8_t committed = 0;
};
#pragma pack(pop)
static_assert(sizeof(pn_bank_commit) == 112, "Bank/reserve commit ABI; upgrade map and char together");
static_assert(sizeof(pn_bank_ack) == 35, "Bank ack ABI");
#endif
