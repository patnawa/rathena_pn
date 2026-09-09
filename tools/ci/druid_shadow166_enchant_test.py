#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  druid_shadow166_enchant_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_shadow166_enchant_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Exact group166 data, original source/Lua and isolated native VM regressions.

--native-vm freshly compiles itemdb/script/pc/skill/clif/malloc with ASan/UBSan.
It requires existing Linux map support objects and denies all socket operations.
It proves parser/effect/equip behavior, not packet charging or a live NPC route.
External source tests skip unless paths supplied; --require-import fails if the
three new overlays are not wired into their Renewal database roots.
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
ITEM = 'db/import/druid_shadow166_items.yml'
COMBO = 'db/import/druid_shadow166_combos.yml'
RECIPE = 'db/import/druid_shadow166_enchants.yml'
FRAGMENT = ROOT / 'client-patch/druid_shadow166/SystemEN/itemInfo_DruidShadow166.lua'
IMPORTS = {'db/item_db.yml': ITEM, 'db/item_combos.yml': COMBO, 'db/item_enchant.yml': RECIPE}
TARGETS = {1270183: ('S_AT_Armor', 'Armor', 'Shadow_Armor'),
           1270184: ('S_AT_Shoes', 'Shoes', 'Shadow_Shoes'),
           1270185: ('S_AT_Earring', 'Earring', 'Shadow_Right_Accessory'),
           1270186: ('S_AT_Pendant', 'Pendant', 'Shadow_Left_Accessory')}
SOULS = {314804: ('AT_Soul_AC', 'Alpha Claw', 'AT_ALPHA_CLAW', 6580),
         314805: ('AT_Soul_FF', 'Frenzy Fang', 'AT_FRENZY_FANG', 6582),
         314806: ('AT_Soul_PS', 'Pinion Shot', 'AT_PINION_SHOT', 6586),
         314807: ('AT_Soul_QS', 'Quill Spear', 'AT_QUILL_SPEAR', 6588),
         314808: ('AT_Soul_GS', 'Glacial Shard', 'AT_GLACIER_SHARD', 6594),
         314809: ('AT_Soul_RP', 'Roaring Piercer', 'AT_ROARING_PIERCER', 6597),
         314810: ('AT_Soul_TH', 'Terra Harvest', 'AT_TERRA_HARVEST', 6603)}
GENERIC = dict(zip(('M_Pow3','M_Sta3','M_Wis3','M_Spl3','M_Con3','M_Crt3',
                   'Nimble_Soul','Casting_Soul','Critical_Soul','Expert_Soul','Robust_Soul'),
                  range(312189,312200)))
MASTER_JOBS = dict.fromkeys(('Alchemist','Assassin','BardDancer','Blacksmith','Crusader','Hunter',
                            'KagerouOboro','Knight','Monk','Priest','Rebellion','Rogue','Sage',
                            'SoulLinker','StarGladiator','SuperNovice','Spirit_Handler','Wizard'), True)
HASHES = {'client': '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d',
          'client_item_names': '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496',
          'effect_reference': '235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a'}
OPTIONS = argparse.Namespace(native_vm=False, require_import=False, client=None,
                             client_item_names=None, effect_reference=None, lua=None,
                             client_root=ROOT.parent.parent)


def load(path):
    return yaml.safe_load((ROOT / path).read_text(encoding='utf-8'))


def configurations(include):
    """Read effective imports with only our three overlay imports added/removed.

    Baseline suppression removes entire files, not selected known fields: guards
    detect unexpected additions to unrelated groups/recipes/combos/items.
    """
    original = Path.read_text

    def read(path, *args, **kwargs):
        text = original(path, *args, **kwargs)
        try:
            relative = path.resolve().relative_to(ROOT).as_posix()
        except ValueError:
            return text
        if not include and relative in IMPORTS.values():
            data = yaml.safe_load(text)
            data['Body'] = []
            return yaml.safe_dump(data)
        if include and relative in IMPORTS:
            data = yaml.safe_load(text)
            imports = data.setdefault('Footer', {}).setdefault('Imports', [])
            if not any(entry['Path'] == IMPORTS[relative] for entry in imports):
                imports.append({'Path': IMPORTS[relative], 'Mode': 'Renewal'})
            return yaml.safe_dump(data)
        return text

    with patch.object(Path, 'read_text', read):
        records = list(renewal_records(ROOT, 'db/item_db.yml'))
        items = {}
        for record in records:
            items.setdefault(record['Id'], {}).update(record)
        return items, server_configuration(ROOT), server_recipes(ROOT), list(renewal_records(ROOT, 'db/item_combos.yml'))


def native_probe(baseline):
    fresh = ('src/map/itemdb.cpp','src/map/script.cpp','src/map/pc.cpp','src/map/skill.cpp',
             'src/map/clif.cpp','src/common/malloc.cpp')
    excluded = {Path(path).stem + '.o' for path in fresh}
    objects = sorted(p for p in (ROOT / 'src/map/obj').rglob('*.o') if p.name not in excluded)
    libraries = [ROOT / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                   '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries):
        raise RuntimeError('Existing local Linux map support objects required')
    sanitizers = ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
    flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219',
             '-fno-strict-aliasing','-fno-omit-frame-pointer'] + sanitizers
    flags.extend('-I' + p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
                 '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql'))
    prefix = (ROOT / 'tools/ci/native_script_vm_test.cpp').read_text().split(
        'extern "C" int __wrap_main(int, char**) {', 1)[0]
    source = prefix + (ROOT / 'tools/ci/druid_shadow166_native_test.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='rathena-druid-shadow166-') as temp:
        build = Path(temp)
        driver = build / 'driver.cpp'
        driver.write_text(source, encoding='ascii')
        fixture = build / 'actual_dependencies.yml'
        fixture.write_text(yaml.safe_dump({'Body': [baseline[i] for i in
            [24792,24793,1001253,*GENERIC.values()]]}), encoding='ascii')
        compiled = []
        for path in (*[ROOT / p for p in fresh], driver):
            obj = build / (path.stem + '.o')
            print(f'Compiling {path.name}; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}', flush=True)
            subprocess.run(flags + ['-c',str(path),'-o',str(obj)], cwd=ROOT, check=True)
            compiled.append(obj)
        exe = build / 'druid_shadow166_vm'
        command = ['g++',*sanitizers,'-o',str(exe),*map(str,compiled+objects+libraries)]
        command.extend('-Wl,--wrap=' + symbol for symbol in WRAPPERS)
        command.extend(['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'])
        subprocess.run(command, cwd=ROOT, check=True)
        result = subprocess.run([str(exe),str(fixture)], cwd=ROOT, capture_output=True, text=True, timeout=60)
        print(result.stdout, end='', flush=True); print(result.stderr, end='', flush=True)
        result.check_returncode()
        if re.search(r'Memory manager: args|runtime error:|ERROR: AddressSanitizer|VM TEST FAIL', result.stdout+result.stderr):
            raise AssertionError('Native allocator/sanitizer reported an error')
        if 'Memory manager: No memory leaks found.' not in result.stdout:
            raise AssertionError('Native allocator did not report leak-free completion')
        if result.stdout.count('DRUID_SHADOW166_NATIVE_COMPLETE: 2662 three-piece refine cases;') != 1:
            raise AssertionError('Native VM completion marker missing')


class Shadow166Tests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.items_doc, cls.combo_doc, cls.recipe_doc = load(ITEM), load(COMBO), load(RECIPE)
        cls.items = {r['Id']: r for r in cls.items_doc['Body']}
        cls.before = configurations(False)
        cls.after = configurations(True)

    def test_exact_items_and_no_invented_rules(self):
        self.assertEqual(self.items_doc['Header'], {'Type':'ITEM_DB','Version':3})
        self.assertEqual(len(self.items_doc['Body']), 13)
        self.assertEqual(set(self.items), set(TARGETS) | set(SOULS) | {24792,24793})
        for item_id, (aegis, name, location) in TARGETS.items():
            self.assertEqual(self.items[item_id], {'Id':item_id,'AegisName':aegis,
                'Name':f'M. Alitea Shadow {name}','Type':'ShadowGear','Weight':0,'Slots':0,
                'Jobs':{'Alitea':True},'Classes':{'Fourth':True},'Locations':{location:True},
                'EquipLevelMin':200,'Refineable':True,'Script':'bonus bMaxHP,getrefine()*10;\n'})
        for item_id, (aegis, skill, symbol, _) in SOULS.items():
            self.assertEqual(self.items[item_id], {'Id':item_id,'AegisName':aegis,'Name':skill+' Soul',
                'Type':'Card','SubType':'Enchant','Script':f'bonus2 bSkillAtk,"{symbol}",2+getrefine()/2;\n'})

    def test_master_job_map_single_addition_and_all_other_fields_unchanged(self):
        for item_id in (24792,24793):
            before = self.before[0][item_id]
            self.assertEqual(before['Jobs'], MASTER_JOBS)
            self.assertEqual(before['Classes'], {'Fourth':True})
            self.assertEqual(self.items[item_id], {'Id':item_id,'Jobs':{**MASTER_JOBS,'Alitea':True}})
            expected = copy.deepcopy(before)
            expected['Jobs']['Alitea'] = True
            self.assertEqual(self.after[0][item_id], expected)
        restored = copy.deepcopy(self.after[0])
        for item_id in {*TARGETS,*SOULS}: del restored[item_id]
        for item_id in (24792,24793): restored[item_id]['Jobs'].pop('Alitea')
        self.assertEqual(restored, self.before[0])

    def test_exact_skills_and_existing_dependencies(self):
        source = (ROOT / 'tools/ci/druid_shadow166_native_test.cpp').read_text()
        oracle = re.search(r'static bool original_pc_isItemClass \(.*?\n\}',source,re.S)[0]
        oracle = oracle.replace('original_pc_isItemClass','pc_isItemClass',1)
        self.assertEqual(hashlib.sha256(oracle.encode()).hexdigest(),
                         '6730e9e85692e73963727e5b7f5bf60ff629d20be528704b38ce9e63e8cab09a')
        skills = {r['Name']:r['Id'] for r in renewal_records(ROOT,'db/skill_db.yml') if 'Name' in r}
        for _, _, symbol, number in SOULS.values(): self.assertEqual(skills[symbol], number)
        self.assertEqual(skills['AT_QUILL_SPEAR_S'],6589)
        self.assertEqual(skills['AT_ROARING_PIERCER_S'],6598)
        for name, item_id in {**GENERIC,'S_Enchant_Essence':1001253,
                             'S_Master_Weapon':24792,'S_Master_Shield':24793}.items():
            self.assertEqual(self.before[0][item_id]['AegisName'],name)
        refine = {r['Group']:r for r in renewal_records(ROOT,'db/refine.yml')}
        for group in ('Shadow_Armor','Shadow_Weapon'):
            self.assertEqual([r['Level'] for r in refine[group]['Levels']], [1])
            self.assertEqual([r['Level'] for r in refine[group]['Levels'][0]['RefineLevels']], list(range(1,11)))

    def test_exact_eighteen_recipes_and_original_groups_preserved(self):
        self.assertEqual(self.recipe_doc['Header'], {'Type':'ITEM_ENCHANT_DB','Version':1})
        self.assertEqual(len(self.recipe_doc['Body']),1)
        group = self.recipe_doc['Body'][0]
        self.assertEqual(set(group), {'Id','TargetItems','MinimumRefine','MinimumEnchantgrade',
                                     'AllowRandomOptions','Reset','Order','Slots'})
        self.assertEqual(group['Id'],166)
        self.assertEqual(group['TargetItems'],dict.fromkeys((v[0] for v in TARGETS.values()),True))
        self.assertEqual((group['MinimumRefine'],group['MinimumEnchantgrade'],group['AllowRandomOptions']), (0,0,True))
        self.assertEqual(group['Reset'],{'Chance':0,'Price':0})
        self.assertEqual(group['Order'],[{'Slot':3},{'Slot':2}])
        self.assertEqual([s['Slot'] for s in group['Slots']],[3,2])
        expected = {3: {name:1 for name in list(GENERIC)[:6]},
                    2: {**{name:3 for name in list(GENERIC)[6:]},**{v[0]:5 for v in SOULS.values()}}}
        for slot in group['Slots']:
            self.assertEqual(set(slot),{'Slot','PerfectEnchants'})
            recipes = slot['PerfectEnchants']
            self.assertEqual(len(recipes),len(expected[slot['Slot']]))
            self.assertEqual({r['Item'] for r in recipes},set(expected[slot['Slot']]))
            for recipe in recipes:
                self.assertEqual(recipe, {'Item':recipe['Item'],'Price':0,'Materials':[
                    {'Material':'S_Enchant_Essence','Amount':expected[slot['Slot']][recipe['Item']]}]})
        self.assertNotIn(166,self.before[1])
        restored = copy.deepcopy(self.after[1]); restored.pop(166)
        self.assertEqual(restored,self.before[1])
        self.assertEqual(self.after[2][0],self.before[2][0] | {166})
        self.assertEqual(self.after[2][1:],self.before[2][1:])

    def test_exact_combo_membership_and_unchanged_original_sets(self):
        self.assertEqual(self.combo_doc['Header'],{'Type':'COMBO_DB','Version':1})
        expected = {frozenset(['S_Master_Shield','S_AT_Armor']), frozenset(['S_Master_Shield','S_AT_Shoes']),
                    frozenset(['S_Master_Weapon','S_AT_Earring']), frozenset(['S_Master_Weapon','S_AT_Pendant']),
                    frozenset(['S_Master_Shield','S_AT_Armor','S_AT_Shoes']),
                    frozenset(['S_Master_Weapon','S_AT_Earring','S_AT_Pendant']),
                    frozenset(['S_Master_Weapon','S_Master_Shield',*(v[0] for v in TARGETS.values())])}
        combos = []
        for record in self.combo_doc['Body']:
            self.assertEqual(set(record),{'Combos','Script'})
            for entry in record['Combos']:
                self.assertEqual(set(entry),{'Combo'})
                self.assertEqual(len(entry['Combo']),len(set(entry['Combo'])))
                combos.append(frozenset(entry['Combo']))
        self.assertEqual(len(combos),7); self.assertEqual(set(combos),expected)
        records = self.combo_doc['Body']
        self.assertEqual(len(records),4)
        self.assertEqual(records[0]['Script'],'bonus bAllTraitStats,2;\n')
        for index, slots in ((1,('SHIELD','ARMOR','SHOES')),(2,('WEAPON','ACC_R','ACC_L'))):
            script = 'bonus bPAtk,1;\nbonus bSMatk,1;\n.@sum = '+ '+'.join(
                'getequiprefinerycnt(EQI_SHADOW_'+slot+')' for slot in slots)+';\nif (.@sum >= 27) {\n'
            for bonus in ('bIgnoreDefRaceRate','bIgnoreMdefRaceRate'):
                for race, amount in (('RC_All',50),('RC_Player_Human',-50),('RC_Player_Doram',-50)):
                    script += f'  bonus2 {bonus},{race},{amount};\n'
            self.assertEqual(records[index]['Script'],script+'}\n')
        script = ''
        for bonus in ('bIgnoreResRaceRate','bIgnoreMResRaceRate'):
            for race, amount in (('RC_All',20),('RC_Player_Human',-20),('RC_Player_Doram',-20)):
                script += f'bonus2 {bonus},{race},{amount};\n'
        self.assertEqual(records[3]['Script'],script)
        # The Shadow overlay is intentionally imported before the later Druid
        # crown/weapon overlays.  Verify its exact contiguous import block, then
        # remove only that block and compare every remaining effective record in
        # order.  Assuming it is appended would make an unrelated later import
        # look like combo drift.
        body = self.combo_doc['Body']
        starts = [index for index in range(len(self.after[3]) - len(body) + 1)
                  if self.after[3][index:index + len(body)] == body]
        self.assertEqual(len(starts), 1)
        start = starts[0]
        self.assertEqual(self.after[3][:start] + self.after[3][start + len(body):],
                         self.before[3])

    def test_required_root_imports(self):
        if not OPTIONS.require_import: self.skipTest('pass --require-import after root integration')
        for root, overlay in IMPORTS.items():
            entries = [r for r in load(root)['Footer']['Imports'] if r['Path']==overlay]
            self.assertEqual(entries,[{'Path':overlay,'Mode':'Renewal'}])
        self.assertEqual(server_configuration(ROOT)[166],self.after[1][166])

    def test_permitted_soul_descriptions(self):
        if OPTIONS.effect_reference is None: self.skipTest('pass --effect-reference')
        raw = OPTIONS.effect_reference.read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(),HASHES['effect_reference'])
        text = raw.decode('utf-8')
        for item_id, (_, skill, _, _) in SOULS.items():
            record = re.search(r'\['+str(item_id)+r'\] = \{(.*?)\n\t\},',text,re.S)[1]
            descriptions = [re.sub(r'\^[0-9A-Fa-f]{6}','',s) for s in
                re.findall(r'^\s*"([^"\n]*)",?$',record,re.M) if s!='_'*23]
            self.assertEqual(descriptions,[skill+' damage +2%','For each 2 Refine Levels:',skill+' damage +1%'])

    def test_original_client_identities_and_all_recipe_properties(self):
        if OPTIONS.client is None or OPTIONS.client_item_names is None: self.skipTest('pass both original client paths')
        for field in ('client','client_item_names'):
            self.assertEqual(hashlib.sha256(getattr(OPTIONS,field).read_bytes()).hexdigest(),HASHES[field])
        names = literal_tables(OPTIONS.client_item_names.read_bytes())['ItemDBNameTbl']
        for item_id, value in {**TARGETS,**SOULS}.items(): self.assertEqual(names[value[0]],item_id)
        for name, item_id in GENERIC.items(): self.assertEqual(names[name],item_id)
        client = client_configuration(OPTIONS.client,lambda name:name)[166]
        self.assertEqual(client['Reset'],{'Enabled':False,'Chance':100000,'Price':10000,
                                         'Materials':{'S_Enchant_Essence':1}})
        client['Reset'] = {'Enabled':False,'Chance':0,'Price':0,'Materials':{}}
        self.assertEqual(client,self.after[1][166])

    def test_actual_lua_metadata_and_native_merge(self):
        if OPTIONS.lua is None: self.skipTest('pass --lua for actual Lua 5.1 merge proof')
        source = 'FRAGMENT = '+json.dumps(windows_path(FRAGMENT))+'\n'+r'''
assert(_VERSION == "Lua 5.1")
os.execute = nil; io.popen = nil
MessageBox = function(message) error(message) end
require("SystemEN/LuaFiles514/rotp_f")
local records = dofile(FRAGMENT)
assert(records == tbl_druidshadow166)
local sentinel = {identifiedDisplayName = "unrelated"}
tbl = {[501]=sentinel}
F_itemInfoMerge(records)
local count = 0
for id, info in pairs(records) do
  assert(tbl[id] == info)
  assert(info.identifiedResourceName == "EpisodClear20" and info.unidentifiedResourceName == "EpisodClear20")
  assert(info.slotCount == 0 and info.ClassNum == 0 and info.costume == false)
  assert(#info.unidentifiedDescriptionName == 1 and info.unidentifiedDescriptionName[1] == "")
  count = count + 1
end
assert(count == 11 and tbl[501] == sentinel)
'''
        for item_id, item in self.items.items():
            if item_id in (24792,24793): continue
            source += f'assert(tbl[{item_id}].identifiedDisplayName == {json.dumps(item["Name"])})\n'
            source += f'assert(tbl[{item_id}].unidentifiedDisplayName == {json.dumps(item["Name"])})\n'
            if item_id in SOULS:
                skill = SOULS[item_id][1]
                descriptions = [skill+' damage +2%.',f'An additional +1% {skill} damage per 2 refine levels of the enchanted equipment.',
                                '_'*23,'^0000CCType:^000000 Enchant','^0000CCWeight:^000000 0']
            else:
                location = TARGETS[item_id][1]
                partner, trio = ('Shield','Armor and Shoes') if item_id<1270185 else ('Weapon','Earring and Pendant')
                descriptions = ['Max HP +10 per refine level.',f'With Master Shadow {partner}: all six trait stats +2.',
                    f'With Master Shadow {partner} and M. Alitea Shadow {trio}: P.ATK +1 and S.MATK +1.',
                    'If those three pieces have a combined refine level of 27 or higher:',
                    'ignore 50% physical and magical defense of all races, excluding players.',
                    'With Master Shadow Weapon and Shield, and all four M. Alitea Shadow pieces:',
                    'ignore 20% physical and magical resistance of all races, excluding players.', '_'*23,
                    '^0000CCType:^000000 Shadow Equipment','^0000CCLocation:^000000 '+location,
                    '^0000CCRequired Level:^000000 200','^0000CCJobs:^000000 Alitea','^0000CCWeight:^000000 0']
            source += f'assert(#tbl[{item_id}].identifiedDescriptionName == {len(descriptions)})\n'
            for index, line in enumerate(descriptions,1):
                source += f'assert(tbl[{item_id}].identifiedDescriptionName[{index}] == {json.dumps(line)})\n'
        source += r'''
F_itemInfoMerge(records)
assert(tbl[501] == sentinel and tbl[1270183] == records[1270183])
tbl = {[1270183]=sentinel}
F_itemInfoMerge(records)
assert(tbl[1270183] == sentinel and tbl[314804] == records[314804])
print("DRUID_SHADOW166_LUA_COMPLETE: 11 exact identities/descriptions and actual merge")
'''
        result = subprocess.run([str(OPTIONS.lua.resolve()),'-'],cwd=OPTIONS.client_root,
                                input=source.encode('ascii'),capture_output=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr.decode('cp949',errors='replace'))
        self.assertEqual(result.stdout.decode('ascii').strip(),
                         'DRUID_SHADOW166_LUA_COMPLETE: 11 exact identities/descriptions and actual merge')

    def test_actual_native_parsers_vm_and_equip(self):
        if not OPTIONS.native_vm: self.skipTest('pass --native-vm for actual parser/VM/equip proof')
        native_probe(self.before[0])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-vm',action='store_true')
    parser.add_argument('--require-import',action='store_true')
    for option in ('client','client-item-names','effect-reference','lua'): parser.add_argument('--'+option,type=Path)
    parser.add_argument('--client-root',type=Path,default=ROOT.parent.parent)
    OPTIONS, remaining = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0],*remaining])
