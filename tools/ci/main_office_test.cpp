// Appended to the Rune harness helpers; production NPC bodies are unchanged.
#include "map/skill.hpp"
#include "map/status.hpp"
namespace { unsigned office_warps=0,skill_packets=0; }
extern "C" int16 office_map(const char*) asm("__wrap__Z17map_mapname2mapidPKc");
extern "C" int16 office_map(const char*){return 0;}
extern "C" uint16 office_index(const char*,const char*) asm("__wrap__Z17mapindex_name2idxPKcS0_");
extern "C" uint16 office_index(const char* name,const char*){check(std::string(name)=="pn_office","warp destination is Main Office");return 77;}
extern "C" e_setpos office_warp(map_session_data*,uint16,int32,int32,clr_type) asm("__wrap__Z9pc_setposP16map_session_datatii8clr_type");
extern "C" e_setpos office_warp(map_session_data* sd,uint16 map,int32 x,int32 y,clr_type){check(sd==attached&&map==77&&x==100&&y==40,"exact lobby warp");++office_warps;return SETPOS_OK;}
extern "C" void office_skill(const map_session_data&,uint16) asm("__wrap__Z13clif_addskillRK16map_session_datat");
extern "C" void office_skill(const map_session_data&,uint16){++skill_packets;}
extern "C" void office_delete(const map_session_data&,uint16,bool) asm("__wrap__Z16clif_deleteskillRK16map_session_datatb");
extern "C" void office_delete(const map_session_data&,uint16,bool){++skill_packets;}
extern "C" void office_message(const block_list*,unsigned long,const char*,bool,send_target,const map_session_data*) asm("__wrap__Z24clif_messagecolor_targetPK10block_listmPKcb11send_targetPK16map_session_data");
extern "C" void office_message(const block_list*,unsigned long,const char* message,bool,send_target,const map_session_data*){messages.emplace_back(message);}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2,"artifact directory supplied");deny_network();static char server[]="main-office-native-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();battle_config.atcommand_disable_npc=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"office:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"office:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto items=read(std::string(argv[1])+"/office-items.yml");auto itemtree=ryml::parse_in_arena(ryml::to_csubstr(items));
    for(auto row:itemtree["Body"])check(item_db.parseBodyNode(row)==1,"actual skill reagent metadata parses");
    auto data=read(std::string(argv[1])+"/skills.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto row:tree["Body"])check(skill_db.parseBodyNode(row)==1,"actual copy skill metadata parses");
    auto source=read("npc/custom/main_office/services.txt");
    auto* teacher=compile(body(source,"\tPN Copy Teacher\t"),"actual Office Copy Teacher");
    auto access=body(source,"\tPN Office Access\t");
    auto start=access.find("OnOffice:")+9;auto finish=access.find("OnInit:",start);
    check(start!=std::string::npos&&finish!=std::string::npos,"exact Office command labels found");
    auto* travel=compile("{"+access.substr(start,finish-start)+"}","actual Office command label");
    const int target=skill_get_index(AL_HEAL),plag=skill_get_index(RG_PLAGIARISM),repro=skill_get_index(SC_REPRODUCE);
    check(target&&plag&&repro,"native skill indices initialized");
    for(int mode=0;mode<12;++mode){
        auto sd=rune_player();sd->status.class_=JOB_SHADOW_CHASER;sd->class_=MAPID_SHADOW_CHASER;
        auto learn=[&](int index,int id,int level){sd->status.skill[index].id=id;sd->status.skill[index].lv=level;sd->status.skill[index].flag=SKILL_FLAG_PERMANENT;};
        learn(plag,RG_PLAGIARISM,mode==3?0:1);learn(repro,SC_REPRODUCE,10);
        int expected=1;
        if(mode==0){learn(plag,RG_PLAGIARISM,5);expected=5;}
        if(mode>=1&&mode<=6){auto* sc=sd->sc.createSCE(SC__REPRODUCE);sc->val1=mode==1?3:10;expected=sc->val1;}
        if(mode==4){learn(repro,SC_REPRODUCE,5);expected=10;}
        if(mode==5)sd->sc.createSCE(SC_PRESERVE);
        if(mode==6)expected=2;
        if(mode==7){sd->sc.createSCE(SC_PRESERVE);expected=0;}
        if(mode==8){learn(plag,RG_PLAGIARISM,0);expected=0;}
        if(mode==9){learn(target,AL_HEAL,4);expected=4;}
        if(mode==10)expected=0;
        if(mode==11)expected=0;
        skill_packets=0;
        walk(teacher,{1,mode==10?2:1},[&](int,int chosen){if(chosen==1){
            if(mode==6)sd->sc.getSCE(SC__REPRODUCE)->val1=2;
            if(mode==11){learn(plag,RG_PLAGIARISM,0);learn(repro,SC_REPRODUCE,0);}
        }});
        check(sd->status.skill[target].lv==expected,"copy level follows selected native copy mode and final cast level");
        if(mode==7||mode==8||mode==9||mode==10||mode==11)check(skill_packets==0,"Preserve/eligibility/cancel/stale denial never modifies copied skills");
        if(mode>=1&&mode<=6)check(sd->reproduceskill_idx==target&&!sd->cloneskill_idx,"active Reproduce selects its own slot independently of Plagiarism level");
        if(mode==0)check(sd->cloneskill_idx==target,"inactive Reproduce falls back to learned Plagiarism slot");
    }
    // Every configured blocked map flag is exercised through the real getmapflag
    // builtin/native map flags. Warp transport itself is a recorded boundary.
    for(int flag:std::vector<int>{-1,MF_NOTELEPORT,MF_NOWARP,MF_NORETURN,MF_NOGO,MF_PVP,MF_GVG,MF_GVG_DUNGEON,MF_GVG_CASTLE,MF_GVG_TE,MF_GVG_TE_CASTLE,MF_BATTLEGROUND}){
        auto sd=rune_player();sd->battle_status.hp=100;sd->m=0;std::strcpy(::map[0].name,"prontera");::map[0].initMapFlags();
        if(flag>=0)::map[0].setMapFlag(flag,true);
        office_warps=0;walk(travel,{});check(office_warps==(flag<0?1:0),"travel respects each native map restriction");
    }
    for(int state=0;state<3;++state){auto sd=rune_player();sd->m=0;sd->battle_status.hp=state==0?0:100;::map[0].initMapFlags();
        std::strcpy(::map[0].name,state==1?"1234#000001":"prontera");sd->instance_id=state==2?37:0;
        office_warps=0;walk(travel,{});
        check(office_warps==(state==2?1:0),"death/current instance blocked; an owned instance outside map does not block travel");
    }
    script_free_code(teacher);script_free_code(travel);attached=nullptr;skill_db.clear();item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;
    timer_final();db_final();malloc_final();std::printf("OFFICE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
