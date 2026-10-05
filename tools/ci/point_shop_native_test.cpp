#include "map/npc.hpp"
#include "map/pet.hpp"
#include "map/cashshop.hpp"
#include "common/nullpo.hpp"
#include "common/utils.hpp"
#include "map/atcommand.hpp"
#include "common/sql.hpp"
#include "common/showmsg.hpp"
extern char sales_table[],barter_table[];
extern Sql* mmysql_handle;
// FUNCTION
extern "C" bool actual_session_set(map_session_data*,int64,int64) asm("__real__Z9pc_setregP16map_session_datall");
extern "C" int __wrap_main(int argc,char**argv) {
 deny_network();static char server[]="point-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"catalog parse");
 auto egg=item_db.find(501);egg->type=IT_PETEGG;
 auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=501;pet->intimate=250;pet_db.put(1002,pet);
 auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->lv=1;mob->jname="Point pet";mob_db.put(1002,mob);
 for(const char* key:{"Points","#Points","##Points","@Points","#CASHPOINTS","#KAFRAPOINTS"})for(int mode=0;mode<5;++mode) {
  ++cases;nums.clear();durable_captured.reset();durable_boundary_calls=0;durable_allow=mode!=2;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;sd->status.account_id=99001;sd->status.char_id=99000002;
  sd->status.inventory_slots=1;sd->max_weight=1000000;sd->cashPoints=sd->kafraPoints=1000;
  for(auto& index:sd->equip_index)index=-1;
  const bool alias=!strcmp(key,"#CASHPOINTS")||!strcmp(key,"#KAFRAPOINTS");
  const int64 balance=mode==1?99:mode==4&&!alias?INT64_C(5000000000):1000;nums[add_str(key)]=balance;
  npc_data nd{};nd.subtype=NPCTYPE_POINTSHOP;std::strcpy(nd.u.shop.pointshop_str,key);fixture_shop=&nd;
  npc_item_list sale{};sale.nameid=501;sale.value=100;nd.u.shop.count=1;nd.u.shop.shop_item=&sale;
  std::vector<s_npc_buy_list> cart={{1,501}};
  const auto result=mode==3?audit_npc_cashshop_buy(sd.get(),501,1,0):audit_npc_cashshop_buylist(sd.get(),0,cart);
  const bool accepted=mode!=1&&mode!=2;
  check(nums[add_str(key)]==balance&&sd->cashPoints==1000&&sd->kafraPoints==1000&&count(501)==0,"no pre-ACK cost or pet delivery");
  check(result==(accepted?pn_shop::cash_pending:mode==1?ERROR_TYPE_MONEY:ERROR_TYPE_PURCHASE_FAIL),"correct pending/refusal result");
  if(accepted){check(durable_captured&&durable_captured->pet_count==1,"entitlement planned");const auto& r=*durable_captured;
   check(r.response==pn_shop::CashNpcResponse,"original NPC response retained");
   if(alias)check(r.point.scope==pn_shop::NoPoint&&r.cash_after==(!strcmp(key,"#CASHPOINTS")?900:1000)&&r.kafra_after==(!strcmp(key,"#KAFRAPOINTS")?900:1000),"cash aliases use canonical balances once");
   else check(!strcmp(r.point.key,key)&&r.point.before==balance&&r.point.after==balance-100&&r.point.scope==(key[0]=='@'?pn_shop::SessionPoint:key[0]=='#'?(key[1]=='#'?pn_shop::GlobalPoint:pn_shop::AccountPoint):pn_shop::CharacterPoint),"exact 64-bit point descriptor");
  }else check(!durable_captured,"failed dispatch has no plan");
 }
 // Certainty material outputs are ordinary items, never pets: receipt opt-in
 // must still plan exact point debit and withhold all assets before the ACK.
 for(const char* key:{"PNRewardAlicePoints","PNRewardBioPoints"})for(int mode=0;mode<4;++mode){
  ++cases;nums.clear();durable_captured.reset();durable_allow=mode!=2;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;sd->status.account_id=99001;sd->status.char_id=99000002;
  sd->status.inventory_slots=1;sd->max_weight=1000000;for(auto& index:sd->equip_index)index=-1;
  nums[add_str(key)]=mode==1?1:10;
  if(mode==3)put(0,501,1);
  npc_data nd{};nd.subtype=NPCTYPE_POINTSHOP;std::strcpy(nd.u.shop.pointshop_str,key);fixture_shop=&nd;
  npc_item_list sale{};sale.nameid=502;sale.value=2;nd.u.shop.count=1;nd.u.shop.shop_item=&sale;
  std::vector<s_npc_buy_list> cart={{1,502}};
  auto before=nums[add_str(key)];auto result=audit_npc_cashshop_buylist(sd.get(),0,cart);
  check(nums[add_str(key)]==before&&count(502)==0,"certainty purchase never mutates before ACK");
  check(result==(mode==0?pn_shop::cash_pending:mode==1?ERROR_TYPE_MONEY:ERROR_TYPE_PURCHASE_FAIL),"certainty failure or pending result exact");
  if(mode==0){const auto& r=*durable_captured;check(r.pet_count==0&&r.point.scope==pn_shop::CharacterPoint&&r.point.before==10&&r.point.after==8&&!strcmp(r.point.key,key),"nonpet guarantee uses durable character debit");check(r.items[0].nameid==502&&r.items[0].amount==1,"nonpet guarantee plans exact material");}
  else check(!durable_captured,"capacity/balance/transport failure gives no certainty plan");
 }
 ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->vars_ok=true;sd->regs.vars=i64db_alloc(DB_OPT_BASE);
 auto r=std::make_shared<pn_shop::Commit>();r->point.scope=pn_shop::SessionPoint;std::strcpy(r->point.key,"@Points");
 sd->shop_commit.request=r;sd->shop_commit.pending=true;auto id=add_str("@Points");
 check(!actual_session_set(sd.get(),id,90),"session cost cannot change while pending");
 sd->shop_commit.applying=true;check(actual_session_set(sd.get(),id,70),"ACK can apply session debit");
 sd->shop_commit.applying=false;r->point.scope=pn_shop::CharacterPoint;std::strcpy(r->point.key,"Points");
 check(!pc_setregistry(sd.get(),add_str("Points"),90),"persistent point key is fenced too");
 sd->shop_commit={};sd->regs.vars->destroy(sd->regs.vars,nullptr);sd->regs.vars=nullptr;attached=nullptr;sd.reset();r.reset();
 durable_captured.reset();egg.reset();pet_db.clear();mob_db.clear();pet.reset();mob.reset();item_db.clear();nums.clear();
 do_final_script();timer_final();db_final();malloc_final();std::printf("POINT_SHOP_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return errors?1:0;
}
