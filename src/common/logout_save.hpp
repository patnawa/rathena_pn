#ifndef LOGOUT_SAVE_HPP
#define LOGOUT_SAVE_HPP

#include <cstddef>
#include <cstdint>

namespace logout_save {
constexpr uint16_t request = 0x2b30;
constexpr uint16_t response = 0x2b31;
constexpr uint16_t version = 1;
constexpr size_t packet_size = 24;
constexpr uint16_t transfer_status_request = 0x2b32;
constexpr uint16_t transfer_status_response = 0x2b33;
// Transfer status uses length.W, version.W, reserved.W, account.L,
// character.L, generation.Q followed by mmo_charstatus. Its 24-byte ACK
// means SQL committed while map ownership is still held by the source.
// command.W version.W account.L character.L reserved.L generation.Q
// The response echoes the request only after earlier synchronous save handlers
// have run on the authenticated owner connection. This is a transport barrier,
// not an assertion that those legacy handlers' SQL writes all succeeded.
}

#endif
