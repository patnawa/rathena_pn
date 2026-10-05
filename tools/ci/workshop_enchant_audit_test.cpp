// Copyright (C) 2026 PN Development Team. GPL-3.0-or-later; see LICENSE.
// Actual Workshop NPC execution. Shared harness supplies explicit UI, player and
// persistence boundaries. Inventory comparison, mutation and deletion are real.
struct Route { std::vector<int> answers; uint64 group; };
#include "routes.inc"
extern "C" void effect(const block_list*,int32,send_target) asm("__wrap__Z18clif_specialeffectPK10block_listi11send_target");
extern "C" void effect(const block_list*,int32,send_target) {}
extern "C" block_list* block_lookup(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* block_lookup(int32 id){return attached&&attached->id==id?attached:nullptr;}

namespace {
struct PlayerFree { void operator()(map_session_data* sd)const {
    if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);
    delete sd;
}};
std::unique_ptr<map_session_data,PlayerFree> setup(int stars=0) {
    ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();closes=0;
    std::unique_ptr<map_session_data,PlayerFree> sd(new map_session_data{});attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    sd->status.zeny=100000000;sd->status.inventory_slots=MAX_INVENTORY;sd->max_weight=1000000;
    for(auto& idx:sd->equip_index)idx=-1;
    setnum("RepPoints9",1500);
    for(int i=0;i<stars;++i){put(i,490136,1);auto& it=sd->inventory.u.items_inventory[i];
        it.unique_id=UINT64_MAX-i;it.card[2]=310709;it.card[3]=310710;}
    sd->inventory.amount=stars;weight();return sd;
}
void drive(script_code* code,const std::vector<int>& answers,
    const std::function<int(const std::string&)>& dynamic={},
    const std::function<void(int)>& before_answer={}) {
    run_script(code,0,attached->id,NPC);unsigned answered=0,pauses=0;
    while(attached->st){
        check(++pauses<512,"dialogue/page navigation terminates");auto* st=attached->st;
        check(!st->freeloop,"normal script execution guard restored before every dialogue yield");
        if(st->state==RERUNLINE){
            if(before_answer)before_answer(answered);
            int choice=answered<answers.size()?answers[answered]:dynamic?dynamic(menu_text.back()):0;
            check(choice>=1&&choice<=255,"supplied answer fits the real menu protocol");
            attached->npc_menu=choice;++answered;
        }else if(st->state==CLOSE)st->state=END;
        else if(st->state==STOP&&closes)st->state=RUN;
        else check(st->state==STOP,"known Next suspension");
        run_script_main(st);
    }
    check(answered>=answers.size(),"all intended menu answers consumed");
    check(errors==0,"no actual script error");
}
std::vector<std::string> options(const std::string& text){
    std::vector<std::string> out;std::stringstream stream(text);std::string part;
    while(std::getline(stream,part,':'))out.push_back(part);return out;
}
int find_option(const std::vector<std::string>& parts,const char* text){
    for(unsigned i=0;i<parts.size();++i)if(parts[i]==text)return i+1;
    return 0;
}
constexpr int MATERIALS[]={1002139,1002137,1002143,1002144,1002145,1002146};
void fund(){for(int i=0;i<6;++i)put(MAX_INVENTORY-6+i,MATERIALS[i],100);
    attached->inventory.amount=0;for(const auto& item:attached->inventory.u.items_inventory)
        if(item.nameid&&item.amount)++attached->inventory.amount;weight();}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2,"external fixture directory supplied");deny_network();
    static char server[]="workshop-enchant-audit";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;
    const std::string dir=argv[1];auto data=read(dir+"/items.yml");auto table=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto row:table["Body"])check(item_db.parseBodyNode(row)==1,"current effective transaction metadata parses");
    data=read(dir+"/reputation.yml");table=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto row:table["Body"])check(reputation_db.parseBodyNode(row)==1,"actual reputation metadata parses");
    for(auto id:GROUPS){auto group=std::make_shared<s_item_enchant>();group->id=id;item_enchant_db.put(id,group);}
    const auto equipment=read(dir+"/equipment.txt"),services=read(dir+"/services.txt");
    auto* npc=compile(body(equipment,"Equipment Enchanter#grademk"),"actual-Equipment-Enchanter");
    auto* legacy=compile(body(services,"Constellation Enchanter#grademk"),"actual-Constellation-template");
    auto* comma=compile(body(read(dir+"/functions.txt"),"function\tscript\tF_InsertComma\t"),"actual-InsertComma");
    strdb_put(script_get_userfunc_db(),"F_InsertComma",comma);
    // A fully expanded inventory can have two upgradeable enchants per item.
    // The actual select builtin must never truncate this menu or exceed byte choices.
    for(int stars:{25,MAX_INVENTORY}){
        auto sd=setup(stars);Snapshot before;
        drive(npc,{17,2},[](const std::string& text){return find_option(options(text),"Cancel");});
        before.unchanged();
    }
    for(const auto& route:ROUTES){auto sd=setup();Snapshot before;drive(npc,route.answers);
        before.unchanged();check(windows.size()==(route.group?1u:0u),"only Open routes request an enchant UI");
        if(route.group)check(windows[0]==route.group,"menu maps to the intended native recipe group");}
    for(int reputation:{1499,1500}){auto sd=setup();setnum("RepPoints9",reputation);Snapshot before;
        drive(npc,{24,4,2});before.unchanged();check(windows.size()==(reputation>=1500?1u:0u),"crown reputation threshold enforced");}
    for(auto* script:{npc,legacy})for(int slot:{2,3})for(int level:{3,4}){
        auto sd=setup(MAX_INVENTORY-6);fund();auto& target=sd->inventory.u.items_inventory[0];
        target.card[2]=slot==2?310706+level:0;target.card[3]=slot==3?310706+level:0;
        // Other copies are ineligible; make a full inventory without ambiguous choices.
        for(int i=1;i<MAX_INVENTORY-6;++i)sd->inventory.u.items_inventory[i].card[2]=sd->inventory.u.items_inventory[i].card[3]=0;
        target.id=198;target.refine=12;target.enchantgrade=4;target.card[0]=4001;target.bound=2;target.favorite=1;
        target.option[0].id=1;target.option[0].value=42;target.option[0].param=3;item expected=target;expected.card[slot]++;
        const int price=slot==3?(level==3?15000000:20000000):(level==3?17500000:25000000);
        const int amount=slot==3?(level==3?10:15):(level==3?12:18);
        const int other=slot==3?(level==3?7:10):(level==3?9:12);
        drive(script,script==npc?std::vector<int>{17,2,1,1}:std::vector<int>{8,1,1});
        check(std::memcmp(&expected,&target,sizeof(item))==0,"only selected enchant changes; all gear metadata preserved");
        check(sd->status.zeny==100000000-price,"exact displayed Zeny price charged once");
        for(int i=0;i<6;++i)check(count(MATERIALS[i])==100-(i?other:amount),"exact recipe materials deducted once");
        check(sd->inventory.amount==MAX_INVENTORY,"upgrade needs no free inventory slot");
    }
    // Failure and stale-target cases run the same real paid NPC path.
    for(auto* script:{npc,legacy})for(int change=0;change<13;++change){auto sd=setup(1);fund();Snapshot before;
        if(change==0){sd->status.zeny=0;before=Snapshot();}
        if(change==1){sd->inventory.u.items_inventory[MAX_INVENTORY-6].amount=0;before=Snapshot();}
        const int confirm=script==npc?3:2;
        drive(script,script==npc?std::vector<int>{17,2,1,change==2?2:change==3?255:1}:
            std::vector<int>{8,1,change==2?2:change==3?255:1},{},[&](int answered){
            if(answered!=confirm||change<4)return;
            auto& item=sd->inventory.u.items_inventory[0];
            if(change==4)++item.unique_id;if(change==5)++item.card[0];if(change==6)item.equip=EQP_ACC_L;
            if(change==7)item.identify=0;if(change==8)item.nameid=490137;if(change==9)item.amount=2;
            if(change>=10)++item.card[change-9];
            before=Snapshot();
        });before.unchanged();}
    for(auto* script:{npc,legacy})for(int material=0;material<6;++material){auto sd=setup(1);fund();
        sd->inventory.u.items_inventory[MAX_INVENTORY-6+material].amount=1;Snapshot before;
        drive(script,script==npc?std::vector<int>{17,2,1,1}:std::vector<int>{8,1,1});before.unchanged();}
    for(auto* script:{npc,legacy})for(int invalid=0;invalid<4;++invalid){auto sd=setup(invalid?1:0);
        if(invalid==1)sd->inventory.u.items_inventory[0].equip=EQP_ACC_L;
        if(invalid==2)sd->inventory.u.items_inventory[0].expire_time=2100000000;
        if(invalid==3)sd->inventory.u.items_inventory[0].card[2]=sd->inventory.u.items_inventory[0].card[3]=310711;
        Snapshot before;drive(script,script==npc?std::vector<int>{17,2}:std::vector<int>{8});before.unchanged();
        check(menu_text.size()==(script==npc?2u:1u),"no upgrade selection offered without an eligible item/level");
    }
    // Every copy and both eligible slots must remain reachable and distinguishable.
    for(auto* script:{npc,legacy}){auto sd=setup(MAX_INVENTORY);Snapshot before;std::set<std::string> seen;
        drive(script,script==npc?std::vector<int>{17,2}:std::vector<int>{8},[&](const std::string& text){
            auto parts=options(text);for(const auto& part:parts)if(part.find("Star of Spell Lv")!=std::string::npos)
                check(seen.insert(part).second,"each item/slot entry has an unambiguous label");
            int next=find_option(parts,"Next page");return next?next:find_option(parts,"Cancel");
        });before.unchanged();check(seen.size()==MAX_INVENTORY*2,"all expanded-inventory item/slot choices reachable");}
    for(auto* script:{npc,legacy}){
        auto sd=setup(MAX_INVENTORY-6);fund();item expected=sd->inventory.u.items_inventory[MAX_INVENTORY-7];
        expected.card[3]=310711;bool selected=false;
        drive(script,script==npc?std::vector<int>{17,2}:std::vector<int>{8},[&](const std::string& text){
            auto parts=options(text);int confirm=find_option(parts,"Upgrade");if(confirm)return confirm;
            const std::string wanted="#"+std::to_string(MAX_INVENTORY-6)+" +0 Star of Spell Lv4 -> Lv5 (slot 3)";
            int target=find_option(parts,wanted.c_str());if(target){selected=true;return target;}
            return find_option(parts,"Next page");
        });check(selected,"last copy/last slot can be selected across all pages");
        check(std::memcmp(&expected,&sd->inventory.u.items_inventory[MAX_INVENTORY-7],sizeof(item))==0,"page offset mutates exactly the last selected item");
        check(sd->inventory.u.items_inventory[0].card[3]==310710,"first copy unaffected by late-page selection");
        check(sd->status.zeny==80000000,"late-page upgrade charges the correct slot/level price");
        for(int i=0;i<6;++i)check(count(MATERIALS[i])==100-(i?10:15),"late-page exact material cost");
    }
    {
        auto sd=setup(25);Snapshot before;int page=0;std::string first;
        drive(npc,{17,2},[&](const std::string& text){auto parts=options(text);++page;
            if(page==1){first=text;return find_option(parts,"Next page");}
            if(page==2)return find_option(parts,"Previous page");
            check(text==first,"Previous returns to the same item choices");return find_option(parts,"Cancel");
        });before.unchanged();check(page==3,"next, previous and cancel each handled once");
    }
    script_free_code(npc);script_free_code(legacy);item_db.clear();item_enchant_db.clear();reputation_db.clear();
    do_final_script();timer_final();db_final();malloc_final();
    std::printf("WORKSHOP_ENCHANT_NATIVE_OK cases=%u assertions=%u routes=%zu\n",cases,assertions,ROUTES.size());
    return 0;
}
