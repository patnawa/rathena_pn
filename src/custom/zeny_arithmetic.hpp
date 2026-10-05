// Checked nonnegative Zeny arithmetic shared by mail and wallet reservations.
#ifndef PN_ZENY_ARITHMETIC_HPP
#define PN_ZENY_ARITHMETIC_HPP
#include <cstdint>
#include <limits>
namespace pn_zeny {
constexpr std::int64_t limit = std::numeric_limits<std::int64_t>::max();
inline bool room(std::int64_t wallet, std::int64_t reserved, std::int64_t credit) {
    return wallet >= 0 && reserved >= 0 && credit >= 0 &&
        reserved <= limit - wallet && credit <= limit - wallet - reserved;
}
// floor(amount * percentage / 100), without overflowing the intermediate product.
inline bool fee_total(std::int64_t amount, std::int64_t percentage,
                      std::int64_t flat, std::int64_t& total) {
    if (amount < 0 || percentage < 0 || flat < 0) return false;
    const auto whole = amount / 100, remainder = amount % 100;
    if (percentage && (whole > limit / percentage || remainder > limit / percentage)) return false;
    const auto high = whole * percentage, low = remainder * percentage / 100;
    if (high > limit - low) return false;
    const auto fee = high + low;
    if (fee > limit - amount || flat > limit - amount - fee) return false;
    total = amount + fee + flat;
    return true;
}
}
#endif
