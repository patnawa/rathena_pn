#!/usr/bin/env python3
"""Druid Gear identity, reference and actual native script-VM regressions.

The optional --native-vm probe requires Linux/WSL and existing map-server support
objects. It freshly compiles script.cpp, pc.cpp, skill.cpp, clif.cpp and malloc.cpp under
ASan/UBSan, using the existing isolated VM harness's world-boundary doubles and
mandatory socket/connect/bind/listen denial. It never starts a server.

External client/reference tests explicitly skip unless paths are supplied.
--require-import checks the final shared item import after integration.
"""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

import yaml

from audit_enchant_upgrades import renewal_records, server_recipes
from audit_initial_enchants import client_configuration, server_configuration
from chapter2_client_helper_test import windows_path
from lua51_literal_table import literal_tables
from run_native_script_vm_test import WRAPPERS


ROOT = Path(__file__).resolve().parents[2]
ITEM_PATH = 'db/import/druid_gear_enchants.yml'
RECIPE_PATH = ROOT / 'db/import/druid_item_enchant.yml'
FRAGMENT = ROOT / 'client-patch/druid_gear/SystemEN/itemInfo_DruidGear.lua'
EXPECTED = {314269: ('Gear_AT2', 'AT_PINION_SHOT', 6586),
            314270: ('Gear_AT1', 'AT_QUILL_SPEAR', 6588)}
REFERENCE_HASH = '235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a'
CLIENT_HASH = '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d'
NAMES_HASH = '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496'
OPTIONS = argparse.Namespace(native_vm=False, require_import=False,
                             client=None, client_item_names=None, effect_reference=None,
                             lua=None, client_root=ROOT.parent.parent)


def recipe_baseline():
    """Read effective settings without the shared Druid overlay's group 24 body.

    Suppress the whole group-24 record, so unexpected target/reset/normal-price
    fields cannot disappear into the reconstructed baseline.
    """
    read_text = Path.read_text

    def without_group24(path, *args, **kwargs):
        text = read_text(path, *args, **kwargs)
        if path.resolve() == RECIPE_PATH.resolve():
            data = yaml.safe_load(text)
            data['Body'] = [r for r in data['Body'] if r['Id'] != 24]
            return yaml.safe_dump(data)
        return text

    with patch.object(Path, 'read_text', without_group24):
        return server_configuration(ROOT), server_recipes(ROOT)


def run_lua_fragment(items):
    # Run the actual Lua fragment and the active client's unmodified merge
    # function on synthetic base tables. This does not alter the active loader.
    source = r'''
assert(_VERSION == "Lua 5.1", "Lua 5.1 runtime required")
os.execute = nil
io.popen = nil
MessageBox = function(message) error(message) end
require("SystemEN/LuaFiles514/rotp_f")
local returned = dofile(FRAGMENT_PATH)
assert(returned == tbl_druidgear and type(returned) == "table")
local sentinel = {identifiedDisplayName = "Unrelated existing item"}
tbl = {[501] = sentinel}
F_itemInfoMerge(tbl_druidgear)
assert(tbl[501] == sentinel, "existing unrelated record must survive")
local count = 0
for id, info in pairs(tbl_druidgear) do
    assert(id == 314269 or id == 314270, "unexpected fragment identity")
    assert(tbl[id] == info, "actual merge must install the fragment record")
    assert(info.identifiedResourceName == "EpisodClear20")
    assert(info.unidentifiedResourceName == "EpisodClear20")
    assert(info.slotCount == 0 and info.ClassNum == 0 and info.costume == false)
    assert(#info.unidentifiedDescriptionName == 1 and info.unidentifiedDescriptionName[1] == "")
    count = count + 1
end
assert(count == 2)
'''
    for item_id, skill in ((314269, 'Pinion Shot'), (314270, 'Quill Spear')):
        name = json.dumps(items[item_id]['Name'])
        expected = [f'{skill} damage +5%.', f'Grade D or higher: an additional +3% {skill} damage.',
                    'Grade C or higher: physical damage against all sizes +10%.',
                    f'Grade B or higher: an additional +4% {skill} damage.',
                    f'Grade A or higher: an additional +6% {skill} damage.',
                    'Grade bonuses are cumulative.', '_' * 23,
                    '^0000CCType:^000000 Enchant', '^0000CCWeight:^000000 0']
        source += f'assert(tbl[{item_id}].identifiedDisplayName == {name})\n'
        source += f'assert(tbl[{item_id}].unidentifiedDisplayName == {name})\n'
        source += f'assert(#tbl[{item_id}].identifiedDescriptionName == {len(expected)})\n'
        for index, description in enumerate(expected, 1):
            source += f'assert(tbl[{item_id}].identifiedDescriptionName[{index}] == {json.dumps(description)})\n'
    source += r'''
local original = tbl[314269]
F_itemInfoMerge(tbl_druidgear)
assert(tbl[314269] == original and tbl[501] == sentinel, "repeated merge is idempotent")
tbl = {[314269] = sentinel}
F_itemInfoMerge(tbl_druidgear)
assert(tbl[314269] == sentinel, "default merge must retain an existing same-ID record")
assert(tbl[314270] == tbl_druidgear[314270])
print("DRUID_GEAR_LUA_COMPLETE: two exact records, descriptions and native merge contract")
'''
    source = 'FRAGMENT_PATH = ' + json.dumps(windows_path(FRAGMENT)) + '\n' + source
    result = subprocess.run([str(OPTIONS.lua.resolve()), '-'], cwd=OPTIONS.client_root,
                            input=source.encode('ascii'), capture_output=True, timeout=30)
    if result.returncode:
        raise AssertionError(result.stderr.decode('cp949', errors='replace'))
    return result.stdout.decode('ascii').strip()


def native_driver(items):
    # Reuse only the isolated fixture's existing initialization helpers and
    # explicit world boundaries; replace its inventory-mutation test entry point.
    prefix = (ROOT / 'tools/ci/native_script_vm_test.cpp').read_text().split(
        'extern "C" int __wrap_main(int, char**) {', 1)[0]
    source = prefix + '\n#include "map/skill.hpp"\n#include "map/skills/skill_impl.hpp"\n'
    source += r'''
extern "C" int __wrap_main(int, char**) {
    deny_network();
    static char test_server_name[] = "druid-gear-script-vm-test";
    SERVER_NAME = test_server_name;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    check(errors == 0, "script subsystem initializes");
    auto player = std::make_unique<map_session_data>();
    attached = player.get();
    player->id = 99000001;
    player->type = BL_PC;
    player->status.account_id = 99000001;
    player->status.char_id = 99000002;
    player->state.ignoretimeout = true;
    player->npc_idle_timer = INVALID_TIMER;
    player->state.lr_flag = LR_FLAG_NONE;
    battle_config.atcommand_disable_npc = 0;
    player->inventory.u.items_inventory[0].nameid = 490136;
    player->inventory.u.items_inventory[1].nameid = 490136;
    current_equip_item_index = 0;
    static_assert(AT_PINION_SHOT == 6586 && AT_QUILL_SPEAR == 6588 && AT_QUILL_SPEAR_S == 6589);
    static_assert(ENCHANTGRADE_NONE == 0 && ENCHANTGRADE_D == 1 && ENCHANTGRADE_C == 2 &&
                  ENCHANTGRADE_B == 3 && ENCHANTGRADE_A == 4);
    const int expected_skill[] = {5, 8, 8, 12, 18};
    const int expected_size[] = {0, 0, 10, 10, 10};
'''
    for skill_id, name in ((6586, 'AT_PINION_SHOT'), (6588, 'AT_QUILL_SPEAR'),
                           (6589, 'AT_QUILL_SPEAR_S')):
        source += ('{ auto skill = std::make_shared<s_skill_db>(); '
                   f'skill->nameid = {skill_id}; std::strcpy(skill->name, "{name}"); '
                   'skill_db.put(skill->nameid, skill); }\n')
    for item_id, item in items.items():
        skill = EXPECTED[item_id][2]
        body = json.dumps('{\n' + item['Script'] + '\n}', ensure_ascii=True)
        source += f'''{{
    script_code* code = parse_script({body}, "{ITEM_PATH}:{item_id}", 1, 0);
    check(code != nullptr, "actual parser accepts item {item_id}");
    for (int grade = 0; grade <= 4; ++grade) {{
        player->skillatk.clear();
        std::memset(player->right_weapon.addsize, 0, sizeof(player->right_weapon.addsize));
        player->inventory.u.items_inventory[0].enchantgrade = grade;
        player->inventory.u.items_inventory[1].enchantgrade = 4 - grade;
        // Running twice checks the engine's additive registration independently
        // of each single-enchant grade result.
        for (int copies = 1; copies <= 2; ++copies) {{
            run_script(code, 0, player->id, 0);
            check(errors == 0, "actual VM executes item script");
            check(player->st == nullptr, "completed VM detaches player");
            check(player->skillatk.size() == 1, "only one canonical skill registered");
            check(player->skillatk[0].id == {skill}, "exact target skill identity");
            check(pc_skillatk_bonus(player.get(), {skill}) == expected_skill[grade] * copies,
                  "cumulative grade skill bonus");
            check(player->right_weapon.addsize[SZ_ALL] == expected_size[grade] * copies,
                  "cumulative grade physical-size bonus");
            for (int size = 0; size < SZ_ALL; ++size)
                check(player->right_weapon.addsize[size] == 0, "no duplicate individual-size bonus");
            check(pc_skillatk_bonus(player.get(), {6588 if skill == 6586 else 6586}) == 0,
                  "unrelated skill receives no skill bonus");
'''
        if skill == 6588:
            source += '''            check(pc_skillatk_bonus(player.get(), AT_QUILL_SPEAR_S) == expected_skill[grade] * copies,
                  "enhanced Quill Spear receives parent bonus exactly once");
'''
        source += '''        }
    }
    script_free_code(code);
}
'''
    source += r'''
    check(added == 0 && removed == 0 && logged.empty(), "no inventory mutation or logging");
    current_equip_item_index = -1;
    attached = nullptr; player.reset(); skill_db.clear();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("DRUID_GEAR_NATIVE_COMPLETE: 20 item/grade/copy cases; %d native assertions\n", assertions);
    return errors ? 1 : 0;
}
'''
    return source


def run_native_probe(items):
    # Current skill.cpp references the current packet helper; compile clif.cpp
    # too so a stale support object cannot hide that added symbol.
    fresh = ('src/map/script.cpp', 'src/map/pc.cpp', 'src/map/skill.cpp',
             'src/map/clif.cpp', 'src/common/malloc.cpp')
    excluded = {Path(path).stem + '.o' for path in fresh}
    objects = sorted(path for path in (ROOT / 'src/map/obj').rglob('*.o') if path.name not in excluded)
    libraries = [ROOT / path for path in ('src/common/obj/common.a',
                 '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not path.is_file() for path in libraries):
        raise RuntimeError('Existing local Linux map-server support objects are required')
    sanitizer = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    flags = ['g++', '-std=c++17', '-O0', '-g', '-DPACKETVER=20260219',
             '-fno-strict-aliasing', '-fno-omit-frame-pointer'] + sanitizer
    flags.extend('-I' + path for path in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                 '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include', '/usr/include/mysql'))
    with tempfile.TemporaryDirectory(prefix='rathena-druid-gear-vm-') as tmp:
        build = Path(tmp)
        driver = build / 'driver.cpp'
        driver.write_text(native_driver(items), encoding='ascii')
        compiled = []
        for path in (*[ROOT / p for p in fresh], driver):
            target = build / (path.stem + '.o')
            print(f'Compiling {path.name}; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}', flush=True)
            subprocess.run(flags + ['-c', str(path), '-o', str(target)], cwd=ROOT, check=True)
            compiled.append(target)
        executable = build / 'druid_gear_vm'
        command = ['g++'] + sanitizer + ['-o', str(executable)]
        command.extend(str(path) for path in compiled + objects + libraries)
        command.extend('-Wl,--wrap=' + symbol for symbol in WRAPPERS)
        command.extend(['-lz', '-ldl', '-lmysqlclient', '-lzstd', '-lssl', '-lcrypto', '-lresolv', '-lm'])
        subprocess.run(command, cwd=ROOT, check=True)
        result = subprocess.run([str(executable)], cwd=ROOT, capture_output=True, text=True, timeout=60)
        print(result.stdout, end='', flush=True)
        print(result.stderr, end='', flush=True)
        result.check_returncode()
        if result.stdout.count('DRUID_GEAR_NATIVE_COMPLETE: 20 item/grade/copy cases') != 1:
            raise AssertionError('Native VM did not reach the required completion marker')


class DruidGearTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.data = yaml.safe_load((ROOT / ITEM_PATH).read_text())
        cls.items = {item['Id']: item for item in cls.data['Body']}

    def test_exact_identities_and_no_invented_item_rules(self):
        self.assertEqual(self.data['Header'], {'Type': 'ITEM_DB', 'Version': 3})
        self.assertEqual(len(self.data['Body']), 2)
        self.assertEqual({i: r['AegisName'] for i, r in self.items.items()},
                         {i: v[0] for i, v in EXPECTED.items()})
        for item in self.items.values():
            self.assertEqual(set(item), {'Id', 'AegisName', 'Name', 'Type', 'SubType', 'Script'})
            self.assertEqual((item['Type'], item['SubType']), ('Card', 'Enchant'))

    def test_loaded_symbolic_skills_and_numeric_ids(self):
        skills = {r['Name']: r['Id'] for r in renewal_records(ROOT, 'db/skill_db.yml') if 'Name' in r}
        for item_id, (_, skill, number) in EXPECTED.items():
            self.assertEqual(skills[skill], number)
            self.assertEqual(set(re.findall(r'bonus2 bSkillAtk,"([A-Z0-9_]+)"',
                                           self.items[item_id]['Script'])), {skill})
        self.assertEqual(skills['AT_QUILL_SPEAR_S'], 6589)

    def test_existing_recipe_material_identities(self):
        items = {r['AegisName']: r['Id'] for r in renewal_records(ROOT, 'db/item_db.yml')
                 if 'AegisName' in r}
        self.assertEqual({name: items[name] for name in ('ClockTower_Gear', 'Shadowdecon', 'Zelunium')},
                         {'ClockTower_Gear': 1000681, 'Shadowdecon': 25729, 'Zelunium': 25731})

    def test_effective_group24_recipes_and_unchanged_original_properties(self):
        overlay = yaml.safe_load(RECIPE_PATH.read_text())
        groups = [r for r in overlay['Body'] if r['Id'] == 24]
        if not groups and not OPTIONS.require_import:
            self.skipTest('group 24 shared recipe wiring is not present yet')
        self.assertEqual(len(groups), 1)
        group = groups[0]
        self.assertEqual(set(group), {'Id', 'Slots'})
        self.assertEqual(len(group['Slots']), 1)
        slot = group['Slots'][0]
        self.assertEqual(set(slot), {'Slot', 'PerfectEnchants'})
        self.assertEqual(slot['Slot'], 2)
        self.assertEqual(len(slot['PerfectEnchants']), 2)
        self.assertEqual({r['Item'] for r in slot['PerfectEnchants']}, {'Gear_AT1', 'Gear_AT2'})
        for recipe in slot['PerfectEnchants']:
            self.assertEqual(set(recipe), {'Item', 'Price', 'Materials'})
            self.assertEqual(recipe['Price'], 0)
            self.assertEqual(len(recipe['Materials']), 3)
            self.assertEqual({r['Material']: r['Amount'] for r in recipe['Materials']},
                             {'ClockTower_Gear': 150, 'Shadowdecon': 150, 'Zelunium': 150})
        before, upgrades_before = recipe_baseline()
        after = server_configuration(ROOT)
        restored = copy.deepcopy(after)
        for name in ('Gear_AT1', 'Gear_AT2'):
            self.assertNotIn(name, before[24]['Slots'][2]['Perfect'])
            self.assertEqual(after[24]['Slots'][2]['Perfect'][name], {'Price': 0, 'Materials': {
                'ClockTower_Gear': 150, 'Shadowdecon': 150, 'Zelunium': 150}})
            del restored[24]['Slots'][2]['Perfect'][name]
        # Includes all original targets, eligibility, reset, ordering, normal
        # distributions/costs, older perfect recipes, and unrelated groups.
        self.assertEqual(restored, before)
        self.assertEqual(server_recipes(ROOT), upgrades_before)

    def test_client_fragment_identity_scope_and_generic_resource(self):
        text = FRAGMENT.read_text(encoding='ascii')
        declarations = re.findall(r'^add\((\d+), ("[^"\n]+"), "([^"\n]+)"\)$', text, re.M)
        self.assertEqual(len(declarations), 2)
        self.assertEqual({int(i): json.loads(name) for i, name, _ in declarations},
                         {i: item['Name'] for i, item in self.items.items()})
        self.assertEqual({int(i): skill for i, _, skill in declarations},
                         {314269: 'Pinion Shot', 314270: 'Quill Spear'})
        self.assertEqual(text.count('ResourceName = "EpisodClear20"'), 2)
        self.assertIn('tbl_druidgear = items', text)
        self.assertIn('^0000CCType:^000000 Enchant', text)
        self.assertIn('^0000CCWeight:^000000 0', text)

    def test_actual_lua_fragment_records_descriptions_and_merge(self):
        if OPTIONS.lua is None:
            self.skipTest('pass --lua for the Lua 5.1 fragment/native merge test')
        self.assertEqual(run_lua_fragment(self.items),
                         'DRUID_GEAR_LUA_COMPLETE: two exact records, descriptions and native merge contract')

    def test_no_conflicting_effective_item_definition(self):
        records = list(renewal_records(ROOT, 'db/item_db.yml'))
        for item_id, item in self.items.items():
            matches = [r for r in records if r['Id'] == item_id or r.get('AegisName') == item['AegisName']]
            self.assertIn(len(matches), (0, 1))
            if matches:
                self.assertEqual(matches[0], item)
            if OPTIONS.require_import:
                self.assertEqual(matches, [item])
        if OPTIONS.require_import:
            root = yaml.safe_load((ROOT / 'db/item_db.yml').read_text())
            self.assertEqual([r for r in root['Footer']['Imports'] if r['Path'] == ITEM_PATH],
                             [{'Path': ITEM_PATH, 'Mode': 'Renewal'}])

    def test_permitted_effect_description(self):
        if OPTIONS.effect_reference is None:
            self.skipTest('pass --effect-reference for permitted loose fallback Lua verification')
        raw = OPTIONS.effect_reference.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(), REFERENCE_HASH)
        text = raw.decode('utf-8')
        for item_id, skill in ((314269, 'Pinion Shot'), (314270, 'Quill Spear')):
            record = re.search(r'\[' + str(item_id) + r'\] = \{(.*?)\n\t\},', text, re.S)[1]
            strings = re.findall(r'^\s*"([^"\n]*)",?$', record, re.M)
            descriptions = [re.sub(r'\^[0-9A-Fa-f]{6}', '', value) for value in strings if value != '_' * 23]
            self.assertEqual(descriptions, [f'{skill} damage +5%', 'Grade >= D:', f'{skill} damage +3%',
                'Grade >= C:', 'Physical damage to All sizes +10%', 'Grade >= B:', f'{skill} damage +4%',
                'Grade >= A:', f'{skill} damage +6%'])

    def test_original_client_identity_and_recipe_evidence(self):
        if OPTIONS.client is None or OPTIONS.client_item_names is None:
            self.skipTest('pass --client and --client-item-names for original active-client verification')
        self.assertEqual(hashlib.sha256(OPTIONS.client.read_bytes()).hexdigest(), CLIENT_HASH)
        raw = OPTIONS.client_item_names.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(), NAMES_HASH)
        names = literal_tables(raw)['ItemDBNameTbl']
        recipes = client_configuration(OPTIONS.client, lambda name: name)[24]['Slots'][2]['Perfect']
        for item_id, (name, _, _) in EXPECTED.items():
            self.assertEqual(names[name], item_id)
            self.assertEqual(recipes[name], {'Price': 0, 'Materials': {
                'ClockTower_Gear': 150, 'Shadowdecon': 150, 'Zelunium': 150}})

    def test_actual_native_vm_all_grades_and_additive_bonuses(self):
        if not OPTIONS.native_vm:
            self.skipTest('pass --native-vm to compile and run the isolated actual script VM')
        run_native_probe(self.items)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-vm', action='store_true')
    parser.add_argument('--require-import', action='store_true')
    parser.add_argument('--client', type=Path)
    parser.add_argument('--client-item-names', type=Path)
    parser.add_argument('--effect-reference', type=Path)
    parser.add_argument('--lua', type=Path)
    parser.add_argument('--client-root', type=Path, default=ROOT.parent.parent)
    OPTIONS, remaining = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0], *remaining])
