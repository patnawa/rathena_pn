// Immutable registry deltas, acknowledged only after their SQL transaction.
#ifndef PN_REGISTRY_SAVE_HPP
#define PN_REGISTRY_SAVE_HPP
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <random>
#include <string>
#include <vector>

namespace pn_registry {
constexpr uint16_t request_packet = 0x30a6, ack_packet = 0x38a6;
constexpr uint16_t login_request = 0x2746, login_ack = 0x2747, version = 1;
constexpr size_t header_size = 48, max_packet = 60000, max_bytes = 4 * 1024 * 1024;
using Packet = std::vector<uint8_t>;
template<class T> T read(const void* bytes, size_t offset) {
	T value; std::memcpy(&value, static_cast<const uint8_t*>(bytes) + offset, sizeof(value)); return value;
}
template<class T> void write(void* bytes, size_t offset, T value) {
	std::memcpy(static_cast<uint8_t*>(bytes) + offset, &value, sizeof(value));
}
struct Identity {
	uint32_t account = 0, character = 0;
	uint64_t high = 0, low = 0, sequence = 0;
	uint8_t scope = 0; // 0: character and local account; 1: global account
};
inline Identity identity(const void* data) {
	return {read<uint32_t>(data, 8), read<uint32_t>(data, 12), read<uint64_t>(data, 16),
		read<uint64_t>(data, 24), read<uint64_t>(data, 32), read<uint8_t>(data, 6)};
}
inline bool header(const void* data, size_t length) {
	if (!data || length < header_size || length > max_packet || read<uint16_t>(data, 2) != length ||
		read<uint16_t>(data, 4) != version || read<uint8_t>(data, 6) > 1 || read<uint8_t>(data, 7)) return false;
	auto id = identity(data);
	if (!id.account || !id.character || !(id.high | id.low) || !id.sequence) return false;
	for (size_t i = 43; i < header_size; ++i) if (read<uint8_t>(data, i)) return false;
	return true;
}
inline Packet acknowledgement(const void* request, bool success, uint16_t packet = ack_packet) {
	Packet result(header_size); std::memcpy(result.data(), request, header_size);
	write<uint16_t>(result.data(), 0, packet); write<uint16_t>(result.data(), 2, header_size);
	write<uint16_t>(result.data(), 40, 0); result[42] = success ? 1 : 0; return result;
}
struct Entry { std::string key, text; uint32_t index = 0; int64_t number = 0; uint8_t kind = 0; };
inline bool decode(const void* data, size_t length, std::vector<Entry>& entries) {
	entries.clear();
	if (!header(data, length) || read<uint8_t>(data, 42)) return false;
	const auto* bytes = static_cast<const uint8_t*>(data);
	size_t cursor = header_size;
	const uint16_t count = read<uint16_t>(data, 40);
	if (!count) return false;
	for (uint16_t i = 0; i < count; ++i) {
		if (cursor >= length) return false;
		const size_t size = bytes[cursor++];
		if (size < 2 || size > 32 || length - cursor < size + 5 || bytes[cursor + size - 1] ||
			std::memchr(bytes + cursor, 0, size - 1)) return false;
		Entry entry; entry.key.assign(reinterpret_cast<const char*>(bytes + cursor), size - 1); cursor += size;
		const bool global = entry.key.size() > 1 && entry.key[0] == '#' && entry.key[1] == '#';
		if (entry.key[0] == '@' || entry.key[0] == '$' || entry.key[0] == '.' || entry.key[0] == '\'' || global != bool(bytes[6])) return false;
		entry.index = read<uint32_t>(bytes, cursor); cursor += 4; entry.kind = bytes[cursor++];
		if (entry.kind > 3 || (entry.key.back() == '$') != (entry.kind >= 2)) return false;
		if (entry.kind == 0) {
			if (length - cursor < 8) return false;
			entry.number = read<int64_t>(bytes, cursor); cursor += 8;
		} else if (entry.kind == 2) {
			if (cursor >= length) return false;
			const size_t size = bytes[cursor++];
			if (!size || size > 254 || length - cursor < size || bytes[cursor + size - 1] || std::memchr(bytes + cursor, 0, size - 1)) return false;
			entry.text.assign(reinterpret_cast<const char*>(bytes + cursor), size - 1); cursor += size;
		}
		entries.push_back(std::move(entry));
	}
	return cursor == length;
}
inline bool append_entry(Packet& packet, const Entry& entry) {
	if (entry.key.empty() || entry.key.size() > 31 || entry.text.size() > 253 || entry.kind > 3) return false;
	const size_t size = 1 + entry.key.size() + 1 + 4 + 1 + (entry.kind == 0 ? 8 : entry.kind == 2 ? 1 + entry.text.size() + 1 : 0);
	if (packet.size() < header_size || packet.size() + size > max_packet) return false;
	size_t offset = packet.size(); packet.resize(offset + size);
	packet[offset++] = uint8_t(entry.key.size() + 1);
	std::memcpy(packet.data() + offset, entry.key.c_str(), entry.key.size() + 1); offset += entry.key.size() + 1;
	write<uint32_t>(packet.data(), offset, entry.index); offset += 4; packet[offset++] = entry.kind;
	if (entry.kind == 0) write<int64_t>(packet.data(), offset, entry.number);
	else if (entry.kind == 2) { packet[offset++] = uint8_t(entry.text.size() + 1); std::memcpy(packet.data() + offset, entry.text.c_str(), entry.text.size() + 1); }
	write<uint16_t>(packet.data(), 40, read<uint16_t>(packet.data(), 40) + 1); return true;
}
class Journal {
	uint64_t high = 0, low = 0;
	std::array<uint64_t, 2> next{{1, 1}};
	size_t bytes = 0;
public:
	std::vector<Packet> packets;
	bool empty() const { return packets.empty(); }
	bool retain(uint32_t account, uint32_t character, std::vector<Packet>& batch) {
		size_t added = 0;
		std::array<uint64_t, 2> counts{};
		if (!account || !character) return false;
		for (const auto& packet : batch) {
			if (packet.size() < header_size || packet.size() > max_packet || packet[6] > 1 ||
				packet.size() > max_bytes - added) return false;
			if (++counts[packet[6]] > std::numeric_limits<uint64_t>::max() - next[packet[6]]) return false;
			added += packet.size();
		}
		if (added > max_bytes - bytes) return false;
		if (!(high | low)) {
			std::random_device random;
			do { high = (uint64_t(random()) << 32) | random(); low = (uint64_t(random()) << 32) | random(); } while (!(high | low));
		}
		for (auto& packet : batch) {
			write<uint16_t>(packet.data(), 0, request_packet); write<uint16_t>(packet.data(), 2, uint16_t(packet.size()));
			write<uint16_t>(packet.data(), 4, version); write<uint32_t>(packet.data(), 8, account); write<uint32_t>(packet.data(), 12, character);
			write<uint64_t>(packet.data(), 16, high); write<uint64_t>(packet.data(), 24, low);
			write<uint64_t>(packet.data(), 32, next[packet[6]]++); packets.push_back(std::move(packet));
		}
		bytes += added; return true;
	}
	bool acknowledge(const void* data, size_t length) {
		if (!header(data, length) || length != header_size || read<uint16_t>(data, 0) != ack_packet ||
			read<uint16_t>(data, 40) || read<uint8_t>(data, 42) != 1) return false;
		const auto ack = identity(data);
		for (auto i = packets.begin(); i != packets.end(); ++i) {
			const auto id = identity(i->data());
			if (id.account == ack.account && id.character == ack.character && id.high == ack.high && id.low == ack.low && id.sequence == ack.sequence && id.scope == ack.scope) {
				bytes -= i->size(); packets.erase(i); return true;
			}
		}
		return false;
	}
};
}
#endif
