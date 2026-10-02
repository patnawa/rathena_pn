// Exact production market functions, real common Sql/SqlStmt and MariaDB.
// Explicit doubles: NPC catalog, item metadata, DBMap and poisoned allocations.
#include <common/sql.hpp>
#include <common/timer.hpp>
#include <cassert>
#include <cinttypes>
#include <cstring>
#include <cstdlib>
#include <deque>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
void display_helpscreen(bool) {}
int32 parse_console(const char*) {return 0;}
using t_itemid = uint32;
using DBKey = int;
struct npc_item_list { uint32 nameid, value; int32 qty; uint8 flag; };
struct s_npc_market { char exname[32]; uint16 count; npc_item_list* list; };
struct DBData { void* ptr; };
static void* db_data2ptr(DBData* p) { return p->ptr; }
constexpr int NPCTYPE_MARKETSHOP=1,NPC_NAME_LENGTH=32;
struct npc_data {
 int subtype=NPCTYPE_MARKETSHOP; char exname[32]="market";
 struct {struct {uint16 count=0; npc_item_list* shop_item=nullptr;}shop;}u;
};
struct item_data {int32 value_sell=1;std::string name="Fixture";};
static struct {
 bool exists(uint32 id) {return id==501 || id==502;}
 std::shared_ptr<item_data> find(uint32 id) {return exists(id)?std::make_shared<item_data>():nullptr;}
} item_db;
static npc_data npc;
static npc_data* npc_name2id(const char* name) {return std::strcmp(name,npc.exname)==0?&npc:nullptr;}
static std::map<std::string,s_npc_market*> markets;
static auto* NPCMarketDB=&markets;
static void* strdb_get(decltype(NPCMarketDB) db,const char* key) {auto it=db->find(key);return it==db->end()?nullptr:it->second;}
static void strdb_put(decltype(NPCMarketDB) db,const char* key,s_npc_market* value) {(*db)[key]=value;}
static size_t db_size(decltype(NPCMarketDB) db) {return db->size();}
static Sql* mmysql_handle;
static const char* market_table="market";
namespace pn_shop {
enum Kind {Market=1,Barter=2,Sale=3};
struct Stock {char name[32];uint32 key;};
struct Commit {Kind kind=Market;uint32 stock_count=1;Stock stocks[1]{{"market",502}};};
}
static bool pn_shop_sale_refresh(const pn_shop::Commit&) {return true;}
struct BarterEntry {uint32 stock;};
struct Barter {std::map<uint16,std::shared_ptr<BarterEntry>> items;};
static struct {std::shared_ptr<Barter> find(const char*){return nullptr;}} barter_db;
static unsigned char poison;
static std::unordered_map<void*,size_t> sizes;
static void* grow(void* old,size_t bytes) {
 void* next=std::malloc(bytes);assert(next);std::memset(next,poison,bytes);
 if(old){std::memcpy(next,old,sizes.at(old));sizes.erase(old);std::free(old);}
 sizes[next]=bytes;return next;
}
#undef RECREATE
#undef CREATE
#define RECREATE(p,type,count) ((p)=static_cast<type*>(grow((p),sizeof(type)*(count))))
#define CREATE(p,type,count) ((p)=static_cast<type*>(std::calloc((count),sizeof(type))))
#define ARR_FIND(begin,end,index,condition) for((index)=(begin);(index)<(end)&&!(condition);++(index)){}
#define ShowInfo(...) ((void)0)
#define ShowError(...) ((void)0)
#define ShowWarning(...) ((void)0)
#define ShowStatus(...) ((void)0)
#define npc_market_clearfromsql(name) npc_market_delfromsql_(name,0,true)
#define npc_market_delfromsql(name,id) npc_market_delfromsql_(name,id,false)
#include "market_sql_body.inc"
static void clear() {
 for(auto& entry:markets){sizes.erase(entry.second->list);std::free(entry.second->list);std::free(entry.second);}
 markets.clear();sizes.erase(npc.u.shop.shop_item);std::free(npc.u.shop.shop_item);npc={};
}
static void catalog(int32 quantity=7) {
 sizes.erase(npc.u.shop.shop_item);std::free(npc.u.shop.shop_item);npc={};
 npc.u.shop.shop_item=static_cast<npc_item_list*>(grow(nullptr,sizeof(npc_item_list)));
 npc.u.shop.shop_item[0]={501,100,quantity,0};npc.u.shop.count=1;
}
static void restore(s_npc_market* market,...) {
 DBData data{market};va_list ap;va_start(ap,market);npc_market_checkall_sub(0,&data,ap);va_end(ap);
}
static int32 quantity(uint32 id) {
 assert(Sql_Query(mmysql_handle,"SELECT amount FROM market WHERE name='market' AND nameid=%u",id)==SQL_SUCCESS);
 assert(Sql_NextRow(mmysql_handle)==SQL_SUCCESS);char* data=nullptr;Sql_GetData(mmysql_handle,0,&data,nullptr);
 int32 result=std::atoi(data);Sql_FreeResult(mmysql_handle);return result;
}
int main(int argc,char** argv) {
 assert(argc>=2);mmysql_handle=Sql_Malloc();
 assert(Sql_Connect(mmysql_handle,"root","market-fixture-only","market-db",3306,"market_probe")==SQL_SUCCESS);
 const std::string mode=argv[1];
 if(mode=="restore") {
  assert(argc==5);int32 expected=std::stoi(argv[2]);poison=std::stoi(argv[3]);int32 existing=std::stoi(argv[4]);
  catalog(existing);npc_market_fromsql();assert(markets.size()==1);restore(markets.begin()->second);
  assert(npc.u.shop.count==2 && npc.u.shop.shop_item[1].nameid==502);
  assert(npc.u.shop.shop_item[1].qty==expected && quantity(502)==expected);
  assert(npc.u.shop.shop_item[0].qty==(existing==-1?-1:9));
  assert(quantity(501)==(existing==-1?-1:9));
  restore(markets.begin()->second);assert(npc.u.shop.count==2 && quantity(502)==expected);
  std::cout<<"MARKET_SQL_RESTORE_PASS qty="<<expected<<" poison="<<int(poison)<<" existing="<<existing<<" reload=stable\n";
 } else if(mode=="fence") {
  catalog();npc_market_fromsql();restore(markets.begin()->second);assert(quantity(502)==7);
  Sql* owner=Sql_Malloc();assert(Sql_Connect(owner,"root","market-fixture-only","market-db",3306,"market_probe")==SQL_SUCCESS);
  assert(Sql_Query(owner,"START TRANSACTION")==SQL_SUCCESS);
  assert(Sql_Query(owner,"UPDATE market SET amount=6 WHERE name='market' AND nameid=502")==SQL_SUCCESS);
  pn_shop_inflight=true;pn_shop_queue.push_back({1,1,1,0});
  assert(pn_shop_stock_busy() && !pn_shop_queue_full());
  npc_market_tosql("market",&npc.u.shop.shop_item[1]);
  npc_market_delfromsql_("market",502,false);npc_market_delfromsql_("market",0,true);
  assert(quantity(502)==7); // MVCC: committed value while independent owner holds its row lock.
  // A catalog reload during the pending purchase may read old committed stock.
  clear();catalog();npc_market_fromsql();restore(markets.begin()->second);
  assert(npc.u.shop.shop_item[1].qty==7 && pn_shop_stock_busy());
  assert(Sql_Query(owner,"COMMIT")==SQL_SUCCESS);Sql_Free(owner);
  assert(quantity(502)==6);
  pn_shop::Commit request;
  assert(pn_shop_stock_refresh(request));
  assert(npc.u.shop.shop_item[1].qty==6 && pn_shop_stock_busy());
  pn_shop_inflight=false;pn_shop_queue.clear();
  clear();catalog();npc_market_fromsql();restore(markets.begin()->second);
  assert(npc.u.shop.shop_item[1].qty==6 && quantity(502)==6);
  std::cout<<"MARKET_SQL_FENCE_PASS committed=6 stale_writes_and_deletes=refused reloaded_pending=7 ack_refresh=6\n";
 } else {assert(false);}
 clear();Sql_Free(mmysql_handle);return 0;
}
