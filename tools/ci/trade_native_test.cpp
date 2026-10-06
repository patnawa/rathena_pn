// GPL-3.0-or-later. Actual trade, inventory, pair snapshot and quest VM paths.
#include <map/trade.hpp>
#include <map/storage.hpp>
#include <map/pc_groups.hpp>
#include <map/npc.hpp>
#include <map/quest.hpp>
#include <map/intif.hpp>
#include <common/socket.hpp>
extern int32 char_fd;
int32 trade_check(map_session_data*,map_session_data*);
int32 impossible_trade_check(map_session_data*);
namespace { bool callback_safe=true;unsigned samples=0; }
extern "C" int32 connected_boundary() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int32 connected_boundary(){return 1;}
extern "C" bool bounded_boundary(const map_session_data*) asm("__wrap__Z25pc_can_give_bounded_itemsPK16map_session_data");
extern "C" bool bounded_boundary(const map_session_data*){return false;}
extern "C" const char* locale_boundary(const map_session_data*,int32) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* locale_boundary(const map_session_data*,int32){return "native trade fixture";}
extern "C" void item_ack(map_session_data&,int32,e_exitem_add_result) asm("__wrap__Z16clif_tradeitemokR16map_session_datai19e_exitem_add_result");
extern "C" void item_ack(map_session_data&,int32,e_exitem_add_result){}
extern "C" void item_packet(map_session_data*,map_session_data*,int32,int32) asm("__wrap__Z17clif_tradeadditemP16map_session_dataS0_ii");
extern "C" void item_packet(map_session_data*,map_session_data*,int32,int32){}
extern "C" void canceled_packet(const map_session_data&) asm("__wrap__Z19clif_tradecancelledRK16map_session_data");
extern "C" void canceled_packet(const map_session_data&){}
extern "C" void completed_packet(const map_session_data&) asm("__wrap__Z19clif_tradecompletedRK16map_session_data");
extern "C" void completed_packet(const map_session_data&){}
extern "C" void lock_packet(map_session_data&,bool) asm("__wrap__Z19clif_tradedeal_lockR16map_session_datab");
extern "C" void lock_packet(map_session_data&,bool){}
extern "C" void rental_packet(const map_session_data*,t_itemid,int32) asm("__wrap__Z16clif_rental_timePK16map_session_dataji");
extern "C" void rental_packet(const map_session_data*,t_itemid,int32){}
extern "C" void rental_expired_packet(const map_session_data*,int32,t_itemid) asm("__wrap__Z19clif_rental_expiredPK16map_session_dataij");
extern "C" void rental_expired_packet(const map_session_data*,int32,t_itemid){}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code* code,map_session_data* sd){
    ++samples;
    if(actors[0]->inventory.u.items_inventory[0].nameid==909||actors[1]->inventory.u.items_inventory[0].nameid==502)callback_safe=false;
    return real_condition(code,sd);
}
namespace {
struct Pair {
    RunePlayer a=rune_player(),b=rune_player();
    Pair(){
        a->group=b->group=std::make_shared<s_player_group>();b->id=b->status.account_id=99000004;b->status.char_id=99000005;
        actors={a.get(),b.get()};attached=a.get();
        a->state.trading=b->state.trading=1;a->trade_partner.id=b->id;b->trade_partner.id=a->id;
        a->bank_ui.trade_id=b->bank_ui.trade_id=1;a->bank_ui.trade_revision=b->bank_ui.trade_revision=1;
        callback_safe=true;samples=0;char_fd=0;
    }
    ~Pair(){a->pair_commit={};b->pair_commit={};actors.clear();}
    void stock(map_session_data& sd,int index,int id,int amount){attached=&sd;put(index,id,amount);weight();}
    void commit(){a->state.deal_locked=b->state.deal_locked=1;trade_tradecommit(a.get());trade_tradecommit(b.get());}
};
void repeat_offer(){
    Pair p;p.stock(*p.a,0,909,10);p.b->status.inventory_slots=1;
    trade_tradeadditem(p.a.get(),0,3);trade_tradeadditem(p.a.get(),0,4);
    std::printf("TRADE_REPEAT_PROBE amount=%d slots=%d\n",p.a->deal.item[0].amount,p.a->deal.inventory_space);std::fflush(stdout);
    check(p.a->deal.item[0].amount==7&&p.a->deal.inventory_space==1,"repeated native additions reserve one recipient slot");
}
void stale_cancel(){
    Pair p;p.b->trade_partner.id=99000007;p.b->bank_ui.trade_id=42;p.b->state.isBoundTrading=1;
    const auto before=p.b->deal;trade_tradecancel(p.a.get());
    check(p.b->trade_partner.id==99000007&&p.b->bank_ui.trade_id==42&&p.b->state.trading&&p.b->state.isBoundTrading&&!memcmp(&before,&p.b->deal,sizeof(before)),"stale cancellation leaves partner's newer trade intact");
}
void metadata(){
    Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,909,5);auto& source=p.a->inventory.u.items_inventory[0];source.option[0].id=1;source.option[0].value=10;
    const auto expected=source;p.a->deal.item[0].index=0;p.a->deal.item[0].amount=10;p.commit();
    std::printf("TRADE_METADATA_PROBE first_amount=%d second_amount=%d option=%d\n",p.b->inventory.u.items_inventory[0].amount,p.b->inventory.u.items_inventory[1].amount,p.b->inventory.u.items_inventory[1].option[0].id);std::fflush(stdout);
    check(p.b->inventory.u.items_inventory[0].amount==5&&p.b->inventory.u.items_inventory[1].amount==10&&compare_item(&p.b->inventory.u.items_inventory[1],const_cast<item*>(&expected)),"native trade retains different option metadata in a separate stack");
    check(p.a->pair_commit.pending&&p.b->pair_commit.pending,"native pair submit owns matching durable snapshots");
}
void callback(){
    Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,502,7);p.a->deal.item[0].index=p.b->deal.item[0].index=0;p.a->deal.item[0].amount=10;p.b->deal.item[0].amount=7;
    npc_data nd{};nd.id=NPC;nd.type=BL_NPC;quest_npc=fake_nd=&nd;map_num=1;map[0].qi_npc={NPC};p.a->qi_display.resize(1);p.b->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
    qi->condition=compile("{ @TradeCallback=countitem(909); achievement_condition(0); }","actual trade callback");nd.qi_data.push_back(qi);
    p.commit();std::printf("TRADE_CALLBACK_PROBE safe=%d samples=%u\n",callback_safe,samples);std::fflush(stdout);
    check(samples&&callback_safe,"both actors' real quest callbacks see all trade source removals completed");
    check(p.a->pair_commit.request==p.b->pair_commit.request&&p.a->pair_commit.pending,"pair snapshot submits after complete delivery");
    map[0].qi_npc.clear();quest_npc=fake_nd=nullptr;
}
void bounds(){
    for(int mode=0;mode<8;++mode){Pair p;p.stock(*p.a,0,909,10);p.a->deal.item[0].amount=1;
        if(mode==0)p.a->deal.item[0].index=-1;
        if(mode==1)p.a->deal.item[0].index=MAX_INVENTORY;
        if(mode==2)p.a->deal.item[0].amount=-1;
        if(mode==3)p.a->deal.item[0].amount=11;
        if(mode==4)p.a->inventory.u.items_inventory[0].nameid=UINT32_MAX;
        if(mode==5)p.a->inventory_data[0]=nullptr;
        if(mode==6)p.a->deal.zeny=-1;
        if(mode==7){p.b->status.inventory_slots=1;p.stock(*p.b,0,501,1);}
        check(!trade_check(p.a.get(),p.b.get()),"native preflight rejects invalid offers and real slot limits without memory errors");
    }
}
void controls(){
    {Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,909,5);trade_tradeadditem(p.a.get(),0,10);check(p.a->deal.item[0].amount==10,"identical item offer accepted");p.commit();check(!p.a->inventory.u.items_inventory[0].nameid&&p.b->inventory.u.items_inventory[0].amount==15,"identical metadata stacks and exact total conserved");auto request=p.a->pair_commit.request;trade_tradecommit(p.a.get());trade_tradecancel(p.b.get());check(p.a->pair_commit.request==request&&p.b->pair_commit.request==request,"duplicate confirmation/cancel cannot replace pending commit");}
    {Pair p;p.a->state.isBoundTrading=p.b->state.isBoundTrading=1;p.a->deal.weight=p.b->deal.weight=100;p.a->deal.inventory_space=p.b->deal.inventory_space=1;trade_tradecancel(p.a.get());check(!p.a->state.isBoundTrading&&!p.b->state.isBoundTrading&&!p.a->deal.weight&&!p.b->deal.inventory_space,"cancel clears complete offers and both bound flags");}
    {Pair p;p.a->status.zeny=INT64_MAX-100;p.b->status.zeny=100;trade_tradeaddzeny(p.b.get(),100);p.commit();check(p.a->status.zeny==INT64_MAX&&p.b->status.zeny==0,"full-width wallet transfer reaches exact int64 limit");}
}
void identity_matrix(){
    // Inventory stack identity fields, including all random option members.
    for(int mode=0;mode<26;++mode){Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,909,5);
        auto& it=p.a->inventory.u.items_inventory[0];
        if(mode==0)it.identify=0;if(mode==1)it.refine=1;if(mode==2)it.attribute=1;
        if(mode==3)it.enchantgrade=1;if(mode==4)it.bound=BOUND_GUILD;if(mode==5)it.unique_id=UINT64_MAX;
        if(mode==6)it.expire_time=2100000000;
        if(mode>=7&&mode<11)it.card[mode-7]=4365;
        if(mode>=11){auto& opt=it.option[(mode-11)/3];if((mode-11)%3==0)opt.id=1;if((mode-11)%3==1)opt.value=10;if((mode-11)%3==2)opt.param=2;}
        const auto original=it;
        check(pc_additem(p.b.get(),&it,10,LOG_TYPE_TRADE)==ADDITEM_SUCCESS,"actual native inventory insertion accepts identity variant");
        check(p.b->inventory.u.items_inventory[0].amount==5&&p.b->inventory.u.items_inventory[1].amount==10&&compare_item(&p.b->inventory.u.items_inventory[1],const_cast<item*>(&original)),"every unequal identity retains metadata in separate inventory stack");
    }
}
void offer_and_state_matrix(){
    for(int mode=0;mode<9;++mode){Pair p;p.stock(*p.a,0,909,10);p.a->deal.item[0].index=0;p.a->deal.item[0].amount=3;
        if(mode==0)p.b->trade_partner.id++;
        if(mode==1)p.b->bank_ui.trade_id++;
        if(mode==2)p.b->state.trading=0;
        if(mode==3)p.b->multi_storage.pending=true;
        if(mode==4)p.a->multi_storage.pending=true;
        if(mode==5)p.b->shop_commit.pending=true;
        if(mode==6)p.a->bank_ui.pending=true;
        if(mode==7)p.b->mail_companion.pending=true;
        if(mode==8)p.a->pair_commit.pending=true;
        auto a_inventory=p.a->inventory,b_inventory=p.b->inventory;const int64 a_zeny=p.a->status.zeny,b_zeny=p.b->status.zeny;
        trade_tradeadditem(p.a.get(),0,2);trade_tradeaddzeny(p.a.get(),1);trade_tradeok(p.a.get());p.commit();
        check(!memcmp(&a_inventory,&p.a->inventory,sizeof(a_inventory))&&!memcmp(&b_inventory,&p.b->inventory,sizeof(b_inventory))&&a_zeny==p.a->status.zeny&&b_zeny==p.b->status.zeny,"stale or busy actor never mutates either inventory/wallet");
        check(!p.a->pair_commit.request&&!p.b->pair_commit.request,"stale or busy confirmations never start a pair decision");
    }
    for(int mode=0;mode<5;++mode){Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,909,MAX_AMOUNT);p.b->status.inventory_slots=2;
        if(mode==0)p.a->inventory.u.items_inventory[0].option[0].id=1;
        if(mode==1){auto data=item_db.find(909);auto saved=data->stack;data->stack.inventory=true;data->stack.amount=5;
            trade_tradeadditem(p.a.get(),0,6);check(!p.a->deal.item[0].amount,"configured item cap rejects oversized offer");data->stack=saved;continue;}
        if(mode==2)p.b->max_weight=0;
        if(mode==3)p.a->inventory_data[0]=nullptr;
        if(mode==4)p.a->inventory.u.items_inventory[0].nameid=UINT32_MAX;
        trade_tradeadditem(p.a.get(),0,3);
        check(p.a->deal.item[0].amount==(mode==0?3:0),"metadata-aware offer ignores unrelated full stack but enforces limits");
    }
    {Pair p;p.stock(*p.a,0,909,10);trade_tradeadditem(p.a.get(),0,8);trade_tradeadditem(p.a.get(),0,8);
        check(p.a->deal.item[0].amount==10&&p.a->deal.inventory_space==1&&p.a->deal.weight==item_db.find(909)->weight*10,"repeated amount clamps before weight and slot accounting");}
    {Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,909,5);p.b->status.inventory_slots=1;battle_config.trade_count_stackable=0;
        trade_tradeadditem(p.a.get(),0,10);check(p.a->deal.item[0].amount==10&&!p.a->deal.inventory_space,"configured stacking permits identical stack with no empty slot");battle_config.trade_count_stackable=1;}
    {Pair p;p.stock(*p.a,0,909,10);p.stock(*p.b,0,502,7);p.a->deal.item[0].index=p.b->deal.item[0].index=0;p.a->deal.item[0].amount=4;p.b->deal.item[0].amount=2;
        p.a->deal.zeny=300;p.b->deal.zeny=50;const int64 a_before=p.a->status.zeny,b_before=p.b->status.zeny;p.commit();
        check(p.a->status.zeny==a_before-250&&p.b->status.zeny==b_before+250&&p.a->inventory.u.items_inventory[0].amount==6&&p.b->inventory.u.items_inventory[0].amount==5,"bilateral partial items and opposing wallets conserve exact values");
        check(p.a->pair_commit.request->side[0].wallet_after==p.b->status.zeny&&p.a->pair_commit.request->side[1].wallet_after==p.a->status.zeny,"real submitted pair snapshot captures both settled wallets");}
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");deny_network();static char name[]="trade-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;map[0].instance_id=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"trade:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"trade:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto data=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:tree["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    if(argc==2||!strcmp(argv[2],"repeat"))repeat_offer();
    if(argc==2||!strcmp(argv[2],"metadata"))metadata();
    if(argc==2||!strcmp(argv[2],"cancel"))stale_cancel();
    if(argc==2||!strcmp(argv[2],"callback"))callback();
    if(argc==2||!strcmp(argv[2],"bounds"))bounds();
    if(argc==2){controls();identity_matrix();offer_and_state_matrix();}
    check(!errors,"no script errors");attached=nullptr;item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("TRADE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
