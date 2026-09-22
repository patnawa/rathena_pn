// Execute production storage cleanup commands and deletion functions. Only the
// player/world/transport boundaries are doubles; no SQL or live server is used.
#include <common/mmo.hpp>
#include <cstdlib>
#include <cstring>
#include <iostream>

#define nullpo_retr(result, value) do { if (!(value)) return result; } while (0)
#define ACMD_FUNC(name) int name(int fd, map_session_data* sd)

struct map_session_data {
    struct { int storage_flag = 0; } state;
    struct { int guild_id = 1; } status;
    bool guild = true;
    s_storage storage{};
};

static s_storage guild_storage{};
static bool guild_loaded = true;
static int personal_saves = 0, guild_saves = 0, guild_logs = 0;
static const char* msg_txt(map_session_data*, int) { return "fixture"; }
static void clif_displaymessage(int, const char*) {}
static void clif_updatestorageamount(map_session_data&, int, int) {}
static void clif_storageitemremoved(map_session_data&, int, int) {}
static void storage_guild_log(map_session_data*, item*, int) { ++guild_logs; }
static s_storage* guild2storage2(int) { return guild_loaded ? &guild_storage : nullptr; }
static void storage_storageclose(map_session_data* sd) {
    if (sd->storage.dirty) ++personal_saves;
    sd->state.storage_flag = 0;
}
static void storage_guild_storageclose(map_session_data* sd) {
    if (guild_storage.dirty) ++guild_saves;
    sd->state.storage_flag = 0;
}

#include "storage_native_audit_functions.inc"

static void require(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
static void add(s_storage& storage, int index) {
    auto& entry = storage.u.items_storage[index];
    entry.nameid = 501;
    entry.amount = 3;
    ++storage.amount;
}
static void empty(const s_storage& storage, const char* message) {
    require(storage.amount == 0, message);
    for (const auto& entry : storage.u.items_storage)
        require(entry.nameid == 0 && entry.amount == 0, message);
}
static void reset(map_session_data& sd) {
    sd = {};
    sd.storage.max_amount = MAX_STORAGE;
    guild_storage = {};
    guild_storage.max_amount = MAX_GUILD_STORAGE;
    guild_loaded = true;
    personal_saves = guild_saves = guild_logs = 0;
}

int main() {
    map_session_data sd;
    int cases = 0;
    for (bool guild : {false, true}) {
        for (int layout = 0; layout < 5; ++layout) {
            reset(sd);
            auto& storage = guild ? guild_storage : sd.storage;
            const int capacity = guild ? MAX_GUILD_STORAGE : MAX_STORAGE;
            if (layout == 1) { add(storage, 0); add(storage, 1); }
            if (layout == 2) { add(storage, 1); add(storage, capacity / 2); }
            if (layout == 3) { add(storage, capacity - 1); storage.max_amount = 1; }
            if (layout == 4) for (int i = 0; i < capacity; ++i) add(storage, i);
            const int occupied = storage.amount;
            const auto command = guild ? cleargstorage : clearstorage;
            require(command(1, &sd) == 0, "Cleanup command rejected valid storage");
            empty(storage, guild ? "Guild cleanup left occupied slots" : "Personal cleanup left occupied slots");
            require(!storage.lock, "Guild cleanup left the storage locked");
            require((guild ? guild_saves : personal_saves) == (occupied ? 1 : 0), "Dirty storage was not saved");
            if (guild) require(guild_logs == occupied, "Guild cleanup failed to log every deleted stack");
            require(command(1, &sd) == 0, "Repeated cleanup failed");
            empty(storage, "Repeated cleanup changed the empty storage count");
            ++cases;
        }
        for (int flag : {1, 2, 3}) {
            if (!guild && flag == 2) continue;
            reset(sd);
            auto& storage = guild ? guild_storage : sd.storage;
            add(storage, 1);
            sd.state.storage_flag = flag;
            require((guild ? cleargstorage : clearstorage)(1, &sd) == -1, "Open-storage guard was bypassed");
            require(storage.amount == 1 && storage.u.items_storage[1].amount == 3, "Rejected cleanup removed an item");
            ++cases;
        }
    }
    reset(sd);
    add(guild_storage, MAX_GUILD_STORAGE - 1);
    sd.guild = false;
    require(cleargstorage(1, &sd) == -1, "Missing guild must reject cleanup");
    sd.guild = true;
    guild_loaded = false;
    require(cleargstorage(1, &sd) == -1, "Unloaded guild storage must reject cleanup");
    require(guild_storage.amount == 1, "Rejected guild cleanup changed storage");
    require(clearstorage(1, nullptr) == -1 && cleargstorage(1, nullptr) == -1, "Missing session must reject cleanup");
    std::cout << "STORAGE_NATIVE_AUDIT_OK cases=" << cases + 3 << '\n';
}
