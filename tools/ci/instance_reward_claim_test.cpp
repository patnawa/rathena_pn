// GPL-3.0-or-later. Real reward NPC bodies, VM, quest and inventory mutation.
#include "map/instance.hpp"
#include "map/npc.hpp"
#include "map/quest.hpp"
script_data* push_val2(script_stack*,c_op,int64,reg_db*);
script_data* push_str(script_stack*,c_op,char*);
struct script_function { int32(*func)(script_state*); const char* name; const char* arg; const char* deprecated; };
extern script_function buildin_func[];
npc_data fixture_npc{};
npc_data* instance_fixture_npc(int32 id){return id==NPC?&fixture_npc:nullptr;}
bool bio=false;
unsigned grant_calls=0, fail_at=0;
std::function<void()> grant_hook;
extern "C" block_list* block_lookup(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* block_lookup(int32 id){if(id==NPC)return &fixture_npc;return attached&&attached->id==id?attached:nullptr;}
extern "C" bool persist(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool persist(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}
extern "C" e_additem_result real_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__real__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result controlled_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__wrap__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result controlled_add(map_session_data* sd,item* it,int32 amount,e_log_pick_type type,bool favorite){
    if(++grant_calls==fail_at)return ADDITEM_OVERWEIGHT;
    const auto result=real_add(sd,it,amount,type,favorite);
    if(result==ADDITEM_SUCCESS&&grant_hook){auto hook=std::move(grant_hook);grant_hook={};hook();}
    return result;
}
extern "C" void qadd(const map_session_data*,const struct quest*) asm("__wrap__Z14clif_quest_addPK16map_session_dataPK5quest");extern "C" void qadd(const map_session_data*,const struct quest*){}
extern "C" void qdel(const map_session_data*,int32) asm("__wrap__Z17clif_quest_deletePK16map_session_datai");extern "C" void qdel(const map_session_data*,int32){}
extern "C" void qstatus(const map_session_data*,int32,bool) asm("__wrap__Z24clif_quest_update_statusPK16map_session_dataib");extern "C" void qstatus(const map_session_data*,int32,bool){}
extern "C" void qobjective(const map_session_data*,const struct quest*) asm("__wrap__Z27clif_quest_update_objectivePK16map_session_dataPK5quest");extern "C" void qobjective(const map_session_data*,const struct quest*){}
#include "weapons.inc"
namespace {
script_code *alice_code=nullptr,*bio_code=nullptr;
int32 world(script_state* st){
    const std::string name=script_getfuncname(st);
    if(name=="strcharinfo")push_str(st->stack,C_CONSTSTR,const_cast<char*>(bio?"1@gol2":"1@alice_mad"));
    else if(name=="instance_mapname")push_str(st->stack,C_CONSTSTR,const_cast<char*>(script_getstr(st,2)));
    else if(name=="gettimetick")push_val2(st->stack,C_INT,1900000000,nullptr);
    else check(false,"only declared world boundaries");
    return SCRIPT_CMD_SUCCESS;
}
int64 iv(const char* name,int index=0){return i64db_i64get(instances[1]->regs.vars,(static_cast<int64>(index)<<32)|add_str(name));}
void iv(const char* name,int index,int64 value){i64db_i64put(instances[1]->regs.vars,(static_cast<int64>(index)<<32)|add_str(name),value);}
void run(const std::string& text){auto* code=compile(text,"fixture-script");run_script(code,0,attached->id,NPC);check(!attached->st,"fixture does not suspend");script_free_code(code);}
void collect(){
    auto* parent=attached->st;
    run_script(bio?bio_code:alice_code,0,attached->id,NPC);
    while(attached->st!=parent){check(attached->st&&attached->st->state==CLOSE,"reward suspends only at final close");attached->st->state=END;run_script_main(attached->st);}
}
struct Bundle { std::vector<int> ids,amounts; };
Bundle bundle(int mode,int fast=0,int weapon=21051){return bio?(mode==1?Bundle{{25787},{2}}:Bundle{{25786,25787,102571,weapon},{9,15,1,1}}):(mode==1?Bundle{{1001074},{1}}:Bundle{{1001074,1001082},{1,1+fast}});}
std::unique_ptr<map_session_data> setup(bool is_bio,int mode,int slots=MAX_INVENTORY,int fast=0,int weapon=21051){
    ++cases;bio=is_bio;nums.clear();strings.clear();messages.clear();menu_text.clear();grant_calls=fail_at=0;grant_hook={};
    if(instances.count(1)){auto& regs=instances[1]->regs;if(regs.arrays)regs.arrays->destroy(regs.arrays,script_free_array_db);regs.vars->destroy(regs.vars,nullptr);instances.clear();}
    instances[1]=std::make_shared<s_instance_data>();instances[1]->state=INSTANCE_BUSY;instances[1]->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;
    sd->status.char_id=99000002;sd->status.party_id=17;sd->type=BL_PC;sd->status.inventory_slots=slots;
    sd->max_weight=1000000;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    if(bio){run("{ 'bio_zone=9; 'bio_party=17; 'bio_boss_started=1; 'bio_eligible[getcharid(0)]=1; setarray 'bio_roster[0],getcharid(0); end; }");iv("'bio_mode",0,mode);iv("'bio_weapon",sd->status.char_id,weapon);}
    else{run("{ 'alice_complete=1; 'alice_party=17; 'alice_eligible[getcharid(0)]=1; setarray 'alice_roster[0],getcharid(0); end; }");iv("'alice_mode",0,mode);iv("'alice_fast_clear",0,fast);}
    setnum("PNWeeklyActive",1);
    if(bio)check(quest_add(sd.get(),mode==1?16399:16400)==0,"real admission quest added");
    return sd;
}
void release(std::unique_ptr<map_session_data>& sd){
    check(!sd->st,"no leaked VM suspension");if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);
    if(sd->quest_log)aFree(sd->quest_log);sd.reset();attached=nullptr;
}
bool claimed(){return iv(bio?"'bio_claimed":"'alice_claimed",attached->status.char_id)!=0;}
int points(){return nums[add_str(bio?"PNRewardBioPoints":"PNRewardAlicePoints")];}
void fill(int slots,const Bundle& b,int variant,int free=0){
    for(int i=0;i<slots-free;++i)put(i,501,1);
    int rows=slots-free;
    for(size_t i=0;i<b.ids.size();++i){put(i,b.ids[i],1);auto& it=attached->inventory.u.items_inventory[i];
        if(variant==1)it.bound=1;if(variant==2)it.expire_time=2100000000;if(variant==3)it.unique_id=12345;
        if(variant>=4&&variant<=7)it.card[variant-4]=12345;}
    attached->inventory.amount=rows;weight();
}
void delivered(const Bundle& b,int existing=0){for(size_t i=0;i<b.ids.size();++i){
    if(count(b.ids[i])!=b.amounts[i]+existing)std::fprintf(stderr,"INSTANCE REWARD MISMATCH item=%d actual=%d expected=%d grant_calls=%u claimed=%d\n",b.ids[i],count(b.ids[i]),b.amounts[i]+existing,grant_calls,claimed());
    check(count(b.ids[i])==b.amounts[i]+existing,"exact reward identity and amount delivered");}}
void finished(const Bundle& b,int mode,int existing=0){
    delivered(b,existing);check(claimed(),"claim marker follows entire delivery");
    check(points()==(!bio||mode==2?1:0),"real certainty credit once only after full delivery");
    if(bio){check(quest_check(attached,mode==1?16399:16400,HAVEQUEST)==2,"actual mode quest completed");check(quest_check(attached,16392,HAVEQUEST)==2,"actual story continuity quest completed");}
    collect();delivered(b,existing);check(points()==(!bio||mode==2?1:0),"repeat cannot add certainty credit");
    check(errors==0,"no unexpected script diagnostics");
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"fixture path and optional route");deny_network();static char name[]="instance-reward-claim-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();
    unsigned found=0;for(int i=0;buildin_func[i].func;++i)if(std::string(buildin_func[i].name)=="instance_mapname"||std::string(buildin_func[i].name)=="strcharinfo"||std::string(buildin_func[i].name)=="gettimetick"){buildin_func[i].func=world;++found;}
    check(found==3,"three explicit world boundaries installed");do_init_script();battle_set_defaults();save_settings=0;
    fixture_npc.id=NPC;fixture_npc.type=BL_NPC;fixture_npc.instance_id=1;
    const std::string dir=argv[1];auto data=read(dir+"/items.yml");auto table=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto row:table["Body"])check(item_db.parseBodyNode(row)==1,"effective output item metadata parses");
    data=read(dir+"/groups.yml");table=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto row:table["Body"])check(itemdb_group.parseBodyNode(row)==1,"actual Bioresearch random weapon group parses");
    for(int id:{16399,16400,16392}){auto entry=std::make_shared<s_quest_db>();entry->id=id;quest_db.put(id,entry);}
    const auto functions=read(dir+"/functions.txt");
    for(const char* fn:{"PN_WeeklyReset","PN_WeeklyBossComplete","PN_RewardClearCredit","PN_ClearRewardCapacity"}){
        std::string marker="function\tscript\t"+std::string(fn)+"\t";
        if(functions.find(marker)!=std::string::npos)strdb_put(script_get_userfunc_db(),fn,compile(body(functions,marker),fn));
    }
    alice_code=compile(body(read(dir+"/Alice.txt"),"script\tAlice#mad_reward\t"),"actual-Alice-reward");
    bio_code=compile(body(read(dir+"/Bio.txt"),"script\tSierra#bio_reward\t"),"actual-Sierra-reward");
    for(bool is_bio:{false,true}){
        if(argc==3&&std::string(argv[2])!=(is_bio?"bio":"alice"))continue;
        for(int mode:{1,2})for(int slots:{100,MAX_INVENTORY})for(int variant=1;variant<=7;++variant){
            auto sd=setup(is_bio,mode,slots,1);auto b=bundle(mode,1);fill(slots,b,variant);collect();
            check(!claimed(),"full incompatible inventory cannot consume reward claim");check(points()==0,"blocked delivery cannot award clear credit");
            for(int id:b.ids)check(count(id)==1,"blocked complete bundle adds no items");
            for(size_t i=0;i<b.ids.size();++i){sd->inventory.u.items_inventory[i]={};sd->inventory_data[i]=nullptr;--sd->inventory.amount;}weight();
            collect();finished(b,mode);release(sd);
        }
    }
    for(bool is_bio:{false,true})for(int mode:{1,2}){
        auto sd=setup(is_bio,mode);auto b=bundle(mode);collect();finished(b,mode);release(sd);
        for(unsigned position=1;position<=b.ids.size();++position){
            b=bundle(mode);
            sd=setup(is_bio,mode);fail_at=position;const unsigned old_errors=errors;collect();
            check(errors==old_errors+1,"one deliberate native grant refusal");errors=old_errors;fail_at=0;
            check(!claimed()&&points()==0,"partial native refusal cannot finalize claim or credit");
            if(is_bio)check(quest_check(attached,mode==1?16399:16400,HAVEQUEST)==Q_ACTIVE,"failed batch cannot complete actual admission quest");
            if(position==2){
                // Moving/using the first delivered reward cannot make a retry
                // grant it again. Use the actual native inventory deletion.
                check(pc_delitem(sd.get(),0,b.amounts[0],0,0,LOG_TYPE_SCRIPT)==0,"previously delivered output consumed before retry");
                b.amounts[0]=0;
            }
            collect();finished(b,mode);release(sd);
        }
        for(int denied=0;denied<3;++denied){sd=setup(is_bio,mode);if(denied==0)sd->status.party_id=18;
            if(denied==1)iv(is_bio?"'bio_eligible":"'alice_eligible",sd->status.char_id,0);
            if(denied==2)iv(is_bio?"'bio_zone":"'alice_complete",0,0);
            collect();check(!claimed()&&grant_calls==0&&points()==0,"wrong party/nonparticipant/incomplete encounter gets no reward");release(sd);}
    }
    for(int weapon:WEAPONS){auto sd=setup(true,2,MAX_INVENTORY,0,weapon);auto b=bundle(2,0,weapon);collect();finished(b,2);release(sd);}
    {auto sd=setup(true,2);iv("'bio_weapon",sd->status.char_id,0);sd->max_weight=0;collect();const int weapon=iv("'bio_weapon",sd->status.char_id);
        check(std::find(std::begin(WEAPONS),std::end(WEAPONS),weapon)!=std::end(WEAPONS),"native random group selects a supported weapon");
        check(!claimed(),"weight blocks grant after caching random weapon");sd->max_weight=1000000;collect();finished(bundle(2,0,weapon),2);
        check(iv("'bio_weapon",sd->status.char_id)==weapon,"inventory retry preserves actual random group roll");release(sd);}
    {auto sd=setup(false,2);auto b=bundle(2);put(0,1001074,1);sd->inventory.u.items_inventory[0].bound=1;put(1,1001082,1);sd->inventory.amount=2;sd->status.inventory_slots=2;weight();collect();
        check(!claimed()&&count(1001074)==1&&count(1001082)==1,"one blocked material blocks the entire preflight batch");release(sd);}
    for(bool is_bio:{false,true}){auto sd=setup(is_bio,2);auto b=bundle(2);
        grant_hook=[](){auto* parent=attached->st;const unsigned before=grant_calls;collect();
            check(attached->st==parent&&grant_calls==before,"synchronous reentry restores parent and grants nothing");};
        collect();finished(b,2);check(grant_calls==b.ids.size(),"reentrant reward event cannot duplicate any item");release(sd);}
    for(bool is_bio:{false,true})for(int mode:{1,2}){
        auto sd=setup(is_bio,mode);auto b=bundle(mode);fill(MAX_INVENTORY,b,0,is_bio&&mode==2?1:0);collect();finished(b,mode,1);release(sd);
    }
    for(bool is_bio:{false,true}){auto sd=setup(is_bio,2);auto b=bundle(2);fill(MAX_INVENTORY,b,0,1);
        sd->inventory.u.items_inventory[0].amount=30000;weight();collect();
        check(!claimed()&&grant_calls==0,"terminal compatible stack overflow blocks complete batch");release(sd);}
    for(bool is_bio:{false,true})for(int over:{0,1}){auto sd=setup(is_bio,2);auto b=bundle(2);int w=0;for(size_t i=0;i<b.ids.size();++i)w+=item_db.find(b.ids[i])->weight*b.amounts[i];sd->max_weight=w-over;collect();
        if(over)check(!claimed()&&grant_calls==0,"one-unit overweight blocks complete batch");else finished(b,2);release(sd);}
    check(errors==0,"no unexpected diagnostics");script_free_code(alice_code);script_free_code(bio_code);
    auto& regs=instances[1]->regs;if(regs.arrays)regs.arrays->destroy(regs.arrays,script_free_array_db);regs.vars->destroy(regs.vars,nullptr);instances.clear();
    itemdb_group.clear();item_db.clear();quest_db.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("INSTANCE_REWARD_NATIVE_OK cases=%u assertions=%u expected_refusals=8 weapons=39\n",cases,assertions);return 0;
}
