// Actual production script VM, registry, item metadata, preparation commands.
// World, storage transport, equip packets and durable submission are explicit
// boundaries. SQL ACK recovery is exercised by existing shop/storage suites.
#include <custom/preparation_policy.hpp>
#include <custom/shop_state.hpp>
#include <map/storage.hpp>
#include <map/map.hpp>
#include <map/skill.hpp>
namespace {bool town=true,connected=true;unsigned submitted=0,batches=0;std::shared_ptr<pn_shop::Commit> captured;std::vector<pn_shop::Grant> captured_grants;}
extern "C" void prep_clear(const map_session_data&,int32) asm("__wrap__Z16clif_scriptclearRK16map_session_datai");
extern "C" void prep_clear(const map_session_data&,int32){}
extern "C" int32 prep_connected() asm("__wrap__Z17chrif_isconnectedv");
extern "C" int32 prep_connected(){return connected;}
extern "C" int32 prep_flags(int16,e_mapflag,u_mapflag_args*) asm("__wrap__Z18map_getmapflag_subs9e_mapflagP14u_mapflag_args");
extern "C" int32 prep_flags(int16,e_mapflag flag,u_mapflag_args*){return flag==MF_TOWN&&town;}
extern "C" bool prep_give(const map_session_data*) asm("__wrap__Z17pc_can_give_itemsPK16map_session_data");
extern "C" bool prep_give(const map_session_data*){return true;}
extern "C" int32 prep_group(const map_session_data*) asm("__wrap__Z18pc_get_group_levelPK16map_session_data");
extern "C" int32 prep_group(const map_session_data*){return 0;}
extern "C" bool prep_available(map_session_data&,int) asm("__wrap__Z22storage_page_availableR16map_session_datai");
extern "C" bool prep_available(map_session_data&,int){return true;}
extern "C" void prep_batch_begin(map_session_data&) asm("__wrap__Z19storage_batch_beginR16map_session_data");
extern "C" void prep_batch_begin(map_session_data& sd){sd.multi_storage.applying=true;}
extern "C" void prep_batch_end(map_session_data&,s_storage&) asm("__wrap__Z17storage_batch_endR16map_session_dataR9s_storage");
extern "C" void prep_batch_end(map_session_data& sd,s_storage&){++batches;sd.multi_storage.applying=false;}
extern "C" void prep_deposit(map_session_data*,s_storage*,int32,int32) asm("__wrap__Z18storage_storageaddP16map_session_dataP9s_storageii");
extern "C" void prep_deposit(map_session_data* sd,s_storage* storage,int32 i,int32 amount){
    auto it=sd->inventory.u.items_inventory[i];check(!pn_preparation::protected_item(it),"protected item never reaches transfer");
    int target=-1;for(int j=0;j<storage->max_amount;++j)if(compare_item(&storage->u.items_storage[j],&it)){target=j;break;}
    if(target<0)for(int j=0;j<storage->max_amount;++j)if(!storage->u.items_storage[j].nameid){target=j;storage->u.items_storage[j]=it;storage->u.items_storage[j].amount=0;break;}
    check(target>=0,"preflight promised capacity");storage->u.items_storage[target].amount+=amount;storage->dirty=true;sd->inventory.u.items_inventory[i]={};
}
extern "C" bool prep_begin(map_session_data&,std::shared_ptr<pn_shop::Commit>,const std::vector<pn_shop::Grant>&,const uint32_t*) asm("__wrap__Z13pn_shop_beginR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEERKSt6vectorINS2_5GrantESaIS6_EEPKj");
extern "C" bool prep_begin(map_session_data&,std::shared_ptr<pn_shop::Commit> request,const std::vector<pn_shop::Grant>& grants,const uint32_t*){++submitted;captured=request;captured_grants=grants;return true;}
extern "C" bool prep_submit(map_session_data&,std::shared_ptr<pn_shop::Commit>,std::vector<pn_shop::Event>,uint32_t) asm("__wrap__Z14pn_shop_submitR16map_session_dataSt10shared_ptrIN7pn_shop6CommitEESt6vectorINS2_5EventESaIS6_EEj");
extern "C" bool prep_submit(map_session_data&,std::shared_ptr<pn_shop::Commit> request,std::vector<pn_shop::Event>,uint32_t){++submitted;captured=request;return true;}
namespace {
int64 temporary(const char* key,int i=0){return pc_readreg(attached,reference_uid(add_str(key),i));}
void command(const std::string& s){invoke("@PNResult="+s+";");}
void prep_item(int id,item_types type,int buy,int sell){auto data=std::make_shared<item_data>();data->nameid=id;data->type=type;data->name="PrepItem";data->ename="Preparation Item";data->weight=10;data->value_buy=buy;data->value_sell=sell;data->equip=type==IT_ARMOR?EQP_HEAD_TOP:0;data->sex=SEX_BOTH;data->class_base[0]=UINT64_MAX;data->class_upper=ITEMJ_NORMAL;item_db.put(id,data);}
RunePlayer prep_player(){auto sd=rune_player();town=connected=true;submitted=batches=0;captured.reset();captured_grants.clear();sd->storage.type=TABLE_STORAGE;sd->storage.id=sd->status.account_id;sd->storage.stor_id=0;sd->storage.max_amount=5;sd->m=0;return sd;}
}
extern "C" int __wrap_main(int argc,char**){
    check(argc==2,"artifact directory supplied");deny_network();static char server[]="preparation-native-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();battle_config.atcommand_disable_npc=0;
    map[0].instance_id=0;
    num_reg_ers=ers_new(sizeof(script_reg_num),"prep:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"prep:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));pc_set_reg_load(false);
    prep_item(501,IT_HEALING,50,25);prep_item(502,IT_HEALING,200,100);prep_item(909,IT_ETC,20,10);prep_item(400999,IT_ARMOR,1000,500);
    const std::string skill_source="Id: 38\nName: MC_OVERCHARGE\nDescription: Overcharge\nMaxLevel: 10\n";
    auto skill_tree=ryml::parse_in_arena(ryml::to_csubstr(skill_source));check(skill_db.parseBodyNode(skill_tree.rootref())==1,"native selling skill metadata loaded");
    load_functions("npc/custom/main_office/preparation.txt");
    {auto sd=prep_player();invoke("callfunc \"PN_Preparation\";",{4});invoke("callfunc \"PN_InventoryTools\";",{6});}
    {auto sd=prep_player();command("pnprepsupply(0,0,501,20)");check(temporary("@PNResult")==1&&reg("PNPrepQuantity",0)==20,"persistent supply target");put(0,501,5);command("pnpreprestock(0,0)");check(temporary("@PNResult")==1&&temporary("@PNPrepTotal")==750&&temporary("@PNPrepAmount")==15,"missing only retail priced restock");Snapshot before;command("pnpreprestock(0,1)");check(submitted==1&&captured->wallet_after==captured->wallet_before-750&&captured_grants[0].amount==15,"durable purchase carries price and quantity");before.unchanged();command("pnpreprestock(0,1)");check(temporary("@PNResult")==-1&&submitted==1,"restock preview consumed prevents duplicate submission");}
    for(int mode=0;mode<4;++mode){auto sd=prep_player();command("pnprepsupply(0,0,501,20)");put(0,501,5);command("pnpreprestock(0,0)");if(mode==0)sd->inventory.u.items_inventory[0].amount=6;if(mode==1)sd->status.zeny=1;if(mode==2)town=false;if(mode==3)connected=false;command("pnpreprestock(0,1)");check(temporary("@PNResult")==-1&&!submitted,"stale quantity, insufficient wallet, unsafe map and disconnected restock refuse");}
    {auto sd=prep_player();command("pnprepsupply(0,0,400999,1)");check(!temporary("@PNResult"),"arbitrary equipment not a retail supply");command("pnprepsupply(0,0,501,20)");command("pnprepsupply(0,1,501,20)");check(!temporary("@PNResult"),"duplicate restock rows refuse");}
    for(int variant=0;variant<15;++variant){auto sd=prep_player();put(0,909,10);auto& it=sd->inventory.u.items_inventory[0];
        switch(variant){case 1:it.favorite=1;break;case 2:it.refine=1;break;case 3:it.card[0]=4001;break;case 4:it.option[0].id=1;break;case 5:it.option[0].value=1;break;case 6:it.option[0].param=1;break;case 7:it.bound=1;break;case 8:it.equip=EQP_HEAD_TOP;break;case 9:it.equipSwitch=EQP_HEAD_TOP;break;case 10:it.expire_time=1;break;case 11:it.unique_id=42;break;case 12:it.enchantgrade=1;break;case 13:it.identify=0;break;case 14:it.attribute=1;break;}
        command("pnprepjunk(0,909)");command("pnprepinventory(2,0)");check(temporary("@PNResult")==(!variant?1:0),"all protection fields filter junk sales");command("pnprepinventory(1,0)");check(temporary("@PNResult")==(!variant?1:0),"all protection fields filter deposits");}
    {auto sd=prep_player();put(0,909,10);weight();command("pnprepinventory(2,0)");check(!temporary("@PNResult"),"valuable misc is never junk by default");command("pnprepjunk(0,909)");command("pnprepinventory(2,0)");const auto price=temporary("@PNPrepTotal");Snapshot before;command("pnprepinventory(2,1)");check(submitted==1&&captured->kind==pn_shop::ItemUse&&!captured->items[0].nameid&&captured->wallet_after==captured->wallet_before+price,"durable junk sale removes only preview and credits exact retail sale");before.unchanged();}
    {auto sd=prep_player();put(0,909,10);command("pnprepjunk(0,909)");command("pnprepinventory(2,0)");const auto idx=skill_get_index(MC_OVERCHARGE);sd->status.skill[idx].id=MC_OVERCHARGE;sd->status.skill[idx].lv=10;command("pnprepinventory(2,1)");check(temporary("@PNResult")==-1&&!submitted,"native overcharge price changed after preview refuses");command("pnprepinventory(2,0)");check(temporary("@PNPrepTotal")==120,"native overcharge retail price visible in preview");}
    for(int mode=0;mode<4;++mode){auto sd=prep_player();put(0,909,10);command("pnprepjunk(0,909)");command("pnprepinventory(2,0)");if(mode==0)sd->inventory.u.items_inventory[0].favorite=1;if(mode==1)sd->inventory.u.items_inventory[0].amount=11;if(mode==2)command("pnprepjunk(0,0)");if(mode==3)town=false;command("pnprepinventory(2,1)");check(temporary("@PNResult")==-1&&!submitted,"UI yield mutation revalidated before sale");}
    {auto sd=prep_player();put(0,909,10);put(1,501,20);command("pnprepinventory(1,0)");sd->storage.max_amount=1;Snapshot before;command("pnprepinventory(1,1)");check(temporary("@PNResult")==-1&&!batches,"whole batch capacity refusal precedes transfer");before.unchanged();}
    {auto sd=prep_player();put(0,909,10);put(1,501,20);command("pnprepinventory(1,0)");command("pnprepinventory(1,1)");check(temporary("@PNResult")==2&&batches==1&&!count(909)&&!count(501)&&!sd->storage.state.put,"deposit uses one native batch and restores storage permissions");}
    {auto sd=prep_player();put(0,400999,1,true);sd->inventory.u.items_inventory[0].expire_time=0;command("pnpreppreset(0,1)");check(temporary("@PNResult")==1&&reg("PNPrepSaved",0)==1,"equipment identity persists natively");sd->inventory.u.items_inventory[0].unique_id=8;command("pnpreppreset(0,0)");check(!temporary("@PNResult")&&!unequips,"changed equipment refuses before first unequip");}
    {auto sd=prep_player();put(0,400999,1,true);sd->state.autoloot=1250;sd->state.autolootid[0]=501;command("pnpreppreset(0,1)");sd->state.autoloot=0;sd->state.autolootid[0]=0;command("pnpreppreset(0,0)");check(temporary("@PNResult")==1&&sd->inventory.u.items_inventory[0].equip==EQP_HEAD_TOP&&sd->state.autoloot==1250&&sd->state.autolootid[0]==501&&sd->state.autolooting,"native gear eligibility and saved loot restore");}
    {auto sd=prep_player();put(0,400999,1,true);command("pnpreppreset(0,1)");sd->inventory.u.items_inventory[0].equip=0;put(1,400999,1,true);sd->inventory.u.items_inventory[1].unique_id=81;sd->state.autoloot=987;fail_equip=true;command("pnpreppreset(0,0)");check(!temporary("@PNResult")&&!sd->inventory.u.items_inventory[0].equip&&sd->inventory.u.items_inventory[1].equip==EQP_HEAD_TOP&&sd->state.autoloot==987,"failed native equip restores previous gear and leaves loot unchanged");}
    {auto sd=prep_player();put(0,400999,1,true);sd->inventory.u.items_inventory[0].unique_id=0;put(1,400999,1);sd->inventory.u.items_inventory[1].unique_id=0;command("pnpreppreset(0,1)");check(!temporary("@PNResult"),"ambiguous identical equipment without GUID cannot be saved");}
    {auto sd=prep_player();put(0,909,10);command("pnprepinventory(1,0)");sd->storage.amount=sd->storage.max_amount;Snapshot before;command("pnprepinventory(1,1)");check(temporary("@PNResult")==-1&&!batches,"corrupt occupied count refuses before native transfer");before.unchanged();}
    {auto sd=prep_player();auto main=std::make_shared<s_storage_table>();strcpy(main->table,"storage");storage_db[0]=main;auto second=std::make_shared<s_storage_table>();strcpy(second->table,"pn_storage_01");second->id=100;storage_db[100]=second;
        sd->storage.u.items_storage[0].nameid=909;sd->storage.u.items_storage[0].amount=13;sd->storage.dirty=true;
        sd->premiumStorage.type=TABLE_STORAGE;sd->premiumStorage.id=sd->status.account_id;sd->premiumStorage.stor_id=100;sd->premiumStorage.u.items_storage[0].nameid=909;sd->premiumStorage.u.items_storage[0].amount=7;
        command("pnprepstorage(\"909\",0)");check(temporary("@PNResult")==1&&temporary("@PNPrepItem")==909&&temporary("@PNPrepAmount")==20,"search sums owned loaded pages including unsaved state without SQL");
        sd->premiumStorage.id=sd->status.account_id+1;command("pnprepstorage(\"909\",0)");check(temporary("@PNResult")==-1,"foreign loaded page never supplies another player's holdings");storage_db.clear();}
    {auto sd=prep_player();put(0,400999,1,true);command("pnpreppreset(0,1)");Snapshot before;command("pnpreppreset(0,2)");check(temporary("@PNResult")==1&&temporary("@PNPrepGearCount")==1&&!unequips&&!equips,"preview validates the saved equipment without changing it");before.unchanged();
        auto* c=compile("{mes \"[fixture]\";callfunc \"PN_PrepareAdventure\",0;end;}","combined preparation cancel");walk(c,{9,2});script_free_code(c);check(!submitted&&!unequips&&!equips,"canceling combined preparation has no charge or equipment change");}
    {auto sd=prep_player();put(0,400999,1,true);command("pnpreppreset(0,1)");sd->inventory.u.items_inventory[0].unique_id++;command("pnpreppreset(0,2)");check(!temporary("@PNResult")&&!unequips,"preview refuses missing exact gear before any mutation");}
    for(int mode=0;mode<4;++mode){auto sd=prep_player();put(0,400999,1,true);command("pnpreppreset(0,1)");
        if(mode!=0)command("pnprepsupply(0,0,501,20)");
        auto* c=compile("{mes \"[fixture]\";callfunc \"PN_PrepareAdventure\",0;end;}","combined preparation confirmation");
        walk(c,{9,1},[&](int,int chosen){if(chosen==1&&mode==2)sd->inventory.u.items_inventory[0].unique_id++;if(chosen==1&&mode==3)put(1,501,1);});script_free_code(c);
        if(mode==0)check(!submitted&&equips==1,"preparation with fulfilled supplies applies gear without a purchase");
        if(mode==1)check(submitted==1&&equips==1&&captured->wallet_after==captured->wallet_before-1000&&captured_grants[0].amount==20,"confirmed preparation applies gear and submits exact missing supplies once");
        if(mode>=2)check(!submitted&&!unequips&&!equips,"changed gear or supplies after destination menu refuses preparation before mutation");
    }
    attached=nullptr;item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();std::printf("PREPARATION_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
