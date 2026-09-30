#include <custom/item_use.hpp>
#include <custom/shop_state.hpp>
#include <map/pet.hpp>
#include <map/mob.hpp>
#include <map/npc.hpp>
#include <map/chrif.hpp>
#include <common/mapindex.hpp>
static bool allow_submit=true;
static unsigned submissions=0,baseline_saves=0,world_calls=0;
static item baseline_item{};
static std::shared_ptr<pn_shop::Commit> captured;
static std::vector<pn_shop::Event> captured_events;
static mob_data* capture_target=nullptr;
static unsigned removals=0,kills=0,roulettes=0;
static bool roulette_success=false;
extern "C" mob_data* target_lookup(int32) asm("__wrap__Z9map_id2mdi");
extern "C" mob_data* target_lookup(int32 id){return capture_target && capture_target->id==id?capture_target:nullptr;}
extern "C" int32 remove_target(block_list*,clr_type,const char*,int32,const char*) asm("__wrap__Z16unit_remove_map_P10block_list8clr_typePKciS3_");
extern "C" int32 remove_target(block_list* target,clr_type,const char*,int32,const char*){check(attached && !attached->shop_commit.pending,"target removal after durable result");++removals;target->prev=nullptr;return 0;}
extern "C" int32 kill_target(block_list*,block_list*,int8,int8,int8,uint8) asm("__wrap__Z21status_percent_changeP10block_listS0_aaah");
extern "C" int32 kill_target(block_list*,block_list*,int8,int8,int8,uint8){++kills;return 0;}
extern "C" void roulette(const map_session_data&,bool) asm("__wrap__Z17clif_pet_rouletteRK16map_session_datab");
extern "C" void roulette(const map_session_data&,bool success){++roulettes;roulette_success=success;}
extern "C" bool submit(map_session_data&,std::shared_ptr<pn_shop::Commit>,std::vector<pn_shop::Event>,uint32_t) asm("__wrap__Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj");
extern "C" bool submit(map_session_data& sd,std::shared_ptr<pn_shop::Commit> request,std::vector<pn_shop::Event> events,uint32_t weight){
 ++submissions;check(!pn_item_use_active(&sd),"scope finalized before submit");
 check(!memcmp(&baseline_item,&sd.inventory.u.items_inventory[0],sizeof(item)),"original input restored before baseline save");
 check(sd.status.zeny==1000 && sd.status.uniqueitem_counter==10,"original wallet and counter restored before submit");
 if(!allow_submit)return false;
 ++baseline_saves;captured=std::make_shared<pn_shop::Commit>(*request);captured_events=events;
 sd.shop_commit.pending=true;sd.shop_commit.request=captured;sd.shop_commit.events=events;sd.shop_commit.final_weight=weight;return true;
}
extern "C" e_setpos warp_boundary(map_session_data*,uint16,int32,int32,clr_type) asm("__wrap__Z9pc_setposP16map_session_datatii8clr_type");
extern "C" e_setpos warp_boundary(map_session_data* sd,uint16,int32,int32,clr_type){check(!pn_item_use_active(sd),"ordinary warp bypasses pet scope");++world_calls;return SETPOS_OK;}
extern "C" int __wrap_main(int argc,char** argv){
 deny_network();static char server[]="item-use-pet-test";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();mapindex_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
 item_db.find(501)->type=IT_PETEGG;item_db.find(502)->type=IT_USABLE;item_db.find(502)->flag.group=false;
 auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=501;pet->itemID=502;pet->capture=10000;pet->intimate=250;pet_db.put(1002,pet);
 auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->lv=1;mob->jname="Item use pet";mob_db.put(1002,mob);
 auto* original_script=item_db.find(502)->script;auto fake=std::make_unique<npc_data>();fake->id=NPC;fake_nd=fake.get();map[0].initMapFlags();
 const char* scripts[]={"{ getitem 501,1; }","{ getitem 501,1; }","{ getitem 501,1; getitem 501,2; }","{ getitem 503,1; getitem 501,1; }","{ getitem 503,1; }","{ warp \"SavePoint\",0,0; }","{ warp \"SavePoint\",0,0; getitem 501,1; }","{ Zeny+=25; getitem 501,1; }","{ mes \"Choice\"; next; getitem 501,1; }",
 "{ catchpet 502; }","{ catchpet 502; }","{ catchpet 502; }","{ catchpet 502; }","{ catchpet 502; }","{ catchpet 502; }","{ catchpet 502; }"};
 for(unsigned mode=0;mode<std::size(scripts);++mode){
  ++cases;captured.reset();captured_events.clear();submissions=baseline_saves=world_calls=0;allow_submit=mode!=1;
  removals=kills=roulettes=0;roulette_success=false;item_db.find(502)->flag.delay_consume=mode==10;
  auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99001;sd->status.char_id=99000002;sd->type=BL_PC;
  sd->status.uniqueitem_counter=10;sd->status.zeny=1000;sd->status.inventory_slots=1;sd->max_weight=1000000;sd->status.base_level=200;sd->status.sex=SEX_MALE;
  sd->permissions.set(PC_PERM_ITEM_UNCONDITIONAL);sd->vars_ok=true;sd->ud.skilltimer=INVALID_TIMER;sd->rental_timer=INVALID_TIMER;
  sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
  safestrncpy(sd->status.save_point.map,"prontera",sizeof(sd->status.save_point.map));
  std::unique_ptr<DBMap,void(*)(DBMap*)> registry(i64db_alloc(DB_OPT_BASE),[](DBMap* db){db_destroy(db);});sd->regs.vars=registry.get();
  for(auto& index:sd->equip_index)index=-1;for(auto& timer:sd->eventtimer)timer=INVALID_TIMER;
  put(0,502,1);
  item unrelated_a{},unrelated_b{},retained_egg{};
  if(mode>=9){
   sd->status.inventory_slots=5;put(1,503,3);put(4,503,2);
   auto& a=sd->inventory.u.items_inventory[1];a.bound=BOUND_ACCOUNT;a.refine=3;a.card[1]=4001;
   auto& b=sd->inventory.u.items_inventory[4];b.bound=BOUND_CHAR;b.refine=7;b.card[2]=4002;
   unrelated_a=a;unrelated_b=b;
   put(3,501,1);auto& egg=sd->inventory.u.items_inventory[3];egg.card[0]=CARD0_PET;egg.card[1]=321;egg.bound=BOUND_ACCOUNT;egg.refine=4;retained_egg=egg;
  }
  weight();baseline_item=sd->inventory.u.items_inventory[0];
  if(mode==0){
   auto ordinary=pn_shop_request(*sd,pn_shop::Asset);std::vector<pn_shop::Event> events;uint32 final_weight=0;
   check(!pn_shop_plan_inventory(*sd,*ordinary,{},nullptr,events,final_weight),"empty grants remain rejected outside ItemUse");
  }
  auto* code=parse_script(scripts[mode],"item-use",1,0);check(code!=nullptr,"item script compiles");item_db.find(502)->script=code;
  int result=pc_useitem(sd.get(),0);const bool pending=mode==0 || mode==2 || mode==3 || mode==7;
  if(mode>=9){
   check(result==1 && pn_item_use_capture_waiting(sd.get()) && pc_transaction_locked(sd.get()),"lure stays reserved across target selection");
   check(!submissions && !roulettes && !memcmp(&baseline_item,&sd->inventory.u.items_inventory[0],sizeof(item)),"no capture cost or result before selection");
   auto target=std::make_unique<mob_data>();target->id=8123;target->type=BL_MOB;target->prev=target.get();target->mob_id=1002;target->level=1;target->status.hp=target->status.max_hp=100;target->db=mob;capture_target=target.get();
   if(mode==9 || mode==10)pet_catch_cancel(*sd);
   else if(mode==11)pet_catch_process_end(*sd,0);
   else if(mode==15)do_timer(gettick()+30001);
   else {if(mode==14)target->pet_capture_token=999;pet_catch_process_end(*sd,target->id);}
   const bool caught=mode==12 || mode==13;
   bool ok=captured && captured->pet_count==(caught?1:0) && !captured->items[0].nameid && !removals && !kills && !roulettes && sd->shop_commit.pending;
   check(captured && !memcmp(&captured->items[1],&unrelated_a,sizeof(item)) && !memcmp(&captured->items[4],&unrelated_b,sizeof(item)),"capture snapshot preserves unrelated slots quantities and metadata");
   check(!memcmp(&sd->inventory.u.items_inventory[1],&unrelated_a,sizeof(item)) && !memcmp(&sd->inventory.u.items_inventory[4],&unrelated_b,sizeof(item)),"waiting result keeps original unrelated inventory live");
   check(submissions==1 && baseline_saves==1,"lure is committed exactly once on cancel failure success and expiry");
   check(!memcmp(&captured->items[3],&retained_egg,sizeof(item)) && !memcmp(&sd->inventory.u.items_inventory[3],&retained_egg,sizeof(item)) && !captured->pet_retire_count,"cursor and commit retain existing pet identity without retirement");
   const bool committed=mode!=12;sd->shop_commit={};pn_item_use_settled(*sd,committed);
   if(!committed)check(!memcmp(&sd->inventory.u.items_inventory[0],&baseline_item,sizeof(item)),"SQL rejection preserves the original lure");
   const auto settled_removals=removals,settled_roulettes=roulettes;pn_item_use_settled(*sd,committed);check(removals==settled_removals && roulettes==settled_roulettes,"duplicate settlement cannot remove a target or acknowledge twice");
   if(mode!=15){const auto before=submissions;do_timer(gettick()+30001);check(submissions==before,"settled cursor timeout cannot consume lure again");}
   ok=ok && roulettes==1 && roulette_success==(caught&&committed) && removals==(caught&&committed?1:0) && kills==removals && !sd->item_use;
   if(!ok)++errors;printf("ITEM_USE_PET case=%u capture %s\n",mode,ok?"PASS":"FAIL");capture_target=nullptr;item_db.find(502)->script=original_script;script_free_code(code);attached=nullptr;continue;
  }
  bool ok=bool(captured)==pending && bool(sd->shop_commit.pending)==pending;
  if(pending){ok=ok && result==1 && submissions==1 && baseline_saves==1 && captured->kind==pn_shop::ItemUse && captured->pet_count==(mode==2?3:1) && !memcmp(&baseline_item,&sd->inventory.u.items_inventory[0],sizeof(item));if(mode==3)ok=ok && captured->items[0].nameid==503 && captured_events.size()==2;if(mode==7)ok=ok && captured->wallet_before==1000 && captured->wallet_after==1025;sd->shop_commit={};pn_item_use_settled(*sd,false);}
  else if(mode==4)ok=ok && result==1 && sd->inventory.u.items_inventory[0].nameid==503 && !submissions;
  else if(mode==5)ok=ok && result==1 && !sd->inventory.u.items_inventory[0].nameid && world_calls==1 && !submissions;
  else ok=ok && result==0 && !memcmp(&baseline_item,&sd->inventory.u.items_inventory[0],sizeof(item)) && !world_calls;
  ok=ok && !sd->item_use;if(!ok)++errors;printf("ITEM_USE_PET case=%u result=%d submissions=%u %s\n",mode,result,submissions,ok?"PASS":"FAIL");
  item_db.find(502)->script=original_script;script_free_code(code);attached=nullptr;
 }
 printf("ITEM_USE_PET cases=%u failures=%u\n",cases,errors);
 fake_nd=nullptr;fake.reset();pet_db.clear();mob_db.clear();pet.reset();mob.reset();item_db.clear();do_final_script();mapindex_final();timer_final();db_final();malloc_final();return errors?1:0;
}

