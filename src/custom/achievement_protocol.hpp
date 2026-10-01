#ifndef PN_ACHIEVEMENT_PROTOCOL_HPP
#define PN_ACHIEVEMENT_PROTOCOL_HPP

#include <cstddef>
#include <cstdint>

namespace pn_achievement_protocol {

// Versioned packet IDs deliberately differ from the upstream 0x3062/0x3862
// pair. A mixed old/new map-char deployment then fails the connection closed
// instead of interpreting rows at different offsets and corrupting progression.
constexpr uint16_t load_request = 0x30A4;
constexpr uint16_t load_response = 0x38A4;
constexpr uint16_t logout_save_request = 0x30A5;
constexpr uint16_t logout_save_response = 0x38A5;
constexpr uint8_t version = 1;
constexpr size_t request_size = 7;  // packet, character, version
constexpr size_t response_size = 10; // packet, length, character, version, success
// Logout saves use a separate, tokenized channel so an ordinary autosave ACK
// can never release the final-save barrier. Both headers keep the uint64 token
// and the following achievement array naturally aligned.
constexpr size_t logout_request_size = 24;
constexpr size_t logout_response_size = 24;
constexpr size_t max_logout_rows(size_t row_size) {
	return row_size ? (UINT16_MAX - logout_request_size) / row_size : 0;
}

} // namespace pn_achievement_protocol

#endif // PN_ACHIEVEMENT_PROTOCOL_HPP
