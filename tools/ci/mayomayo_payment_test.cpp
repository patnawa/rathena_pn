static std::unique_ptr<map_session_data> may_player(bool reset){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();closes=unequips=0;fail_unequip=false;fail_equip=false;unequip_hook={};
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 put(0,1224,1);auto& weapon=sd->inventory.u.items_inventory[0];weapon.equip=EQP_HAND_R;sd->equip_index[EQI_HAND_R]=0;
 if(reset)weapon.card[3]=4700;
 put(1,reset?6417:6422,reset?1:15);weight();seed_roll(0,531);return sd;
}
static void may_finish(){
 check(!attached->st,"dialogue cleanly finishes");
 if(attached->regs.arrays){attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db);attached->regs.arrays=nullptr;}attached=nullptr;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==3,"directory and source");deny_network();static char server[]="mayomayo-refund-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/armor-items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"real item parses");
 auto globals=read("npc/other/Global_Functions.txt");
 for(auto name:{"F_IsEquipIDHack","F_IsEquipRefineHack","F_IsEquipCardHack"})strdb_put(script_get_userfunc_db(),name,compile(body(globals,"function\tscript\t"+std::string(name)),name));
 auto code=compile(body(read(argv[2]),"\tMayomayo#mal\t"),"actual Mayomayo NPC");
 for(bool reset:{true,false}){
  const int currency=reset?6417:6422,amount=reset?1:15;
  const std::vector<int> answers=reset?std::vector<int>{3,2}:std::vector<int>{2,2,2};
  for(int flag=0;flag<12;++flag){
   auto sd=may_player(reset);auto& coin=sd->inventory.u.items_inventory[1];
   if(flag==0)coin.bound=1;if(flag==1)coin.expire_time=2100000000;if(flag==2)coin.favorite=1;
   if(flag==3)coin.identify=0;if(flag==4)coin.attribute=1;if(flag==5)coin.refine=1;
   if(flag==6)coin.enchantgrade=1;if(flag==7)coin.card[0]=4700;if(flag==8)coin.option[4].id=1;if(flag==9)coin.unique_id=42;
   if(flag==10)coin.option[4].value=1;if(flag==11)coin.option[4].param=1;
   Snapshot before;walk(code,answers);before.unchanged();check(unequips==0,"unsupported payment refused before native mutation");may_finish();
  }
  for(bool reject:{false,true}){
   auto sd=may_player(reset);fail_unequip=reject;auto before=sd->inventory.u.items_inventory[0];walk(code,answers);
   check(unequips==1,"native mutation reached with normal plain payment");
   check(count(currency)==(reject?amount:0),"plain payment charged or refunded exactly");
   auto expected=before;if(!reject)expected.card[3]=reset?0:4787;
   check(std::memcmp(&expected,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"weapon exact metadata and intended enchant/reset result");may_finish();
  }
  {
   auto sd=may_player(reset);put(2,currency,1);sd->inventory.u.items_inventory[2].bound=2;weight();Snapshot before;
   walk(code,answers);before.unchanged();check(unequips==0,"mixed plain and bound stacks refused even when plain stack suffices");may_finish();
  }
  {
   auto sd=may_player(reset);std::unique_ptr<Snapshot> changed;
   walk(code,answers,[&](int,int chosen){if(chosen==(reset?1:2)&&sd->st->state==RERUNLINE&&!changed){sd->inventory.u.items_inventory[1]={};sd->inventory_data[1]=nullptr;weight();changed=std::make_unique<Snapshot>();}});
   check(changed!=nullptr,"final dialogue mutation exercised");changed->unchanged();may_finish();
  }
 }
 for(bool reject:{false,true}){
  auto sd=may_player(false);sd->inventory.u.items_inventory[1].amount=7;put(2,6422,8);weight();fail_unequip=reject;
  walk(code,{2,2,2});check(count(6422)==(reject?15:0),"split plain stacks charge/refund exact total");may_finish();
 }
 {
  auto sd=may_player(false);bool filled=false;
  walk(code,{2,2,2},[&](int,int chosen){if(chosen==2&&sd->st->state==RERUNLINE&&!filled){
   auto coin=sd->inventory.u.items_inventory[1];for(int i=1;i<MAX_INVENTORY-1;++i)put(i,1201,1);put(MAX_INVENTORY-1,6422,15);sd->inventory.u.items_inventory[MAX_INVENTORY-1]=coin;weight();filled=true;
  }});check(filled&&count(6422)==0&&count(1201)==MAX_INVENTORY-2,"full 200-row final payment scan succeeds without limit or filler loss");may_finish();
 }
 script_free_code(code);nums.clear();strings.clear();item_db.clear();do_final_script();timer_final();db_final();malloc_final();
 std::printf("MAYOMAYO_PAYMENT_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
