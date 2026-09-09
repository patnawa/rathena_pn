// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  druid_shadow166_native_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_shadow166_native_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Appended to the existing isolated VM fixture's boundary
// doubles by druid_shadow166_enchant_test.py. No live player/server/network.
#include "map/skill.hpp"
#include "map/skills/skill_impl.hpp"

namespace {
// Exact pre-fix predicate from 53df51fe3:src/map/pc.cpp, renamed only.
// Retained as an independent oracle, not generated from the new implementation.
static bool original_pc_isItemClass ( const map_session_data* sd, const item_data* item ) {
	while (1) {
		if (item->class_upper&ITEMJ_NORMAL && !(sd->class_&(JOBL_UPPER|JOBL_BABY|JOBL_THIRD|JOBL_FOURTH)))	//normal classes (no upper, no baby, no third, no fourth)
			break;
#ifndef RENEWAL
		//allow third classes to use trans. class items
		if (item->class_upper&ITEMJ_UPPER && sd->class_&(JOBL_UPPER|JOBL_THIRD))	//trans. classes
			break;
		//third-baby classes can use same item too
		if (item->class_upper&ITEMJ_BABY && sd->class_&JOBL_BABY)	//baby classes
			break;
		//don't need to decide specific rules for third-classes?
		//items for third classes can be used for all third classes
		if (item->class_upper&(ITEMJ_THIRD|ITEMJ_THIRD_UPPER|ITEMJ_THIRD_BABY) && sd->class_&JOBL_THIRD)
			break;
#else
		//trans. classes (exl. third-trans.)
		if (item->class_upper&ITEMJ_UPPER && sd->class_&JOBL_UPPER && !(sd->class_&JOBL_THIRD))
			break;
		//baby classes (exl. third-baby)
		if (item->class_upper&ITEMJ_BABY && sd->class_&JOBL_BABY && !(sd->class_&JOBL_THIRD))
			break;
		//third classes (exl. third-trans. and baby-third and fourth)
		if (item->class_upper&ITEMJ_THIRD && sd->class_&JOBL_THIRD && !(sd->class_&(JOBL_UPPER|JOBL_BABY)) && !(sd->class_&JOBL_FOURTH))
			break;
		//trans-third classes (exl. fourth)
		if (item->class_upper&ITEMJ_THIRD_UPPER && sd->class_&JOBL_THIRD && sd->class_&JOBL_UPPER && !(sd->class_&JOBL_FOURTH))
			break;
		//third-baby classes (exl. fourth)
		if (item->class_upper&ITEMJ_THIRD_BABY && sd->class_&JOBL_THIRD && sd->class_&JOBL_BABY && !(sd->class_&JOBL_FOURTH))
			break;
		//fourth classes
		if (item->class_upper&ITEMJ_FOURTH && sd->class_&JOBL_FOURTH)
			break;
#endif
		return false;
	}
	return true;
}
ryml::Tree read_yaml(const char* path) {
    std::ifstream file(path);
    check(file.good(), "YAML fixture opens");
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return ryml::parse_in_arena(ryml::to_csubstr(text));
}
void reset_effects(map_session_data& player) {
    player.skillatk.clear();
    player.bonus.hp = 0;
    player.base_status.patk = player.base_status.smatk = 0;
    std::memset(&player.indexed_bonus, 0, sizeof(player.indexed_bonus));
}
void run_checked(script_code* code, map_session_data& player) {
    check(code != nullptr, "actual item/combo parser produced bytecode");
    run_script(code, 0, player.id, 0);
    check(errors == 0 && player.st == nullptr, "actual VM finishes without errors");
}
void check_race(const int32* rates, int amount) {
    for (int race = 0; race < RC_MAX; ++race) {
        int expected = race == RC_ALL ? amount :
            (race == RC_PLAYER_HUMAN || race == RC_PLAYER_DORAM ? -amount : 0);
        check(rates[race] == expected, "exact race array including both player cancellations");
        if (race != RC_ALL)
            check(rates[RC_ALL] + rates[race] ==
                (race == RC_PLAYER_HUMAN || race == RC_PLAYER_DORAM ? 0 : amount),
                "effective all-races bonus excludes human and Doram players");
    }
}
}

extern "C" int __wrap_main(int argc, char** argv) {
    deny_network();
    check(argc == 2, "one generated actual-record dependency fixture required");
    static char server_name[] = "druid-shadow166-native-test";
    SERVER_NAME = server_name;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    check(errors == 0, "script subsystem initializes");
    const std::pair<int, const char*> skills[] = {
        {6580,"AT_ALPHA_CLAW"}, {6582,"AT_FRENZY_FANG"}, {6586,"AT_PINION_SHOT"},
        {6588,"AT_QUILL_SPEAR"}, {6594,"AT_GLACIER_SHARD"},
        {6597,"AT_ROARING_PIERCER"}, {6603,"AT_TERRA_HARVEST"},
        {6589,"AT_QUILL_SPEAR_S"}, {6598,"AT_ROARING_PIERCER_S"}
    };
    for (const auto& [id, name] : skills) {
        auto skill = std::make_shared<s_skill_db>();
        skill->nameid = id; std::strcpy(skill->name, name); skill_db.put(id, skill);
    }
    auto dependencies = read_yaml(argv[1]);
    for (auto node : dependencies["Body"])
        check(item_db.parseBodyNode(node) == 1, "actual baseline dependency parses");
    // item_data owns script pointers: snapshot only observed non-owning fields,
    // never copy item_data (its destructor would free the same scripts twice).
    struct MasterSnapshot { uint64 class_base[3]; int class_upper, equip, elv; script_code* script; };
    MasterSnapshot master_before[2]{};
    for (int i = 0; i < 2; ++i) {
        const auto data = item_db.find(24792 + i);
        std::copy(std::begin(data->class_base), std::end(data->class_base), master_before[i].class_base);
        master_before[i].class_upper = data->class_upper;
        master_before[i].equip = data->equip; master_before[i].elv = data->elv;
        master_before[i].script = data->script;
    }
    auto items = read_yaml("db/import/druid_shadow166_items.yml");
    for (auto node : items["Body"])
        check(item_db.parseBodyNode(node) == 1, "actual overlay item/partial override parses");
    item_db.loadingFinished();
    auto player = std::make_unique<map_session_data>();
    attached = player.get();
    player->id = player->status.account_id = 99000001;
    player->status.char_id = 99000002; player->type = BL_PC;
    player->state.ignoretimeout = true; player->npc_idle_timer = INVALID_TIMER;
    player->state.lr_flag = LR_FLAG_NONE;
    player->status.base_level = 200; player->class_ = MAPID_ALITEA;
    battle_config.allow_equip_restricted_item = 1;
    battle_config.atcommand_disable_npc = 0;
    check(player->permissions.none() && !pc_has_permission(player.get(), PC_PERM_USE_ALL_EQUIPMENT),
          "no GM equipment permission bypass");
    // Every valid native job and all seven-bit class masks. Normal job-mask,
    // level, sex and condition checks remain active; map restrictions alone are
    // bypassed because there is no loaded map in this isolated test.
    item_data matrix_item{};
    matrix_item.type = IT_SHADOWGEAR; matrix_item.equip = EQP_SHADOW_ARMOR;
    matrix_item.sex = SEX_BOTH;
    for (auto& bank : matrix_item.class_base) bank = ~0ULL;
    player->inventory_data[0] = &matrix_item;
    player->status.base_level = 275;
    int matrix_cases = 0, new_cases = 0, valid_jobs = 0;
    for (int job = 0; job < JOB_MAX; ++job) {
        uint64 mapped = pc_jobid2mapid(job);
        if (mapped == static_cast<uint64>(-1)) continue;
        ++valid_jobs;
        player->class_ = mapped;
        for (int mask = 0; mask < 128; ++mask) {
            matrix_item.class_upper = mask;
            bool old = original_pc_isItemClass(player.get(), &matrix_item);
            bool expected = old || ((mask & ITEMJ_FOURTH) && pc_is_trait_job(mapped));
            bool actual = pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_OK;
            check(actual == expected, "all-job/all-class-mask actual equip matches pinned old plus trait-era rule");
            check(!old || actual, "no formerly allowed job/class-mask loses eligibility");
            if (actual && !old) {
                check((mask & ITEMJ_FOURTH) && pc_is_upper_expanded_second(mapped) && !(mapped & JOBL_FOURTH),
                      "new eligibility restricted to trait-era expanded jobs without Fourth bit");
                ++new_cases;
            }
            ++matrix_cases;
        }
    }
    check(valid_jobs > 100 && matrix_cases == valid_jobs * 128 && new_cases > 0,
          "broad class matrix executes real supported jobs, not malformed IDs");
    player->class_ = MAPID_ALITEA;
    matrix_item.class_upper = ITEMJ_FOURTH;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_OK, "positive matrix control");
    matrix_item.elv = 276;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAILLEVEL, "minimum level still enforced");
    matrix_item.elv = 0; matrix_item.elvmax = 274;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAILLEVEL, "maximum level still enforced");
    matrix_item.elvmax = 0; matrix_item.sex = SEX_MALE; player->status.sex = SEX_FEMALE;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAIL, "sex restriction still enforced");
    matrix_item.sex = SEX_BOTH; player->inventory.u.items_inventory[0].attribute = 1;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAIL, "broken equipment still rejected");
    player->inventory.u.items_inventory[0].attribute = 0;
    matrix_item.class_base[1] = 0;
    check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAIL, "trait predicate cannot bypass Jobs filter");
    player->status.base_level = 200;
    const uint64 alitea_bit = 1ULL << (MAPID_ALITEA & MAPID_FIRSTMASK);
    const int locations[] = {EQP_SHADOW_ARMOR, EQP_SHADOW_SHOES, EQP_SHADOW_ACC_R, EQP_SHADOW_ACC_L};
    for (int i = 0; i < 4; ++i) {
        auto data = item_db.find(1270183 + i);
        check(data && data->type == IT_SHADOWGEAR && data->equip == locations[i], "exact shadow type/location");
        check(data->weight == 0 && data->slots == 0 && data->elv == 200 && !data->flag.no_refine,
              "exact weight/slots/minimum/refineable");
        check(data->class_base[0] == 0 && data->class_base[1] == alitea_bit && data->class_base[2] == 0,
              "Alitea parsed in second-1 job mask, not base Druid mask");
        check(data->class_upper == ITEMJ_FOURTH, "trait-era item class filter");
        player->inventory_data[0] = data.get();
        player->inventory.u.items_inventory[0].nameid = data->nameid;
        player->class_ = MAPID_ALITEA; player->status.base_level = 200;
        check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_OK, "actual Alitea can equip target");
        player->status.base_level = 199;
        check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_FAILLEVEL, "level 199 rejected");
        player->status.base_level = 200;
        for (auto job : {MAPID_DRUID, MAPID_KARNOS, MAPID_DRAGON_KNIGHT, MAPID_RUNE_KNIGHT}) {
            player->class_ = job;
            check(pc_isequip(player.get(), 0) != ITEM_EQUIP_ACK_OK, "unrelated and earlier jobs rejected");
        }
        player->class_ = MAPID_ALITEA;
        current_equip_item_index = 0;
        for (int refine = 0; refine <= 10; ++refine) {
            reset_effects(*player);
            player->inventory.u.items_inventory[0].refine = refine;
            player->inventory.u.items_inventory[1].refine = 10 - refine;
            run_checked(data->script, *player);
            check(player->bonus.hp == refine * 10, "generic shadow HP uses current host refine exactly once");
        }
    }
    for (int i = 0; i < 2; ++i) {
        auto data = item_db.find(24792 + i);
        for (int mask = 0; mask < 3; ++mask)
            check(data->class_base[mask] == (master_before[i].class_base[mask] | (mask == 1 ? alitea_bit : 0)),
                  "Master partner job mask preserves every old bit and adds only Alitea");
        check(data->class_upper == master_before[i].class_upper && data->equip == master_before[i].equip &&
              data->elv == master_before[i].elv && data->script == master_before[i].script,
              "Master class/location/level/script unchanged by partial override");
        player->inventory_data[0] = data.get();
        player->inventory.u.items_inventory[0].nameid = data->nameid;
        check(pc_isequip(player.get(), 0) == ITEM_EQUIP_ACK_OK, "Alitea can actually equip required Master partner");
    }
    for (int i = 0; i < 7; ++i) {
        auto data = item_db.find(314804 + i);
        check(data && data->type == IT_CARD && data->subtype == CARD_ENCHANT, "Soul is a native enchant card");
        for (int refine = 0; refine <= 10; ++refine) {
            reset_effects(*player);
            player->inventory.u.items_inventory[0].refine = refine;
            player->inventory.u.items_inventory[1].refine = 10 - refine;
            for (int copies = 1; copies <= 2; ++copies) {
                run_checked(data->script, *player);
                check(player->skillatk.size() == 1 && player->skillatk[0].id == skills[i].first,
                      "Soul registers only exact canonical skill");
                check(pc_skillatk_bonus(player.get(), skills[i].first) == (2 + refine / 2) * copies,
                      "Soul bonus is cumulative and floors host refine halves");
                for (int other = 0; other < 7; ++other)
                    if (i != other) check(pc_skillatk_bonus(player.get(), skills[other].first) == 0,
                                         "unrelated skills unchanged");
                if (i == 3 || i == 5)
                    check(pc_skillatk_bonus(player.get(), i == 3 ? 6589 : 6598) == (2 + refine / 2) * copies,
                          "enhanced skill inherits parent bonus exactly once");
            }
        }
    }
    auto combos = read_yaml("db/import/druid_shadow166_combos.yml");
    int combo_count = 0;
    for (auto node : combos["Body"]) combo_count += itemdb_combo.parseBodyNode(node);
    check(combo_count == 7 && errors == 0, "all seven exact sets accepted by real combo parser");
    itemdb_combo.loadingFinished();
    std::fill(std::begin(player->equip_index), std::end(player->equip_index), -1);
    const int slots[] = {EQI_SHADOW_WEAPON, EQI_SHADOW_SHIELD, EQI_SHADOW_ARMOR,
                         EQI_SHADOW_SHOES, EQI_SHADOW_ACC_R, EQI_SHADOW_ACC_L};
    const int ids[] = {24792,24793,1270183,1270184,1270185,1270186};
    for (int i = 0; i < 6; ++i) {
        player->equip_index[slots[i]] = i;
        player->inventory_data[i] = item_db.find(ids[i]).get();
        player->inventory.u.items_inventory[i].nameid = ids[i];
    }
    for (const auto& entry : itemdb_combo) {
        const auto& combo = entry.second;
        if (combo->nameid.size() == 3) {
            // Exhaustive normal-range refinements of each required three-piece
            // set. Other slots use +10 so an accidental wrong slot is observable.
            for (int a = 0; a <= 10; ++a) for (int b = 0; b <= 10; ++b) for (int c = 0; c <= 10; ++c) {
                reset_effects(*player);
                for (int i = 0; i < 6; ++i) player->inventory.u.items_inventory[i].refine = 10;
                const int levels[] = {a,b,c};
                for (int j = 0; j < 3; ++j) for (int i = 0; i < 6; ++i)
                    if (ids[i] == combo->nameid[j]) player->inventory.u.items_inventory[i].refine = levels[j];
                run_checked(combo->script, *player);
                check(player->base_status.patk == 1 && player->base_status.smatk == 1, "three-piece PAtk/SMatk unconditional");
                check_race(player->indexed_bonus.ignore_def_by_race, a+b+c >= 27 ? 50 : 0);
                check_race(player->indexed_bonus.ignore_mdef_by_race, a+b+c >= 27 ? 50 : 0);
            }
        } else {
            reset_effects(*player); run_checked(combo->script, *player);
            if (combo->nameid.size() == 2) {
                for (int stat = PARAM_STR; stat < PARAM_MAX; ++stat)
                    check(player->indexed_bonus.param_bonus[stat] == (stat >= PARAM_POW ? 2 : 0),
                          "all six trait stats and no basic stats");
            } else {
                check(combo->nameid.size() == 6, "only the sourced full six-piece combo");
                check_race(player->indexed_bonus.ignore_res_by_race, 20);
                check_race(player->indexed_bonus.ignore_mres_by_race, 20);
            }
        }
    }
    // Native additive registration when both three-piece sets and all pair sets
    // are present. Membership itself is validated through the actual combo DB.
    reset_effects(*player);
    for (int i = 0; i < 6; ++i) player->inventory.u.items_inventory[i].refine = 9;
    for (const auto& entry : itemdb_combo) run_checked(entry.second->script, *player);
    check(player->base_status.patk == 2 && player->base_status.smatk == 2, "both three-piece bonuses stack");
    for (int stat = PARAM_POW; stat < PARAM_MAX; ++stat)
        check(player->indexed_bonus.param_bonus[stat] == 8, "four two-piece trait bonuses stack");
    check_race(player->indexed_bonus.ignore_def_by_race, 100);
    check_race(player->indexed_bonus.ignore_mdef_by_race, 100);
    check_race(player->indexed_bonus.ignore_res_by_race, 20);
    check_race(player->indexed_bonus.ignore_mres_by_race, 20);
    auto recipes = read_yaml("db/import/druid_shadow166_enchants.yml");
    check(item_enchant_db.parseBodyNode(recipes["Body"][0]) == 1, "real enchant DB accepts sourced group");
    auto group = item_enchant_db.find(166);
    check(group && group->target_item_ids.size() == 4 && group->order == std::vector<uint16>({3,2}), "native targets and order");
    check(group->reset.chance == 0 && group->reset.zeny == 0 && group->reset.materials.empty(), "reset disabled, no dormant cost");
    check(group->minimumRefine == 0 && group->minimumEnchantgrade == 0 && group->allowRandomOptions, "native eligibility");
    int recipe_count = 0;
    for (const auto& [slot_id, slot] : group->slots) {
        check(slot->normal.enchants.empty() && slot->upgrade.enchants.empty() && slot->perfect_upgrades.empty(), "no invented random or upgrades");
        for (const auto& [id, recipe] : slot->perfect.enchants) {
            int amount = slot_id == 3 ? 1 : (id >= 314804 && id <= 314810 ? 5 : 3);
            check(recipe->zeny == 0 && recipe->materials.size() == 1 && recipe->materials.at(1001253) == amount,
                  "actual perfect recipe resolves exact essence amount and zero Zeny");
            ++recipe_count;
        }
    }
    check(recipe_count == 18, "all eighteen native perfect recipes");
    check(added == 0 && removed == 0 && logged.empty(), "effect proof does not mutate inventory or claim charging proof");
    current_equip_item_index = -1;
    attached = nullptr; player.reset(); skill_db.clear(); item_enchant_db.clear(); itemdb_combo.clear(); item_db.clear();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("DRUID_SHADOW166_NATIVE_COMPLETE: 2662 three-piece refine cases; %d native assertions\n", assertions);
    std::printf("DRUID_SHADOW166_EQUIP_MATRIX: %d supported jobs; %d masks; %d newly eligible combinations\n",
                valid_jobs, matrix_cases, new_cases);
    return errors ? 1 : 0;
}
