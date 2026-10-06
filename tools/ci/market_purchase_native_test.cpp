// GPL-3.0-or-later. Actual vending/buying-store mutation and pair snapshot paths.
#include <map/vending.hpp>
#include <map/buyingstore.hpp>
#include <map/storage.hpp>
#include <map/pc_groups.hpp>
#include <map/npc.hpp>
#include <map/quest.hpp>
#include <map/intif.hpp>
extern int32 char_fd;
namespace { bool safe=true, buying=false;unsigned samples=0; }
extern "C" int32 connected() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int32 connected(){return 1;}
extern "C" bool bounded(const map_session_data*) asm("__wrap__Z25pc_can_give_bounded_itemsPK16map_session_data");
extern "C" bool bounded(const map_session_data*){return false;}
extern "C" const char* locale(const map_session_data*,int32) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* locale(const map_session_data*,int32){return "native market fixture";}
extern "C" bool remote(map_session_data&,int32) asm("__wrap__Z22searchstore_queryremoteR16map_session_datai");
extern "C" bool remote(map_session_data&,int32){return false;}
extern "C" void clear_remote(map_session_data&) asm("__wrap__Z23searchstore_clearremoteR16map_session_data");
extern "C" void clear_remote(map_session_data&){}
extern "C" void failed_seller(const map_session_data*,int16,t_itemid) asm("__wrap__Z36clif_buyingstore_trade_failed_sellerPK16map_session_datasj");
extern "C" void failed_seller(const map_session_data*,int16,t_itemid){}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition(script_code* code,map_session_data* sd){
    ++samples;auto* seller=actors[0];auto* buyer=actors[1];
    if(buying){if(seller->inventory.u.items_inventory[0].nameid||buyer->buyingstore.items[0].amount||buyer->status.zeny!=7654321-1000||seller->status.zeny!=7654321+1000)safe=false;}
    else if(seller->cart.u.items_cart[0].nameid||seller->vend_num||buyer->status.zeny!=7654321-1000||seller->bank_vault!=1000)safe=false;
    return real_condition(code,sd);
}
namespace {
struct Pair {
    RunePlayer seller=rune_player(),buyer=rune_player();
    Pair(bool buy=false){buying=buy;seller->group=buyer->group=std::make_shared<s_player_group>();buyer->id=buyer->status.account_id=99000004;buyer->status.char_id=99000005;
        actors={seller.get(),buyer.get()};attached=seller.get();char_fd=0;safe=true;samples=0;
        seller->permissions.set(PC_PERM_TRADE);buyer->permissions.set(PC_PERM_TRADE);
        seller->rental_timer=buyer->rental_timer=INVALID_TIMER;
        seller->state.trading=buyer->state.trading=0;
        auto* shop=buy?buyer.get():seller.get();shop->market.published=true;
        if(buy){shop->state.buyingstore=true;shop->buyer_id=12;shop->buyingstore.slots=1;shop->buyingstore.zenylimit=10000;shop->buyingstore.items[0]={100,10,909};}
        else{shop->state.vending=true;shop->vender_id=12;shop->vend_num=1;shop->vending[0]={0,10,100};seller->sc.option|=OPTION_CART;seller->sc.createSCE(SC_PUSH_CART);}
    }
    ~Pair(){seller->pair_commit={};buyer->pair_commit={};actors.clear();}
    void stock(map_session_data& sd,int i,int amount){attached=&sd;put(i,909,amount);weight();}
    void cart(int i,int amount){stock(*seller,i,amount);seller->cart.u.items_cart[i]=seller->inventory.u.items_inventory[i];seller->cart_weight+=item_db.find(909)->weight*amount;++seller->cart_num;seller->inventory.u.items_inventory[i]={};seller->inventory_data[i]=nullptr;weight();}
    void purchase(int amount=10){uint16 packet[]={static_cast<uint16>(amount),2};vending_purchasereq(buyer.get(),seller->id,12,reinterpret_cast<uint8*>(packet),1,true);}
    void sell(int amount=10){PACKET_CZ_REQ_TRADE_BUYING_STORE_sub packet{};packet.index=2;packet.itemId=909;packet.amount=amount;buyingstore_trade(seller.get(),buyer->id,12,&packet,1,true);}
};
void callback(bool buy){
    Pair p(buy);if(buy)p.stock(*p.seller,0,10);else p.cart(0,10);
    npc_data nd{};nd.id=NPC;nd.type=BL_NPC;quest_npc=fake_nd=&nd;map_num=1;map[0].qi_npc={NPC};p.seller->qi_display.resize(1);p.buyer->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;qi->condition=compile("{ @MarketCallback=countitem(909); achievement_condition(0); }","actual market callback");nd.qi_data.push_back(qi);
    if(buy)p.sell();else p.purchase();
    std::printf("MARKET_CALLBACK_PROBE buying=%d safe=%d samples=%u\n",buy,safe,samples);std::fflush(stdout);
    check(samples&&safe,"real quest callbacks see all stock, listing and money changes completed");
    check(p.seller->pair_commit.pending&&p.buyer->pair_commit.pending,"settled native market submits durable pair");
    map[0].qi_npc.clear();quest_npc=fake_nd=nullptr;
}
void capacity(){
    Pair p;p.cart(0,10);p.seller->cart.u.items_cart[0].option[0].id=1;p.stock(*p.buyer,0,MAX_AMOUNT);p.buyer->status.inventory_slots=2;p.purchase();
    std::printf("MARKET_CAPACITY_PROBE added=%d\n",p.buyer->inventory.u.items_inventory[1].amount);std::fflush(stdout);
    check(p.buyer->inventory.u.items_inventory[0].amount==MAX_AMOUNT&&p.buyer->inventory.u.items_inventory[1].amount==10,"unrelated full stack does not reject different metadata");
}
void busy(){
    Pair p(true);p.stock(*p.seller,0,10);p.buyer->status.zeny=1000;p.buyer->bank_ui.pending=true;const auto budget=p.buyer->buyingstore.zenylimit;p.sell();
    std::printf("MARKET_BUSY_PROBE budget=%lld\n",static_cast<long long>(p.buyer->buyingstore.zenylimit));std::fflush(stdout);
    check(p.buyer->buyingstore.zenylimit==budget&&p.seller->inventory.u.items_inventory[0].amount==10,"busy transaction leaves buying order budget and assets unchanged");
}
void cart_invalid(){
    Pair p;p.cart(0,10);p.seller->cart_num=0;p.purchase();
    std::printf("MARKET_CART_PROBE source=%d delivered=%d\n",p.seller->cart.u.items_cart[0].amount,p.buyer->inventory.u.items_inventory[0].amount);std::fflush(stdout);
    check(p.seller->cart.u.items_cart[0].amount==10&&!p.buyer->inventory.u.items_inventory[0].nameid&&p.buyer->status.zeny==7654321,"invalid cart counters cannot duplicate stock");
}
void wide_prices(){
    const int64 prices[]={3000000000LL,9007199254740993LL,INT64_MAX};
    for(int64 price:prices){
        Pair p;p.cart(0,1);p.seller->vending[0].amount=1;p.seller->vending[0].value=price;p.buyer->status.zeny=price;
        p.purchase(1);
        check(p.buyer->pair_commit.pending&&p.seller->pair_commit.pending,"wide purchase submits one durable pair");
        check(p.buyer->status.zeny==0 && p.seller->bank_vault==price,"wide debit and seller bank credit remain exact");
        check(p.buyer->inventory.u.items_inventory[0].amount==1 && !p.seller->cart.u.items_cart[0].nameid,"wide purchase transfers one item");
        check(pn_pair::conserved(*p.buyer->pair_commit.request),"wide purchase receipt conserves money");
    }
    {
        Pair p;p.cart(0,1);p.seller->vending[0].amount=1;p.seller->vending[0].value=3000000000LL;
        p.buyer->status.zeny=3000000000LL;p.seller->bank_vault=MAX_BANK_ZENY-3000000000LL;p.purchase(1);
        check(p.buyer->pair_commit.pending && p.seller->bank_vault==MAX_BANK_ZENY,"seller may reach the exact bank limit");
    }
    for(int boundary=0;boundary<3;++boundary){
        Pair p;p.cart(0,2);p.seller->vending[0].amount=2;p.seller->vending[0].value=INT64_MAX;p.buyer->status.zeny=INT64_MAX;
        if(boundary==0)p.seller->bank_vault=1;
        if(boundary==1)--p.buyer->status.zeny;
        const int64 wallet=p.buyer->status.zeny,bank=p.seller->bank_vault;
        p.purchase(boundary==2?2:1);
        check(!p.buyer->pair_commit.pending && !p.seller->pair_commit.pending,"overflow or insufficient wallet never begins payment");
        check(p.buyer->status.zeny==wallet && p.seller->bank_vault==bank && p.seller->cart.u.items_cart[0].amount==2 && !p.buyer->inventory.u.items_inventory[0].nameid,"refused wide purchase leaves assets unchanged");
    }
    {
        Pair p;p.cart(0,1);p.seller->vending[0].amount=1;p.seller->vending[0].value=INT64_MAX;p.buyer->status.zeny=INT64_MAX;
        battle_config.vending_tax=500;p.purchase(1);battle_config.vending_tax=0;
        check(p.buyer->pair_commit.pending && p.buyer->status.zeny==0 && p.seller->bank_vault==8762203435012037016LL,"maximum-price sale applies five percent tax without overflow");
        check(pn_pair::conserved(*p.buyer->pair_commit.request),"maximum-price taxed receipt conserves money");
    }
    std::printf("MARKET_WIDE_PRICE_PASS cases=8 max=%lld\n",static_cast<long long>(INT64_MAX));
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");deny_network();static char name[]="market-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;battle_config.vending_tax=0;battle_config.feature_buying_store=1;map[0].instance_id=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"market:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"market:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto data=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:tree["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    if(argc==2||!strcmp(argv[2],"callback"))callback(false);
    if(argc==2||!strcmp(argv[2],"buying"))callback(true);
    if(argc==2||!strcmp(argv[2],"capacity"))capacity();
    if(argc==2||!strcmp(argv[2],"busy"))busy();
    if(argc==2||!strcmp(argv[2],"cart"))cart_invalid();
    if(argc==2||!strcmp(argv[2],"wide"))wide_prices();
    check(!errors,"no script errors");attached=nullptr;item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("MARKET_PURCHASE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
