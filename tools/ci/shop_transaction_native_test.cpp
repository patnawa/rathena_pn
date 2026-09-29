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
    attached=nullptr;item_db.clear();do_final_script();timer_final();db_final();malloc_final();return errors?1:0;
}
