// Native VM and pc_bonus registration; explicit player/equipment/world fixture.
#include "map/elemental.hpp"
#include "map/skill.hpp"
#include <nlohmann/json.hpp>
using nlohmann::json;
extern int16 current_equip_item_index;

extern "C" int __wrap_main(int argc, char** argv) {
    check(argc == 2, "input directory");
    deny_network();
    static char server[] = "dimension-equipment-test";
    SERVER_NAME = server;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script(); battle_set_defaults();
    num_reg_ers = ers_new(sizeof(script_reg_num), "dimension:num", (ERSOptions)(ERS_OPT_CLEAN | ERS_OPT_FLEX_CHUNK));
    str_reg_ers = ers_new(sizeof(script_reg_str), "dimension:str", (ERSOptions)(ERS_OPT_CLEAN | ERS_OPT_FLEX_CHUNK));
    pc_set_reg_load(false);
    auto input = json::parse(read(std::string(argv[1]) + "/input.json"));
    for (auto it = input["items"].begin(); it != input["items"].end(); ++it) {
        auto data = std::make_shared<item_data>();
        data->nameid = std::stoi(it.key());
        data->type = it.value()["Type"] == "Weapon" ? IT_WEAPON : IT_ARMOR;
        data->weapon_level = it.value().value("WeaponLevel", 0);
        item_db.put(data->nameid, data);
    }
    auto raw = read(std::string(argv[1]) + "/skills.yml");
    auto tree = ryml::parse_in_arena(ryml::to_csubstr(raw));
    for (auto row : tree["Body"]) check(skill_db.parseBodyNode(row) == 1, "production skill metadata");
    unsigned tested = 0;
    for (const auto& profile : input["profiles"])
    for (int summon : {20817, 0, 20816, 20818, 20819, 20820, 2114})
    for (int learned : {10, 1, 0})
    for (int grade : {ENCHANTGRADE_A, ENCHANTGRADE_B})
    for (int crown_grade : {ENCHANTGRADE_A, ENCHANTGRADE_B})
    for (int refine : {11, 12}) {
        auto sd = rune_player();
        sd->status.class_ = JOB_ELEMENTAL_MASTER;
        sd->status.base_level = 275;
        sd->status.skill[skill_get_index(EM_ELEMENTAL_BUSTER)].id = EM_ELEMENTAL_BUSTER;
        sd->status.skill[skill_get_index(EM_ELEMENTAL_BUSTER)].lv = learned;
        s_elemental_data elemental;
        elemental.elemental.class_ = summon;
        sd->ed = summon ? &elemental : nullptr;
        int weapon = profile["weapon"];
        for (int slot : {EQI_HAND_R, EQI_HEAD_TOP}) {
            int id = slot == EQI_HAND_R ? weapon : 400536;
            sd->equip_index[slot] = slot;
            auto& item = sd->inventory.u.items_inventory[slot];
            item.nameid = id; item.amount = 1; item.refine = refine;
            item.enchantgrade = slot == EQI_HAND_R ? grade : crown_grade;
            sd->inventory_data[slot] = item_db.find(id).get();
        }
        for (const auto& entry : profile["scripts"]) {
            current_equip_item_index = EQI_HAND_R;
            auto* code = compile("{ " + entry["script"].get<std::string>() + " }", "production EM gear script");
            run_script(code, 0, sd->id, NPC);
            check(!sd->st && !errors, "equipment VM completed");
            script_free_code(code);
        }
        // Arrival of a summon must not need re-equipping to enable a gear proc.
        // The execution test separately checks the current summon requirement.
        elemental.elemental.class_ = ELEMENTALID_DILUVIO;
        sd->ed = &elemental;
        bool eligible = grade == ENCHANTGRADE_A && learned >= (weapon == 540114 ? 10 : 1)
            && (weapon == 540114 ? refine >= 12 : crown_grade == ENCHANTGRADE_A);
        unsigned expected = eligible
            ? (weapon == 540114 ? 2 : 1) : 0;
        if (sd->autospell3.size() != expected) {
            std::fprintf(stderr, "REGISTRATION_FAIL weapon=%d equipped_summon=%d learned=%d grade=%d expected=%u actual=%zu\n",
                weapon, summon, learned, grade, expected, sd->autospell3.size());
        }
        check(sd->autospell3.size() == expected, "Buster proc available after summon without re-equipping");
        std::vector<uint16> actual_triggers;
        for (const auto& proc : sd->autospell3) {
            actual_triggers.push_back(proc.trigger_skill);
            check(proc.id == EM_ELEMENTAL_BUSTER && proc.lv == learned && proc.rate == 1000,
                "learned-level Buster at 100 percent");
            check(proc.flag & AUTOSPELL_FORCE_TARGET, "ground-skill proc targets caster");
            check(proc.trigger_skill == (weapon == 540080 ? EM_TERRA_DRIVE : EM_DIAMOND_STORM)
                || (weapon == 540114 && proc.trigger_skill == EM_TERRA_DRIVE), "correct weapon trigger");
        }
        std::vector<uint16> expected_triggers;
        if (expected) {
            expected_triggers.push_back(weapon == 540080 ? EM_TERRA_DRIVE : EM_DIAMOND_STORM);
            if (weapon == 540114) expected_triggers.push_back(EM_TERRA_DRIVE);
        }
        std::sort(actual_triggers.begin(), actual_triggers.end());
        std::sort(expected_triggers.begin(), expected_triggers.end());
        check(actual_triggers == expected_triggers, "every advertised trigger registered exactly once");
        sd->ed = nullptr;
        ++tested;
    }
    for (const auto& profile : input["other_profiles"])
    for (int mounted : {0, 1})
    for (int learned : {0, 1, 5})
    for (int grade : {ENCHANTGRADE_A, ENCHANTGRADE_B})
    for (int crown_grade : {ENCHANTGRADE_A, ENCHANTGRADE_B}) {
        auto sd = rune_player();
        bool performer = profile["kind"] == "performer";
        uint16 skill = performer ? WM_REVERBERATION : NC_AXETORNADO;
        sd->status.class_ = performer ? JOB_TROUBADOUR : JOB_MEISTER;
        sd->sc.option = mounted ? OPTION_MADOGEAR : 0;
        sd->status.skill[skill_get_index(skill)].id = skill;
        sd->status.skill[skill_get_index(skill)].lv = learned;
        for (int slot : {EQI_HAND_R, EQI_HEAD_TOP}) {
            int id = profile[slot == EQI_HAND_R ? "weapon" : "crown"];
            sd->equip_index[slot] = slot;
            auto& item = sd->inventory.u.items_inventory[slot];
            item.nameid = id; item.amount = 1; item.refine = slot == EQI_HAND_R ? 12 : 11;
            item.enchantgrade = slot == EQI_HAND_R ? grade : crown_grade;
            sd->inventory_data[slot] = item_db.find(id).get();
        }
        for (const auto& entry : profile["scripts"]) {
            auto* code = compile("{ " + entry["script"].get<std::string>() + " }", "production other job crown combo");
            run_script(code, 0, sd->id, NPC);
            check(!sd->st && !errors, "other job equipment VM completed");
            script_free_code(code);
        }
        bool eligible = grade == ENCHANTGRADE_A && crown_grade == ENCHANTGRADE_A;
        unsigned expected = eligible && learned > 0 ? 1 : 0;
        if (sd->autospell3.size() != expected)
            std::fprintf(stderr, "OTHER_REGISTRATION_FAIL weapon=%d mounted=%d learned=%d expected=%u actual=%zu\n",
                profile["weapon"].get<int>(), mounted, learned, expected, sd->autospell3.size());
        check(sd->autospell3.size() == expected, "other job proc respects grade/learning and works mounted");
        int expected_bonus = performer ? 45 + (eligible ? 2 * (12 + 11) : 0) : (eligible ? 5 * (12 + 11) : 0);
        int actual_bonus = pc_skillatk_bonus(sd.get(), skill);
        if (actual_bonus != expected_bonus)
            std::fprintf(stderr, "OTHER_BONUS_FAIL weapon=%d expected=%d actual=%d\n",
                profile["weapon"].get<int>(), expected_bonus, actual_bonus);
        check(actual_bonus == expected_bonus, "official combined-refine skill damage bonus");
        for (const auto& proc : sd->autospell3)
            check(proc.id == skill && proc.lv == learned && proc.rate == 1000
                && proc.trigger_skill == (performer ? TR_METALIC_FURY : MT_MIGHTY_SMASH),
                "other job exact trigger and learned level");
        ++tested;
    }
    current_equip_item_index = -1; attached = nullptr;
    skill_db.clear(); item_db.clear(); do_final_script();
    ers_destroy(num_reg_ers); ers_destroy(str_reg_ers); num_reg_ers = str_reg_ers = nullptr;
    timer_final(); db_final(); malloc_final();
    std::printf("DIMENSION_EQUIPMENT_OK cases=%u assertions=%u\n", tested, assertions);
    return 0;
}
