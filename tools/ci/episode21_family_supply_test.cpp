// GPL-3.0-or-later. Appended to the tracked crown fixture boundary prefix.
// Actual Mandel/select/Next/inventory/loaded-registry/quest/reputation/QI VM.
#include "map/npc.hpp"
#include "map/quest.hpp"
#include "map/mob.hpp"
#include "common/ers.hpp"
#include <array>
#include <tuple>

namespace {
constexpr std::array<int,7> MATERIALS{{1001629,1001648,1001639,1001637,1001646,1001642,1001645}};
constexpr std::array<int,7> QUESTS{{17782,17783,17785,17786,17787,17784,17788}};
constexpr std::array<int,7> REPUTATIONS{{13,14,15,16,17,18,19}};
constexpr std::array<const char*,7> REP_VARIABLES{{"REP_EP21","REP_EP21_Nerius","REP_EP21_Heine","REP_EP21_Lugenburg","REP_EP21_Walter","REP_EP21_Wigner","REP_EP21_Richard"}};
constexpr int VOUCHER=1001618;

std::map<int,std::unique_ptr<npc_data>> nodes;
std::vector<std::pair<int,int>> item_log;
std::vector<std::tuple<int,int64,int64>> reputation_packets;
std::vector<std::tuple<bool,int64,int64>> marker_values;
std::vector<std::pair<int64,int64>> marker_packets;
unsigned marker_checks=0,quest_packets=0,save_requests=0,clock_reads=0;
unsigned expected_current_errors=0;
bool expected_diagnostic_mode=false;

// The one native container holds both pointer-backed persistent rows and
// integer-backed temporary character rows.  Preserve both in snapshots.
using Registry=std::map<int64,std::string>;
int32 registry_row(DBKey key,DBData* data,va_list args){
    auto* result=va_arg(args,Registry*);
    const char* name=get_str(script_getvarid(key.i64));
    if(name&&name[0]=='@')return 0; // transient getinventorylist/menu state is an explicit boundary
    if(data->type==DB_DATA_INT){(*result)[key.i64]="I:"+std::to_string(data->u.i);return 0;}
    if(data->type==DB_DATA_UINT){(*result)[key.i64]="U:"+std::to_string(data->u.ui);return 0;}
    if(data->type==DB_DATA_I64){(*result)[key.i64]="L:"+std::to_string(data->u.i64);return 0;}
    check(data->type==DB_DATA_PTR,"known native registry representation");
    auto* number=static_cast<script_reg_num*>(db_data2ptr(data));check(number,"owned pointer-backed registry row");
    if(number->flag.type){auto* text=static_cast<script_reg_str*>(db_data2ptr(data));(*result)[key.i64]="S:"+std::string(text->value?text->value:"")+":"+std::to_string(text->flag.update!=0);}
    else (*result)[key.i64]="N:"+std::to_string(number->value)+":"+std::to_string(number->flag.update!=0);
    return 0;
}
Registry registry_snapshot(){Registry result;attached->regs.vars->foreach(attached->regs.vars,registry_row,&result);return result;}
void persistent(const char* name,int64 value){check(attached->vars_ok&&pc_setreg2(attached,name,value),"actual loaded persistent write");}
int64 persistent(const char* name){return pc_readreg2(attached,name);}
int quest_state(int id){return std::max(0,quest_check(attached,id,HAVEQUEST));}
std::vector<struct quest> quest_snapshot(){
    if(!attached->num_quests)return {};
    return {attached->quest_log,attached->quest_log+attached->num_quests};
}
void check_partition(){
    int active=0;
    for(int i=0;i<attached->num_quests;++i){
        check((i<attached->avail_quests)==(attached->quest_log[i].state<Q_COMPLETE),"native active/completed partition ordering");
        if(attached->quest_log[i].state<Q_COMPLETE)++active;
    }
    check(active==attached->avail_quests,"exact native available quest count");
}
void seed_quest(int id,int state){
    check(quest_add(attached,id)==0,"native family quest seed");
    if(state==2)check(quest_update_status(attached,id,Q_COMPLETE)==0,"native completed family quest seed");
    check_partition();
}
void clear_inventory(){
    attached->inventory={};for(auto& data:attached->inventory_data)data=nullptr;weight();
}
int empty_slot(){
    for(int i=0;i<attached->status.inventory_slots;++i)if(!attached->inventory.u.items_inventory[i].nameid)return i;
    return -1;
}
void add_plain(int id,int amount){int slot=empty_slot();check(slot>=0,"fixture has an empty inventory slot");put(slot,id,amount);weight();}

struct FamilySnapshot:Snapshot{
    Registry registry=registry_snapshot();
    std::vector<struct quest> quests=quest_snapshot();
    int available=attached->avail_quests;bool dirty=attached->vars_dirty,save=attached->save_quest;
    void unchanged()const{
        Snapshot::unchanged();
        check(registry==registry_snapshot()&&dirty==attached->vars_dirty,"loaded character registry and dirty flags unchanged");
        auto now=quest_snapshot();
        check(quests.size()==now.size()&&(!quests.size()||!std::memcmp(quests.data(),now.data(),quests.size()*sizeof(struct quest))),"entire native quest log unchanged");
        check(available==attached->avail_quests&&save==attached->save_quest,"quest partition/save state unchanged");
    }
};
struct PlayerDelete{void operator()(map_session_data* sd)const{
    check(!sd->st,"no suspended family VM at teardown");
    if(sd->quest_log)aFree(sd->quest_log);
    if(sd->regs.arrays){sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);sd->regs.arrays=nullptr;}
    sd->regs.vars->destroy(sd->regs.vars,script_reg_destroy);sd->regs.vars=nullptr;delete sd;
}};
using Player=std::unique_ptr<map_session_data,PlayerDelete>;

void reset_observations(){
    errors=0;messages.clear();menu_text.clear();windows.clear();closes=unequips=equips=0;
    item_log.clear();reputation_packets.clear();marker_values.clear();marker_packets.clear();
    quest_packets=save_requests=clock_reads=0;expected_diagnostic_mode=false;
}
Player patron(int active_family=0,int material=0,int amount=0,int selected_rep=0,
              int64 daily=0,bool unlocked=true,bool campaign=true){
    ++cases;nums.clear();strings.clear();windows.clear();unequips=equips=0;
    Player sd(new map_session_data());attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.base_level=250;sd->status.class_=JOB_ALITEA;sd->class_=MAPID_ALITEA;
    sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->m=0;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=1000000;
    for(auto& index:sd->equip_index)index=-1;for(auto& index:sd->equip_switch_index)index=-1;
    sd->regs.vars=i64db_alloc(DB_OPT_BASE);sd->vars_ok=true;pc_set_reg_load(true);
    if(campaign)persistent("EP21_Main_Complete",1);
    if(unlocked)persistent("EP21_SupplyUnlocked",1);
    if(active_family)persistent("EP21_SupplyFamily",active_family);
    if(daily)persistent("EP21_Supply_Daily",daily);
    for(size_t i=0;i<REP_VARIABLES.size();++i)persistent(REP_VARIABLES[i],active_family==static_cast<int>(i+1)?selected_rep:0);
    pc_set_reg_load(false);
    if(material&&amount)put(0,material,amount);weight();
    setnum("$@FAMILY_TEST_KEY",100);
    pc_show_questinfo_reinit(sd.get());check(sd->qi_display.size()==1,"one native Mandel marker display slot");
    check_partition();reset_observations();return sd;
}

void flow(script_code* code,const std::vector<int>& choices={},const std::function<void(int,int)>& hook={},bool allow_expected=false){
    expected_diagnostic_mode=allow_expected;
    run_script(code,0,attached->id,NPC);int pauses=0,selected=0;
    while(attached->st){
        check(++pauses<20,"bounded actual Mandel dialogue");auto* st=attached->st;
        if(st->state==RERUNLINE){
            if(hook)hook(pauses,selected);
            check(selected<static_cast<int>(choices.size()),"answer supplied for actual Mandel select");
            attached->npc_menu=choices[selected++];
        }else if(st->state==CLOSE)st->state=END;
        else check(st->state==STOP,"only actual Next/close/select suspensions");
        run_script_main(st);
    }
    expected_diagnostic_mode=false;
    check(!attached->state.menu_or_input&&windows.empty()&&!unequips&&!equips,"no pending menu or unrelated family operation");
    check_partition();
    if(!allow_expected)check(!errors,"candidate/current ordinary flow emits no native error");
}

int rep(int family){return static_cast<int>(persistent(REP_VARIABLES.at(family-1)));}
void accepted(int family){
    check(persistent("EP21_SupplyFamily")==family,"exact selected family persisted");
    check(quest_state(QUESTS.at(family-1))==1,"selected family quest is active");
    check(!errors,"acceptance has no duplicate quest diagnostic");
}
void refused(const FamilySnapshot& before){
    before.unchanged();
    check(item_log.empty()&&reputation_packets.empty(),"refusal has no inventory/reputation mutation");
}
bool capacity_prediction(script_code* helper){
    check(pc_setreg2(attached,"@FAMILY_CAPACITY_RESULT",0),"clear transient capacity result");
    unsigned prior_errors=errors;run_script(helper,0,attached->id,NPC);
    check(!attached->st&&errors==prior_errors,"exact voucher helper executes without dialogue/error");
    return pc_readreg2(attached,"@FAMILY_CAPACITY_RESULT")!=0;
}
void committed(const FamilySnapshot& before,int family,int old_rep,int old_material,int old_vouchers,int64 key){
    check(count(MATERIALS.at(family-1))==old_material-10&&count(VOUCHER)==old_vouchers+10,"exact ten-material debit and ten-voucher grant");
    check(item_log==std::vector<std::pair<int,int>>{{MATERIALS.at(family-1),-10},{VOUCHER,10}},"native family inventory mutation order/amount");
    check(rep(family)==std::min(1000,old_rep+100),"actual bounded family reputation addition");
    for(int other=1;other<=7;++other)if(other!=family)check(rep(other)==0,"unselected family reputation unchanged");
    check(reputation_packets==std::vector<std::tuple<int,int64,int64>>{{family,REPUTATIONS.at(family-1),rep(family)}},"one exact family reputation notification");
    check(quest_state(QUESTS.at(family-1))==2,"delivered family quest completed");
    check(persistent("EP21_Supply_Daily")==key&&persistent("EP21_SupplyFamily")==0,"fresh key committed and active family cleared");
    if(family==1)check(persistent("EP21_Reputation")==rep(1),"Gaebolg compatibility mirror follows actual reputation");
    else check(persistent("EP21_Reputation")==0,"non-Gaebolg delivery leaves compatibility mirror absent");
    check(attached->status.zeny==before.zeny&&attached->weight==before.weight-100,"only ten weight-10 materials alter weight; no Zeny");
    check(!marker_values.empty()&&!std::get<0>(marker_values.back()),"final explicit marker evaluation observes completion");
    check(std::get<1>(marker_values.back())==key&&std::get<2>(marker_values.back())==0,"final marker callback observes final daily/family registers");
    check(!attached->qi_display[0].is_active,"final native Mandel marker is hidden");
    check(attached->save_quest&&attached->vars_dirty,"actual quest and loaded registry persistence flags set");
    for(int i=0;i<MAX_INVENTORY;++i){
        const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        if(old.nameid==MATERIALS.at(family-1)){
            if(now.nameid==old.nameid){item expected=old;expected.amount=now.amount;check(!std::memcmp(&expected,&now,sizeof(item)),"remaining supply metadata unchanged");}
            else if(now.nameid==VOUCHER){item expected{};expected.nameid=VOUCHER;expected.amount=10;expected.identify=1;check(!std::memcmp(&expected,&now,sizeof(item)),"plain voucher may reuse fully consumed supply row");}
            else check(!now.nameid&&!attached->inventory_data[i],"consumed supply row clears native item-data pointer");
        }else if(now.nameid==VOUCHER&&old.nameid!=VOUCHER){
            item expected{};expected.nameid=VOUCHER;expected.amount=10;expected.identify=1;
            check(!std::memcmp(&expected,&now,sizeof(item)),"new voucher grant is exact plain native item");
        }else if(old.nameid==VOUCHER){item expected=old;expected.amount=now.amount;check(!std::memcmp(&expected,&now,sizeof(item)),"existing voucher metadata retained");}
        else check(!std::memcmp(&old,&now,sizeof(item)),"unrelated inventory row unchanged");
    }
}
int integer(ryml::NodeRef node){int value;node>>value;return value;}
std::string string(ryml::NodeRef node){std::string value;node>>value;return value;}
}

extern "C" npc_data* npc_lookup(int32 id){auto found=nodes.find(id);return found==nodes.end()?nullptr:found->second.get();}
extern "C" block_list* family_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* family_world(int32 id){if(attached&&attached->id==id)return attached;return npc_lookup(id);}
extern "C" void error(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error(const char* format,...){
    ++errors;
    if(expected_diagnostic_mode){++expected_current_errors;return;}
    std::fputs("[Error]: ",stderr);va_list args;va_start(args,format);std::vfprintf(stderr,format,args);va_end(args);
}
extern "C" int64 switch_read(int64) asm("__wrap__Z14mapreg_readregl");
extern "C" int64 switch_read(int64 key){
    const char* name=get_str(script_getvarid(key));if(name&&std::strcmp(name,"$@FAMILY_TEST_KEY")==0)++clock_reads;
    return nums[key];
}
extern "C" void crown_log(const map_session_data* sd,e_log_pick_type type,int32 amount,const item* it){
    check(sd==attached&&type==LOG_TYPE_SCRIPT,"actual family script inventory log boundary");
    check(it&&(it->nameid==VOUCHER||std::find(MATERIALS.begin(),MATERIALS.end(),it->nameid)!=MATERIALS.end()),"only reviewed family items mutate");
    item_log.emplace_back(it->nameid,amount);
}
extern "C" void family_rep(const map_session_data&,int64,int64) asm("__wrap__Z20clif_reputation_typeRK16map_session_datall");
extern "C" void family_rep(const map_session_data& sd,int64 id,int64 value){
    check(&sd==attached,"actual family reputation notification target");
    auto found=std::find(REPUTATIONS.begin(),REPUTATIONS.end(),id);check(found!=REPUTATIONS.end(),"reviewed family reputation ID");
    int family=static_cast<int>(found-REPUTATIONS.begin())+1;check(value==rep(family),"notification carries actual current reputation");
    reputation_packets.emplace_back(family,id,value);
}
extern "C" void qi_packet(const map_session_data*,const block_list*,e_questinfo_types,e_questinfo_markcolor) asm("__wrap__Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor");
extern "C" void qi_packet(const map_session_data* sd,const block_list* nd,e_questinfo_types icon,e_questinfo_markcolor color){
    check(sd==attached&&nd==npc_lookup(NPC),"native Mandel marker packet destination");marker_packets.emplace_back(icon,color);
}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool checked_condition(script_code* code,map_session_data* sd){
    check(sd==attached&&sd->vars_ok&&code==npc_lookup(NPC)->qi_data.at(0)->condition,"actual Mandel marker condition and loaded player");
    auto registry=registry_snapshot();auto quests=quest_snapshot();auto inventory=sd->inventory;auto* old=sd->st;int rid=old?old->rid:0;
    bool value=real_condition(code,sd);
    check(registry==registry_snapshot(),"marker condition leaves character registry unchanged");
    auto now=quest_snapshot();check(quests.size()==now.size()&&(!quests.size()||!std::memcmp(quests.data(),now.data(),quests.size()*sizeof(struct quest))),"marker condition leaves quest log unchanged");
    check(!std::memcmp(&inventory,&sd->inventory,sizeof(inventory)),"marker condition leaves inventory unchanged");
    check(sd->st==old&&(!old||old->rid==rid),"nested marker VM restores outer caller and RID");
    marker_values.emplace_back(value,persistent("EP21_Supply_Daily"),persistent("EP21_SupplyFamily"));++marker_checks;return value;
}
extern "C" void quest_add_packet(const map_session_data*,const struct quest*) asm("__wrap__Z14clif_quest_addPK16map_session_dataPK5quest");
extern "C" void quest_add_packet(const map_session_data* sd,const struct quest*){check(sd==attached,"family quest-add packet target");++quest_packets;}
extern "C" void quest_obj_packet(const map_session_data*,const struct quest*) asm("__wrap__Z27clif_quest_update_objectivePK16map_session_dataPK5quest");
extern "C" void quest_obj_packet(const map_session_data* sd,const struct quest*){check(sd==attached,"family quest-objective packet target");++quest_packets;}
extern "C" void quest_delete_packet(const map_session_data*,int32) asm("__wrap__Z17clif_quest_deletePK16map_session_datai");
extern "C" void quest_delete_packet(const map_session_data* sd,int32){check(sd==attached,"family quest-delete packet target");++quest_packets;}
extern "C" void quest_status_packet(const map_session_data*,int32,bool) asm("__wrap__Z24clif_quest_update_statusPK16map_session_dataib");
extern "C" void quest_status_packet(const map_session_data* sd,int32,bool){check(sd==attached,"family quest-status packet target");++quest_packets;}
extern "C" int32 quest_save(map_session_data*,int32) asm("__wrap__Z10chrif_saveP16map_session_datai");
extern "C" int32 quest_save(map_session_data* sd,int32){check(sd==attached,"family quest persistence transport boundary");++save_requests;return 0;}

extern struct eri* num_reg_ers;
extern struct eri* str_reg_ers;
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit family fixture directory/mode");
    const bool candidate=std::string(argv[2])=="candidate";check(candidate||std::string(argv[2])=="current","known family proof phase");
    deny_network();static char server[]="episode21-family-supply-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    script_config_read("conf/script_athena.conf");check(script_config.check_cmdcount==655360&&script_config.check_gotocount==2048,"actual configured script limits");
    num_reg_ers=ers_new(sizeof(script_reg_num),"family:native-num-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"family:native-str-reg",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    battle_config.atcommand_disable_npc=0;save_settings|=CHARSAVE_QUEST;
    std::strcpy(::map[0].name,"jor_mbase");std::strcpy(::map[1].name,"prontera");
    auto mandel=std::make_unique<npc_data>();mandel->id=NPC;mandel->type=BL_NPC;mandel->m=0;fake_nd=mandel.get();nodes.emplace(NPC,std::move(mandel));
    const std::string directory=argv[1];
    auto load=[&](const char* file,const auto& parse){auto text=read(directory+"/"+file+".yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(text));for(auto row:tree["Body"])parse(row);};
    load("items",[&](ryml::NodeRef row){check(item_db.parseBodyNode(row)==1,"actual family item definition parses");});
    for(int id:MATERIALS){auto data=item_db.find(id);check(data&&data->type==IT_ETC&&data->weight==10&&!data->flag.guid&&!data->stack.inventory,"exact family material native identity");}
    {auto data=item_db.find(VOUCHER);check(data&&data->type==IT_ETC&&!data->weight&&!data->flag.guid&&!data->stack.inventory,"exact voucher native identity");}
    load("reputation",[&](ryml::NodeRef row){check(reputation_db.parseBodyNode(row)==1,"actual family reputation definition parses");});
    check(reputation_db.size()==7,"exact seven family reputation rows");
    for(size_t i=0;i<REPUTATIONS.size();++i){auto data=reputation_db.find(REPUTATIONS[i]);check(data&&data->variable==REP_VARIABLES[i]&&data->minimum==-1000&&data->maximum==1000,"exact native family reputation identity/range");}
    load("quests",[&](ryml::NodeRef row){check(quest_db.parseBodyNode(row)==1,"actual family quest definition parses");});
    check(quest_db.size()==9,"eight service quests plus main-completion identity");
    const auto source=read(directory+(candidate?"/candidate.txt":"/current.txt"));
    auto* main_complete=compile(body(read(directory+"/main-source.txt"),"function\tscript\tEP21_MainComplete"),"actual-EP21-MainComplete");
    strdb_put(script_get_userfunc_db(),"EP21_MainComplete",main_complete);
    auto* all_family=compile(body(source,"function\tscript\tEP21_AllFamilyReputation"),"actual-EP21-AllFamilyReputation");
    strdb_put(script_get_userfunc_db(),"EP21_AllFamilyReputation",all_family);
    auto* daily=compile("{ return $@FAMILY_TEST_KEY; }","deterministic-EP21-DailyKey-boundary");
    strdb_put(script_get_userfunc_db(),"EP21_DailyKey",daily);
    auto* marker=compile(read(directory+"/marker.txt"),"exact-source-Mandel-OnInit-registration");run_script(marker,0,0,NPC);
    check(!errors&&nodes[NPC]->qi_data.size()==1&&::map[0].qi_npc==std::vector<int32>{NPC},"one actual Mandel OnInit marker registration");script_free_code(marker);
    auto* npc=compile(body(source,"Mandel#ep21_reputation"),candidate?"proposed-Mandel-family-service":"current-Mandel-family-service");
    script_code* capacity=nullptr;
    if(candidate){
        const auto npc_body=body(source,"Mandel#ep21_reputation");
        const auto begin=npc_body.find("L_EP21VoucherCapacity:");const auto finish=npc_body.find("\nOnInit:",begin);
        check(begin!=std::string::npos&&finish!=std::string::npos,"exact candidate voucher helper extraction");
        const auto helper=npc_body.substr(begin,finish-begin);
        capacity=compile("{ @FAMILY_CAPACITY_RESULT=callsub(L_EP21VoucherCapacity); end;\n"+helper+"\n}","exact-candidate-plain-voucher-helper");
    }

    // Unchanged campaign denial and first unlock exercise the actual full body;
    // its explicit end must never fall through into the preserved OnInit label.
    {auto sd=patron(0,0,0,0,0,true,false);FamilySnapshot before;flow(npc);before.unchanged();check(nodes[NPC]->qi_data.size()==1,"denied click cannot duplicate OnInit registration");}
    {auto sd=patron(0,0,0,0,0,false,true);flow(npc);check(persistent("EP21_SupplyUnlocked")==1&&quest_state(17781)==2,"actual one-time family unlock quest completes");check(nodes[NPC]->qi_data.size()==1,"unlock click cannot duplicate OnInit registration");}

    if(candidate){
        // The script helper predicts the real plain pc_additem without a debit.
        // This covers dynamic allowed-slot boundaries and every native stack
        // identity field that pc_additem compares.
        auto compare_capacity=[&](int slots,const std::function<void()>& setup,bool expected){
            auto sd=patron();clear_inventory();sd->status.inventory_slots=slots;setup();weight();reset_observations();
            bool predicted=capacity_prediction(capacity);item output{};output.nameid=VOUCHER;output.identify=1;
            auto actual=pc_additem(sd.get(),&output,10,LOG_TYPE_SCRIPT);
            check(predicted==expected&&predicted==(actual==ADDITEM_SUCCESS),"voucher helper exactly predicts native plain pc_additem");
        };
        compare_capacity(3,[&](){put(0,VOUCHER,29990);put(1,MATERIALS[0],1);put(2,MATERIALS[1],1);},true);
        compare_capacity(3,[&](){put(0,VOUCHER,29991);put(1,MATERIALS[0],1);put(2,MATERIALS[1],1);},false);
        compare_capacity(3,[&](){put(0,MATERIALS[0],1);put(1,MATERIALS[1],1);},true);
        compare_capacity(3,[&](){put(0,MATERIALS[0],1);put(1,MATERIALS[1],1);put(2,MATERIALS[2],1);},false);
        for(int metadata=0;metadata<4;++metadata)for(bool free_slot:{false,true}){
            compare_capacity(3,[&](){put(0,VOUCHER,1);auto& row=attached->inventory.u.items_inventory[0];
                if(metadata==0)row.bound=BOUND_CHAR;else if(metadata==1)row.card[0]=4700;else if(metadata==2)row.unique_id=UINT64_MAX;else row.expire_time=2100000000;
                put(1,MATERIALS[0],1);if(!free_slot)put(2,MATERIALS[1],1);},free_slot);
        }
        compare_capacity(2,[&](){put(0,MATERIALS[0],1);put(1,MATERIALS[1],1);put(2,VOUCHER,1);},false);
        compare_capacity(3,[&](){put(0,VOUCHER,29991);put(1,VOUCHER,1);put(2,MATERIALS[0],1);},false);
        compare_capacity(3,[&](){put(0,VOUCHER,1);auto& row=attached->inventory.u.items_inventory[0];row.refine=12;row.identify=0;row.attribute=1;row.enchantgrade=3;row.favorite=1;row.option[0].id=1;row.option[0].value=23;put(1,MATERIALS[0],1);put(2,MATERIALS[1],1);},true);
        compare_capacity(MAX_INVENTORY,[&](){for(int i=0;i<MAX_INVENTORY-1;++i)put(i,MATERIALS[i%MATERIALS.size()],1);},true);
        compare_capacity(MAX_INVENTORY,[&](){for(int i=0;i<MAX_INVENTORY;++i)put(i,MATERIALS[i%MATERIALS.size()],1);},false);

        // Every mapping and each reputation boundary executes a real delivery.
        for(int family=1;family<=7;++family)for(int old_rep:{-1000,0,900,950,999,1000}){
            auto sd=patron(family,MATERIALS[family-1],10,old_rep);seed_quest(QUESTS[family-1],1);reset_observations();FamilySnapshot before;
            flow(npc,{1});committed(before,family,old_rep,10,0,100);
        }
        // Full accept/deliver/reaccept-next-period/deliver cycles prove that a
        // completed row is erased and recreated without touching another quest.
        for(int family=1;family<=7;++family){
            auto sd=patron(0,MATERIALS[family-1],20,0);flow(npc,{family});accepted(family);
            reset_observations();FamilySnapshot first;flow(npc,{1});committed(first,family,0,20,0,100);
            setnum("$@FAMILY_TEST_KEY",200);reset_observations();flow(npc,{family});accepted(family);
            reset_observations();FamilySnapshot second;flow(npc,{1});committed(second,family,100,10,10,200);
            check(nodes[NPC]->qi_data.size()==1,"repeat cycle preserves one OnInit condition");
        }
        // An already-active selected journal row is a recovery path, not a
        // duplicate setquest call.
        {auto sd=patron();seed_quest(QUESTS[2],1);reset_observations();flow(npc,{3});accepted(3);check(attached->num_quests==1,"active recovery retains exactly one quest row");}
        // The menu crosses the reset.  The fresh key is committed, and a second
        // contract in that period is refused without any state mutation.
        {auto sd=patron(1,MATERIALS[0],20,0);seed_quest(QUESTS[0],1);reset_observations();FamilySnapshot before;
            flow(npc,{1},[&](int,int selected){if(!selected)setnum("$@FAMILY_TEST_KEY",200);});committed(before,1,0,20,0,200);
            reset_observations();FamilySnapshot blocked;flow(npc);blocked.unchanged();
            check(count(MATERIALS[0])==10&&count(VOUCHER)==10&&rep(1)==100,"one and only one post-reset completion");
            check(nodes[NPC]->qi_data.size()==1,"cross-reset/reopen clicks preserve one OnInit condition");
        }
        // Transaction-level defensive cases prove the new preflight runs before
        // payment. Metadata-only voucher rows in a full inventory cannot absorb
        // the plain grant; a real free slot can.
        for(int metadata=0;metadata<4;++metadata)for(bool free_slot:{false,true}){
            auto sd=patron(1,MATERIALS[0],10,0);sd->status.inventory_slots=free_slot?4:3;
            put(1,VOUCHER,1);auto& row=sd->inventory.u.items_inventory[1];
            if(metadata==0)row.bound=BOUND_CHAR;else if(metadata==1)row.card[0]=4700;else if(metadata==2)row.unique_id=UINT64_MAX;else row.expire_time=2100000000;
            put(2,MATERIALS[1],1);seed_quest(QUESTS[0],1);weight();reset_observations();FamilySnapshot before;flow(npc,{1});
            if(free_slot)committed(before,1,0,10,1,100);else refused(before);
        }
        {auto sd=patron(1,MATERIALS[0],10,0);sd->status.inventory_slots=3;put(1,VOUCHER,29990);put(2,MATERIALS[1],1);seed_quest(QUESTS[0],1);weight();reset_observations();FamilySnapshot before;flow(npc,{1});committed(before,1,0,10,29990,100);check(count(VOUCHER)==30000,"exact maximum voucher stack succeeds");}
        {auto sd=patron(1,MATERIALS[0],10,0);sd->status.inventory_slots=3;put(1,VOUCHER,29991);put(2,MATERIALS[1],1);seed_quest(QUESTS[0],1);weight();reset_observations();FamilySnapshot before;flow(npc,{1});refused(before);}
        for(bool free_slot:{false,true}){auto sd=patron(1,MATERIALS[0],10,0);sd->status.inventory_slots=free_slot?3:2;put(1,MATERIALS[1],1);seed_quest(QUESTS[0],1);weight();reset_observations();FamilySnapshot before;flow(npc,{1});if(free_slot)committed(before,1,0,10,0,100);else refused(before);}
    }else{
        // Genuine current source: a delivery resumed after the key changes
        // records the old key, then permits another post-reset completion.
        {auto sd=patron(1,MATERIALS[0],10,0);add_plain(MATERIALS[1],10);seed_quest(QUESTS[0],1);reset_observations();
            flow(npc,{1},[&](int,int selected){if(!selected)setnum("$@FAMILY_TEST_KEY",200);});
            check(persistent("EP21_Supply_Daily")==100&&quest_state(QUESTS[0])==2&&count(VOUCHER)==10&&rep(1)==100,"current source reproduces stale-key first commit");
            check(!marker_values.empty()&&std::get<0>(marker_values.back())&&attached->qi_display[0].is_active,"current stale marker remains visibly active");
            reset_observations();flow(npc,{2});check(quest_state(QUESTS[1])==1&&persistent("EP21_SupplyFamily")==2,"current stale key permits second family acceptance");
            reset_observations();flow(npc,{1});
            check(persistent("EP21_Supply_Daily")==200&&count(VOUCHER)==20&&rep(1)==100&&rep(2)==100,"current source reproduces two post-reset completions");
            check(quest_state(QUESTS[0])==2&&quest_state(QUESTS[1])==2,"both current-source family quests complete");
        }
        // With no key crossing, completequest refreshes too early.  Only a
        // manual later refresh observes the final markers and hides the icon.
        {auto sd=patron(1,MATERIALS[0],10,0);seed_quest(QUESTS[0],1);reset_observations();flow(npc,{1});
            check(persistent("EP21_Supply_Daily")==100&&!marker_values.empty()&&std::get<0>(marker_values.back()),"current completequest marker evaluates before daily write");
            check(attached->qi_display[0].is_active,"current marker remains active after successful delivery");
            unsigned prior=marker_checks;pc_show_questinfo(attached);check(marker_checks==prior+1&&!std::get<0>(marker_values.back())&&!attached->qi_display[0].is_active,"manual refresh exposes missing final refresh");
        }
        // Actual quest_add rejects both completed and already-active rows while
        // the current script still persists a contract and claims registration.
        for(int state:{1,2}){auto sd=patron();seed_quest(QUESTS[0],state);reset_observations();
            flow(npc,{1},{},true);check(errors>0&&quest_state(QUESTS[0])==state&&persistent("EP21_SupplyFamily")==1,"current duplicate setquest error and misleading persisted contract reproduced");
            reset_observations();
        }
    }

    check(nodes[NPC]->qi_data.size()==1&&::map[0].qi_npc==std::vector<int32>{NPC},"all ordinary clicks retain one native marker registration");
    attached=nullptr;script_free_code(npc);if(capacity)script_free_code(capacity);::map[0].qi_npc.clear();nodes.clear();fake_nd=nullptr;
    quest_db.clear();reputation_db.clear();item_db.clear();nums.clear();strings.clear();do_final_script();
    ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf(candidate?"FAMILY_CANDIDATE_OK cases=%u assertions=%u marker_checks=%u expected_current_errors=%u\n":"FAMILY_CURRENT_DEFECTS_OK cases=%u assertions=%u marker_checks=%u expected_current_errors=%u\n",cases,assertions,marker_checks,expected_current_errors);
    return 0;
}
