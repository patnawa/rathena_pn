// GPL-3.0-or-later. Appended to the existing isolated crown VM fixture helpers.
// Production NPC bodies and their fashion mapping functions are parsed unchanged.
// Persistence, inventory debit and outbound UI are explicit fixture boundaries.
namespace {
bool reject_debit=false;
unsigned debit_calls=0;

std::unique_ptr<map_session_data> fashion_player() {
    ++cases; nums.clear(); strings.clear(); messages.clear(); menu_text.clear();
    closes=0; debit_calls=0; reject_debit=false;
    auto sd=std::make_unique<map_session_data>(); attached=sd.get();
    sd->id=sd->status.account_id=99000001; sd->status.char_id=99000002; sd->type=BL_PC;
    sd->status.zeny=7654321; sd->status.inventory_slots=MAX_INVENTORY;
    sd->fd=0; sd->state.ignoretimeout=true; sd->npc_idle_timer=INVALID_TIMER;
    for(auto& index:sd->equip_index) index=-1;
    setnum("#FP_Fashion",100);
    return sd;
}
bool says(const std::string& text) {
    for(const auto& message:messages) if(message.find(text)!=std::string::npos) return true;
    return false;
}
void finish_player() {
    check(!attached->st&&!attached->state.menu_or_input,"dialogue cleanly detaches");
    if(attached->regs.arrays) { attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db); attached->regs.arrays=nullptr; }
    attached=nullptr;
}
void item_fixture(int id,item_types type,uint32 location,const char* name) {
    auto data=std::make_shared<item_data>(); data->nameid=id; data->type=type;
    data->equip=location; data->ename=name;
    item_db.put(id,data);
}
}

extern "C" bool fashion_account(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool fashion_account(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}
extern "C" char fashion_debit(map_session_data*,int32,int32,int32,int16,e_log_pick_type) asm("__wrap__Z10pc_delitemP16map_session_dataiiis15e_log_pick_type");
extern "C" char fashion_debit(map_session_data* sd,int32 index,int32 amount,int32,int16,e_log_pick_type){
    ++debit_calls;
    check(sd==attached&&index==0&&amount==1,"costume debit names the selected inventory record");
    if(reject_debit) return 1;
    sd->inventory.u.items_inventory[index]={};sd->inventory_data[index]=nullptr;
    return 0;
}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"source path supplied");deny_network();static char server[]="npc-audit-fashion-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;
    // These minimal identities exercise menu construction; full item effects,
    // payment commit and native equipment mutation are outside this fixture.
    item_fixture(19961,IT_ARMOR,EQP_COSTUME_HEAD_TOP,"Test costume");
    item_fixture(19962,IT_ARMOR,EQP_COSTUME_GARMENT,"Test garment");
    for(int id:{4700,4710,4884,4807,310654}) item_fixture(id,IT_CARD,0,("Enchant "+std::to_string(id)).c_str());
    for(int id:{6636,6946,6644,6908,1000520}) item_fixture(id,IT_ETC,0,("Stone "+std::to_string(id)).c_str());
    const auto source=read(argv[1]);
    for(const char* name:{"FP_LoadBox","FP_StoneFromEnchant","FP_IsTombCostume"}) {
        auto* fn=compile(body(source,std::string("function\tscript\t")+name),name);
        strdb_put(script_get_userfunc_db(),name,fn);
    }
    auto* gold=compile(body(source,"\tGold Point Manager#FP\t"),"actual Gold Point Manager");
    auto* recover=compile(body(source,"\tGregio Grumani#FP\t"),"actual Gregio Grumani");
    auto* designer=compile(body(source,"\tFashion Designer#FP\t"),"actual Fashion Designer");

    for(int choice:{1,2,3,255}) {
        auto sd=fashion_player();Snapshot before;
        walk(gold,{choice});before.unchanged();
        check(menu_text.size()==1,"one Gold manager menu");
        check(menu_text[0]=="Exchange Gold Points (1 for 1):About account scope:Leave","Gold menu exposes exactly the three routed actions");
        check(says("login account")==(choice==2),"About reaches the account explanation only");
        check(says("You do not have any Gold Points")==(choice==1),"only Exchange enters the exchange flow");
        check(nums[add_str("#FP_Fashion")]==100&&!debit_calls,"informational/cancel paths never charge");
        finish_player();
    }

    // Real forward/reverse mappings resolve all five costume slot families.
    // Choose each displayed enchant, then cancel payment before mutation.
    for(bool garment:{false,true}) for(int selection=1;selection<=(garment?2:3);++selection) {
        auto sd=fashion_player();int costume=garment?19962:19961;
        put(0,costume,1);auto& it=sd->inventory.u.items_inventory[0];
        const int part=garment?EQI_COSTUME_GARMENT:EQI_COSTUME_HEAD_TOP;
        it.equip=garment?EQP_COSTUME_GARMENT:EQP_COSTUME_HEAD_TOP;sd->equip_index[part]=0;
        it.card[0]=garment?4807:4700;it.card[1]=garment?310654:4710;if(!garment)it.card[2]=4884;
        Snapshot before;walk(recover,{1,selection,4});before.unchanged();
        check(menu_text.size()==3,"costume, enchant and payment menus reached");
        const int options=std::count(menu_text[1].begin(),menu_text[1].end(),':');
        check(options==(garment?2:3),"exactly one enchant option per recoverable slot");
        const int expected=garment?(selection==1?6908:1000520):(selection==1?6636:selection==2?6946:6644);
        check(says("Recover ^0000ffStone "+std::to_string(expected)+"^000000 from slot "+std::to_string(selection)+"."),"displayed enchant resolves to its exact slot and physical stone");
        check(nums[add_str("#FP_Fashion")]==100&&!debit_calls,"payment cancellation keeps points and costume");
        finish_player();
    }

    for(bool failed:{false,true}) {
        auto sd=fashion_player();put(0,19961,1);Snapshot before;reject_debit=failed;
        walk(designer,{1,1});check(debit_calls==1,"exactly one costume debit attempted");
        check(nums[add_str("#FP_Fashion")]==(failed?100:101),"Fashion Points require successful inventory debit");
        check(says("Trade complete.")!=failed,"success is reported only after debit success");
        if(failed){before.unchanged();check(says("No Fashion Points were awarded"),"failed debit explains refusal");}
        else check(!count(19961),"successful debit consumes costume");
        finish_player();
    }
    script_free_code(gold);script_free_code(recover);script_free_code(designer);
    item_db.clear();nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("NPC_AUDIT_FASHION_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
