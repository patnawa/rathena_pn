// Lossless storage encoding only. The immutable inter-server request is unchanged.
#ifndef PN_SHOP_RECEIPT_CODEC_HPP
#define PN_SHOP_RECEIPT_CODEC_HPP
#include <cstdint>
#include <cstring>
#include <vector>
#include <zlib.h>

namespace pn_shop_receipt_codec {
constexpr size_t limit = 1024 * 1024;
constexpr unsigned char magic[] = {'P','N','R','Z',1,0,0,0};
constexpr size_t header_size = sizeof(magic) + 4;

inline std::vector<unsigned char> encode(const void* request, size_t size) {
    const auto* raw = static_cast<const unsigned char*>(request);
    std::vector<unsigned char> result(raw, raw + size);
    if (size <= header_size || size > limit) return result;
    uLongf length = compressBound(static_cast<uLong>(size));
    std::vector<unsigned char> packed(header_size + length);
    std::memcpy(packed.data(), magic, sizeof(magic));
    for (unsigned i = 0; i < 4; ++i)
        packed[sizeof(magic) + i] = static_cast<unsigned char>(size >> (8 * i));
    if (compress2(packed.data() + header_size, &length, raw,
                  static_cast<uLong>(size), Z_BEST_SPEED) != Z_OK || header_size + length >= size)
        return result;
    packed.resize(header_size + length);
    return packed;
}

inline bool matches(const void* stored, size_t length, const void* request, size_t size) {
    if (!stored || !request || !size || size > limit) return false;
    // Historical rows are raw fixed-size requests, including any coincidental
    // magic prefix. Encoded rows are always strictly shorter than that size.
    if (length == size) return std::memcmp(stored, request, size) == 0;
    if (length <= header_size || length >= size) return false;
    const auto* bytes = static_cast<const unsigned char*>(stored);
    if (std::memcmp(bytes, magic, sizeof(magic))) return false;
    uint32_t decoded_size = 0;
    for (unsigned i = 0; i < 4; ++i)
        decoded_size |= uint32_t(bytes[sizeof(magic) + i]) << (8 * i);
    if (decoded_size != size) return false;
    // Allocate only the trusted request size, never a length from stored data.
    std::vector<unsigned char> decoded(size);
    z_stream stream{};
    stream.next_in = const_cast<Bytef*>(bytes + header_size);
    stream.avail_in = static_cast<uInt>(length - header_size);
    stream.next_out = decoded.data();
    stream.avail_out = static_cast<uInt>(size);
    if (inflateInit(&stream) != Z_OK) return false;
    const int status = inflate(&stream, Z_FINISH);
    const bool exact = status == Z_STREAM_END && stream.total_out == size && stream.avail_in == 0;
    inflateEnd(&stream);
    return exact && std::memcmp(decoded.data(), request, size) == 0;
}
}
#endif
