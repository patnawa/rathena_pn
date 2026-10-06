// GPL-3.0-or-later. Actual guild storage, inventory, packet and quest VM paths.
#include <map/storage.hpp>
#include <map/guild.hpp>
#include <map/pc_groups.hpp>
#include <map/npc.hpp>
#include <map/quest.hpp>
#include <map/achievement.hpp>
#include <common/sql.hpp>
#include <common/socket.hpp>
extern std::map<int32,s_storage> guild_storage_db;
void clif_parse_MoveFromKafra(int32,map_session_data*);
namespace { bool connected=true,can_give=true,callback_safe=true;unsigned samples=0,log_writes=0;std::string last_query,guild_name_parameter;SqlDataType guild_bound_column_type=SQLDT_NULL;std::vector<guild_log_entry> observed_logs; }
extern "C" int32 connected_boundary() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int32 connected_boundary(){return connected;}
extern "C" int32 flag_boundary(int16,e_mapflag,u_mapflag_args*) asm("__wrap__Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args");
extern "C" int32 flag_boundary(int16,e_mapflag f,u_mapflag_args*){return f==MF_TOWN;}
extern "C" bool give_boundary(const map_session_data*) asm("__wrap__Z17pc_can_give_itemsPK16map_session_data");
extern "C" bool give_boundary(const map_session_data*){return can_give;}
extern "C" bool bounded_boundary(const map_session_data*) asm("__wrap__Z25pc_can_give_bounded_itemsPK16map_session_data");
extern "C" bool bounded_boundary(const map_session_data*){return false;}
extern "C" int32 group_boundary(const map_session_data*) asm("__wrap__Z18pc_get_group_levelPK16map_session_data");
extern "C" int32 group_boundary(const map_session_data*){return 0;}
extern "C" const char* locale_boundary(const map_session_data*,int32) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* locale_boundary(const map_session_data*,int32){return "guild storage fixture message";}
extern "C" void added_boundary(const map_session_data*,const item*,int32,int32) asm("__wrap__Z21clif_storageitemaddedPK16map_session_dataPK4itemii");
extern "C" void added_boundary(const map_session_data*,const item*,int32,int32){}
extern "C" void amount_boundary(const map_session_data&,uint16,uint16) asm("__wrap__Z24clif_updatestorageamountRK16map_session_datatt");
extern "C" void amount_boundary(const map_session_data&,uint16,uint16){}
extern "C" void removed_boundary(const map_session_data&,uint16,uint32) asm("__wrap__Z23clif_storageitemremovedRK16map_session_datatj");
extern "C" void removed_boundary(const map_session_data&,uint16,uint32){}
extern "C" void drop_boundary(const map_session_data&,int32,int32) asm("__wrap__Z13clif_dropitemRK16map_session_dataii");
extern "C" void drop_boundary(const map_session_data&,int32,int32){}
// SQL statement preparation/execution are boundaries. SQL handles/statement
// allocation and the production guild-log query construction remain real.
extern "C" int32 prepare_boundary(SqlStmt*,const char*) asm("__wrap__ZN7SqlStmt10PrepareStrEPKc");
extern "C" int32 prepare_boundary(SqlStmt*,const char* q){last_query=q;return SQL_SUCCESS;}
extern "C" int32 execute_boundary(SqlStmt*) asm("__wrap__ZN7SqlStmt7ExecuteEv");
extern "C" int32 execute_boundary(SqlStmt*){++log_writes;return SQL_SUCCESS;}
extern "C" int32 parameter_boundary(SqlStmt*,size_t,SqlDataType,void*,size_t) asm("__wrap__ZN7SqlStmt9BindParamEm11SqlDataTypePvm");
extern "C" int32 parameter_boundary(SqlStmt*,size_t i,SqlDataType t,void* value,size_t size){check(i==0&&t==SQLDT_STRING,"guild name is the sole string parameter");guild_name_parameter.assign(static_cast<char*>(value),size);return SQL_SUCCESS;}
extern "C" int32 column_boundary(SqlStmt*,size_t,SqlDataType,void*,size_t,uint32*,int8*) asm("__wrap__ZN7SqlStmt10BindColumnEm11SqlDataTypePvmPjPa");
extern "C" int32 column_boundary(SqlStmt*,size_t i,SqlDataType t,void*,size_t,uint32*,int8*){if(i==9)guild_bound_column_type=t;return SQL_SUCCESS;}
extern "C" int32 row_boundary(SqlStmt*) asm("__wrap__ZN7SqlStmt7NextRowEv");
extern "C" int32 row_boundary(SqlStmt*){return SQL_NO_DATA;}
extern "C" void log_packet_boundary(const map_session_data&,const std::vector<guild_log_entry>&,e_guild_storage_log) asm("__wrap__Z22clif_guild_storage_logRK16map_session_dataRKSt6vectorI15guild_log_entrySaIS3_EE19e_guild_storage_log");
extern "C" void log_packet_boundary(const map_session_data&,const std::vector<guild_log_entry>& entries,e_guild_storage_log){observed_logs=entries;}
extern "C" void cart_added_boundary(const map_session_data*,int32,int32) asm("__wrap__Z17clif_cart_additemPK16map_session_dataii");
extern "C" void cart_added_boundary(const map_session_data*,int32,int32){}
extern "C" void cart_removed_boundary(const map_session_data&,int32,int32) asm("__wrap__Z17clif_cart_delitemRK16map_session_dataii");
extern "C" void cart_removed_boundary(const map_session_data&,int32,int32){}
extern "C" void cart_ack_boundary(const map_session_data&,e_ack_additem_to_cart) asm("__wrap__Z21clif_cart_additem_ackRK16map_session_data21e_ack_additem_to_cart");
extern "C" void cart_ack_boundary(const map_session_data&,e_ack_additem_to_cart){}
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code* code,map_session_data* sd){
    ++samples;for(const auto& it:guild_storage_db.at(sd->status.guild_id).u.items_guild)if(it.nameid==909&&it.amount)callback_safe=false;
    return real_condition(code,sd);
}
namespace {
RunePlayer setup(){
    auto sd=rune_player();connected=can_give=true;callback_safe=true;samples=log_writes=0;last_query.clear();guild_name_parameter.clear();observed_logs.clear();guild_storage_db.clear();
    sd->group=std::make_shared<s_player_group>();
    sd->status.guild_id=123;sd->guild=std::make_shared<MapGuild>();auto& g=sd->guild->guild;
    g.guild_id=123;g.max_member=1;g.member[0].account_id=sd->status.account_id;g.member[0].char_id=sd->status.char_id;g.member[0].position=0;g.position[0].mode=GUILD_PERM_STORAGE;
    sd->state.storage_flag=2;sd->cart_weight_max=100000000;
    auto& st=guild_storage_db[123];st.type=TABLE_GUILD_STORAGE;st.id=123;st.status=true;st.max_amount=5;
    return sd;
}
s_storage& guild_page(){return guild_storage_db.at(123);}
int stored(){int n=0;for(const auto& it:guild_page().u.items_guild)if(it.nameid==909)n+=it.amount;return n;}
void stock(){auto& st=guild_page();st.u.items_guild[0].nameid=909;st.u.items_guild[0].amount=10;st.u.items_guild[0].identify=1;st.amount=1;}
void withdraw_packet(){
    // In-memory transport buffer drives the actual per-packet handler.
    // It does not bypass permission checks in the storage open path: this actor
    // already opened legitimately, then had its storage permission revoked.
    const int fd=64;socket_data transport{};uint8 data[8]{};transport.rdata=data;transport.max_rdata=transport.rdata_size=sizeof(data);
    session[fd]=&transport;WBUFW(data,0)=0x00f5;WBUFW(data,2)=1;WBUFL(data,4)=10;
    packet_db[0x00f5].pos[0]=2;packet_db[0x00f5].pos[1]=4;
    clif_parse_MoveFromKafra(fd,attached);session[fd]=nullptr;
}
void revoked_permission(){
    auto sd=setup();stock();sd->guild->guild.position[0].mode=0;
    withdraw_packet();std::printf("GUILD_REVOKED_PROBE inventory=%d storage=%d logs=%u\n",count(909),stored(),log_writes);std::fflush(stdout);
    check(!count(909)&&stored()==10&&!log_writes,"revoked storage permission blocks actual withdrawal packet");
}
void permitted_control(){
    auto sd=setup();stock();check(guild_getposition(*sd)==0&&guild_has_permission(*sd,GUILD_PERM_STORAGE),"control resolves actual member role");
    withdraw_packet();check(count(909)==10&&!stored()&&log_writes==1,"same permitted packet transfers exact items");
}
void locked_cart(){
    auto sd=setup();put(0,909,10);sd->cart.u.items_cart[0]=sd->inventory.u.items_inventory[0];clear_items();sd->cart_num=1;sd->cart_weight=item_db.find(909)->weight*10;
    guild_page().lock=true;storage_guild_storageaddfromcart(sd.get(),0,10);
    std::printf("GUILD_LOCKED_CART_PROBE cart=%d storage=%d logs=%u\n",sd->cart.u.items_cart[0].amount,stored(),log_writes);std::fflush(stdout);
    check(sd->cart.u.items_cart[0].amount==10&&!stored()&&!log_writes,"locked guild page refuses native cart deposit");
}
void cart_metadata(){
    auto sd=setup();stock();auto& original=guild_page().u.items_guild[0];original.option[0].id=1;original.option[0].value=10;
    sd->cart.u.items_cart[0]=original;sd->cart.u.items_cart[0].amount=5;sd->cart.u.items_cart[0].option[0]={};sd->cart_num=1;sd->cart_weight=item_db.find(909)->weight*5;
    storage_guild_storagegettocart(sd.get(),0,10);
    std::printf("GUILD_CART_METADATA_PROBE first_amount=%d second_id=%u second_option=%d\n",sd->cart.u.items_cart[0].amount,sd->cart.u.items_cart[1].nameid,sd->cart.u.items_cart[1].option[0].id);std::fflush(stdout);
    check(sd->cart.u.items_cart[0].amount==5&&sd->cart.u.items_cart[1].amount==10&&sd->cart.u.items_cart[1].option[0].id==1,"different option metadata never merges into plain cart stack");
}
void log_binding(){
    auto sd=setup();guild_bound_column_type=SQLDT_NULL;check(storage_guild_log_read(sd.get())==GUILDSTORAGE_LOG_EMPTY,"native empty log result");
    std::printf("GUILD_LOG_BOUND_PROBE type=%d char_type=%d\n",guild_bound_column_type,SQLDT_CHAR);std::fflush(stdout);
    check(guild_bound_column_type==SQLDT_CHAR||guild_bound_column_type==SQLDT_UINT8||guild_bound_column_type==SQLDT_UCHAR,"guild log binds one byte into packed bound field");
}
void quoted_name(){
    auto sd=setup();strcpy(sd->status.name,"O'Brien");put(0,909,10);weight();storage_guild_storageadd(sd.get(),0,10);
    std::printf("GUILD_LOG_NAME_PROBE parameter=%s parameterized=%d\n",guild_name_parameter.c_str(),last_query.find('?')!=std::string::npos);std::fflush(stdout);
    check(guild_name_parameter=="O'Brien"&&last_query.find('?')!=std::string::npos&&last_query.find("O'Brien")==std::string::npos,"character name stays SQL parameter data, including apostrophes");
    check(stored()==10&&!count(909)&&log_writes==1,"quoted-name deposit still records one complete transfer");
}
void cart_stock(){
    put(0,909,10);attached->cart.u.items_cart[0]=attached->inventory.u.items_inventory[0];clear_items();attached->cart_num=1;attached->cart_weight=item_db.find(909)->weight*10;
}
void transfer(int path,int index=0,int amount=10){
    if(path==0)storage_guild_storageadd(attached,index,amount);
    if(path==1)storage_guild_storageget(attached,index,amount,false);
    if(path==2)storage_guild_storageaddfromcart(attached,index,amount);
    if(path==3)storage_guild_storagegettocart(attached,index,amount);
}
struct GuildSnapshot {
    Snapshot inventory;
    decltype(map_session_data::cart) cart=attached->cart;
    s_storage page=guild_page();
    uint32 cart_weight=attached->cart_weight;
    void unchanged(){inventory.unchanged();check(!memcmp(&cart,&attached->cart,sizeof(cart))&&cart_weight==attached->cart_weight,"refusal leaves complete cart and weight unchanged");
        check(!memcmp(&page,&guild_page(),sizeof(page))&&!log_writes,"refusal leaves complete guild page unchanged and writes no log");}
};
void access_matrix(){
    for(int path=0;path<4;++path)for(int mode=0;mode<24;++mode){
        auto sd=setup();cart_stock();put(0,909,10);weight();stock();auto& st=guild_page();
        switch(mode){
        case 0:sd->state.storage_flag=0;break;case 1:st.status=false;break;
        case 2:st.lock=true;break;case 3:sd->guild->guild.position[0].mode=0;break;
        case 4:sd->guild->guild.member[0].position=MAX_GUILDPOSITION;break;
        case 5:sd->guild->guild.member[0].position=-1;break;
        case 6:sd->guild->guild.member[0].account_id++;break;
        case 7:sd->guild->guild.member[0].char_id++;break;
        case 8:sd->guild->guild.guild_id++;break;case 9:sd->guild.reset();break;
        case 10:st.id++;break;case 11:st.type=TABLE_STORAGE;break;
        case 12:st.max_amount=0;break;case 13:st.max_amount=MAX_GUILD_STORAGE+1;break;
        case 14:st.amount=st.max_amount+1;break;
        case 15:sd->multi_storage.pending=true;break;case 16:sd->shop_commit.pending=true;break;
        case 17:sd->pair_commit.pending=true;break;case 18:sd->bank_ui.pending=true;break;
        case 19:sd->mail_companion.pending=true;break;case 20:sd->achievement_data.reward_pending_id=1;break;
        case 21:sd->multi_storage.loading=true;break;case 22:connected=false;break;case 23:can_give=false;break;
        }
        GuildSnapshot before;transfer(path);before.unchanged();
    }
}
void invalid_matrix(){
    for(int path=0;path<4;++path)for(int mode=0;mode<6;++mode){
        auto sd=setup();cart_stock();put(0,909,10);weight();stock();int index=0,amount=10;
        if(mode==0)index=-1;if(mode==1)index=path==0?MAX_INVENTORY:path==2?MAX_CART:guild_page().max_amount;
        if(mode==2)amount=0;if(mode==3)amount=11;
        if(mode==4){if(path==0)sd->inventory_data[0]=nullptr;else if(path==2)sd->cart.u.items_cart[0].nameid=0;else guild_page().u.items_guild[0].nameid=0;}
        if(mode==5){if(path==0)sd->inventory.u.items_inventory[0].nameid=UINT32_MAX;else if(path==2)sd->cart.u.items_cart[0].nameid=UINT32_MAX;else guild_page().u.items_guild[0].nameid=UINT32_MAX;}
        GuildSnapshot before;transfer(path,index,amount);before.unchanged();
    }
}
void identity_change(item& it,int mode){
    if(mode==0)it.identify=0;if(mode==1)it.refine=1;if(mode==2)it.attribute=1;
    if(mode==3)it.enchantgrade=1;if(mode==4)it.bound=BOUND_ACCOUNT;if(mode==5)it.unique_id=UINT64_MAX;
    if(mode==6)it.expire_time=2100000000;
    if(mode>=7&&mode<11)it.card[mode-7]=4365;
    if(mode>=11){auto& opt=it.option[(mode-11)/3];if((mode-11)%3==0)opt.id=1;if((mode-11)%3==1)opt.value=10;if((mode-11)%3==2)opt.param=2;}
}
void cart_identity_matrix(){
    for(int mode=0;mode<26;++mode){auto sd=setup();stock();auto& source=guild_page().u.items_guild[0];
        sd->cart.u.items_cart[0]=source;sd->cart.u.items_cart[0].amount=5;sd->cart_num=1;sd->cart_weight=item_db.find(909)->weight*5;
        identity_change(source,mode);const auto original=source;transfer(3);
        check(sd->cart.u.items_cart[0].amount==5&&sd->cart.u.items_cart[1].amount==10,"every unequal stack identity uses a new cart slot");
        check(compare_item(&sd->cart.u.items_cart[1],const_cast<item*>(&original))&&!stored()&&log_writes==1,"cart withdrawal retains full source identity and total");
    }
    {auto sd=setup();stock();sd->cart.u.items_cart[0]=guild_page().u.items_guild[0];sd->cart.u.items_cart[0].amount=5;sd->cart_num=1;sd->cart_weight=item_db.find(909)->weight*5;
        transfer(3);check(sd->cart.u.items_cart[0].amount==15&&sd->cart_num==1&&!stored(),"identical cart metadata still stacks normally");}
}
void limits_and_roundtrip(){
    for(int path:{0,2})for(int mode=0;mode<5;++mode){auto sd=setup();cart_stock();put(0,909,10);weight();auto data=item_db.find(909);const auto old=data->stack;
        if(mode==0){guild_page().max_amount=1;auto& it=guild_page().u.items_guild[0];it.nameid=501;it.amount=1;it.identify=1;guild_page().amount=1;}
        if(mode==1){stock();guild_page().u.items_guild[0].amount=MAX_AMOUNT;}
        if(mode==2){data->stack.guild_storage=true;data->stack.amount=5;}
        if(mode==3){sd->inventory.u.items_inventory[0].bound=BOUND_ACCOUNT;sd->cart.u.items_cart[0].bound=BOUND_ACCOUNT;}
        if(mode==4){sd->inventory.u.items_inventory[0].expire_time=2100000000;sd->cart.u.items_cart[0].expire_time=2100000000;}
        GuildSnapshot before;transfer(path);before.unchanged();data->stack=old;
    }
    for(int path:{1,3})for(int mode=0;mode<2;++mode){auto sd=setup();stock();
        if(mode==0){sd->max_weight=sd->cart_weight_max=0;}
        else {sd->status.inventory_slots=1;put(0,501,1);weight();for(auto& it:sd->cart.u.items_cart){it.nameid=501;it.amount=1;it.identify=1;}sd->cart_num=MAX_CART;sd->cart_weight=item_db.find(501)->weight*MAX_CART;}
        GuildSnapshot before;transfer(path);before.unchanged();
    }
    {auto sd=setup();put(0,400999,1);auto& gear=sd->inventory.u.items_inventory[0];gear.bound=BOUND_GUILD;gear.refine=12;gear.enchantgrade=3;gear.card[0]=4365;gear.option[0].id=1;gear.option[0].value=10;gear.favorite=1;weight();const auto original=gear;
        transfer(0,0,1);check(!count(400999)&&!memcmp(&original,&guild_page().u.items_guild[0],sizeof(item)),"guild-bound equipment deposits with complete metadata");
        storage_guild_storageget(sd.get(),0,1,true);check(count(400999)==1&&!memcmp(&original,&sd->inventory.u.items_inventory[0],sizeof(item))&&guild_page().amount==0&&log_writes==2,"guild-bound equipment returns exact GUID cards refine grade options favorite");}
}
void opening_guards(){
    for(int mode=0;mode<4;++mode){auto sd=setup();sd->state.storage_flag=0;guild_page().status=false;
        if(mode==0)sd->guild.reset();if(mode==1)sd->guild->guild.member[0].position=MAX_GUILDPOSITION;
        if(mode==2)sd->multi_storage.pending=true;if(mode==3)connected=false;
        const auto before=guild_page();check(storage_guild_storageopen(sd.get())!=GSTORAGE_OPEN,"invalid actor/pending/disconnected opening refuses");
        check(!sd->state.storage_flag&&!memcmp(&before,&guild_page(),sizeof(before)),"refused open does not claim the shared page");}
}
void withdrawal_callback(){
    auto sd=setup();stock();npc_data nd{};nd.id=NPC;nd.type=BL_NPC;quest_npc=fake_nd=&nd;map_num=1;map[0].qi_npc={NPC};sd->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
    qi->condition=compile("{ @GuildCallbackSeen=1; @GuildInventory=countitem(909); achievement_condition(0); }","actual guild withdrawal callback");nd.qi_data.push_back(qi);
    storage_guild_storageget(sd.get(),0,10,false);
    std::printf("GUILD_CALLBACK_PROBE safe=%d samples=%u inventory=%d storage=%d\n",callback_safe,samples,count(909),stored());std::fflush(stdout);
    check(samples&&callback_safe&&pc_readreg(sd.get(),add_str("@GuildInventory"))==10,"guild callback follows source removal");
    check(count(909)==10&&!stored()&&log_writes==1,"withdraw retains exact total and one log");map[0].qi_npc.clear();quest_npc=fake_nd=nullptr;
}
void sql_logs(){
    auto sd=setup();strcpy(sd->status.name,"O'Brien");put(0,909,10);auto& it=sd->inventory.u.items_inventory[0];
    it.unique_id=UINT64_MAX;it.bound=BOUND_GUILD;it.enchantgrade=3;it.card[0]=4365;it.option[0].id=1;it.option[0].value=10;it.option[0].param=2;weight();
    storage_guild_storageadd(sd.get(),0,10);check(!count(909)&&stored()==10,"real SQL logging accompanies complete native transfer");
    check(storage_guild_log_read(sd.get())==GUILDSTORAGE_LOG_FINAL_SUCCESS&&observed_logs.size()==1,"actual guild log reader fetches saved SQL row");
    const auto& row=observed_logs.front();check(!strcmp(row.name,"O'Brien")&&row.item.unique_id==UINT64_MAX&&row.item.bound==BOUND_GUILD&&row.item.enchantgrade==3&&row.item.card[0]==4365&&row.item.option[0].id==1&&row.item.option[0].value==10&&row.item.option[0].param==2&&row.amount==10,"actual prepared SQL and native bindings preserve quoted name and packed item metadata");
    observed_logs.clear();std::printf("GUILD_STORAGE_SQL_LOG_OK quoted_name=true guid_uint64_max=true metadata=true\n");
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");const bool sql_fixture=argc==3&&!strcmp(argv[2],"sql_logs");
    if(sql_fixture)check(getenv("PN_GUILD_SQL_FIXTURE")&&!strcmp(getenv("PN_GUILD_SQL_FIXTURE"),"owned-disposable-only"),"SQL fixture must be explicitly owned and disposable");else deny_network();
    static char name[]="guild-storage-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;
    mmysql_handle=Sql_Malloc();map[0].instance_id=0;num_reg_ers=ers_new(sizeof(script_reg_num),"guild:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"guild:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    if(sql_fixture)check(Sql_Connect(mmysql_handle,"ragnarok","ragnarok","release-db",3306,"ragnarok")==SQL_SUCCESS,"connect only to internal fixture alias with fixture credentials");
    auto data=read(std::string(argv[1])+"/items.yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:tree["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    if(argc==2||!strcmp(argv[2],"permission"))revoked_permission();
    if(argc==2||!strcmp(argv[2],"callback"))withdrawal_callback();
    if(argc==2||!strcmp(argv[2],"control"))permitted_control();
    if(argc==2||!strcmp(argv[2],"locked_cart"))locked_cart();
    if(argc==2||!strcmp(argv[2],"cart_metadata"))cart_metadata();
    if(argc==2||!strcmp(argv[2],"log_binding"))log_binding();
    if(argc==2||!strcmp(argv[2],"quoted_name"))quoted_name();
    if(argc==2){access_matrix();invalid_matrix();cart_identity_matrix();limits_and_roundtrip();opening_guards();}
    if(sql_fixture)sql_logs();
    check(errors==0,"no script errors");attached=nullptr;guild_storage_db.clear();item_db.clear();Sql_Free(mmysql_handle);mmysql_handle=nullptr;do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("GUILD_STORAGE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
