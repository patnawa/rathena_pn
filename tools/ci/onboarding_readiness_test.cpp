#include "map/party.hpp"
#include "map/quest.hpp"
#include "map/mob.hpp"
#include "map/instance.hpp"
#include "common/ers.hpp"
#include <ctime>
#include <regex>
#include <nlohmann/json.hpp>
static party_data guide_party{};
static std::string destination;
// Only the simulation deadline cases replace the wall clock. Quest parsing,
// quest_add/quest_time, expiry checks and the readiness VM remain production code.
static time_t guide_clock;
extern "C" time_t __real_time(time_t*);
extern "C" time_t __wrap_time(time_t* out){if(!guide_clock)return __real_time(out);if(out)*out=guide_clock;return guide_clock;}
extern "C" void guide_quest_add(const map_session_data*,const struct quest*) asm("__wrap__Z14clif_quest_addPK16map_session_dataPK5quest");
extern "C" void guide_quest_add(const map_session_data*,const struct quest*){}
extern "C" void guide_quest_objectives(const map_session_data*,const struct quest*) asm("__wrap__Z27clif_quest_update_objectivePK16map_session_dataPK5quest");
extern "C" void guide_quest_objectives(const map_session_data*,const struct quest*){}
extern "C" party_data* guide_party_search(int32 id) asm("__wrap__Z12party_searchi");
extern "C" party_data* guide_party_search(int32 id){return id==7?&guide_party:nullptr;}
extern "C" void guide_nav(const map_session_data*,const char*,uint16,uint16,uint8,bool,uint16) asm("__wrap__Z15clif_navigateToPK16map_session_dataPKctthbt");
extern "C" void guide_nav(const map_session_data*,const char* map,uint16 x,uint16 y,uint8,bool,uint16){destination=std::string(map)+":"+std::to_string(x)+","+std::to_string(y);}
static void functions(const std::string& source){
    std::regex declaration("function[\\t ]+script[\\t ]+([A-Za-z0-9_]+)[\\t ]+\\{");
    for(std::sregex_iterator it(source.begin(),source.end(),declaration),end;it!=end;++it){
        const std::string name=(*it)[1];strdb_put(script_get_userfunc_db(),name.c_str(),compile(body(source,(*it).str()),name.c_str()));
    }
}
static void invoke_guide(const std::string& command){auto* c=compile("{"+command+" end;}","guide entry");walk(c,{});script_free_code(c);}
static void seedquest(int id,int state=1,bool expired=false){
    auto* sd=attached;RECREATE(sd->quest_log,struct quest,sd->num_quests+1);auto& q=sd->quest_log[sd->num_quests++];q={};q.quest_id=id;q.state=state==2?Q_COMPLETE:Q_ACTIVE;q.time=time(nullptr)+(expired?-60:3600);
    sd->avail_quests=sd->num_quests;
}
static std::unique_ptr<map_session_data> fresh(){
    ++cases;nums.clear();strings.clear();messages.clear();destination.clear();guide_party={};guide_players.clear();guide_member_nums.clear();
    auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=200;sd->status.inventory_slots=MAX_INVENTORY;sd->status.zeny=7654321;sd->max_weight=10000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->vars_ok=true;
    sd->status.party_id=7;guide_party.data[0].sd=sd.get();guide_party.party.member[0].leader=1;
    guide_party.party.member[0].account_id=sd->status.account_id;guide_party.party.member[0].char_id=sd->status.char_id;
    std::strcpy(guide_party.party.member[0].name,"Leader");
    return sd;
}
static void clean(){if(attached->quest_log)aFree(attached->quest_log);attached->quest_log=nullptr;if(attached->regs.arrays)attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db);attached->regs.arrays=nullptr;}
static std::string reason(const std::string& name){messages.clear();invoke_guide("mes callfunc(\"PN_InstanceMissing\",\""+name+"\");");check(messages.size()==1,"one readiness result");return messages[0];}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"fixture arguments");deny_network();static char server[]="onboarding-readiness-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    map_num=1;std::strcpy(map[0].name,"prontera");map[0].initMapFlags();
    num_reg_ers=ers_new(sizeof(script_reg_num),"guide:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"guide:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    auto items=read(std::string(argv[1])+"/armor-items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(items));for(auto row:tree["Body"])check(item_db.parseBodyNode(row)==1,"actual Apple metadata");
    // Seed only the world identity boundary. The objective and hunt check use
    // the actual imported quest row and production QuestDatabase/parser.
    for(const auto& row:nlohmann::json::parse(read(std::string(argv[1])+"/guide-mobs.json"))){auto mob=std::make_shared<s_mob_db>();mob->id=row["Id"];mob->sprite=row["AegisName"];mob_db.put(mob->id,mob);}
    auto quest_source=read(std::string(argv[1])+"/guide-quest.yml");auto quest_tree=ryml::parse_in_arena(ryml::to_csubstr(quest_source));for(auto row:quest_tree["Body"])check(quest_db.parseBodyNode(row)==1,"actual hunt objective metadata");
    functions(read(argv[2]));strdb_put(script_get_userfunc_db(),"CH2_Complete",compile(body(read("npc/custom/chapter2/Chapter2.txt"),"function\tscript\tCH2_Complete"),"actual chapter completion"));
    functions(read("npc/custom/main_office/chapter1_guide.txt"));
    // The unprefixed "3d 4h" is an exact server-local clock schedule, not +76h.
    auto simulation=quest_search(12661);
    check(simulation&&simulation->time_at&&simulation->time==273600&&simulation->time_week==-1,"actual 12661 parser uses a 3-day offset at 04:00");
    setenv("TZ","UTC0",1);tzset();
    std::tm day{};day.tm_year=126;day.tm_mon=9;day.tm_mday=3;const time_t midnight=mktime(&day);
    const int starts[]={0,7200,14399,14400,14401,43200,86399};
    const int waits[]={273600,266400,259201,345600,345599,316800,273601};
    save_settings=0;
    for(size_t i=0;i<sizeof(starts)/sizeof(*starts);++i){
        auto sd=fresh();guide_clock=midnight+starts[i];seedquest(12660,2);
        check(quest_add(sd.get(),12661)==0,"actual simulation quest admission");
        const time_t expiry=sd->quest_log[1].time;
        check(expiry-guide_clock==waits[i],"simulation deadline follows 04:00 carry rather than a fixed 76-hour duration");
        const auto* end=localtime(&expiry);check(end->tm_hour==4&&end->tm_min==0&&end->tm_sec==0,"simulation expires at server-local 04:00");
        auto r=reason("Simulated Dark Whisper");
        check(r.find(std::to_string((waits[i]+59)/60)+" minutes remain")!=std::string::npos,"readiness uses actual deadline remaining minutes");
        check(r.find("3 days after the next 04:00 following entry")!=std::string::npos&&r.find("76-hour")==std::string::npos,"readiness describes the exact-clock schedule");
        guide_clock=expiry;check(quest_check(sd.get(),12661,PLAYTIME)==0,"expiry comparison retains the exact boundary second");
        guide_clock=expiry+1;check(quest_check(sd.get(),12661,PLAYTIME)==2,"timer expires after the deadline");
        check(reason("Simulated Dark Whisper").find("Timer expired")!=std::string::npos,"expired simulation directs the player to clear the timer");
        clean();guide_clock=0;
    }
    const int quests[]={0,0,18368,18369,18370,18371,24069,24070,24071,24072};
    const char* expected[]={"pn_office:46,39","prt_fild05:353,252","ygg_edge:253,246","ygg_fruit:80,122","ygg_fruit:82,120","ygg_roots:334,138","ygg_roots:299,59","ygg_roots:186,117","ygg_roots:167,135","ygg_roots:166,135"};
    for(int i=0;i<10;++i){auto sd=fresh();if(!i)sd->status.base_level=199;if(i>=2){seedquest(18368,i==2?1:2);if(i>2)seedquest(quests[i]);}Snapshot before;auto n=nums;auto count=sd->num_quests;
        invoke_guide("callfunc \"PN_GuideOnboarding\";");check(destination==expected[i],"exact native quest-state destination");check(nums==n&&sd->num_quests==count,"guidance never changes progression registry or quests");before.unchanged();clean();}
    // Every curated anchor is independently checked against the live NPC source
    // by the generator. Exercise the actual VM for each active story state too.
    auto catalog=nlohmann::json::parse(read("npc/custom/main_office/chapter1_guide.json"));
    for(const auto& row:catalog["steps"]){
        auto sd=fresh();int q=row["quest"];if(q!=18368)seedquest(18368,2);seedquest(q);
        std::string expected=row["map"].get<std::string>()+":"+std::to_string(row["x"].get<int>())+","+std::to_string(row["y"].get<int>());
        if(q==17891)expected="hem_fild:102,161";if(q==18376)expected="mu_fild01:95,154";if(q==19225)expected="ygg_edge:211,211";
        Snapshot before;auto n=nums;auto count=sd->num_quests;
        const bool marker=(q>=17913&&q<=17915)||(q>=19231&&q<=19237);
        if(marker)invoke_guide("callfunc \"PN_CH1GuideCatalog\","+std::to_string(q)+";");
        else{if(q==19230)expected="ch1_vrgef1:75,140";invoke_guide("callfunc \"PN_GuideOnboarding\";");}
        check(destination==expected,"all story states resolve to their validated handoff or conditional gate");
        check(nums==n&&sd->num_quests==count,"complete guide has no progression writes");before.unchanged();clean();
    }
    for(int collected=0;collected<=7;++collected){auto sd=fresh();seedquest(18368,2);seedquest(19225);seedquest(19230);
        for(int q=19231;q<19231+collected;++q)seedquest(q,2);
        const char* targets[]={"ch1_vrgef1:75,140","ch1_vrgef1:62,140","ch1_vrgef1:62,145","ch1_vrgef1:76,144","ch1_vrgef1:77,137","ch1_vrgef1:63,137","ch1_vrgef1:67,145","ch1_vrgef1:66,137"};
        invoke_guide("callfunc \"PN_GuideOnboarding\";");check(destination==targets[collected],"seven-object investigation selects remaining clue before umbrella quest");clean();}
    for(int collected=0;collected<3;++collected){auto sd=fresh();seedquest(18368,2);seedquest(17912);for(int q=17913;q<17913+collected;++q)seedquest(q,2);
        invoke_guide("callfunc \"PN_GuideOnboarding\";");check(destination==std::string("ch1_dw:340,")+(collected==0?"85":collected==1?"82":"78"),"corpse sampling selects unsearched duplicate, not the base NPC every time");clean();}
    for(int collected=0;collected<3;++collected){auto sd=fresh();seedquest(18368,2);seedquest(8959);for(int q=8960;q<8960+collected;++q)seedquest(q,2);
        const char* targets[]={"ch1_sf02:114,46","ch1_sf02:111,271","ch1_sf02:45,54"};invoke_guide("callfunc \"PN_GuideOnboarding\";");check(destination==targets[collected],"Brimir exploration selects remaining resident");clean();}
    {auto sd=fresh();seedquest(18368,2);seedquest(18377,2);invoke_guide("callfunc \"PN_GuideOnboarding\";");check(destination=="ygg_edge:156,179","completed Chapter 1 points to actual Chapter 2 start");clean();}
    for(const auto& name:std::vector<std::string>{"Crossroads of Tangled Mana","Nyrholt","Phantom of Nyrholt","Ghost Palace","Charleston in Distress"}){
        for(int variant=0;variant<4;++variant){auto sd=fresh();setnum("CH2_Step",name=="Crossroads of Tangled Mana"?2:name=="Nyrholt"?9:11);if(name=="Charleston in Distress")seedquest(13184);
            if(variant==1)sd->status.party_id=0;if(variant==2)guide_party.party.member[0].leader=0;if(variant==3)guide_party.instance_id=42;
            Snapshot before;auto n=nums;auto r=reason(name);check(variant? !r.empty():r.empty(),"ready/no party/nonleader/other reservation boundaries");check(nums==n,"readiness preserves progression registry");before.unchanged();clean();}
    }
    for(const auto& cfg:std::vector<std::pair<std::string,int>>{{"Ghost Palace",120},{"Charleston in Distress",130}}){
        for(int delta:{-1,0,1}){auto sd=fresh();sd->status.base_level=cfg.second+delta;if(cfg.second==130)seedquest(13184);check(reason(cfg.first).empty()==(delta>=0),"exact entrance level boundary");clean();}
        for(int state:{1,2})for(bool expired:{false,true}){auto sd=fresh();if(cfg.second==130)seedquest(13184);seedquest(cfg.second==120?1261:13185,state,expired);auto r=reason(cfg.first);check(r.find(expired?"Timer expired":"Cooldown")!=std::string::npos,"native active/completed/expired quest timer semantics");clean();}
    }
    for(int quantity:{4,5}){auto sd=fresh();setnum("CH2_Step",8);put(0,512,quantity);check(reason("Nyrholt").empty()==(quantity==5),"Apple requirement boundary without consumption");check(sd->inventory.u.items_inventory[0].amount==quantity,"readiness does not consume Apples");clean();}
    for(int offset:{-60,60}){auto sd=fresh();seedquest(27101,2);seedquest(27119,2);setnum("CH2_Daily_CD_27119",time(nullptr)+offset);check(reason("Phantom of Nyrholt").empty()==(offset<0),"Phantom claimed reward cooldown boundary");clean();}
    for(const auto& name:std::vector<std::string>{"Crossroads of Tangled Mana","Nyrholt","Phantom of Nyrholt","Ghost Palace","Charleston in Distress"}){
        auto sd=fresh();setnum("CH2_Step",name=="Crossroads of Tangled Mana"?2:name=="Nyrholt"?9:11);if(name=="Charleston in Distress")seedquest(13184);
        auto definition=std::make_shared<s_instance_db>();definition->id=1;definition->name=name;instance_db.put(1,definition);
        auto reservation=std::make_shared<s_instance_data>();reservation->id=1;reservation->state=INSTANCE_BUSY;reservation->mode=IM_PARTY;reservation->owner_id=7;instances[42]=reservation;guide_party.instance_id=42;
        reservation->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
        const bool chapter=name!="Ghost Palace"&&name!="Charleston in Distress";
        if(chapter){invoke_guide("setinstancevar 'ch2_roster$,\",99000002,\",42;");guide_party.party.member[0].leader=0;}
        check(reason(name).empty(),"matching reservation admits original members, including nonleader for Chapter 2");
        if(chapter){invoke_guide("setinstancevar 'ch2_roster$,\",999,\",42;");check(reason(name).find("original roster")!=std::string::npos,"late joining character cannot reuse original roster");}
        if(reservation->regs.arrays)reservation->regs.arrays->destroy(reservation->regs.arrays,script_free_array_db);
        if(reservation->regs.vars)db_destroy(reservation->regs.vars);
        reservation->regs={};instances.clear();instance_db.clear();clean();
    }
    for(const auto& name:std::vector<std::string>{"Ominous Dark Whisper","Simulated Dark Whisper"}){
        for(int variant=0;variant<6;++variant){auto sd=fresh();if(variant!=1)seedquest(name=="Ominous Dark Whisper"?12666:12660,name=="Ominous Dark Whisper"?1:2);
            if(variant==2)sd->status.party_id=0;if(variant==3)guide_party.party.member[0].leader=0;if(variant==4)guide_party.instance_id=42;
            if(variant==5){if(name=="Simulated Dark Whisper")seedquest(12661);else{clean();sd->num_quests=sd->avail_quests=0;seedquest(12663);}}
            auto r=reason(name);check((variant==0)==r.empty(),"new story/simulation prerequisites, party, leader, reservation and cooldown match shared admission");
            if(variant==5&&name=="Simulated Dark Whisper")check(r.find("60 minutes remain")!=std::string::npos,"native live timer reports actual remaining minutes");clean();}
    }
    for(int scenario=0;scenario<3;++scenario){
        auto sd=fresh();std::vector<std::unique_ptr<map_session_data>> members;
        for(int i=1;i<=3;++i){
            auto member=std::make_unique<map_session_data>();member->id=member->status.account_id=99000100+i;member->status.char_id=99000200+i;member->type=BL_PC;
            member->status.party_id=7;member->status.base_level=200;member->state.ignoretimeout=true;member->npc_idle_timer=INVALID_TIMER;member->vars_ok=true;
            auto& m=guide_party.party.member[i];m.account_id=member->status.account_id;m.char_id=member->status.char_id;std::snprintf(m.name,sizeof(m.name),"Member%d",i);
            guide_party.data[i].sd=member.get();guide_players[member->id]=member.get();members.push_back(std::move(member));
        }
        members[2]->npc_id=777;
        guide_party.party.member[4].account_id=99000300;guide_party.party.member[4].char_id=99000301;std::strcpy(guide_party.party.member[4].name,"Offline");
        if(!scenario){
            seedquest(13184);members[0]->status.base_level=129;
            attached=members[1].get();seedquest(13184);seedquest(13185);attached=sd.get();
            invoke_guide("callfunc \"PN_PartyReadiness\",\"Charleston in Distress\";");
        }else{
            setnum("CH2_Step",2);guide_member_nums[members[0]->id][add_str("CH2_Step")]=2;guide_member_nums[members[1]->id][add_str("CH2_Step")]=scenario==1?1:2;
            auto definition=std::make_shared<s_instance_db>();definition->id=1;definition->name="Crossroads of Tangled Mana";instance_db.put(1,definition);
            auto reservation=std::make_shared<s_instance_data>();reservation->id=1;reservation->state=INSTANCE_BUSY;reservation->mode=IM_PARTY;reservation->owner_id=7;instances[42]=reservation;guide_party.instance_id=42;
            reservation->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);invoke_guide("setinstancevar 'ch2_roster$,\",99000002,99000201,\",42;");
            auto registry=guide_member_nums;invoke_guide("callfunc \"PN_PartyReadiness\",\"Crossroads of Tangled Mana\";");
            check(guide_member_nums==registry,"party review preserves every member's persistent registry");
            if(reservation->regs.arrays)reservation->regs.arrays->destroy(reservation->regs.arrays,script_free_array_db);db_destroy(reservation->regs.vars);reservation->regs={};instances.clear();instance_db.clear();
        }
        auto says=[](const char* part){for(const auto& m:messages)if(m.find(part)!=std::string::npos)return true;return false;};
        check(says("Leader: Ready"),"leader is ready in all party snapshots");
        check(says(!scenario?"Member1: Missing prerequisite: Base Level 130.":"Member1: Ready: matching reservation"),"each online member's actual state is inspected");
        check(says(!scenario?"Member2: Cooldown:":scenario==1?"Member2: Missing prerequisite: Chapter 2 stage 2.":"Member2: Missing prerequisite: you were not on this expedition's original roster."),"individual cooldown, prerequisite and late-join roster checks");
        check(says("Member3: Unavailable: currently using another NPC.")&&members[2]->npc_id==777,"busy member's dialogue remains untouched");
        check(says("Offline: Unavailable: offline or on another map server."),"offline or remote state never reported ready");
        check(!sd->st&&!members[0]->st&&!members[1]->st,"caller restored and temporary member attachments released");
        clean();for(auto& member:members){attached=member.get();clean();}attached=nullptr;guide_players.clear();guide_member_nums.clear();
    }
    attached=nullptr;quest_db.clear();mob_db.clear();item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();std::printf("ONBOARDING_READINESS_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
