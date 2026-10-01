// Offline actual purchase functions. SQL transport is an explicit deterministic double.
#include "map/npc.hpp"
#include "map/pet.hpp"
#include "common/nullpo.hpp"
#include "common/utils.hpp"
#include "common/sql.hpp"
#include "common/showmsg.hpp"
#include <array>
extern char barter_table[],market_table[];
extern Sql* mmysql_handle;
namespace sql_fixture {
int mode=0,fail_at=1,calls=0,pending=-1;
std::array<int,2> persisted{{5,5}};
std::vector<std::function<int()>> requested;
bool failing(){return calls==fail_at&&mode!=0;}
int write(){if(!failing()||mode==3)persisted.at(pending)=requested.at(pending)();return failing()?SQL_ERROR:SQL_SUCCESS;}
struct Statement {
 int Prepare(const char*,...){pending=calls++;return calls==fail_at&&mode==1?SQL_ERROR:SQL_SUCCESS;}
 int Execute(){return write();}
};
void debug(Statement&){}
int query(Sql*,const char*,...){pending=calls++;return write();}
void debug_sql(Sql*){}
void reset(int kind,int count){mode=kind;fail_at=count;calls=0;pending=-1;persisted={5,5};requested.clear();}
}
// FUNCTION
extern "C" int __wrap_main(int argc,char** argv) {
 deny_network();static char server[]="shop-sql-offline";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"effective item parses");
 for(bool barter:{false,true})for(int mode:{0,1,2,3})for(int rows:{1,2}) {
  ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
  sd->status.inventory_slots=MAX_INVENTORY;sd->status.zeny=1000;sd->max_weight=1000000;
  for(auto& i:sd->equip_index)i=-1;
  sql_fixture::reset(mode,rows);
  std::vector<npc_item_list> sales(rows);npc_data nd={};nd.subtype=NPCTYPE_MARKETSHOP;fixture_shop=&nd;
  std::vector<s_npc_buy_list> orders;
  std::vector<s_barter_purchase> barters;auto shop=std::make_shared<s_npc_barter>();shop->name="offline";
  for(int i=0;i<rows;++i){
   sales[i].nameid=501+i;sales[i].value=100;sales[i].qty=5;orders.push_back({1,501u+i});
   auto entry=std::make_shared<s_npc_barter_item>();entry->nameid=501+i;entry->price=100;entry->stock=5;entry->stockLimited=true;entry->index=i;
   auto req=std::make_shared<s_npc_barter_requirement>();req->nameid=503;req->amount=1;req->refine=-1;entry->requirements[0]=req;barters.push_back({entry,1,nullptr});
   sql_fixture::requested.push_back(barter?std::function<int()>([entry]{return entry->stock;}):std::function<int()>([&,i]{return sales[i].qty;}));
  }
  nd.u.shop.count=rows;nd.u.shop.shop_item=sales.data();put(0,503,20);weight();
  auto result=barter?audit_npc_barter_purchase(*sd,shop,barters):audit_npc_buylist(sd.get(),orders);
  bool ok;
  const bool completed=mode==0||(!barter&&mode==3);
  if(completed){ok=result==e_purchase_result::PURCHASE_SUCCEED&&sd->status.zeny==1000-100*rows&&count(501)==1&&count(502)==rows-1&&count(503)==20-(barter?rows:0);}
  else {ok=result!=e_purchase_result::PURCHASE_SUCCEED&&sd->status.zeny==1000&&count(501)==0&&count(502)==0&&count(503)==20;}
  for(int i=0;i<rows;++i)ok=ok&&sql_fixture::persisted[i]==(completed?4:5)&&(barter?int(barters[i].item->stock):sales[i].qty)==(completed?4:5);
  std::printf("SQL path=%s mode=%d rows=%d result=%d zeny=%lld material=%d outputs=%d,%d memory=%d,%d database=%d,%d %s\n",barter?"barter":"market",mode,rows,int(result),(long long)sd->status.zeny,count(503),count(501),count(502),sql_fixture::requested[0](),rows==2?sql_fixture::requested[1]():-1,sql_fixture::persisted[0],sql_fixture::persisted[1],ok?"PASS":"FAIL");
  if(!ok)++errors;
 }
 attached=nullptr;item_db.clear();do_final_script();timer_final();db_final();malloc_final();std::printf("SQL_SUMMARY cases=%u correctness_failures=%u\n",cases,errors);return errors?1:0;
}
