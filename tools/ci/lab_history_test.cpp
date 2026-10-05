#include "map/mob.hpp"
#include "map/skill.hpp"
static mob_data* lab_target=nullptr;
extern "C" block_list* lab_lookup(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* lab_lookup(int32 id){
 if(attached&&attached->id==id)return attached;
 return lab_target&&lab_target->id==id?lab_target:nullptr;
}
// Only outgoing view refresh is doubled; target configuration/status are real.
extern "C" void lab_refresh(const block_list*,bool) asm("__wrap__Z12unit_refreshPK10block_listb");
extern "C" void lab_refresh(const block_list*,bool){}
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
static void comparison_cases(){
 auto reset=[](){exec_lab("deletearray PNLabValid[0],3; PNLabNext=0;");};
 auto save=[](const char* label,int dps,const char* target="normal target",const char* gear="gear",const char* buff="buff",const char* build="build",int variable=0){
  exec_lab("callfunc \"PN_LabSave\",\""+std::string(label)+"\","+std::to_string(dps*60)+",60000,\""+target+"\",\""+gear+"\",\""+buff+"\",\""+build+"\","+std::to_string(variable)+";");
 };
 auto persistent_numbers=[](){std::map<int64,int64> result;for(const auto& entry:nums)if(entry.second&&std::string(get_str(script_getvarid(entry.first))).rfind("PNLab",0)==0)result.insert(entry);return result;};
 auto persistent_strings=[](){std::map<int64,std::string> result;for(const auto& entry:strings)if(!entry.second.empty()&&std::string(get_str(script_getvarid(entry.first))).rfind("PNLab",0)==0)result.insert(entry);return result;};
 auto history=[&](int choice=1){
  auto before_num=persistent_numbers();auto before_str=persistent_strings();messages.clear();exec_lab("callfunc \"PN_LabHistory\";",{choice});
  check(persistent_numbers()==before_num&&persistent_strings()==before_str,"comparison explanation never mutates saved history");
 };
 ++cases;reset();save("A",1000);save("B",2000,"boss target");
 check(nums[reference_uid(add_str("PNLabValid"),0)]==1&&nums[reference_uid(add_str("PNLabValid"),1)]==1,"both comparison records are valid completed saves");
 exec_lab("LabCompat=callfunc(\"PN_LabCompatible\",0,1);");check(nums[add_str("LabCompat")]==0,"target mismatch is actually incompatible");
 // Changing only the target produces a compatible repeat and visible output.
 exec_lab("PNLabSetup$[1]=\"normal target\";");history();
 check(said("Identical setup repeats: 2; median DPS 1500"),"same-target control reaches the actual comparison UI");
 exec_lab("PNLabSetup$[1]=\"boss target\";");history();
 check(said("Identical setup repeats: 1; median DPS 1000"),"incompatible target remains outside repeat statistics");
 check(said("Run 2 excluded: target settings differ."),"incompatible saved target has a visible exclusion reason");
 check(said("Comparison counts: 1 identical; 0 A/B; 1 excluded."),"target exclusion count is explicit");
 ++cases;reset();save("A",1000);save("B",3000);save("C",2000);history();
 check(said("Comparison counts: 3 identical; 0 A/B; 0 excluded.")&&said("median DPS 2000; min/max 1000 / 3000; spread 2000"),"three exact repeats preserve median and spread");
 ++cases;reset();save("A",1000);save("B",3000,"normal target","other gear");save("C",2000,"boss target");history();
 check(said("A/B candidate run 2")&&!said("A/B candidate run 3")&&said("Comparison counts: 1 identical; 1 A/B; 1 excluded."),"gear A/B stays separate from repeat statistics and incompatible targets");
 for(const char* change:{"PNLabBuff$[1]=\"other buff\";","PNLabBuild$[1]=\"other build\";","PNLabBuild$[1]=\"\";","PNLabVariable[1]=1;"}){
  ++cases;reset();save("A",1000);save("B",2000);exec_lab(change);history();
  const std::string mutation=change;
  const char* why=mutation.find("Buff$")!=std::string::npos?"initial buffs differ.":mutation.find("other build")!=std::string::npos?"server build differs.":mutation.find("Build$")!=std::string::npos?"verified server build is unavailable.":"buffs changed during this run.";
  check(said(("Run 2 excluded: "+std::string(why)).c_str()),"each existing incompatibility has its own visible reason");
  check(said("Comparison counts: 1 identical; 0 A/B; 1 excluded.")&&said("Identical setup repeats: 1; median DPS 1000"),"explanations preserve eligibility and statistics");
 }
 ++cases;exec_lab("PNLabVariable[0]=1;");history();
 check(said("selected run's buffs changed during the run.")&&said("Comparison counts: 0 identical; 0 A/B; 2 excluded.")&&said("No comparable stable-buff runs."),"unstable selected run explains why every saved run is excluded");
 ++cases;reset();save("A",1000);history();
 check(!said("Run 2 excluded:")&&!said("Run 3 excluded:")&&said("Comparison counts: 1 identical; 0 A/B; 0 excluded."),"empty slots are not counted as excluded histories");
 std::printf("LAB_COMPARISON_UI_OK target/buff/build reasons, counts, unchanged statistics and read-only history\n");
}
extern "C" int __wrap_main(int argc,char**argv){
 check(argc==3||argc==4,"source");const bool red=argc==4&&std::string(argv[3])=="red";const bool comparison=argc==4&&std::string(argv[3])=="comparison";deny_network();static char server[]="lab-history-audit";SERVER_NAME=server;
 malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
 const auto source=read(argv[1]);
 const auto challenges=read("npc/custom/main_office/lab_challenges.txt");
 strdb_put(script_get_userfunc_db(),"PN_LabBestSave",compile(body(challenges,"function\tscript\tPN_LabBestSave"),"PN_LabBestSave"));
 auto* console=compile(body(source,"\tscript\tPN Lab Console"),"complete lab console");script_free_code(console);
 for(auto name:{"PN_LabSave","PN_LabCompatible","PN_LabHistory"}) {
  strdb_put(script_get_userfunc_db(),name,compile(body(source,std::string("function\tscript\t")+name),name));
 }
 if(!red)for(auto name:{"PN_LabTargetValid","PN_LabPresetSave","PN_LabPresetLoad","PN_LabTargetIdentity","PN_LabApplyTarget"})
  strdb_put(script_get_userfunc_db(),name,compile(body(source,std::string("function\tscript\t")+name),name));
 auto sd=lab_player();
 if(comparison){
  comparison_cases();lab_detach();sd.reset();nums.clear();strings.clear();do_final_script();timer_final();db_final();malloc_final();
  std::printf("LAB_HISTORY_OK comparison-only cases=%u assertions=%u\n",cases,assertions);return 0;
 }
 {
  auto target=std::make_unique<mob_data>();lab_target=target.get();target->id=99000004;target->type=BL_MOB;
  target->db=std::make_shared<s_mob_db>();target->vd=&target->db->vd;target->ud.bl=target.get();
  target->ud.walktimer=target->ud.attacktimer=target->ud.skilltimer=target->ud.steptimer=INVALID_TIMER;
  target->db->status.hp=target->db->status.max_hp=2000000000;
  target->db->status.ele_lv=1;target->status=target->db->status;
  if(red){
   auto begin=source.find("setunitdata .@mob,UMOB_MODE,");check(begin!=std::string::npos,"exact production target mode");
   exec_lab(".@mob=99000004; "+source.substr(begin,source.find(';',begin)-begin+1));
   std::printf("LAB_TARGET actual_class=%d expected_normal=%d\n",target->status.class_,CLASS_NORMAL);
   check(target->status.class_==CLASS_NORMAL,"normal-class lab target must activate normal-only combat bonuses");
  }else{
   ++cases;
   exec_lab("setarray @PNLabTarget[0],0,1,0,0,1,0,0,0,0; LabApplied=callfunc(\"PN_LabApplyTarget\",99000004);");
   check(nums[add_str("LabApplied")]==1&&target->status.class_==CLASS_NORMAL,"normal target actual class after every production setting");
   check(!status_has_mode(&target->status,MD_STATUSIMMUNE)&&status_has_mode(&target->status,MD_KNOCKBACKIMMUNE),"normal target permits effects and resists knockback");
   check(!status_has_mode(&target->status,MD_CANATTACK)&&!status_has_mode(&target->status,MD_CANMOVE),"target neither attacks nor moves");
   map_num=1;std::strcpy(map[0].name,"lab_test");map[0].initMapFlags();
   std::vector<mapcell> cells(64);for(auto& c:cells){c.walkable=1;c.shootable=1;}
   map[0].xs=map[0].ys=8;map[0].cell=cells.data();
   target->m=0;target->x=4;target->y=4;target->damagetaken=100;
   sd->m=0;sd->x=3;sd->y=4;sd->status.base_level=1;sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;
   sd->battle_status.hp=sd->battle_status.max_hp=10000;sd->battle_status.batk=10000;
   sd->battle_status.rhw.atk=sd->battle_status.rhw.atk2=10000;sd->battle_status.watk=10000;
   sd->battle_status.rhw.ele=ELE_NEUTRAL;sd->bonus.perfect_hit=100;
   for(auto& rate:sd->right_weapon.atkmods)rate=100;
   auto item_source=read(argv[2]);auto tree=ryml::parse_in_arena(ryml::to_csubstr(item_source));
   for(auto row:tree["Body"])check(item_db.parseBodyNode(row)>0,"real required-item identity parses");
   check(skill_db.load(),"actual skill metadata loads");check(status_db.load(),"actual status metadata loads");
   check(elemental_attribute_db.load(),"actual element table loads");
   auto damage=[&](){generator.seed(42);auto hit=battle_calc_attack(BF_WEAPON,sd.get(),target.get(),0,0,0);check(hit.damage>0&&hit.damage2==0,"real physical hit succeeds");return hit.damage;};
   auto normal=damage();sd->right_weapon.addclass[CLASS_BOSS]=100;check(damage()==normal,"boss-only bonus excluded on normal target");
   exec_lab("@PNLabTarget[0]=1; callfunc \"PN_LabApplyTarget\",99000004;");
   check(target->status.class_==CLASS_BOSS&&status_has_mode(&target->status,MD_STATUSIMMUNE),"boss actual class and immunity after all settings");
   auto bonus=damage();sd->right_weapon.addclass[CLASS_BOSS]=0;auto boss=damage();
   std::printf("LAB_CLASS_DAMAGE normal=%lld boss=%lld bonus=%lld\n",(long long)normal,(long long)boss,(long long)bonus);
   check(bonus>boss&&boss==normal,"boss bonus increases actual damage only on boss-class target");
   exec_lab("@PNLabTarget[5]=400; callfunc \"PN_LabApplyTarget\",99000004;");auto def=damage();
   check(target->status.def==400&&def<boss,"configured DEF reduces actual physical damage");
   exec_lab("@PNLabTarget[5]=0; @PNLabTarget[6]=400; callfunc \"PN_LabApplyTarget\",99000004;");auto res=damage();
   check(target->status.res==400&&res<boss,"configured RES reduces actual physical damage");
   std::printf("LAB_DAMAGE normal=%lld boss=%lld boss_bonus=%lld def400=%lld res400=%lld\n",(long long)normal,(long long)boss,(long long)bonus,(long long)def,(long long)res);
   ++cases;
   exec_lab("setarray @PNLabTarget[0],1,2,9,8,4,321,456,789,1000; LabSaved=callfunc(\"PN_LabPresetSave\",2,\"Boss preset\"); LabTarget$=callfunc(\"PN_LabTargetIdentity\");");
   check(nums[add_str("LabSaved")]==1,"complete named preset saved");auto saved=strings[add_str("LabTarget$")];
   check(saved.find("boss/status-immune")!=std::string::npos&&saved.find("def=321 res=456 mdef=789 mres=1000")!=std::string::npos,"identity includes class and all defenses");
   // Simulated relog removes every temporary registry and reconstructs player.
   lab_detach();sd.reset();
   for(auto it=nums.begin();it!=nums.end();)if(get_str(script_getvarid(it->first))[0]=='@')it=nums.erase(it);else ++it;
   for(auto it=strings.begin();it!=strings.end();)if(get_str(script_getvarid(it->first))[0]=='@')it=strings.erase(it);else ++it;
   sd=lab_player();exec_lab("LabLoaded=callfunc(\"PN_LabPresetLoad\",2); LabTarget$=callfunc(\"PN_LabTargetIdentity\");");
   check(nums[add_str("LabLoaded")]==1&&strings[add_str("LabTarget$")]==saved,"all nine settings survive character-registry relog boundary");
   for(int i=0;i<9;++i){
    exec_lab("callfunc \"PN_LabPresetLoad\",2; @PNLabTarget["+std::to_string(i)+"]=0; LabTarget$=callfunc(\"PN_LabTargetIdentity\");");
    check(strings[add_str("LabTarget$")]!=saved,"each setting changes comparison identity or invalidates setup");
   }
   exec_lab("PNLabPreset[26]=1001; LabLoaded=callfunc(\"PN_LabPresetLoad\",2); LabApplied=callfunc(\"PN_LabApplyTarget\",99000004);");
   check(nums[add_str("LabLoaded")]==0&&nums[add_str("LabApplied")]==0&&target->status.res==400,"corrupt preset rejected before target mutation");
   auto before_num=nums;exec_lab("callfunc \"PN_LabPresetSave\",3,\"invalid\";");check(nums==before_num,"invalid slot changes no registry state");
   exec_lab("PNLabPresetVersion[2]=0; LabLoaded=callfunc(\"PN_LabPresetLoad\",2);");check(nums[add_str("LabLoaded")]==0,"incomplete preset cannot load");
   map[0].cell=nullptr;elemental_attribute_db.clear();
  }
  aFree(target->base_status);target->base_status=nullptr;lab_target=nullptr;
 }
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
 comparison_cases();
 lab_detach();sd.reset();nums.clear();strings.clear();status_db.clear();skill_db.clear();item_db.clear();do_final_script();timer_final();db_final();malloc_final();
 std::printf("LAB_HISTORY_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
