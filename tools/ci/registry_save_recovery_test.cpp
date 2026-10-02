#include <common/cbasetypes.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <vector>
#include <custom/registry_save.hpp>

struct script_reg_state { uint32 type:1; uint32 update:1; };
struct script_reg_num { script_reg_state flag; int64 value; };
struct script_reg_str { script_reg_state flag; char* value; };
static script_reg_num variable{};
enum { DB_DATA_PTR = 1, CSAVE_NORMAL = 0 };
struct DBKey { int64 i64 = 1; };
struct DBData { int type = DB_DATA_PTR; void* data = &variable; };
static DBData row;
static void* db_data2ptr(DBData* data) { return data->data; }
struct DBIterator {
    int index = 0;
    DBData* (*first)(DBIterator*, DBKey*);
    bool (*exists)(DBIterator*);
    DBData* (*next)(DBIterator*, DBKey*);
};
static DBIterator* db_iterator(void*) {
    static DBIterator iterator{0,
        [](DBIterator* i, DBKey* k) { i->index = 0; k->i64 = 1; return &row; },
        [](DBIterator* i) { return i->index == 0; },
        [](DBIterator* i, DBKey*) -> DBData* { ++i->index; return nullptr; }};
    return &iterator;
}
static void dbi_destroy(DBIterator*) {}
static int script_getvarid(int64 key) { return static_cast<int>(key); }
static uint32 script_getvaridx(int64) { return 0; }
static const char* get_str(int) { return "PNQuestStep"; }
static bool script_check_RegistryVariableLength(int, const char* value, size_t* size) {
    *size = std::strlen(value); return true;
}
static void safestrncpy(char* out, const char* in, size_t length) { std::snprintf(out, length, "%s", in); }
static void ShowError(const char*, ...) {}
static void ShowDebug(const char*, ...) {}
struct map_session_data {
    struct { uint32 account_id = 9001, char_id = 7001; } status;
    struct { void* vars = &row; } regs;
    bool vars_dirty = true;
    pn_registry::Journal registry_saves;
};
static bool pn_item_use_save_defer(map_session_data*, int) { return false; }
static bool pc_transaction_pending(map_session_data*) { return false; }
static void script_reg_destroy_single(map_session_data*, int64, script_reg_state*) {}
static std::vector<std::vector<uint8>> sent;
static bool connected = true;
static bool chrif_isconnected() { return connected; }
static bool chrif_save_packet(const void* data, size_t size) {
    const auto* bytes = static_cast<const uint8*>(data);
    sent.emplace_back(bytes, bytes + size); return true;
}
#define WBUFB(b,n) (*(reinterpret_cast<uint8*>(b)+(n)))
#define WBUFW(b,n) (*reinterpret_cast<uint16*>(reinterpret_cast<uint8*>(b)+(n)))
#define WBUFL(b,n) (*reinterpret_cast<uint32*>(reinterpret_cast<uint8*>(b)+(n)))
#define WBUFQ(b,n) (*reinterpret_cast<int64*>(reinterpret_cast<uint8*>(b)+(n)))
#define WBUFCP(b,n) (reinterpret_cast<char*>(b)+(n))
#include "registry-save.inc"

int main() {
    map_session_data player;
    variable.value = 17; variable.flag.update = 1;
    intif_saveregistry(&player);
    if (sent.size() != 1 || WBUFW(sent[0].data(), 40) != 1) return 2;
    // The connection drops after enqueue and before the character service
    // consumes the frame. On reconnect the same live player is saved again.
    sent.clear();
    intif_saveregistry(&player);
    for (const auto& packet : sent) {
        uint16 count = 0; std::memcpy(&count, packet.data() + 40, sizeof(count));
        if (count) {
            const auto old = packet;
            auto ack = pn_registry::acknowledgement(old.data(), true);
            // A new edit is independent of the older receipt.
            variable.value = 18; variable.flag.update = 1; player.vars_dirty = true;
            if (!player.registry_saves.acknowledge(ack.data(), ack.size()) || !variable.flag.update || !player.vars_dirty) return 3;
            if (player.registry_saves.acknowledge(ack.data(), ack.size())) return 4;
            connected = false; sent.clear(); intif_saveregistry(&player);
            if (player.registry_saves.empty() || variable.flag.update || !sent.empty()) return 5;
            connected = true; intif_saveregistry(&player);
            std::vector<pn_registry::Entry> entries;
            if (sent.empty() || !pn_registry::decode(sent.back().data(), sent.back().size(), entries) || entries[0].number != 18) return 6;
            if (player.registry_saves.acknowledge(ack.data(), ack.size()) || player.registry_saves.empty()) return 7;
            const auto latest = sent.back();
            auto failed = pn_registry::acknowledgement(latest.data(), false);
            if (player.registry_saves.acknowledge(failed.data(), failed.size())) return 8;
            // Every truncation and trailing byte is rejected before SQL.
            for (size_t n = 0; n < latest.size(); ++n) if (pn_registry::decode(latest.data(), n, entries)) return 9;
            auto trailing = latest; trailing.push_back(0);
            pn_registry::write<uint16_t>(trailing.data(), 2, trailing.size());
            if (pn_registry::decode(trailing.data(), trailing.size(), entries)) return 10;
            std::cout << "PASS unacknowledged registry retry, newer edits, offline capture, stale/failed ACK and malformed frames\n";
            return 0;
        }
    }
    std::cerr << "FAIL disconnect lost PNQuestStep=17: retry contains no registry update\n";
    return 1;
}
