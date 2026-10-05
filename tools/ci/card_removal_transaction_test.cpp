// Actual card-removal NPC VM and native inventory commands. Transport, registry,
// equip callbacks and persistence use the documented crown-fixture boundaries.
#include "map/npc.hpp"
static std::unique_ptr<map_session_data> card_player(){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();closes=0;fail_unequip=fail_equip=false;unequip_hook={};quest_hook={};
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 put(0,400547,1,true);sd->equip_index[EQI_HEAD_TOP]=0;
 sd->inventory.u.items_inventory[0].card[0]=4365;
 sd->inventory.u.items_inventory[0].refine=7;sd->inventory.u.items_inventory[0].enchantgrade=3;
 sd->inventory.u.items_inventory[0].bound=1;sd->inventory.u.items_inventory[0].favorite=1;
 put(1,1000,2);put(2,715,2);weight();return sd;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==3,"directory and mode");deny_network();static char server[]="card-removal-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 std::string dir=argv[1],data=read(dir+"/card-items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
 auto globals=read(dir+"/globals.txt");
 for(auto name:{"F_getpositionname","F_InsertComma"})strdb_put(script_get_userfunc_db(),name,compile(body(globals,"function\tscript\t"+std::string(name)),name));
 auto source=read(dir+"/card-npc.txt");
 // Per-NPC constants have no cross-player behavior; localize them only because
 // this fixture has no world NPC registry. All dialogue/payment commands remain.
 for(auto key:{"zenycost","percardcost","faildestroy"}){
   std::string old="."+std::string(key),value=".@"+std::string(key);size_t at=0;
   while((at=source.find(old,at))!=std::string::npos){source.replace(at,old.size(),value);at+=value.size();}
 }
 const std::string helper=source.find("function\tscript\tF_CardRemovalStopped")!=std::string::npos?"F_CardRemovalStopped":"F_CardRemovalRefund";
 strdb_put(script_get_userfunc_db(),helper.c_str(),compile(body(source,"function\tscript\t"+helper),"service stop"));
 auto code=compile(body(source,"Wise Old Woman#eAcustom"),"actual card NPC");
 for(int roll=0;roll<100;++roll)for(int preference:{2,3}){
   auto sd=card_player();seed_roll(roll,100);auto original=sd->inventory.u.items_inventory[0];
   walk(code,{1,1,1,preference},[&](int,int){
     if(sd->status.zeny!=7654321 && sd->st->state!=CLOSE)
       check(false,"disconnect-safe: no dialogue suspension after fee debit before card removal");
   });
   check(sd->status.zeny==7654321-225000,"exact fee for advertised outcome");
   check(count(1000)==1&&count(715)==1,"one material payment");
   bool destroyed=roll<2||(roll<8&&preference==3);
   check(count(400547)==(destroyed?0:1),"equipment survival matches selected outcome");
   check(count(4365)==(roll>=10||(roll>=2&&roll<8&&preference==3)?1:0),"card delivery matches selected outcome");
   if(!destroyed){if(roll<8||roll>=10)original.card[0]=0;check(std::memcmp(&original,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"equipment metadata preserved");}
 }
 for(int change=0;change<4;++change){
   auto sd=card_player();seed_roll(50,100);std::unique_ptr<Snapshot> snapshot;
   walk(code,{1,1,1,2},[&](int,int chosen){
     if(chosen==3&&sd->st->state==RERUNLINE&&!snapshot){
       if(change==0)sd->status.zeny=0;
       if(change==1){sd->inventory.u.items_inventory[1]={};sd->inventory_data[1]=nullptr;weight();}
       if(change==2)sd->inventory.u.items_inventory[0].unique_id=42;
       if(change==3)sd->inventory.u.items_inventory[0].card[0]=0;
       snapshot=std::make_unique<Snapshot>();
     }
   });
   check(snapshot!=nullptr,"final confirmation exercised");snapshot->unchanged();
 }
 {
   auto sd=card_player();seed_roll(50,100);fail_unequip=true;
   sd->inventory.u.items_inventory[1].bound=1;sd->inventory.u.items_inventory[2].bound=2;
   Snapshot snapshot;walk(code,{1,1,1,2});snapshot.unchanged();
 }
 for(int failure=0;failure<3;++failure){
   auto sd=card_player();seed_roll(50,100);
   if(failure==0){for(int i=3;i<MAX_INVENTORY;++i)put(i,1000,1);weight();}
   if(failure==1)sd->max_weight=sd->weight;
   if(failure==2){sd->inventory.u.items_inventory[1]={};sd->inventory_data[1]=nullptr;weight();}
   Snapshot snapshot;walk(code,failure==2?std::vector<int>{1,1,1}:std::vector<int>{1,1});snapshot.unchanged();
 }
 for(const auto& choices:{std::vector<int>{3},std::vector<int>{1,1,2},std::vector<int>{1,1,1,1}}){
   auto sd=card_player();Snapshot snapshot;walk(code,choices);snapshot.unchanged();
 }
 {
   auto sd=card_player();seed_roll(50,100);
   sd->inventory.u.items_inventory[1].bound=1;sd->inventory.u.items_inventory[2].bound=2;
   auto star=sd->inventory.u.items_inventory[1],gem=sd->inventory.u.items_inventory[2];--star.amount;--gem.amount;
   walk(code,{1,1,1,2});
   check(std::memcmp(&star,&sd->inventory.u.items_inventory[1],sizeof(item))==0&&std::memcmp(&gem,&sd->inventory.u.items_inventory[2],sizeof(item))==0,"bound fee stacks consumed in place");
   Snapshot snapshot;walk(code,{1,1});snapshot.unchanged();
 }
 auto direct=[](const std::string& expression){
   auto test=compile("{ $@__SWfee_VAL = "+expression+"; end; }","direct card fee builtin");
   walk(test,{});script_free_code(test);return nums[add_str("$@__SWfee_VAL")];
 };
 for(const auto& expression:{
   "successremovecards(EQI_HEAD_TOP,-1,1000,715)",
   "successremovecards(EQI_HEAD_TOP,100,1000,1000)",
   "successremovecards(EQI_HEAD_TOP,100,1000,4365)",
   "successremovecards(EQI_HEAD_TOP,100,1000,0)",
   "successremovecards(EQI_HEAD_TOP,100,1000)",
   "failedremovecards(EQI_HEAD_TOP,3,-1,1000,715)"}) {
   auto sd=card_player();Snapshot before;check(direct(expression)==0,"invalid fee args rejected");before.unchanged();
 }
 for(int failure=0;failure<5;++failure){
   auto sd=card_player();
   if(failure==0)sd->bank_ui.pending=true;
   if(failure==1)sd->inventory.u.items_inventory[1].equipSwitch=EQP_HEAD_TOP;
   if(failure==2)sd->inventory.u.items_inventory[1].amount=0;
   if(failure==3)sd->inventory.u.items_inventory[0].card[0]=0;
   if(failure==4)sd->status.zeny=99;
   Snapshot before;check(direct("successremovecards(EQI_HEAD_TOP,100,1000,715)")==0,"unavailable paid service rejected");before.unchanged();
 }
 for(int change=0;change<3;++change){
   auto sd=card_player();auto original=sd->inventory.u.items_inventory[0];
   unequip_hook=[&](){
     if(change==0)sd->status.zeny=0;
     if(change==1){sd->inventory.u.items_inventory[1]={};sd->inventory_data[1]=nullptr;weight();}
     if(change==2)sd->bank_ui.pending=true;
   };
   check(direct("successremovecards(EQI_HEAD_TOP,100,1000,715)")==0,"post-unequip payment revalidated");
   check(std::memcmp(&original,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"callback abort retains equipment and cards");
   check(count(4365)==0&&count(715)==2&&count(1000)==(change==1?0:2),"callback abort does not consume fees or return cards");
   check(sd->status.zeny==(change==0?0:7654321),"callback abort does not debit fee");
 }
 for(int mode:{-1,0,1,2,3}){
   auto sd=card_player();std::string expression=mode<0?"successremovecards(EQI_HEAD_TOP)":"failedremovecards(EQI_HEAD_TOP,"+std::to_string(mode)+")";
   check(direct(expression)==1,"legacy no-fee command completes");
   check(sd->status.zeny==7654321&&count(1000)==2&&count(715)==2,"legacy no-fee commands do not charge");
 }
 for(int mode:{-1,0,1,2,3}){
   auto sd=card_player();sd->inventory.u.items_inventory[0].card[0]=0;Snapshot before;
   std::string expression=mode<0?"successremovecards(EQI_HEAD_TOP)":"failedremovecards(EQI_HEAD_TOP,"+std::to_string(mode)+")";
   check(direct(expression)==1,"legacy no-card no-op remains successful");before.unchanged();
 }
 for(int mode:{0,1,2,3}){
   auto sd=card_player();unsigned observed=0;
   quest_hook=[&](){
     if(sd->status.zeny==7654321){check(count(1000)==2&&count(715)==2,"precommit quest refresh observes no fee consumption");return;}
     ++observed;check(sd->status.zeny==7654321-100&&count(1000)==1&&count(715)==1,"final quest refresh observes whole payment");
     check(count(400547)==(mode==0||mode==2?0:1),"final quest refresh observes final equipment outcome");
   };
   check(direct("failedremovecards(EQI_HEAD_TOP,"+std::to_string(mode)+",100,1000,715)")==1,"paid failure outcome completes");
   check(observed>0,"post-commit quest refresh exercised");
 }
 for(int mode:{-1,0,1,2,3,4,5}){
   auto sd=card_player();map_num=1;sd->m=0;
   auto nd=std::make_unique<npc_data>();nd->id=NPC;nd->type=BL_NPC;nd->m=0;
   quest_npc=nd.get();auto previous_fake=fake_nd;fake_nd=nd.get();native_quest=true;
   map[0].qi_npc={NPC};sd->qi_display.resize(1);
   auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
   qi->condition=compile(mode==4?
     "{ $@__SWcalls_VAL++; if(countitem(4365)>0) Zeny=0; achievement_condition(0); }":
     "{ $@__SWcalls_VAL++; if (Zeny != 7654321 && (countitem(1000)!=1 || countitem(715)!=1)) $@__SWpartial_VAL=1; achievement_condition(0); }","real quest fee observer");
   nd->qi_data.push_back(qi);
   const auto expression=mode<0?"successremovecards(EQI_HEAD_TOP,100,1000,715)":"failedremovecards(EQI_HEAD_TOP,"+std::to_string(mode)+",100,1000,715)";
   if(mode==4){
     check(direct("successremovecards(EQI_HEAD_TOP,100,1000,715)")==0,"real card-output quest callback depleting fee aborts operation");
     check(count(4365)==0&&count(1000)==2&&count(715)==2,"output callback abort rolls card back without consuming fee materials");
     check(sd->inventory.u.items_inventory[0].card[0]==4365,"output callback abort preserves compounded card");
   }else if(mode==5){
     // Sensitivity: the real quest script detects the old charge/delete
     // interleaving when callback deferral is deliberately omitted here.
     check(pc_payzeny(sd.get(),100,LOG_TYPE_SCRIPT)==0,"sensitivity fixture debits fee");
     check(pc_delitem(sd.get(),1,1,0,0,LOG_TYPE_SCRIPT)==0,"sensitivity fixture uses ordinary deletion");
     check(nums[add_str("$@__SWpartial_VAL")]==1,"real observer detects deliberately partial payment");
   }else check(direct(expression)==1,"paid outcome with real quest condition completes");
   check(nums[add_str("$@__SWcalls_VAL")]>0,"real quest condition executed");
   if(mode!=5)check(nums[add_str("$@__SWpartial_VAL")]==0,"real quest condition never sees partial fee consumption");
   native_quest=false;quest_npc=nullptr;fake_nd=previous_fake;map[0].qi_npc.clear();
 }
 std::printf("CARD_NPC_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}


