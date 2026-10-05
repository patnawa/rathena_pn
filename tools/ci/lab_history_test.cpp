extern "C" bool lab_set(map_session_data*,int64,int64) asm("__wrap__Z14pc_setregistryP16map_session_datall");
extern "C" bool lab_set(map_session_data* sd,int64 key,int64 value){nums[key]=value;script_array_update(&sd->regs,key,value==0);return true;}
extern "C" bool lab_setstr(map_session_data*,int64,const char*) asm("__wrap__Z18pc_setregistry_strP16map_session_datalPKc");
extern "C" bool lab_setstr(map_session_data* sd,int64 key,const char* value){strings[key]=value;script_array_update(&sd->regs,key,!value||!*value);return true;}
extern "C" char* lab_readstr(const map_session_data*,int64) asm("__wrap__Z19pc_readregistry_strPK16map_session_datal");
extern "C" char* lab_readstr(const map_session_data*,int64 key){return strings[key].data();}
static void exec_lab(const std::string& text,const std::vector<int>& choices={}) {
 auto* code=compile("{ "+text+" end; }","lab fixture");walk(code,choices);script_free_code(code);
}
static bool said(const char* part){for(const auto& m:messages)if(m.find(part)!=std::string::npos)return true;return false;}
static std::unique_ptr<map_session_data> lab_player(){
 auto sd=std::make_unique<map_session_data>();attached=sd.get();sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
 sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
 for(auto& i:sd->equip_index)i=-1;return sd;
}
static void lab_detach(){if(attached->regs.arrays)attached->regs.arrays->destroy(attached->regs.arrays,script_free_array_db);attached->regs.arrays=nullptr;attached=nullptr;}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==2,"source");deny_network();static char server[]="lab-history-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 const auto source=read(argv[1]);
 for(auto name:{"PN_LabSave","PN_LabCompatible","PN_LabHistory"}) {
  strdb_put(script_get_userfunc_db(),name,compile(body(source,std::string("function\tscript\t")+name),name));
 }
 auto sd=lab_player();
 ++cases;
 for(int d:{3000,1000,2000})exec_lab("callfunc \"PN_LabSave\",\"repeat\","+std::to_string(d*60)+",60000,\"target\",\"gear\",\"buff\",\"build\",0;");
 exec_lab("callfunc \"PN_LabHistory\";",{1});
 check(said("median DPS 2000; min/max 1000 / 3000; spread 2000"),"three-run median and spread from production VM");
 ++cases;
 auto before_num=nums;auto before_str=strings;
 exec_lab("callfunc \"PN_LabSave\",\"interrupted\",999,59999,\"target\",\"gear\",\"buff\",\"build\",0;");
 check(nums==before_num&&strings==before_str,"interrupted short run cannot replace history");
 ++cases;
 lab_detach();sd.reset();sd=lab_player();messages.clear();
 exec_lab("callfunc \"PN_LabHistory\";",{2});
 check(said("Identical setup repeats: 3; median DPS 2000"),"three persistent runs survive recreated player; character registry boundary retained");
 ++cases;
 exec_lab("callfunc \"PN_LabSave\",\"different target\",60000,60000,\"other\",\"new gear\",\"buff\",\"build\",0; LabCompat=callfunc(\"PN_LabCompatible\",0,1);");
 check(nums[add_str("LabCompat")]==0,"different targets refused despite gear A/B change");
 ++cases;
 exec_lab("callfunc \"PN_LabSave\",\"buff changed\",60000,60000,\"target\",\"gear\",\"buff\",\"build\",1; LabCompat=callfunc(\"PN_LabCompatible\",1,2);");
 check(nums[add_str("LabCompat")]==0,"variable buff run excluded");
 ++cases;
 exec_lab("callfunc \"PN_LabSave\",\"new build\",60000,60000,\"target\",\"gear\",\"buff\",\"build2\",0; PNLabSetup$[0]=\"target\"; LabCompat=callfunc(\"PN_LabCompatible\",2,0);");
 check(nums[add_str("LabCompat")]==0,"different builds refused");
 exec_lab("PNLabBuild$[2]=\"build\"; PNLabBuff$[2]=\"other buff\"; LabCompat=callfunc(\"PN_LabCompatible\",2,0);");
 check(nums[add_str("LabCompat")]==0,"different initial buffs refused");
 ++cases;
 for(auto gear:{"gear A","gear B","gear A"})exec_lab(std::string("callfunc \"PN_LabSave\",\"ab\",60000,60000,\"target\",\"")+gear+"\",\"buff\",\"build\",0;");
 messages.clear();exec_lab("callfunc \"PN_LabHistory\";",{1});
 check(said("A/B candidate run 2")&&said("Identical setup repeats: 2"),"different equipment remains A/B but excluded from repeat median");
 ++cases;
 auto identity=[&](int mode){exec_lab("LabIdentity$=pnlabidentity("+std::to_string(mode)+");");return strings[add_str("LabIdentity$")];};
 auto gear=identity(0);sd->status.pow++;check(identity(0)!=gear,"trait fingerprint");
 sd->status.pow--;sd->equip_index[EQI_ARMOR]=0;auto& it=sd->inventory.u.items_inventory[0];it.nameid=2307;it.refine=7;it.equip=EQP_ARMOR;
 gear=identity(0);it.refine++;check(identity(0)!=gear,"refine fingerprint");it.refine--;
 it.card[3]=4700;check(identity(0)!=gear,"enchant fingerprint");it.card[3]=0;
 it.option[0].id=1;it.option[0].value=5;check(identity(0)!=gear,"random option fingerprint");it.option[0]={};
 it.enchantgrade=1;check(identity(0)!=gear,"grade fingerprint");it.enchantgrade=0;
 it.unique_id=42;check(identity(0)==gear,"item serial does not alter combat setup");
 auto buff=identity(1);auto* sce=sd->sc.createSCE(SC_BLESSING);sce->val1=10;
 check(identity(1)!=buff,"active buff fingerprint");auto active=identity(1);auto timer_before=sce->timer;sce->timer=55;check(identity(1)==active,"timer allocation excluded");sce->timer=timer_before;sce->val1=9;check(identity(1)!=active,"buff potency fingerprint");sd->sc.deleteSCE(SC_BLESSING);
 check(identity(2).empty(),"missing startup attestation fails closed");
 exec_lab("PNLabBuild$[0]=\"\"; PNLabBuild$[1]=\"\"; LabCompat=callfunc(\"PN_LabCompatible\",0,1);");
 check(nums[add_str("LabCompat")]==0,"empty build IDs never compare");
 lab_detach();sd.reset();nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
 std::printf("LAB_HISTORY_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
