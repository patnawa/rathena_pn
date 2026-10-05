// Visual NPC effects have no world target in this isolated transaction fixture.
extern "C" block_list* socket_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* socket_world(int32){return nullptr;}
static std::unique_ptr<map_session_data> socket_player(){
 ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();closes=0;
 auto sd=std::make_unique<map_session_data>();attached=sd.get();
 sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;
 sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 put(0,2307,1);put(1,999,3);weight();seed_roll(49,100);return sd;
}
static void socket_finish(){
 check(!attached->st,"socket dialogue finishes");
 if(attached->regs.arrays){attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db);attached->regs.arrays=nullptr;}
 attached=nullptr;
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==4,"directory, source and function");deny_network();static char server[]="socket-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
 for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"real item parses");
 for(const char* helper:{"F_Socket_SelectPermanent","F_Socket_DeletePermanent"})
  strdb_put(script_get_userfunc_db(),helper,compile(body(read(std::string(argv[1])+"/socket_helpers.txt"),"function\tscript\t"+std::string(helper)+"\t"),helper));
 strdb_put(script_get_userfunc_db(),argv[3],compile(body(read(argv[2]),"function\tscript\t"+std::string(argv[3])+"\t"),"actual socket function"));
 auto code=compile("{ callfunc \""+std::string(argv[3])+"\",2307,2308,40,66,200,999,3; end; }","socket caller");
 for(int roll=0;roll<100;++roll){
  auto sd=socket_player();seed_roll(roll,100);walk(code,{1});
  check(count(2307)==0&&count(2308)==(roll>=40&&roll<65?1:0)&&count(999)==0,"advertised success/failure boundaries");
  check(sd->status.zeny==7454321,"exact normal fee");socket_finish();
 }
 for(int change=0;change<3;++change){
  auto sd=socket_player();std::unique_ptr<Snapshot> changed;
  walk(code,{1},[&](int,int chosen){
   if(chosen==1&&!changed&&std::find(messages.begin(),messages.end(),"You'd better pray for a successful result.")!=messages.end()){
    if(change==0)sd->status.zeny=0;
    if(change==1){sd->inventory.u.items_inventory[1].amount=2;weight();}
    if(change==2){sd->inventory.u.items_inventory[0]={};sd->inventory_data[0]=nullptr;weight();}
    changed=std::make_unique<Snapshot>();
   }
  });
  check(changed!=nullptr,"post-check dialogue boundary reached");changed->unchanged();socket_finish();
 }
 {
  auto sd=socket_player();Snapshot before;walk(code,{2});before.unchanged();socket_finish();
 }
 for(int bound=1;bound<BOUND_MAX;++bound){
  auto sd=socket_player();sd->inventory.u.items_inventory[0].bound=bound;
  walk(code,{1});check(count(2308)==1,"bound equipment converted");
  for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid==2308)
   check(it.bound==bound,"socket output retains equipment binding");
  socket_finish();
 }
 {
  auto sd=socket_player();sd->inventory.u.items_inventory[0].expire_time=2100000000;
  item rental=sd->inventory.u.items_inventory[0];put(2,2307,1);weight();
  walk(code,{1});check(count(2308)==1,"permanent equipment converted beside rental");
  check(std::memcmp(&rental,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"rental equipment untouched");
  check(sd->inventory.u.items_inventory[2].nameid!=2307,"permanent target consumed");socket_finish();
 }
 {
  auto sd=socket_player();sd->inventory.u.items_inventory[1].expire_time=2100000000;
  item rental=sd->inventory.u.items_inventory[1];put(2,999,3);weight();
  walk(code,{1});check(count(2308)==1,"permanent material recipe succeeds beside rental");
  check(std::memcmp(&rental,&sd->inventory.u.items_inventory[1],sizeof(item))==0,"rental materials untouched");
  check(sd->inventory.u.items_inventory[2].nameid!=999,"permanent materials consumed");socket_finish();
 }
 {
  auto sd=socket_player();put(1,999,1);put(2,999,2);weight();
  walk(code,{1});check(count(999)==0&&count(2308)==1,"split permanent material stacks consumed exactly");socket_finish();
 }
 {
  auto sd=socket_player();sd->inventory.u.items_inventory[0].expire_time=2100000000;
  Snapshot before;walk(code,{1});before.unchanged();socket_finish();
 }
 {
  auto sd=socket_player();sd->inventory.u.items_inventory[0].refine=9;
  sd->inventory.u.items_inventory[0].bound=BOUND_CHAR;
  item valuable=sd->inventory.u.items_inventory[0];put(2,2307,1);
  sd->inventory.u.items_inventory[2].bound=BOUND_ACCOUNT;weight();
  walk(code,{1});check(std::memcmp(&valuable,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"refined alternative preserved");
  for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid==2308)
   check(it.bound==BOUND_ACCOUNT,"binding belongs to exact preferred target");
  check(count(2308)==1,"ordinary alternative converted");socket_finish();
 }
 {
  auto sd=socket_player();Snapshot before;
  walk(code,{1},[&](int,int){
   if(std::find(messages.begin(),messages.end(),"You'd better pray for a successful result.")!=messages.end())sd->bank_ui.pending=true;
  });
  before.unchanged();sd->bank_ui.pending=false;socket_finish();
 }
 script_free_code(code);
 auto recipes_data=read(std::string(argv[1])+"/recipes.yml");
 auto recipes=ryml::parse_in_arena(ryml::to_csubstr(recipes_data));
 for(auto recipe:recipes[ryml::to_csubstr(argv[3])]){
  std::vector<int> args;std::string caller="{ callfunc \""+std::string(argv[3])+"\"";
  for(auto value:recipe){int arg;value>>arg;args.push_back(arg);caller+=","+std::to_string(arg);}
  caller+="; end; }";code=compile(caller,"configured socket recipe");
  for(int roll=0;roll<100;++roll){
   auto sd=socket_player();sd->status.zeny=2000000000;
   put(0,args[0],1);put(1,args[5],args[6]);
   if(args.size()==9)put(2,args[7],args[8]);
   weight();seed_roll(roll,100);walk(code,{1});
   check(count(args[0])==0,"configured target consumed");
   check(count(args[5])==0,"configured first material consumed");
   if(args.size()==9)check(count(args[7])==0,"configured second material consumed");
   check(count(args[1])==((roll+1>args[2]&&roll+1<args[3])?1:0),"configured exact success boundaries");
   check(sd->status.zeny==2000000000-args[4]*1000,"configured exact fee");socket_finish();
  }
  script_free_code(code);
 }
 if(std::string(argv[3])=="Func_Socket2"){
  code=compile(body(read(argv[2]),"Leablem#dummy::SocketEnchant2"),"actual Leablem NPC");
  auto hat_player=[](){
   auto sd=socket_player();sd->status.zeny=207654321;
   put(0,5022,1);put(1,969,2);weight();return sd;
  };
  for(int roll=0;roll<100;++roll){
   auto sd=hat_player();seed_roll(roll,100);walk(code,{3,1});
   check(count(5022)==0&&count(969)==0,"hat recipe consumes exact materials");
   check(count(5353)==(roll>=4&&roll<94?1:0),"hat exact ninety-percent success boundaries");
   check(sd->status.zeny==7654321,"hat exact two-hundred-million fee");socket_finish();
  }
  for(int change=0;change<3;++change){
   auto sd=hat_player();std::unique_ptr<Snapshot> changed;
   walk(code,{3,1},[&](int,int){
    if(!changed&&std::find(messages.begin(),messages.end(),"Pray to your gods for good luck.")!=messages.end()){
     if(change==0)sd->status.zeny=0;
     if(change==1){sd->inventory.u.items_inventory[1].amount=1;weight();}
     if(change==2){sd->inventory.u.items_inventory[0]={};sd->inventory_data[0]=nullptr;weight();}
     changed=std::make_unique<Snapshot>();
    }
   });
   check(changed!=nullptr,"hat final payment pause reached");changed->unchanged();socket_finish();
  }
  {auto sd=hat_player();Snapshot before;walk(code,{3,2});before.unchanged();socket_finish();}
  for(int bound=1;bound<BOUND_MAX;++bound){
   auto sd=hat_player();sd->inventory.u.items_inventory[0].bound=bound;
   walk(code,{3,1});check(count(5353)==1,"bound hat converted");
   for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid==5353)
    check(it.bound==bound,"hat output retains equipment binding");
   socket_finish();
  }
  {
   auto sd=hat_player();sd->inventory.u.items_inventory[0].expire_time=2100000000;
   sd->inventory.u.items_inventory[1].expire_time=2100000000;
   item rental_hat=sd->inventory.u.items_inventory[0],rental_gold=sd->inventory.u.items_inventory[1];
   put(2,5022,1);put(3,969,1);put(4,969,1);weight();walk(code,{3,1});
   check(count(5353)==1&&count(969)==2&&count(5022)==1,"hat consumes permanent target and split permanent Gold only");
   check(std::memcmp(&rental_hat,&sd->inventory.u.items_inventory[0],sizeof(item))==0,"rental hat preserved");
   check(std::memcmp(&rental_gold,&sd->inventory.u.items_inventory[1],sizeof(item))==0,"rental Gold preserved");socket_finish();
  }
  script_free_code(code);
 }
 nums.clear();strings.clear();item_db.clear();
 do_final_script();timer_final();db_final();malloc_final();
 std::printf("SOCKET_TRANSACTION_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
