// Exact shop handler bodies and native inventory/payment/quest execution.
// World lookup, client transport, SQL-free catalogue and registries are fixtures.
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
// Equipment legality, inventory mutation and EquipScript are native. Combat
// stat recalculation is an explicit boundary; no combat world starts here.
extern "C" void delivery_status_boundary(block_list*,std::bitset<SCB_MAX>,uint8) asm("__wrap__Z15status_calc_bl_P10block_listSt6bitsetILm45EEh");
extern "C" void delivery_status_boundary(block_list*,std::bitset<SCB_MAX>,uint8){}
// FUNCTION
extern "C" int __wrap_main(int argc,char** argv){
 deny_network();static char server[]="shop-delivery-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto raw=read(std::string(argv[1])+"/items.yml");auto rows=ryml::parse_in_arena(ryml::to_csubstr(raw));
 for(auto row:rows["Body"])check(item_db.parseBodyNode(row)==1,"item parses");
 int failures=0;map_num=1;map[0].initMapFlags();
 for(int mode=0;mode<24;++mode){
  const int kind=mode%4;const bool metadata=mode>=4;const bool positive=mode>=20;const int outputs=metadata?1:2;
  ++cases;nums.clear();auto sd=std::make_unique<map_session_data>();attached=sd.get();
  sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;sd->m=0;
  sd->status.inventory_slots=metadata&&!positive?2:3;sd->status.zeny=10000;sd->cashPoints=10000;sd->max_weight=1000000;
  sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
  for(auto& i:sd->equip_index)i=-1;
  script_reg_num registry[2]{};registry[0].value=sd->cashPoints;sd->vars_ok=true;sd->regs.vars=i64db_alloc(DB_OPT_BASE);
  i64db_put(sd->regs.vars,add_str(CASHPOINT_VAR),&registry[0]);i64db_put(sd->regs.vars,add_str(KAFRAPOINT_VAR),&registry[1]);
  auto nd=std::make_unique<npc_data>();nd->id=NPC;nd->type=BL_NPC;nd->m=0;nd->subtype=kind==2?NPCTYPE_CASHSHOP:NPCTYPE_SHOP;
  fixture_shop=nd.get();quest_npc=nd.get();fake_nd=nd.get();map[0].qi_npc={NPC};sd->qi_display.resize(1);
  auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
  qi->condition=compile("{ if (!$@__SWfilled_VAL && countitem(501)>0 && countitem(502)==0) { $@__SWfilled_VAL=1; getitem 503,1; getitem 504,1; } achievement_condition(0); }","fill inventory between shop outputs");
  if(!metadata)nd->qi_data.push_back(qi);
  if(metadata){put(0,501,1);auto& existing=sd->inventory.u.items_inventory[0];
   if(mode<8||positive)existing.bound=1;
   else if(mode<12)existing.expire_time=2100000000;
   else if(mode<16)existing.card[0]=123;
   else existing.unique_id=123456;put(1,503,1);weight();}
  const auto inventory_before=sd->inventory;
  npc_item_list sales[2]{};for(int i=0;i<2;++i){sales[i].nameid=501+i;sales[i].value=100;sales[i].qty=-1;}
  nd->u.shop.count=outputs;nd->u.shop.shop_item=sales;
  std::vector<s_npc_buy_list> buy={{1,501}};if(!metadata)buy.push_back({1,502});bool success=false;
  if(kind==0)success=audit_npc_buylist(sd.get(),buy)==e_purchase_result::PURCHASE_SUCCEED;
  if(kind==1){
   auto barter=std::make_shared<s_npc_barter>();std::vector<s_barter_purchase> purchases;
   for(int i=0;i<outputs;++i){auto entry=std::make_shared<s_npc_barter_item>();entry->nameid=501+i;entry->price=100;entry->stockLimited=false;
    auto req=std::make_shared<s_npc_barter_requirement>();req->nameid=503;req->amount=1;req->refine=-1;if(!metadata)entry->requirements[0]=req;purchases.push_back({entry,1,nullptr});}
   if(!metadata){put(0,503,2);weight();}success=audit_npc_barter_purchase(*sd,barter,purchases)==e_purchase_result::PURCHASE_SUCCEED;
  }
  if(kind==2)success=audit_npc_cashshop_buylist(sd.get(),0,buy)==ERROR_TYPE_NONE;
  if(kind==3){
   auto tab=std::make_shared<s_cash_item_tab>();tab->tab=CASHSHOP_TAB_NEW;
   PACKET_CZ_SE_PC_BUY_CASHITEM_LIST_sub buy[2]{};
   for(int i=0;i<outputs;++i){auto entry=std::make_shared<s_cash_item>();entry->nameid=501+i;entry->price=100;tab->items.push_back(entry);buy[i].itemId=501+i;buy[i].amount=1;buy[i].tab=CASHSHOP_TAB_NEW;}
   cash_shop_db.put(CASHSHOP_TAB_NEW,tab);success=audit_cashshop_buylist(sd.get(),0,outputs,buy);cash_shop_db.clear();
  }
  const auto balance=kind<2?sd->status.zeny:sd->cashPoints;
  const bool full=success&&count(501)==1&&count(502)==1&&balance==9800;
  const bool aborted=!success&&count(501)==0&&count(502)==0&&balance==10000&&(kind!=1||count(503)==2);
  const bool case_ok=metadata?(positive?
   (success&&balance==9900&&count(501)==2&&sd->inventory.u.items_inventory[0].bound==1&&sd->inventory.u.items_inventory[2].nameid==501&&!sd->inventory.u.items_inventory[2].bound):
   (!success&&balance==10000&&!std::memcmp(&inventory_before,&sd->inventory,sizeof(inventory_before)))):full||aborted;
  std::printf("DELIVERY mode=%d success=%d balance=%lld first=%d second=%d filled=%lld %s\n",mode,success,static_cast<long long>(balance),count(501),count(502),static_cast<long long>(nums[add_str("$@__SWfilled_VAL")]),case_ok?"PASS":"FAIL");
  if(!case_ok)++failures;
  map[0].qi_npc.clear();quest_npc=nullptr;fake_nd=nullptr;
  sd->regs.vars->destroy(sd->regs.vars,nullptr);sd->regs.vars=nullptr;
 }
 // Native achievement conditions and quest conditions observe scope timing.
 // The condition returns false, so achievement persistence/rewards are not used.
 for(int mode=0;mode<4;++mode){
  ++cases;nums.clear();auto sd=std::make_unique<map_session_data>();attached=sd.get();
  sd->id=sd->status.account_id=99000001;sd->type=BL_PC;sd->m=0;sd->status.inventory_slots=3;sd->max_weight=1000000;
  sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
  script_reg_num argument{};sd->vars_ok=true;sd->regs.vars=i64db_alloc(DB_OPT_BASE);i64db_put(sd->regs.vars,add_str("ARG0"),&argument);
  for(auto& i:sd->equip_index)i=-1;
  auto nd=std::make_unique<npc_data>();nd->id=NPC;nd->type=BL_NPC;quest_npc=nd.get();fake_nd=nd.get();map[0].qi_npc={NPC};sd->qi_display.resize(1);
  auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
  qi->condition=compile("{ $@__SWquests_VAL++; achievement_condition(0); }","scope quest observer");nd->qi_data.push_back(qi);
  auto achievement=std::make_shared<s_achievement_db>();achievement->achievement_id=99000001;achievement->group=AG_GET_ITEM;
  achievement->condition=compile("{ $@__SWach_VAL++; if(countitem(502)==0) $@__SWpartial_VAL++; achievement_condition(0); }","scope achievement observer");
  achievement_db.put(achievement->achievement_id,achievement);battle_config.feature_achievement=true;
  auto add=[&](int id){item it{};it.nameid=id;it.identify=1;check(pc_additem(sd.get(),&it,1,LOG_TYPE_NPC)==ADDITEM_SUCCESS,"scope item added");};
  if(mode==0){add(501);check(nums[add_str("$@__SWach_VAL")]==1&&nums[add_str("$@__SWquests_VAL")]==1,"callbacks remain immediate outside scope");add(502);}
  if(mode==1||mode==2){
   {PcItemDeliveryScope outer(*sd);add(501);
    if(mode==2){PcItemDeliveryScope inner(*sd);add(502);inner.refresh_questinfo();}else add(502);
    check(nums[add_str("$@__SWach_VAL")]==0&&nums[add_str("$@__SWquests_VAL")]==0,"nested scope cannot flush outer delivery");}
   check(nums[add_str("$@__SWpartial_VAL")]==0,"all achievement callbacks see complete delivery");
  }
  if(mode==3){auto early=[&](){PcItemDeliveryScope delivery(*sd);add(501);return;};early();}
  const int added=mode==3?1:2;
  check(nums[add_str("$@__SWach_VAL")]==added,"each actual addition receives its achievement event");
  check(nums[add_str("$@__SWquests_VAL")]==(mode==0?2:1),"outer scope flushes quest refresh exactly once, including early return");
  if(mode==0||mode==3)check(nums[add_str("$@__SWpartial_VAL")]==1,"unbatched and early-return observers remain active");
  map[0].qi_npc.clear();quest_npc=nullptr;fake_nd=nullptr;achievement_db.clear();
  sd->regs.vars->destroy(sd->regs.vars,nullptr);sd->regs.vars=nullptr;
  std::printf("SCOPE mode=%d PASS\n",mode);
 }
 battle_config.feature_achievement=false;
 auto armor=item_db.find(2301);armor->flag.autoequip=true;
 armor->equip_script=compile("{ $@__SWequip_VAL++; $@__SWsafe_VAL=(countitem(501)==1); }","native autoequip observer");
 for(int batched=0;batched<2;++batched){
  ++cases;nums.clear();auto sd=std::make_unique<map_session_data>();attached=sd.get();
  sd->id=sd->status.account_id=99000001;sd->type=BL_PC;sd->m=0;sd->status.inventory_slots=3;sd->max_weight=1000000;
  sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;sd->status.sex=SEX_MALE;sd->status.base_level=1;sd->status.job_level=1;
  sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
  for(auto& i:sd->equip_index)i=-1;
  auto nd=std::make_unique<npc_data>();nd->id=NPC;nd->type=BL_NPC;quest_npc=nd.get();fake_nd=nd.get();
  auto add=[&](int id){item it{};it.nameid=id;it.identify=1;check(pc_additem(sd.get(),&it,1,LOG_TYPE_NPC)==ADDITEM_SUCCESS,"autoequip item added");};
  if(batched){PcItemDeliveryScope batch(*sd);add(2301);add(501);check(nums[add_str("$@__SWequip_VAL")]==0,"autoequip deferred during batch");}
  else{add(2301);add(501);}
  check(nums[add_str("$@__SWequip_VAL")]==1,"actual EquipScript executes once");
  check(nums[add_str("$@__SWsafe_VAL")]==batched,"actual EquipScript sees later output only after batch ends");
  check(sd->inventory.u.items_inventory[0].equip==EQP_ARMOR,"actual native equipment state updated");
  quest_npc=nullptr;fake_nd=nullptr;std::printf("AUTOEQUIP batched=%d PASS\n",batched);
 }
 // Actual native VM and native timer dispatch: no script-side mutation may
 // touch the immutable player state before a durable receipt resolves.
 {
  ++cases;nums.clear();auto sd=std::make_unique<map_session_data>();attached=sd.get();
  sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;sd->m=0;
  sd->status.inventory_slots=3;sd->max_weight=1000000;sd->status.zeny=100;
  sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;sd->status.sex=SEX_MALE;sd->status.base_level=1;sd->status.job_level=1;
  sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;for(auto& i:sd->equip_index)i=-1;
  auto nd=std::make_unique<npc_data>();nd->id=NPC;nd->type=BL_NPC;quest_npc=nd.get();fake_nd=nd.get();
  put(0,2301,1);sd->inventory.u.items_inventory[0].equip=EQP_ARMOR;sd->equip_index[EQI_ARMOR]=0;weight();
  auto* pending_script=compile("{ successrefitem EQI_ARMOR,1; Zeny -= 7; $@__SWresume_VAL++; end; }","pending purchase script fence");
  sd->shop_commit.pending=true;
  run_script(pending_script,0,sd->id,NPC);
  check(sd->inventory.u.items_inventory[0].refine==0&&sd->status.zeny==100&&nums[add_str("$@__SWresume_VAL")]==0,"pending script pauses before metadata and payment mutations");
  sd->shop_commit.pending=false;
  do_timer(gettick()+150);
  check(sd->inventory.u.items_inventory[0].refine==1&&sd->status.zeny==93&&nums[add_str("$@__SWresume_VAL")]==1,"native timer resumes the preserved script once after unlock");
  check(sd->st==nullptr,"resumed script completes and detaches");
  script_free_code(pending_script);quest_npc=nullptr;fake_nd=nullptr;
  std::printf("PENDING_SCRIPT native_timer_resume PASS\n");
 }
 check(durable_boundary_calls==0,"synchronous controls never enter durable purchase boundary");
 std::printf("SHOP_DELIVERY cases=%u failures=%d\n",cases,failures);
 check(errors==0,"native fixture has no script diagnostics");
 attached=nullptr;armor.reset();item_db.clear();job_db.clear();do_final_script();timer_final();db_final();malloc_final();return failures?1:0;
}
