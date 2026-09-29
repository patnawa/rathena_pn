void clif_parse_itempackage_select(int32,map_session_data*);
#include "common/socket.hpp"
extern "C" int32 announce(map_session_data*,uint32,uint32,unsigned char) asm("__wrap__Z35intif_broadcast_obtain_special_itemP16map_session_datajjh");
extern "C" int32 announce(map_session_data*,uint32,uint32,unsigned char){return 0;}
extern "C" void progress(const map_session_data*,unsigned long,uint32) asm("__wrap__Z16clif_progressbarPK16map_session_datamj");
extern "C" void progress(const map_session_data*,unsigned long,uint32){}
static std::unique_ptr<map_session_data> kplayer(int source=23919,int amount=10,int free_slots=MAX_INVENTORY-1){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();closes=0;
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 put(0,source,amount);for(int i=1;i<MAX_INVENTORY-free_slots;++i)put(i,501,1);weight();return sd;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==2,"directory");deny_network();static char server[]="kachua-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 std::string dir=argv[1],data=read(dir+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
 data=read(dir+"/groups.yml");auto groups=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:groups["Body"])check(itemdb_group.parseBodyNode(n)==1,"group parses");
 data=read(dir+"/packages.yml");auto packages=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:packages["Body"])check(item_package_db.parseBodyNode(n)==1,"package parses");
 auto code=compile(body(read(dir+"/npc-after.txt"),"Kachua's Secret Box#bm"),"Kachua actual NPC");
 for(int slots:{0,1}){auto sd=kplayer(23919,10,slots);Snapshot snap;walk(code,{1,1});snap.unchanged();}
 {auto sd=kplayer();Snapshot snap;walk(code,{1,2});snap.unchanged();}
 {auto sd=kplayer(23919,9);Snapshot snap;walk(code,{2,1});snap.unchanged();}
 {auto sd=kplayer();sd->max_weight=1;Snapshot snap;walk(code,{1,1});snap.unchanged();}
 {auto sd=kplayer();put(1,1000274,MAX_AMOUNT);weight();Snapshot snap;walk(code,{1,1});snap.unchanged();}
 {auto sd=kplayer();walk(code,{1,1});check(count(23919)==9,"one key charged");check(count(1000274)==1,"one mileage coupon");}
 {auto sd=kplayer();walk(code,{2,1});check(count(23919)==0,"ten keys charged");check(count(1000274)==10,"ten mileage coupons");}
 {auto sd=kplayer();bool stopped=false;walk(code,{2,1},[&](int,int){if(count(23919)==9&&!stopped){sd->max_weight=std::max(uint32(1),sd->weight);stopped=true;}});check(stopped&&count(23919)==9&&count(1000274)==1,"ten-draw interruption stops before the next charge");}
 for(int source:{23914,102701}){
  auto item=item_db.find(source);check(item->type==IT_DELAYCONSUME,"box not automatically consumed before validation");
  {auto sd=kplayer(source,2,0);Snapshot snap;walk(item->script,{});snap.unchanged();}
  {auto sd=kplayer(source,2);sd->max_weight=sd->weight;Snapshot snap;walk(item->script,{});snap.unchanged();}
  {auto sd=kplayer(source,2);walk(item->script,{});check(count(source)==1,"one box consumed on success");check(count(1000274)==(source==23914?1:0),"normal/event coupon policy preserved");}
 }
 for(int groupid:{IG_MAIN_LUCKY_BOX,IG_MAIN_LUCKY_BOX_}){
  auto sub=itemdb_group.find(groupid)->random.at(6);auto original=sub->data;auto total=sub->total_rate;
  for(const auto& pair:original){
   sub->data.clear();sub->data.emplace(pair);sub->total_rate=pair.second->adj_rate;
   int source=groupid==IG_MAIN_LUCKY_BOX?23919:102701;auto sd=kplayer(source,2);
   if(source==23919)walk(code,{1,1});else walk(item_db.find(source)->script,{});
   check(count(source)==1,"exactly one source consumed for each outcome");check(count(pair.second->nameid)==pair.second->amount,"every configured outcome grants its exact quantity");check(count(1000274)==(source==23919?1:0),"coupon policy for every outcome");
  }
  sub->data=original;sub->total_rate=total;
 }
 {
  auto sd=kplayer(102733,2,0);sd->status.base_level=275;Snapshot snap;
  struct __attribute__((packed)) Request{int16 type;uint16 index;uint32 aid,item,choice;} request{0xbaf,2,sd->status.account_id,102733,0};
  socket_data wire{};wire.rdata=reinterpret_cast<uint8*>(&request);session[42]=&wire;
  clif_parse_itempackage_select(42,sd.get());session[42]=nullptr;
  check(count(102733)==2,"selection box retained when reward inventory is full");snap.unchanged();
 }
 for(const auto& pack : item_package_db) for(const auto& choice : pack.second->groups){
  auto invoke=[&](map_session_data* sd,uint32 aid,uint32 group){
   struct __attribute__((packed)) Request{int16 type;uint16 index;uint32 aid,item,choice;} request{0xbaf,2,aid,pack.first,group};
   socket_data wire{};wire.rdata=reinterpret_cast<uint8*>(&request);session[42]=&wire;
   clif_parse_itempackage_select(42,sd);session[42]=nullptr;
  };
  {auto sd=kplayer(pack.first,2);sd->status.base_level=275;invoke(sd.get(),sd->status.account_id,choice.first);check(count(pack.first)==1,"selection charges exactly one box");
   for(const auto& reward:choice.second->items){check(count(reward.first)==reward.second->amount,"selection exact item and amount");
    for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid==reward.first){check(it.identify==1&&it.refine==reward.second->refine&&it.enchantgrade==reward.second->grade,"selection identification refinement grade");}
   }
  }
  uint64 rewardweight=0,slots=0;for(const auto& reward:choice.second->items){rewardweight+=uint64(item_db.find(reward.first)->weight)*reward.second->amount;slots+=itemdb_isstackable(reward.first)&&!reward.second->rentalhours?1:reward.second->amount;}
  if(rewardweight){auto sd=kplayer(pack.first,2);sd->status.base_level=275;sd->max_weight=0;Snapshot snap;invoke(sd.get(),sd->status.account_id,choice.first);snap.unchanged();}
  if(slots==1){auto sd=kplayer(pack.first,1,0);sd->status.base_level=275;invoke(sd.get(),sd->status.account_id,choice.first);check(count(pack.first)==0,"last box frees the required output slot");for(const auto& reward:choice.second->items)check(count(reward.first)==reward.second->amount,"reward uses consumed box slot");}
  {auto sd=kplayer(pack.first,2,0);sd->status.base_level=275;Snapshot snap;invoke(sd.get(),sd->status.account_id,choice.first);snap.unchanged();}
  {auto sd=kplayer(pack.first,2);sd->status.base_level=275;Snapshot snap;invoke(sd.get(),sd->status.account_id+1,choice.first);snap.unchanged();}
  {auto sd=kplayer(pack.first,2);sd->status.base_level=275;Snapshot snap;invoke(sd.get(),sd->status.account_id,UINT32_MAX);snap.unchanged();}
 }
 std::printf("KACHUA_TEST_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
