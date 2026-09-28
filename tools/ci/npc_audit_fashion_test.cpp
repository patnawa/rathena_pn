// GPL-3.0-or-later. Appended to the existing isolated crown VM fixture helpers.
// Production NPC bodies and their fashion mapping functions are parsed unchanged.
// Persistence, inventory debit and outbound UI are explicit fixture boundaries.
namespace {
bool reject_debit=false;
unsigned debit_calls=0;
bool awaiting_amount=false;
bool reject_add=false;
unsigned reject_debit_at=0;

std::unique_ptr<map_session_data> fashion_player() {
    ++cases; nums.clear(); strings.clear(); messages.clear(); menu_text.clear();
    closes=unequips=equips=0; debit_calls=0; reject_debit=reject_add=false;
    fail_unequip=fail_equip=false;unequip_hook={};
    reject_debit_at=0;
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
    if(reject_debit||(reject_debit_at&&debit_calls==reject_debit_at)) return 1;
    auto& it=sd->inventory.u.items_inventory[index];
    check(it.amount>=amount,"exact stack covers debit");it.amount-=amount;
    if(!it.amount){it={};sd->inventory_data[index]=nullptr;}
    return 0;
}
extern "C" void fashion_input(map_session_data&,uint32) asm("__wrap__Z16clif_scriptinputR16map_session_dataj");
extern "C" void fashion_input(map_session_data&,uint32){awaiting_amount=true;}
extern "C" e_additem_result fashion_add(map_session_data*,item*,int32,e_log_pick_type,bool) asm("__wrap__Z10pc_additemP16map_session_dataP4itemi15e_log_pick_typeb");
extern "C" e_additem_result fashion_add(map_session_data* sd,item* it,int32 amount,e_log_pick_type,bool){
    check(sd==attached&&amount>0,"delivery targets attached inventory with a positive amount");
    if(reject_add)return ADDITEM_OVERAMOUNT;
    // Match the stack identity fields used by pc_additem. Full inventory can
    // still accept a returned stone when a compatible permanent stack exists.
    for(int i=0;i<sd->status.inventory_slots;++i){
        auto& current=sd->inventory.u.items_inventory[i];
        if(itemdb_isstackable2(item_db.find(it->nameid).get())&&current.nameid==it->nameid
            &&current.bound==it->bound&&current.expire_time==it->expire_time
            &&std::memcmp(current.card,it->card,sizeof(it->card))==0
            &&current.amount+amount<=MAX_AMOUNT){current.amount+=amount;weight();return ADDITEM_SUCCESS;}
    }
    for(int i=0;i<sd->status.inventory_slots;++i)if(!sd->inventory.u.items_inventory[i].nameid){
        put(i,it->nameid,amount);sd->inventory.u.items_inventory[i]=*it;
        sd->inventory.u.items_inventory[i].amount=amount;weight();return ADDITEM_SUCCESS;
    }
    return ADDITEM_OVERAMOUNT;
}
extern "C" bool unequip(map_session_data*,int32,int32) asm("__wrap__Z14pc_unequipitemP16map_session_dataii");
extern "C" bool unequip(map_session_data* sd,int32 index,int32 flags){
    ++unequips;check(flags==3,"native fashion mutation forces expected unequip flags");
    if(fail_unequip)return false;
    sd->inventory.u.items_inventory[index].equip=0;
    for(auto& equipped:sd->equip_index)if(equipped==index)equipped=-1;
    if(unequip_hook)unequip_hook();return true;
}
extern "C" bool equip(map_session_data*,int16,int32,bool) asm("__wrap__Z12pc_equipitemP16map_session_datasib");
extern "C" bool equip(map_session_data* sd,int16 index,int32 position,bool){
    ++equips;if(fail_equip)return false;
    sd->inventory.u.items_inventory[index].equip=position;
    for(int part=0;part<EQI_MAX;++part)if(position&equip_bitmask[part])sd->equip_index[part]=index;
    return true;
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
    item_fixture(19963,IT_ARMOR,EQP_COSTUME_HEAD_MID,"Test middle costume");
    item_fixture(19964,IT_ARMOR,EQP_COSTUME_HEAD_LOW,"Test lower costume");
    item_fixture(19965,IT_ARMOR,EQP_COSTUME_HEAD_TOP|EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW,"Test three-position costume");
    item_fixture(19966,IT_ARMOR,EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW,"Test middle-lower costume");
    item_fixture(50000,IT_ETC,0,"Server Coin");
    for(int id:{4700,4710,4884,4807,310654}) item_fixture(id,IT_CARD,0,("Enchant "+std::to_string(id)).c_str());
    for(int id:{6636,6946,6644,6908,1000520}) item_fixture(id,IT_ETC,0,("Stone "+std::to_string(id)).c_str());
    const auto source=read(argv[1]);
    std::vector<std::vector<int>> box_materials;
    size_t position=0;
    while((position=source.find(".@d$=\"",position))!=std::string::npos) {
        position+=6;const auto end=source.find('"',position);
        box_materials.emplace_back();
        std::istringstream values(source.substr(position,end-position));std::string token;bool stone=true;
        while(std::getline(values,token,',')) {
            int id=std::stoi(token);
            if(stone)box_materials.back().push_back(id);
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
        check(says("No supported enchant stone from the catalogue"),"full ordinary inventory completes stone scan");
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
    // The visual/Festa offers are late in the real 26-entry menu. Exercise
    // purchase and the actual installed item script together, so a misplaced
    // menu entry or box-to-pool mapping cannot silently award another family.
    const auto box_db=read("db/import/fashion_points_box_item_db.yml");
    const char* visual_box_names[]={"Top Visual Effect Box","Middle Visual Effect Box",
        "Lower Visual Effect Box","Garment Footprint Box","Festa Upper Slot 2 Box"};
    for(int family=0;family<5;++family) {
        const int id=41500+family, index=21+family;
        item_fixture(id,IT_DELAYCONSUME,0,visual_box_names[family]);
        const auto record=box_db.find("  - Id: "+std::to_string(id));
        check(record!=std::string::npos,"visual box item DB record exists");
        const auto start=box_db.find("callfunc ",record);
        const auto end=box_db.find(';',start);
        check(start!=std::string::npos&&end!=std::string::npos,"visual box has a callable item script");
        auto* open=compile("{ "+box_db.substr(start,end-start+1)+" end; }","actual visual box item entry");
        auto sd=fashion_player();
        fashion_walk(boxes,{index+1,1});
        check(count(id)==1&&nums[add_str("#FP_Fashion")]==50,"each visual/Festa box costs exactly 50 Fashion Points");
        check(menu_text[0].find(std::string(visual_box_names[family])+" - 50 FP")!=std::string::npos,"visual box name and price are discoverable in the player menu");
        fashion_walk(open,{});
        int rewards=0;
        for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid) {
            check(std::find(box_materials[index].begin(),box_materials[index].end(),it.nameid)!=box_materials[index].end(),"visual box awards only its own physical stone family");
            rewards+=it.amount;
        }
        check(rewards==1&&count(id)==0,"visual box consumes one box and delivers exactly one material");
        check(nums[add_str("#FP_Fashion")]==50,"opening the visual box never charges Fashion Points twice");
        finish_player();script_free_code(open);
    }
    for(int category=1;category<=10;++category) {
        auto sd=fashion_player();Snapshot before;fashion_walk(enchanter,{category});before.unchanged();
        check(says("You do not have a supported"),"all enchant categories complete empty inventory scan");finish_player();
    }
    // One visual effect uses the fourth card slot independently of stat stones.
    // Shared Electric enchant IDs recover the material for the equipped family.
    const int effect_costume[]={19961,19963,19964,19962};
    const int effect_part[]={EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW,EQI_COSTUME_GARMENT};
    const int effect_mask[]={EQP_COSTUME_HEAD_TOP,EQP_COSTUME_HEAD_MID,EQP_COSTUME_HEAD_LOW,EQP_COSTUME_GARMENT};
    const int effect_stone[]={25058,25136,1000882,1001615};
    const int effect_card[]={29041,29142,29142,313065};
    for(int family=0;family<4;++family) {
        auto sd=fashion_player();put(0,effect_costume[family],1);
        auto& it=sd->inventory.u.items_inventory[0];
        it.equip=effect_mask[family];sd->equip_index[effect_part[family]]=0;it.card[3]=effect_card[family];
        Snapshot before;fashion_walk(recover,{1,1,4});before.unchanged();
        check(says("Recover ^0000ffStone "+std::to_string(effect_stone[family])+"^000000 from slot 4."),"visual recovery retains costume family and slot four");
        check(!debit_calls,"cancelled visual recovery does not charge");finish_player();
    }
    // A single inventory costume must not appear once per occupied position.
    // Unique effects resolve outside the first position; shared IDs have one
    // canonical material chosen from the costume's actual supported positions.
    const int multi_card[]={29040,29143,29142,29144,29142};
    const int multi_stone[]={25059,25137,25206,25138,25136};
    for(int example=0;example<5;++example) {
        auto sd=fashion_player();const bool middle_lower=example==4;
        put(0,middle_lower?19966:19965,1);auto& it=sd->inventory.u.items_inventory[0];
        it.equip=EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
        if(!middle_lower){it.equip|=EQP_COSTUME_HEAD_TOP;sd->equip_index[EQI_COSTUME_HEAD_TOP]=0;}
        sd->equip_index[EQI_COSTUME_HEAD_MID]=sd->equip_index[EQI_COSTUME_HEAD_LOW]=0;
        it.card[3]=multi_card[example];
        Snapshot before;fashion_walk(recover,{1,1,4});before.unchanged();
        check(menu_text.size()==3,"multi-position recovery reaches costume, enchant and payment menus");
        check(std::count(menu_text[0].begin(),menu_text[0].end(),':')==1,"one recovery entry for the same multi-position inventory record");
        check(std::count(menu_text[1].begin(),menu_text[1].end(),':')==1,"one recovery option for the shared visual slot");
        check(says("Recover ^0000ffStone "+std::to_string(multi_stone[example])+"^000000 from slot 4."),"multi-position visual resolves unique and canonical shared materials");
        check(!debit_calls&&nums[add_str("#FP_Fashion")]==100,"cancelled multi-position recovery preserves payment");
        finish_player();
    }
    for(int selection=1;selection<=3;++selection) {
        auto sd=fashion_player();put(0,19965,1);auto& it=sd->inventory.u.items_inventory[0];
        it.equip=EQP_COSTUME_HEAD_TOP|EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
        for(int part:{EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW})sd->equip_index[part]=0;
        it.card[0]=4700;it.card[1]=314796;it.card[3]=29143;
        Snapshot before;fashion_walk(recover,{1,selection,4});before.unchanged();
        const int expected[]={6636,1002790,25137};const int slot[]={1,2,4};
        check(std::count(menu_text[0].begin(),menu_text[0].end(),':')==1,"mixed enchant costume remains deduplicated");
        check(std::count(menu_text[1].begin(),menu_text[1].end(),':')==3,"stat, Festa and visual slots remain independently recoverable");
        check(says("Recover ^0000ffStone "+std::to_string(expected[selection-1])+"^000000 from slot "+std::to_string(slot[selection-1])+"."),"multi-position costume retains stat and Festa recovery alongside unique visual");
        check(!debit_calls&&nums[add_str("#FP_Fashion")]==100,"mixed enchant cancellation preserves payment");
        finish_player();
    }
    {
        auto sd=fashion_player();put(0,19965,1);put(1,19962,1);
        auto& head=sd->inventory.u.items_inventory[0];auto& garment=sd->inventory.u.items_inventory[1];
        head.equip=EQP_COSTUME_HEAD_TOP|EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
        for(int part:{EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW})sd->equip_index[part]=0;
        garment.equip=EQP_COSTUME_GARMENT;sd->equip_index[EQI_COSTUME_GARMENT]=1;garment.card[3]=313065;
        Snapshot before;fashion_walk(recover,{2,1,4});before.unchanged();
        check(std::count(menu_text[0].begin(),menu_text[0].end(),':')==2,"deduplication retains a distinct equipped garment inventory record");
        check(says("Recover ^0000ffStone 1001615^000000 from slot 4."),"second distinct costume resolves its own visual");
        check(!debit_calls&&nums[add_str("#FP_Fashion")]==100,"distinct costume cancellation preserves payment");
        finish_player();
    }
    for(int family=0;family<4;++family) for(bool occupied:{false,true}) {
        auto sd=fashion_player();put(0,effect_costume[family],1);put(1,effect_stone[family],1);
        auto& it=sd->inventory.u.items_inventory[0];
        it.equip=effect_mask[family];sd->equip_index[effect_part[family]]=0;
        if(occupied)it.card[3]=effect_card[family];
        Snapshot before;fashion_walk(enchanter,occupied?std::vector<int>{6+family,1}:std::vector<int>{6+family,1,2});before.unchanged();
        check(says("already contains an enchant")==occupied,"visual application refuses occupied fourth slots");
        check(!debit_calls,"cancelled or occupied visual application consumes no stone");finish_player();
    }
    // Regional Festa uses upper costume slot two; legacy Loft IDs stay recoverable.
    for(bool legacy_loft:{false,true}) {
        auto sd=fashion_player();put(0,legacy_loft?19962:19961,1);
        auto& it=sd->inventory.u.items_inventory[0];
        it.equip=legacy_loft?EQP_COSTUME_GARMENT:EQP_COSTUME_HEAD_TOP;
        sd->equip_index[legacy_loft?EQI_COSTUME_GARMENT:EQI_COSTUME_HEAD_TOP]=0;
        it.card[legacy_loft?0:1]=legacy_loft?25934:314796;
        Snapshot before;fashion_walk(recover,{1,1,4});before.unchanged();
        check(says(legacy_loft?"Recover ^0000ffStone 25934^000000 from slot 1.":"Recover ^0000ffStone 1002790^000000 from slot 2."),"regional recovery respects legacy aliases and canonical guaranteed Festa material");
        check(!debit_calls,"cancelled regional recovery does not charge");finish_player();
    }
    for(bool occupied:{false,true}) {
        auto sd=fashion_player();put(0,19961,1);put(1,1002790,1);
        auto& it=sd->inventory.u.items_inventory[0];
        it.equip=EQP_COSTUME_HEAD_TOP;sd->equip_index[EQI_COSTUME_HEAD_TOP]=0;
        it.card[0]=4700;it.card[3]=29041;if(occupied)it.card[1]=314796;
        Snapshot before;fashion_walk(enchanter,occupied?std::vector<int>{10,1}:std::vector<int>{10,1,2});before.unchanged();
        check(says("already contains an enchant")==occupied,"Festa checks slot two independently of upper stats and visuals");
        check(!debit_calls,"cancelled or occupied Festa application consumes no stone");finish_player();
    }
    // Execute the real in-place application/recovery commit, retaining every
    // unrelated item byte through repeated multi-position equip transitions.
    for(bool prevent_reequip:{false,true}) {
        auto sd=fashion_player();put(0,19965,1);put(1,25058,1);
        auto& it=sd->inventory.u.items_inventory[0];
        it.equip=EQP_COSTUME_HEAD_TOP|EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
        for(int part:{EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW})sd->equip_index[part]=0;
        it.card[0]=4700;it.card[1]=4710;it.card[2]=4884;
        it.refine=12;it.enchantgrade=3;it.bound=2;it.favorite=1;it.expire_time=2100000000;
        for(int i=0;i<MAX_ITEM_RDM_OPT;++i){it.option[i].id=i+1;it.option[i].value=30+i;it.option[i].param=i;}
        const item original=it;
        for(int cycle=0;cycle<3;++cycle) {
            fail_equip=prevent_reequip;
            messages.clear();menu_text.clear();closes=0;
            fashion_walk(enchanter,{6,1,1});
            item applied=original;applied.card[3]=29041;if(prevent_reequip)applied.equip=0;
            check(std::memcmp(&applied,&it,sizeof(item))==0,"application changes only requested visual card and optional safe unequip state");
            check(count(25058)==0,"successful application consumes exactly one physical stone");
            check(nums[add_str("#FP_Fashion")]==100-30*cycle,"application never charges recovery payment");
            if(prevent_reequip){fail_equip=false;equip(sd.get(),0,original.equip,false);}
            messages.clear();menu_text.clear();closes=0;
            fashion_walk(recover,{1,4,1,1});
            check(std::memcmp(&original,&it,sizeof(item))==0,"recovery restores original costume bytes including stats identity binding rental and all options");
            check(count(25058)==1,"recovery returns exactly one stone without duplication");
            check(nums[add_str("#FP_Fashion")]==100-30*(cycle+1),"successful recovery charges exactly thirty points");
            for(int part:{EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW})
                check(sd->equip_index[part]==0,"multi-position costume equipment indexes survive each mutation");
        }
        finish_player();
    }
    for(bool multi_position:{false,true}) for(bool stack_exists:{false,true}) {
        auto sd=fashion_player();
        if(const char* slots=std::getenv("FASHION_TRANSACTION_SLOTS"))sd->status.inventory_slots=std::atoi(slots);
        check(sd->status.inventory_slots>=2&&sd->status.inventory_slots<=MAX_INVENTORY,"transaction fixture capacity is valid");
        for(int i=0;i<sd->status.inventory_slots;++i)put(i,501,1);
        const int equipment=multi_position?sd->status.inventory_slots-2:0;
        const int returned_stone=multi_position?25206:25058;
        put(equipment,multi_position?19965:19961,1);
        auto& it=sd->inventory.u.items_inventory[equipment];it.equip=EQP_COSTUME_HEAD_TOP;
        sd->equip_index[EQI_COSTUME_HEAD_TOP]=equipment;it.card[3]=multi_position?29142:29041;
        if(multi_position){
            it.equip|=EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
            sd->equip_index[EQI_COSTUME_HEAD_MID]=sd->equip_index[EQI_COSTUME_HEAD_LOW]=equipment;
            it.card[0]=4700;it.card[1]=4710;it.card[2]=4884;
        }
        if(stack_exists)put(sd->status.inventory_slots-1,returned_stone,7);
        const item original=it;Snapshot before;
        fashion_walk(recover,{1,multi_position?4:1,1,1});
        if(stack_exists){
            item expected=original;expected.card[3]=0;
            check(std::memcmp(&expected,&it,sizeof(item))==0,"full inventory recovery may reuse a compatible stone stack");
            check(count(returned_stone)==8&&nums[add_str("#FP_Fashion")]==70,"full inventory stacked recovery commits exactly one canonical return and payment");
        }else{
            before.unchanged();check(nums[add_str("#FP_Fashion")]==100,"full inventory without room preserves payment");
            check(says("Make room for the returned stone"),"full inventory refusal explains capacity before payment");
        }
        finish_player();
    }
    // The slowest payment path stages a last-index stack in a full inventory
    // and validates/debits mixed-bound coin rows before attempting mutation.
    for(int failure=0;failure<3;++failure) {
        auto sd=fashion_player();
        for(int i=0;i<MAX_INVENTORY;++i)put(i,501,1);
        put(0,19965,1);auto& it=sd->inventory.u.items_inventory[0];
        it.equip=EQP_COSTUME_HEAD_TOP|EQP_COSTUME_HEAD_MID|EQP_COSTUME_HEAD_LOW;
        for(int part:{EQI_COSTUME_HEAD_TOP,EQI_COSTUME_HEAD_MID,EQI_COSTUME_HEAD_LOW})sd->equip_index[part]=0;
        it.card[0]=4700;it.card[1]=4710;it.card[2]=4884;it.card[3]=29142;
        put(MAX_INVENTORY-1,25206,7);
        for(int i=1;i<=10;++i){put(i,50000,1);sd->inventory.u.items_inventory[i].bound=(i%2)+1;}
        const item original=it;
        fail_unequip=failure==1;reject_debit_at=failure==2?2:0;
        fashion_walk(recover,{1,4,2,1});
        item expected=original;if(!failure)expected.card[3]=0;
        check(std::memcmp(&expected,&it,sizeof(item))==0,"mixed-bound coin recovery changes only the committed card");
        check(count(25206)==(failure?7:8)&&count(50000)==(failure?10:0),"coin failure cannot duplicate or lose staged stones or payment");
        check(nums[add_str("#FP_Fashion")]==100,"coin payment never changes Fashion Points");
        if(failure){
            int bound[3]={};for(const auto& row:sd->inventory.u.items_inventory)if(row.nameid==50000)bound[row.bound]+=row.amount;
            check(bound[1]==5&&bound[2]==5,"compensation preserves every paid coin binding group");
        }
        finish_player();
    }
    for(bool recovery:{false,true}) {
        auto sd=fashion_player();put(0,19961,1);put(1,25058,1);
        auto& it=sd->inventory.u.items_inventory[0];it.equip=EQP_COSTUME_HEAD_TOP;
        sd->equip_index[EQI_COSTUME_HEAD_TOP]=0;if(recovery)it.card[3]=29041;
        fail_unequip=true;Snapshot before;
        fashion_walk(recovery?recover:enchanter,recovery?std::vector<int>{1,1,1,1}:std::vector<int>{6,1,1});
        before.unchanged();check(nums[add_str("#FP_Fashion")]==100,"mutation refusal refunds all staged payment");
        check(count(25058)==1,"mutation refusal restores exact physical stone count");
        finish_player();
    }
    for(bool refuse:{false,true}) {
        auto sd=fashion_player();
        for(int i=0;i<MAX_INVENTORY;++i)put(i,501,1);
        put(0,19961,1);put(MAX_INVENTORY-1,25058,1);
        sd->inventory.u.items_inventory[MAX_INVENTORY-1].bound=2;
        auto& it=sd->inventory.u.items_inventory[0];it.equip=EQP_COSTUME_HEAD_TOP;
        sd->equip_index[EQI_COSTUME_HEAD_TOP]=0;it.card[1]=4710;it.card[2]=4884;
        const item original=it;Snapshot before;fail_unequip=refuse;
        fashion_walk(enchanter,{6,1,1});
        if(refuse){before.unchanged();check(sd->inventory.u.items_inventory[MAX_INVENTORY-1].bound==2,"full inventory application refund retains material binding");}
        else{item expected=original;expected.card[3]=29041;check(std::memcmp(&expected,&it,sizeof(item))==0,"full inventory application consumes last-index stone without recreating costume");}
        check(count(25058)==(refuse?1:0)&&nums[add_str("#FP_Fashion")]==100,"full inventory application consumes only a successful stone");
        finish_player();
    }
    // Real OnUnequip callbacks can change inventory after confirmation. The
    // native compare-and-swap must preserve that callback state and refund the
    // stone instead of applying to a changed or relocated costume record.
    for(int change=0;change<3;++change) {
        auto sd=fashion_player();put(0,19961,1);put(1,25058,1);
        auto& it=sd->inventory.u.items_inventory[0];it.equip=EQP_COSTUME_HEAD_TOP;
        sd->equip_index[EQI_COSTUME_HEAD_TOP]=0;it.card[1]=4710;it.card[2]=4884;
        item expected=it;expected.equip=0;const int target=change==2?3:0;
        if(change==0)expected.unique_id=77;
        if(change==1)expected.card[1]=4700;
        unequip_hook=[&]{
            if(change==0)it.unique_id=77;
            if(change==1)it.card[1]=4700;
            if(change==2){sd->inventory.u.items_inventory[3]=it;sd->inventory_data[3]=sd->inventory_data[0];it={};sd->inventory_data[0]=nullptr;}
        };
        fashion_walk(enchanter,{6,1,1});
        check(std::memcmp(&expected,&sd->inventory.u.items_inventory[target],sizeof(item))==0,"unequip callback identity card and inventory moves are never overwritten");
        check(count(25058)==1&&nums[add_str("#FP_Fashion")]==100,"unequip callback refusal compensates physical stone without payment");
        check(says("Your stone was refunded"),"callback refusal is explicitly reported");
        finish_player();
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
