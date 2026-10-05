#include "map/party.hpp"
#include "map/mapreg.hpp"
#include "common/mapindex.hpp"
#include <regex>
static party_data social_party{};
static std::string office_destination;static unsigned office_warps;
extern DBMap* mapindex_db;
extern "C" int16 social_map(uint16) asm("__wrap__Z18map_mapindex2mapidt");
extern "C" int16 social_map(uint16 index){return index>=1&&index<=4?index-1:-1;}
extern "C" void social_nav(const map_session_data*,const char*,uint16,uint16,uint8,bool,uint16) asm("__wrap__Z15clif_navigateToPK16map_session_dataPKctthbt");
extern "C" void social_nav(const map_session_data*,const char* map,uint16 x,uint16 y,uint8,bool,uint16){office_destination=std::string(map)+":"+std::to_string(x)+","+std::to_string(y);}
extern "C" e_setpos social_warp(map_session_data*,uint16,int32,int32,clr_type) asm("__wrap__Z9pc_setposP16map_session_datatii8clr_type");
extern "C" e_setpos social_warp(map_session_data* sd,uint16 index,int32 x,int32 y,clr_type){++office_warps;check(index==1&&x==50&&y==35,"legacy migration warp uses compact office entrance");sd->mapindex=index;sd->m=social_map(index);sd->x=x;sd->y=y;return SETPOS_OK;}
extern "C" const char* social_msg(const map_session_data*,int) asm("__wrap__Z11map_msg_txtPK16map_session_datai");
extern "C" const char* social_msg(const map_session_data*,int){return "Novice";}
extern "C" void social_clear(const map_session_data&,int) asm("__wrap__Z16clif_scriptclearRK16map_session_datai");
extern "C" void social_clear(const map_session_data&,int){}
extern "C" party_data* social_party_lookup(int32) asm("__wrap__Z12party_searchi");
extern "C" party_data* social_party_lookup(int32 id){return id==7?&social_party:nullptr;}
extern "C" bool social_num(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool social_num(map_session_data* sd,int64 key,int64 value){nums[key]=value;script_array_update(&sd->regs,key,value==0);return true;}
extern "C" bool social_str(map_session_data*,int64,const char*) asm("__wrap__Z18pc_setregistry_strP16map_session_datalPKc");
extern "C" bool social_str(map_session_data* sd,int64 key,const char* value){strings[key]=value;script_array_update(&sd->regs,key,!value||!*value);return true;}
extern "C" char* social_readstr(const map_session_data*,int64) asm("__wrap__Z19pc_readregistry_strPK16map_session_datal");
extern "C" char* social_readstr(const map_session_data*,int64 key){return strings[key].data();}
extern "C" bool social_mapstr(int64,const char*) asm("__wrap__Z16mapreg_setregstrlPKc");
extern "C" bool social_mapstr(int64 key,const char* value){strings[key]=value?value:"";return true;}
extern "C" char* social_mapreadstr(int64) asm("__wrap__Z17mapreg_readregstrl");
extern "C" char* social_mapreadstr(int64 key){return strings[key].data();}
static void load_functions(const std::string& path){
 auto source=read(path);std::regex declaration("function[\\t ]+script[\\t ]+([A-Za-z0-9_]+)[\\t ]+\\{");
 for(std::sregex_iterator it(source.begin(),source.end(),declaration),last;it!=last;++it){const std::string name=(*it)[1];strdb_put(script_get_userfunc_db(),name.c_str(),compile(body(source,(*it).str()),name.c_str()));}
}
static void exec_social(const std::string& text,const std::vector<int>& choices={}){auto* c=compile("{"+text+" end;}","social fixture");walk(c,choices);script_free_code(c);}
static int64 n(const char* name,int index=0){return nums[reference_uid(add_str(name),index)];}
static std::string s(const char* name,int index=0){return strings[reference_uid(add_str(name),index)];}
static bool said(const char* text){for(const auto& m:messages)if(m.find(text)!=std::string::npos)return true;return false;}
extern "C" int __wrap_main(int argc,char**){
 check(argc==1,"no external service arguments");deny_network();static char name[]="social-features-test";SERVER_NAME=name;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 mapindex_db=strdb_alloc(DB_OPT_DUP_KEY,MAP_NAME_LENGTH);map_num=4;std::vector<mapcell> office_cells(10000);for(auto& cell:office_cells){cell.walkable=1;cell.shootable=1;}
 const std::vector<std::string> office_maps={"pn_office","pn_train","pn_style","prontera"};for(int i=0;i<4;++i){mapindex_addmap(i+1,office_maps[i].c_str());std::strcpy(map[i].name,office_maps[i].c_str());map[i].index=i+1;map[i].xs=map[i].ys=100;map[i].cell=office_cells.data();map[i].initMapFlags();}
 for(auto path:{"npc/custom/main_office/party_board.txt","npc/custom/main_office/build_sharing.txt","npc/custom/main_office/lab_challenges.txt","npc/custom/quality_services.txt"})load_functions(path);
 auto* console=compile(body(read("npc/custom/quality_services.txt"),"\tscript\tPN Lab Console"),"full damage console");script_free_code(console);
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;sd->status.base_level=100;std::strcpy(sd->status.name,"TestPlayer");sd->status.zeny=7654321;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;
 auto crown=std::make_shared<item_data>();crown->nameid=900001;crown->name="Test_Crown";crown->ename="Test Crown";crown->type=IT_ARMOR;item_db.put(crown->nameid,crown);
 sd->inventory_data[0]=crown.get();auto& gear=sd->inventory.u.items_inventory[0];gear.nameid=crown->nameid;gear.amount=1;gear.identify=1;gear.equip=EQP_HEAD_TOP;gear.refine=12;gear.card[0]=4365;gear.option[0].id=1;gear.option[0].value=23;sd->equip_index[EQI_HEAD_TOP]=0;
 sd->status.party_id=7;social_party.party.party_id=7;social_party.data[0].sd=sd.get();social_party.party.member[0].account_id=sd->id;social_party.party.member[0].char_id=sd->status.char_id;social_party.party.member[0].leader=1;std::strcpy(social_party.party.member[0].name,"TestPlayer");
 Snapshot assets;
 ++cases;exec_social("Result=callfunc(\"PN_PartyPublish\",2,1,1,\"Meet at the gate\");");check(n("Result")==1&&n("$@PNPartyOwner")==sd->status.char_id,"leader publication owns its actual character");
 exec_social("Result=callfunc(\"PN_PartyPublish\",4,2,0,\"Updated\");");check(n("$@PNPartyActivity")==4&&!n("$@PNPartyOwner",1),"refresh replaces one listing without duplicates");
 ++cases;social_party.party.member[0].leader=0;exec_social("Result=callfunc(\"PN_PartyPublish\",0,0,0,\"Forged\");");check(!n("Result")&&s("$@PNPartyNote$")=="Updated","nonleader cannot publish or overwrite");
 exec_social("Result=callfunc(\"PN_PartyListingValid\",0);");check(!n("Result")&&!n("$@PNPartyOwner"),"leadership change invalidates stale listing");social_party.party.member[0].leader=1;
 ++cases;exec_social("callfunc \"PN_PartyPublish\",0,0,1,\"Test\"; $@PNPartyUntil[0]=0; Result=callfunc(\"PN_PartyListingValid\",0);");check(!n("Result"),"expiry clears listing");
 exec_social("callfunc \"PN_PartyPublish\",0,0,1,\"Test\"; $@PNPartyOwner[0]=999; Result=callfunc(\"PN_PartyListingValid\",0);");check(!n("Result"),"offline or different character is never advertised");
 ++cases;exec_social("Result=callfunc(\"PN_PartyPublish\",8,0,0,\"Test\");");check(!n("Result"),"invalid activity cannot be published");
 ++cases;exec_social("PNBuildNote$[0]=\"Rotation: use skills carefully\"; PNBuildStats$[0]=\"STR 10 AGI 20\"; callfunc \"PN_BuildSharing\";",{1,1,2});check(!n("$PNShareOwner"),"canceling public consent publishes nothing");
 exec_social("Share=callfunc(\"PN_BuildSharePublish\",0);");const auto first=n("Share");check(first>100000&&n("$PNShareOwner")==sd->status.char_id&&s("$PNShareNote$")==s("PNBuildNote$"),"explicit publication creates public persistent snapshot");
 std::printf("EQUIP_SNAPSHOT %s | %s | %s\n",s("$PNShareEquip$",EQI_HEAD_TOP).c_str(),s("$PNShareCards$",EQI_HEAD_TOP).c_str(),s("$PNShareOpts$",EQI_HEAD_TOP).c_str());
 check(s("$PNShareEquip$",EQI_HEAD_TOP).find("+12 Test Crown")!=std::string::npos&&s("$PNShareCards$",EQI_HEAD_TOP).find("4365")!=std::string::npos&&s("$PNShareOpts$",EQI_HEAD_TOP).find("1:23:0")!=std::string::npos,"snapshot includes real equipped refine cards and options");
 exec_social("PNBuildNote$[0]=\"Changed private note\"; callfunc \"PN_BuildShareShow\",Share;");check(said("Rotation: use skills carefully")&&!said("Changed private note"),"public lookup reads snapshot rather than current private notes");
 ++cases;sd->status.char_id++;exec_social("Result=callfunc(\"PN_BuildShareRemove\");");check(!n("Result")&&n("$PNShareCode")==first,"another character cannot unpublish author snapshot");sd->status.char_id--;
 exec_social("Share=callfunc(\"PN_BuildSharePublish\",0); Result=callfunc(\"PN_BuildShareFind\","+std::to_string(first)+");");check(n("Share")>first&&n("Result")==-1&&!n("$PNShareOwner",1),"replacement invalidates original code and uses one slot");
 exec_social("callfunc \"PN_BuildShareRemove\"; Result=callfunc(\"PN_BuildShareFind\",Share);");check(n("Result")==-1&&s("$PNShareNote$").empty()&&s("$PNShareName$").empty(),"unpublication removes public content");
 ++cases;exec_social("$PNShareSerial=2147483647; Result=callfunc(\"PN_BuildSharePublish\",0);");check(!n("Result")&&!n("$PNShareOwner"),"share-code exhaustion fails without publication");exec_social("$PNShareSerial=100010; Share=callfunc(\"PN_BuildSharePublish\",0); $PNShareUntil[0]=0; Result=callfunc(\"PN_BuildShareFind\",Share);");check(n("Result")==-1,"expired codes are inaccessible");
 exec_social("callfunc \"PN_BuildShareRemove\"; Result=callfunc(\"PN_BuildSharePublish\",-1); Result2=callfunc(\"PN_PartyListingValid\",-1);");check(!n("Result")&&!n("Result2"),"negative indexes are refused before array reads in eager VM");
 ++cases;exec_social("for (.@i=0;.@i<256;.@i++) {$PNShareOwner[.@i]=1000+.@i; $PNShareUntil[.@i]=gettimetick(2)+600;}");exec_social("Result=callfunc(\"PN_BuildSharePublish\",0);");check(!n("Result"),"full unexpired registry refuses publication without stealing a slot");
 exec_social("$PNShareUntil[255]=0; Share=callfunc(\"PN_BuildSharePublish\",0);");check(n("Share")>100010&&n("$PNShareOwner",255)==sd->status.char_id,"expired last slot is reclaimed for current owner");
 exec_social("Result=callfunc(\"PN_BuildShareFind\",Share);");check(n("Result")==255,"bounded code lookup reaches last registry slot");exec_social("callfunc \"PN_BuildSharing\";",{3});check(said("Changed private note"),"my snapshot UI can reach last slot under production loop limits");
 ++cases;for(int i=0;i<3;++i){exec_social("Result=callfunc(\"PN_LabChallengeLoad\","+std::to_string(i)+");");check(n("Result")==1&&n("@PNLabChallenge")==i+1&&n("@PNLabTarget",4)>=1,"all fixed targets validate under production target rules");}
 exec_social("Result=callfunc(\"PN_LabChallengeLoad\",3);");check(!n("Result"),"invalid challenge index refused");
 ++cases;auto lab_source=read("npc/custom/quality_services.txt");auto queue_start=lab_source.find("@PNLabChallenge=0; @PNLabGoal=0;");auto queue_end=lab_source.find("mes \"Give this run a short label",queue_start);check(queue_start!=std::string::npos&&queue_end!=std::string::npos,"exact live console queue block exists");
 exec_social("@PNLabPendingChallenge=2; @PNLabPendingGoal=1500; "+lab_source.substr(queue_start,queue_end-queue_start)+" QueuedLoaded=.@loaded;");check(n("QueuedLoaded")==1&&n("@PNLabChallenge")==2&&n("@PNLabGoal")==1500&&!n("@PNLabPendingChallenge")&&!n("@PNLabPendingGoal"),"console consumes queued exact challenge and goal without showing ordinary target menu");
 auto save=[](int damage,int ms=60000,const char* target="target",const char* buff="buff",const char* build="build",int changed=0){exec_social("Result=callfunc(\"PN_LabBestSave\",\"Run\","+std::to_string(damage)+","+std::to_string(ms)+",\""+target+"\",\"gear\",\""+buff+"\",\""+build+"\","+std::to_string(changed)+");");};
 ++cases;save(60000);check(n("Result")==1&&n("PNBestDps")==1000,"completed stable run creates best");save(30000);check(!n("Result")&&n("PNBestDps")==1000,"slower repeat preserves best");save(120000);check(n("Result")==1&&n("PNBestDps")==2000&&!n("PNBestValid",1),"faster exact repeat replaces same best");
 for(int kind=0;kind<3;++kind){save(900000,kind==0?59999:60000,"target","buff",kind==1?"":"build",kind==2);check(!n("Result")&&n("PNBestDps")==2000,"incomplete unstable or unverifiable runs cannot become best");}
 save(30000,60000,"other target");save(30000,60000,"target","other buff");save(30000,60000,"target","buff","other build");check(n("PNBestValid",3)==1,"incompatible targets buffs and builds retain separate bests");
 auto before=nums;auto before_strings=strings;exec_social("callfunc \"PN_LabBests\";");check(nums==before&&strings==before_strings,"viewing bests is read only");
 ++cases;for(int i=0;i<15;++i)save(60000,60000,("new target "+std::to_string(i)).c_str());int valid=0;for(int i=0;i<12;++i)valid+=n("PNBestValid",i)!=0;check(valid==12,"new target combinations remain bounded at twelve personal bests");
 auto persistent_before=nums;auto persistent_strings=strings;if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);sd->regs.arrays=nullptr;
 auto relog=std::make_unique<map_session_data>(*sd);sd.reset();sd=std::move(relog);attached=sd.get();exec_social("callfunc \"PN_LabBests\";");check(nums==persistent_before&&strings==persistent_strings,"character recreation keeps saved bests readable without modifying registry");social_party.data[0].sd=sd.get();
 // Dashboard route tests execute its actual VM menu. Exported feature stubs
 // record routing only; separate cases above execute the feature code itself.
 for(const char* target:{"CH1_Complete","CH2_Complete"})strdb_put(script_get_userfunc_db(),target,compile("{return 0;}",target));
 load_functions("npc/custom/main_office/services.txt");load_functions("npc/custom/main_office/placements.txt");load_functions("npc/custom/main_office/adventure.txt");
 const std::vector<std::string> office_expected={"pn_office:30,29","pn_office:29,43","pn_office:43,43","pn_office:62,43","pn_office:37,67"};
 for(int group=1;group<=5;++group){++cases;office_destination.clear();exec_social("callfunc \"PN_OfficeFind\";",{group,1});check(office_destination==office_expected[group-1],"actual directory category navigates to first generated compact-office desk");}
 auto office_source=read("npc/custom/main_office/services.txt");auto welcome=body(office_source,"\tscript\tPN Office Welcome");auto* login=compile("{goto OnPCLoginEvent; "+welcome.substr(1,welcome.size()-2)+"}","actual office login migration");
 for(int legacy=0;legacy<4;++legacy){++cases;std::strcpy(sd->status.save_point.map,office_maps[legacy].c_str());sd->status.save_point.x=88;sd->status.save_point.y=89;sd->m=3;sd->x=60;sd->y=60;office_warps=0;walk(login,{});if(legacy<3)check(std::string(sd->status.save_point.map)=="pn_office"&&sd->status.save_point.x==50&&sd->status.save_point.y==35,"old office/training/fashion savepoint moves to compact entrance");else check(std::string(sd->status.save_point.map)=="prontera"&&sd->status.save_point.x==88&&sd->status.save_point.y==89,"unrelated town savepoint remains unchanged");check(!office_warps,"savepoint migration alone does not move player in unrelated town");}
 for(int legacy=0;legacy<3;++legacy){++cases;sd->m=legacy;sd->x=88;sd->y=89;office_warps=0;walk(login,{});check(office_warps==1&&sd->m==0&&sd->x==50&&sd->y==35,"login on old floors or outside compact office redirects to valid entrance");}
 sd->m=0;sd->x=50;sd->y=35;office_warps=0;walk(login,{});check(!office_warps,"existing compact entrance login does not warp again");script_free_code(login);
 auto adventure=read("npc/custom/main_office/adventure.txt");
 for(auto name:{"PN Adventure Desk","PN Preparation Desk","PN Inventory Desk","PN Party Desk","PN Reward Desk"}){auto* c=compile(body(adventure,std::string("\tscript\t")+name),name);script_free_code(c);}
 const std::vector<std::string> routes={"PN_GuideNext","PN_PartyBoard","PN_GuideNext","PN_EquipmentPlanner","PN_InstanceReadinessDesk","PN_Preparation","PN_InventoryTools","PN_WeeklyBoard","PN_RewardProgress","PN_BuildSharing","PN_OfficeFind"};
 for(auto& target:routes){auto* c=compile("{ Route$=\""+target+"\"; return;}",target.c_str());auto* old=static_cast<script_code*>(strdb_get(script_get_userfunc_db(),target.c_str()));if(old)script_free_code(old);strdb_put(script_get_userfunc_db(),target.c_str(),c);}
 for(const char* target:{"PN_LabChallengeMenu"}){auto* c=compile("{ Route$=\""+std::string(target)+"\"; return;}",target);auto* old=static_cast<script_code*>(strdb_get(script_get_userfunc_db(),target));if(old)script_free_code(old);strdb_put(script_get_userfunc_db(),target,c);}
 for(int action=1;action<=11;++action){++cases;exec_social("Route$=\"\"; callfunc \"PN_Adventure\";",action==1?std::vector<int>{1,1,12}:std::vector<int>{action,12});check(s("Route$")==routes[action-1],"each dashboard menu action reaches its documented interface");}
 for(int option=1;option<=5;++option){++cases;exec_social("Route$=\"\"; callfunc \"PN_Adventure\";",{1,option,12});const std::vector<std::string> expected={"PN_GuideNext","PN_LabChallengeMenu","PN_WeeklyBoard","PN_EquipmentPlanner",""};check(s("Route$")==expected[option-1],"all short-session choices including Back route correctly");}
 exec_social("Route$=\"\"; callfunc \"PN_Adventure\";",{12});check(s("Route$").empty(),"leaving dashboard invokes no service");
 assets.unchanged();check(!unequips&&!equips,"social features never equip or mutate inventory");
 if(sd->regs.arrays)sd->regs.arrays->destroy(sd->regs.arrays,script_free_array_db);sd->regs.arrays=nullptr;attached=nullptr;sd.reset();nums.clear();strings.clear();item_db.clear();
 if(::regs.arrays)::regs.arrays->destroy(::regs.arrays,script_free_array_db);::regs.arrays=nullptr;
 do_final_script();mapindex_final();for(int i=0;i<4;++i){map[i].cell=nullptr;}map_num=0;timer_final();db_final();malloc_final();
 std::printf("SOCIAL_FEATURES_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
