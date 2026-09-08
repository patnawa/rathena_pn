// Report-boundary doubles only. Production pn_battlestats is inserted unchanged.
#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>
#define RENEWAL_CAST
#define CHAT_SIZE_MAX 256
#define AMOTION_ZERO_ASPD 2000
#define AMOTION_INTERVAL 10
#define safesnprintf snprintf
enum {RC_ALL=12,RC_MAX=13,SZ_ALL=3,SZ_MAX=4,ELE_ALL=10,ELE_MAX=11,CLASS_NORMAL=0,CLASS_ALL=6,CLASS_MAX=7,RC2_NONE=0,RC2_MAX=16};
enum {SC_ITEMBOOST,SC_PERIOD_RECEIVEITEM_2ND};
struct stats {
    unsigned base_level=275,job_level=1,sex=0;
    int str=0,agi=0,vit=0,int_=0,dex=0,luk=0,pow=0,sta=0,wis=0,spl=0,con=0,crt=0;
};
struct weapon {unsigned atk=0,atk2=0,range=1,ele=0;};
struct status_data:stats {
    unsigned hp=100,max_hp=100,sp=50,max_sp=50,ap=0,max_ap=200,def_ele=0,ele_lv=1,matk_min=0,matk_max=0,speed=150,amotion=100;
    int def=0,def2=0,mdef=0,mdef2=0,res=0,mres=0,flee=0,flee2=0,batk=0,eatk=0,hit=0,cri=0,patk=0,smatk=0,hplus=0,crate=0;
    weapon rhw,lhw;
};
struct indexed {
    int subrace[RC_MAX]{},magic_addrace[RC_MAX]{},arrow_addrace[RC_MAX]{},subsize[SZ_MAX]{},weapon_subsize[SZ_MAX]{},magic_subsize[SZ_MAX]{},magic_addsize[SZ_MAX]{},arrow_addsize[SZ_MAX]{};
    int subele[ELE_MAX]{},subele_script[ELE_MAX]{},subdefele[ELE_MAX]{},magic_subdefele[ELE_MAX]{},magic_addele[ELE_MAX]{},magic_addele_script[ELE_MAX]{},magic_atk_ele[ELE_MAX]{};
    int subclass[CLASS_MAX]{},magic_addclass[CLASS_MAX]{},subrace2[RC2_MAX]{},magic_addrace2[RC2_MAX]{},dropaddrace[RC_MAX]{},dropaddclass[CLASS_MAX]{};
};
struct weapon_bonus {int addrace[RC_MAX]{},addsize[SZ_MAX]{},addele[ELE_MAX]{},addclass[CLASS_MAX]{},addrace2[RC2_MAX]{};};
struct bonuses {
    int near_attack_def_rate=0,long_attack_def_rate=0,magic_def_rate=0,misc_def_rate=0,crit_def_rate=0,short_attack_atk_rate=0,long_attack_atk_rate=0,crit_atk_rate=0,atk_rate=0;
    int varcastrate=0,add_varcast=0,fixcastrate=0,add_fixcast=0,delayrate=0;
};
struct skill_bonus {unsigned id;int val;};
struct sce {int val1;};
struct statuses {std::map<int,sce> rows;sce* getSCE(int i){auto it=rows.find(i);return it==rows.end()?nullptr:&it->second;}};
struct map_session_data {
    stats status;status_data battle_status;indexed indexed_bonus;weapon_bonus right_weapon,left_weapon;bonuses bonus;statuses sc;
    int class_=0,matk_rate=100,dsprate=100,castrate=100;
    std::vector<skill_bonus> skillatk,subskill,skillcastrate,skillfixcast;
};
struct jobinfo {std::vector<std::array<int,12>> job_bonus=std::vector<std::array<int,12>>(1);};
struct jobs {std::shared_ptr<jobinfo> value=std::make_shared<jobinfo>();std::shared_ptr<jobinfo> find(int){return value;}} job_db;
struct {int vcast_stat_scale=530;} battle_config;
int pc_mapid2jobid(int job,unsigned){return job;}
const char* script_get_constant_str(const char*,int){return "RC2_TEST";}
const char* skill_get_desc(unsigned){return "Test Skill";}
std::vector<std::string> output;
void clif_displaymessage(int,const char* value){output.emplace_back(value);}
// PRODUCTION_HELPER
bool contains(const std::string& text){return std::any_of(output.begin(),output.end(),[&](const auto& s){return s.find(text)!=std::string::npos;});}
int report(map_session_data& sd,const char* input,bool defense=false){output.clear();return pn_battlestats(1,sd,input,defense);}
int main(){
    map_session_data sd;
    assert(report(sd,"")==0 && contains("Offense: summary") && output.size()<=14);
    assert(report(sd,"summary",true)==0 && contains("Defense: summary"));
    assert(report(sd,"race")==0 && contains("No nonzero modifiers"));
    sd.right_weapon.addrace[0]=5;sd.right_weapon.addrace[RC_ALL]=10;
    sd.left_weapon.addrace[0]=-3;sd.indexed_bonus.magic_addrace[0]=7;sd.indexed_bonus.arrow_addrace[0]=2;
    assert(report(sd,"race")==0 && contains("Formless R +15% / L -3% / M +7% / ammo +2%"));
    sd.indexed_bonus.subele[0]=-15;sd.indexed_bonus.subele[ELE_ALL]=2;sd.indexed_bonus.subele_script[0]=3;
    sd.indexed_bonus.subdefele[0]=4;sd.indexed_bonus.magic_subdefele[0]=6;
    assert(report(sd,"element",true)==0 && contains("Neutral reduction: incoming attack -10% / enemy armor P +4% M +6%"));
    sd.status.str=40;sd.battle_status.str=54;job_db.value->job_bonus[0][0]=13;
    assert(report(sd,"stats")==0 && contains("STR 54 = base 40 + job 13 + other +1"));
    sd.status.job_level=99;
    assert(report(sd,"stats")==0 && contains("STR 54 = base 40 + job 0 + other +14"));
    sd.battle_status.dex=265;sd.bonus.varcastrate=26;sd.bonus.fixcastrate=-50;sd.bonus.add_fixcast=-500;
    assert(report(sd,"casting")==0 && contains("VCT stat reduction 100.00%") && contains("FCT script reduction +50%, flat adjustment -500 ms"));
    assert(contains("FCT has no general DEX/INT reduction"));
    for(unsigned i=1;i<=25;++i)sd.skillatk.push_back({i,(int)i});
    assert(report(sd,"skills 2")==0 && output.size()==14 && contains("(13):") && contains("(24):") && !contains("(25):"));
    assert(report(sd,"skills 3")==0 && output.size()==3 && contains("(25):"));
    for(const char* invalid:{"skills 4","skills 0","skills -1","skills 2 x","skills\tbogus","skills 999999999999999999999","unknown"})assert(report(sd,invalid)==-1);
    assert(report(sd,"  summary  ")==0);
    sd.indexed_bonus.dropaddrace[RC_ALL]=10;sd.indexed_bonus.dropaddclass[CLASS_ALL]=20;sd.sc.rows[SC_ITEMBOOST]={50};
    assert(report(sd,"drops")==0 && contains("race-all +10% + class-all +20% + item buffs +50%") && contains("no universal MVP-card chance"));
    assert(sd.status.str==40 && sd.bonus.varcastrate==26 && sd.skillatk.size()==25 && sd.sc.rows[SC_ITEMBOOST].val1==50);
    std::cout<<"BATTLESTATS_REPORT_OK: real formatter, arithmetic, negative modifiers, page bounds, read-only state\n";
}
