// GPL-3.0-or-later. Appended to the existing isolated crown VM fixture helpers.
// Production NPC bodies and their fashion mapping functions are parsed unchanged.
// Persistence, inventory debit and outbound UI are explicit fixture boundaries.
namespace {
bool reject_debit=false;
unsigned debit_calls=0;
bool awaiting_amount=false;

std::unique_ptr<map_session_data> fashion_player() {
    ++cases; nums.clear(); strings.clear(); messages.clear(); menu_text.clear();
    closes=0; debit_calls=0; reject_debit=false;
    awaiting_amount=false;
    auto sd=std::make_unique<map_session_data>(); attached=sd.get();
    sd->id=sd->status.account_id=99000001; sd->status.char_id=99000002; sd->type=BL_PC;
    sd->status.zeny=7654321; sd->status.inventory_slots=MAX_INVENTORY;
    sd->max_weight=1000000;
    sd->fd=0; sd->state.ignoretimeout=true; sd->npc_idle_timer=INVALID_TIMER;
    for(auto& index:sd->equip_index) index=-1;
    setnum("#FP_Fashion",100);
    setnum("@FP_TimerRunning",1);
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
void fashion_walk(script_code* code,const std::vector<int>& choices,
                  const std::function<void(int,int)>& hook={}) {
    run_script(code,0,attached->id,NPC);int chosen=0,pause=0;
    while(attached->st) {
        check(++pause<40,"fashion dialogue has bounded suspensions");
        auto* st=attached->st;if(hook)hook(pause,chosen);
        if(st->state==RERUNLINE) {
            check(chosen<(int)choices.size(),"answer supplied for actual fashion prompt");
            if(awaiting_amount){attached->npc_amount=choices[chosen++];awaiting_amount=false;}
            else attached->npc_menu=choices[chosen++];
        } else if(st->state==CLOSE)st->state=END;
        else if(st->state==STOP&&closes)st->state=RUN;
        else check(st->state==STOP,"known fashion suspension");
        run_script_main(st);
    }
    check(errors==0,"fashion path has no VM diagnostics");
    check(chosen==(int)choices.size(),"all expected fashion actions reached");
}
}

extern "C" bool fashion_account(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool fashion_account(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}
extern "C" char fashion_debit(map_session_data*,int32,int32,int32,int16,e_log_pick_type) asm("__wrap__Z10pc_delitemP16map_session_dataiiis15e_log_pick_type");
extern "C" char fashion_debit(map_session_data* sd,int32 index,int32 amount,int32,int16,e_log_pick_type){
    ++debit_calls;
    check(sd==attached&&index>=0&&index<MAX_INVENTORY&&amount>0,"debit names valid selected inventory record");
    if(reject_debit) return 1;
    auto& it=sd->inventory.u.items_inventory[index];
    check(it.amount>=amount,"exact stack covers debit");it.amount-=amount;
    if(!it.amount){it={};sd->inventory_data[index]=nullptr;}
    return 0;
}
extern "C" void fashion_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void fashion_input(map_session_data&,uint32){awaiting_amount=true;}
extern "C" e_additem_result fashion_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__wrap__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result fashion_add(map_session_data* sd,item* it,int32 amount,e_log_pick_type,bool){
    check(sd==attached&&amount==1,"purchase delivers exactly one item");
    for(int i=0;i<MAX_INVENTORY;++i)if(!sd->inventory.u.items_inventory[i].nameid){put(i,it->nameid,amount);return ADDITEM_SUCCESS;}
    return ADDITEM_OVERAMOUNT;
}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"source path supplied");deny_network();static char server[]="npc-audit-fashion-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    script_config.check_cmdcount=655360;
    script_config.check_gotocount=2048;
    battle_config.atcommand_disable_npc=0;
    // These minimal identities exercise menu construction; full item effects,
    // payment commit and native equipment mutation are outside this fixture.
    item_fixture(19961,IT_ARMOR,EQP_COSTUME_HEAD_TOP,"Test costume");
    item_fixture(19962,IT_ARMOR,EQP_COSTUME_GARMENT,"Test garment");
    for(int id:{4700,4710,4884,4807,310654}) item_fixture(id,IT_CARD,0,("Enchant "+std::to_string(id)).c_str());
    for(int id:{6636,6946,6644,6908,1000520}) item_fixture(id,IT_ETC,0,("Stone "+std::to_string(id)).c_str());
    const auto source=read(argv[1]);
    size_t position=0;
    while((position=source.find(".@d$=\"",position))!=std::string::npos) {
        position+=6;const auto end=source.find('"',position);
        std::istringstream values(source.substr(position,end-position));std::string token;bool stone=true;
        while(std::getline(values,token,',')) {
            int id=std::stoi(token);
            if(!item_db.find(id))item_fixture(id,stone?IT_ETC:IT_CARD,0,((stone?"Stone ":"Enchant ")+std::to_string(id)).c_str());
            if(!stone)item_db.find(id)->subtype=CARD_ENCHANT;
            stone=!stone;
        }
        position=end;
    }
    for(const char* name:{"FP_LoadBox","FP_StoneKnown","FP_StoneFromEnchant","FP_IsTombCostume","FP_TradeItem","FP_BuyItem","FP_EnchantSupported","FP_OpenBox"}) {
        auto* fn=compile(body(source,std::string("function\tscript\t")+name),name);
        strdb_put(script_get_userfunc_db(),name,fn);
    }
    auto* gold=compile(body(source,"\tGold Point Manager#FP\t"),"actual Gold Point Manager");
    auto* recover=compile(body(source,"\tGregio Grumani#FP\t"),"actual Gregio Grumani");
    auto* designer=compile(body(source,"\tFashion Designer#FP\t"),"actual Fashion Designer");
    auto* catalogue=compile(body(source,"\tFashion Catalogue#FP\t"),"actual Fashion Catalogue");
    auto* boxes=compile(body(source,"\tFashion Box Shop#FP\t"),"actual Fashion Box Shop");
    auto* enchanter=compile(body(read("npc/custom/fashion_points/FashionEnchant.txt"),"\tComplete Fashion Enchanter#FP\t"),"actual Fashion Enchanter");

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
    for(bool garment:{false,true}) {
        auto sd=fashion_player();put(0,garment?19962:19961,1);
        auto& it=sd->inventory.u.items_inventory[0];
        const int part=garment?EQI_COSTUME_GARMENT:EQI_COSTUME_HEAD_TOP;
        it.equip=garment?EQP_COSTUME_GARMENT:EQP_COSTUME_HEAD_TOP;sd->equip_index[part]=0;
        Snapshot before;fashion_walk(recover,{1});before.unchanged();
        check(says("no recoverable enchant"),"empty costume recovery scan completes");finish_player();
    }
    // A normal full inventory must reach the no-stones result without hitting
    // the production script command/goto limits.
    {
        auto sd=fashion_player();item_fixture(501,IT_HEALING,0,"Red Potion");
        const int n=std::getenv("FASHION_INVENTORY_COUNT")?std::atoi(std::getenv("FASHION_INVENTORY_COUNT")):MAX_INVENTORY;
        for(int i=0;i<n;++i) put(i,501,1);
        Snapshot before;walk(designer,{2});before.unchanged();
        check(says("No enchant stone"),"full ordinary inventory completes stone scan");
        finish_player();
    }
    for(int amount:{0,1,50})for(int confirm:{1,2}) {
        auto sd=fashion_player();setnum("#FP_Gold",50);
        fashion_walk(gold,amount?std::vector<int>{1,amount,confirm}:std::vector<int>{1,0});
        const int exchanged=amount&&confirm==1?amount:0;
        check(nums[add_str("#FP_Gold")]==50-exchanged&&nums[add_str("#FP_Fashion")]==100+exchanged,"Gold exchanges exact amount or cancels");
        finish_player();
    }
    for(int kind=0;kind<3;++kind) {
        auto sd=fashion_player();setnum("#FP_Gold",50);
        if(kind==1)setnum("#FP_Fashion",INT64_MAX);
        fashion_walk(gold,{1,50,1},[&](int,int chosen){if(kind==0&&chosen==2)setnum("#FP_Gold",1);});
        check(nums[add_str("#FP_Fashion")]==(kind==1?INT64_MAX:kind==0?100:150),"Gold stale balance and overflow are refused");
        finish_player();
    }
    for(bool failed:{false,true}) {
        auto sd=fashion_player();put(0,6636,10);reject_debit=failed;
        fashion_walk(designer,{2,1,4,1});
        check(count(6636)==(failed?10:6)&&nums[add_str("#FP_Fashion")]==(failed?100:140),"stone exchange commits exact quantity and credit together");
        finish_player();
    }
    for(int flag=0;flag<4;++flag) {
        auto sd=fashion_player();put(0,19961,1);
        auto& it=sd->inventory.u.items_inventory[0];
        if(flag==0)it.favorite=1;if(flag==1)it.bound=1;if(flag==2)it.option[0].id=1;if(flag==3)it.identify=0;
        Snapshot before;fashion_walk(designer,{1});before.unchanged();
        check(!debit_calls&&nums[add_str("#FP_Fashion")]==100,"protected costume cannot be exchanged");finish_player();
    }
    item_fixture(480177,IT_ARMOR,EQP_COSTUME_GARMENT,"Costume: Black Bear Backpack");
    item_fixture(41090,IT_DELAYCONSUME,0,"Top Box 1");
    for(bool box:{false,true})for(int kind=0;kind<5;++kind) {
        auto sd=fashion_player();setnum("#FP_Fashion",kind==1?0:300);
        if(kind==2)for(int i=0;i<MAX_INVENTORY;++i)put(i,501,1);
        Snapshot before;
        if(box) fashion_walk(boxes,kind==1||kind==2?std::vector<int>{1}:std::vector<int>{1,kind==3?2:1},[&](int,int chosen){if(kind==4&&chosen==1)setnum("#FP_Fashion",0);});
        else fashion_walk(catalogue,{1,1,kind==3?2:1},[&](int,int chosen){if(kind==4&&chosen==2)setnum("#FP_Fashion",0);});
        bool success=kind==0;
        check(count(box?41090:480177)==(success?1:0),"purchase delivery follows confirmation and capacity");
        check(nums[add_str("#FP_Fashion")]==(success?(box?250:150):(kind==1||kind==4?0:300)),"only delivered purchases debit points");
        if(!success)before.unchanged();
        if(!box)check(std::count(menu_text[1].begin(),menu_text[1].end(),':')==1,"real costume colon cannot create extra menu option");
        finish_player();
    }
    for(int category=1;category<=5;++category) {
        auto sd=fashion_player();Snapshot before;fashion_walk(enchanter,{category});before.unchanged();
        check(says("You do not have a supported"),"all enchant categories complete empty inventory scan");finish_player();
    }
    auto* open_box=compile("{ callfunc \"FP_OpenBox\",0,41090; end; }","actual item box entry");
    for(int kind=0;kind<3;++kind) {
        auto sd=fashion_player();if(kind!=2)put(0,41090,1);
        if(kind==1)for(int i=1;i<MAX_INVENTORY;++i)put(i,501,1);
        Snapshot before;fashion_walk(open_box,{});
        check(count(41090)==(kind==1?1:0),"box consumed only with successful stone delivery");
        if(kind==0) {
            int rewards=0;for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid&&it.nameid!=41090)rewards+=it.amount;
            check(rewards==1,"box delivers exactly one stone");
        }
        if(kind!=0)before.unchanged();
        check(nums[add_str("#FP_Fashion")]==100,"opening a paid box does not charge points again");finish_player();
    }
    script_free_code(open_box);script_free_code(catalogue);script_free_code(boxes);script_free_code(enchanter);
    script_free_code(gold);script_free_code(recover);script_free_code(designer);
    item_db.clear();nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("NPC_AUDIT_FASHION_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
