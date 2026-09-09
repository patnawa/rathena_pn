// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  episode18_gudra_transaction_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode18_gudra_transaction_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to the tracked crown fixture's boundary prefix.
// Actual Gudra/story/helper VM, inventory, loaded registry, quest mutations and
// wolfvill QuestInfo/achievement conditions. pc_gainexp is a recorded boundary,
// not a claim to execute level-up/status/equipment gameplay.
#include "map/npc.hpp"
#include "map/quest.hpp"
#include "map/mob.hpp"
#include "common/ers.hpp"
#include <climits>
#include <tuple>

namespace {
constexpr int NOTE=1000408, AMETHYST=1000405;
std::map<int,std::unique_ptr<npc_data>> nodes;
std::vector<script_code*> conditions;
std::map<script_code*,std::pair<unsigned,unsigned>> condition_hits;
std::map<script_code*,unsigned> achievement_hits;
std::vector<std::pair<int,int>> item_log;
std::vector<std::pair<uint64,uint64>> exp_calls;
std::vector<int> reputation_packets;
unsigned nested_checks=0, achievement_checks=0, quest_packets=0, save_requests=0;
using Registry=std::map<int64,std::pair<int64,bool>>;
int32 registry_row(DBKey key,DBData* data,va_list args){
    auto* result=va_arg(args,Registry*);check(data->type==DB_DATA_PTR,"owned native registry record");
    auto* p=static_cast<script_reg_num*>(db_data2ptr(data));check(!p->flag.type,"numeric persistent registry");
    (*result)[key.i64]={p->value,p->flag.update!=0};return 0;
}
Registry registry_snapshot(){Registry result;attached->regs.vars->foreach(attached->regs.vars,registry_row,&result);return result;}
void persistent(const char* name,int64 value){check(attached->vars_ok&&pc_setreg2(attached,name,value),"actual loaded registry mutation");}
int reputation(){return static_cast<int>(pc_readreg2(attached,"RepPointsWolf"));}
int state(int id){return std::max(0,quest_check(attached,id,HAVEQUEST));}
std::vector<struct quest> quest_snapshot(){if(!attached->num_quests)return {};return {attached->quest_log,attached->quest_log+attached->num_quests};}
struct GudraSnapshot:Snapshot{
    Registry regs=registry_snapshot();bool dirty=attached->vars_dirty,save=attached->save_quest;
    std::vector<struct quest> qs=quest_snapshot();int avail=attached->avail_quests;
    unsigned callbacks=nested_checks,achievements=achievement_checks;
    void unchanged()const{
        Snapshot::unchanged();check(regs==registry_snapshot()&&dirty==attached->vars_dirty,"persistent values and pending flags unchanged");
        auto current=quest_snapshot();check(qs.size()==current.size()&&(!qs.size()||!std::memcmp(qs.data(),current.data(),qs.size()*sizeof(struct quest))),"entire native quest log unchanged");
        check(avail==attached->avail_quests&&save==attached->save_quest,"quest partition and pending save unchanged");
    }
};
struct PlayerDelete{void operator()(map_session_data* sd)const{
    check(!sd->st&&!sd->regs.arrays,"no suspended native VM or persistent arrays at teardown");
    if(sd->quest_log)aFree(sd->quest_log);sd->regs.vars->destroy(sd->regs.vars,script_reg_destroy);delete sd;
}};
using Player=std::unique_ptr<map_session_data,PlayerDelete>;
void clear_quests(){if(attached->quest_log)aFree(attached->quest_log);attached->quest_log=nullptr;attached->num_quests=attached->avail_quests=0;attached->save_quest=false;}
void seed_quest(int id,int desired){check(quest_add(attached,id)==0,"native quest seed adds real definition");if(desired==2)check(quest_update_status(attached,id,Q_COMPLETE)==0,"native completed partition seed");}
void partition(){int active=0;for(int i=0;i<attached->num_quests;++i){check((i<attached->avail_quests)==(attached->quest_log[i].state<Q_COMPLETE),"native active/completed quest partition");if(attached->quest_log[i].state<Q_COMPLETE)++active;}
    check(active==attached->avail_quests,"exact available quest count");}
void clear_inventory(){attached->inventory={};for(auto& d:attached->inventory_data)d=nullptr;weight();}
void reset_observations(){errors=0;messages.clear();menu_text.clear();closes=0;item_log.clear();exp_calls.clear();reputation_packets.clear();}
Player researcher(int stage=0,int rep=900,int notes=1){
    ++cases;nums.clear();strings.clear();windows.clear();unequips=equips=0;
    Player sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=200;sd->status.class_=JOB_ALITEA;sd->class_=MAPID_ALITEA;
    sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->m=0;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=1000000;
    for(auto& i:sd->equip_index)i=-1;for(auto& i:sd->equip_switch_index)i=-1;
    sd->regs.vars=i64db_alloc(DB_OPT_BASE);sd->vars_ok=true;pc_set_reg_load(true);
    persistent("ep18_main",36);persistent("RepPointsWolf",rep);pc_set_reg_load(false);
    const auto initial=registry_snapshot();check(!sd->vars_dirty&&std::all_of(initial.begin(),initial.end(),[](auto& r){return !r.second.second;}),"native loaded registry begins clean");
    if(notes)put(0,NOTE,notes);weight();
    if(stage==1){for(int i:{16551,16552,16553})seed_quest(i,2);seed_quest(16554,1);}
    if(stage==2){seed_quest(16554,2);for(int i:{16555,16556,16557})seed_quest(i,2);seed_quest(16558,1);}
    if(stage==3)seed_quest(16554,2);
    pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==35,"real wolfvill display has 35 owners");
    sd->save_quest=false;partition();reset_observations();return sd;
}
void flow(script_code* code,const std::vector<int>& choices={},const std::function<void(int)>& hook={},bool original=false,int cancel=-1){
    run_script(code,0,attached->id,NPC);int pauses=0,selected=0;
    while(attached->st){check(++pauses<40,"bounded real Gudra/story dialogue");auto* st=attached->st;
        if(hook)hook(pauses);
        if(cancel==pauses){pc_close_npc(attached,2);continue;}
        if(st->state==RERUNLINE){check(selected<static_cast<int>(choices.size()),"answer for actual select");attached->npc_menu=choices[selected++];}
        else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"known native Next/close suspension");
        run_script_main(st);
    }
    if(!original)check(!errors,"candidate emitted no script error");
    check(!attached->state.menu_or_input&&!unequips&&!equips&&windows.empty(),"no pending menu or unrelated equip operation");partition();
}
void refused(const GudraSnapshot& before){before.unchanged();check(item_log.empty()&&exp_calls.empty()&&reputation_packets.empty(),"refusal has no debit/grant/EXP/reputation");}
void committed(const GudraSnapshot& before,int stage,int old_notes,int old_amethyst,int amount,int oldrep){
    check(count(NOTE)==old_notes-1&&count(AMETHYST)==old_amethyst+amount,"exact Note debit and Amethyst grant");
    check(reputation()==std::min(5000,oldrep+(stage==1?100:30)),"actual exact reputation addition and cap");
    check(item_log==std::vector<std::pair<int,int>>{{NOTE,-1},{AMETHYST,amount}},"unchanged native material mutation order");
    check(exp_calls==std::vector<std::pair<uint64,uint64>>{{stage==1?7769124U:18252408U,stage==1?3000000U:2000000U}},"actual getexp boundary receives original amounts and order");
    check(reputation_packets==std::vector<int>{reputation()},"one actual reputation notification");
    check(state(16554)==2&&state(16559)==1,"completed first story and cooldown committed");
    check(attached->save_quest,"actual quest save flag set");
    if(stage==2)for(int i=16555;i<=16558;++i)check(!state(i),"daily story/hand-in records erased");
    else for(int i=16551;i<=16553;++i)check(state(i)==2,"first story records remain complete");
    check(pc_readreg2(attached,"ep18_main")== (reputation()>=1000?37:36),"unchanged main progression threshold");
    check(state(18082)==(reputation()>=1000?1:0),"exact main quest at post-award reputation1000");
    check(nested_checks>before.callbacks&&achievement_checks>=before.achievements+7,"real nonempty QI and seven achievement conditions executed per commit");
    check(pc_readreg2(attached,"ARG0")==0&&!attached->achievement_data.count,"zero sell-value achievement cleanup, no completion");
    check(attached->weight==before.weight&&attached->status.zeny==before.zeny,"zero-weight transaction and no new price");
    auto expected_regs=before.regs;expected_regs[add_str("RepPointsWolf")]={reputation(),true};
    if(reputation()>=1000)expected_regs[add_str("ep18_main")]={37,true};
    check(registry_snapshot()==expected_regs&&attached->vars_dirty,"only exact reputation/progression registry writes, including zero and unchanged-cap dirty flags");
    for(int i=0;i<MAX_INVENTORY;++i){const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        if(now.nameid&&now.nameid==old.nameid){item expected=old;expected.amount=now.amount;
            check(!std::memcmp(&expected,&now,sizeof(item)),"retained stack metadata exactly preserved");
            check(old.nameid==NOTE||old.nameid==AMETHYST||old.amount==now.amount,"unrelated item amount preserved");}
        else if(now.nameid==AMETHYST){check(!old.nameid||old.nameid==NOTE,"plain reward only reuses an empty/debited Note slot");
            item expected{};expected.nameid=AMETHYST;expected.amount=amount;expected.identify=1;
            check(!std::memcmp(&expected,&now,sizeof(item)),"plain output does not inherit consumed Note metadata");}
        else check(!now.nameid&&(!old.nameid||old.nameid==NOTE),"only Note inventory row can disappear");
        check(now.nameid?(attached->inventory_data[i]&&attached->inventory_data[i]->nameid==now.nameid):!attached->inventory_data[i],"native inventory/cache identity is coherent");
    }
}
int integer(ryml::NodeRef n){int v;n>>v;return v;}
std::string string(ryml::NodeRef n){std::string v;n>>v;return v;}
void probe(ryml::NodeRef row){
    clear_quests();persistent("ep18_main",0);persistent("ep19_main",0);attached->status.base_level=1;
    for(auto q:row["Quests"]){int id=integer(q["Id"]);seed_quest(id,integer(q["State"]));
        if(q.has_child("CompleteCounts")){bool complete;q["CompleteCounts"]>>complete;if(complete){auto d=quest_db.find(id);for(int j=0;j<attached->num_quests;++j)if(attached->quest_log[j].quest_id==id)for(size_t k=0;k<d->objectives.size();++k)attached->quest_log[j].count[k]=d->objectives[k]->count;}}}
    for(auto v:row["Variables"]){auto name=string(v["Name"]);if(name=="BaseLevel")attached->status.base_level=integer(v["Value"]);else persistent(name.c_str(),integer(v["Value"]));}
    int slot=20;for(auto i:row["Items"])put(slot++,integer(i["Id"]),integer(i["Amount"]));weight();
}
}
extern "C" npc_data* npc_lookup(int32 id){auto i=nodes.find(id);return i==nodes.end()?nullptr:i->second.get();}
extern "C" block_list* gudra_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* gudra_world(int32 id){return attached&&attached->id==id?static_cast<block_list*>(attached):npc_lookup(id);}
extern "C" void crown_log(const map_session_data* sd,e_log_pick_type type,int32 amount,const item* it){check(sd==attached&&type==LOG_TYPE_SCRIPT,"actual script inventory mutation log");item_log.emplace_back(it->nameid,amount);}
extern "C" void gudra_rep(const map_session_data&,int64,int64) asm("__wrap__Z20clif_reputation_typeRK16map_session_datall");
extern "C" void gudra_rep(const map_session_data& sd,int64 id,int64 value){check(&sd==attached&&id==3&&value==reputation(),"actual reputation3 packet boundary");reputation_packets.push_back(value);}
extern "C" void gudra_exp(map_session_data*,block_list*,t_exp,t_exp,uint8) asm("__wrap__Z10pc_gainexpP16map_session_dataP10block_listmmh");
extern "C" void gudra_exp(map_session_data* sd,block_list* src,t_exp base,t_exp job,uint8 flag){check(sd==attached&&!src&&flag==1&&!sd->hd,"explicit pc_gainexp boundary with no homunculus fixture");exp_calls.emplace_back(base,job);}
extern "C" void qi_packet(const map_session_data*,const block_list*,e_questinfo_types,e_questinfo_markcolor) asm("__wrap__Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor");
extern "C" void qi_packet(const map_session_data* sd,const block_list* nd,e_questinfo_types,e_questinfo_markcolor){check(sd==attached&&nodes.count(nd->id),"real wolfvill QI packet target");}
extern "C" void quest_add_packet(const map_session_data*,const struct quest*) asm("__wrap__Z14clif_quest_addPK16map_session_dataPK5quest");
extern "C" void quest_add_packet(const map_session_data* sd,const struct quest*){check(sd==attached,"quest packet target");++quest_packets;}
extern "C" void quest_obj_packet(const map_session_data*,const struct quest*) asm("__wrap__Z27clif_quest_update_objectivePK16map_session_dataPK5quest");
extern "C" void quest_obj_packet(const map_session_data* sd,const struct quest*){check(sd==attached,"quest objective packet target");++quest_packets;}
extern "C" void quest_delete_packet(const map_session_data*,int32) asm("__wrap__Z17clif_quest_deletePK16map_session_datai");
extern "C" void quest_delete_packet(const map_session_data* sd,int32){check(sd==attached,"quest deletion packet target");++quest_packets;}
extern "C" void quest_status_packet(const map_session_data*,int32,bool) asm("__wrap__Z24clif_quest_update_statusPK16map_session_dataib");
extern "C" void quest_status_packet(const map_session_data* sd,int32,bool){check(sd==attached,"quest status packet target");++quest_packets;}
extern "C" int32 quest_save(map_session_data*,int32) asm("__wrap__Z10chrif_saveP16map_session_datai");
extern "C" int32 quest_save(map_session_data* sd,int32){check(sd==attached,"quest save transport boundary");++save_requests;return 0;}
extern "C" void story_talk(const block_list*,const char*,send_target) asm("__wrap__Z19clif_disp_overhead_PK10block_listPKc11send_target");
extern "C" void story_talk(const block_list* bl,const char*,send_target target){check(bl==attached&&target==SELF,"actual story completion overhead message boundary");}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code* code,map_session_data* sd){
    check(sd==attached&&sd->vars_ok,"callback has loaded native registry");auto* old=sd->st;int rid=old?old->rid:0;GudraSnapshot before;
    bool value=real_condition(code,sd);before.unchanged();check(sd->st==old&&(!old||old->rid==rid),"nested native VM restores caller and RID");
    if(condition_hits.count(code)){auto& h=condition_hits.at(code);(value?h.second:h.first)++;++nested_checks;}
    else check(!value&&achievement_hits.count(code),"known current zero-sell item condition is false");
    return value;
}
// Calls from achievement.cpp to its own achievement_check_condition need not
// cross the linker wrapper. Observe the real cross-object VM entry instead.
extern "C" void real_run_script(script_code*,int32,int32,int32) asm("__real__Z10run_scriptP11script_codeiii");
extern "C" void observed_run_script(script_code*,int32,int32,int32) asm("__wrap__Z10run_scriptP11script_codeiii");
extern "C" void observed_run_script(script_code* code,int32 pos,int32 rid,int32 oid){
    if(!achievement_hits.count(code)){real_run_script(code,pos,rid,oid);return;}
    check(attached&&rid==attached->id&&pos==0,"actual achievement condition VM entry");GudraSnapshot before;
    real_run_script(code,pos,rid,oid);before.unchanged();
    check(attached->st&&script_getnum(attached->st,2)==0,"actual zero-sell achievement result false");
    ++achievement_hits.at(code);++achievement_checks;
}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit fixture directory/mode");bool original=std::string(argv[2])=="original";
    check(original||std::string(argv[2])=="candidate","known proof mode");deny_network();
    static char server[]="episode18-gudra-test";SERVER_NAME=server;malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    script_config_read("conf/script_athena.conf");check(script_config.check_cmdcount==655360&&script_config.check_gotocount==2048,"actual configured limits; no freeloop");
    battle_config.atcommand_disable_npc=0;battle_config.feature_achievement=1;battle_config.quest_exp_rate=100;save_settings|=CHARSAVE_QUEST;
    num_reg_ers=ers_new(sizeof(script_reg_num),"gudra:native-num-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"gudra:native-str-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    std::strcpy(::map[0].name,"wolfvill");std::strcpy(::map[1].name,"prontera");
    auto n=std::make_unique<npc_data>();n->id=NPC;n->type=BL_NPC;n->m=0;fake_nd=n.get();nodes.emplace(NPC,std::move(n));
    const std::string dir=argv[1];auto load=[&](const char* file,const auto& parse){auto txt=read(dir+"/"+file+".yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(txt));for(auto row:tree["Body"])parse(row);};
    load("items",[&](ryml::NodeRef row){check(item_db.parseBodyNode(row)==1,"actual dependency item definition parses");});
    for(int id:{NOTE,AMETHYST}){auto d=item_db.find(id);check(d&&d->type==IT_ETC&&!d->weight&&!d->value_sell&&!d->flag.guid&&!d->stack.inventory,"native exact Gudra Etc scalar premise");}
    load("reputation",[&](ryml::NodeRef row){check(reputation_db.parseBodyNode(row)==1,"actual ordered reputation parser");});
    int64 constant=-1;check(script_get_constant("REPUTATION_EP18",&constant)&&constant==3,"actual unique reputation constant");
    load("quest-mob-identities",[&](ryml::NodeRef row){auto m=std::make_shared<s_mob_db>();m->id=integer(row["Id"]);m->sprite=string(row["AegisName"]);mob_db.put(m->id,m);});
    load("quests",[&](ryml::NodeRef row){check(quest_db.parseBodyNode(row)==1,"actual ordered quest parser");});check(quest_db.size()==67,"67 native quest dependencies");
    load("achievements",[&](ryml::NodeRef row){check(achievement_db.parseBodyNode(row)==1,"actual Get_Item achievement parser");});check(achievement_db.size()==7,"all seven native item conditions");
    for(auto& row:achievement_db){check(row.second->condition!=nullptr,"actual parsed achievement condition");achievement_hits[row.second->condition]=0;}
    int nodeid=NPC+1;load("questinfo",[&](ryml::NodeRef row){auto q=std::make_unique<npc_data>();q->id=nodeid;q->type=BL_NPC;q->m=0;nodes.emplace(nodeid,std::move(q));
        auto* code=compile(string(row["Script"]),"actual-wolfvill-registration");run_script(code,0,0,nodeid);check(!errors,"literal original QI registration succeeds");script_free_code(code);
        auto* nd=nodes[nodeid++].get();check(nd->qi_data.size()==row["Conditions"].num_children(),"original registration order/count");for(auto& qi:nd->qi_data){conditions.push_back(qi->condition);condition_hits[qi->condition]={0,0};}});
    check(::map[0].qi_npc.size()==35&&conditions.size()==72,"actual 35-owner/72-condition wolfvill closure");
    const auto source=read(dir+(original?"/before.txt":"/after.txt"));
    auto* npc=compile(body(source,"Folklorist Gudra#ep18"),original?"genuine-original-Gudra":"proposed-Gudra");
    std::vector<script_code*> stories;for(const char* name:{"Dinar#ep18","Amira#ep18","Shanina#ep18"})stories.push_back(compile(body(source,name),name));

    // Real acceptance -> all three actual story NPCs -> hand-in, both rounds.
    {auto sd=researcher(0,900,0);flow(npc,{1,1,1},{},original);check(count(NOTE)==1,"one original-policy initial Note");
        for(auto* story:stories)flow(story,{1,1},{},original);check(state(16554)==1,"actual first stories set active hand-in");
        for(int id:{16551,16552,16553})check(state(id)==2,"actual first story completion satisfies final guards");
        reset_observations();GudraSnapshot before;flow(npc,{}, {},original);committed(before,1,1,0,20,900);
    }
    if(original){
        // Explicit injected pause fixture; ordinary party receiver reachability
        // is separately source-derived, not falsely claimed as this fixture.
        for(int stage:{1,2}){auto sd=researcher(stage,stage==1?900:4970);put(1,AMETHYST,29980);weight();
            flow(npc,{},[&](int pause){if(pause==1)put(1,AMETHYST,30000);},true);
            check(errors==1&&count(NOTE)==0&&count(AMETHYST)==30000&&state(16559)==1&&reputation()==(stage==1?1000:5000)&&exp_calls.size()==1,"original loses output but still commits completion/reputation/EXP");}
        {auto sd=researcher(0,900,0);sd->status.inventory_slots=2;put(0,AMETHYST,1);put(1,7110,1);weight();flow(npc,{1,1,1},{},true);
            check(errors==1&&!count(NOTE)&&state(16551)==1&&state(16552)==1&&state(16553)==1,"original initial capacity mismatch commits quests without Note");}
        {auto sd=researcher(1,900,0);flow(npc,{}, {},true);check(errors==1&&state(16554)==1&&!state(16559)&&exp_calls.empty()&&reputation()==900,"original missing first Note ends before commit and never recovers");}
        for(int rep:{4969,4970}){auto sd=researcher(2,rep);put(1,AMETHYST,30000-(rep==4969?3:4));weight();GudraSnapshot before;flow(npc,{}, {},true);refused(before);}
    }else{
        for(int stage:{1,2})for(int rep:{-5000,-100,-30,-1,0,899,900,969,970,4969,4970,4999,5000})for(int notes:{1,2}){
            auto sd=researcher(stage,rep,notes);int amount=stage==1?20:3+(rep>=4970);put(1,AMETHYST,30000-amount);weight();GudraSnapshot before;
            flow(npc);committed(before,stage,notes,30000-amount,amount,rep);
        }
        for(int stage:{1,2})for(int pause=1;pause<=4;++pause)for(int change=0;change<7;++change){
            auto sd=researcher(stage,900);put(1,AMETHYST,29980);weight();std::unique_ptr<GudraSnapshot> before;bool reached=false;
            flow(npc,{},[&](int n){if(n!=pause)return;reached=true;
                if(change==0)put(1,AMETHYST,30000);else if(change==1){sd->inventory.u.items_inventory[0]={};sd->inventory_data[0]=nullptr;}
                else if(change==2)sd->m=1;else if(change==3)persistent("ep18_main",35);
                else if(change==4)check(quest_delete(sd.get(),stage==1?16551:16555)==0,"injected stale story state");
                else if(change==5)check(quest_update_status(sd.get(),stage==1?16554:16558,Q_COMPLETE)==0,"injected stale hand-in state");
                else seed_quest(16559,1);
                weight();reset_observations();before=std::make_unique<GudraSnapshot>();});
            check(reached&&before!=nullptr,"requested genuine Next reached");refused(*before);
        }
        for(int stage:{1,2})for(int rep:{4969,4970}){auto sd=researcher(stage,rep);int updated=rep==4969?4970:4969;int amount=stage==1?20:3+(updated>=4970);
            std::unique_ptr<GudraSnapshot> before;flow(npc,{},[&](int pause){if(pause==4){persistent("RepPointsWolf",updated);before=std::make_unique<GudraSnapshot>();}});
            check(before!=nullptr,"fresh reputation injected at final Next");committed(*before,stage,1,0,amount,updated);}
        for(int stage:{1,2}){auto sd=researcher(stage,900,0);flow(npc);check(!count(NOTE)&&count(AMETHYST)==(stage==1?20:3)&&state(16559)==1,"missing first and daily Note recover then complete");}
        for(int stage:{0,3}){auto sd=researcher(stage,900,0);sd->status.inventory_slots=2;put(0,AMETHYST,1);put(1,7110,1);weight();GudraSnapshot before;
            flow(npc,stage==0?std::vector<int>{1,1,1}:std::vector<int>{1});refused(before);}
        for(int stage:{0,3})for(int pause=1;pause<=(stage==0?13:4);++pause)for(int change=0;change<4;++change){
            auto sd=researcher(stage,900,0);std::unique_ptr<GudraSnapshot> before;bool reached=false;
            flow(npc,stage==0?std::vector<int>{1,1,1}:std::vector<int>{1},[&](int n){if(n!=pause)return;reached=true;
                if(change==0)sd->m=1;else if(change==1)persistent("ep18_main",35);
                else if(change==2)seed_quest(stage==0?16551:16555,1);
                else {sd->status.inventory_slots=1;put(0,7110,1);weight();}
                reset_observations();before=std::make_unique<GudraSnapshot>();});
            check(reached&&before!=nullptr,"actual acceptance yield reached");refused(*before);
        }
        for(int stage:{0,3}){auto sd=researcher(stage,900,1);flow(npc,stage==0?std::vector<int>{1,1,1}:std::vector<int>{1});check(count(NOTE)==2,"preserve original unconditional Note issuance policy on new acceptance");}
        for(bool daily:{false,true})for(bool full:{false,true}){auto sd=researcher(daily?3:0,900,0);seed_quest(daily?16555:16551,1);
            if(full){sd->status.inventory_slots=1;put(0,AMETHYST,1);weight();}reset_observations();GudraSnapshot before;flow(npc);
            if(full)refused(before);else check(count(NOTE)==1&&state(daily?16555:16551)==1&&exp_calls.empty(),"existing incomplete-story Note recovery preserves progress");}
        for(bool expired:{false,true}){auto sd=researcher(2,900);seed_quest(16559,1);
            if(expired)for(int i=0;i<sd->num_quests;++i)if(sd->quest_log[i].quest_id==16559)sd->quest_log[i].time=static_cast<uint32>(time(nullptr)-1);
            reset_observations();GudraSnapshot before;flow(npc);if(expired)committed(before,2,1,0,3,900);else refused(before);}
        // Slot released by actual preferred/fallback delitem; compatible output
        // metadata and first-match overflow are not approximated by ID-only.
        for(int variant=0;variant<6;++variant)for(int notes:{1,2}){
            auto sd=researcher(1,900,notes);sd->status.inventory_slots=3;put(1,AMETHYST,1);put(2,7110,1);
            auto& out=sd->inventory.u.items_inventory[1];if(variant==0)out.bound=BOUND_CHAR;else if(variant==1)out.unique_id=UINT64_MAX;
            else if(variant==2)out.card[2]=4700;else if(variant==3)out.expire_time=2100000000;
            else if(variant==4){out.refine=12;out.identify=0;out.attribute=1;out.option[0].id=1;out.option[0].value=1;}
            weight();GudraSnapshot before;flow(npc);
            if(variant<4&&notes==2)refused(before);else committed(before,1,notes,1,20,900);
        }
        for(bool preferred:{false,true}){auto sd=researcher(1,900,2);sd->status.inventory_slots=3;attached->inventory.u.items_inventory[0].card[0]=4700;
            put(1,NOTE,1);if(!preferred)attached->inventory.u.items_inventory[1].refine=1;put(2,7110,1);weight();GudraSnapshot before;flow(npc);
            if(preferred){committed(before,1,3,0,20,900);check(attached->inventory.u.items_inventory[0].amount==2,"native preferred later Note supplies free cell");}
            else refused(before);
        }
        for(int variant=0;variant<6;++variant){auto sd=researcher(1,900,1);sd->status.inventory_slots=3;
            auto& first=sd->inventory.u.items_inventory[0];put(1,NOTE,2);auto& second=sd->inventory.u.items_inventory[1];second.card[0]=4700;
            if(variant==0)first.card[0]=4700;else if(variant==1)first.refine=1;else if(variant==2){first.card[0]=4700;first.refine=1;}
            else if(variant==3)first.bound=BOUND_CHAR;else if(variant==4)first.unique_id=UINT64_MAX;else first.expire_time=2100000000;
            put(2,7110,1);weight();GudraSnapshot before;flow(npc);committed(before,1,3,0,20,900);
            check(sd->inventory.u.items_inventory[1].amount==2&&sd->inventory.u.items_inventory[0].nameid==AMETHYST,"actual fallback or metadata-agnostic preferred first Note frees the output cell");}
        {auto sd=researcher(1,900);sd->status.inventory_slots=2;put(2,AMETHYST,1);weight();GudraSnapshot before;flow(npc);refused(before);}
        {auto sd=researcher(1,900);sd->status.inventory_slots=MAX_INVENTORY;for(int i=1;i<MAX_INVENTORY;++i)put(i,7110,1);weight();GudraSnapshot before;flow(npc);committed(before,1,1,0,20,900);}
        for(int slots:{0,MAX_INVENTORY+1}){auto sd=researcher(1);sd->status.inventory_slots=slots;GudraSnapshot before;flow(npc);refused(before);}
        for(int stage:{1,2})for(int pause=1;pause<=4;++pause){auto sd=researcher(stage);GudraSnapshot before;flow(npc,{}, {},false,pause);refused(before);}
        // Exercise the genuine daily acceptance/story transition, not merely a
        // hand-crafted completed fixture that could hide a new state restriction.
        {auto sd=researcher(3,970,0);flow(npc,{1});for(auto* story:stories)flow(story,{1,1});check(state(16558)==1,"actual daily stories set hand-in");
            reset_observations();GudraSnapshot before;flow(npc);committed(before,2,1,0,3,970);}
        // Complete the actual first stories after native Note removal. This
        // proves recovery retains genuine story transitions, not a seeded guess.
        {auto sd=researcher(0,900,0);flow(npc,{1,1,1});check(pc_delitem(sd.get(),0,1,0,0,LOG_TYPE_SCRIPT)==0,"actual issued Note removal");
            for(auto* story:stories)flow(story,{1,1});check(state(16554)==1&&!count(NOTE),"actual stories do not require the lost Note");
            reset_observations();flow(npc);check(state(16554)==2&&count(AMETHYST)==20&&!count(NOTE),"recovery completes genuine lost-Note first story");}
        const auto npc_body=body(source,"Folklorist Gudra#ep18");const auto begin=npc_body.find("L_GudraPlain:");const auto finish=npc_body.find("\nOnInit:",begin);
        check(begin!=std::string::npos&&finish!=std::string::npos,"exact candidate local helper extraction");const auto helper=npc_body.substr(begin,finish-begin);
        auto helper_call=[&](int output,int amount,int debit){const auto text="{ @gudra_result=callsub(L_GudraPlain,"+std::to_string(output)+","+std::to_string(amount)+","+std::to_string(debit)+"); end;\n"+helper+"\n}";
            auto* code=compile(text,"exact-candidate-Gudra-helper");GudraSnapshot before;flow(code);before.unchanged();script_free_code(code);return nums[add_str("@gudra_result")]!=0;};
        for(auto [output,amount,debit]:std::vector<std::tuple<int,int,int>>{{7110,1,0},{AMETHYST,0,0},{AMETHYST,-1,0},{AMETHYST,30001,0},{AMETHYST,1,-1},{AMETHYST,1,2},{NOTE,1,1}}){auto sd=researcher();check(!helper_call(output,amount,debit),"invalid helper argument refuses without mutation");}
        {auto sd=researcher();check(helper_call(AMETHYST,30000,0),"exact native maximum is accepted when capacity exists");}
        auto* native_debit=compile("{ delitem 1000408,1; end; }","native-one-Note-reference-debit");
        for(int variant=0;variant<6;++variant)for(int quantity:{3,4,20})for(int old_amount:{0,29980,30000})for(int notes:{1,2}){
            auto sd=researcher(0,0,notes);sd->status.inventory_slots=3;put(1,7110,1);
            if(old_amount){put(2,AMETHYST,old_amount);auto& out=sd->inventory.u.items_inventory[2];
                if(variant==0)out.bound=BOUND_CHAR;else if(variant==1)out.unique_id=UINT64_MAX;else if(variant==2)out.card[0]=4700;else if(variant==3)out.expire_time=2100000000;
                else if(variant==4){out.refine=12;out.identify=0;out.attribute=1;out.enchantgrade=3;out.favorite=1;out.option[0].id=1;out.option[0].value=23;}}
            weight();const bool predicted=helper_call(AMETHYST,quantity,1);flow(native_debit);
            item output{};output.nameid=AMETHYST;output.identify=1;const auto actual=pc_additem(sd.get(),&output,quantity,LOG_TYPE_SCRIPT);
            check(predicted==(actual==ADDITEM_SUCCESS),"exact helper prediction matches real native debit plus plain pc_additem");
        }
        script_free_code(native_debit);
        size_t index=0;load("qi-probes",[&](ryml::NodeRef row){auto sd=researcher(0,0,0);persistent("ep18_main",0);sd->status.base_level=1;GudraSnapshot before;
            check(!checked_condition(conditions.at(index),sd.get()),"each actual condition false baseline");before.unchanged();probe(row);GudraSnapshot ready;
            check(checked_condition(conditions.at(index++),sd.get()),"source-derived native quest/registry condition true");ready.unchanged();pc_show_questinfo(sd.get());});
        check(index==72,"all 72 condition probes executed");for(auto [code,hits]:condition_hits)check(hits.first&&hits.second,"every wolfvill condition true and false");
        {auto sd=researcher();sd->qi_display.pop_back();unsigned before=nested_checks;pc_show_questinfo(sd.get());check(nested_checks==before,"wrong-size display really skips native callback loop");
            pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==35&&nested_checks==before,"real reinit restores display without evaluation");pc_show_questinfo(sd.get());check(nested_checks>before,"reinitialized loop executes");}
    }
    for(auto [code,hits]:achievement_hits)check(hits>0,"each of the seven item conditions entered the actual VM");
    attached=nullptr;script_free_code(npc);for(auto* s:stories)script_free_code(s);conditions.clear();condition_hits.clear();achievement_hits.clear();::map[0].qi_npc.clear();nodes.clear();fake_nd=nullptr;
    quest_db.clear();mob_db.clear();reputation_db.clear();item_db.clear();achievement_db.clear();nums.clear();strings.clear();do_final_script();
    ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf(original?"GUDRA_ORIGINAL_OK cases=%u assertions=%u nested_checks=%u achievement_checks=%u\n":"GUDRA_NATIVE_OK cases=%u assertions=%u nested_checks=%u achievement_checks=%u\n",cases,assertions,nested_checks,achievement_checks);return 0;
}
