// Native world adapter for the private expedition; no live services.
int64 clock_now=1900000000;
std::string room="PN Weekly Expedition",where="guild_vs1";
int x=51,next_gid=5000;bool town=true;
std::set<int> live;
std::vector<int> choices;
std::vector<std::tuple<int,int,int64>> changes;
std::map<int64,int64> global_numbers;
bool expedition_world(script_state* st,const std::string& command){
 if(command=="gettimetick")script_pushint(st,clock_now);
 else if(command=="select"){boundary(!choices.empty(),"explicit menu choice");script_pushint(st,choices.front());choices.erase(choices.begin());}
 else if(command=="instance_live_info")script_pushstrcopy(st,room.c_str());
 else if(command=="strcharinfo")script_pushstrcopy(st,script_getnum(st,2)==3?where.c_str():"Challenger");
 else if(command=="getmapxy"){
  auto set=[&](int argument,int64 number,const char* text){auto* data=script_getdata(st,argument);auto key=reference_getuid(data);if(text)set_reg_str(st,nullptr,key,get_str(reference_getid(data)),text,reference_getref(data));else set_reg_num(st,nullptr,key,get_str(reference_getid(data)),number,reference_getref(data));};
  set(2,0,where.c_str());set(3,x,nullptr);set(4,50,nullptr);script_pushint(st,0);
 }
 else if(command=="getmapflag")script_pushint(st,script_getnum(st,3)==MF_TOWN&&town);
 else if(command=="unitexists")script_pushint(st,live.count(script_getnum(st,2)));
 else if(command=="setunitdata")changes.emplace_back(script_getnum(st,2),script_getnum(st,3),script_getnum64(st,4));
 else if(command=="killmonsterall")live.clear();
 else if(command=="instance_create")script_pushint(st,1);
 else if(command=="instance_enter")script_pushint(st,IE_OK);
 else if(command=="setnpctimer"||command=="initnpctimer"||command=="stopnpctimer"||command=="setmapflag"||command=="setmapflagnosave"||command=="instance_destroy"){}
 else return false;
 return true;
}
// MAIN
extern "C" bool expedition_global_write(int64,int64) asm("__wrap__Z13mapreg_setregll");
extern "C" bool expedition_global_write(int64 key,int64 value){global_numbers[key]=value;return true;}
extern "C" int64 expedition_global_read(int64) asm("__wrap__Z14mapreg_readregl");
extern "C" int64 expedition_global_read(int64 key){return global_numbers[key];}
extern "C" map_session_data* expedition_char(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* expedition_char(int32 id){for(auto& p:players)if(p->status.char_id==id)return p.get();return nullptr;}
extern "C" int __wrap_main(int argc,char** argv){
 boundary(argc==2,"fixture directory supplied");fixture_dir=argv[1];deny_network();static char name[]="weekly-expedition-test";SERVER_NAME=name;
 malloc_init();db_init();do_init_database();timer_init();install_world_doubles();do_init_script();battle_set_defaults();
 npc.id=NPC;npc.type=BL_NPC;npc.instance_id=1;
 instances[1]=std::make_shared<s_instance_data>();instances[1]->state=INSTANCE_BUSY;instances[1]->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
 for(int i=0;i<2;++i){auto p=std::make_unique<map_session_data>();p->id=p->status.account_id=99000010+i;p->status.char_id=100+i;p->status.base_level=150;p->type=BL_PC;p->battle_status.hp=1000;p->state.ignoretimeout=true;p->vars_ok=true;p->npc_idle_timer=INVALID_TIMER;players.emplace_back(std::move(p));}
 auto load=[&](const char* title,const char* file,bool function){std::ifstream input(fixture_dir+"/"+file+".txt");std::string text{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};auto* code=parse_script(text.c_str(),title,1,0);boundary(code!=nullptr,"actual source parses");if(function)strdb_put(script_get_userfunc_db(),title,code);else codes[title]=code;};
 // LOAD
 auto exec=[&](const std::string& text,unsigned who=0){auto* code=parse_script(("{"+text+"end;}").c_str(),"expedition fixture",1,0);boundary(code!=nullptr,"fixture parses");attached=players[who].get();run_script(code,0,attached->id,NPC);boundary(!attached->st,"fixture ends synchronously");script_free_code(code);};
 auto result=[&](){return registries[players[0]->id]["Result"];};
 auto run=[&](int rotation,int mode){reset();live.clear();changes.clear();where="guild_vs1";room="PN Weekly Expedition";players[0]->battle_status.hp=1000;clock_now=345600+(3000+rotation)*604800LL+60;choices={mode+1,1};finish("console");};
 auto drain=[&](){unsigned limit=0;while(!events.empty()){boundary(++limit<20,"bounded event queue");auto e=events.front();events.erase(events.begin());finish(e.substr(e.find("::")+2));}};
 auto kill=[&](int gid,unsigned who=0){live.erase(gid);players[who]->killedgid=gid;finish("OnMobDead",who);};
 for(int rotation=0;rotation<3;++rotation){
  run(rotation,0);drain();check(stage("'PXStage")==1&&stage("'PXWave")==1,"start enters the first authentic wave");
  check(stage("'PXRemaining")== (rotation==0?6:4),"weekly modifier chooses the correct wave size");
  auto remaining=stage("'PXRemaining");kill(99999);check(stage("'PXRemaining")==remaining,"unrelated death cannot reduce the wave");
  auto first=*live.begin();players[1]->killedgid=first;finish("OnMobDead",1);check(stage("'PXRemaining")==remaining,"another character cannot forge owner clear callbacks");
  clock_now+=10;kill(first);remaining=stage("'PXRemaining");players[0]->killedgid=first;finish("OnMobDead");check(stage("'PXRemaining")==remaining,"duplicate GID death callback is ignored");
  while(stage("'PXStage")==1){std::vector<int> wave(live.begin(),live.end());for(int gid:wave)kill(gid);clock_now+=10;drain();}
  check(stage("'PXStage")==2&&stage("'PXWave")==5&&live.empty(),"exactly five complete waves clear the expedition");
  if(rotation==1){int changed=0;for(auto [gid,field,value]:changes)if(field==UMOB_ELETYPE)++changed;check(changed==20,"element-shift applies to every enemy across all waves");}
  exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(result()==1&&registries[players[0]->id]["PNExpeditionStamps"]==1,"actual eligible clear awards one cosmetic stamp");
  exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(!result()&&registries[players[0]->id]["PNExpeditionStamps"]==1,"same completed run cannot be recorded twice");
  stage("'PXRecorded",0);exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(result()==1&&registries[players[0]->id]["PNExpeditionStamps"]==1,"another clear in the same difficulty/week does not grant another stamp");
  stage("'PXRecorded",0);where="prontera";exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(!result(),"outside the owning room cannot claim");where="guild_vs1";
  room="PN Damage Lab";exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(!result(),"damage lab cannot claim an expedition record");room="PN Weekly Expedition";
  clock_now+=604800;exec("Result=callfunc(\"PN_ExpeditionRecord\");");check(!result(),"an old completed instance cannot grant a new-week stamp");
 }
 run(2,1);drain();stage("'PXTick",14);x=51;finish("OnTimer1000");check(stage("'PXWarning")==4,"hazard gives an explicit four-second warning");
 for(int i=0;i<4;++i)finish("OnTimer1000");check(stage("'PXMistakes")==1,"remaining in announced unsafe half records one mistake");
 stage("'PXTick",29);x=51;finish("OnTimer1000");for(int i=0;i<4;++i)finish("OnTimer1000");check(stage("'PXMistakes")==1,"moving into the safe half avoids a mistake");
 stage("'PXMistakes",2);stage("'PXWarning",1);stage("'PXSide",1);finish("OnTimer1000");check(stage("'PXStage")==3&&live.empty(),"third hazard mistake fails and cleans up the attempt");
 run(0,0);drain();players[0]->battle_status.hp=0;finish("OnTimer1000");check(stage("'PXStage")==3,"death fails a running expedition");
 run(0,0);drain();where="prontera";finish("OnTimer1000");check(stage("'PXStage")==3,"leaving fails a running expedition");
 run(0,0);drain();clock_now+=600;finish("OnTimer1000");check(stage("'PXStage")==3,"ten-minute timeout fails without a clear");
 reset();room="PN Weekly Expedition";where="prontera";town=false;choices={1};exec("callfunc \"PN_WeeklyExpedition\";");check(moves.empty(),"entry from a combat map cannot bypass town restriction");
 check(items.empty()&&experience.empty()&&reputation.empty(),"pilot grants no items, experience or progression currency");check(!errors,"all actual source functions and event branches execute without VM errors");
 for(auto& entry:codes)script_free_code(entry.second);codes.clear();reset();for(auto& p:players)if(p->regs.arrays)p->regs.arrays->destroy(p->regs.arrays,script_free_array_db);players.clear();attached=nullptr;
 script_free_vars(instances[1]->regs.vars);instances.clear();do_final_script();timer_final();db_final();malloc_final();
 std::printf("WEEKLY_EXPEDITION_OK cases=%u checks=%u failures=%u errors=%u\n",cases,checks,failures,errors);return failures||errors?1:0;
}
