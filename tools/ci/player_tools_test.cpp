#include "map/npc.hpp"
#include "map/storage.hpp"
#include "map/mob.hpp"
#include "common/ers.hpp"
#include "common/sql.hpp"
#include <nlohmann/json.hpp>
#include <regex>
npc_data* planner_npc=nullptr;
static std::string planner_destination;
extern "C" int16 planner_map(uint16) asm("__wrap__Z18map_mapindex2mapidt");
extern "C" int16 planner_map(uint16 index){return index==1?0:-1;}
extern "C" bool planner_set(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool planner_set(map_session_data* sd,int64 key,int64 value){nums[key]=value;script_array_update(&sd->regs,key,value==0);return true;}
extern "C" void planner_nav(const map_session_data*,const char*,uint16,uint16,uint8,bool,uint16) asm("__wrap__Z15clif_navigateToPK16map_session_dataPKctthbt");
extern "C" void planner_nav(const map_session_data*,const char* map,uint16 x,uint16 y,uint8,bool,uint16){planner_destination=std::string(map)+":"+std::to_string(x)+","+std::to_string(y);}
static void functions(const std::string& source){std::regex pattern("function[\\t ]+script[\\t ]+([A-Za-z0-9_]+)[\\t ]+\\{");for(std::sregex_iterator it(source.begin(),source.end(),pattern),end;it!=end;++it){std::string name=(*it)[1];strdb_put(script_get_userfunc_db(),name.c_str(),compile(body(source,(*it).str()),name.c_str()));}}
static void invoke(const std::string& command,const std::vector<int>& choices={}){++cases;auto* code=compile("{"+command+" end;}","player tools");walk(code,choices);script_free_code(code);}
static int64 num(const char* key,int index=0){return nums[reference_uid(add_str(key),index)];}
static std::string str(const char* key,int index=0){return strings[reference_uid(add_str(key),index)];}
static bool said(const char* part){for(const auto& text:messages)if(text.find(part)!=std::string::npos)return true;return false;}
static void page(int id,const char* table){auto entry=std::make_shared<s_storage_table>();entry->id=id;entry->max_num=MAX_STORAGE;std::strcpy(entry->table,table);storage_db[id]=entry;}
static void sql(const std::string& query){check(Sql_QueryStr(qsmysql_handle,query.c_str())==SQL_SUCCESS,"fixture SQL succeeds");Sql_FreeResult(qsmysql_handle);}
static std::string scalar(const std::string& query){check(Sql_QueryStr(qsmysql_handle,query.c_str())==SQL_SUCCESS,"fixture scalar query");check(Sql_NextRow(qsmysql_handle)==SQL_SUCCESS,"fixture scalar row");char* value=nullptr;size_t size=0;check(Sql_GetData(qsmysql_handle,0,&value,&size)==SQL_SUCCESS && value,"fixture scalar data");std::string result(value,size);Sql_FreeResult(qsmysql_handle);return result;}
extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2 || argc==3,"fixture arguments");const bool database=argc==3;if(!database)deny_network();
    static char server[]="player-tools-test";SERVER_NAME=server;malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    num_reg_ers=ers_new(sizeof(script_reg_num),"planner:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));str_reg_ers=ers_new(sizeof(script_reg_str),"planner:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    for(const char* name:{"items","shops"}){auto source=read(std::string(argv[1])+"/"+name+".yml");auto tree=ryml::parse_in_arena(ryml::to_csubstr(source));for(auto row:tree["Body"])check(std::string(name)=="items"?item_db.parseBodyNode(row)==1:barter_db.parseBodyNode(row)==1,"production metadata parser");}
    functions(read("npc/custom/main_office/equipment_planner.txt"));functions(read("npc/custom/main_office/purchase_history.txt"));
    auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;sd->vars_ok=true;sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    map_num=1;strcpy(map[0].name,"prontera");map[0].initMapFlags();npc_data npc{};npc.id=99000100;npc.m=0;npc.x=150;npc.y=180;strcpy(npc.name,"Fixture exchange");planner_npc=&npc;
    auto shop=barter_db.find("barter_ep21_gaebolg_equipment");check(shop!=nullptr,"actual Episode 21 recipe exists");shop->npcid=npc.id;
    const int gear=shop->items.at(0)->nameid,material=shop->items.at(0)->requirements.at(0)->nameid;
    put(0,material,25);page(0,"planner_regular");page(117,"planner_character");page(102,"planner_paid");
    sd->storage.type=sd->premiumStorage.type=TABLE_STORAGE;sd->storage.id=sd->status.account_id;sd->premiumStorage.id=sd->status.char_id;sd->premiumStorage.stor_id=117;
    sd->storage.u.items_storage[0].nameid=material;sd->storage.u.items_storage[0].amount=60;sd->storage.dirty=true;
    sd->premiumStorage.u.items_storage[0].nameid=material;sd->premiumStorage.u.items_storage[0].amount=10;
    Snapshot before;auto original_storage=sd->storage,original_character=sd->premiumStorage;
    invoke("@result=pnplansearch(\"gaebolg\",0);");check(num("@result")==10 && !num("@PNPlanMore"),"actual ten equipment outputs are searchable");
    invoke("@result=pnplansearch(\""+std::to_string(gear)+"\",0);");check(num("@result")==1 && num("@PNPlanItem")==gear,"numeric search preserves equipment identity");
    invoke("@result=pnplansearch(\"gaebolg\",-1);");check(num("@result")==0,"negative pages rejected");
    invoke("@result=pnplansearch(\"\",0);");check(num("@result")==15 && num("@PNPlanMore"),"catalog uses bounded pages");const auto page_end=num("@PNPlanItem",14);
    invoke("@result=pnplansearch(\"\",1);");check(num("@result")>0 && num("@PNPlanItem")>page_end,"next page advances in stable item order");
    invoke("@result=pnplanrecipes("+std::to_string(gear)+");");check(num("@result")==1,"actual recipe loaded");
    const std::string recipe="@result=pnplanrecipe(\"barter_ep21_gaebolg_equipment\",0);";
    invoke(recipe);check(num("@result")==1 && num("@PNRequired")==100 && num("@PNInventory")==25 && num("@PNStorage")==70 && num("@PNPlanKnown")==1,"owned live account/character pages counted once; locked pages excluded");
    check(str("@PNPlanMap$")=="prontera" && num("@PNPlanX")==150,"loaded NPC is authoritative navigation destination");
    for(int kind=0;kind<3;++kind){if(!kind)sd->multi_storage.loading=true;if(kind==1)sd->multi_storage.pending=true;if(kind==2)sd->state.storage_flag=1;invoke(recipe);check(!num("@PNPlanKnown") && num("@PNStorage")==-1,"busy holdings are unknown, not zero");sd->multi_storage.loading=sd->multi_storage.pending=false;sd->state.storage_flag=0;}
    invoke("callfunc \"PN_PlanGoal\","+std::to_string(gear)+";",{2,4});check(num("PNWishItem")==gear,"wishlist persists character item ID");
    auto saved=nums;for(auto it=saved.begin();it!=saved.end();)if(get_str(script_getvarid(it->first))[0]=='@')it=saved.erase(it);else ++it;
    nums=saved;invoke("callfunc \"PN_PlanGoal\","+std::to_string(gear)+";",{4});check(num("PNWishItem")==gear,"wishlist survives loss of session variables");
    invoke("callfunc \"PN_PlanGoal\","+std::to_string(gear)+";",{2,4});check(num("PNWishItem")==0,"remove wishlist goal");
    messages.clear();invoke("callfunc \"PN_PlanRecipe\",\"barter_ep21_gaebolg_equipment\",0;",{3});check(said("missing quantity 5"),"actual dialog shows held/required/missing totals");
    auto mob=std::make_shared<s_mob_db>();mob->id=1002;mob->jname="Fixture Poring";auto drop=std::make_shared<s_mob_drop>();drop->nameid=material;drop->rate=100;mob->dropitem.push_back(drop);mob_db.put(mob->id,mob);mob_spawn_data[1002].push_back({1,5});
    invoke("@result=pnplansources("+std::to_string(material)+",0);");check(num("@result")==1 && str("@PNSourceMap$")=="prontera","drop source requires an actual loaded spawn");mob_spawn_data.clear();invoke("@result=pnplansources("+std::to_string(material)+",0);");check(num("@result")==0,"unspawned monster is not presented as a farming destination");
    if(database){
        qsmysql_handle=Sql_Malloc();check(Sql_Connect(qsmysql_handle,"root","shop-fixture-only","shop-recovery-db",3306,"shop_recovery_probe")==SQL_SUCCESS,"isolated SQL connection");check(scalar("SELECT DATABASE()") == "shop_recovery_probe","fixture database guard");
        for(const char* table:{"planner_regular","planner_character","planner_paid"}){sql(std::string("CREATE TABLE ")+table+" (account_id INT,nameid INT,amount INT) ENGINE=InnoDB");}
        sql("INSERT INTO planner_regular VALUES(99000001,"+std::to_string(material)+",999),(123,"+std::to_string(material)+",888)");
        sql("INSERT INTO planner_character VALUES(99000002,"+std::to_string(material)+",35),(99000003,"+std::to_string(material)+",777)");
        sql("INSERT INTO planner_paid VALUES(99000001,"+std::to_string(material)+",50)");
        sd->premiumStorage.id=0;invoke(recipe);check(num("@PNStorage")==95 && num("@PNPlanKnown"),"SQL uses character owner and excludes stale cached/other owners/locked storage");
        nums[reference_uid(add_str("#PNStoragePaid"),102)]=1;invoke(recipe);check(num("@PNStorage")==145,"only unlocked paid page enters SQL total");
        sd->storage.id=0;invoke(recipe);check(num("@PNStorage")==1084,"unloaded regular page reads authoritative SQL without another account");
        page(101,"planner_regular");invoke(recipe);check(!num("@PNPlanKnown"),"aliased storage tables cannot double-count");storage_db.erase(101);
        sql("DROP TABLE planner_paid");invoke(recipe);check(!num("@PNPlanKnown") && num("@PNStorage")==-1,"SQL failure does not fabricate missing quantities");nums.erase(reference_uid(add_str("#PNStoragePaid"),102));errors=0;
        nlohmann::json detail={{"version",1},{"zeny",-100},{"cash",0},{"kafra",0},{"items",{{gear,1},{material,-100}}},{"pets",nlohmann::json::array()}};
        const auto encoded=detail.dump();SqlStmt stmt{*qsmysql_handle};check(stmt.Prepare("INSERT INTO pn_purchase_history(account_id,char_id,nonce_hi,nonce_lo,sequence,kind,outcome,details) VALUES(99000001,99000002,111,222,1,2,1,?)")==SQL_SUCCESS && stmt.BindParam(0,SQLDT_STRING,const_cast<char*>(encoded.data()),encoded.size())==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS,"seed owned display receipt");
        sql("INSERT INTO pn_purchase_history(account_id,char_id,nonce_hi,nonce_lo,sequence,kind,outcome,details) VALUES(99000001,99000003,111,222,2,1,1,'{}'),(99000004,99000002,111,222,3,1,1,'{}')");
        sql("INSERT INTO pn_pet_entitlements(account_id,char_id,nonce_hi,nonce_lo,sequence,ordinal,pet_id,payload,egg,claimed) VALUES(99000001,99000002,111,222,1,0,900001,'','',0),(99000001,99000002,111,222,1,1,900002,'','',1),(99000001,99000003,111,222,1,2,900003,'','',0)");
        const auto records=scalar("SELECT COUNT(*) FROM pn_purchase_history");
        invoke("@result=pnpurchasehistory(0);");check(num("@result")==1 && str("@PNHistoryRef$")=="111-222-1","both account and character limit history");
        invoke("@result=pnpurchasedetail(\"111-222-1\");");check(num("@result")>5,"owned detail loads");
        bool charged=false,output=false;for(int i=0;i<num("@result");++i){charged|=str("@PNHistoryLine$",i).find("zeny: spent 100")!=std::string::npos;output|=str("@PNHistoryLine$",i).find("+1 x ")!=std::string::npos;}check(charged&&output,"history displays recorded currency and outputs");
        bool pending=false,recovered=false;for(int i=0;i<num("@result");++i){pending|=str("@PNHistoryLine$",i).find("1 pet reward(s) awaiting recovery")!=std::string::npos;recovered|=str("@PNHistoryLine$",i).find("1 pet reward(s) already recovered")!=std::string::npos;}check(pending&&recovered,"current pet status excludes another character's entitlement");
        sql("UPDATE pn_pet_entitlements SET claimed=1 WHERE pet_id=900001");invoke("@result=pnpurchasedetail(\"111-222-1\");");recovered=false;for(int i=0;i<num("@result");++i)recovered|=str("@PNHistoryLine$",i).find("2 pet reward(s) already recovered")!=std::string::npos;check(recovered,"history reflects later recovery without replaying original delivery");
        for(const char* ref:{"111-222-2","111-222-3","111-222-1 OR 1=1","18446744073709551616-222-1"}){invoke(std::string("@result=pnpurchasedetail(\"")+ref+"\");");check(num("@result")==-1,"foreign owner and invalid references refused");}
        messages.clear();invoke("callfunc \"PN_PurchaseHistory\";",{1,4});check(said("spent 100")&&said("Support reference"),"actual history dialogue includes costs and support identity");
        check(scalar("SELECT COUNT(*) FROM pn_purchase_history")==records,"history reads never issue another receipt or deliver");
        Sql_Free(qsmysql_handle);qsmysql_handle=nullptr;
    }else{invoke("@result=pnpurchasehistory(0);");check(num("@result")==-1,"history unavailable without DB, never invented empty");}
    before.unchanged();sd->storage=original_storage;sd->premiumStorage=original_character;
    if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);sd->regs.arrays=nullptr;attached=nullptr;planner_npc=nullptr;
    shop->npcid=0;barter_db.clear();storage_db.clear();mob_db.clear();mob_spawn_data.clear();item_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);timer_final();db_final();malloc_final();printf("PLAYER_TOOLS_OK cases=%u assertions=%u sql=%d\n",cases,assertions,database);return 0;
}
