// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  player_settings_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/player_settings_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Appended to crown/Rune fixture helpers. Registry/VM are production code;
// atcommand execution and UI/message transport are recorded boundaries.
#include "map/mob.hpp"
namespace {std::vector<std::string> dispatched;bool number_input=false;}
extern "C" bool settings_command(int32,map_session_data*,const char*,int32) asm("__wrap__Z12is_atcommandiP16map_session_dataPKci");
extern "C" bool settings_command(int32,map_session_data* sd,const char* cmd,int32 type){check(sd==attached&&type==0,"attached normal atcommand dispatch");dispatched.emplace_back(cmd);return true;}
extern "C" void settings_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void settings_input(map_session_data&,uint32){number_input=true;}
extern "C" void settings_message(const block_list*,unsigned long,const char*,bool,send_target,const map_session_data*) asm("__wrap__Z24clif_messagecolor_targetPK10block_listmPKcb11send_targetPK16map_session_data");
extern "C" void settings_message(const block_list*,unsigned long,const char* msg,bool,send_target,const map_session_data*){messages.emplace_back(msg);}
namespace {
void settings_walk(script_code* code,const std::vector<int>& choices={},const std::vector<int>& inputs={}){
    run_script(code,0,attached->id,NPC);size_t choice=0,input=0;int pauses=0;
    while(attached->st){check(++pauses<80,"bounded settings dialogue");auto* st=attached->st;
        if(st->state==RERUNLINE){
            if(number_input){check(input<inputs.size(),"numeric answer supplied");attached->npc_amount=inputs[input++];number_input=false;}
            else{check(choice<choices.size(),"menu answer supplied");attached->npc_menu=choices[choice++];}
        }else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"known Next suspension");
        run_script_main(st);
    }
    check(!errors&&choice==choices.size()&&input==inputs.size(),"actual VM consumed all intended answers without error");
}
std::string label(const std::string& source,const std::string& start,const std::string& stop){
    auto first=source.find(start);check(first!=std::string::npos,"exact start label");first+=start.size();
    auto last=source.find(stop,first);check(last!=std::string::npos,"exact end label");return source.substr(first,last-first);
}
void kc(const std::string& command,const std::vector<std::string>& args){
    std::string prefix="{ .@atcmd_numparameters="+std::to_string(args.size())+";";
    if(!args.empty()){prefix+="setarray .@atcmd_parameters$[0]";for(auto& arg:args)prefix+=",\""+arg+"\"";prefix+=";";}
    auto* code=compile(prefix+command+"}","actual killcounter command");settings_walk(code);script_free_code(code);
}
bool said(const std::string& part){return std::any_of(messages.begin(),messages.end(),[&](const auto& s){return s.find(part)!=std::string::npos;});}
}
extern "C" int __wrap_main(int argc,char**){
    check(argc==2,"artifact directory supplied");deny_network();static char server[]="player-settings-native-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();battle_config.atcommand_disable_npc=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"settings:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"settings:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    load_functions("npc/custom/player_commands.txt");
    auto source=read("npc/custom/player_commands.txt");auto settings=body(source,"\tPN Player Settings\t"),counter=body(source,"\tPN Kill Counter\t");
    auto* menu=compile("{"+label(settings,"OnSettings:","OnPCLoginEvent:")+"}","actual settings menu");
    auto* login=compile("{"+label(settings,"OnPCLoginEvent:","OnInit:")+"}","actual login handler");
    check(counter.find("OnNPCKillEvent:")==std::string::npos,"native credited-kill helper owns increments without double script counting");
    auto command=label(counter,"OnCommand:","OnInit:");
    auto poring=std::make_shared<s_mob_db>();poring->id=1002;poring->jname="Poring";mob_db.put(1002,poring);
    auto lunatic=std::make_shared<s_mob_db>();lunatic->id=1063;lunatic->jname="Lunatic";mob_db.put(1063,lunatic);
    {auto sd=rune_player();dispatched.clear();settings_walk(login);check(dispatched.empty(),"unset login preferences preserve session defaults");}
    {auto sd=rune_player();reg("#PNSetting",0,38);reg("#PNSetting",1,2);reg("PNSetting",1,1);reg("PNSetting",4,2);
        dispatched.clear();settings_walk(login);auto first=dispatched;
        check(first==std::vector<std::string>({"@autoloot 37","@showexp off","@noask on"}),"char explicit-off overrides account-on; unrelated unset not dispatched");
        settings_walk(login);check(std::vector<std::string>(dispatched.begin()+first.size(),dispatched.end())==first,"repeat login uses identical explicit values, no toggle");}
    {auto sd=rune_player();dispatched.clear();settings_walk(menu,{2,1,1,2,6});check(reg("PNSetting",1)==2&&reg("#PNSetting",1)==0&&dispatched==std::vector<std::string>{"@showexp on"},"character save persists explicit on");}
    {auto sd=rune_player();reg("PNSetting",1,1);reg("#PNSetting",1,2);dispatched.clear();settings_walk(menu,{2,1,2,6});check(!reg("PNSetting",1)&&dispatched==std::vector<std::string>{"@showexp on"},"clear character override applies inherited account setting");}
    {auto sd=rune_player();reg("PNSetting",1,1);dispatched.clear();settings_walk(menu,{2,2,1,2,6});check(reg("#PNSetting",1)==2&&reg("PNSetting",1)==1&&dispatched==std::vector<std::string>{"@showexp off"}&&said("still has an override"),"account save preserves character priority");}
    {auto sd=rune_player();reg("PNSetting",1,2);dispatched.clear();settings_walk(menu,{2,1,2,6});check(!reg("PNSetting",1)&&dispatched==std::vector<std::string>{"@showexp off"},"clearing last override restores explicit off");}
    {auto sd=rune_player();dispatched.clear();settings_walk(menu,{2,3,6});check(!reg("PNSetting",1)&&!reg("#PNSetting",1)&&dispatched.empty(),"scope cancel makes no change");}
    {auto sd=rune_player();dispatched.clear();settings_walk(menu,{2,1,3,6});check(!reg("PNSetting",1)&&dispatched.empty(),"action cancel makes no change");}
    for(int index=1;index<5;++index)for(int value=0;value<2;++value){auto sd=rune_player();dispatched.clear();settings_walk(menu,{index+1,1,1,value+1,6});
        const char* commands[]={"autoloot","showexp","showzeny","showdelay","noask"};
        check(reg("PNSetting",index)==value+1&&dispatched==std::vector<std::string>{std::string("@")+commands[index]+(value?" on":" off")},"each boolean menu maps to correct explicit command and encoded value");}
    for(int input:{-1,0,37,100,101}){auto sd=rune_player();dispatched.clear();settings_walk(menu,{1,2,1,6},{input});int expected=std::clamp(input,0,100);check(reg("#PNSetting",0)==expected+1&&dispatched==std::vector<std::string>{"@autoloot "+std::to_string(expected)},"native bounded input clamps to0..100 before encoded save");}
    // Account/character transport is modeled by loading saved rows into another
    // real registry. This is not a claim to exercise a char-server SQL roundtrip.
    {auto alt=rune_player();alt->status.char_id=99000004;pc_set_reg_load(true);reg("#PNSetting",1,2);pc_set_reg_load(false);dispatched.clear();settings_walk(login);check(dispatched==std::vector<std::string>{"@showexp on"},"fresh alt inherits persisted account row");}
    {auto sd=rune_player();kc(command,{"1002","1"});kc(command,{"1002","5"});kc(command,{"1063","3"});
        check(reg("PNKCMob",0)==1002&&reg("PNKCMob",4)==1002&&reg("PNKCMob",2)==1063,"exact five-slot registration");
        reg("PNKCKills",0,2147483647);reg("PNKCKills",4,2);
        kc(command,{"status"});check(said("Poring (1002): 2147483647")&&said("[2] Empty"),"status includes exact native monster name and count");
        messages.clear();kc(command,{});check(said("Poring (1002): 2147483647"),"bare command opens status");
        kc(command,{"reset","1"});check(!reg("PNKCMob",0)&&!reg("PNKCKills",0)&&reg("PNKCMob",4)==1002,"single-slot reset isolates other slots");
        kc(command,{"reset"});for(int i=0;i<5;++i)check(!reg("PNKCMob",i)&&!reg("PNKCKills",i),"all reset removes both persistent arrays");}
    for(const auto& args:std::vector<std::vector<std::string>>{{"1002","0"},{"1002","6"},{"1002","01"},{"1002"},{"1002","1","extra"},{"-1","1"},{"1002x","1"},{"0","1"},{"65536","1"},{"999999999","1"},{"9999999999","1"},{"65000","1"}}){
        auto sd=rune_player();reg("PNKCMob",0,1002);reg("PNKCKills",0,7);kc(command,args);check(reg("PNKCMob",0)==1002&&reg("PNKCKills",0)==7,"malformed or unknown monster/slot cannot overwrite saved count");}
    {auto sd=rune_player();reg("PNKCMob",0,1002);reg("PNKCKills",0,7);kc(command,{"1063","1"});check(reg("PNKCMob",0)==1063&&!reg("PNKCKills",0),"registration intentionally resets previous count");}
    script_free_code(menu);script_free_code(login);attached=nullptr;mob_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;
    timer_final();db_final();malloc_final();std::printf("PLAYER_SETTINGS_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
