// GPL-3.0-or-later. Appended to the established synthetic native VM scaffold.
// The real equip functions are deliberately NOT linker-wrapped by this runner.
#include "map/status.hpp"
#include "map/skill.hpp"
#include "map/npc.hpp"
#include <nlohmann/json.hpp>
using nlohmann::json;

namespace {
block_list* target_lookup=nullptr;
unsigned equip_acks=0,unequip_acks=0;
template<class Database> void pipeline_parse(const std::string& path, Database& db) {
    auto source=read(path);auto tree=ryml::parse_in_arena(ryml::to_csubstr(source));
    for(auto row:tree["Body"])check(db.parseBodyNode(row)>0,"native database row parses");
}
void pipeline_costume(int id,uint32 position) {
    auto data=std::make_shared<item_data>();data->nameid=id;data->type=IT_ARMOR;data->equip=position;
    data->sex=SEX_BOTH;data->class_upper=ITEMJ_NORMAL;data->flag.available=true;
    for(auto& mask:data->class_base)mask=UINT64_MAX;
    data->ename="Synthetic combat carrier";item_db.put(id,data);
}
void pipeline_put(int index,int id,const json& cards) {
    put(index,id,1);
    for(size_t n=0;n<cards.size();++n)attached->inventory.u.items_inventory[index].card[n]=cards[n].get<int>();
}
json pipeline_snapshot(map_session_data& sd, map_session_data& target) {
    // Fix only attack inputs, after real status recalculation, so variance and
    // weapon identity do not conceal a bonus. This is a calculation boundary.
    sd.battle_status.matk_min=sd.battle_status.matk_max=10000;
    sd.battle_status.smatk=0;sd.battle_status.cri=0;sd.battle_status.batk=10000;
    sd.battle_status.rhw.atk=sd.battle_status.rhw.atk2=0;
    sd.battle_status.watk=sd.battle_status.eatk=0;
    sd.bonus.perfect_hit=100;
    const auto magic=battle_calc_attack(BF_MAGIC,&sd,&target,MG_FIREBOLT,1,0);
    const auto melee=battle_calc_attack(BF_WEAPON,&sd,&target,SM_BASH,1,0);
    check(magic.damage2==0&&magic.div_==1&&magic.dmg_lv==ATK_DEF,"final magic calculation is a single successful hit");
    check((magic.flag&(BF_MAGIC|BF_SKILL))==(BF_MAGIC|BF_SKILL),"magic result retains attack and skill flags");
    check(melee.damage2==0&&melee.div_==1&&melee.dmg_lv==ATK_DEF,"final physical calculation is a single successful hit");
    return {{"magic",magic.damage},{"melee",melee.damage},
            {"melee_rate",sd.bonus.short_attack_atk_rate},{"long_rate",sd.bonus.long_attack_atk_rate},
            {"cast",skill_vfcastfix(&sd,400,MG_SOULSTRIKE,1)},
            {"delay",skill_delayfix(&sd,MG_SOULSTRIKE,1)},{"combos",sd.combos.size()}};
}
void pipeline_expect(const json& got,const json& expected) {
    check(got["magic"]==expected["magic"],"final magic damage agrees with independent item bonus");
    // Bare controlled status attack is 2*10000; Bash Lv1 is 130%.
    // Described short bonuses give 26780/28860/29640 for 3/11/14%, followed
    // by the controlled target's one point of status defense.
    check(got["melee"]==expected["melee"],"final physical damage agrees with independent item bonus");
    for(const char* key:{"melee_rate","long_rate","delay"})check(got[key]==expected[key],"native item bonus or delay agrees with independent oracle");
    check(std::abs(got["cast"].get<int>()-expected["cast"].get<int>())<=1,"native cast timing agrees within one millisecond truncation");
}
}
extern "C" block_list* pipeline_world(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* pipeline_world(int32 id) {
    if(attached&&attached->id==id)return attached;
    return target_lookup&&target_lookup->id==id?target_lookup:nullptr;
}
extern "C" void pipeline_equipack(const map_session_data&,uint8,int32,int32) asm("__wrap__Z17clif_equipitemackRK16map_session_datahii");
extern "C" void pipeline_equipack(const map_session_data&,uint8 flag,int32,int32){if(flag==ITEM_EQUIP_ACK_OK)++equip_acks;}
extern "C" void pipeline_unequipack(const map_session_data&,uint16,int32,bool) asm("__wrap__Z19clif_unequipitemackRK16map_session_datatib");
extern "C" void pipeline_unequipack(const map_session_data&,uint16,int32,bool success){if(success)++unequip_acks;}
extern "C" void pipeline_look(block_list*,int32,int32) asm("__wrap__Z15clif_changelookP10block_listii");
extern "C" void pipeline_look(block_list*,int32,int32){}
extern "C" void pipeline_skillblock(const map_session_data&) asm("__wrap__Z19clif_skillinfoblockRK16map_session_data");
extern "C" void pipeline_skillblock(const map_session_data&){}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"input directory");deny_network();static char server[]="combat-pipeline-native";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    check(job_db.load(),"native job metadata loads");
    map_num=1;std::strcpy(map[0].name,"pipeline_test");map[0].initMapFlags();
    // Native map cell checks run on a tiny, controlled passable map.
    std::vector<mapcell> cells(64);for(auto& cell:cells){cell.walkable=1;cell.shootable=1;}
    map[0].xs=map[0].ys=8;map[0].cell=cells.data();
    npc_data fake{};fake.id=NPC;fake.type=BL_NPC;fake_nd=&fake;
    pipeline_parse(std::string(argv[1])+"/items.yml",item_db);
    check(skill_db.load(),"native skill metadata loads");
    check(elemental_attribute_db.load(),"native element matchup table loads");
    pipeline_parse(std::string(argv[1])+"/combos.yml",itemdb_combo);itemdb_combo.loadingFinished();
    pipeline_costume(19961,EQP_COSTUME_HEAD_TOP);pipeline_costume(19962,EQP_COSTUME_HEAD_MID);
    pipeline_costume(19963,EQP_COSTUME_HEAD_LOW);pipeline_costume(19964,EQP_COSTUME_GARMENT);
    auto target=std::make_unique<map_session_data>();target->type=BL_PC;target->id=99000004;
    target->m=0;target->x=4;target->y=4;target->status.class_=JOB_NOVICE;target->status.base_level=1;
    target->battle_status.hp=target->battle_status.max_hp=100000000;
    target->battle_status.size=SZ_MEDIUM;target->battle_status.race=RC_DEMIHUMAN;
    target->battle_status.class_=CLASS_NORMAL;target->battle_status.def_ele=ELE_NEUTRAL;
    target->battle_status.ele_lv=1;target->battle_status.def2=1;target_lookup=target.get();
    auto input=json::parse(read(std::string(argv[1])+"/cases.json"));json results=json::array();json bare;
    for(const auto& test:input) {
        ++cases;errors=0;auto sd=std::make_unique<map_session_data>();attached=sd.get();
        sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
        sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;sd->status.sex=SEX_MALE;
        sd->status.base_level=sd->status.job_level=1;sd->status.inventory_slots=MAX_INVENTORY;
        sd->status.str=sd->status.agi=sd->status.vit=sd->status.int_=sd->status.dex=sd->status.luk=1;
        sd->fd=0;sd->m=0;sd->x=3;sd->y=4;sd->state.connect_new=true;sd->state.ignoretimeout=true;
        sd->npc_idle_timer=INVALID_TIMER;
        for(auto& index:sd->equip_index)index=-1;
        for(auto& index:sd->equip_switch_index)index=-1;
        const char* keys[]={"upper","middle","lower","garment"};
        const uint32 positions[]={EQP_COSTUME_HEAD_TOP,EQP_COSTUME_HEAD_MID,EQP_COSTUME_HEAD_LOW,EQP_COSTUME_GARMENT};
        for(int n=0;n<4;++n)pipeline_put(n,19961+n,test.value(keys[n],json::array()));
        status_calc_pc(sd.get(),SCO_FIRST);
        const auto initial=pipeline_snapshot(*sd,*target);
        if(cases==1)bare=initial;else check(initial==bare,"unequipped items have no combat effect");
        const auto equip_before=equip_acks,unequip_before=unequip_acks;
        for(int n=0;n<4;++n)check(pc_equipitem(sd.get(),n,positions[n],false),"actual equip function accepts valid carrier");
        check(equip_acks-equip_before==4,"actual equips emit four success acknowledgments at transport boundary");
        auto equipped=pipeline_snapshot(*sd,*target);pipeline_expect(equipped,test["expected"]);
        if(cases==1)bare=equipped;
        for(int repeat=0;repeat<25;++repeat) {
            const auto before=pipeline_snapshot(*sd,*target);check(before==equipped,"final damage and timing are deterministic");
            // Real removal discovers/removes combos and recomputes status.
            for(int n=3;n>=0;--n)check(pc_unequipitem(sd.get(),n,3),"actual unequip completes");
            check(pipeline_snapshot(*sd,*target)==bare,"real unequip restores bare final damage and timing");
            for(int n=0;n<4;++n)check(pc_equipitem(sd.get(),n,positions[n],false),"actual re-equip completes");
            check(pipeline_snapshot(*sd,*target)==equipped,"re-equip restores exact final damage and timing without accumulation");
        }
        check(unequip_acks-unequip_before==100,"actual repeated removals acknowledge every transition");
        equipped["name"]=test["name"];results.push_back(equipped);
        check(!errors&&!sd->st,"native combat and equip transitions finish without script errors");attached=nullptr;
    }
    std::ofstream(std::string(argv[1])+"/results.json")<<results.dump(2)<<"\n";
    target_lookup=nullptr;target.reset();fake_nd=nullptr;map[0].cell=nullptr;
    itemdb_combo.clear();item_db.clear();skill_db.clear();job_db.clear();elemental_attribute_db.clear();
    do_final_script();timer_final();db_final();malloc_final();
    std::printf("COMBAT_PIPELINE_NATIVE_OK cases=%u assertions=%u equip_acks=%u unequip_acks=%u\n",cases,assertions,equip_acks,unequip_acks);
    return 0;
}
