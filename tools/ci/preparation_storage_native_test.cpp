// GPL-3.0-or-later. Native storage, inventory, preparation VM and quest conditions.
#include <custom/preparation_policy.hpp>
#include <map/storage.hpp>
#include <map/map.hpp>
#include <map/npc.hpp>
#include <map/quest.hpp>
#include <map/intif.hpp>
#include <map/achievement.hpp>
#include <map/pet.hpp>
#include <common/nullpo.hpp>
int32 audit_storeall(const int32,map_session_data*,const char*,const char*);
namespace {
bool town=true,connected=true;
bool observe_withdrawal=false,callback_safe=true;
unsigned callback_samples=0;
unsigned commits=0;
s_storage saved_page{};
decltype(map_session_data::inventory) saved_inventory{};
uint64 committed_sequence=0;
}
// Observe the native quest condition boundary and forward to the actual VM.
// This read-only probe avoids querying a service disabled by the pending fence.
extern "C" bool real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool condition_boundary(script_code* code,map_session_data* sd){
    ++callback_samples;
    if(observe_withdrawal){
        for(const auto& it:sd->storage.u.items_storage)if(it.nameid==909&&it.amount)callback_safe=false;
        if(!sd->multi_storage.pending||commits!=1)callback_safe=false;
    }
    return real_condition(code,sd);
}
extern "C" int32 connected_boundary() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int32 connected_boundary(){return connected;}
extern "C" int32 flag_boundary(int16,e_mapflag,u_mapflag_args*) asm("__wrap__Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args");
extern "C" int32 flag_boundary(int16,e_mapflag f,u_mapflag_args*){return f==MF_TOWN&&town;}
extern "C" bool give_boundary(const map_session_data*) asm("__wrap__Z17pc_can_give_itemsPK16map_session_data");
extern "C" bool give_boundary(const map_session_data*){return true;}
extern "C" int32 group_boundary(const map_session_data*) asm("__wrap__Z18pc_get_group_levelPK16map_session_data");
extern "C" int32 group_boundary(const map_session_data*){return 0;}
extern "C" void commit_boundary(map_session_data&) asm("__wrap__Z20intif_storage_commitR16map_session_data");
extern "C" void commit_boundary(map_session_data& sd){
    check(sd.multi_storage.pending&&!sd.multi_storage.applying,"native pending fence precedes interserver commit");
    ++commits;saved_page=sd.storage;saved_inventory=sd.inventory;committed_sequence=sd.multi_storage.sequence;
}
extern "C" void added_boundary(const map_session_data*,const item*,int32,int32) asm("__wrap__Z21clif_storageitemaddedPK16map_session_dataPK4itemii");
extern "C" void added_boundary(const map_session_data*,const item*,int32,int32){}
extern "C" void amount_boundary(const map_session_data&,uint16,uint16) asm("__wrap__Z24clif_updatestorageamountRK16map_session_datatt");
extern "C" void amount_boundary(const map_session_data&,uint16,uint16){}
extern "C" void removed_boundary(const map_session_data&,uint16,uint32) asm("__wrap__Z23clif_storageitemremovedRK16map_session_datatj");
extern "C" void removed_boundary(const map_session_data&,uint16,uint32){}
extern "C" void drop_boundary(const map_session_data&,int32,int32) asm("__wrap__Z13clif_dropitemRK16map_session_dataii");
extern "C" void drop_boundary(const map_session_data&,int32,int32){}
extern "C" const char* locale_boundary(const map_session_data*,int32) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* locale_boundary(const map_session_data*,int32){return "storage fixture message";}
extern "C" void close_boundary(const map_session_data&) asm("__wrap__Z17clif_storagecloseRK16map_session_data");
extern "C" void close_boundary(const map_session_data&){}

namespace {
int64 temp(const char* key){return pc_readreg(attached,add_str(key));}
void command(const char* text){invoke(std::string("@DepositResult=")+text+";");}
RunePlayer setup(){
    auto sd=rune_player();connected=town=true;commits=0;committed_sequence=0;saved_page={};saved_inventory={};
    observe_withdrawal=false;callback_safe=true;callback_samples=0;
    sd->m=0;sd->storage.type=TABLE_STORAGE;sd->storage.id=sd->status.account_id;sd->storage.stor_id=0;sd->storage.max_amount=5;
    for(auto& index:sd->equip_switch_index)index=-1;
    return sd;
}
int stored(int id,int bound=-1){int n=0;for(const auto& it:attached->storage.u.items_storage)if(it.nameid==id&&(bound<0||it.bound==bound))n+=it.amount;return n;}
void callback(npc_data& nd,bool withdrawal=false){
    observe_withdrawal=withdrawal;
    nd.id=NPC;nd.type=BL_NPC;quest_npc=fake_nd=&nd;map_num=1;map[0].qi_npc={NPC};attached->qi_display.resize(1);
    auto qi=std::make_shared<s_questinfo>();qi->icon=QTYPE_QUEST;qi->color=QMARK_NONE;
    qi->condition=compile(withdrawal?"{ if(!@DepositSeen) { @DepositSeen=1; @DepositSafe=(countitem(909)==10); } achievement_condition(0); }":
        "{ if(!@DepositSeen) { @DepositSeen=1; @DepositSafe=(countitem(501)==0 && countitem(909)==0); } achievement_condition(0); }","actual storage quest callback");
    nd.qi_data.push_back(qi);
}
void clear_callback(){map[0].qi_npc.clear();quest_npc=fake_nd=nullptr;}
void acknowledge(){
    check(attached->multi_storage.pending&&commits==1,"one pending transaction at ACK boundary");
    // Actual wire matching/SQL receipt recovery have separate suites. This
    // boundary simulates only the tagged successful ACK releasing the fence.
    attached->multi_storage.pending=false;attached->storage.dirty=false;
    do_timer(gettick()+300);
}
void deposit_callback(){
    auto sd=setup();put(0,501,20);put(1,909,10);weight();npc_data nd{};callback(nd);
    command("pnprepinventory(1,0)");check(temp("@DepositResult")==2,"actual preview contains two ordinary stacks");
    command("pnprepinventory(1,1)");
    std::printf("STORAGE_BEFORE_ACK seen=%lld safe=%lld inventory501=%d inventory909=%d commits=%u\n",(long long)temp("@DepositSeen"),(long long)temp("@DepositSafe"),count(501),count(909),commits);std::fflush(stdout);
    if(sd->multi_storage.pending)acknowledge();
    std::printf("STORAGE_CALLBACK_PROBE seen=%lld safe=%lld plain=%d bound=%d commits=%u\n",(long long)temp("@DepositSeen"),(long long)temp("@DepositSafe"),stored(909,0),stored(909,BOUND_ACCOUNT),commits);std::fflush(stdout);
    check(temp("@DepositSeen")==1&&temp("@DepositSafe")==1,"callbacks cannot observe or replace a partially transferred batch");
    check(temp("@DepositResult")==2&&stored(501)==20&&stored(909,0)==10&&!stored(909,BOUND_ACCOUNT)&&!count(501)&&!count(909),"only exact previewed ordinary items reach storage");
    check(commits==1&&committed_sequence==1,"whole batch commits once");clear_callback();
}
void withdraw_callback(){
    auto sd=setup();sd->storage.state.get=1;sd->storage.u.items_storage[0]={};sd->storage.u.items_storage[0].nameid=909;sd->storage.u.items_storage[0].amount=10;sd->storage.u.items_storage[0].identify=1;sd->storage.amount=1;
    npc_data nd{};callback(nd,true);storage_storageget(sd.get(),&sd->storage,0,10,false);
    if(sd->multi_storage.pending)acknowledge();
    std::printf("STORAGE_WITHDRAW_PROBE seen=%lld safe=%lld callback_source_removed=%d samples=%u inventory909=%d stored909=%d\n",(long long)temp("@DepositSeen"),(long long)temp("@DepositSafe"),callback_safe,callback_samples,count(909),stored(909));std::fflush(stdout);
    check(temp("@DepositSeen")==1&&temp("@DepositSafe")==1&&callback_samples&&callback_safe,"withdraw callback follows source removal and commit fence");
    check(count(909)==10&&!stored(909)&&commits==1,"withdraw preserves total and one commit");clear_callback();
}
struct TransferSnapshot {
    Snapshot inventory;
    s_storage page=attached->storage;
    void unchanged(){inventory.unchanged();check(!memcmp(&page,&attached->storage,sizeof(page)),"rejected transfer preserves complete storage page");check(commits==0,"rejected transfer sends no commit");}
};
void special(item& it,int mode){
    switch(mode){
    case 0:it.favorite=1;break;case 1:it.identify=0;break;
    case 2:it.equip=EQP_HAND_R;break;case 3:it.equipSwitch=EQP_HAND_R;break;
    case 4:it.refine=1;break;case 5:it.bound=BOUND_ACCOUNT;break;
    case 6:it.expire_time=2100000000;break;case 7:it.attribute=1;break;
    case 8:it.enchantgrade=1;break;case 9:it.unique_id=UINT64_MAX;break;
    case 10:case 11:case 12:case 13:it.card[mode-10]=4700;break;
    default:it.option[(mode-14)/3].id=(mode-14)%3==0?1:0;
        it.option[(mode-14)/3].value=(mode-14)%3==1?1:0;
        it.option[(mode-14)/3].param=(mode-14)%3==2?1:0;break;
    }
}
void protection_matrix(){
    for(int mode=0;mode<29;++mode){
        auto sd=setup();put(0,501,20);put(1,909,10);special(sd->inventory.u.items_inventory[1],mode);weight();
        const auto protected_before=sd->inventory.u.items_inventory[1];
        command("pnprepinventory(1,0)");check(temp("@DepositResult")==1,"bulk preview excludes every protected metadata field");
        command("pnprepinventory(1,1)");check(temp("@DepositResult")==1&&stored(501)==20&&!stored(909),"bulk deposits only ordinary eligible item");
        check(!memcmp(&protected_before,&sd->inventory.u.items_inventory[1],sizeof(item)),"protected item remains byte-identical");
        acknowledge();
    }
}
void stale_preview_matrix(){
    for(int mode=0;mode<36;++mode){
        auto sd=setup();put(0,501,20);put(1,909,10);weight();command("pnprepinventory(1,0)");
        if(mode<29)special(sd->inventory.u.items_inventory[1],mode);
        if(mode==29)--sd->inventory.u.items_inventory[1].amount;
        if(mode==30)sd->storage.id++;
        if(mode==31)sd->storage.lock=true;
        if(mode==32)town=false;
        if(mode==33)connected=false;
        if(mode==34)pc_setreg(sd.get(),reference_uid(add_str("@PNPrepIndex"),1),0);
        if(mode==35)std::swap(sd->inventory.u.items_inventory[0],sd->inventory.u.items_inventory[1]);
        weight();TransferSnapshot before;command("pnprepinventory(1,1)");
        check(temp("@DepositResult")==-1,"changed preview/context rejects entire batch");before.unchanged();
    }
}
void capacity_matrix(){
    for(int mode=0;mode<5;++mode){
        auto sd=setup();put(0,501,20);put(1,909,10);weight();
        sd->storage.max_amount=mode==0?1:2;
        if(mode>=1){sd->storage.u.items_storage[0]=sd->inventory.u.items_inventory[1];sd->storage.u.items_storage[0].amount=mode==1?MAX_AMOUNT:mode==2?MAX_AMOUNT-5:1;sd->storage.amount=1;}
        if(mode==3)sd->storage.amount=2; // occupied-slot counter disagrees with contents
        auto data=item_db.find(909);const auto old_stack=data->stack;
        if(mode==4){data->stack.storage=true;data->stack.amount=10;}
        TransferSnapshot before;command("pnprepinventory(1,0)");command("pnprepinventory(1,1)");
        check(temp("@DepositResult")==-1,"late item capacity/stack failure rejects the entire batch");before.unchanged();data->stack=old_stack;
    }
    {auto sd=setup();put(0,909,20);put(1,909,10);weight();sd->storage.max_amount=1;
        command("pnprepinventory(1,0)");command("pnprepinventory(1,1)");
        check(temp("@DepositResult")==2&&stored(909)==30&&sd->storage.amount==1,"multiple inventory stacks share one storage slot");
        check(!memcmp(saved_page.u.items_storage,sd->storage.u.items_storage,sizeof(saved_page.u.items_storage))&&!memcmp(&saved_inventory,&sd->inventory,sizeof(saved_inventory)),"commit submits completed page contents and inventory snapshots");
        const auto sequence=sd->multi_storage.sequence;
        storage_storageget(sd.get(),&sd->storage,0,1,false);command("pnprepinventory(1,1)");
        check(!count(909)&&stored(909)==30&&commits==1&&sd->multi_storage.sequence==sequence,"pending fence blocks repeated commit and opposite transfer");acknowledge();}
}
void native_rejections(){
    for(int mode=0;mode<8;++mode){auto sd=setup();put(0,909,10);weight();sd->storage.state.put=1;
        int index=0,amount=10;if(mode==0)index=-1;if(mode==1)index=MAX_INVENTORY;
        if(mode==2)amount=0;if(mode==3)amount=11;if(mode==4)sd->storage.state.put=0;
        if(mode==5)sd->storage.id++;if(mode==6)connected=false;if(mode==7)sd->inventory_data[0]=nullptr;
        TransferSnapshot before;storage_storageadd(sd.get(),&sd->storage,index,amount);before.unchanged();}
    for(int mode=0;mode<8;++mode){auto sd=setup();sd->storage.state.get=1;
        auto& it=sd->storage.u.items_storage[0];it.nameid=909;it.amount=10;it.identify=1;sd->storage.amount=1;
        int index=0,amount=10;if(mode==0)index=-1;if(mode==1)index=MAX_STORAGE;
        if(mode==2)amount=0;if(mode==3)amount=11;if(mode==4)sd->storage.state.get=0;
        if(mode==5)sd->storage.id++;if(mode==6)sd->max_weight=0;
        if(mode==7){sd->status.inventory_slots=1;put(0,501,1);weight();}
        TransferSnapshot before;storage_storageget(sd.get(),&sd->storage,index,amount,false);before.unchanged();}
}
void metadata_roundtrip(){
    auto sd=setup();put(0,400999,1);weight();sd->storage.state.put=sd->storage.state.get=1;
    auto& it=sd->inventory.u.items_inventory[0];it.bound=BOUND_ACCOUNT;it.unique_id=UINT64_MAX;it.refine=7;it.enchantgrade=2;it.card[0]=4365;it.option[0].id=1;it.option[0].value=10;it.option[0].param=2;it.favorite=1;
    const auto original=it;storage_storageadd(sd.get(),&sd->storage,0,1);
    check(!count(400999)&&!memcmp(&original,&sd->storage.u.items_storage[0],sizeof(item)),"individual deposit retains full equipment metadata");acknowledge();commits=0;
    storage_storageget(sd.get(),&sd->storage,0,1,true);acknowledge();
    check(count(400999)==1&&!stored(400999)&&!memcmp(&original,&sd->inventory.u.items_inventory[0],sizeof(item)),"individual withdrawal preserves bound GUID cards options refine grade favorite");
}
void storeall_callback(){
    auto sd=setup();put(0,501,20);put(1,909,10);weight();sd->storage.state.put=1;sd->state.storage_flag=1;
    npc_data nd{};callback(nd);check(audit_storeall(0,sd.get(),"@storeall","")==0,"actual storeall command succeeds");
    check(temp("@DepositSeen")==1&&temp("@DepositSafe")==1,"storeall callback sees completed batch");
    check(stored(501)==20&&stored(909)==10&&!count(501)&&!count(909)&&commits==1,"storeall preserves exact totals under one commit");
    acknowledge();clear_callback();
}
}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2||argc==3,"artifact input");deny_network();static char server[]="storage-transfer-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();save_settings=0;
    map[0].instance_id=0;num_reg_ers=ers_new(sizeof(script_reg_num),"storage:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"storage:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    auto data=read(std::string(argv[1])+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"effective metadata parses");
    auto main=std::make_shared<s_storage_table>();main->id=0;strcpy(main->table,"storage");storage_db[0]=main;
    load_functions("npc/custom/main_office/preparation.txt");
    if(argc==2||!strcmp(argv[2],"deposit_callback"))deposit_callback();
    if(argc==2||!strcmp(argv[2],"withdraw_callback"))withdraw_callback();
    if(argc==2){protection_matrix();stale_preview_matrix();capacity_matrix();native_rejections();metadata_roundtrip();storeall_callback();}
    check(errors==0,"no unexpected script errors");attached=nullptr;storage_db.clear();item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("STORAGE_TRANSFER_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
