// Explicit boundaries around production autocast and Elemental Buster dispatch.
// The Python driver inserts functions verbatim; no alternate dispatch algorithm.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using uint8 = uint8_t; using uint16 = uint16_t; using int16 = int16_t;
using int32 = int32_t; using t_tick = int64_t; using t_itemid = uint32_t;
enum { BL_PC=1, BL_CHAR=7, BL_SKILL=8, INF_GROUND_SKILL=1, INF_SUPPORT_SKILL=2,
       INF_SELF_SKILL=4, INF_ATTACK_SKILL=8, INF2_NOTARGETSELF=0, NK_NODAMAGE=0,
       AUTOSPELL_FORCE_TARGET=1, AUTOSPELL_FORCE_SELF=0, AUTOSPELL_FORCE_RANDOM_LEVEL=2,
       MAX_PC_BONUS=50, SKILL_NOCONSUME_REQ=0x100, BCT_ENEMY=1, SD_LEVEL=2, SD_SPLASH=4,
       AREA_SIZE=14, CELL_CHKWALL=0, ELEMENTALID_DILUVIO=20816, ELEMENTALID_ARDOR,
       ELEMENTALID_PROCELLA, ELEMENTALID_TERREMOTUS, ELEMENTALID_SERPENS };
// SKILL_CONSTANTS
enum { WZ_METEOR=30001, WZ_ICEWALL, MO_BODYRELOCATION, CR_CULTIVATION, HW_GANBANTEIN,
       SC_ESCAPE, SU_CN_METEOR, NPC_RAINOFMETEOR, HN_METEOR_STORM_BUSTER,
       NW_GRENADES_DROPPING, NPC_SEEDTRAP };
enum e_cast_type { CAST_GROUND, CAST_NODAMAGE, CAST_DAMAGE };
enum sc_type { SC_NONE, SC_CURSEDCIRCLE_ATKER };
struct status_change_entry {};
struct status_change { status_change_entry* getSCE(sc_type) { return nullptr; } };
struct block_list { int32 id=1, type=BL_PC, m=0, x=10, y=10; };
struct elemental_data { struct { int32 class_=ELEMENTALID_DILUVIO; } elemental; };
struct s_autospell { uint16 id,lv,trigger_skill; int16 rate,battle_flag; t_itemid card_id; uint8 flag; bool lock; };
struct s_autobonus { int rate=0, atk_type=0; };
struct pet_data { std::vector<std::shared_ptr<s_autobonus>> autobonus3; };
struct map_session_data: block_list {
    struct { bool autocast=false, arrow_atk=false; } state;
    std::vector<s_autospell> autospell3;
    std::vector<std::shared_ptr<s_autobonus>> autobonus3;
    pet_data* pd=nullptr;
    elemental_data* ed=nullptr;
    t_tick canskill_tick=0;
};
struct s_skill_unit_group {};
class SkillImpl {
    uint16 id;
public:
    explicit SkillImpl(uint16 value):id(value){}
    virtual ~SkillImpl()=default;
    uint16 getSkillId() const {return id;}
    virtual void castendPos2(block_list*,int32,int32,uint16,t_tick,int32&) const {}
    virtual void castendNoDamageId(block_list*,block_list*,uint16,t_tick,int32&) const {}
};
class SkillElementalBuster:public SkillImpl {
public: SkillElementalBuster():SkillImpl(EM_ELEMENTAL_BUSTER){}
    void castendNoDamageId(block_list*,block_list*,uint16,t_tick,int32&) const override;
};
class SkillDiamondStorm:public SkillImpl {
public: SkillDiamondStorm():SkillImpl(EM_DIAMOND_STORM){}
    void castendPos2(block_list*,int32,int32,uint16,t_tick,int32&) const override;
};
class SkillTerraDrive:public SkillImpl {
public: SkillTerraDrive():SkillImpl(EM_TERRA_DRIVE){}
    void castendPos2(block_list*,int32,int32,uint16,t_tick,int32&) const override;
};
struct s_skill_db {
    int32 inf=0; std::array<bool,1> inf2{},nk{}; std::shared_ptr<SkillImpl> impl;
};
struct SkillDB {
    std::map<uint16,std::shared_ptr<s_skill_db>> rows;
    std::shared_ptr<s_skill_db> find(uint16 id) { auto it=rows.find(id);return it==rows.end()?nullptr:it->second; }
} skill_db;
struct { bool autospell_check_range=false; } battle_config;
#define BL_CAST(kind,ptr) ((ptr)&&((ptr)->type==(kind))?static_cast<map_session_data*>(ptr):nullptr)
#define nullpo_ret(ptr) if(!(ptr))return 0
#define nullpo_retr(value,ptr) if(!(ptr))return value
template<typename T,typename L,typename H> T cap_value(T v,L lo,H hi) {return std::clamp(v,T(lo),T(hi));}
template<typename T> T rnd_value(T lo,T) {return lo;}
int32 rnd() {return 0;} // Fixed roll; rate=1000 effects must always activate.
int assertions=0, failures=0, consumed=0, rejected=0, area_calls=0;
std::vector<uint16> placed, cast, animations;
uint16 area_skill=0, checked_skill=0, reject_ground_skill=0;
block_list* range_origin=nullptr;
std::function<void(map_session_data*,uint16)> on_cast;
void check(bool ok,const std::string& message) {
    ++assertions;
    if(!ok){++failures;std::cerr<<"DIMENSION_AUTOCAST_FAIL: "<<message<<'\n';}
}
void ShowWarning(const char* fmt,...) {std::cerr<<"UNEXPECTED WARNING: "<<fmt;std::abort();}
bool skill_isNotOk(uint16,map_session_data&) {return false;}
bool skill_pos_maxcount_check(block_list*,int32,int32,uint16 id,uint16,int,bool) {
    checked_skill=id;return id!=reject_ground_skill;
}
int32 skill_get_range2(const block_list*,uint16,uint16,bool) {return 5;}
void skill_consume_requirement(map_session_data*,uint16,uint16,int kind) {if(kind==1)++consumed;}
void pc_exeautobonus(map_session_data&,void*,std::shared_ptr<s_autobonus>) {std::abort();}
void pet_exeautobonus(map_session_data&,void*,std::shared_ptr<s_autobonus>) {std::abort();}
bool status_isdead(const block_list&) {return false;}
status_change* status_get_sc(block_list*) {return nullptr;}
sc_type skill_get_sc(uint16) {return SC_NONE;}
int32 skill_get_inf(uint16 id) {return skill_db.find(id)->inf;}
void status_change_end(block_list*,sc_type) {}
t_tick gettick() {return 5000;}
void battle_consume_ammo(map_session_data*,uint16,uint16) {std::abort();}
int32 clif_skill_nodamage(block_list*,block_list&,uint16 id,uint16) {animations.push_back(id);return 0;}
void clif_skill_poseffect(block_list&,uint16,uint16,int32,int32,t_tick) {}
void clif_skill_fail(map_session_data&,uint16) {++rejected;}
std::shared_ptr<s_skill_unit_group> skill_unitsetting(block_list*,uint16 id,uint16,int32,int32,int32) {
    placed.push_back(id);return nullptr;
}
int32 skill_area_temp[8]{};
int32 skill_area_sub(block_list*,va_list) {return 0;}
int32 skill_castend_damage_id(block_list*,block_list*,uint16,uint16,t_tick,int32);
int32 map_foreachinrange(int32(*)(block_list*,va_list),block_list*,int32,int32,
                        block_list*,int skill,uint16,t_tick,int32,
                        int32(*)(block_list*,block_list*,uint16,uint16,t_tick,int32)) {
    ++area_calls;area_skill=skill;return 1;
}
int32 distance_bl(const block_list* a,const block_list* b) {return std::max(std::abs(a->x-b->x),std::abs(a->y-b->y));}
bool check_distance_bl(const block_list* a,const block_list* b,int32 range) {return distance_bl(a,b)<=range;}
bool check_distance_client_bl(const block_list* a,const block_list* b,int32 range) {
    range_origin=const_cast<block_list*>(a);return check_distance_bl(a,b,range);
}
bool path_search_long(void*,int32,int32,int32,int32,int32,int32) {return true;}
bool battle_check_range(const block_list*,const block_list*,int32);
int32 skill_onskillusage(map_session_data*,block_list*,uint16,t_tick);
int32 skill_castend_pos2(block_list*,int32,int32,uint16,uint16,t_tick,int32);
int32 skill_castend_nodamage_id(block_list* src,block_list* target,uint16 id,uint16 lv,t_tick tick,int32 flag) {
    cast.push_back(id);
    auto skill=skill_db.find(id);
    if(skill && skill->impl)skill->impl->castendNoDamageId(src,target,lv,tick,flag);
    if(on_cast)on_cast(static_cast<map_session_data*>(src),id);
    return 0;
}
int32 skill_castend_damage_id(block_list* src,block_list*,uint16 id,uint16,t_tick,int32) {
    cast.push_back(id);if(on_cast)on_cast(static_cast<map_session_data*>(src),id);return 0;
}
// PRODUCTION_FUNCTIONS
void add_skill(uint16 id,int inf,bool nodamage,bool no_target_self) {
    auto row=std::make_shared<s_skill_db>();row->inf=inf;row->nk[0]=nodamage;row->inf2[0]=no_target_self;
    row->impl=std::make_shared<SkillImpl>(id);skill_db.rows[id]=row;
}
void reset() {
    consumed=rejected=area_calls=0;placed.clear();cast.clear();animations.clear();
    checked_skill=reject_ground_skill=area_skill=0;range_origin=nullptr;on_cast={};
    battle_config.autospell_check_range=false;
}
void register_effect(map_session_data& sd,uint16 trigger,uint16 skill,uint16 level=5,uint16 rate=1000) {
    auto db=skill_db.find(skill);
    // pc_bonus4's automatic target choice is a boundary here; the production
    // vector registration and all subsequent dispatch functions are exercised.
    const bool self=(db->inf&INF_SUPPORT_SKILL)||((db->inf&INF_SELF_SKILL)&&!db->inf2[INF2_NOTARGETSELF]);
    pc_bonus_autospell_onskill(sd.autospell3,trigger,skill,level,rate,0,self?AUTOSPELL_FORCE_TARGET:AUTOSPELL_FORCE_SELF);
}
void em_cases(bool range) {
    const std::pair<int,uint16> summons[]={{ELEMENTALID_DILUVIO,EM_ELEMENTAL_BUSTER_WATER},
        {ELEMENTALID_ARDOR,EM_ELEMENTAL_BUSTER_FIRE},{ELEMENTALID_PROCELLA,EM_ELEMENTAL_BUSTER_WIND},
        {ELEMENTALID_TERREMOTUS,EM_ELEMENTAL_BUSTER_GROUND},{ELEMENTALID_SERPENS,EM_ELEMENTAL_BUSTER_POISON}};
    for(uint16 trigger:{uint16(EM_DIAMOND_STORM),uint16(EM_TERRA_DRIVE)}) {
        for(const auto& summon:summons) {
            reset();map_session_data sd;elemental_data ed;ed.elemental.class_=summon.first;sd.ed=&ed;
            register_effect(sd,trigger,EM_ELEMENTAL_BUSTER);battle_config.autospell_check_range=range;
            skill_castend_pos2(&sd,12,12,trigger,5,5000,0);
            check(placed==std::vector<uint16>{trigger},"ground trigger places its production skill unit");
            check(area_calls==1 && area_skill==summon.second,"EM ground trigger casts current summon Elemental Buster (range="+std::to_string(range)+")");
            check(consumed==1,"one autocast requirement debit per dispatch");
            check(!sd.state.autocast && !sd.autospell3[0].lock,"autocast state and recursion lock released");
        }
        for(int invalid:{0,20813,20821}) {
            reset();map_session_data sd;elemental_data ed;ed.elemental.class_=invalid;if(invalid)sd.ed=&ed;
            register_effect(sd,trigger,EM_ELEMENTAL_BUSTER);
            skill_castend_pos2(&sd,12,12,trigger,5,5000,0);
            check(area_calls==0 && rejected==0 && consumed==0 && cast.empty(),"missing or non-advanced summon silently skips Buster without resource debit");
        }
    }
}
void ground_cases() {
    for(bool target_full:{false,true}) {
        reset();map_session_data sd;block_list target;target.type=2;target.x=12;
        register_effect(sd,SOA_TALISMAN_OF_WHITE_TIGER,SOA_TALISMAN_OF_BLACK_TORTOISE);
        check(skill_get_casttype(SOA_TALISMAN_OF_BLACK_TORTOISE)==CAST_GROUND,"Soul Ascetic follow-up is a ground skill");
        reject_ground_skill=target_full?SOA_TALISMAN_OF_BLACK_TORTOISE:SOA_TALISMAN_OF_WHITE_TIGER;
        skill_onskillusage(&sd,&target,SOA_TALISMAN_OF_WHITE_TIGER,5000);
        check(checked_skill==SOA_TALISMAN_OF_BLACK_TORTOISE,"ground maxcount evaluates autocast skill instead of trigger skill");
        check(consumed==(target_full?0:1),"ground maxcount blocks only when the autocast skill is full");
    }
}
void chain_cases() {
    for(const auto& chain:std::vector<std::array<uint16,3>>{
        {IQ_FIRST_BRAND,IQ_SECOND_FLAME,IQ_THIRD_FLAME_BOMB},
        {GC_PHANTOMMENACE,GC_ROLLINGCUTTER,SHC_IMPACT_CRATER}}) {
        reset();map_session_data sd;block_list target;target.type=2;
        register_effect(sd,chain[0],chain[1]);register_effect(sd,chain[1],chain[2]);
        on_cast=[&target](map_session_data* player,uint16 id){skill_onskillusage(player,&target,id,5000);};
        skill_onskillusage(&sd,&target,chain[0],5000);
        check(cast==std::vector<uint16>{chain[1],chain[2]},"Inquisitor/Shadow Cross secondary on-skill dispatch stays enabled");
        check(consumed==2 && !sd.state.autocast,"chained follow-ups each consume once and release autocast state");
    }
    reset();map_session_data sd;block_list target;target.type=2;
    register_effect(sd,ABC_DEFT_STAB,ABC_ABYSS_DAGGER);
    register_effect(sd,ABC_ABYSS_DAGGER,ABC_DEFT_STAB);
    on_cast=[&target](map_session_data* player,uint16 id){
        check(player->state.autocast,"nested dispatch executes with autocast context");
        check(cast.size()<=2,"cyclic on-skill bonuses stop after each entry executes once");
        if(cast.size()<=2)skill_onskillusage(player,&target,id,5000);
    };
    skill_onskillusage(&sd,&target,ABC_DEFT_STAB,5000);
    check(cast==std::vector<uint16>{ABC_ABYSS_DAGGER,ABC_DEFT_STAB},"chained skills activate once and cannot recurse indefinitely");
    check(!sd.state.autocast && !sd.autospell3[0].lock && !sd.autospell3[1].lock,"all cycle locks clear");
    reset();sd.autospell3.clear();register_effect(sd,ABC_DEFT_STAB,ABC_ABYSS_DAGGER,5,0);
    skill_onskillusage(&sd,&target,ABC_DEFT_STAB,5000);
    check(cast.empty(),"zero chance never registers an effect");
    reset();sd.autospell3.clear();register_effect(sd,NW_SPIRAL_SHOOTING,NW_WILD_FIRE);
    battle_config.autospell_check_range=true;target.x=50;
    skill_onskillusage(&sd,&target,NW_SPIRAL_SHOOTING,5000);
    check(consumed==0,"on-skill range measures caster to enemy, rejecting a distant target");
}
int main(int argc,char** argv) {
    // DATABASE_INIT
    skill_db.find(EM_DIAMOND_STORM)->impl=std::make_shared<SkillDiamondStorm>();
    skill_db.find(EM_TERRA_DRIVE)->impl=std::make_shared<SkillTerraDrive>();
    skill_db.find(EM_ELEMENTAL_BUSTER)->impl=std::make_shared<SkillElementalBuster>();
    std::string selected=argc>1?argv[1]:"all";
    if(selected=="all"||selected=="em")em_cases(false);
    if(selected=="all"||selected=="range")em_cases(true);
    if(selected=="all"||selected=="ground")ground_cases();
    if(selected=="all"||selected=="chain")chain_cases();
    std::cout<<"DIMENSION_AUTOCAST_RUNTIME: case="<<selected<<" assertions="<<assertions<<" failures="<<failures<<'\n';
    return failures?1:0;
}
