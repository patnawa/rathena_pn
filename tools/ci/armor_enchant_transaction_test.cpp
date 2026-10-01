static bool progress_seen=false;
static void armor_finish(){
 check(!attached->st,"NPC finishes without suspended transaction");
 if(attached->regs.arrays){attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db);attached->regs.arrays=nullptr;}
 attached=nullptr;
}
extern "C" void armor_progress(const map_session_data*,unsigned long,uint32) asm("__wrap__Z16clif_progressbarPK16map_session_datamj");
extern "C" void armor_progress(const map_session_data*,unsigned long,uint32){progress_seen=true;}
static std::unique_ptr<map_session_data> armor_player(){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();closes=0;progress_seen=false;
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 put(0,2307,1);sd->inventory.u.items_inventory[0].refine=7;sd->inventory.u.items_inventory[0].card[3]=4700;weight();return sd;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==3,"directory and source");deny_network();static char server[]="armor-enchant-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/armor-items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"real item parses");
 auto code=compile(body(read(argv[2]),"Apprentice Craftsman"),"actual armor enchant NPC");
 for(int flag=0;flag<8;++flag){
  auto sd=armor_player();auto& it=sd->inventory.u.items_inventory[0];
  if(flag==0)it.bound=1;if(flag==1)it.expire_time=2100000000;if(flag==2)it.option[0].id=1;
  if(flag==3)it.enchantgrade=1;if(flag==4)it.favorite=1;if(flag==5)it.identify=0;if(flag==6)it.attribute=1;
  if(flag==7){it.equip=EQP_ARMOR;sd->equip_index[EQI_ARMOR]=0;}
  Snapshot before;seed_roll(0,50);walk(code,{1,1,2});before.unchanged();armor_finish();
 }
 for(int change=0;change<4;++change){
  auto sd=armor_player();seed_roll(0,50);std::unique_ptr<Snapshot> after;
  walk(code,{1,1,2},[&](int,int){if(progress_seen&&!after){
   if(change==0)sd->status.zeny=0;
   if(change==1){sd->inventory.u.items_inventory[0]={};sd->inventory_data[0]=nullptr;weight();}
   if(change==2)sd->inventory.u.items_inventory[0].unique_id=42;
   if(change==3)sd->inventory.u.items_inventory[0].bound=2;
   after=std::make_unique<Snapshot>();
  }});check(after!=nullptr,"actual progressbar suspension reached");after->unchanged();armor_finish();
 }
 for(int roll:{0,49}){
  auto sd=armor_player();seed_roll(roll,50);walk(code,{1,1,2});
  check(sd->status.zeny==7654321-400000,"ordinary attempt costs exact advertised fee");
  check(count(2307)==(roll==0?1:0),"ordinary success/failure preserved");
  if(roll==0){const auto& it=sd->inventory.u.items_inventory[0];check(it.refine==0&&it.card[0]==0&&it.card[1]==0&&it.card[2]==0&&it.card[3]==4702,"advertised refine/card reset and enchant preserved");}
  armor_finish();
 }
 {
  auto sd=armor_player();put(0,2309,1);
  for(int i=1;i<MAX_INVENTORY-1;++i)put(i,2309,1);
  put(MAX_INVENTORY-1,2307,1);weight();seed_roll(0,50);walk(code,{1,1,2});
  check(count(2307)==1&&count(2309)==MAX_INVENTORY-1,"full inventory final-slot armor replaced without losing filler");
  check(sd->status.zeny==7654321-400000,"full inventory costs exact fee");armor_finish();
 }
 script_free_code(code);nums.clear();strings.clear();item_db.clear();
 do_final_script();timer_final();db_final();malloc_final();
 std::printf("ARMOR_ENCHANT_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
