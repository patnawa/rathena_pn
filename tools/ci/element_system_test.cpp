#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <map>
#include <memory>
using int16=int16_t; using int32=int32_t; using int64=int64_t; using uint16=uint16_t;
#define RENEWAL
enum { ELE_NONE=-1,ELE_NEUTRAL,ELE_WATER,ELE_EARTH,ELE_FIRE,ELE_WIND,ELE_POISON,ELE_HOLY,ELE_DARK,ELE_GHOST,ELE_UNDEAD,ELE_ALL,ELE_MAX };
#define CHK_ELEMENT(e) ((e)>ELE_NONE && (e)<ELE_MAX)
#define CHK_ELEMENT_LEVEL(l) ((l)>=1 && (l)<=4)
// STATUS_ENUM
enum { BL_PC=1, BL_SKILL=16, GN_WALLOFTHORN=1000, GN_CARTCANNON, UNT_FIREWALL, BF_MAGIC=2, CLASS_BOSS=1 };
struct status_change_entry { int val1=0,val2=0,val3=0; };
struct status_change {
    std::map<int,status_change_entry> data;
    bool empty() const { return data.empty(); }
    status_change_entry* getSCE(int id) { auto it=data.find(id); return it==data.end()?nullptr:&it->second; }
};
struct block_list { int type=BL_PC, cls=0, skill=0; status_change sc; };
struct s_skill_unit_group { int val3=0,src_id=0,unit_id=0,skill_id=0,skill_lv=0,limit=0,tick=0; };
struct skill_unit : block_list { bool alive=true; std::shared_ptr<s_skill_unit_group> group; };
status_change* status_get_sc(block_list* b) { return &b->sc; }
int rnd() { return 3; }
int errors=0;
template<class... T> void ShowError(const char*,T...) { ++errors; }
int battle_getcurrentskill(block_list* b) { return b ? b->skill : 0; }
block_list* map_id2bl(int) { return nullptr; }
bool status_isdead(block_list&) { return false; }
void skill_unitsetting(block_list*,int,int,int,int,int) { assert(false); }
int gettick() { return 0; }
#define DIFF_TICK(a,b) ((a)-(b))
void status_change_end(block_list* b,int id) { b->sc.data.erase(id); }
int skill_get_type(int) { return BF_MAGIC; }
int status_get_class_(block_list* b) { return b->cls; }
int cap_value(int v,int lo,int hi) { return std::clamp(v,lo,hi); }
struct { int attr_recover=0; } battle_config;
struct AttributeDatabase {
    // TABLE_DATA
    int16 getAttribute(uint16,uint16,uint16);
} elemental_attribute_db;
// PRODUCTION_FUNCTIONS
int main() {
    int cases=0;
    for(int l=1;l<=4;++l) for(int a=0;a<10;++a) for(int d=0;d<10;++d) {
        const int r=elemental_attribute_db.getAttribute(l,a,d);
        for(int64 n : {0LL,1LL,3LL,99LL,100LL,101LL,1000000000LL}) {
            // Independent integer expectation: renewal rounds damage reductions down,
            // but increases up to a whole damage point are truncated.
            const int64 expected = r<=100 ? (n*std::max(r,0)+99)/100 : n*r/100;
            assert(battle_attr_fix(nullptr,nullptr,n,a,d,l,0)==expected);
            ++cases;
        }
    }
    assert(elemental_attribute_db.getAttribute(0,0,0)==100);
    assert(elemental_attribute_db.getAttribute(5,0,0)==100);
    assert(elemental_attribute_db.getAttribute(1,99,0)==100);
    assert(battle_attr_fix(nullptr,nullptr,123,0,99,1,0)==123 && errors==1);
    assert(battle_attr_fix(nullptr,nullptr,100,-1,2,1,0)==150); // RNG fire fallback.
    elemental_attribute_db.attr_fix_table[0][0][0]=-50;
    assert(battle_attr_fix(nullptr,nullptr,100,0,0,1,0)==0);
    assert(battle_attr_fix(nullptr,nullptr,100,0,0,1,1)==-50);
    battle_config.attr_recover=1;
    assert(battle_attr_fix(nullptr,nullptr,100,0,0,1,0)==-50);
    block_list src,target;
    src.sc.data[SC_VOLCANO].val3=20;
    target.sc.data[SC_SPIDERWEB]={};
    assert(battle_attr_fix(&src,&target,100,3,2,1,0)==270);
    assert(!target.sc.getSCE(SC_SPIDERWEB));
    assert(battle_attr_fix(&src,&target,100,3,2,1,0)==170);
    src.sc.data.clear(); target.sc.data[SC_ORATIO].val1=5;
    assert(battle_attr_fix(&src,&target,100,6,9,4,0)==210);
    target.sc.data.clear(); src.sc.data[SC_TELEKINESIS_INTENSE].val3=100;
    assert(battle_attr_fix(&src,&target,3,8,0,1,0)==6);
    target.sc.data[SC_FREEZE]={};
    assert(status_calc_element(&target,&target.sc,ELE_FIRE)==ELE_WATER);
    assert(status_calc_element_lv(&target,&target.sc,4)==1);
    target.sc.data.clear(); target.sc.data[SC_STONE]={};
    assert(status_calc_element(&target,&target.sc,ELE_WATER)==ELE_EARTH);
    assert(status_calc_element_lv(&target,&target.sc,4)==1);
    target.sc.data.clear(); target.sc.data[SC_ELEMENTALCHANGE]={3,ELE_DARK,0};
    assert(status_calc_element(&target,&target.sc,0)==ELE_DARK);
    assert(status_calc_element_lv(&target,&target.sc,1)==3);
    assert(status_calc_element_lv(&target,nullptr,0)==1);
    assert(status_calc_element_lv(&target,nullptr,9)==4);
    std::cout << "PASS: 400 effective element cells / " << cases << " damage cases; recovery, status bonuses, consumption and defense-level transitions\n";
}
