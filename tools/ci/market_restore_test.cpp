// Production restoration body; explicit NPC, allocation and SQL boundaries.
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <vector>
using int32 = int32_t;
using uint16 = uint16_t;
using DBKey = int;
struct npc_item_list { uint32_t nameid, value; int32 qty; uint8_t flag; };
struct s_npc_market { char exname[32]; uint16 count; npc_item_list* list; };
struct DBData { void* ptr; };
static void* db_data2ptr(DBData* d) { return d->ptr; }
constexpr int NPCTYPE_MARKETSHOP = 1;
struct npc_data {
    int subtype = NPCTYPE_MARKETSHOP;
    char exname[32] = "market";
    struct { struct { uint16 count; npc_item_list* shop_item; } shop; } u{};
};
static npc_data npc;
static bool present = true;
static unsigned char fill = 0;
static std::vector<npc_item_list> writes;
static int deleted = 0, cleared = 0;
static npc_data* npc_name2id(const char*) { return present ? &npc : nullptr; }
static struct { bool exists(uint32_t id) { return id == 501 || id == 502; } } item_db;
static void npc_market_tosql(const char*, npc_item_list* p) { writes.push_back(*p); }
static void npc_market_delfromsql(const char*, uint32_t) { ++deleted; }
static void npc_market_clearfromsql(const char*) { ++cleared; }
#define ShowInfo(...) ((void)0)
#define ShowError(...) ((void)0)
#define ARR_FIND(begin, end, index, condition) for ((index)=(begin); (index)<(end) && !(condition); ++(index)) {}
static npc_item_list* grow(npc_item_list* old, size_t count) {
    auto* p = static_cast<npc_item_list*>(std::malloc(count * sizeof(*old)));
    assert(p);
    std::memset(p, fill, count * sizeof(*old));
    if (old) std::memcpy(p, old, (count-1) * sizeof(*old));
    std::free(old);
    return p;
}
#define RECREATE(result, type, count) ((result) = grow((result), (count)))
#include "market_restore_body.inc"

static void restore(s_npc_market* market, ...) {
    DBData data{market}; va_list args; va_start(args, market);
    npc_market_checkall_sub(0, &data, args); va_end(args);
}
static void reset(int32 existing = 5) {
    std::free(npc.u.shop.shop_item); npc = {}; present = true;
    npc.u.shop.count = 1;
    npc.u.shop.shop_item = static_cast<npc_item_list*>(std::calloc(1, sizeof(npc_item_list)));
    npc.u.shop.shop_item[0] = {501, 10, existing, 0};
    writes.clear(); deleted = cleared = 0;
}
static void require(bool value, const char* reason) {
    if (!value) { std::cerr << "FAIL " << reason << " fill=" << int(fill) << '\n'; std::exit(1); }
}
int main() {
    int cases = 0;
    for (unsigned char poison : {0x00, 0x7f, 0x80, 0xff}) {
        fill = poison;
        for (int32 stock : {0, 1, 37, std::numeric_limits<int32>::max(), -1}) {
            reset(); npc_item_list entry{502, 123, stock, 1};
            s_npc_market market{"market", 1, &entry}; restore(&market);
            require(npc.u.shop.count == 2, "restored dynamic entry missing");
            require(npc.u.shop.shop_item[1].qty == stock, "restored quantity depends on allocator bytes");
            require(writes.size() == 1 && writes[0].qty == stock && writes[0].value == 123 && writes[0].flag == 1,
                    "SQL write did not preserve persisted fields");
            // A second restoration must be stable, with no duplicate catalog row.
            restore(&market);
            require(npc.u.shop.count == 2 && writes.size() == 2 && writes.back().qty == stock, "reload changed restored stock");
            ++cases;
        }
    }
    for (int32 initial : {-1, 0, 8}) {
        reset(initial); npc_item_list entry{501, 77, 9, 0};
        s_npc_market market{"market", 1, &entry}; restore(&market);
        require(npc.u.shop.shop_item[0].qty == (initial == -1 ? -1 : 9), "existing unlimited/limited policy changed");
        ++cases;
    }
    reset(); npc_item_list entry{502, 123, 4, 0}; s_npc_market market{"market", 1, &entry};
    restore(&market); require(deleted == 1 && writes.empty(), "obsolete row not removed"); ++cases;
    reset(); entry.nameid = 999; entry.flag = 1; restore(&market);
    require(deleted == 1 && writes.empty(), "invalid item restored"); ++cases;
    reset(); present = false; restore(&market); require(cleared == 1, "missing NPC not handled"); ++cases;
    std::free(npc.u.shop.shop_item);
    std::cout << "MARKET_RESTORE_PASS " << cases << " cases\n";
}
