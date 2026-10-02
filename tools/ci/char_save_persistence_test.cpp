#include <common/mmo.hpp>
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

// Deterministic boundaries; the writer and packet handler below are unmodified
// production function bodies, and use the actual mmo_charstatus layout.
static int checks = 0;
static void require(bool condition, const char* message) {
    ++checks;
    if (!condition) { std::cerr << "FAIL " << message << '\n'; std::exit(1); }
}
struct StringBuf { std::string text; };
static void StringBuf_Init(StringBuf* b) { b->text.clear(); }
static void StringBuf_Clear(StringBuf* b) { b->text.clear(); }
static void StringBuf_AppendStr(StringBuf* b, const char* s) { b->text += s; }
static const char* StringBuf_Value(StringBuf* b) { return b->text.c_str(); }
static void StringBuf_Printf(StringBuf* b, const char* fmt, ...) {
    char text[32768]; va_list ap; va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap); va_end(ap); b->text += text;
}
static constexpr int SQL_SUCCESS = 0, SQL_ERROR = -1;
static void* sql_handle = nullptr;
static bool transaction = false, begin_ok = true, end_ok = true, engine_ok = true, row_exists = true;
static int fail_write = 0, writes = 0, commits = 0, rollbacks = 0, fail_query = 0, queries = 0;
static std::vector<std::string> persisted, before;
static int Sql_QueryStr(void*, const char* text) {
    if (++queries == fail_query) return SQL_ERROR;
    const std::string query(text);
    if (query.find("SELECT") == 0 || query.find("SHOW") == 0) return SQL_SUCCESS;
    if (++writes == fail_write) return SQL_ERROR;
    persisted.push_back(query); return SQL_SUCCESS;
}
static int Sql_Query(void* db, const char* fmt, ...) {
    char text[65536]; va_list ap; va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap); va_end(ap); return Sql_QueryStr(db, text);
}
static void Sql_ShowDebug(void*) {}
static void Sql_FreeResult(void*) {}
static int Sql_NextRow(void*) { return row_exists ? SQL_SUCCESS : SQL_ERROR; }
static uint64 Sql_NumRows(void*) { return row_exists ? 1 : 0; }
static int Sql_GetData(void*, int, char** data, void*) {
    static char innodb[] = "InnoDB", myisam[] = "MyISAM";
    *data = engine_ok ? innodb : myisam; return SQL_SUCCESS;
}
static void Sql_EscapeString(void*, char* out, const char* in) { std::strcpy(out, in); }
static int Sql_BeginTransaction(void*) {
    if (!begin_ok || transaction) return SQL_ERROR;
    transaction = true; before = persisted; return SQL_SUCCESS;
}
static bool Sql_InTransaction(void*) { return transaction; }
static int Sql_EndTransaction(void*, bool commit) {
    require(transaction, "end without transaction"); transaction = false;
    if (!commit || !end_ok) { persisted = before; ++rollbacks; }
    else ++commits;
    return end_ok ? SQL_SUCCESS : SQL_ERROR;
}
static struct {
    const char* char_db = "char";
    const char* memo_db = "memo";
    const char* skill_db = "skill";
    const char* friend_db = "friends";
    const char* hotkey_db = "hotkey";
    const char* mercenary_owner_db = "mercenary_owner";
} schema_config;
static struct { bool save_log = false; } charserv_config;
static void ShowInfo(const char*, ...) {}
static void ShowError(const char*, ...) {}
static void ShowWarning(const char*, ...) {}
static bool mercenary_owner_tosql(uint32, mmo_charstatus*) {
    return Sql_Query(sql_handle, "REPLACE INTO mercenary_owner") == SQL_SUCCESS;
}
static std::map<uint32, std::shared_ptr<mmo_charstatus>> characters;
static auto& char_get_chardb() { return characters; }
namespace util {
template<class Map, class Key> auto umap_find(Map& m, Key key) -> typename Map::mapped_type {
    const auto it = m.find(key); return it == m.end() ? nullptr : it->second;
}
}
#include "char-writer.inc"

struct online_char_data { uint32 char_id = 7001; int32 server = 0; };
static std::map<uint32, std::shared_ptr<online_char_data>> online;
static auto& char_get_onlinedb() { return online; }
static constexpr int MAX_MAP_SERVERS = 2;
static struct { int fd = 9; } map_server[MAX_MAP_SERVERS];
static int offline_calls = 0, online_calls = 0, acks = 0, skipped = 0, eof_calls = 0;
static void char_set_char_offline(uint32, uint32) { ++offline_calls; }
static void char_set_char_online(int32, uint32, uint32) { ++online_calls; }
static void set_eof(int32) { ++eof_calls; }
static std::vector<unsigned char> incoming(65536), outgoing(65536);
static size_t incoming_size = 0;
#define RFIFOP(fd,n) (incoming.data() + (n))
#define RFIFOB(fd,n) (*reinterpret_cast<uint8*>(RFIFOP(fd,n)))
#define RFIFOW(fd,n) (*reinterpret_cast<uint16*>(RFIFOP(fd,n)))
#define RFIFOL(fd,n) (*reinterpret_cast<uint32*>(RFIFOP(fd,n)))
#define RFIFOREST(fd) incoming_size
#define RFIFOSKIP(fd,n) (skipped += (n))
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,n) (outgoing.data() + (n))
#define WFIFOW(fd,n) (*reinterpret_cast<uint16*>(WFIFOP(fd,n)))
#define WFIFOL(fd,n) (*reinterpret_cast<uint32*>(WFIFOP(fd,n)))
#define WFIFOSET(fd,n) (++acks)
#include "char-save-handler.inc"

static mmo_charstatus reset() {
    characters.clear(); online.clear();
    auto cp = std::make_shared<mmo_charstatus>();
    cp->char_id = 7001; cp->account_id = 9001; cp->base_level = 1;
    characters[cp->char_id] = cp;
    online[cp->account_id] = std::make_shared<online_char_data>();
    auto p = *cp;
    p.base_level = 12; p.hair = 3; p.mer_id = 21;
    std::strcpy(p.memo_point[0].map, "prontera"); p.memo_point[0].x = 100;
    p.skill[0].id = 1; p.skill[0].lv = 10; p.skill[0].flag = SKILL_FLAG_PERMANENT;
    p.friends[0].char_id = 7002; p.friends[0].account_id = 9002;
#ifdef HOTKEY_SAVING
    p.hotkeys[0].id = 1; p.hotkeys[0].lv = 10;
#endif
    transaction = false; begin_ok = end_ok = engine_ok = row_exists = true;
    fail_write = writes = commits = rollbacks = fail_query = queries = 0;
    persisted = {"previous committed state"}; before.clear();
    offline_calls = online_calls = acks = skipped = eof_calls = 0;
    return p;
}
static void packet(const mmo_charstatus& p, bool final = true) {
    incoming_size = sizeof(p) + 13;
    require(incoming_size <= incoming.size(), "fixture frame fits protocol");
    RFIFOW(9,0) = 0x2b01; RFIFOW(9,2) = incoming_size;
    RFIFOL(9,4) = p.account_id; RFIFOL(9,8) = p.char_id;
    RFIFOB(9,12) = final; std::memcpy(RFIFOP(9,13), &p, sizeof(p));
}
static void run(const std::string& which) {
    if (which == "writer-failure" || which == "all") {
        reset(); auto p = *characters[7001]; p.base_level = 12; fail_write = 1;
        require(char_mmo_char_tosql(p.char_id, &p) != 0, "SQL failure reported as successful character save");
        require(characters[p.char_id]->base_level == 1, "failed save changed cache");
    }
    if (which == "ack-failure" || which == "all") {
        reset(); auto p = *characters[7001]; p.base_level = 12; fail_write = 1; packet(p);
        chmapif_parse_reqsavechar(9, 0);
        require(acks == 0 && offline_calls == 0, "failed final save acknowledged and character released");
        require(skipped == incoming_size, "failed frame not consumed");
        fail_write = 0; packet(p); chmapif_parse_reqsavechar(9, 0);
        require(acks == 1 && offline_calls == 1, "successful retry not acknowledged exactly once");
        require(characters[p.char_id]->base_level == p.base_level, "successful retry not cached");
    }
    if (which == "atomicity" || which == "all") {
        auto p = reset(); require(char_mmo_char_tosql(p.char_id, &p) == 0, "control failed");
        const int total = writes;
        require(total >= 9, "did not exercise replacement and scalar writers");
        for (int failure = 1; failure <= total; ++failure) {
            p = reset(); fail_write = failure;
            require(char_mmo_char_tosql(p.char_id, &p) != 0, "one failed statement hidden");
            require(persisted == std::vector<std::string>{"previous committed state"}, "failed replacement left partial data");
            require(!transaction && commits == 0 && characters[p.char_id]->base_level == 1, "failed transaction leaked or cached");
            fail_write = 0;
            require(char_mmo_char_tosql(p.char_id, &p) == 0, "retry failed");
            require(commits == 1 && characters[p.char_id]->base_level == 12, "retry failed to commit/cache");
        }
        for (int boundary = 0; boundary < 4; ++boundary) {
            p = reset();
            if (boundary == 0) begin_ok = false;
            if (boundary == 1) end_ok = false;
            if (boundary == 2) engine_ok = false;
            if (boundary == 3) row_exists = false;
            require(char_mmo_char_tosql(p.char_id, &p) != 0, "transaction boundary failure hidden");
            require(persisted == std::vector<std::string>{"previous committed state"}, "boundary failure changed durable state");
            require(characters[p.char_id]->base_level == 1 && !transaction, "boundary failure changed cache or leaked transaction");
        }
        p = reset(); require(char_mmo_char_tosql(p.char_id + 1, &p) != 0, "wrong character identity accepted");
    }
    if (which == "packet-identity" || which == "all") {
        for (int fault = 0; fault < 4; ++fault) {
            auto p = reset(); packet(p);
            if (fault == 0) RFIFOL(9,4) += 1;
            if (fault == 1) RFIFOL(9,8) += 1;
            if (fault == 2) online[p.account_id]->char_id += 1;
            if (fault == 3) online[p.account_id]->server = 1;
            map_server[1].fd = 10;
            chmapif_parse_reqsavechar(9, 0);
            require(acks == 0 && writes == 0 && offline_calls == 0, "unowned or mismatched final save accepted");
        }
    }
}
int main(int argc, char** argv) {
    run(argc > 1 ? argv[1] : "all");
    std::cout << "PASS character save persistence: " << checks << " assertions\n";
}
