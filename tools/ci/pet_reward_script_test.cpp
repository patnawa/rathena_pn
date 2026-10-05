// Appended to the native VM fixture prefix. SQL/transport ends at submit;
// the actual builtin, inventory planner, and Child Manager script run here.
#include "map/pet.hpp"
#include "map/mob.hpp"
#include "custom/shop_state.hpp"
static std::shared_ptr<pn_shop::Commit> submitted;
static bool accept_submission=true;
static bool group_submission=false;
extern "C" bool pet_submit(map_session_data&,std::shared_ptr<pn_shop::Commit>,std::vector<pn_shop::Event>,uint32_t)
 asm("__wrap__Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj");
extern "C" bool pet_submit(map_session_data&,std::shared_ptr<pn_shop::Commit> r,std::vector<pn_shop::Event> events,uint32_t){
 check(r->kind==(group_submission?pn_shop::ItemUse:pn_shop::Asset),"script uses durable asset transaction");
 if(!group_submission){check(events.size()==r->pet_count,"one deferred event per egg");for(auto& e:events)check(e.index==-1,"no delivery before acknowledgement");}
 submitted=std::move(r);return accept_submission;
}
static std::unique_ptr<map_session_data> pet_player(){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();submitted.reset();accept_submission=true;
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 put(0,1000103,12);put(1,1000103,18);weight();return sd;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==3,"fixture directory and actual NPC source");deny_network();static char server[]="pet-reward-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/pet-items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"effective item metadata parses");
 auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=9123;pet->intimate=250;pet_db.put(1002,pet);
 auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->lv=1;mob->jname="Pet script fixture";mob_db.put(1002,mob);
 auto group=std::make_shared<s_item_group_db>();group->id=1;
 auto subgroup=std::make_shared<s_item_group_random>();subgroup->total_rate=1;subgroup->algorithm=GROUP_ALGORITHM_RANDOM;
 auto entry=std::make_shared<s_item_group_entry>();entry->nameid=9123;entry->amount=2;entry->rate=entry->adj_rate=1;
 subgroup->data[0]=entry;group->random[1]=subgroup;itemdb_group.put(1,group);
 auto npc=compile(body(read(argv[2]),"script\tChild Manager"),"actual Child Manager");
 for(int mode=0;mode<6;++mode){
  auto sd=pet_player();
  if(mode==1)accept_submission=false;
  if(mode==2)sd->inventory.u.items_inventory[1].amount=17;
  if(mode==3)sd->inventory.u.items_inventory[1].equip=EQP_HEAD_TOP;
  if(mode==4)sd->mail.pending_slots=1;
  if(mode==5){sd->status.inventory_slots=2;sd->max_weight=0;}
  // Consuming all tickets makes the final weight zero; the egg remains claimable.
  weight();Snapshot before;walk(npc,{1,1});before.unchanged();
  check(bool(submitted)==(mode<2||mode==5),"submission only after eligible payment and idle state");
  if(submitted){check(submitted->pet_count==1,"one pet entitlement planned");check(!submitted->items[0].nameid&&!submitted->items[1].nameid,"split ticket stacks consumed in snapshot only");}
  attached=nullptr;
 }
 const char* calls[]={"getpetreward(9123,2,1000103,30)","getpetreward(9123,1)","getpetreward(9123,0)","getpetreward(9123,1,1000103)","getpetreward(1000103,1)","getpetreward(9123,1,1000103,-1)","getpetreward(9123,10000)"};
 for(int mode=0;mode<7;++mode){
  auto sd=pet_player();auto code=compile(std::string("{ @result=")+calls[mode]+"; end; }","pet reward command");
  Snapshot before;walk(code,{});before.unchanged();check(bool(submitted)==(mode<2),"invalid command cannot reach transport");
  check(nums[add_str("@result")]==(mode<2?1:0),"explicit submitted/refused result");
  if(submitted)check(submitted->pet_count==(mode==0?2:1),"quantity creates individual entitlements");
  script_free_code(code);attached=nullptr;
 }
 const char* grants[]={"getitem 9123,2;","getitembound 9123,2,1;","getitem2 9123,2,1,0,0,0,0,0,0;","makepet 1002;","getrandgroupitem 1,2,1,1;","rentitem 9123,60;","rentitem2 9123,60,1,0,0,0,0,0,0;"};
 for(int mode=0;mode<7;++mode){
  auto sd=pet_player();sd->status.inventory_slots=2;
  auto code=compile(std::string("{ ")+grants[mode]+" end; }","production pet grant family");
  Snapshot before;walk(code,{});before.unchanged();check(bool(submitted),"grant family uses durable transport with full inventory");
  check(submitted->pet_count==(mode==3 || mode>=5?1:2),"batch quantity retained");
  check(submitted->pets[0].output.egg.bound==(mode==1?1:0),"bound metadata retained");
  if(mode>=5)check(submitted->pets[0].output.egg.expire_time>time(nullptr),"rental expiry preserved in entitlement");
  script_free_code(code);attached=nullptr;
 }
 group_submission=true;
 for(uint16 id:{2,3}){auto definition=std::make_shared<s_item_group_db>();definition->id=id;auto all=std::make_shared<s_item_group_random>();all->algorithm=GROUP_ALGORITHM_ALL;
  auto ordinary=std::make_shared<s_item_group_entry>();ordinary->nameid=1201;ordinary->amount=1;all->data[0]=ordinary;
  if(id==2){auto egg=std::make_shared<s_item_group_entry>();egg->nameid=9123;egg->amount=1;all->data[1]=egg;}
  definition->random[0]=all;itemdb_group.put(id,definition);
 }
 for(int mode=0;mode<6;++mode){
  auto sd=pet_player();sd->vars_ok=true;sd->status.inventory_slots=(mode==3 || mode==4 || mode==5)?2:3;
  std::unique_ptr<DBMap,void(*)(DBMap*)> registry(i64db_alloc(DB_OPT_BASE),[](DBMap* db){db_destroy(db);});sd->regs.vars=registry.get();
  accept_submission=mode!=2;Snapshot before;
  const auto result=itemdb_group.pc_get_itemgroup(mode==4?1:(mode==0 || mode==5?3:2),true,*sd);
  if(mode==0){check(!submitted && result==0 && sd->inventory.u.items_inventory[2].nameid==1201,"ordinary group retains synchronous delivery");}
  else {before.unchanged();check(result==((mode==2 || mode==3)?4:0),"group refusal is explicit");
   check(bool(submitted)==(mode==1 || mode==2 || mode==4),"only complete mixed group reaches submit");
   if(submitted){check(submitted->pet_count==(mode==4?2:1),"outside-item-use group batches pet outputs");if(mode!=4)check(submitted->items[2].nameid==1201,"ordinary output shares immutable snapshot");}
  }
  if(sd->item_use)pn_item_use_settled(*sd,false);attached=nullptr;
 }
 group_submission=false;
 script_free_code(npc);submitted.reset();pet_db.clear();mob_db.clear();pet.reset();mob.reset();
 itemdb_group.clear();group.reset();subgroup.reset();entry.reset();nums.clear();strings.clear();item_db.clear();
 do_final_script();
 timer_final();
 db_final();
 malloc_final();
 std::printf("PET_REWARD_SCRIPT_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
