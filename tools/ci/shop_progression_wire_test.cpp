#include <custom/shop_commit.hpp>
#include <custom/shop_progression.hpp>

#include <cassert>
#include <cstring>
#include <iostream>

static achievement row(int id, int count, time_t completed = 0) {
	achievement value{};
	value.achievement_id = id;
	value.count[0] = count;
	value.completed = completed;
	return value;
}

int main() {
	std::vector<achievement> before{row(220023, 1)};
	std::vector<achievement> after{row(220023, 1, 1234), row(240001, 0, 1235)};
	auto bytes = pn_shop_progression::encode(11, 22, 33, 44, 55, before, after);
	assert(bytes.size() == sizeof(pn_shop_progression::Header) + 3 * sizeof(achievement));
	pn_shop_progression::Bundle decoded;
	assert(pn_shop_progression::decode(bytes.data(), bytes.size(), decoded));
	assert(decoded.bytes == bytes && decoded.before.size() == 1 && decoded.after.size() == 2);
	assert(decoded.after[1].achievement_id == 240001 && decoded.header.sequence == 55);

	std::vector<achievement> large_before, large_after;
	for (int id = 1; id <= 600; ++id)
		large_before.push_back(row(300000 + id, 1));
	large_after = large_before;
	large_after.back().count[0] = 2;
	auto compact = pn_shop_progression::encode(11, 22, 33, 44, 56, large_before, large_after);
	assert(compact.size() == sizeof(pn_shop_progression::Header) + 2 * sizeof(achievement));
	assert(pn_shop_progression::decode(compact.data(), compact.size(), decoded));
	assert(decoded.before.size() == 1 && decoded.after.size() == 1 &&
		decoded.before[0].achievement_id == 300600 && decoded.after[0].count[0] == 2);

	pn_shop::Commit request;
	request.length = sizeof(request); request.account_id = 11; request.char_id = 22;
	request.nonce_hi = 33; request.nonce_lo = 44; request.sequence = 55;
	request.stock_count = 1; std::strcpy(request.stocks[0].name, "fixture");
	request.stocks[0].key = 501; request.stocks[0].before = 2; request.stocks[0].after = 1;
	request.progression_size = static_cast<uint32_t>(bytes.size());
	assert(pn_shop::valid(request));

	auto malformed = bytes;
	pn_shop_progression::Header header{}; std::memcpy(&header, malformed.data(), sizeof(header));
	header.version_id++; std::memcpy(malformed.data(), &header, sizeof(header));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::memcpy(&header, malformed.data(), sizeof(header)); header.length--;
	std::memcpy(malformed.data(), &header, sizeof(header));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::memcpy(&header, malformed.data(), sizeof(header)); header.after_count++;
	std::memcpy(malformed.data(), &header, sizeof(header));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::vector<achievement> wire_rows(3); std::memcpy(wire_rows.data(), malformed.data() + sizeof(header), 3 * sizeof(achievement));
	wire_rows[1].achievement_id = wire_rows[2].achievement_id;
	std::memcpy(malformed.data() + sizeof(header), wire_rows.data(), 3 * sizeof(achievement));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::memcpy(wire_rows.data(), malformed.data() + sizeof(header), 3 * sizeof(achievement)); wire_rows[0].count[0] = -1;
	std::memcpy(malformed.data() + sizeof(header), wire_rows.data(), 3 * sizeof(achievement));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::memcpy(wire_rows.data(), malformed.data() + sizeof(header), 3 * sizeof(achievement)); wire_rows[0].rewarded = 1; wire_rows[0].completed = 0;
	std::memcpy(malformed.data() + sizeof(header), wire_rows.data(), 3 * sizeof(achievement));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	malformed = bytes;
	std::memcpy(wire_rows.data(), malformed.data() + sizeof(header), 3 * sizeof(achievement)); wire_rows[1].count[0] = 0; wire_rows[1].completed = 0;
	std::memcpy(malformed.data() + sizeof(header), wire_rows.data(), 3 * sizeof(achievement));
	assert(!pn_shop_progression::decode(malformed.data(), malformed.size(), decoded));
	auto decreasing = after; decreasing[0].count[0] = 0;
	assert(pn_shop_progression::encode(11, 22, 33, 44, 55, before, decreasing).empty());
	auto deleting = after; deleting.erase(deleting.begin());
	assert(pn_shop_progression::encode(11, 22, 33, 44, 55, before, deleting).empty());
	auto reordered = after; std::swap(reordered[0], reordered[1]);
	assert(pn_shop_progression::encode(11, 22, 33, 44, 55, before, reordered).empty());
	assert(pn_shop_progression::encode(11, 22, 0, 0, 55, before, after).empty());
	request.progression_size = sizeof(pn_shop_progression::Header) - 1;
	assert(!pn_shop::valid(request));

	std::cout << "PASS shop progression wire: exact round trip and malformed frames rejected\n";
}
