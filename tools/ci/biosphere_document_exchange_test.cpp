// GPL-3.0-or-later. Appended to the tracked crown fixture boundary helpers.
// Native NPC/input/inventory/reputation/loaded registry/QuestInfo execution.
#include "map/npc.hpp"
#include "map/quest.hpp"
#include "map/mob.hpp"
#include "common/ers.hpp"
#include <climits>
#include <tuple>

namespace {
constexpr int DOCUMENT=1001289;
std::map<int,std::unique_ptr<npc_data>> nodes;
std::vector<script_code*> conditions;
std::map<script_code*,std::pair<unsigned,unsigned>> condition_hits;
std::vector<int> debits,rep_packets;
unsigned input_count=0,nested_checks=0;
bool waiting_input=false;
using Registry=std::map<int64,std::pair<int64,bool>>;
int32 collect_registry(DBKey key,DBData* data,va_list args){
    auto* result=va_arg(args,Registry*);check(data->type==DB_DATA_PTR,"native persistent registry stores owned records");
    auto* p=static_cast<script_reg_num*>(db_data2ptr(data));check(!p->flag.type,"numeric persistent fixture registry");
    (*result)[key.i64]={p->value,p->flag.update!=0};return 0;
}
Registry registry_snapshot(){Registry rows;attached->regs.vars->foreach(attached->regs.vars,collect_registry,&rows);return rows;}
void set_persistent(const char* name,int64 value){check(attached->vars_ok&&pc_setreg2(attached,name,value),"actual loaded persistent registry write");}
int rep(){return static_cast<int>(pc_readreg2(attached,"RepPoints6"));}
struct DocumentSnapshot:Snapshot{
    Registry regs=registry_snapshot();bool dirty=attached->vars_dirty;unsigned prior_callbacks=nested_checks;
    void unchanged()const{Snapshot::unchanged();check(regs==registry_snapshot()&&dirty==attached->vars_dirty,"all loaded persistent values and dirty state unchanged");}
};
struct PlayerDelete {void operator()(map_session_data* sd)const{
    check(!sd->st&&!sd->regs.arrays,"fixture has no suspended state or persistent arrays at teardown");
    if(sd->quest_log)aFree(sd->quest_log);
    sd->regs.vars->destroy(sd->regs.vars,script_reg_destroy);sd->regs.vars=nullptr;delete sd;
}};
using Player=std::unique_ptr<map_session_data,PlayerDelete>;
Player researcher(int reputation,int documents,bool split=false){
    ++cases;errors=0;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    closes=unequips=equips=0;debits.clear();rep_packets.clear();input_count=0;waiting_input=false;
    Player sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=250;sd->status.class_=JOB_ALITEA;sd->class_=MAPID_ALITEA;
    sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->m=0;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=1000000;
    for(auto& i:sd->equip_index)i=-1;
    sd->regs.vars=i64db_alloc(DB_OPT_BASE);sd->vars_ok=true;
    pc_set_reg_load(true);
    set_persistent("ep17_2_main",33);set_persistent("RepPoints6",reputation);set_persistent("RepPoints9",-100);
    pc_set_reg_load(false);
    auto initial=registry_snapshot();check(!sd->vars_dirty&&std::all_of(initial.begin(),initial.end(),[](const auto& row){return !row.second.second;}),"native load seeds clean values without pending update flags");
    if(documents){int first=split&&documents>1?1:documents;put(0,DOCUMENT,first);
        if(first<documents){put(1,DOCUMENT,documents-first);sd->inventory.u.items_inventory[1].unique_id=UINT64_MAX;}}
    put(5,7110,7);weight();pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==19,"actual nonempty QuestInfo display initialization");
    return sd;
}
bool success_text(){for(const auto& s:messages)if(s.find("Research registered. You gained")==0)return true;return false;}
void flow(script_code* code,int amount,const std::function<void()>& hook={},int yield=1,bool cancel=false,bool old=false){
    run_script(code,0,attached->id,NPC);int pauses=0;bool injected=false;
    while(attached->st){check(++pauses<12,"actual exchange dialogue terminates");auto* st=attached->st;
        const bool numeric=st->state==RERUNLINE&&waiting_input;
        if(!injected&&((yield==1&&numeric)||(yield==0&&st->state==STOP))){if(hook)hook();injected=true;
            if(cancel){waiting_input=false;pc_close_npc(attached,2);continue;}}
        if(numeric){waiting_input=false;attached->npc_amount=amount;}
        else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"only Next or numeric input suspends exchange");
        run_script_main(st);
    }
    if(hook||cancel)check(injected,"requested real dialogue yield reached");
    if(!old)check(!errors,"candidate emits no native error");
    check(!attached->state.menu_or_input&&menu_text.empty()&&windows.empty()&&!equips&&!unequips,"no pending input or unrelated service");
}
void refused(const DocumentSnapshot& before){before.unchanged();check(debits.empty()&&rep_packets.empty()&&!success_text(),"refusal never debits, awards or claims success");}
void committed(const DocumentSnapshot& before,int amount,int gain){
    check(nested_checks>before.prior_callbacks,"actual document deletion executes nonempty native QuestInfo loop");
    auto prior=before.regs.find(add_str("RepPoints6"));int old_rep=prior==before.regs.end()?0:prior->second.first;
    check(rep()==old_rep+gain,"exact native reputation delta");
    Registry expected=before.regs;expected[add_str("RepPoints6")]={rep(),true};
    check(expected==registry_snapshot()&&attached->vars_dirty,"only reputation value is persisted and marked dirty");
    int removed=0;for(int n:debits){check(n>0&&n<=6668,"document debit fits proven native bound");removed+=n;}
    check(removed==2*amount&&count(DOCUMENT)==[&](){int n=0;for(auto& it:before.inventory.u.items_inventory)if(it.nameid==DOCUMENT)n+=it.amount;return n;}()-2*amount,"exact document debit");
    check(rep_packets==std::vector<int>{rep()},"actual reputation builtin emits one exact current total");
    check(attached->weight==before.weight-2*amount&&attached->status.zeny==before.zeny,"native document weight and unchanged Zeny");
    for(int i=0;i<MAX_INVENTORY;++i){const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        if(old.nameid==DOCUMENT){if(now.nameid){item expected=old;expected.amount=now.amount;check(!std::memcmp(&expected,&now,sizeof(item)),"retained document metadata unchanged");}else check(!attached->inventory_data[i],"consumed document data pointer cleared");}
        else check(!std::memcmp(&old,&now,sizeof(item)),"all unrelated inventory unchanged");}
    std::string expected_text="Research registered. You gained ^33AA33"+std::to_string(gain)+" Depth 1 reputation^000000.";
    check(std::find(messages.begin(),messages.end(),expected_text)!=messages.end(),"original confirmation reports actual award");
}
int integer(ryml::NodeRef n){int v;n>>v;return v;}
std::string string(ryml::NodeRef n){std::string v;n>>v;return v;}
void apply_probe(ryml::NodeRef probe){
    auto qs=probe["Quests"];int n=static_cast<int>(qs.num_children());
    if(n)attached->quest_log=(struct quest*)aCalloc(n,sizeof(struct quest));attached->num_quests=n;int index=0;
    for(auto row:qs){auto& q=attached->quest_log[index++];q.quest_id=integer(row["Id"]);q.state=static_cast<e_quest_state>(integer(row["State"]));
        bool expired,complete;row["Expired"]>>expired;row["CompleteCounts"]>>complete;q.time=static_cast<uint32>(time(nullptr)+(expired?-1000:86400));
        if(complete){auto data=quest_db.find(q.quest_id);check(data!=nullptr,"actual quest target definition exists");for(size_t i=0;i<data->objectives.size();++i)q.count[i]=data->objectives[i]->count;}}
    for(auto row:probe["Variables"]){auto name=string(row["Name"]);if(name=="BaseLevel")attached->status.base_level=integer(row["Value"]);else set_persistent(name.c_str(),integer(row["Value"]));}
    int slot=MAX_INVENTORY-1;for(auto row:probe["Items"])put(slot--,integer(row["Id"]),integer(row["Amount"]));weight();
}
}
extern "C" npc_data* npc_lookup(int32 id){auto i=nodes.find(id);return i==nodes.end()?nullptr:i->second.get();}
extern "C" block_list* document_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* document_world(int32 id){if(attached&&attached->id==id)return attached;return npc_lookup(id);}
extern "C" void document_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void document_input(map_session_data&,uint32){waiting_input=true;++input_count;}
extern "C" void crown_log(const map_session_data* sd,e_log_pick_type type,int32 amount,const item* it){check(sd==attached&&type==LOG_TYPE_SCRIPT&&it->nameid==DOCUMENT&&amount<0,"only actual document deletion logged");debits.push_back(-amount);}
extern "C" void document_rep(const map_session_data&,int64,int64) asm("__wrap__Z20clif_reputation_typeRK16map_session_datall");
extern "C" void document_rep(const map_session_data& sd,int64 type,int64 points){check(&sd==attached&&type==6&&points==rep(),"actual reputation notification arguments before packet construction");rep_packets.push_back(points);}
extern "C" void qi_packet(const map_session_data*,const block_list*,e_questinfo_types,e_questinfo_markcolor) asm("__wrap__Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor");
extern "C" void qi_packet(const map_session_data* sd,const block_list* nd,e_questinfo_types,e_questinfo_markcolor){check(sd==attached&&nodes.count(nd->id),"actual QI packet destination");}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code* code,map_session_data* sd){
    check(sd==attached&&sd->vars_ok,"nested condition uses actual loaded player");auto* old=sd->st;int rid=old?old->rid:0;DocumentSnapshot before;
    std::vector<struct quest> qs;if(sd->num_quests)qs.assign(sd->quest_log,sd->quest_log+sd->num_quests);
    bool value=real_condition(code,sd);before.unchanged();check(sd->st==old&&(!old||old->rid==rid),"nested VM restores actual caller and RID");
    check(sd->num_quests==static_cast<int>(qs.size())&&(!sd->num_quests||!std::memcmp(sd->quest_log,qs.data(),qs.size()*sizeof(struct quest))),"QI leaves quest log unchanged");
    auto& hits=condition_hits.at(code);(value?hits.second:hits.first)++;++nested_checks;return value;
}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit artifact directory and mode");bool original=std::string(argv[2])=="original";check(original||std::string(argv[2])=="candidate","known proof mode");deny_network();
    static char server[]="biosphere-document-exchange-test";SERVER_NAME=server;malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    int64 binding=-1;check(script_get_constant("REPUTATION_BIOSPHERE_DEPTH1",&binding)&&binding==6,"actual ConstantDatabase file initialization resolves unique gated binding");
    num_reg_ers=ers_new(sizeof(script_reg_num),"document-test:native-num-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"document-test:native-str-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    battle_config.atcommand_disable_npc=0;std::strcpy(::map[0].name,"ba_in01");std::strcpy(::map[1].name,"prontera");
    auto fake=std::make_unique<npc_data>();fake->id=NPC;fake->type=BL_NPC;fake->m=0;fake_nd=fake.get();nodes.emplace(NPC,std::move(fake));
    const std::string dir=argv[1];
    auto load=[&](const char* file,const auto& parse){auto text=read(dir+"/"+file+".yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(text));for(auto row:tree["Body"])parse(row);};
    load("items",[&](ryml::NodeRef row){check(item_db.parseBodyNode(row)==1,"actual dependency item parses");});
    load("reputation",[&](ryml::NodeRef row){check(reputation_db.parseBodyNode(row)==1,"actual reputation definition parses");});
    auto r=reputation_db.find(6);check(r&&r->variable=="RepPoints6"&&r->minimum==-5000&&r->maximum==5000,"actual bounded reputation identity");
    load("quest-mob-identities",[&](ryml::NodeRef row){auto m=std::make_shared<s_mob_db>();m->id=integer(row["Id"]);m->sprite=string(row["AegisName"]);mob_db.put(m->id,m);});
    load("quests",[&](ryml::NodeRef row){check(quest_db.parseBodyNode(row)==1,"actual ordered quest definition parses");});check(quest_db.size()==36,"effective native quest dependencies");
    int nodeid=NPC+1;load("questinfo",[&](ryml::NodeRef row){auto n=std::make_unique<npc_data>();n->id=nodeid;n->type=BL_NPC;n->m=0;nodes.emplace(nodeid,std::move(n));
        auto* s=compile(string(row["Script"]),"original-ba-in01-registrations");run_script(s,0,0,nodeid);check(!errors,"actual QI registration succeeds");script_free_code(s);
        auto* nd=nodes[nodeid++].get();check(nd->qi_data.size()==row["Conditions"].num_children(),"original owner registration order/count");for(auto& qi:nd->qi_data){conditions.push_back(qi->condition);condition_hits[qi->condition]={0,0};}});
    check(::map[0].qi_npc.size()==19&&conditions.size()==33,"real nonempty 19-owner/33-condition service map");
    const auto source=read(dir+(original?"/before.txt":"/after.txt"));
    auto* access=compile(body(source,"function\tscript\tF_BioDepthQuestAccess"),"actual-Depth-access");strdb_put(script_get_userfunc_db(),"F_BioDepthQuestAccess",access);
    auto* npc=compile(body(source,"Depth Research Administrator#bio_d1"),original?"pinned-original-document-exchange":"actual-document-exchange");
    struct Change{int before,amount,after;bool allow;int gain;};
    const Change changes[]={{4990,4,4999,false,0},{4000,10,5000,false,0},{4990,2,4995,true,5},{4999,1,4000,true,3},{0,5,4999,false,0},{4990,3,4980,true,9},{-5000,3334,-4999,false,0}};
    if(original){
        for(auto c:changes)for(int yield:{0,1}){auto sd=researcher(c.before,2*c.amount);flow(npc,c.amount,[&](){set_persistent("RepPoints6",c.after);},yield,false,true);
            int oldgain=std::min(3*c.amount,5000-c.before),actual=std::min(5000,c.after+oldgain)-c.after;
            check(!errors&&!count(DOCUMENT)&&rep()==c.after+actual,"genuine original stale-state arithmetic and debit");
            check(rep_packets==std::vector<int>{rep()},"original actual award notification recorded");
            std::string claimed="Research registered. You gained ^33AA33"+std::to_string(oldgain)+" Depth 1 reputation^000000.";
            check(std::find(messages.begin(),messages.end(),claimed)!=messages.end(),"original confirmation uses stale gain");}
        for(int input:{0,-1,6,INT32_MAX}){auto sd=researcher(0,10);flow(npc,input,{},1,false,true);int q=input<1?1:5;check(!errors&&count(DOCUMENT)==10-2*q&&rep()==3*q,"original ignores invalid input status");}
        for(int yield:{0,1}){auto sd=researcher(0,4);std::unique_ptr<DocumentSnapshot> changed;flow(npc,2,[&](){--sd->inventory.u.items_inventory[0].amount;weight();changed=std::make_unique<DocumentSnapshot>();},yield,false,true);check(errors==1,"original insufficient single debit fails exactly once");refused(*changed);}
    }else{
        for(int reputation:{-5000,-4999,-3,-1,0,4995,4996,4997,4998,4999})for(bool maximum:{false,true})for(bool split:{false,true}){
            int q=maximum?(5000-reputation+2)/3:1;auto sd=researcher(reputation,2*q+1,split);DocumentSnapshot before;flow(npc,q);committed(before,q,std::min(3*q,5000-reputation));}
        for(int docs:{0,1}){auto sd=researcher(0,docs);DocumentSnapshot before;flow(npc,1);refused(before);check(!input_count,"insufficient initial documents never request input");}
        {auto sd=researcher(5000,10);DocumentSnapshot before;flow(npc,1);refused(before);check(!input_count,"already capped never requests input");}
        for(int input:{0,-1,6,INT32_MAX}){auto sd=researcher(0,10);DocumentSnapshot before;flow(npc,input);refused(before);}
        for(auto c:changes)for(int yield:{0,1}){auto sd=researcher(c.before,2*c.amount);std::unique_ptr<DocumentSnapshot> changed;
            flow(npc,c.amount,[&](){set_persistent("RepPoints6",c.after);changed=std::make_unique<DocumentSnapshot>();},yield);
            if(c.allow)committed(*changed,c.amount,c.gain);else refused(*changed);}
        for(int yield:{0,1}){auto sd=researcher(0,4);std::unique_ptr<DocumentSnapshot> changed;flow(npc,2,[&](){--sd->inventory.u.items_inventory[0].amount;weight();changed=std::make_unique<DocumentSnapshot>();},yield);refused(*changed);}
        for(int yield:{0,1})for(int change:{0,1,2}){auto sd=researcher(0,4);std::unique_ptr<DocumentSnapshot> changed;
            flow(npc,2,[&](){if(change==0)sd->status.base_level=249;else if(change==1)set_persistent("ep17_2_main",32);else sd->m=1;changed=std::make_unique<DocumentSnapshot>();},yield);refused(*changed);}
        for(int yield:{0,1}){auto sd=researcher(0,4);DocumentSnapshot before;flow(npc,2,{},yield,true);refused(before);}
        size_t index=0;load("qi-probes",[&](ryml::NodeRef probe){auto sd=researcher(0,2);sd->status.base_level=1;set_persistent("ep17_2_main",0);DocumentSnapshot before;
            check(!checked_condition(conditions.at(index),sd.get()),"actual condition false with absent quests");before.unchanged();apply_probe(probe);DocumentSnapshot ready;
            check(checked_condition(conditions.at(index++),sd.get()),"source-derived condition true using native registry and quests");ready.unchanged();pc_show_questinfo(sd.get());});
        check(index==33,"all actual condition true/false probes");for(auto [code,hits]:condition_hits)check(hits.first&&hits.second,"every condition sees both outcomes");
        {auto sd=researcher(0,2);sd->qi_display.pop_back();unsigned before=nested_checks;pc_show_questinfo(sd.get());
            check(nested_checks==before,"wrong display size really skips native QuestInfo evaluation");
            pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==19&&nested_checks==before,"native reinit only restores display size");
            pc_show_questinfo(sd.get());check(nested_checks>before,"restored display enables real native condition execution");}
    }
    attached=nullptr;script_free_code(npc);conditions.clear();condition_hits.clear();::map[0].qi_npc.clear();nodes.clear();fake_nd=nullptr;
    quest_db.clear();mob_db.clear();reputation_db.clear();item_db.clear();nums.clear();strings.clear();do_final_script();
    ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf(original?"DOCUMENT_ORIGINAL_OK cases=%u assertions=%u nested_checks=%u\n":"DOCUMENT_NATIVE_OK cases=%u assertions=%u nested_checks=%u\n",cases,assertions,nested_checks);return 0;
}
