// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  finalbattle_reward_capacity_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/finalbattle_reward_capacity_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to the existing crown test's boundary utilities.
// The only removed prefix wrapper is map_id2nd, replaced by the instance-aware
// world lookup below. Actual production VM/helper/grants/achievements stay native.
#include "map/instance.hpp"
#include "map/npc.hpp"

namespace {
npc_data crystal_npc{};
unsigned achievement_packets=0, argument_writes=0;
const int output_ids[]={103512,1001653,1001654,1001655,1001656,1001657,1001658,
    1001659,1001660,1001661,1001662,1001663,1001664,1001480,1001034,1001035,
    1001036,1001037,1000812,1000813,1000814};
const char* claim_keys[]={"EP21_FB_Crystal_Daily","EP21_FB_Crystal_RollKey",
    "EP21_FB_Crystal_RollMode","EP21_FB_Crystal_ItemMask","EP21_FB_Crystal_ExtraMask",
    "EP21_FB_Crystal_BoxCount"};

struct PlayerDeleter {
    void operator()(map_session_data* sd)const {
        check(!sd->st,"no abandoned actual script");
        if(sd->achievement_data.achievements)aFree(sd->achievement_data.achievements);
        sd->achievement_data.achievements=nullptr;
        delete sd;
    }
};
using Player=std::unique_ptr<map_session_data,PlayerDeleter>;
Player recipient(){
    ++cases;errors=0;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    closes=unequips=equips=achievement_packets=argument_writes=0;
    Player sd(new map_session_data);attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->fd=0;sd->status.base_level=275;sd->status.zeny=1234567;
    sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=2000000000;
    sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->m=0;sd->instance_id=1;
    for(auto& i:sd->equip_index)i=-1;
    for(auto& i:sd->equip_switch_index)i=-1;
    return sd;
}
void stage(int value){i64db_i64put(instances.at(1)->regs.vars,add_str("'fb_stage"),value);}
std::vector<int64> claims(){std::vector<int64> result;for(auto k:claim_keys)result.push_back(nums[add_str(k)]);return result;}
void finish(script_code* code,bool old=false){
    run_script(code,0,attached->id,NPC);
    unsigned pauses=0;
    while(attached->st){
        check(++pauses==1,"crystal has only terminal close suspension");
        auto* st=attached->st;check(st->state==CLOSE,"no intermediate suspension in crystal grant");
        st->state=END;run_script_main(st);
    }
    if(!old)check(errors==0,"no candidate script errors");
    check(windows.empty()&&unequips==0&&equips==0,"grant never equips or opens services");
    check(attached->status.zeny==1234567,"crystal does not alter Zeny");
}
int64 daily_key(){
    auto* c=compile("{ @FB_Key=callfunc(\"EP21_DailyKey\"); end; }","actual-daily-key");
    run_script(c,0,attached->id,NPC);check(!attached->st&&errors==0,"actual daily helper completes");
    script_free_code(c);return nums[add_str("@FB_Key")];
}
void saved(int mode,int mask,int extras=0,int diamonds=0){
    stage(20);setnum(claim_keys[0],0);setnum(claim_keys[1],daily_key());setnum(claim_keys[2],mode);
    setnum(claim_keys[3],mask);setnum(claim_keys[4],extras);setnum(claim_keys[5],diamonds);
}
void variant(int index,int id,int amount,int kind){
    put(index,id,amount);auto& it=attached->inventory.u.items_inventory[index];
    if(kind==1)it.bound=BOUND_ACCOUNT;
    else if(kind==2)it.unique_id=UINT64_MAX;
    else if(kind>=3&&kind<=6)it.card[kind-3]=4700;
    else if(kind==7)it.expire_time=2100000000;
    else if(kind==8){it.id=123;it.identify=0;it.refine=12;it.attribute=1;it.enchantgrade=3;
        it.favorite=1;it.option[0].id=1;it.option[0].value=23;it.option[0].param=1;}
    weight();
}
// Exact production array-reference API, including nonzero starting offsets.
// Check caller arrays in the VM after helper return; no Python capacity model.
bool helper(const std::vector<int>& ids,const std::vector<int>& amounts,int count_arg=-999,int offset=0){
    check(ids.size()==amounts.size(),"fixture paired arrays");
    std::ostringstream text;text<<"{ setarray .@reward_item[0],-777; setarray .@reward_amount[0],-888; ";
    if(!ids.empty()){
        text<<"setarray .@reward_item["<<offset<<"],";
        for(size_t i=0;i<ids.size();++i)text<<(i?",":"")<<ids[i];text<<"; setarray .@reward_amount["<<offset<<"],";
        for(size_t i=0;i<amounts.size();++i)text<<(i?",":"")<<amounts[i];text<<"; ";
    }
    text<<"@FB_Result=callfunc(\"EP21_FB_CheckPlainBatch\",.@reward_item["<<offset<<"],.@reward_amount["<<offset<<"],"
        <<(count_arg==-999?static_cast<int>(ids.size()):count_arg)<<"); @FB_ArraysOK=1; ";
    for(size_t i=0;i<ids.size();++i)text<<"if (.@reward_item["<<offset+i<<"] != "<<ids[i]<<" || .@reward_amount["<<offset+i<<"] != "<<amounts[i]<<") @FB_ArraysOK=0; ";
    if(offset)text<<"if (.@reward_item[0] != -777 || .@reward_amount[0] != -888) @FB_ArraysOK=0; ";
    text<<"end; }";
    auto* c=compile(text.str(),"actual-array-reference-helper");Snapshot before;
    const auto before_claims=claims();run_script(c,0,attached->id,NPC);
    check(!attached->st&&errors==0,"helper terminates synchronously");
    before.unchanged();check(claims()==before_claims,"preflight never changes saved/daily claims");
    check(nums[add_str("@FB_ArraysOK")]==1,"caller arrays and offset prefix preserved");
    bool result=nums[add_str("@FB_Result")]!=0;script_free_code(c);return result;
}
void awarded(const Snapshot& before,const std::map<int,int>& rewards,int64 key){
    int delta_weight=0;
    for(auto [id,quantity]:rewards){int old=0;for(auto& it:before.inventory.u.items_inventory)if(it.nameid==id)old+=it.amount;
        check(count(id)==old+quantity,"exact aggregate crystal reward");delta_weight+=item_db.find(id)->weight*quantity;}
    for(int i=0;i<MAX_INVENTORY;++i){const auto& old=before.inventory.u.items_inventory[i];const auto& now=attached->inventory.u.items_inventory[i];
        if(old.nameid){item expected=old;if(rewards.count(old.nameid))expected.amount=now.amount;
            check(std::memcmp(&expected,&now,sizeof(item))==0,"all retained item metadata preserved");}
        else if(now.nameid){item expected{};expected.nameid=now.nameid;expected.identify=1;expected.amount=now.amount;
            check(rewards.count(now.nameid)&&std::memcmp(&expected,&now,sizeof(item))==0,"only exact plain output in new cell");}
        check(now.nameid?(attached->inventory_data[i]&&attached->inventory_data[i]->nameid==now.nameid):!attached->inventory_data[i],"native inventory cache coherent");
    }
    check(attached->weight==before.weight+delta_weight,"exact summed output weight");
    check(nums[add_str(claim_keys[0])]==key,"daily claim consumed once");
    for(int i=1;i<6;++i)check(nums[add_str(claim_keys[i])]==0,"saved roll cleared only after successful batch");
}
void grant_vector(const std::vector<int>& ids,const std::vector<int>& amounts){
    std::ostringstream s;s<<"{ ";for(size_t i=0;i<ids.size();++i)s<<"getitem "<<ids[i]<<","<<amounts[i]<<"; ";s<<"end; }";
    auto* c=compile(s.str(),"actual-sequential-plain-grants");run_script(c,0,attached->id,NPC);
    check(!attached->st&&errors==0,"helper-approved exact grants succeed");script_free_code(c);
}
}

extern "C" npc_data* crystal_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* crystal_lookup(int32 id){return id==NPC?&crystal_npc:fake_nd&&fake_nd->id==id?fake_nd:nullptr;}
extern "C" block_list* crystal_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* crystal_world(int32 id){if(attached&&attached->id==id)return attached;return crystal_lookup(id);}
extern "C" map_session_data* crystal_character(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* crystal_character(int32 id){return attached&&attached->status.char_id==id?attached:nullptr;}
extern "C" bool crystal_registry(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool crystal_registry(map_session_data* sd,int64 key,int64 value){
    const std::string name=get_str(script_getvarid(key));check(sd==attached,"registry attached to correct player");
    check(name=="ARG0"||name.rfind("EP21_FB_Crystal_",0)==0,"only exact achievement/crystal persistent variables");
    nums[key]=value;if(name=="ARG0")++argument_writes;return true;
}
extern "C" void achievement_packet(map_session_data*,const struct achievement*,int32) asm("__wrap__Z23clif_achievement_updateP16map_session_dataPK11achievementi");
extern "C" void achievement_packet(map_session_data* sd,const struct achievement*,int32){check(sd==attached,"achievement packet owner");++achievement_packets;}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==3,"explicit artifact directory and isolated mode");
    const bool old=std::string(argv[2])=="original";check(old||std::string(argv[2])=="candidate","known mode");
    deny_network();
    for(long call:{SYS_connect,SYS_bind,SYS_listen})
        check(syscall(call,-1,nullptr,0)==-1&&errno==EPERM,"connect/bind/listen actively denied");
    static char server[]="finalbattle-reward-capacity-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    script_config_read("conf/script_athena.conf");
    check(script_config.check_cmdcount==655360&&script_config.check_gotocount==2048,"actual configured command/jump budgets, no freeloop");
    battle_config.atcommand_disable_npc=0;battle_config.feature_achievement=1;
    std::strcpy(::map[0].name,"1@ep21b");::map[0].instance_id=1;
    check(::map[0].qi_npc.empty(),"actual instance map has no quest-info NPCs");
    crystal_npc.id=NPC;crystal_npc.type=BL_NPC;crystal_npc.m=0;crystal_npc.instance_id=1;
    auto fake=std::make_unique<npc_data>();fake->id=NPC+1;fake_nd=fake.get();
    instances[1]=std::make_shared<s_instance_data>();instances[1]->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    const std::string dir=argv[1];
    auto text=read(dir+"/items.yml");auto rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:rows["Body"])check(item_db.parseBodyNode(row)==1,"actual item definition parses");
    check(item_db.size()==22&&item_db.find(644),"exact21 outputs plus actual deferred Gift_Box metadata");
    item_db.loadingFinished();
    check(item_db.size()==23&&item_db.find(ITEMID_DUMMY),"actual finalization supplies native dummy and derived prices");
    for(int id:output_ids){auto d=item_db.find(id);check(d&&itemdb_isstackable2(d.get())&&!d->flag.guid&&!d->flag.autoequip&&!d->stack.inventory,"actual native plain output contract");}
    check(item_db.find(1001480)->value_sell==300000,"actual Golden Diamond derived Sell value");
    text=read(dir+"/achievements.yml");rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:rows["Body"])check(achievement_db.parseBodyNode(row)==1,"actual relevant achievement parses");
    check(achievement_db.size()==27,"all7Get_Item and20Goal_Achieve actual records");
    text=read(dir+"/levels.yml");rows=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto row:rows["Body"])check(achievement_level_db.parseBodyNode(row)==1,"actual achievement level parses");
    auto* daily=compile(read(dir+"/daily.txt"),"actual-EP21-DailyKey");strdb_put(script_get_userfunc_db(),"EP21_DailyKey",daily);
    if(!old){auto* c=compile(read(dir+"/helper.txt"),"actual-plain-batch-helper");strdb_put(script_get_userfunc_db(),"EP21_FB_CheckPlainBatch",c);}
    auto* normal=compile(read(dir+(old?"/normal-before.txt":"/normal-after.txt")),old?"original-normal-crystal":"actual-normal-crystal");
    auto* hard=compile(read(dir+(old?"/hard-before.txt":"/hard-after.txt")),old?"original-hard-crystal":"actual-hard-crystal");
    if(old){
        {auto sd=recipient();saved(1,1);variant(0,1001653,1,1);sd->status.inventory_slots=1;Snapshot before;
            finish(normal,true);check(errors==1,"original Normal exact grant failure");before.unchanged();
            check(nums[add_str(claim_keys[0])]!=0&&nums[add_str(claim_keys[1])]==0,"original loses daily and pending roll despite no delivery");}
        {auto sd=recipient();saved(2,1);put(0,103512,1);variant(1,1001653,1,1);sd->status.inventory_slots=2;
            finish(hard,true);check(errors==1&&count(103512)==2&&count(1001653)==1,"original Hard partial delivery then failure");
            check(nums[add_str(claim_keys[0])]!=0&&nums[add_str(claim_keys[1])]==0,"original partial delivery cannot retry");}
        {auto sd=recipient();saved(1,1);variant(0,1001653,1,1);variant(1,1001653,30000,0);
            finish(normal,true);check(errors==1&&count(1001653)==30001,"original first-compatible cap failure despite empty slots");
            check(nums[add_str(claim_keys[0])]!=0&&nums[add_str(claim_keys[1])]==0,"original cap failure consumes claim");}
    }else{
        for(int id:output_ids)for(int kind=0;kind<=8;++kind)for(int room=0;room<2;++room){
            auto sd=recipient();variant(0,id,10,kind);sd->status.inventory_slots=1+room;
            bool expected=room||kind==0||kind==8;check(helper({id},{1})==expected,"every output and matching field capacity");
            if(expected){int before=count(id);grant_vector({id},{1});check(count(id)==before+1,"actual grant matches helper acceptance");}
        }
        for(int id:output_ids){
            {auto sd=recipient();variant(0,id,30000,1);variant(1,id,100,0);sd->status.inventory_slots=2;
                check(helper({id},{1}),"bound-first full but plain-later room accepted");grant_vector({id},{1});check(count(id)==30101,"plain-later exact grant");}
            {auto sd=recipient();variant(0,id,30000,0);variant(1,id,10,0);check(!helper({id},{1}),"first compatible full is terminal despite later or new room");}
            {auto sd=recipient();variant(3,id,100,0);sd->status.inventory_slots=2;check(!helper({id},{1}),"first compatible outside reduced limit rejected");}
            {auto sd=recipient();variant(3,id,100,1);sd->status.inventory_slots=2;check(helper({id},{1}),"incompatible outside limit leaves native fresh slot available");grant_vector({id},{1});}
            {auto sd=recipient();variant(0,id,29999,0);sd->status.inventory_slots=1;check(helper({id},{1}),"exact native30000 boundary");grant_vector({id},{1});check(!helper({id},{1}),"one beyond native cap");}
        }
        {auto sd=recipient();check(helper({1001653,1001653},{10000,20000}),"duplicate IDs aggregate under cap");grant_vector({1001653,1001653},{10000,20000});check(count(1001653)==30000,"duplicate actual grants share one row");}
        {auto sd=recipient();sd->status.inventory_slots=1;check(helper({1001653,1001653},{1,2}),"duplicate IDs reserve one fresh slot");}
        {auto sd=recipient();check(!helper({1001653,1001653},{30000,1}),"duplicate aggregate cap rejected");}
        for(int amount:{0,-1,30001,65536,INT32_MAX}){auto sd=recipient();check(!helper({1001653},{amount}),"invalid amount rejected before narrowing");}
        for(int id:{0,501,1001652,1001665,INT32_MAX}){auto sd=recipient();check(!helper({id},{1}),"closed21identity purpose contract");}
        for(int count_arg:{-1,22,INT32_MAX}){auto sd=recipient();check(!helper({1001653},{1},count_arg),"invalid count rejected before copying");}
        {auto sd=recipient();sd->status.inventory_slots=0;sd->max_weight=0;check(helper({},{}),"empty roll ignores unavailable capacity");check(!helper({103512},{1}),"nonempty requires allowed slot");}
        {auto sd=recipient();sd->status.inventory_slots=MAX_INVENTORY+1;check(!helper({103512},{1}),"invalid excessive physical slot count rejected");}
        {auto sd=recipient();check(helper({1001653,1001654},{1,2},-999,3),"actual nonzero array-reference offsets");}
        for(int slots:{1,2,3}){auto sd=recipient();sd->status.inventory_slots=slots;check(helper({1001653,1001654,1001655},{1,1,1})==(slots==3),"cumulative unique fresh-slot reservations");}
        {auto sd=recipient();put(0,1000812,1);put(3,1000813,1);weight();sd->status.inventory_slots=4;
            check(helper({1001653,1001654},{1,1}),"sparse accessible slots counted fully");grant_vector({1001653,1001654},{1,1});check(!helper({1001655},{1}),"filled sparse slots cannot be reused");}
        for(int limit:{929,930}){auto sd=recipient();sd->max_weight=limit;check(helper({1001480,1001034},{3,1})==(limit==930),"aggregate exact weight threshold");}
        // Full physical inventory, all 21 outputs, with compatible cells only
        // at its end: exercise real command/jump guards rather than freeloop.
        for(int kind:{0,1,2}){
            auto sd=recipient();saved(2,4095,127,3);
            for(int i=0;i<MAX_INVENTORY-21;++i){
                if(kind==0)put(i,644,1);
                else variant(i,output_ids[i%21],10,kind==1?1:1+i%7);
            }
            for(int i=0;i<21;++i)put(MAX_INVENTORY-21+i,output_ids[i],10);
            weight();std::vector<int> ids(std::begin(output_ids),std::end(output_ids));
            check(helper(ids,std::vector<int>(21,1)),"full inventory helper fits actual jump budget");
            Snapshot before;auto key=claims()[1];std::map<int,int> rewards;
            for(int id:output_ids)rewards[id]=id==1001480?3:1;
            finish(hard);awarded(before,rewards,key);
        }
        {auto sd=recipient();saved(1,0);sd->status.inventory_slots=0;sd->max_weight=0;
            Snapshot before;auto key=claims()[1];finish(normal);awarded(before,{},key);}
        for(int mode:{1,2})for(int mask:{0,1,4095})for(int extra:{0,127})for(int diamonds:{0,1,2,3}){
            auto sd=recipient();saved(mode,mask,extra,diamonds);auto saved_state=claims();Snapshot before;
            std::map<int,int> expected;if(mode==2)expected[103512]=1;
            for(int i=0;i<12;++i)if(mask&(1<<i))expected[1001653+i]=1;
            if(diamonds)expected[1001480]=diamonds;
            const int extras[]={1001034,1001035,1001036,1001037,1000812,1000813,1000814};
            for(int i=0;i<7;++i)if(extra&(1<<i))expected[extras[i]]=1;
            finish(mode==1?normal:hard);awarded(before,expected,saved_state[1]);Snapshot after;
            finish(mode==1?normal:hard);after.unchanged();check(claims()[0]==saved_state[1],"successful per-character claim idempotent");
            if(diamonds){for(int id=220023;id<=220029;++id)check(achievement_check_progress(sd.get(),id,ACHIEVEINFO_COMPLETE)>0,"Golden Diamond completes each actual Get_Item condition");
                check(achievement_check_progress(sd.get(),240001,ACHIEVEINFO_COMPLETE)>0,"actual first Goal_Achieve completion reached recursively");
                check(achievement_packets>0&&sd->achievement_data.level>0,"actual completion packets and level calculation reached");}
        }
        for(int mode:{1,2}){
            auto sd=recipient();saved(mode,1);if(mode==2)put(1,103512,1);variant(0,1001653,1,1);sd->status.inventory_slots=mode;
            auto preserved=claims();Snapshot before;finish(mode==1?normal:hard);before.unchanged();check(claims()==preserved,"failed batch retains all saved claim state");
            sd->status.inventory_slots=mode+1;Snapshot retry;finish(mode==1?normal:hard);
            std::map<int,int> expected{{1001653,1}};if(mode==2)expected[103512]=1;awarded(retry,expected,preserved[1]);
        }
        // Force no-capacity rejection while the actual RNG builds a saved roll;
        // a second conversation must reuse it instead of advancing the RNG.
        {auto sd=recipient();stage(20);sd->status.inventory_slots=0;seed_roll(0,100000);finish(hard);auto roll=claims();auto rng=generator;
            check(roll[0]==0&&roll[1]!=0&&roll[2]==2,"new rejected roll persisted before capacity");finish(hard);check(claims()==roll&&generator==rng,"retry does not reroll saved crystal");}
        for(int mode:{1,2}){auto sd=recipient();saved(mode,1);stage(19);Snapshot before;auto prior=claims();finish(mode==1?normal:hard);before.unchanged();check(claims()==prior,"premature shared stage grants nothing");}
        // Explicit actual Goal_Achieve condition evaluation at every current
        // threshold; completion recursion is separately reached above.
        {auto sd=recipient();Snapshot before;for(int level=0;level<=21;++level){sd->achievement_data.level=level;
            for(int threshold=1;threshold<=20;++threshold){auto a=achievement_db.find(240000+threshold);
                check(achievement_check_condition(a->condition,sd.get())==(level>=threshold),"all actual Goal_Achieve condition boundaries");}}
            before.unchanged();}
    }
    attached=nullptr;script_free_code(normal);script_free_code(hard);
    script_free_vars(instances[1]->regs.vars);instances[1]->regs.vars=nullptr;instances.clear();
    item_db.clear();achievement_db.clear();achievement_level_db.clear();
    nums.clear();strings.clear();fake_nd=nullptr;fake.reset();do_final_script();timer_final();db_final();malloc_final();
    check(errors==(old?1u:0u),"no additional errors through complete cleanup");
    std::printf(old?"FINALBATTLE_OLD_FAILURES_OK cases=%u assertions=%u\n":"FINALBATTLE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);
    return 0;
}
