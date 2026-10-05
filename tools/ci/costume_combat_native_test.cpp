// GPL-3.0-or-later. Appended to the established isolated VM fixture.
// Native item/combo parsing, combo discovery, status recalculation and bonus
// application execute unchanged. Transport, registries and world lookup are
// fixture boundaries; no game server, network or player database is started.
#include "map/status.hpp"
#include "map/skill.hpp"
#include "map/mob.hpp"
#include <nlohmann/json.hpp>
using nlohmann::json;
extern int32 status_calc_pc_sub(map_session_data*,uint8);
extern "C" block_list* costume_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* costume_world(int32 id){return attached&&attached->id==id?static_cast<block_list*>(attached):nullptr;}
namespace {
template<class Database> void parse_rows(const std::string& path, Database& database) {
    auto source=read(path);auto tree=ryml::parse_in_arena(ryml::to_csubstr(source));
    for(auto row:tree["Body"])check(database.parseBodyNode(row)>0,"native database row parses");
}
void fixture_costume(int id,uint32 location) {
    auto d=std::make_shared<item_data>();d->nameid=id;d->type=IT_ARMOR;d->equip=location;
    d->ename="Costume combat fixture";d->weight=0;item_db.put(id,d);
}
void equip(int index,int slot,uint32 location,int id,const json& cards) {
    put(index,id,1);auto& it=attached->inventory.u.items_inventory[index];it.equip=location;
    attached->equip_index[slot]=index;
    for(size_t n=0;n<cards.size();++n)it.card[n]=cards[n].get<int>();
}
json snapshot(map_session_data& sd) {
    json skills=json::object();for(auto& b:sd.skillatk)skills[std::to_string(b.id)]=b.val;
    sd.battle_status=sd.base_status;
    auto target=std::make_unique<map_session_data>();target->type=BL_PC;target->m=0;
    target->status.class_=JOB_NOVICE;target->battle_status.size=SZ_MEDIUM;
    target->battle_status.race=RC_DEMIHUMAN;target->battle_status.class_=CLASS_NORMAL;
    target->battle_status.def_ele=ELE_NEUTRAL;
    int magic_hit=10000+battle_calc_cardfix(BF_MAGIC,&sd,target.get(),{},ELE_NEUTRAL,ELE_NEUTRAL,10000,0,BF_MAGIC|BF_SKILL|BF_LONG);
    int weapon_hit=10000+battle_calc_cardfix(BF_WEAPON,&sd,target.get(),{},ELE_NEUTRAL,ELE_NEUTRAL,10000,2,BF_WEAPON|BF_SKILL|BF_SHORT);
    sd.state.arrow_atk=true;
    int projectile_hit=10000+battle_calc_cardfix(BF_WEAPON,&sd,target.get(),{},ELE_NEUTRAL,ELE_NEUTRAL,10000,2,BF_WEAPON|BF_SKILL|BF_LONG);
    sd.state.arrow_atk=false;
    return {{"str",sd.base_status.str},{"agi",sd.base_status.agi},{"vit",sd.base_status.vit},
      {"int",sd.base_status.int_},{"dex",sd.base_status.dex},{"luk",sd.base_status.luk},
      {"pow",sd.base_status.pow},{"sta",sd.base_status.sta},{"wis",sd.base_status.wis},
      {"spl",sd.base_status.spl},{"con",sd.base_status.con},{"crt",sd.base_status.crt},
      {"patk",sd.base_status.patk},{"smatk",sd.base_status.smatk},
      {"hp",sd.base_status.max_hp},{"sp",sd.base_status.max_sp},
      {"crit_bonus",sd.bonus.crit_atk_rate},{"crit",sd.base_status.cri},
      {"short",sd.bonus.short_attack_atk_rate},{"long",sd.bonus.long_attack_atk_rate},
      {"magic_all",sd.indexed_bonus.magic_atk_ele[ELE_ALL]},
      {"size_all",sd.right_weapon.addsize[SZ_ALL]},{"physical_class",sd.right_weapon.addclass[CLASS_ALL]},
      {"magic_class",sd.indexed_bonus.magic_addclass[CLASS_ALL]},{"magic_hit",magic_hit},
      {"weapon_cardfix_hit",weapon_hit},{"projectile_cardfix_hit",projectile_hit},
      {"soulstrike_delay",skill_delayfix(&sd,MG_SOULSTRIKE,1)},
      {"soulstrike_cast",skill_vfcastfix(&sd,400,MG_SOULSTRIKE,1)},
      {"matk_bonus",sd.bonus.ematk},{"batk_bonus",sd.bonus.eatk},{"hit",sd.base_status.hit},
      {"hp_rate",sd.hprate},{"sp_rate",sd.sprate},{"matk_rate",sd.matk_rate},{"greed",pc_checkskill(&sd,BS_GREED)},
      {"fixed",sd.bonus.add_fixcast},{"vct",sd.bonus.varcastrate},{"delay",-sd.bonus.delayrate},
      {"effects",sd.ud.hatEffects},{"combos",sd.combos.size()},{"skills",skills}};
}
}
extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"input directory");deny_network();static char server[]="costume-combat-native";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    check(job_db.load(),"native job metadata loads");
    map_num=1;std::strcpy(map[0].name,"costume_test");map[0].initMapFlags();
    parse_rows(std::string(argv[1])+"/items.yml",item_db);
    check(skill_db.load(),"native skill metadata loads");
    parse_rows(std::string(argv[1])+"/combos.yml",itemdb_combo);
    itemdb_combo.loadingFinished();
    parse_rows(std::string(argv[1])+"/skills.yml",skill_db);
    fixture_costume(19961,EQP_COSTUME_HEAD_TOP);fixture_costume(19962,EQP_COSTUME_HEAD_MID);
    fixture_costume(19963,EQP_COSTUME_HEAD_LOW);fixture_costume(19964,EQP_COSTUME_GARMENT);
    fixture_costume(19965,EQP_ARMOR);
    auto input=json::parse(read(std::string(argv[1])+"/cases.json"));json results=json::array();
    for(const auto& test:input) {
        ++cases;errors=0;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=sd->status.account_id=99000001;sd->type=BL_PC;sd->status.char_id=99000002;
        sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;sd->status.sex=SEX_MALE;
        sd->status.base_level=1;sd->status.job_level=1;sd->status.inventory_slots=MAX_INVENTORY;
        sd->fd=0;sd->state.connect_new=true;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->m=0;
        sd->status.str=sd->status.agi=sd->status.vit=sd->status.int_=sd->status.dex=sd->status.luk=1;
        for(auto& i:sd->equip_index)i=-1;
        equip(0,EQI_COSTUME_HEAD_TOP,EQP_COSTUME_HEAD_TOP,19961,test.value("upper",json::array()));
        equip(1,EQI_COSTUME_HEAD_MID,EQP_COSTUME_HEAD_MID,19962,test.value("middle",json::array()));
        equip(2,EQI_COSTUME_HEAD_LOW,EQP_COSTUME_HEAD_LOW,19963,test.value("lower",json::array()));
        equip(3,EQI_COSTUME_GARMENT,EQP_COSTUME_GARMENT,19964,test.value("garment",json::array()));
        if(test.contains("armor"))equip(4,EQI_ARMOR,EQP_ARMOR,19965,test["armor"]);
        if(test.contains("learned"))for(auto it=test["learned"].begin();it!=test["learned"].end();++it){
            int id=skill_name2id(it.key().c_str());check(id>0,"native named skill exists");auto& sk=sd->status.skill[skill_get_index(id)];sk.id=id;sk.lv=it.value();sk.flag=SKILL_FLAG_PERM_GRANTED;
        }
        pc_load_combo(sd.get());
        int rc=status_calc_pc_sub(sd.get(),SCO_FIRST);
        if(rc||errors||sd->st)std::fprintf(stderr,"Status rc=%d errors=%u st=%p\n",rc,errors,static_cast<void*>(sd->st));
        check(rc==0&&!errors&&!sd->st,"real status recalculation completes without diagnostics");
        auto result=snapshot(*sd);result["name"]=test["name"];
        if(test["kind"]=="independent")std::printf("CASE %s %s\n",test["name"].get<std::string>().c_str(),result.dump().c_str());
        results.push_back(result);
        for(auto it=test["expected"].begin();it!=test["expected"].end();++it){
            if(result[it.key()]!=it.value())std::fprintf(stderr,"Expected %s=%s, got %s\n",it.key().c_str(),it.value().dump().c_str(),result[it.key()].dump().c_str());
            check(result[it.key()]==it.value(),"independent described bonus reaches native status");
        }
        if(test.contains("timing"))for(auto it=test["timing"].begin();it!=test["timing"].end();++it)
            check(std::abs(result[it.key()].get<int>()-it.value().get<int>())<=1,"native skill timing agrees within one millisecond float truncation");
        check(status_calc_pc_sub(sd.get(),SCO_FIRST)==0,"second status recalculation completes");
        auto repeated=snapshot(*sd);result.erase("name");
        check(repeated==result,"recalculation does not accumulate stone bonuses");
        if(test.contains("remove_middle")) {
            auto& middle=sd->inventory.u.items_inventory[1];auto saved=middle.card[1];middle.card[1]=0;
            sd->combos.clear();pc_load_combo(sd.get());
            check(status_calc_pc_sub(sd.get(),SCO_FIRST)==0,"partial combo removal recalculates");
            auto partial=snapshot(*sd);
            for(auto it=test["remove_middle"].begin();it!=test["remove_middle"].end();++it)
                check(partial[it.key()]==it.value(),"partial removal retains only remaining bonuses");
            middle.card[1]=saved;sd->combos.clear();pc_load_combo(sd.get());
            check(status_calc_pc_sub(sd.get(),SCO_FIRST)==0,"combo restoration recalculates");
            check(snapshot(*sd)==result,"reinserted combo restores exact original snapshot");
        }
        // Inventory replacement is a fixture boundary; native combo discovery
        // and status calculation still execute after removal.
        for(auto& it:sd->inventory.u.items_inventory)for(int slot=0;slot<MAX_SLOTS;++slot)if(it.card[slot]){
            auto data=item_db.find(it.card[slot]);if(data&&data->unequip_script)run_script(data->unequip_script,0,sd->id,0);
            it.card[slot]=0;
        }
        sd->combos.clear();pc_load_combo(sd.get());
        check(status_calc_pc_sub(sd.get(),SCO_FIRST)==0,"status recalculation after removal completes");
        auto removed=snapshot(*sd);
        for(const char* key:{"combos","short","long","magic_all","fixed","vct","delay","size_all","physical_class","magic_class","greed"})
            check(removed[key]==0,"removed stone or combo leaves no bonus behind");
        if(test["kind"]=="smoke"){
            auto baseline=results[0];baseline.erase("name");
            check(removed==baseline,"all observed stats, skills and effects return to bare baseline");
        }
        check(!errors&&!sd->st,"all recalculations finish cleanly");
        attached=nullptr;
    }
    std::ofstream(std::string(argv[1])+"/results.json")<<results.dump(2)<<"\n";
    itemdb_combo.clear();item_db.clear();skill_db.clear();job_db.clear();do_final_script();timer_final();db_final();malloc_final();
    std::printf("COSTUME_COMBAT_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return 0;
}
