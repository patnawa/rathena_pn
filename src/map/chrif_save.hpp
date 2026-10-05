#ifndef CHRIF_SAVE_HPP
#define CHRIF_SAVE_HPP

#include <cstddef>
#include <cstdint>
#include <limits>
#include <vector>

// Serialized replacement saves survive destruction of the live logout session.
// This journal lives only until the owning character server acknowledges the
// ordered stream barrier; it must never replay after the final offline save.
struct ChrifSaveBuffer {
	static constexpr size_t max_bytes = 4 * 1024 * 1024;
	std::vector<std::vector<uint8_t>> packets;
	size_t bytes = 0;
	bool failed = false;

	bool append(const void* data, size_t size) {
		if (failed || !data || size < 2 || size > std::numeric_limits<uint16_t>::max() ||
			size > max_bytes - bytes) {
			failed = true;
			return false;
		}
		const auto* first = static_cast<const uint8_t*>(data);
		packets.emplace_back(first, first + size);
		bytes += size;
		return true;
	}
};

// The map event loop is single threaded. Only replacement-save serializers use
// this scope; gameplay requests and shared guild storage keep their own paths.
inline ChrifSaveBuffer* chrif_save_capture = nullptr;
class ChrifSaveCapture {
	ChrifSaveBuffer* previous;
public:
	explicit ChrifSaveCapture(ChrifSaveBuffer& buffer) : previous(chrif_save_capture) {
		chrif_save_capture = &buffer;
	}
	~ChrifSaveCapture() { chrif_save_capture = previous; }
	ChrifSaveCapture(const ChrifSaveCapture&) = delete;
	ChrifSaveCapture& operator=(const ChrifSaveCapture&) = delete;
};

bool chrif_save_available();
bool chrif_save_packet(const void* data, size_t size);

#endif
