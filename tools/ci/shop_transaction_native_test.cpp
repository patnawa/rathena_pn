// Actual extracted npc_barter_purchase, real itemdb/pc inventory/zeny implementations.
#include "map/npc.hpp"
#include "map/pet.hpp"
#include "map/cashshop.hpp"
extern char sales_table[];
#include "common/nullpo.hpp"
#include "common/utils.hpp"
#include "map/atcommand.hpp"
#include "common/sql.hpp"
#include "common/showmsg.hpp"
extern char barter_table[];
extern Sql* mmysql_handle;
// FUNCTION
extern "C" int __wrap_main(int argc,char** argv) {
    deny_network(); static char server[]="shop-audit";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    auto data=read(std::string(argv[1])+"/items.yml"); auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"item parses");
    // Real caller regressions: allocate costs across eligible stacks and reuse
    // slots released by those costs, without widening material eligibility.
    for(int mode=0;mode<8;++mode) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->status.inventory_slots=(mode==1||mode==7)?1:MAX_INVENTORY;
        sd->status.zeny=1000;sd->max_weight=1000000;sd->type=BL_PC;
        sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
        for(auto& i:sd->equip_index)i=-1;
        auto shop=std::make_shared<s_npc_barter>();auto entry=std::make_shared<s_npc_barter_item>();
        entry->nameid=501;entry->price=100;entry->stockLimited=false;
        auto req=std::make_shared<s_npc_barter_requirement>();req->nameid=502;req->amount=10;req->refine=-1;
        entry->requirements[0]=req;
        put(0,502,(mode==1||mode==7)?10:4);
        if(mode!=1&&mode!=7){put(1,502,mode==3?5:6);sd->inventory.u.items_inventory[1].bound=1;}
        if(mode==2){req->amount=6;auto second=std::make_shared<s_npc_barter_requirement>(*req);second->amount=4;entry->requirements[1]=second;}
        const int old_hide=battle_config.hide_fav_sell;battle_config.hide_fav_sell=1;
        if(mode==4)sd->inventory.u.items_inventory[1].favorite=1;
        if(mode==5)sd->inventory.u.items_inventory[1].equipSwitch=1;
        if(mode==6)sd->max_weight=0;
        const bool old_guid=item_db.find(501)->flag.guid;
        if(mode==7){item_db.find(501)->flag.guid=true;req->amount=5;}
        weight();const auto before=sd->inventory;const auto before_weight=sd->weight;
        std::vector<s_barter_purchase> purchases={{entry,mode==7?2u:1u,nullptr}};
        const auto result=audit_npc_barter_purchase(*sd,shop,purchases);
        battle_config.hide_fav_sell=old_hide;
        item_db.find(501)->flag.guid=old_guid;
        const bool success=mode<=2||mode==7;
        const bool ok=success ? result==e_purchase_result::PURCHASE_SUCCEED&&count(502)==0&&count(501)==(mode==7?2:1)&&sd->status.zeny==(mode==7?800:900) :
            result!=e_purchase_result::PURCHASE_SUCCEED&&sd->status.zeny==1000&&sd->weight==before_weight&&
            !std::memcmp(&before,&sd->inventory,sizeof(before));
        std::printf("BARTER_ALLOCATION mode=%d result=%d %s\n",mode,int(result),ok?"PASS":"FAIL");
        if(!ok)++errors;
        if(mode==7)check(sd->inventory.u.items_inventory[0].unique_id==((uint64(99000002)<<32)|10)&&sd->status.uniqueitem_counter==11,
                        "GUID output remains one native stack with one generated identity");
    }
    for(int mode=0;mode<8;++mode) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->status.inventory_slots=MAX_INVENTORY;sd->status.zeny=10000000000LL;sd->max_weight=1000000;sd->type=BL_PC;
        for(auto& i:sd->equip_index)i=-1;
        auto shop=std::make_shared<s_npc_barter>();auto entry=std::make_shared<s_npc_barter_item>();
        entry->nameid=501;entry->price=mode==0?1500000000:mode==1?1073741824:100;entry->stockLimited=false;
        auto req=std::make_shared<s_npc_barter_requirement>();req->nameid=502;req->amount=1;req->refine=-1;entry->requirements[0]=req;
        put(0,502,20);weight();uint32 amount=mode==0?2:mode==1?4:mode==2?0:mode==3?UINT32_MAX:1;
        if(mode==5)sd->status.zeny=0;
        if(mode==6)sd->max_weight=0;
        const auto before_zeny=sd->status.zeny;
        std::vector<s_barter_purchase> purchases={{entry,amount,nullptr}};
        // Same-object pc.cpp calls bypass --wrap; exercise the actual quest VM
        // rather than assuming a wrapper sees native item callbacks.
        auto nd=std::make_unique<npc_data>();
        if(mode==7){
            nums.clear();map_num=1;sd->m=0;sd->id=sd->status.account_id=99000001;
            sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
            nd->id=NPC;nd->type=BL_NPC;quest_npc=nd.get();fake_nd=nd.get();map[0].qi_npc={NPC};sd->qi_display.resize(1);
            auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
            qi->condition=compile("{ if (!$@__SWseen_VAL) { $@__SWseen_VAL=1; $@__SWsafe_VAL=(countitem(502)==19 && countitem(501)==1 && Zeny==9999999900); Zeny=0; } achievement_condition(0); }","barter callback state");
            nd->qi_data.push_back(qi);
        }
        auto result=audit_npc_barter_purchase(*sd,shop,purchases);
        const bool callback_seen=nums[add_str("$@__SWseen_VAL")],callback_safe=nums[add_str("$@__SWsafe_VAL")];
        if(mode==7){map[0].qi_npc.clear();quest_npc=nullptr;fake_nd=nullptr;}
        bool invalid=mode==2||mode==3||mode==5||mode==6;
        bool ok=invalid?(result!=e_purchase_result::PURCHASE_SUCCEED && count(502)==20 && count(501)==0 && sd->status.zeny==before_zeny):
            (result==e_purchase_result::PURCHASE_SUCCEED && count(502)==20-amount && count(501)==amount && sd->status.zeny==10000000000LL-int64(entry->price)*amount);
        if(mode==7)ok=result==e_purchase_result::PURCHASE_SUCCEED&&callback_seen&&callback_safe&&sd->status.zeny==0&&count(501)==1;
        std::printf("BARTER mode=%d result=%d zeny=%lld inputs=%d outputs=%d %s\n",mode,int(result),(long long)sd->status.zeny,count(502),count(501),ok?"PASS":"FAIL");
        if(!ok)++errors;
    }
    for(int mode=0;mode<5;++mode) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->status.inventory_slots=MAX_INVENTORY;sd->status.zeny=10000000000LL;sd->max_weight=100000000;sd->type=BL_PC;
        for(auto& i:sd->equip_index)i=-1;
        npc_data nd={};nd.subtype=NPCTYPE_SHOP;fixture_shop=&nd;
        std::vector<npc_item_list> sales(mode==4?257:1);for(auto& other:sales)other.nameid=502;auto& sale=sales.back();sale.nameid=501;sale.value=100;sale.qty=-1;nd.u.shop.count=sales.size();nd.u.shop.shop_item=sales.data();
        if(mode==4)nd.subtype=NPCTYPE_MARKETSHOP;
        if(mode==0){put(0,501,MAX_AMOUNT-2);weight();}
        std::vector<s_npc_buy_list> purchases=mode==0?std::vector<s_npc_buy_list>{{2,501},{2,501}}:std::vector<s_npc_buy_list>{{mode==1?0:mode==2?-65535:1,501}};
        auto result=audit_npc_buylist(sd.get(),purchases);
        bool ok=mode>=3?(result==e_purchase_result::PURCHASE_SUCCEED&&count(501)==1&&sd->status.zeny==9999999900LL):
           (result!=e_purchase_result::PURCHASE_SUCCEED&&count(501)==(mode==0?MAX_AMOUNT-2:0)&&sd->status.zeny==10000000000LL);
        std::printf("SHOP mode=%d result=%d zeny=%lld outputs=%d %s\n",mode,int(result),(long long)sd->status.zeny,count(501),ok?"PASS":"FAIL");if(!ok)++errors;
    }
    for(auto subtype:{NPCTYPE_CASHSHOP,NPCTYPE_ITEMSHOP,NPCTYPE_POINTSHOP}) for(int mode=0;mode<2;++mode) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->status.inventory_slots=MAX_INVENTORY;sd->cashPoints=1000000;sd->max_weight=100000000;sd->type=BL_PC;
        for(auto& i:sd->equip_index)i=-1;
        npc_data nd={};nd.subtype=subtype;fixture_shop=&nd;
        npc_item_list sale={};sale.nameid=501;sale.value=mode==0?100:INT32_MAX;nd.u.shop.count=1;nd.u.shop.shop_item=&sale;
        if(mode==0){put(0,501,MAX_AMOUNT-2);weight();}
        std::vector<s_npc_buy_list> purchases=mode==0?std::vector<s_npc_buy_list>{{2,501},{2,501}}:std::vector<s_npc_buy_list>{{2,501}};
        auto result=audit_npc_cashshop_buylist(sd.get(),0,purchases);
        bool ok=result!=ERROR_TYPE_NONE&&sd->cashPoints==1000000&&count(501)==(mode==0?MAX_AMOUNT-2:0);
        std::printf("CASHSHOP subtype=%d mode=%d result=%d cash=%d outputs=%d %s\n",int(subtype),mode,result,sd->cashPoints,count(501),ok?"PASS":"FAIL");if(!ok)++errors;
    }
    for(int mode=0;mode<11;++mode) {
        ++cases;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->status.inventory_slots=MAX_INVENTORY;sd->cashPoints=1000000;sd->max_weight=100000000;sd->type=BL_PC;
        // Seed real in-memory account registry records; no character server.
        script_reg_num registry[2]{};registry[0].value=sd->cashPoints;
        sd->vars_ok=true;sd->regs.vars=i64db_alloc(DB_OPT_BASE);
        i64db_put(sd->regs.vars,add_str(CASHPOINT_VAR),&registry[0]);
        i64db_put(sd->regs.vars,add_str(KAFRAPOINT_VAR),&registry[1]);
        for(auto& i:sd->equip_index)i=-1;
        auto tab=std::make_shared<s_cash_item_tab>();tab->tab=CASHSHOP_TAB_NEW;
        auto entry=std::make_shared<s_cash_item>();entry->nameid=501;entry->price=mode==1?INT32_MAX:100;tab->items.push_back(entry);cash_shop_db.put(CASHSHOP_TAB_NEW,tab);
        if(mode==0){put(0,501,MAX_AMOUNT-2);weight();}
        PACKET_CZ_SE_PC_BUY_CASHITEM_LIST_sub purchases[2]={};for(auto& row:purchases){row.itemId=501;row.amount=(mode==2||mode>=9)?1:mode==3?0:mode==4?100:mode==8?99:2;row.tab=CASHSHOP_TAB_NEW;}
        if(mode==7)sd->bank_ui.pending=true;
        if(mode==9){sd->kafraPoints=1000;registry[1].value=1000;}
        auto result=mode==10?(pc_paycash(sd.get(),-1,0,LOG_TYPE_CASH)>=0):audit_cashshop_buylist(sd.get(),mode==6?UINT32_MAX:mode==9?1000:0,mode==0?2:mode==5?0:1,purchases);
        bool ok=(mode==2||mode==8)?(result&&count(501)==(mode==2?1:99)&&sd->cashPoints==(mode==2?999900:990100)):
            (!result&&sd->cashPoints==1000000&&count(501)==(mode==0?MAX_AMOUNT-2:0));
        if(mode==9)ok=result&&count(501)==1&&sd->cashPoints==1000000&&sd->kafraPoints==900;
        ok=ok&&registry[0].value==sd->cashPoints&&registry[1].value==sd->kafraPoints;
        std::printf("CASHBUTTON mode=%d result=%d cash=%d outputs=%d %s\n",mode,result,sd->cashPoints,count(501),ok?"PASS":"FAIL");if(!ok)++errors;
        cash_shop_db.clear();
        sd->regs.vars->destroy(sd->regs.vars,nullptr);sd->regs.vars=nullptr;
    }
    if(durable_boundary_calls){std::fprintf(stderr,"Unexpected durable purchase in synchronous fixture\n");++errors;}
    // Real caller + planner/catalog code; transport accepts an immutable plan
    // explicitly, without pretending to perform a character-server commit.
    auto egg_data=item_db.find(501);const auto original_type=egg_data->type;
    egg_data->type=IT_PETEGG;
    auto pet=std::make_shared<s_pet_db>();pet->class_=1002;pet->EggID=501;pet->intimate=250;pet_db.put(1002,pet);
    auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->lv=1;mob->jname="Pet fixture";mob_db.put(1002,mob);
    for(int mode=0;mode<7;++mode) {
        ++cases;durable_boundary_calls=0;durable_captured.reset();durable_allow=mode!=5;
        auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
        sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
        sd->status.inventory_slots=mode==6?1:MAX_INVENTORY;sd->status.zeny=1000;sd->max_weight=1000000;
        for(auto& index:sd->equip_index)index=-1;
        if(mode>=3 && mode!=5)put(0,502,mode==6?1:2);
        weight();const auto before=sd->inventory;const auto before_weight=sd->weight;
        e_purchase_result result;
        if(mode==3 || mode==4 || mode==6) {
            auto shop=std::make_shared<s_npc_barter>();shop->name="pet-fixture";
            auto entry=std::make_shared<s_npc_barter_item>();entry->nameid=501;entry->price=100;
            entry->stockLimited=mode==4;entry->stock=5;entry->index=0;
            auto cost=std::make_shared<s_npc_barter_requirement>();cost->nameid=502;cost->amount=1;cost->refine=-1;entry->requirements[0]=cost;
            std::vector<s_barter_purchase> cart={{entry,1,nullptr}};
            result=audit_npc_barter_purchase(*sd,shop,cart);
        }else{
            npc_data nd{};nd.subtype=(mode==1 || mode==2)?NPCTYPE_MARKETSHOP:NPCTYPE_SHOP;fixture_shop=&nd;
            std::strcpy(nd.exname,"pet-fixture");npc_item_list sale{};sale.nameid=501;sale.value=100;sale.qty=mode==1?5:-1;
            nd.u.shop.count=1;nd.u.shop.shop_item=&sale;
            std::vector<s_npc_buy_list> cart={{1,501}};result=audit_npc_buylist(sd.get(),cart);
        }
        bool ok=durable_boundary_calls==1 && sd->status.zeny==1000 && sd->status.uniqueitem_counter==10 &&
            sd->weight==before_weight && !memcmp(&before,&sd->inventory,sizeof(before));
        if(mode==5)ok=ok && result!=e_purchase_result::PURCHASE_PENDING && !durable_captured;
        else{
            ok=ok && result==e_purchase_result::PURCHASE_PENDING && durable_captured && durable_captured->pet_count==1;
            if(durable_captured){const auto& r=*durable_captured;
                ok=ok && r.wallet_after==900 && r.counter_after==11 && r.pets[0].output.pet_class==1002 &&
                    r.pets[0].output.egg.unique_id==((uint64(99000002)<<32)|10) && r.pets[0].inventory_index==-1;
                if(mode==1 || mode==4)ok=ok && r.stock_count==1 && r.stocks[0].before==5 && r.stocks[0].after==4;
                else ok=ok && r.kind==pn_shop::Asset && !r.stock_count;
                if(mode==3 || mode==4 || mode==6)ok=ok && r.items[0].amount==(mode==6?0:1);
            }
        }
        std::printf("PET_PURCHASE_CALLER mode=%d result=%d %s\n",mode,int(result),ok?"PASS":"FAIL");if(!ok)++errors;
    }
    for(int mode=0;mode<2;++mode) {
        ++cases;durable_boundary_calls=0;durable_captured.reset();durable_allow=true;
        auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;
        sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
        sd->status.inventory_slots=1;sd->status.zeny=1000;sd->max_weight=1000000;
        for(auto& index:sd->equip_index)index=-1;
        put(0,501,1);sd->inventory.u.items_inventory[0].card[0]=CARD0_PET;
        sd->inventory.u.items_inventory[0].card[1]=mode?0:42;weight();const auto before=sd->inventory;
        auto shop=std::make_shared<s_npc_barter>();shop->name="pet-material-fixture";
        auto entry=std::make_shared<s_npc_barter_item>();entry->nameid=502;entry->price=100;entry->stockLimited=false;
        auto cost=std::make_shared<s_npc_barter_requirement>();cost->nameid=501;cost->amount=1;cost->refine=-1;entry->requirements[0]=cost;
        std::vector<s_barter_purchase> cart={{entry,1,nullptr}};
        const auto result=audit_npc_barter_purchase(*sd,shop,cart);
        bool ok=sd->status.zeny==1000 && !memcmp(&before,&sd->inventory,sizeof(before));
        if(!mode)ok=ok && result==e_purchase_result::PURCHASE_PENDING && durable_boundary_calls==1 && durable_captured &&
            durable_captured->pet_retire_count==1 && durable_captured->retired_pets[0].pet_id==42 &&
            durable_captured->kind==pn_shop::Asset && durable_captured->items[0].nameid==502 && durable_captured->wallet_after==900;
        else ok=ok && result!=e_purchase_result::PURCHASE_PENDING && !durable_boundary_calls;
        std::printf("PET_MATERIAL_CALLER mode=%d result=%d %s\n",mode,int(result),ok?"PASS":"FAIL");if(!ok)++errors;
    }
    // Both NPC packet generations and the cash button preserve all live assets
    // until the durable boundary accepts/rejects the whole pet cart.
    for(int mode=0;mode<12;++mode) {
        ++cases;durable_boundary_calls=0;durable_captured.reset();durable_allow=mode!=2;
        auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;sd->permissions.set(PC_PERM_TRADE);sd->group=std::make_shared<s_player_group>();
        sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.uniqueitem_counter=10;
        sd->status.inventory_slots=mode==4?1:MAX_INVENTORY;sd->status.zeny=1000;
        sd->cashPoints=mode==3?50:1000;sd->kafraPoints=100;sd->max_weight=1000000;
        for(auto& index:sd->equip_index)index=-1;
        const bool item_shop=mode>=4 && mode<=8;
        npc_data nd{};nd.subtype=item_shop?NPCTYPE_ITEMSHOP:NPCTYPE_CASHSHOP;fixture_shop=&nd;
        npc_item_list sales[2]{};sales[0].nameid=501;sales[0].value=item_shop?10:100;
        sales[1].nameid=502;sales[1].value=item_shop?1:10;nd.u.shop.count=2;nd.u.shop.shop_item=sales;
        if(item_shop){nd.u.shop.itemshop_nameid=502;put(0,502,mode==4?10:4);if(mode!=4)put(1,502,6);}
        if(mode==6)sd->inventory.u.items_inventory[1].equipSwitch=1;
        if(mode==7)sd->inventory.u.items_inventory[1].amount=5;
        weight();const auto before=sd->inventory;const auto before_weight=sd->weight;
        int result=0;
        if(mode>=9){
            auto tab=std::make_shared<s_cash_item_tab>();tab->tab=CASHSHOP_TAB_NEW;
            auto entry=std::make_shared<s_cash_item>();entry->nameid=501;entry->price=100;tab->items.push_back(entry);cash_shop_db.put(CASHSHOP_TAB_NEW,tab);
            if(mode==11){put(0,502,1);sd->status.inventory_slots=1;weight();}
            PACKET_CZ_SE_PC_BUY_CASHITEM_LIST_sub cart{};cart.itemId=501;cart.amount=mode==10?2:1;cart.tab=CASHSHOP_TAB_NEW;
            result=audit_cashshop_buylist(sd.get(),50,1,&cart)?pn_shop::cash_pending:ERROR_TYPE_PURCHASE_FAIL;
            cash_shop_db.clear();
        }else if(mode==1 || mode==8)result=audit_npc_cashshop_buy(sd.get(),501,1,item_shop?0:25);
        else{std::vector<s_npc_buy_list> cart={{1,501}};result=audit_npc_cashshop_buylist(sd.get(),item_shop?0:mode==3?0:25,cart);}
        const bool accepted=mode!=2 && mode!=3 && mode!=6 && mode!=7;
        bool ok=result==(accepted?pn_shop::cash_pending:mode==3?ERROR_TYPE_MONEY:ERROR_TYPE_PURCHASE_FAIL) &&
            sd->cashPoints==(mode==3?50:1000) && sd->kafraPoints==100 && sd->status.uniqueitem_counter==10;
        if(mode!=11)ok=ok && sd->weight==before_weight && !memcmp(&before,&sd->inventory,sizeof(before));
        if(accepted){
            ok=ok && durable_boundary_calls==1 && durable_captured && durable_captured->kind==pn_shop::Asset &&
                durable_captured->response==(mode>=9?pn_shop::CashButtonResponse:pn_shop::CashNpcResponse) &&
                durable_captured->pet_count==(mode==10?2:1);
            if(durable_captured){const auto& r=*durable_captured;
                if(item_shop)ok=ok && r.items[0].nameid==0 && r.items[1].nameid==0 && r.cash_after==1000 && r.kafra_after==100;
                else ok=ok && r.cash_after==(mode>=9?(mode==10?850:950):925) && r.kafra_after==(mode>=9?50:75);
                if(mode==11)ok=ok && r.items[0].nameid==502 && r.items[0].amount==1;
            }
        }else ok=ok && !durable_captured;
        std::printf("PAID_PET_CALLER mode=%d result=%d %s\n",mode,result,ok?"PASS":"FAIL");if(!ok)++errors;
    }
    for(int mode=0;mode<2;++mode) {
        ++cases;durable_boundary_calls=0;durable_captured.reset();durable_allow=true;
        auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->type=BL_PC;sd->permissions.set(PC_PERM_TRADE);sd->group=std::make_shared<s_player_group>();
        sd->status.account_id=99001;sd->status.char_id=99000002;sd->status.inventory_slots=1;sd->max_weight=1000000;
        for(auto& index:sd->equip_index)index=-1;
        put(0,501,1);sd->inventory.u.items_inventory[0].card[0]=CARD0_PET;sd->inventory.u.items_inventory[0].card[1]=mode?0:42;weight();
        const auto before=sd->inventory;
        npc_data nd{};nd.subtype=NPCTYPE_ITEMSHOP;nd.u.shop.itemshop_nameid=501;fixture_shop=&nd;
        npc_item_list sale{};sale.nameid=502;sale.value=1;nd.u.shop.count=1;nd.u.shop.shop_item=&sale;
        std::vector<s_npc_buy_list> cart={{1,502}};const auto result=audit_npc_cashshop_buylist(sd.get(),0,cart);
        bool ok=!memcmp(&before,&sd->inventory,sizeof(before));
        if(!mode)ok=ok && result==pn_shop::cash_pending && durable_captured && durable_captured->pet_retire_count==1 &&
            durable_captured->retired_pets[0].pet_id==42 && durable_captured->items[0].nameid==502;
        else ok=ok && result==ERROR_TYPE_PURCHASE_FAIL && !durable_captured;
        std::printf("ITEMSHOP_PET_CURRENCY mode=%d result=%d %s\n",mode,result,ok?"PASS":"FAIL");if(!ok)++errors;
    }
    durable_allow=false;durable_captured.reset();egg_data->type=original_type;
    pet_db.clear();mob_db.clear();pet.reset();mob.reset();egg_data.reset();
    attached=nullptr;item_db.clear();do_final_script();timer_final();db_final();malloc_final();return errors?1:0;
}
