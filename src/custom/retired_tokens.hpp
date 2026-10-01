// Legacy currency items remain identifiable solely for offline migration.
#ifndef PN_RETIRED_TOKENS_HPP
#define PN_RETIRED_TOKENS_HPP
#include <cstdint>
namespace pn_tokens {
constexpr int64_t value(uint32_t id) { return id == 6024 ? 499000000 : id == 12781 ? 998000 : 0; }
constexpr bool retired(uint32_t id) { return value(id) != 0; }
}
#endif
