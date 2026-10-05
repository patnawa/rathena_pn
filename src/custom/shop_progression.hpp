#ifndef PN_SHOP_PROGRESSION_HPP
#define PN_SHOP_PROGRESSION_HPP

#include <common/mmo.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <unordered_set>
#include <vector>

namespace pn_shop_progression {

constexpr uint16_t packet = 0x3099;
constexpr uint16_t version = 1;

#pragma pack(push, 1)
struct Header {
	uint16_t packet_id = packet;
	uint16_t length = 0;
	uint16_t version_id = version;
	uint16_t reserved = 0;
	uint32_t account_id = 0;
	uint32_t char_id = 0;
	uint64_t nonce_hi = 0;
	uint64_t nonce_lo = 0;
	uint64_t sequence = 0;
	uint16_t before_count = 0;
	uint16_t after_count = 0;
};
#pragma pack(pop)

struct Bundle {
	Header header{};
	std::vector<achievement> before;
	std::vector<achievement> after;
	std::vector<uint8_t> bytes;
};

inline bool persisted(const achievement& row) {
	if (row.completed || row.rewarded)
		return true;
	for (int32 value : row.count)
		if (value)
			return true;
	return false;
}

inline bool valid_rows(const std::vector<achievement>& rows) {
	int32 previous = 0;
	for (const auto& row : rows) {
		if (row.achievement_id <= 0 || row.completed < 0 || row.rewarded < 0 ||
			(row.rewarded && !row.completed) || row.score != 0 || !persisted(row) ||
			row.achievement_id <= previous)
			return false;
		for (int32 value : row.count)
			if (value < 0)
				return false;
		previous = row.achievement_id;
	}
	return true;
}

// Planned completion capture is monotonic: it may add rows, increase counters,
// or complete an incomplete row. It never deletes progress, rewrites a prior
// completion timestamp, or claims a reward.
inline bool valid_transition(const std::vector<achievement>& before,
	const std::vector<achievement>& after) {
	if (!valid_rows(before) || !valid_rows(after))
		return false;
	size_t next = 0;
	for (const auto& old_row : before) {
		while (next < after.size() && after[next].achievement_id < old_row.achievement_id)
			++next;
		if (next == after.size() || after[next].achievement_id != old_row.achievement_id)
			return false;
		const auto& new_row = after[next];
		if ((old_row.completed && new_row.completed != old_row.completed) ||
			new_row.rewarded != old_row.rewarded)
			return false;
		for (size_t i = 0; i < MAX_ACHIEVEMENT_OBJECTIVES; ++i)
			if (new_row.count[i] < old_row.count[i])
				return false;
	}
	for (const auto& row : after) {
		auto found = std::lower_bound(before.begin(), before.end(), row.achievement_id,
			[](const achievement& value, int32 id) { return value.achievement_id < id; });
		if (found == before.end() || found->achievement_id != row.achievement_id)
			if (row.rewarded)
				return false;
	}
	return true;
}

inline bool same_state(const achievement& left, const achievement& right) {
	return left.achievement_id == right.achievement_id && left.completed == right.completed &&
		left.rewarded == right.rewarded &&
		std::equal(std::begin(left.count), std::end(left.count), std::begin(right.count));
}

inline std::vector<uint8_t> encode(uint32_t account_id, uint32_t char_id,
	uint64_t nonce_hi, uint64_t nonce_lo, uint64_t sequence,
	const std::vector<achievement>& before, const std::vector<achievement>& after) {
	if (!account_id || !char_id || !(nonce_hi | nonce_lo) || !sequence ||
		!valid_transition(before, after))
		return {};
	// Only rows changed by these planned success callbacks need a baseline. Character SQL
	// treats every omitted row as opaque and preserves it. This keeps a player
	// with hundreds of unrelated durable achievements below the 16-bit inter-
	// server frame limit without weakening compare-and-merge for touched rows.
	std::vector<achievement> changed_before, changed_after;
	changed_before.reserve(before.size());
	changed_after.reserve(after.size());
	size_t prior = 0;
	for (const auto& row : after) {
		while (prior < before.size() && before[prior].achievement_id < row.achievement_id)
			++prior;
		if (prior < before.size() && before[prior].achievement_id == row.achievement_id) {
			if (same_state(before[prior], row))
				continue;
			changed_before.push_back(before[prior]);
		}
		changed_after.push_back(row);
	}
	if (changed_before.size() > std::numeric_limits<uint16_t>::max() ||
		changed_after.size() > std::numeric_limits<uint16_t>::max())
		return {};
	const size_t size = sizeof(Header) + (changed_before.size() + changed_after.size()) * sizeof(achievement);
	if (size > std::numeric_limits<uint16_t>::max())
		return {};
	Header header;
	header.length = static_cast<uint16_t>(size);
	header.account_id = account_id;
	header.char_id = char_id;
	header.nonce_hi = nonce_hi;
	header.nonce_lo = nonce_lo;
	header.sequence = sequence;
	header.before_count = static_cast<uint16_t>(changed_before.size());
	header.after_count = static_cast<uint16_t>(changed_after.size());
	std::vector<uint8_t> result(size);
	size_t offset = 0;
	memcpy(result.data() + offset, &header, sizeof(header));
	offset += sizeof(header);
	if (!changed_before.empty()) {
		memcpy(result.data() + offset, changed_before.data(), changed_before.size() * sizeof(achievement));
		offset += changed_before.size() * sizeof(achievement);
	}
	if (!changed_after.empty())
		memcpy(result.data() + offset, changed_after.data(), changed_after.size() * sizeof(achievement));
	return result;
}

inline bool decode(const void* data, size_t size, Bundle& out) {
	out = {};
	if (!data || size < sizeof(Header) || size > std::numeric_limits<uint16_t>::max())
		return false;
	Header header;
	memcpy(&header, data, sizeof(header));
	const size_t expected = sizeof(Header) +
		(static_cast<size_t>(header.before_count) + header.after_count) * sizeof(achievement);
	if (header.packet_id != packet || header.length != size || header.version_id != version ||
		header.reserved || !header.account_id || !header.char_id ||
		!(header.nonce_hi | header.nonce_lo) || !header.sequence || expected != size)
		return false;
	const auto* cursor = static_cast<const uint8_t*>(data) + sizeof(Header);
	out.header = header;
	out.before.resize(header.before_count);
	out.after.resize(header.after_count);
	if (!out.before.empty()) {
		memcpy(out.before.data(), cursor, out.before.size() * sizeof(achievement));
		cursor += out.before.size() * sizeof(achievement);
	}
	if (!out.after.empty())
		memcpy(out.after.data(), cursor, out.after.size() * sizeof(achievement));
	if (!valid_transition(out.before, out.after)) {
		out = {};
		return false;
	}
	out.bytes.assign(static_cast<const uint8_t*>(data), static_cast<const uint8_t*>(data) + size);
	return true;
}

} // namespace pn_shop_progression

#endif
