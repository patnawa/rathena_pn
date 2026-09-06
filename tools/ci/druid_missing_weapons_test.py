#!/usr/bin/env python3
"""Twelve sourced weapon records, ten sets, actual Lua merge and isolated native VM.

The two unresolved Booster weapons are deliberately excluded. Native execution
requires compiled local support objects and the two actual crown dependencies.
No live server, network, client, acquisition, or existing database is mutated.
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
from audit_enchant_upgrades import renewal_records
from audit_initial_enchants import client_configuration
from chapter2_client_helper_test import windows_path
from lua51_literal_table import literal_tables
from run_native_script_vm_test import WRAPPERS

ROOT = Path(__file__).resolve().parents[2]
ITEM = 'db/import/druid_missing_weapons.yml'
COMBO = 'db/import/druid_missing_weapon_combos.yml'
FRAGMENT = ROOT / 'client-patch/druid_missing_weapons/SystemEN/itemInfo_DruidMissingWeapons.lua'
IMPORTS = {'db/item_db.yml': ITEM, 'db/item_combos.yml': COMBO}
# Independently transcribed primary metadata: name, type, weight, ATK, MATK, slots, level.
SPECS = {
    510185: ('Repeat_Dagger_AD','Dagger',900,150,0,2,170),
    510189: ('Solid_Whinger','Dagger',1100,200,0,2,220),
    510190: ('Glacier_N_Knife','Dagger',1200,210,210,0,210),
    510191: ('D_Glacier_N_Knife','Dagger',1200,210,210,1,230),
    510193: ('Dimen_AT_Knife','Dagger',1800,240,0,2,250),
    520047: ('F_Ein_AXE','1hAxe',2000,230,0,2,250),
    520052: ('Axe_Furious','1hAxe',5000,230,0,2,205),
    590104: ('Mocadas_Garz','Mace',1200,220,230,2,250),
    590117: ('Hall_Furious','Mace',1100,100,180,2,205),
    620056: ('Glacier_N_Axe','2hAxe',6000,350,180,0,210),
    620057: ('D_Glacier_N_Axe','2hAxe',6000,350,180,1,230),
    620059: ('Dimen_AT_Axe','2hAxe',4000,380,0,2,250),
}
UNRESOLVED = {510200:'NP_B_Dagger',620064:'SC_B_Axe'}
DEPENDENCIES = {470265:'FuriousBoots',400999:'Time_DM_R_Crown_AT',401176:'FuriousCirclet_AT',
                450270:'D_Glacier_Armor',470197:'D_Glacier_Boots',480283:'D_Glacier_Manteau',
                450271:'D_Glacier_Robe',470198:'D_Glacier_Shoes',480284:'D_Glacier_Muffler'}
SKILLS = {'KR_CHOP_CHOP':6552,'KR_SHARPEN_GUST':6555,'AT_PRIMAL_CLAW':6578,
          'AT_FERAL_CLAW':6579,'AT_ALPHA_CLAW':6580,'AT_SAVAGE_LUNGE':6581,
          'AT_FRENZY_FANG':6582,'AT_PINION_SHOT':6586,'AT_QUILL_SPEAR':6588,
          'AT_QUILL_SPEAR_S':6589,'AT_TEMPEST_FLAP':6590,'AT_GLACIER_MONOLITH':6592,
          'AT_GLACIER_NOVA':6593,'AT_GLACIER_SHARD':6594,'AT_TERRA_HARVEST':6603}
OPTIONS = argparse.Namespace(native_vm=False,require_import=False,client=None,
                             client_item_names=None,lua=None,client_root=ROOT.parent.parent,
                             dependency_items=[])


def load(path):
    return yaml.safe_load((ROOT / path).read_text(encoding='utf-8'))


def configuration(include):
    original = Path.read_text
    def read(path, *args, **kwargs):
        text = original(path, *args, **kwargs)
        try: relative = path.resolve().relative_to(ROOT).as_posix()
        except ValueError: return text
        if not include and relative in IMPORTS.values():
            data = yaml.safe_load(text); data['Body'] = []; return yaml.safe_dump(data)
        if include and relative in IMPORTS:
            data = yaml.safe_load(text)
            imports = data.setdefault('Footer',{}).setdefault('Imports',[])
            if not any(r['Path'] == IMPORTS[relative] for r in imports):
                imports.append({'Path':IMPORTS[relative],'Mode':'Renewal'})
            return yaml.safe_dump(data)
        return text
    with patch.object(Path,'read_text',read):
        items = {}
        for r in renewal_records(ROOT,'db/item_db.yml'): items.setdefault(r['Id'],{}).update(r)
        return items, list(renewal_records(ROOT,'db/item_combos.yml'))


def native_probe(dependencies, skills):
    fresh = ('src/map/itemdb.cpp','src/map/script.cpp','src/map/pc.cpp','src/map/skill.cpp',
             'src/map/clif.cpp','src/common/malloc.cpp')
    excluded = {Path(p).stem+'.o' for p in fresh}
    objects = sorted(p for p in (ROOT/'src/map/obj').rglob('*.o') if p.name not in excluded)
    libraries = [ROOT/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a',
                                  '3rdparty/rapidyaml/obj/ryml.a')]
    if not objects or any(not p.is_file() for p in libraries): raise RuntimeError('Local Linux map support objects required')
    san = ['-fsanitize=address,undefined','-fno-sanitize-recover=all']
    flags = ['g++','-std=c++17','-O0','-g','-DPACKETVER=20260219',
             '-fno-strict-aliasing','-fno-omit-frame-pointer',*san]
    flags.extend('-I'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src',
                 '3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include','/usr/include/mysql'))
    prefix = (ROOT/'tools/ci/native_script_vm_test.cpp').read_text().split('extern "C" int __wrap_main(int, char**) {',1)[0]
    with tempfile.TemporaryDirectory(prefix='rathena-druid-weapons-') as temp:
        build = Path(temp); driver = build/'driver.cpp'
        driver.write_text(prefix+(ROOT/'tools/ci/druid_missing_weapons_test.cpp').read_text(),encoding='ascii')
        fixture = build/'actual_dependencies.yml'
        fixture.write_text(yaml.safe_dump({'Items':list(dependencies.values()),'Skills':list(skills.values())}),encoding='ascii')
        compiled = []
        for path in (*[ROOT/p for p in fresh],driver):
            obj = build/(path.stem+'.o')
            print(f'Compiling {path.name}; SHA256 {hashlib.sha256(path.read_bytes()).hexdigest()}',flush=True)
            subprocess.run(flags+['-c',str(path),'-o',str(obj)],cwd=ROOT,check=True); compiled.append(obj)
        exe = build/'druid_missing_weapons_vm'
        command = ['g++',*san,'-o',str(exe),*map(str,compiled+objects+libraries)]
        command.extend('-Wl,--wrap='+symbol for symbol in WRAPPERS)
        command.extend(['-lz','-ldl','-lmysqlclient','-lzstd','-lssl','-lcrypto','-lresolv','-lm'])
        subprocess.run(command,cwd=ROOT,check=True)
        result = subprocess.run([str(exe),str(fixture)],cwd=ROOT,capture_output=True,text=True,timeout=60)
        print(result.stdout,end='',flush=True); print(result.stderr,end='',flush=True); result.check_returncode()
        if re.search(r'Memory manager: args|runtime error:|ERROR: AddressSanitizer|VM TEST FAIL|\[Error\]:|\[Warning\]:',result.stdout+result.stderr):
            raise AssertionError('Native allocator/sanitizer error')
        if 'Memory manager: No memory leaks found.' not in result.stdout: raise AssertionError('Missing leak-free completion')
        if result.stdout.count('DRUID_MISSING_WEAPONS_NATIVE_COMPLETE:') != 1: raise AssertionError('Missing completion marker')


class MissingWeaponsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.item_doc = load(ITEM); cls.combo_doc = load(COMBO)
        cls.items = {r['Id']:r for r in cls.item_doc['Body']}
        cls.before = configuration(False); cls.after = configuration(True)
        cls.dependencies = {i:cls.after[0][i] for i in DEPENDENCIES if i in cls.after[0]}
        for path in OPTIONS.dependency_items:
            for r in yaml.safe_load(path.read_text(encoding='utf-8'))['Body']:
                if r['Id'] in DEPENDENCIES: cls.dependencies.setdefault(r['Id'],{}).update(r)
        all_skills = {r['Name']:r for r in renewal_records(ROOT,'db/skill_db.yml') if 'Name' in r}
        cls.skills = {name:all_skills[name] for name in SKILLS}

    def test_exact_metadata_and_no_invented_economy(self):
        self.assertEqual(self.item_doc['Header'],{'Type':'ITEM_DB','Version':3})
        self.assertEqual(len(self.item_doc['Body']),12); self.assertEqual(set(self.items),set(SPECS))
        for item_id, (name,kind,weight,atk,matk,slots,level) in SPECS.items():
            record = copy.deepcopy(self.items[item_id]); record.pop('Name')
            script = record.pop('Script',None)
            expected = {'Id':item_id,'AegisName':name,'Type':'Weapon','SubType':kind,'Weight':weight,
                'Attack':atk,'Range':1,'Slots':slots,'Jobs':{'Karnos' if item_id==510185 else 'Alitea':True},
                'Locations':{'Both_Hand' if kind=='2hAxe' else 'Right_Hand':True},'WeaponLevel':5,
                'EquipLevelMin':level,'Refineable':True,'Gradable':True}
            if item_id != 510185: expected['Classes'] = {'Fourth':True}
            if matk: expected['MagicAttack'] = matk
            self.assertEqual(record,expected)
            if item_id in (510190,510191): self.assertIsNone(script)  # Fully sourced MATK enchant chassis, not placeholder bonuses.
            else: self.assertTrue(script)
            self.assertEqual('bUnbreakableWeapon' in (script or ''), kind in ('1hAxe','2hAxe','Mace'))
        self.assertTrue(set(UNRESOLVED).isdisjoint(self.items))

    def test_preserves_every_old_item_and_combo(self):
        self.assertTrue(set(SPECS).isdisjoint(self.before[0]))
        restored = copy.deepcopy(self.after[0])
        for item_id in SPECS: del restored[item_id]
        self.assertEqual(restored,self.before[0])
        new = self.combo_doc['Body']; remaining = copy.deepcopy(self.after[1])
        for record in new: remaining.remove(record)
        self.assertEqual(remaining,self.before[1])
        before_sets = {frozenset(c['Combo']) for r in self.before[1] for c in r['Combos']}
        new_sets = [frozenset(c['Combo']) for r in new for c in r['Combos']]
        self.assertEqual(len(new_sets),10); self.assertEqual(len(set(new_sets)),10)
        self.assertTrue(before_sets.isdisjoint(new_sets))

    def test_exact_sets_and_dependencies(self):
        expected = []
        for weapon in ('Axe_Furious','Hall_Furious'):
            for partner in ('FuriousBoots','FuriousCirclet_AT'): expected.append(frozenset((weapon,partner)))
        for weapon in ('Dimen_AT_Axe','Dimen_AT_Knife'): expected.append(frozenset((weapon,'Time_DM_R_Crown_AT')))
        for weapon in ('D_Glacier_N_Axe','D_Glacier_N_Knife'):
            for trio in (('D_Glacier_Armor','D_Glacier_Boots','D_Glacier_Manteau'),
                         ('D_Glacier_Robe','D_Glacier_Shoes','D_Glacier_Muffler')):
                expected.append(frozenset((weapon,*trio)))
        self.assertEqual(self.combo_doc['Header'],{'Type':'COMBO_DB','Version':1})
        self.assertEqual({frozenset(c['Combo']) for r in self.combo_doc['Body'] for c in r['Combos']},set(expected))
        for record in self.combo_doc['Body']:
            self.assertEqual(set(record),{'Combos','Script'}); self.assertTrue(record['Script'])
            for combo in record['Combos']: self.assertEqual(set(combo),{'Combo'})
        if OPTIONS.require_import or OPTIONS.native_vm:
            self.assertEqual(set(self.dependencies),set(DEPENDENCIES),'Actual two crown dependencies required; pass --dependency-items before integration')
        for item_id, record in self.dependencies.items(): self.assertEqual(record['AegisName'],DEPENDENCIES[item_id])
        for name,number in SKILLS.items(): self.assertEqual(self.skills[name]['Id'],number)
        script = '\n'.join(r.get('Script','') for r in [*self.items.values(),*self.combo_doc['Body']])
        referenced = set(re.findall(r'"((?:KR_|AT_)[A-Z_]+)"',script))
        self.assertEqual(referenced,set(SKILLS)-{'AT_QUILL_SPEAR_S'})
        # Every permitted bonus is observed by the native oracle. Unmodeled
        # effects or acquisition commands must not slip past zero-field checks.
        bonuses = set(re.findall(r'\bbonus2?\s+(\w+)',script))
        self.assertEqual(bonuses,{'bAtkRate','bVariableCastrate','bSkillAtk','bLongAtkRate',
            'bCon','bPAtk','bCritical','bBaseAtk','bCritAtkRate','bCRate','bUnbreakableWeapon',
            'bAspdRate','bAddSize','bSkillUseSP','bDelayrate','bAddEle','bShortAtkRate',
            'bMatkRate','bMagicAtkEle','bMagicAddSize','bSMatk','bMatk','bAddRace',
            'bSkillCooldown','bPow','bSpl','bMagicAddEle'})
        self.assertEqual(set(re.findall(r'\b([A-Za-z_]\w*)\s*\(',script)),
                         {'getrefine','getenchantgrade','getequiprefinerycnt','if'})
        for line in script.splitlines():
            self.assertRegex(line.strip(),r'^(?:$|[{}]|\.@[rg]\s*=|if\s*\(|bonus2?\s)')

    def test_required_imports_when_requested(self):
        if not OPTIONS.require_import: self.skipTest('pass --require-import after shared integration')
        for root,overlay in IMPORTS.items():
            self.assertEqual([r for r in load(root)['Footer']['Imports'] if r['Path']==overlay],
                             [{'Path':overlay,'Mode':'Renewal'}])
        actual = {}
        for r in renewal_records(ROOT,'db/item_db.yml'): actual.setdefault(r['Id'],{}).update(r)
        for item_id in (*SPECS,*DEPENDENCIES): self.assertIn(item_id,actual)

    def test_original_client_identity_and_slot_evidence(self):
        if OPTIONS.client_item_names is None or OPTIONS.client is None: self.skipTest('pass original --client-item-names and --client')
        self.assertEqual(hashlib.sha256(OPTIONS.client_item_names.read_bytes()).hexdigest(),
                         '2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496')
        self.assertEqual(hashlib.sha256(OPTIONS.client.read_bytes()).hexdigest(),
                         '664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d')
        names = literal_tables(OPTIONS.client_item_names.read_bytes())['ItemDBNameTbl']
        for item_id,name in {**{i:s[0] for i,s in SPECS.items()},**UNRESOLVED}.items(): self.assertEqual(names[name],item_id)
        groups = client_configuration(OPTIONS.client,lambda name:name)
        self.assertEqual(groups[31]['Order'],[3,2,1,0])
        self.assertEqual(self.items[510190]['Slots'],0)
        self.assertIn('Glacier_N_Knife',groups[31]['Targets'])
        for slot in groups[31]['Order']:
            self.assertGreaterEqual(slot,self.items[510190]['Slots'])
            self.assertTrue(groups[31]['Slots'][slot]['Perfect'])
        # The actual packet path enforces this same collision check; native test
        # separately parses zero slots. No packet charging claim is made here.
        source = (ROOT/'src/map/clif.cpp').read_text()
        self.assertIn('if( slot < sd->inventory_data[index]->slots )',source)

    def test_actual_lua_records_and_existing_merge_function(self):
        if OPTIONS.lua is None: self.skipTest('pass --lua for actual Lua5.1 proof')
        script = 'FRAGMENT = '+json.dumps(windows_path(FRAGMENT))+'\n'+r'''
assert(_VERSION == "Lua 5.1")
os.execute=nil; io.popen=nil
MessageBox=function(m) error(m) end
require("SystemEN/LuaFiles514/rotp_f")
local records=dofile(FRAGMENT)
assert(records==tbl_druidmissingweapons)
local sentinel={identifiedDisplayName="untouched"}
tbl={[501]=sentinel}
F_itemInfoMerge(records)
local count=0
for id,record in pairs(records) do
  assert(tbl[id]==record and record.identifiedResourceName=="EpisodClear20")
  assert(record.unidentifiedResourceName=="EpisodClear20" and record.costume==false)
  assert(#record.unidentifiedDescriptionName==1 and record.unidentifiedDescriptionName[1]=="")
  assert(#record.identifiedDescriptionName>=8)
  count=count+1
end
assert(count==12 and tbl[501]==sentinel and tbl[510200]==nil and tbl[620064]==nil)
'''
        for item_id, item in self.items.items():
            view = {'Dagger':1,'1hAxe':6,'2hAxe':7,'Mace':8}[item['SubType']]
            script += f'assert(tbl[{item_id}].slotCount=={item["Slots"]} and tbl[{item_id}].ClassNum=={view})\n'
            script += f'assert(tbl[{item_id}].identifiedDisplayName=={json.dumps(item["Name"])} and tbl[{item_id}].unidentifiedDisplayName=={json.dumps(item["Name"])})\n'
            for line in (f'^0000CCWeight:^000000 {item["Weight"]//10}',f'^0000CCRequired Level:^000000 {item["EquipLevelMin"]}'):
                script += f'do local found=false; for _,v in ipairs(tbl[{item_id}].identifiedDescriptionName) do if v=={json.dumps(line)} then found=true end end; assert(found) end\n'
        script += '''F_itemInfoMerge(records); assert(tbl[501]==sentinel)
tbl={[510185]=sentinel}; F_itemInfoMerge(records)
assert(tbl[510185]==sentinel and tbl[510189]==records[510189])
print("DRUID_MISSING_WEAPONS_LUA_COMPLETE: 12 exact identities and native merge")
'''
        result = subprocess.run([str(OPTIONS.lua.resolve()),'-'],cwd=OPTIONS.client_root,input=script.encode('ascii'),capture_output=True,timeout=30)
        self.assertEqual(result.returncode,0,result.stderr.decode('cp949',errors='replace'))
        self.assertEqual(result.stdout.decode('ascii').strip(),'DRUID_MISSING_WEAPONS_LUA_COMPLETE: 12 exact identities and native merge')

    def test_native_parser_vm_all_thresholds_equip_and_sets(self):
        if not OPTIONS.native_vm: self.skipTest('pass --native-vm for fresh native proof')
        native_probe(self.dependencies,self.skills)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--native-vm',action='store_true'); parser.add_argument('--require-import',action='store_true')
    for name in ('client','client-item-names','lua','client-root'): parser.add_argument('--'+name,type=Path)
    parser.add_argument('--dependency-items',action='append',type=Path,default=[])
    options,rest = parser.parse_known_args()
    if options.client_root is None: options.client_root = ROOT.parent.parent
    OPTIONS = options
    unittest.main(argv=[sys.argv[0],*rest])
