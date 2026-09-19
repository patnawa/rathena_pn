// PN account bank. GPL-3.0-or-later.
#ifndef PN_BANK_STATE_HPP
#define PN_BANK_STATE_HPP
#include "bank_protocol.hpp"
struct pn_bank_state {
    uint64_t nonce_hi = 0, nonce_lo = 0, request_id = 0;
    uint32_t action = 0;
    uint8_t native_action = 0; // Original client UI also waits for the SQL commit.
    int64_t amount = 0, bank_before = 0, wallet_before = 0;
    int64_t reserve_before = 0, reserve_after = 0;
    uint32_t reserve_item[2] = {}, reserve_quantity[2] = {};
    int64_t last_action_tick = 0;
    bool pending = false, applying = false;
    int32_t companion_fd = 0;
    bool open_requested = false;
    uint32_t result = pn_bank::Ok;
};
#endif
