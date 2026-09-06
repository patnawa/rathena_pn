// GPL-3.0-or-later. The runner prefixes the existing isolated VM boundary
// wrappers (not its main). Production Item/Combo/Skill DBs and Script/pc bonus
// functions execute here. No simulated bonus application or network access.
#include <map>
#include <set>
#include <algorithm>
#include <tuple>

namespace {
std::string scalar(const ryml::NodeRef& node) {
    std::string value; node >> value; return value;
}
int number(const ryml::NodeRef& node) { int value; node >> value; return value; }
int optional(const ryml::NodeRef& node, const char* key) {
    auto name=ryml::to_csubstr(key);return node.has_child(name) ? number(node[name]) : 0;
}
ryml::Tree tree_file(const std::string& path) {
    auto source = read(path); return ryml::parse_in_arena(ryml::to_csubstr(source));
}
int constant(const std::string& name) {
    int64 value = 0; check(script_get_constant(name.c_str(), &value), "real script constant resolves"); return value;
}
int skill_id(const std::string& name) {
    auto id = skill_name2id(name.c_str()); check(id != 0, "real skill name resolves"); return id;
}
struct Effect {
    std::string op; int selector = 0, value = 0, step = 0;
    int refine = 0, grade = 0, sum = 0, crown_grade = 0, weapon_grade = 0;
    int learned = 0, level = 0, cast = 0, castlevel = 0;
};
using Key = std::tuple<std::string, int, int, int>;
Key key(const Effect& e) { return {e.op,e.selector,e.cast,e.castlevel}; }
std::vector<Effect> effects(const ryml::NodeRef& node) {
    std::vector<Effect> result;
    for (auto row : node) {
        Effect e; e.op = scalar(row["op"]); e.value = number(row["value"]); e.step = number(row["step"]);
        if (!row["selector"].val_is_null()) {
            auto name = scalar(row["selector"]);
            if (e.op == "bAutoSpellOnSkill") {
                const auto first = name.find('|'), last = name.rfind('|');
                e.selector = skill_id(name.substr(0, first)); e.cast = skill_id(name.substr(first+1,last-first-1));
                e.castlevel = std::stoi(name.substr(last+1));
            } else if (e.op == "bSkillAtk" || e.op == "bSkillCooldown") e.selector = skill_id(name);
            else e.selector = constant(name);
        }
        const auto c = row["conditions"];
        e.refine = optional(c,"refine"); e.grade = optional(c,"grade"); e.sum = optional(c,"sum");
        e.crown_grade = optional(c,"crown_grade"); e.weapon_grade = optional(c,"weapon_grade");
        if (c.has_child("learned")) { e.learned = skill_id(scalar(c["learned"])); e.level = number(c["learned_level"]); }
        result.push_back(e);
    }
    return result;
}
int observed(map_session_data& s, const Key& k) {
    const auto& [op, selector, cast, castlevel] = k;
    if(op=="bBaseAtk") return s.bonus.eatk;
    if(op=="bMatk") return s.bonus.ematk;
    if(op=="bAtkRate") return s.bonus.atk_rate;
    if(op=="bMatkRate") return s.matk_rate;
    if(op=="bCritical") return s.base_status.cri/10;
    if(op=="bHit") return s.base_status.hit;
    if(op=="bPAtk") return s.base_status.patk;
    if(op=="bSMatk") return s.base_status.smatk;
    if(op=="bCRate") return s.base_status.crate;
    if(op=="bPow") return s.indexed_bonus.param_bonus[PARAM_POW];
    if(op=="bSpl") return s.indexed_bonus.param_bonus[PARAM_SPL];
    if(op=="bCon") return s.indexed_bonus.param_bonus[PARAM_CON];
    if(op=="bMaxHP") return s.bonus.hp;
    if(op=="bMaxSP") return s.bonus.sp;
    if(op=="bMaxHPrate") return s.hprate;
    if(op=="bMaxSPrate") return s.sprate;
    if(op=="bCritAtkRate") return s.bonus.crit_atk_rate;
    if(op=="bNonCritAtkRate") return s.bonus.non_crit_atk_rate;
    if(op=="bShortAtkRate") return s.bonus.short_attack_atk_rate;
    if(op=="bLongAtkRate") return s.bonus.long_attack_atk_rate;
    if(op=="bVariableCastrate") return -s.bonus.varcastrate;
    if(op=="bDelayrate") return -s.bonus.delayrate;
    if(op=="bFixedCast") return s.bonus.add_fixcast;
    if(op=="bPerfectHitAddRate") return s.bonus.perfect_hit_add;
    if(op=="bUnbreakableWeapon") return (s.bonus.unbreakable_equip & EQP_WEAPON) ? 1 : 0;
    if(op=="bMagicAtkEle") return s.indexed_bonus.magic_atk_ele[selector];
    if(op=="bMagicAddSize") return s.indexed_bonus.magic_addsize[selector];
    if(op=="bMagicAddEle") return s.indexed_bonus.magic_addele_script[selector];
    if(op=="bAddSize") return s.right_weapon.addsize[selector];
    if(op=="bAddEle") return s.right_weapon.addele[selector];
    if(op=="bAddRace") return s.right_weapon.addrace[selector];
    if(op=="bSkillAtk") return pc_skillatk_bonus(&s,selector);
    if(op=="bSkillCooldown") {
        int total=0; for(const auto& b:s.skillcooldown) if(b.id==selector) total+=b.val;
        check(pc_get_skillcooldown(&s,selector,1)==std::max(0,skill_get_cooldown(selector,1)+total),
              "actual cooldown consumer includes exact skill-specific adjustment"); return total;
    }
    if(op=="bAutoSpellOnSkill") {
        int total=0; for(const auto& b:s.autospell3)
            if(b.trigger_skill==selector && b.id==cast && b.lv==castlevel) total+=b.rate;
        return total;
    }
    check(false,"unreviewed observed native bonus field"); return 0;
}
void reset(map_session_data& s) {
    // Reset only POD bonus/status fields and owning containers through their
    // APIs. Never copy or memset item_data/map_session_data owning objects.
    s.base_status = {}; s.bonus = {}; s.indexed_bonus = {};
    s.right_weapon = {}; s.left_weapon = {};
    s.hprate = s.sprate = s.matk_rate = 0;
    s.skillatk.clear(); s.skillcooldown.clear(); s.autospell3.clear();
}
void verify(script_code* code,map_session_data& s,const std::vector<Effect>& oracle,
            const std::set<Key>& all_keys,int hr,int wr,int hg,int wg,int learned,int index) {
    reset(s); current_equip_item_index=index;
    s.inventory.u.items_inventory[0].refine=hr; s.inventory.u.items_inventory[1].refine=wr;
    s.inventory.u.items_inventory[0].enchantgrade=hg; s.inventory.u.items_inventory[1].enchantgrade=wg;
    std::map<Key,int> expected;
    for(const auto& e:oracle) {
        if(e.learned) {auto ix=skill_get_index(e.learned); check(ix>0,"learned skill has real index");
            s.status.skill[ix].id=e.learned;s.status.skill[ix].lv=learned;}
        const int r=index==0?hr:wr,g=index==0?hg:wg;
        if(r>=e.refine && g>=e.grade && hr+wr>=e.sum && hg>=e.crown_grade && wg>=e.weapon_grade && learned>=e.level)
            expected[key(e)] += e.value * (e.step>0 ? r/e.step : e.step==-1 ? hr+wr : 1);
    }
    attached=&s; run_script(code,0,s.id,0); ++executions;
    check(errors==0 && s.st==nullptr,"exact Script ran in real VM and detached cleanly");
    int autocasts=0;
    for(const auto& k:all_keys) {
        const int actual=observed(s,k), wanted=expected[k];
        if(actual!=wanted) std::fprintf(stderr,"op=%s selector=%d actual=%d expected=%d hr=%d wr=%d hg=%d wg=%d learned=%d\n",
            std::get<0>(k).c_str(),std::get<1>(k),actual,wanted,hr,wr,hg,wg,learned);
        check(actual==wanted,"all native bonus fields equal independent typed arithmetic oracle, including zero unrelated fields");
        if(std::get<0>(k)=="bAutoSpellOnSkill" && wanted) ++autocasts;
        if(std::get<0>(k)=="bSkillAtk") {
            for(int dummy:{AG_ASTRAL_STRIKE_ATK,TR_ROSEBLOSSOM_ATK,AT_QUILL_SPEAR_S})
                if(skill_dummy2skill_id(dummy)==std::get<1>(k))
                    check(pc_skillatk_bonus(&s,dummy)==wanted,"actual damage dummy inherits parent bonus once");
        }
    }
    check(s.autospell3.size()==static_cast<size_t>(autocasts),"no unreviewed autocast registrations");
}
}

extern "C" int __wrap_main(int argc,char** argv) {
    deny_network();check(argc==2,"explicit isolated fixture directory");
    const std::string dir=argv[1];static char name[]="missing-crowns-native-vm";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();
    battle_set_defaults();
    battle_config.allow_equip_restricted_item=1;battle_config.atcommand_disable_npc=0;
    auto skills=tree_file(dir+"/skills.yml");
    for(auto n:skills.rootref()) check(skill_db.parseBodyNode(n)==1,"production skill parser");
    skill_db.loadingFinished();
    for(const char* path:{"db/import/druid_missing_crowns.yml","db/import/sky_crown_partner_weapons.yml"}) {
        auto t=tree_file(path);for(auto n:t["Body"]) check(item_db.parseBodyNode(n)==1,"exact new item parses through production ItemDatabase");
    }
    item_db.loadingFinished();
    auto combos=tree_file("db/import/druid_missing_crown_combos.yml");
    for(auto n:combos["Body"]) check(itemdb_combo.parseBodyNode(n)==1,"exact new set parses through production ComboDatabase");
    itemdb_combo.loadingFinished();
    check(item_db.size()==24 && item_db.find(ITEMID_DUMMY)->name=="UNKNOWN_ITEM" &&
          itemdb_combo.size()==11 && errors==0,"23 actual items plus native automatic dummy, and exactly 11 sets");
    auto facts=tree_file(dir+"/facts.yml");std::set<Key> keys;
    for(auto group:{facts["Items"],facts["Combos"]}) for(auto row:group)
        for(const auto& e:effects(row["Effects"])) keys.insert(key(e));
    auto s=player(400999);attached=s.get();s->state.lr_flag=LR_FLAG_NONE;
    s->permissions.reset();s->status.base_level=275;
    // Exhaustive +0..20 and all five grade levels for all 23 item scripts.
    for(auto row:facts["Items"]) {
        const int id=number(row["Id"]),index=number(row["Index"]);auto data=item_db.find(id);
        check(data && data->script,"actual parsed item owns effect script");
        s->inventory_data[index]=data.get();s->inventory.u.items_inventory[index].nameid=id;
        const auto oracle=effects(row["Effects"]);
        for(int r=0;r<=20;++r) for(int g=0;g<=4;++g)
            verify(data->script,*s,oracle,keys,index==0?r:7,index==1?r:8,index==0?g:2,index==1?g:3,0,index);
        // Exact intended job(s), both sex values and below/at minimum level;
        // importantly no GM 'use all equipment' permission bypass.
        for(auto job:row["Jobs"]) for(int sex:{SEX_FEMALE,SEX_MALE}) for(int offset:{-1,0}) {
            const int mapjob=constant(scalar(job));s->class_=mapjob;s->status.sex=sex;
            s->status.base_level=data->elv+offset;
            const bool valid=offset==0 && (data->sex==SEX_BOTH || data->sex==sex);
            check((pc_isequip(s.get(),index)==ITEM_EQUIP_ACK_OK)==valid,"native job/sex/minimum-level eligibility");
        }
        s->class_=MAPID_NOVICE;s->status.base_level=275;s->status.sex=SEX_MALE;
        check(pc_isequip(s.get(),index)!=ITEM_EQUIP_ACK_OK,"unrelated novice cannot equip trait gear");
    }
    int combo_cases=0;
    for(auto row:facts["Combos"]) {
        const int head=number(row["Head"]),weapon=number(row["Weapon"]);
        std::shared_ptr<s_item_combo> found;
        for(const auto& pair:itemdb_combo) {
            const std::set<t_itemid> ids(pair.second->nameid.begin(),pair.second->nameid.end());
            if(ids==std::set<t_itemid>{static_cast<t_itemid>(head),static_cast<t_itemid>(weapon)}) found=pair.second;
        }
        check(found && found->script,"exact two native member identities resolve one set");
        s->inventory_data[0]=item_db.find(head).get();s->inventory_data[1]=item_db.find(weapon).get();
        s->inventory.u.items_inventory[0].nameid=head;s->inventory.u.items_inventory[1].nameid=weapon;
        const auto oracle=effects(row["Effects"]);int level=0;
        for(const auto& e:oracle) level=std::max(level,e.level);
        for(int hr=0;hr<=20;++hr) for(int wr=0;wr<=20;++wr)
        for(int hg:{0,3,4}) for(int wg:{0,3,4}) for(int learned:{level-1,level}) {
            verify(found->script,*s,oracle,keys,hr,wr,hg,wg,learned,0);++combo_cases;
        }
    }
    attached=nullptr;s.reset();itemdb_combo.clear();item_db.clear();skill_db.clear();
    do_final_script();timer_final();db_final();malloc_final();
    check(executions==89733 && combo_cases==87318,"all item and set arithmetic cases executed");
    std::printf("NATIVE_MISSING_CROWNS_OK items=23 combos=11 executions=%d assertions=%d\n",executions,assertions);
    return errors?1:0;
}
