// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  native_allocator_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/native_allocator_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Standalone allocator regression: compile the ACTUAL current allocator source.
// No map/common archives, player state, normal server startup, SQL or networking.
// Only console/version services are replaced. Including the implementation lets
// the diagnostic print its real private layout, rather than a copied definition.
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "../../src/common/malloc.cpp"

static unsigned allocator_errors;
static unsigned assertions;
static char test_server_name[] = "isolated-native-allocator-test";
char* SERVER_NAME = test_server_name;
const char* get_git_hash() { return "\x02"; }
const char* get_svn_revision() { return "\x02"; }

#define CONSOLE_SERVICE(name, is_error) \
    void name(const char* format, ...) { \
        if (is_error) ++allocator_errors; \
        va_list arguments; \
        va_start(arguments, format); \
        std::vfprintf(stderr, format, arguments); \
        va_end(arguments); \
    }
CONSOLE_SERVICE(ShowStatus, false)
CONSOLE_SERVICE(ShowInfo, false)
CONSOLE_SERVICE(ShowWarning, true)
CONSOLE_SERVICE(ShowError, true)
CONSOLE_SERVICE(ShowFatalError, true)

static void check(bool passed, const char* message) {
    ++assertions;
    if (!passed) {
        std::fprintf(stderr, "CHECK FAILED: %s\n", message);
        ++allocator_errors;
    }
}

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);
    const char* mode = argc == 2 ? argv[1] : "matrix";
    if (std::strcmp(mode, "matrix") && std::strcmp(mode, "pool") &&
        std::strcmp(mode, "large-odd") && std::strcmp(mode, "shutdown")) {
        std::fprintf(stderr, "Usage: native_allocator_test [matrix|pool|large-odd|shutdown]\n");
        return 2;
    }
    std::printf("ACTUAL malloc.cpp: pointer=%zu max_align=%zu long_align=%zu\n",
        sizeof(void*), alignof(std::max_align_t), alignof(long));
    std::printf("block: sizeof=%zu align=%zu data_offset=%zu; unit_head: "
        "sizeof=%zu align=%zu checksum_offset=%zu; unit_head_large: sizeof=%zu\n",
        sizeof(block), alignof(block), offsetof(block, data), sizeof(unit_head),
        alignof(unit_head), offsetof(unit_head, checksum), sizeof(unit_head_large));
    malloc_init();
    if (!std::strcmp(mode, "shutdown")) {
        aMalloc(3);
        aMalloc(65537);
        malloc_final();
        const bool expected_warning = allocator_errors == 1;
        allocator_errors = 0;
        check(expected_warning, "shutdown reports the intentional outstanding allocations");
        for (block* current = block_first; current; current = current->block_next)
            check(current->unit_used == 0, "shutdown releases outstanding pooled units");
        std::printf("%s actual allocator shutdown: %u assertions; deliberate leak report expected\n",
                    allocator_errors ? "FAIL" : "PASS", assertions);
        return allocator_errors ? 1 : 0;
    }
    std::vector<void*> allocations;
    unsigned misaligned = 0;
    auto allocate = [&](size_t size) {
        void* memory = aMalloc(size);
        if (!memory) {
            ++allocator_errors;
            return;
        }
        const size_t remainder = reinterpret_cast<uintptr_t>(memory) % alignof(std::max_align_t);
        if (remainder && ++misaligned <= 8)
            std::printf("MISALIGNED size=%zu pointer=%p remainder=%zu (mod %zu)\n",
                size, memory, remainder, alignof(std::max_align_t));
        // Byte writes are legal regardless of alignment; no synthetic typed
        // access is needed to expose allocator's own UB under UBSan.
        std::memset(memory, 0x5a, size);
        check(malloc_verify_ptr(memory), "first payload byte is active");
        check(malloc_verify_ptr(static_cast<char*>(memory) + size - 1), "last payload byte is active");
        check(!malloc_verify_ptr(static_cast<char*>(memory) + size), "one-past payload is not active");
        check(!malloc_verify_ptr(static_cast<char*>(memory) - 1), "header byte is not active");
        allocations.push_back(memory);
    };
    if (!std::strcmp(mode, "pool")) {
        allocate(64);
    } else if (!std::strcmp(mode, "large-odd")) {
        allocate(65537);
    } else {
        // Multiple adjacent units and multiple backing blocks, plus both sides
        // of size-class and pooled/direct-allocation boundaries.
        for (size_t size : {1u, 2u, 3u, 4u, 7u, 8u, 15u, 16u, 17u, 31u, 32u,
             33u, 63u, 64u, 65u, 127u, 128u, 129u, 2047u, 2048u, 2049u,
             4096u, 40896u, 40897u, 40920u, 40927u, 40928u, 40929u, 40960u, 65536u, 65537u}) {
            allocate(size);
            allocate(size);
        }
        for (unsigned i = 0; i < 3000; ++i) allocate(8);
    }
    // Free every other allocation while its backing block still has live units,
    // then allocate replacements to exercise actual free-unit reuse.
    for (size_t index = 0; index < allocations.size(); index += 2) {
        aFree(allocations[index]);
        allocations[index] = nullptr;
    }
    if (!std::strcmp(mode, "matrix")) {
        for (unsigned i = 0; i < 1600; ++i) allocate(8);
    }
    // Non-FIFO frees exercise both the pooled and large linked lists.
    for (auto iterator = allocations.rbegin(); iterator != allocations.rend(); ++iterator)
        aFree(*iterator);

    if (!std::strcmp(mode, "matrix")) {
        auto* zeros = static_cast<unsigned char*>(aCalloc(19, 3));
        check(reinterpret_cast<uintptr_t>(zeros) % alignof(std::max_align_t) == 0, "calloc alignment");
        for (size_t i = 0; i < 57; ++i) check(zeros[i] == 0, "calloc clears all requested bytes");
        std::memset(zeros, 0x39, 57);
        auto* grown = static_cast<unsigned char*>(aRealloc(zeros, 65537));
        check(reinterpret_cast<uintptr_t>(grown) % alignof(std::max_align_t) == 0, "pooled-to-large realloc alignment");
        for (size_t i = 0; i < 57; ++i) check(grown[i] == 0x39, "realloc preserves existing bytes");
        grown[65536] = 0x71;
        auto* larger = static_cast<unsigned char*>(aRealloc(grown, 131073));
        check(reinterpret_cast<uintptr_t>(larger) % alignof(std::max_align_t) == 0, "large-to-large realloc alignment");
        check(larger[65536] == 0x71, "large realloc preserves final old byte");
        check(aRealloc(larger, 10) == larger, "existing shrink-in-place semantics preserved");
        aFree(larger);
        void* from_null = aRealloc(nullptr, 17);
        check(malloc_verify_ptr(from_null), "realloc null allocates");
        aFree(from_null);
        char* text = aStrdup("odd-sized string");
        check(std::strcmp(text, "odd-sized string") == 0, "strdup copies terminator");
        aFree(text);
        check(aStrdup(nullptr) == nullptr, "strdup null preserved");
        aFree(nullptr);

        // Corrupt and restore only the allocator-owned guard byte. A rejected
        // free must retain the allocation and its accounting until repaired.
        for (size_t size : {3u, 65537u}) {
            char* guarded = static_cast<char*>(aMalloc(size));
            unsigned char original_guard[sizeof(long)];
            std::memcpy(original_guard, guarded + size, sizeof(original_guard));
            const auto before_errors = allocator_errors;
            const auto before_usage = memmgr_usage_bytes;
            guarded[size] ^= 1;
            aFree(guarded);
            const bool detected = allocator_errors == before_errors + 1;
            allocator_errors = before_errors; // the one expected diagnostic
            check(detected, "tail corruption is diagnosed exactly once");
            check(memmgr_usage_bytes == before_usage, "failed guard check does not release bytes");
            std::memcpy(guarded + size, original_guard, sizeof(original_guard));
            aFree(guarded);
        }
    }
    if (memmgr_usage_bytes != 0) ++allocator_errors;
    malloc_final();
    std::printf("%s actual allocator: %zu matrix allocations; %u misaligned; %u allocator errors; %u assertions\n",
        misaligned || allocator_errors ? "FAIL" : "PASS", allocations.size(),
        misaligned, allocator_errors, assertions);
    return misaligned || allocator_errors ? 1 : 0;
}
