#ifndef PN_MARKET_STATE_HPP
#define PN_MARKET_STATE_HPP
#include <cstdint>
struct pn_market_state {
    uint64_t revision = 0, sequence = 0;
    uint32_t last_target = 0;
    bool published = false;
    // Native sale reports are emitted only after the paired SQL receipt.
    uint32_t sold_count = 0, bought_count = 0;
    struct Bought { int16_t index=0, amount=0; uint32_t item_id=0; int64_t price=0, total=0; } bought[5]{};
    struct Sale { int16_t cart_index=0, amount=0; int64_t net=0; } sold[20]{};
};
// One single-threaded map process owns shop revisions; a new shop cannot reuse
// an old quote even if its native integer shop id eventually wraps.
inline uint64_t pn_market_revision() {
    static uint64_t next = 0;
    return ++next;
}
#endif
