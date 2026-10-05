// GPL-3.0-or-later. Actual Chapter 1 functions, VM and native item delivery.
// The generator supplies explicit transport, registry and world boundaries.
extern "C" block_list* block_lookup(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* block_lookup(int32 id){return attached&&attached->id==id?attached:nullptr;}
extern "C" bool persist(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool persist(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}
bool fail_delivery=false;
extern "C" e_additem_result real_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__real__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result controlled_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__wrap__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result controlled_add(map_session_data* sd,item* it,int32 amount,e_log_pick_type type,bool favorite){
    if(fail_delivery)return ADDITEM_OVERWEIGHT;
    return real_add(sd,it,amount,type,favorite);
}
namespace {
const char* pending_name(int id){return id==1001972?"CH1_Pending_Coin":id==1001973?"CH1_Pending_Amulet":"CH1_Pending_Sample";}
int64 pending(int id){return nums[add_str(pending_name(id))];}
std::unique_ptr<map_session_data> reward_player(int slots=MAX_INVENTORY){
    ++cases;nums.clear();strings.clear();messages.clear();fail_delivery=false;
    auto sd=std::make_unique<map_session_data>();attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->status.inventory_slots=slots;sd->max_weight=1000000;
    sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    return sd;
}
void release(std::unique_ptr<map_session_data>& sd){
    check(!sd->st,"reward functions never suspend");
    if(sd->regs.arrays){sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);sd->regs.arrays=nullptr;}
    sd.reset();attached=nullptr;
}
void bag(int slots,int id,int amount,int variant){
    for(int i=0;i<slots;++i)put(i,501,1);
    put(0,id,amount);auto& it=attached->inventory.u.items_inventory[0];
    if(variant==1)it.bound=1;
    if(variant==2)it.expire_time=2100000000;
    if(variant==3)it.unique_id=12345;
    if(variant>=4&&variant<=7)it.card[variant-4]=12345;
    attached->inventory.amount=slots;weight();
}
void call(const std::string& expression){
    auto* code=compile("{ RewardTestReturn="+expression+"; end; }","reward-call");
    run_script(code,0,attached->id,NPC);check(!attached->st,"actual reward call completes synchronously");
    script_free_code(code);
}
void give(int id,int amount){call("callfunc(\"F_CH1_GiveReward\","+std::to_string(id)+","+std::to_string(amount)+")");}
void claim(){call("callfunc(\"F_CH1_ClaimRewards\",1)");}
int64 result(){return nums[add_str("RewardTestReturn")];}
void remove_row(int row){attached->inventory.u.items_inventory[row]={};attached->inventory_data[row]=nullptr;--attached->inventory.amount;weight();}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2,"fixture directory");deny_network();static char server[]="chapter1-reward-claim-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    const std::string dir=argv[1];auto data=read(dir+"/items.yml");auto table=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto row:table["Body"])check(item_db.parseBodyNode(row)==1,"actual reward metadata parses");
    const auto source=read(dir+"/RewardClaims.txt");
    // Register every actual helper, including capacity helpers added by the fix.
    for(const std::string name:{"F_CH1_InventoryState","F_CH1_CanDeliver","F_CH1_DeliverReward","F_CH1_GiveReward","F_CH1_ClaimRewards"}){
        const auto marker="function\tscript\t"+name+"\t";
        if(source.find(marker)!=std::string::npos)strdb_put(script_get_userfunc_db(),name.c_str(),compile(body(source,marker),name.c_str()));
    }
    for(int id:{1001972,1001973,1001974})for(int slots:{1,100,MAX_INVENTORY})for(int variant=0;variant<=7;++variant){
        auto sd=reward_player(slots);bag(slots,id,1,variant);
        give(id,10);check(result()==1,"earned reward accepted");
        check(count(id)+pending(id)==11,"full bag preserves every earned unit regardless of stack metadata");
        if(variant){check(count(id)==1&&pending(id)==10,"incompatible stack queues without native error");claim();check(count(id)==1&&pending(id)==10,"retry cannot clear undelivered reward");
            remove_row(0);claim();check(count(id)==10&&pending(id)==0,"freeing one slot delivers queued ordinary reward");}
        else check(count(id)==11&&pending(id)==0,"compatible full-bag stack delivers");
        const int delivered=count(id);claim();check(count(id)==delivered&&pending(id)==0,"repeat claim grants nothing");
        check(errors==0,"ordinary capacity failures produce no native error");release(sd);
    }
    for(int id:{1001972,1001973,1001974}){
        // A new plain stack is possible beside a bound stack.
        auto sd=reward_player(2);put(0,id,1);sd->inventory.u.items_inventory[0].bound=1;sd->inventory.amount=1;weight();
        give(id,10);check(count(id)==11&&pending(id)==0,"separate ordinary stack uses free slot");release(sd);
        // Native searches past incompatible rows; ID-only checkweight does not.
        sd=reward_player(2);bag(2,id,30000,1);put(1,id,1);weight();give(id,10);
        check(count(id)==30011&&pending(id)==0,"bound capped stack cannot hide a compatible ordinary stack");release(sd);
        // A capped first compatible stack is terminal, even with another stack.
        sd=reward_player(2);bag(2,id,29995,0);put(1,id,1);weight();give(id,10);
        check(count(id)==29996&&pending(id)==10,"first compatible stack overflow queues entire award");release(sd);
        sd=reward_player(1);put(1,id,1);sd->inventory.amount=1;weight();give(id,10);
        check(count(id)==1&&pending(id)==10,"compatible row outside current quota cannot accept reward");release(sd);
        sd=reward_player();give(id,30000);check(count(id)==30000&&pending(id)==0,"maximum native stack quantity delivers");release(sd);
        sd=reward_player();setnum(pending_name(id),30010);claim();check(count(id)==30000&&pending(id)==10,"oversized historical pending balance delivers bounded chunk");
        remove_row(0);claim();check(count(id)==10&&pending(id)==0,"remainder stays claimable");release(sd);
        // Simulate a native refusal after successful preflight. Queue survives.
        sd=reward_player();fail_delivery=true;const unsigned before=errors;give(id,10);
        check(errors==before+1&&count(id)==0&&pending(id)==10,"native failure preserves newly earned pending reward");
        errors=before;fail_delivery=false;claim();check(count(id)==10&&pending(id)==0,"retry after native refusal delivers once");claim();check(count(id)==10,"native-failure replay cannot duplicate");release(sd);
    }
    for(int amount:{-1,0,30001}){auto sd=reward_player();give(1001972,amount);check(result()==0&&count(1001972)==0&&pending(1001972)==0,"invalid reward amount cannot mutate inventory or queue");release(sd);}
    {auto sd=reward_player();give(501,1);check(result()==0&&count(501)==0,"unsupported output rejected even with free inventory");release(sd);}
    for(int extra:{0,1}){auto sd=reward_player();sd->max_weight=10-extra;give(1001972,10);check(count(1001972)==(extra?0:10)&&pending(1001972)==(extra?10:0),"exact weight boundary matches native grant");release(sd);}
    {auto sd=reward_player(2);setnum("CH1_Pending_Coin",10);setnum("CH1_Pending_Amulet",5);setnum("CH1_Pending_Sample",1);claim();
        check(count(1001972)==10&&count(1001973)==5&&pending(1001974)==1&&result()==2,"partial multi-type claim preserves blocked type");remove_row(0);claim();check(count(1001974)==1&&pending(1001974)==0,"remaining type resumes");release(sd);}
    for(int crowded:{0,1}){auto sd=reward_player();
        for(int i=0;i<MAX_INVENTORY;++i){put(i,crowded?1001972:501,1);if(crowded)sd->inventory.u.items_inventory[i].expire_time=2100000000+i;}
        sd->inventory.amount=MAX_INVENTORY;weight();setnum("CH1_Pending_Coin",10);setnum("CH1_Pending_Amulet",5);setnum("CH1_Pending_Sample",1);claim();
        check(pending(1001972)==10&&pending(1001973)==5&&pending(1001974)==1&&result()==0,"all three pending types survive full inventory within script budgets");
        check(errors==0,"full-bag multi-type claim stays within normal VM limits");release(sd);}
    check(errors==0,"no unexpected native diagnostics");item_db.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("CHAPTER1_REWARD_NATIVE_OK cases=%u assertions=%u expected_refusals=3\n",cases,assertions);return 0;
}
