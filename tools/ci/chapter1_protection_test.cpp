// Real production NPC body in the native VM; world/timer/packet effects are doubles.
#include "map/map.hpp"
#include "common/mapindex.hpp"
namespace {
map_data world_map;
int queued=0,warps=0,notices=0,last_x=0,last_y=0;
const char* event_name="CH1_DimensionalGuard::OnProtectionCheck";
}
extern "C" map_data* guard_map(int16) asm("__wrap__Z14map_getmapdatas");
extern "C" map_data* guard_map(int16){return &world_map;}
extern "C" bool guard_add(map_session_data*,int32,const char*) asm("__wrap__Z16pc_addeventtimerP16map_session_dataiPKc");
extern "C" bool guard_add(map_session_data* sd,int32 tick,const char* name){check(sd==attached&&tick==1000&&!strcmp(name,event_name),"one-second named recheck");++queued;return true;}
extern "C" bool guard_del(map_session_data*,const char*) asm("__wrap__Z16pc_deleventtimerP16map_session_dataPKc");
extern "C" bool guard_del(map_session_data*,const char* name){check(!strcmp(name,event_name),"cancel own timer");queued=0;return true;}
extern "C" uint16 guard_index(const char*,const char*) asm("__wrap__Z17mapindex_name2idxPKcS0_");
extern "C" uint16 guard_index(const char* name,const char*){check(!strcmp(name,"hem_fild"),"safe exit map");return 200;}
extern "C" e_setpos guard_warp(map_session_data*,uint16,int32,int32,clr_type) asm("__wrap__Z9pc_setposP16map_session_datatii8clr_type");
extern "C" e_setpos guard_warp(map_session_data* sd,uint16 index,int32 x,int32 y,clr_type){check(sd==attached&&index==200,"warp attached player only");++warps;last_x=x;last_y=y;return SETPOS_OK;}
extern "C" map_session_data* guard_nick(const char*,bool) asm("__wrap__Z11map_nick2sdPKcb");
extern "C" map_session_data* guard_nick(const char*,bool){return attached;}
extern "C" void guard_message(int32,const char*) asm("__wrap__Z19clif_displaymessageiPKc");
extern "C" void guard_message(int32,const char*){++notices;}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==2,"fixture path");deny_network();static char name[]="chapter1-protection-test";SERVER_NAME=name;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;strcpy(sd->status.name,"GuardTest");
 std::ifstream f(argv[1]);check(f.good(),"read exact extracted NPC body");std::string body{std::istreambuf_iterator<char>(f),{}};
 auto* code=parse_script(body.c_str(),"CH1_DimensionalGuard",1,0);check(code,"parse exact NPC body");
 unsigned cases=0;
 for(const char* mapname:{"hem_dun02","ch1_gfn01","ch1_gfn03","prontera","hem_fild"})for(int mask=0;mask<4;++mask){
  strcpy(world_map.name,mapname);sd->sc.deleteSCE(SC_CONTENTS_37);sd->sc.deleteSCE(SC_CONTENTS_38);
  if(mask&1)sd->sc.createSCE(SC_CONTENTS_37);if(mask&2)sd->sc.createSCE(SC_CONTENTS_38);
  queued=3;warps=notices=0;run_script(code,0,sd->id,0);
  bool hem=!strcmp(mapname,"hem_dun02"),gfn=!strcmp(mapname,"ch1_gfn01")||!strcmp(mapname,"ch1_gfn03");
  bool expired=(hem&&!(mask&1))||(gfn&&!(mask&2));
  check(warps==int(expired)&&notices==int(expired),"matching protection required, outside maps ignored");
  check(queued==int((hem||gfn)&&!expired),"one timer maximum after repeated map loads");
  if(expired)check(last_x==(hem?329:231)&&last_y==(hem?188:296),"correct entrance return");
  if((hem||gfn)&&!expired){sd->sc.deleteSCE(hem?SC_CONTENTS_37:SC_CONTENTS_38);run_script(code,0,sd->id,0);check(warps==1&&queued==0,"expiration expels and terminates timer");}
  check(!sd->st&&errors==0,"native VM completes cleanly");++cases;
 }
 script_free_code(code);attached=nullptr;sd.reset();do_final_script();timer_final();db_final();malloc_final();
 printf("CH1_PROTECTION_OK cases=%u assertions=%d\n",cases,assertions);return errors?1:0;
}
