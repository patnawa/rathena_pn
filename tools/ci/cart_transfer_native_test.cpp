// GPL-3.0-or-later. Actual inventory/cart, stockall and quest VM operations.
#include <map/storage.hpp>
#include <map/pc_groups.hpp>
#include <map/npc.hpp>
#include <map/quest.hpp>
#include <map/achievement.hpp>
#include <common/nullpo.hpp>
#include <common/showmsg.hpp>
#include <common/socket.hpp>
void clif_parse_PutItemToCart(int32,map_session_data*);
void clif_parse_GetItemFromCart(int32,map_session_data*);
int32 audit_stockall(const int32,map_session_data*,const char*,const char*);
char atcmd_output[CHAT_SIZE_MAX];
namespace { bool callback_safe=true,cart_blocked=false;unsigned samples=0;int callback_cart_id=909; }
extern "C" int32 flag_boundary(int16,e_mapflag,u_mapflag_args*) asm("__wrap__Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args");
extern "C" int32 flag_boundary(int16,e_mapflag flag,u_mapflag_args*){return flag==MF_NOUSECART&&cart_blocked;}
extern "C" const char* locale_boundary(const map_session_data*,int32) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* locale_boundary(const map_session_data*,int32 id){return id==1535?"%d items are transferred (%d skipped)!":"cart fixture message";}
extern "C" bool bounded_boundary(const map_session_data*) asm("__wrap__Z25pc_can_give_bounded_itemsPK16map_session_data");
extern "C" bool bounded_boundary(const map_session_data*){return false;}
extern "C" void cart_added(const map_session_data*,int32,int32) asm("__wrap__Z17clif_cart_additemPK16map_session_dataii");
extern "C" void cart_added(const map_session_data*,int32,int32){}
extern "C" void cart_removed(const map_session_data&,int32,int32) asm("__wrap__Z17clif_cart_delitemRK16map_session_dataii");
extern "C" void cart_removed(const map_session_data&,int32,int32){}
extern "C" void cart_ack(const map_session_data&,e_ack_additem_to_cart) asm("__wrap__Z21clif_cart_additem_ackRK16map_session_data21e_ack_additem_to_cart");
extern "C" void cart_ack(const map_session_data&,e_ack_additem_to_cart){}
extern "C" void rental_packet(const map_session_data*,t_itemid,int32) asm("__wrap__Z16clif_rental_timePK16map_session_dataji");
extern "C" void rental_packet(const map_session_data*,t_itemid,int32){}
extern "C" void rental_expired_packet(const map_session_data*,int32,t_itemid) asm("__wrap__Z19clif_rental_expiredPK16map_session_dataij");
extern "C" void rental_expired_packet(const map_session_data*,int32,t_itemid){}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code* code,map_session_data* sd){
    ++samples;for(const auto& it:sd->cart.u.items_cart)if(it.nameid==callback_cart_id&&it.amount>0)callback_safe=false;
    return real_condition(code,sd);
}
namespace {
RunePlayer setup(){
    auto sd=rune_player();sd->group=std::make_shared<s_player_group>();sd->cart_weight_max=100000000;
    sd->sc.createSCE(SC_PUSH_CART);sd->sc.option|=OPTION_CART;sd->rental_timer=INVALID_TIMER;
    callback_safe=true;cart_blocked=false;samples=0;callback_cart_id=909;return sd;
}
int cart_count(int id){int n=0;for(const auto& it:attached->cart.u.items_cart)if(it.nameid==id)n+=it.amount;return n;}
void cart_stock(int index=0,int id=909,int amount=10){
    put(0,id,amount);attached->cart.u.items_cart[index]=attached->inventory.u.items_inventory[0];clear_items();
    attached->cart_num++;attached->cart_weight+=item_db.find(id)->weight*amount;
}
struct CartSnapshot {
    decltype(map_session_data::inventory) inventory=attached->inventory;
    decltype(map_session_data::cart) cart=attached->cart;
    uint32 weight=attached->weight;int32 cart_weight=attached->cart_weight,cart_num=attached->cart_num;
    int64 zeny=attached->status.zeny;
    void unchanged(){check(!memcmp(&inventory,&attached->inventory,sizeof(inventory))&&!memcmp(&cart,&attached->cart,sizeof(cart)),"refusal preserves entire inventory and cart");
        check(weight==attached->weight&&cart_weight==attached->cart_weight&&cart_num==attached->cart_num&&zeny==attached->status.zeny,"refusal preserves both weights count and full-width wallet");}
};
void callback(npc_data& nd){
    nd.id=NPC;nd.type=BL_NPC;quest_npc=fake_nd=&nd;map_num=1;map[0].qi_npc={NPC};attached->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
    qi->condition=compile("{ @CartSeen=cartcountitem(909); @InventorySeen=countitem(909); achievement_condition(0); }","actual cart transfer callback");nd.qi_data.push_back(qi);
}
void clear_callback(){map[0].qi_npc.clear();quest_npc=fake_nd=nullptr;}
void withdraw_callback(){
    auto sd=setup();cart_stock();npc_data nd{};callback(nd);check(pc_getitemfromcart(sd.get(),0,10),"native withdrawal succeeds");
    std::printf("CART_CALLBACK_PROBE safe=%d samples=%u inventory=%d cart=%d seen_cart=%lld\n",callback_safe,samples,count(909),cart_count(909),(long long)pc_readreg(sd.get(),add_str("@CartSeen")));std::fflush(stdout);
    check(samples&&callback_safe&&pc_readreg(sd.get(),add_str("@CartSeen"))==0&&pc_readreg(sd.get(),add_str("@InventorySeen"))==10,"actual quest callback runs after cart source removal");clear_callback();
}
void pending_deposit(){
    auto sd=setup();put(0,909,10);weight();sd->bank_ui.pending=true;CartSnapshot before;pc_putitemtocart(sd.get(),0,10);
    std::printf("CART_PENDING_DEPOSIT_PROBE inventory=%d cart=%d\n",count(909),cart_count(909));std::fflush(stdout);before.unchanged();
}
void pending_stockall(){
    auto sd=setup();cart_stock();sd->bank_ui.pending=true;CartSnapshot before;audit_stockall(0,sd.get(),"@stockall","");
    std::printf("CART_PENDING_STOCKALL_PROBE inventory=%d cart=%d\n",count(909),cart_count(909));std::fflush(stdout);before.unchanged();
}
void failed_capacity(){
    auto sd=setup();cart_stock();put(0,501,1);weight();sd->status.inventory_slots=1;CartSnapshot before;
    const bool result=pc_getitemfromcart(sd.get(),0,10);before.unchanged();
    std::printf("CART_CAPACITY_PROBE result=%d\n",result);std::fflush(stdout);check(!result,"failed inventory capacity reports failure to bulk caller");
    audit_stockall(0,sd.get(),"@stockall","");check(!messages.empty()&&messages.back()=="0 items are transferred (10 skipped)!","stockall displays successful and skipped amounts accurately");before.unchanged();
}
void negative_delete(){
    auto sd=setup();cart_stock();CartSnapshot before;pc_cart_delitem(sd.get(),0,-1,0,LOG_TYPE_NONE);
    std::printf("CART_NEGATIVE_DELETE_PROBE amount=%d\n",cart_count(909));std::fflush(stdout);before.unchanged();
}
void oversized_add(){
    auto sd=setup();item incoming{};incoming.nameid=909;incoming.identify=1;CartSnapshot before;
    auto result=pc_cart_additem(sd.get(),&incoming,MAX_AMOUNT+1,LOG_TYPE_NONE);
    std::printf("CART_OVERSIZED_ADD_PROBE result=%d cart=%d\n",result,cart_count(909));std::fflush(stdout);
    check(result!=ADDITEM_SUCCESS,"native cart insertion rejects quantity above MAX_AMOUNT");before.unchanged();
}
void bulk_callback(){
    auto sd=setup();cart_stock();cart_stock(1,502,7);callback_cart_id=502;npc_data nd{};callback(nd);
    check(audit_stockall(0,sd.get(),"@stockall","")==0,"actual stockall body succeeds");
    std::printf("CART_BULK_CALLBACK_PROBE safe=%d samples=%u inventory=%d cart=%d\n",callback_safe,samples,count(909),cart_count(909));std::fflush(stdout);
    check(samples&&callback_safe&&!cart_count(909)&&!cart_count(502)&&count(909)==10&&count(502)==7,"bulk quest callback sees complete stockall batch");
    check(messages.back()=="17 items are transferred (0 skipped)!","bulk success displays exact transferred total");clear_callback();
}
void fence(map_session_data& sd,int mode){
    switch(mode){
    case 0:sd.bank_ui.pending=true;break;case 1:sd.multi_storage.pending=true;break;
    case 2:sd.mail_companion.pending=true;break;case 3:sd.pair_commit.pending=true;break;
    case 4:sd.shop_commit.pending=true;break;case 5:sd.achievement_data.reward_pending_id=1;break;
    case 6:sd.multi_storage.loading=true;break;case 7:sd.mail.pending_slots=1;break;
    case 8:sd.mail.pending_weight=1;break;case 9:sd.mail.pending_zeny=1;break;
    case 10:sd.state.trading=1;break;case 11:sd.state.vending=1;break;
    case 12:sd.state.prevend=1;break;case 13:sd.state.buyingstore=1;break;
    case 14:sd.state.storage_flag=1;break;case 15:sd.state.refineui_open=1;break;
    case 16:sd.state.stylist_open=1;break;case 17:sd.state.inventory_expansion_confirmation=1;break;
    case 18:sd.npc_shopid=1;break;case 19:sd.state.barter_open=1;break;
    case 20:sd.state.barter_extended_open=1;break;case 21:sd.state.laphine_synthesis=1;break;
    case 22:sd.state.laphine_upgrade=1;break;case 23:sd.state.roulette_open=1;break;
    case 24:sd.state.enchantgrade_open=1;break;case 25:sd.state.item_reform=1;break;
    case 26:sd.state.item_reform_save_id=1;break;case 27:sd.state.item_enchant_index=1;break;
    case 28:sd.sc.opt1=OPT1_STONE;break;case 29:cart_blocked=true;break;
    case 30:sd.sc.deleteSCE(SC_PUSH_CART);sd.sc.option&=~OPTION_CART;break;
    }
}
void packet(bool deposit,int index,int amount){
    const int fd=64;socket_data transport{};uint8 data[8]{};
    transport.rdata=data;transport.max_rdata=transport.rdata_size=sizeof(data);session[fd]=&transport;
    WBUFW(data,0)=deposit?0x126:0x127;WBUFW(data,2)=index+2;WBUFL(data,4)=amount;
    if(deposit)clif_parse_PutItemToCart(fd,attached);else clif_parse_GetItemFromCart(fd,attached);
    session[fd]=nullptr;
}
void fence_matrix(){
    for(int mode=0;mode<31;++mode)for(int path=0;path<5;++path){auto sd=setup();cart_stock();put(0,909,10);weight();fence(*sd,mode);CartSnapshot before;
        if(path==0)pc_putitemtocart(sd.get(),0,10);
        if(path==1)check(!pc_getitemfromcart(sd.get(),0,10),"busy/no-cart native withdrawal refuses");
        if(path==2)check(audit_stockall(0,sd.get(),"@stockall","")!=0,"busy/no-cart stockall command refuses");
        if(path>=3)packet(path==3,0,10);
        before.unchanged();
    }
    // Internal storage/market apply phases retain access to core mutations;
    // user transfer entry points remain fenced even during an apply phase.
    for(int mode=0;mode<5;++mode)for(bool applying:{false,true}){auto sd=setup();cart_stock();
        if(mode==0){sd->bank_ui.pending=true;sd->bank_ui.applying=applying;}
        if(mode==1){sd->multi_storage.pending=true;sd->multi_storage.applying=applying;}
        if(mode==2){sd->mail_companion.pending=true;sd->mail_companion.applying=applying;}
        if(mode==3){sd->pair_commit.pending=true;sd->pair_commit.applying=applying;}
        if(mode==4){sd->shop_commit.pending=true;sd->shop_commit.applying=applying;}
        auto item=sd->cart.u.items_cart[0];CartSnapshot before;
        const auto added=pc_cart_additem(sd.get(),&item,2,LOG_TYPE_NONE);pc_cart_delitem(sd.get(),0,1,0,LOG_TYPE_NONE);
        if(applying)check(added==ADDITEM_SUCCESS&&cart_count(909)==11,"owning apply phase can mutate core cart with exact quantities");
        else {check(added!=ADDITEM_SUCCESS,"pending core insertion refuses");before.unchanged();}
        check(!pc_getitemfromcart(sd.get(),0,1),"user withdrawal cannot bypass owning transaction fence");
    }
}
void invalid_matrix(){
    for(int mode=0;mode<10;++mode)for(bool deposit:{false,true}){auto sd=setup();cart_stock();put(0,909,10);weight();int index=0,amount=10;
        if(mode==0)index=-1;if(mode==1)index=deposit?MAX_INVENTORY:MAX_CART;
        if(mode==2)amount=0;if(mode==3)amount=-1;if(mode==4)amount=11;
        if(mode==5){if(deposit)sd->inventory.u.items_inventory[0].nameid=UINT32_MAX;else sd->cart.u.items_cart[0].nameid=UINT32_MAX;}
        if(mode==6){if(deposit)sd->inventory_data[0]=nullptr;else sd->cart_num=0;}
        if(mode==7){if(deposit)sd->inventory_data[0]=item_db.find(501).get();else sd->cart_weight=-1;}
        if(mode==8){if(deposit)sd->inventory.u.items_inventory[0].equipSwitch=EQP_HEAD_TOP;else sd->cart_weight=0;}
        if(mode==9){if(deposit)sd->weight=0;else sd->cart_num=MAX_CART+1;}
        CartSnapshot before;if(deposit)pc_putitemtocart(sd.get(),index,amount);else check(!pc_getitemfromcart(sd.get(),index,amount),"invalid native withdrawal refuses");before.unchanged();
    }
    for(int mode=0;mode<8;++mode){auto sd=setup();cart_stock();int index=0,amount=1;
        if(mode==0)index=-1;if(mode==1)index=MAX_CART;if(mode==2)amount=0;if(mode==3)amount=-1;
        if(mode==4)amount=11;if(mode==5)sd->cart_num=0;if(mode==6)sd->cart_weight=-1;if(mode==7)sd->cart_weight=0;
        CartSnapshot before;pc_cart_delitem(sd.get(),index,amount,0,LOG_TYPE_NONE);before.unchanged();
    }
    for(int mode=0;mode<5;++mode){auto sd=setup();cart_stock();int index=0,amount=1;
        if(mode==0)index=-1;if(mode==1)index=MAX_CART;if(mode==2)amount=0;if(mode==3)amount=-1;if(mode==4)amount=11;
        check(pc_cartitem_amount(sd.get(),index,amount)==-1,"cart amount query rejects invalid input before array access");
    }
    check(pc_cartitem_amount(nullptr,0,1)==-1,"null cart quantity query is safe");
}
void identity(item& it,int mode){
    if(mode==0)it.identify=0;if(mode==1)it.refine=1;if(mode==2)it.attribute=1;
    if(mode==3)it.enchantgrade=1;if(mode==4)it.bound=BOUND_ACCOUNT;if(mode==5)it.unique_id=UINT64_MAX;
    if(mode==6)it.expire_time=2100000000;
    if(mode>=7&&mode<11)it.card[mode-7]=4365;
    if(mode>=11){auto& opt=it.option[(mode-11)/3];if((mode-11)%3==0)opt.id=1;if((mode-11)%3==1)opt.value=10;if((mode-11)%3==2)opt.param=2;}
}
void metadata_matrix(){
    for(int mode=0;mode<26;++mode){auto sd=setup();cart_stock(0,909,5);put(0,909,10);weight();auto& source=sd->inventory.u.items_inventory[0];identity(source,mode);auto expected=source;
        pc_putitemtocart(sd.get(),0,10);check(!count(909)&&sd->cart.u.items_cart[0].amount==5&&sd->cart.u.items_cart[1].amount==10,"unequal identity retains two separate cart stacks");
        check(compare_item(&sd->cart.u.items_cart[1],&expected),"inventory to cart preserves all stack identity fields");
        check(pc_getitemfromcart(sd.get(),1,10)&&count(909)==10&&cart_count(909)==5&&sd->cart_num==1,"identity roundtrip returns exact quantities");
        check(compare_item(&sd->inventory.u.items_inventory[0],&expected),"cart to inventory retains complete identity");
    }
    {auto sd=setup();put(0,400999,1);auto& gear=sd->inventory.u.items_inventory[0];gear.bound=BOUND_ACCOUNT;gear.refine=12;gear.enchantgrade=3;gear.card[0]=4365;gear.option[0]={1,10,2};gear.favorite=1;weight();auto expected=gear;
        pc_putitemtocart(sd.get(),0,1);check(!count(400999)&&sd->cart_num==1&&!sd->cart.u.items_cart[0].favorite&&!sd->cart.u.items_cart[0].equip&&!sd->cart.u.items_cart[0].equipSwitch,"equipment deposit clears native cart display/equip fields");
        check(pc_getitemfromcart(sd.get(),0,1)&&compare_item(&sd->inventory.u.items_inventory[0],&expected)&&!sd->cart_num&&!sd->cart_weight,"equipment GUID/refine/grade/card/option roundtrip survives");}
}
void limits_and_controls(){
    for(int mode=0;mode<7;++mode){auto sd=setup();put(0,909,10);weight();auto data=item_db.find(909);auto saved=data->stack;
        if(mode==0)sd->cart_weight_max=0;
        if(mode==1){for(auto& it:sd->cart.u.items_cart){it.nameid=501;it.identify=1;it.amount=1;}sd->cart_num=MAX_CART;sd->cart_weight=item_db.find(501)->weight*MAX_CART;}
        if(mode==2){cart_stock(0,909,MAX_AMOUNT);put(0,909,10);weight();}
        if(mode==3){data->stack.cart=true;data->stack.amount=5;}
        if(mode==4)sd->inventory.u.items_inventory[0].bound=BOUND_GUILD;
        if(mode==5)sd->cart_num=MAX_CART;
        if(mode==6)sd->cart_weight=-1;
        CartSnapshot before;pc_putitemtocart(sd.get(),0,10);before.unchanged();data->stack=saved;
    }
    {auto sd=setup();cart_stock();put(0,909,5);weight();packet(true,0,5);check(!count(909)&&cart_count(909)==15&&sd->cart_num==1,"actual deposit packet stacks identical metadata once");packet(false,0,4);check(count(909)==4&&cart_count(909)==11&&sd->weight==item_db.find(909)->weight*4&&sd->cart_weight==item_db.find(909)->weight*11,"actual withdrawal packet conserves partial quantities and both weights");}
    {auto sd=setup();cart_stock();check(pc_cartitem_amount(sd.get(),0,3)==7&&pc_cartitem_amount(sd.get(),0,10)==0,"valid cart amount query returns exact remainder");pc_cart_delitem(sd.get(),0,10,0,LOG_TYPE_NONE);check(!sd->cart_num&&!sd->cart_weight&&!cart_count(909),"complete cart removal clears counters and slot");}
    {auto sd=setup();cart_stock();sd->cart.u.items_cart[0].nameid=UINT32_MAX;sd->cart_weight=0;pc_cart_delitem(sd.get(),0,10,0,LOG_TYPE_OTHER);check(!sd->cart_num&&!sd->cart.u.items_cart[0].nameid,"unknown zero-weight loaded items remain removable by native cleanup");}
    {auto sd=setup();cart_stock(0,909,MAX_AMOUNT+1);pc_cart_delitem(sd.get(),0,MAX_AMOUNT+1,0,LOG_TYPE_OTHER);check(!sd->cart_num&&!sd->cart_weight,"legacy oversized stacks remain removable by native cleanup");}
    {auto sd=setup();cart_stock();cart_stock(1,502,7);check(audit_stockall(0,sd.get(),"@stockall","3")==0&&count(909)==10&&cart_count(502)==7&&messages.back()=="10 items are transferred (0 skipped)!","stockall item-type filtering remains usable");}
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");deny_network();static char name[]="cart-transfer-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;map[0].instance_id=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"cart:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"cart:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto data=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:tree["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    if(argc==2||!strcmp(argv[2],"callback"))withdraw_callback();
    if(argc==2||!strcmp(argv[2],"pending_deposit"))pending_deposit();
    if(argc==2||!strcmp(argv[2],"pending_stockall"))pending_stockall();
    if(argc==2||!strcmp(argv[2],"capacity"))failed_capacity();
    if(argc==2||!strcmp(argv[2],"negative"))negative_delete();
    if(argc==2||!strcmp(argv[2],"oversized"))oversized_add();
    if(argc==2||!strcmp(argv[2],"bulk_callback"))bulk_callback();
    if(argc==2){fence_matrix();invalid_matrix();metadata_matrix();limits_and_controls();}
    check(!errors,"no script errors");attached=nullptr;item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("CART_TRANSFER_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
