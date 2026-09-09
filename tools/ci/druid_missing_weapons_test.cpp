// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  druid_missing_weapons_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_missing_weapons_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to native_script_vm_test.cpp's world-boundary
// doubles. Actual current itemdb/script/pc/skill/clif are freshly compiled.
#include <map>
#include "map/skill.hpp"

namespace {
ryml::Tree read_yaml(const char* path) {
    std::ifstream file(path); check(file.good(), "actual YAML opens");
    std::string text((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    return ryml::parse_in_arena(ryml::to_csubstr(text));
}
const int weapon_ids[] = {510185,510189,510190,510191,510193,520047,520052,590104,590117,620056,620057,620059};
const int skill_ids[] = {6552,6555,6578,6579,6580,6581,6582,6586,6588,6589,6590,6592,6593,6594,6603};
struct Expected {
    int atk=0,atk_rate=0,matk=0,matk_rate=0,cri=0,patk=0,smatk=0,crate=0;
    int melee=0,ranged=0,crit_damage=0,aspd=0,delay=0,varcast=0;
    int pow=0,con=0,spl=0,physical_size=0,magic_size=0;
    int small=0,medium=0,large=0,water=0,earth=0,element=0,magic_element=0,race=0;
    bool unbreakable=false;
    std::map<int,int> skills,sp,cooldown;
};
void reset(map_session_data& p) {
    p.bonus={}; p.base_status={}; p.battle_status={}; p.indexed_bonus={};
    p.right_weapon={}; p.left_weapon={}; p.matk_rate=0; p.dsprate=100;
    p.skillatk.clear(); p.skillusesp.clear(); p.skillcooldown.clear();
}
void execute(script_code* script, map_session_data& p) {
    if (script) run_script(script,0,p.id,0);
    check(errors==0 && p.st==nullptr,"native VM completes without errors");
}
int value(const std::vector<s_item_bonus>& values,int id) {
    int result=0; for (const auto& v:values) if(v.id==id) result+=v.val; return result;
}
int value(const std::map<int,int>& values,int id) {
    auto it=values.find(id); return it==values.end()?0:it->second;
}
void verify(map_session_data& p,const Expected& e) {
    check(p.bonus.eatk==e.atk && p.bonus.atk_rate==e.atk_rate,"flat physical ATK and percent ATK");
    check(p.bonus.ematk==e.matk && p.matk_rate==e.matk_rate,"flat MATK and percent MATK");
    check(p.base_status.cri==10*e.cri && p.base_status.crate==e.crate,"CRI native tenths versus C.RATE");
    check(p.base_status.patk==e.patk && p.base_status.smatk==e.smatk,"P.ATK and S.MATK");
    check(p.bonus.short_attack_atk_rate==e.melee && p.bonus.long_attack_atk_rate==e.ranged,"melee versus ranged");
    check(p.bonus.crit_atk_rate==e.crit_damage,"critical damage");
    check(p.base_status.aspd_rate2==e.aspd && p.bonus.delayrate==e.delay && p.bonus.varcastrate==e.varcast,"ASPD, aftercast and variable cast are distinct");
    check(p.bonus.unbreakable_equip==(e.unbreakable?EQP_WEAPON:0),"exact sourced breakability");
    for(int stat=0;stat<PARAM_MAX;++stat)
        check(p.indexed_bonus.param_bonus[stat]==(stat==PARAM_POW?e.pow:stat==PARAM_CON?e.con:stat==PARAM_SPL?e.spl:0),"all trait/basic stats including negative controls");
    for(int size=0;size<SZ_MAX;++size) {
        check(p.right_weapon.addsize[size]==(size==SZ_ALL?e.physical_size:0),"physical all-size target array");
        int magic=size==SZ_ALL?e.magic_size:size==SZ_SMALL?e.small:size==SZ_MEDIUM?e.medium:size==SZ_BIG?e.large:0;
        check(p.indexed_bonus.magic_addsize[size]==magic,"magic size-specific versus all-size array");
    }
    for(int ele=0;ele<ELE_MAX;++ele) {
        check(p.right_weapon.addele[ele]==(ele==ELE_ALL?e.element:0),"physical target-element array");
        check(p.indexed_bonus.magic_addele_script[ele]==(ele==ELE_ALL?e.magic_element:0),"magical target-element script array consumed by battle");
        check(p.indexed_bonus.magic_addele[ele]==0,"legacy magical target-element array unchanged");
        check(p.indexed_bonus.magic_atk_ele[ele]==(ele==ELE_WATER?e.water:ele==ELE_EARTH?e.earth:0),"casting element separate from target element");
    }
    for(int race=0;race<RC_MAX;++race) {
        int expected=race==RC_ALL?e.race:(race==RC_PLAYER_HUMAN||race==RC_PLAYER_DORAM)?-e.race:0;
        check(p.right_weapon.addrace[race]==expected,"all-race physical bonus explicitly cancels both player races");
    }
    for(int id:skill_ids) {
        int parent=id==6589?6588:id;
        check(pc_skillatk_bonus(&p,id)==value(e.skills,parent),"exact skill bonus and enhanced Quill parent mapping");
        check(value(p.skillusesp,id)==value(e.sp,id),"exact native SP cost adjustment sign");
        check(value(p.skillcooldown,id)==value(e.cooldown,id),"cooldown milliseconds and no wrong skill");
    }
}
Expected weapon_expected(int id,int r,int g,int level) {
    Expected e;
    bool d=g>=ENCHANTGRADE_D,c=g>=ENCHANTGRADE_C,b=g>=ENCHANTGRADE_B,a=g>=ENCHANTGRADE_A;
    bool r7=r>=7,r9=r>=9,r10=r>=10,r11=r>=11;
    switch(id) {
      case 510185:
        e.atk_rate=5; e.varcast=r7?10:0; e.skills[6555]=(r9?25:0)+(r11?20:0); e.ranged=r11?15:0; break;
      case 510189:
        e.skills[6588]=6*(r/2)+(c?10:0); e.skills[6586]=8*(r/3); e.ranged=4*(r/4);
        e.con=d?2:0; e.patk=b?2:0; break;
      case 510190: case 510191: break;
      case 510193: case 620059:
        e.unbreakable=id==620059; e.cri=5+(r9?15:0); e.crate=r9?5:0;
        e.skills[id==510193?6588:6582]=15+5*(r/3)+(r11?15:0)+(c?15:0);
        e.atk=25*(r/2); e.atk_rate=r/2; e.patk=(d?5:0)+(a?3*(r/2):0);
        if(id==510193) {e.crit_damage=r7?20:0;e.ranged=b?10:0;}
        else e.melee=(r7?25:0)+(b?10:0);
        break;
      case 520047:
        e.unbreakable=true; e.atk_rate=10; e.atk=r7?70:0; e.aspd=r7?10:0;
        e.skills[6552]=20+(r9?15:0)+(r11?10:0)+(c?10:0)+(a?10:0);
        e.skills[6582]=20+(r9?10:0)+(r11?10:0)+(c?10:0)+(a?10:0);
        e.physical_size=r9?15:0; e.sp[6552]=e.sp[6582]=-25*((r9?1:0)+(r11?1:0));
        e.delay=r11?15:0; e.patk=d?10:0; e.element=b?10:0; break;
      case 520052:
        e.unbreakable=true; e.atk_rate=(level>=210?4:0)+(a?3*(r/2):0); e.atk=level>=210?40:0;
        e.skills[6580]=5+(level>=220?5:0)+(r9?10:0)+(r11?10:0)+(c?5:0);
        e.physical_size=level>=230?10:0; e.melee=(r7?10:0)+(d?15:0); e.cri=r10?10:0;
        e.skills[6578]=e.skills[6579]=b?10:0; e.patk=a?3*(r/2):0; break;
      case 590104:
        e.unbreakable=true; e.matk_rate=5; e.skills[6592]=e.skills[6593]=10+(r9?10:0)+(r11?10:0);
        e.water=r7?10:0; e.small=d?15:0; e.medium=c?15:0; e.large=b?15:0; e.smatk=a?5:0; break;
      case 590117:
        e.unbreakable=true; e.matk_rate=(level>=210?4:0)+(a?3*(r/2):0); e.matk=level>=210?40:0;
        e.skills[6603]=10+(level>=220?5:0)+(r9?10:0)+(c?5:0);
        e.skills[6594]=(r11?10:0)+(b?10:0); e.magic_size=level>=230?10:0;
        e.water=e.earth=(r7?10:0)+(r10?10:0)+(d?10:0); e.smatk=a?3*(r/2):0; break;
      case 620056: case 620057: e.unbreakable=true; break;
      default: check(false,"unknown test identity");
    }
    return e;
}
int slot_for(const item_data& data) {
    if(data.type==IT_WEAPON) return EQI_HAND_R;
    if(data.equip&EQP_HEAD_TOP) return EQI_HEAD_TOP;
    if(data.equip&EQP_SHOES) return EQI_SHOES;
    if(data.equip&EQP_ARMOR) return EQI_ARMOR;
    if(data.equip&EQP_GARMENT) return EQI_GARMENT;
    check(false,"unexpected combo location"); return -1;
}
}
extern "C" int __wrap_main(int argc,char** argv) {
    deny_network(); check(argc==2,"actual item and skill dependency fixture supplied");
    static char name[]="druid-missing-weapons-native"; SERVER_NAME=name;
    malloc_init(); db_init(); do_init_database(); timer_init(); battle_set_defaults(); do_init_script();
    auto fixture=read_yaml(argv[1]);
    for(auto node:fixture["Skills"]) check(skill_db.parseBodyNode(node)==1,"actual current skill record parses");
    for(auto node:fixture["Items"]) check(item_db.parseBodyNode(node)==1,"actual existing/candidate dependency parses");
    auto items=read_yaml("db/import/druid_missing_weapons.yml");
    for(auto node:items["Body"]) check(item_db.parseBodyNode(node)==1,"actual weapon YAML parses");
    item_db.loadingFinished();
    check(!item_db.find(510200)&&!item_db.find(620064),"unresolved Booster items not fabricated");
    auto p=std::make_unique<map_session_data>(); attached=p.get();
    p->id=p->status.account_id=99000001; p->status.char_id=99000002; p->type=BL_PC;
    p->state.ignoretimeout=true; p->npc_idle_timer=INVALID_TIMER; p->state.lr_flag=LR_FLAG_NONE;
    battle_config.allow_equip_restricted_item=1; battle_config.atcommand_disable_npc=0;
    check(p->permissions.none()&&!pc_has_permission(p.get(),PC_PERM_USE_ALL_EQUIPMENT),"GM equipment bypass disabled");
    std::fill(std::begin(p->equip_index),std::end(p->equip_index),-1); p->equip_index[EQI_HAND_R]=0;
    current_equip_item_index=0;
    int effects=0,equips=0;
    for(int id:weapon_ids) {
        auto data=item_db.find(id); check(data&&data->type==IT_WEAPON&&data->weapon_level==5,"actual level5 weapon metadata");
        p->inventory_data[0]=data.get(); p->inventory.u.items_inventory[0].nameid=id;
        check(data->range==1,"explicit documented project category reach");
        check(data->sex==SEX_BOTH,"both-sex native default");
        p->status.base_level=275;
        for(int job=0;job<JOB_MAX;++job) {
            uint64 mapped=pc_jobid2mapid(job); if(mapped==static_cast<uint64>(-1)) continue;
            p->class_=mapped;
            bool expected=mapped==MAPID_ALITEA || (id==510185 && (mapped==MAPID_KARNOS||mapped==MAPID_BABY_KARNOS));
            bool actual=pc_isequip(p.get(),0)==ITEM_EQUIP_ACK_OK;
            if(actual!=expected) std::fprintf(stderr,"Equip mismatch item=%d job=%d mapped=%llu actual=%d expected=%d\n",id,job,static_cast<unsigned long long>(mapped),actual,expected);
            check(actual==expected,"actual all-supported-job equip matrix"); ++equips;
        }
        p->class_=MAPID_ALITEA; p->status.base_level=data->elv-1;
        check(pc_isequip(p.get(),0)==ITEM_EQUIP_ACK_FAILLEVEL,"minimum level negative control");
        p->status.base_level=data->elv;
        check(pc_isequip(p.get(),0)==ITEM_EQUIP_ACK_OK,"exact minimum level positive control");
        p->inventory.u.items_inventory[0].attribute=1;
        check(pc_isequip(p.get(),0)!=ITEM_EQUIP_ACK_OK,"broken weapon rejected");
        p->inventory.u.items_inventory[0].attribute=0;
        for(int r=0;r<=20;++r) for(int g=0;g<=ENCHANTGRADE_A;++g)
          for(int level:{169,170,204,205,209,210,219,220,229,230,249,250,275}) {
            reset(*p); p->status.base_level=level;
            p->inventory.u.items_inventory[0].refine=r; p->inventory.u.items_inventory[0].enchantgrade=g;
            auto e=weapon_expected(id,r,g,level); execute(data->script,*p); verify(*p,e); ++effects;
            if(id==520047) for(int skill:{6552,6582}) {
                auto required=skill_get_requirement(p.get(),skill,1);
                check(required.sp==skill_db.find(skill)->require.sp[0]-value(e.sp,skill),"real skill requirement adds exactly 25/50 SP at refine9/11, not before");
            }
          }
    }
    check(item_db.find(510190)->slots==0&&item_db.find(620056)->slots==0,"native Glacier chassis permits all four original enchant indexes");
    for(int slot:{3,2,1,0}) check(slot>=item_db.find(510190)->slots,"original group31 order has no native physical-slot collision");
    auto combos=read_yaml("db/import/druid_missing_weapon_combos.yml"); int parsed=0;
    for(auto node:combos["Body"]) parsed+=itemdb_combo.parseBodyNode(node);
    check(parsed==10&&errors==0,"ten exact sets parse with actual named dependencies"); itemdb_combo.loadingFinished();
    int set_cases=0;
    for(const auto& entry:itemdb_combo) {
        const auto& combo=entry.second;
        p->combos.clear(); std::fill(std::begin(p->equip_index),std::end(p->equip_index),-1);
        for(int i=0;i<MAX_INVENTORY;++i) {p->inventory_data[i]=nullptr;p->inventory.u.items_inventory[i]={};}
        int wi=-1,hi=-1,si=-1;
        for(size_t i=0;i<combo->nameid.size();++i) {
            auto data=item_db.find(combo->nameid[i]); int slot=slot_for(*data);
            p->inventory_data[i]=data.get(); p->inventory.u.items_inventory[i].nameid=data->nameid;
            p->inventory.u.items_inventory[i].equip=data->equip; p->equip_index[slot]=i;
            if(slot==EQI_HAND_R) wi=i; if(slot==EQI_HEAD_TOP) hi=i; if(slot==EQI_SHOES) si=i;
        }
        check(wi>=0,"every set contains exactly the weapon dependency");
        check(pc_load_combo(p.get())==1&&p->combos.size()==1&&p->combos[0]->id==combo->id,"actual equipment matcher activates exact set once");
        check(pc_load_combo(p.get())==0&&p->combos.size()==1,"duplicate scan cannot register set twice");
        for(size_t absent=0;absent<combo->nameid.size();++absent) {
            auto saved=p->inventory_data[absent]; p->inventory_data[absent]=nullptr; p->combos.clear();
            check(pc_load_combo(p.get())==0&&p->combos.empty(),"every individual missing partner prevents set activation");
            p->inventory_data[absent]=saved;
        }
        const int weapon=combo->nameid[wi];
        bool physical=std::find(combo->nameid.begin(),combo->nameid.end(),450270)!=combo->nameid.end();
        for(int r=0;r<=20;++r) for(int partner_r=0;partner_r<=20;++partner_r)
          for(int g=0;g<=ENCHANTGRADE_A;++g) for(int partner_g=0;partner_g<=ENCHANTGRADE_A;++partner_g) {
            reset(*p); Expected e;
            for(size_t i=0;i<combo->nameid.size();++i) {
                p->inventory.u.items_inventory[i].refine=i==static_cast<size_t>(wi)?r:partner_r;
                p->inventory.u.items_inventory[i].enchantgrade=i==static_cast<size_t>(wi)?g:partner_g;
            }
            if(combo->nameid.size()==4) {
                int amount=(g>=ENCHANTGRADE_C?6:0)+(g>=ENCHANTGRADE_B?5:0)+(g>=ENCHANTGRADE_A?4:0);
                e.con=5*((g>=ENCHANTGRADE_C?1:0)+(g>=ENCHANTGRADE_B?1:0)+(g>=ENCHANTGRADE_A?1:0));
                if(physical) {e.atk=10*(r/3);e.patk=amount;e.pow=e.con;e.element=weapon==620057&&g>=ENCHANTGRADE_A?20:0;}
                else {e.matk=10*(r/3);e.smatk=amount;e.spl=e.con;e.magic_element=weapon==620057&&g>=ENCHANTGRADE_A?20:0;}
            } else if(weapon==520052) {
                if(si>=0) e.skills[6578]=e.skills[6579]=r+partner_r; else e.race=10;
            } else if(weapon==590117) e.skills[6594]=si>=0?r+partner_r:10;
            else {
                bool grade_a=g>=ENCHANTGRADE_A&&partner_g>=ENCHANTGRADE_A;
                if(weapon==620059) {e.skills[6581]=60+(grade_a?r+partner_r:0);e.cooldown[6582]=grade_a?-200:0;}
                else {check(weapon==510193,"only exact Dimensions dagger set");e.skills[6590]=45;e.ranged=15;e.skills[6588]=grade_a?r+partner_r:0;e.cooldown[6588]=grade_a?-200:0;}
            }
            current_equip_item_index=wi; execute(combo->script,*p); verify(*p,e); ++set_cases;
            for(int skill:{6582,6588})
                check(pc_get_skillcooldown(p.get(),skill,1)==std::max(0,skill_get_cooldown(skill,1)+value(e.cooldown,skill)),"actual skill cooldown consumes exact millisecond modifier");
          }
    }
    check(added==0&&removed==0&&logged.empty(),"no inventory mutation or acquisition claimed");
    current_equip_item_index=-1; attached=nullptr; p.reset();
    itemdb_combo.clear(); item_db.clear(); skill_db.clear();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("DRUID_MISSING_WEAPONS_NATIVE_COMPLETE: %d weapon cases; %d set cases; %d job cases; %d assertions\n",effects,set_cases,equips,assertions);
    return errors?1:0;
}
