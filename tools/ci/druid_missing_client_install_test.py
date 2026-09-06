#!/usr/bin/env python3
"""Read-only actual Lua/deep-merge proof of the 35 missing-equipment additions.

Requires the separate pre-install loader and DATA.INI backup. This test does
not install anything and does not claim native-client rendering or gameplay.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from audit_enchant_upgrades import renewal_records
from chapter2_client_helper_test import windows_path
from chapter2_gear_client_install_test import import_arrays, require
from druid_missing_targets_test import TARGETS

ROOT = Path(__file__).resolve().parents[2]
CLIENT = ROOT.parent.parent
PARTNERS = {610093, 500136, 550194, 570093, 580093, 590116,
            630066, 650062, 830047, 540125, 550197}
IDS = {number for targets in TARGETS.values() for number in targets.values()} | PARTNERS
PACKAGES = (
    ('druid_missing_crowns', 'itemInfo_DruidMissingCrowns.lua', 'druidmissingcrowns'),
    ('druid_missing_weapons', 'itemInfo_DruidMissingWeapons.lua', 'druidmissingweapons'),
)

LUA = r'''
assert(_VERSION == "Lua 5.1")
os.execute, io.popen = nil, nil
local function copy(v)
  if type(v) ~= "table" then return v end
  local r = {}; for k,c in pairs(v) do r[k]=copy(c) end; return r
end
local comparisons=0
local function same(a,b)
  comparisons=comparisons+1
  if type(a)~=type(b) then return false end
  if type(a)~="table" then return a==b end
  for k,v in pairs(a) do if not same(v,b[k]) then return false end end
  for k in pairs(b) do if a[k]==nil then return false end end
  return true
end
local function count(t) local n=0; for _ in pairs(t) do n=n+1 end; return n end
local probe={name="old",lines={"one","two"}}
local saved=copy(probe); probe.lines[2]="changed"
assert(not same(probe,saved),"deep comparison negative control")
dofile(BEFORE_LOADER)
local before=copy(tbl)
local old_files,old_tables=copy(ImportFiles),copy(ImportTables)
for id in pairs(EXPECTED) do assert(before[id]==nil,"existing metadata collision: "..id) end
dofile(CHECKED_LOADER)
assert(#ImportFiles==#old_files+2 and #ImportTables==#old_tables+2)
for i,v in ipairs(old_files) do assert(ImportFiles[i]==v and ImportTables[i]==old_tables[i]) end
for i,p in ipairs(PACKAGES) do
  assert(ImportFiles[#old_files+i]==p.file and ImportTables[#old_tables+i]==p.table)
end
for id,entry in pairs(before) do assert(same(entry,tbl[id]),"old metadata changed: "..id) end
local added={}; for id,entry in pairs(tbl) do if before[id]==nil then added[id]=entry end end
assert(count(added)==35 and count(EXPECTED)==35)
local reviewed={}
for _,p in ipairs(PACKAGES) do
  local environment=setmetatable({}, {__index=_G})
  local chunk=assert(loadfile(p.path)); setfenv(chunk,environment); chunk()
  for id,entry in pairs(assert(rawget(environment,"tbl_"..p.table))) do
    assert(not reviewed[id],"reviewed fragments overlap"); reviewed[id]=entry
  end
end
assert(count(reviewed)==35 and same(reviewed,added),"installed records differ from reviewed fragments")
for id,entry in pairs(added) do
  local expected=assert(EXPECTED[id])
  assert(entry.identifiedDisplayName==expected.name and entry.slotCount==expected.slots)
  assert(entry.costume==false and type(entry.identifiedResourceName)=="string")
  assert(#entry.identifiedResourceName>0 and #entry.identifiedDescriptionName>0)
end
print('{"result":"PASS","previous_records":'..count(before)..',"checked_records":'..count(tbl)..
      ',"added_records":35,"recursive_comparisons":'..comparisons..',"old_metadata_unchanged":true}')
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-loader', type=Path, required=True)
    parser.add_argument('--before-data-ini', type=Path, required=True)
    parser.add_argument('--loader', type=Path, default=CLIENT / 'SystemEN/itemInfo.lua')
    parser.add_argument('--lua', type=Path,
                        default=ROOT.parent / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe')
    args = parser.parse_args()
    before, checked = args.before_loader.resolve(), args.loader.resolve()
    require(before != checked, 'Separate pre-install loader backup required')
    require(args.before_data_ini.resolve() != (CLIENT / 'DATA.INI').resolve(),
            'Separate DATA.INI backup required')
    require(args.before_data_ini.read_bytes() == (CLIENT / 'DATA.INI').read_bytes(),
            'Archive precedence changed')
    old, new = import_arrays(before), import_arrays(checked)
    require(new['ImportFiles'] == old['ImportFiles'] + [p[1] for p in PACKAGES], 'File import delta')
    require(new['ImportTables'] == old['ImportTables'] + [p[2] for p in PACKAGES], 'Table import delta')
    restored = checked.read_text(encoding='utf-8-sig')
    for _, filename, table in PACKAGES:
        for value in (filename, table):
            restored, n = re.subn(r'^\s*"'+re.escape(value)+r'",[^\n]*\n', '', restored, flags=re.M)
            require(n == 1, 'Exactly one added import line required: ' + value)
    require(restored == before.read_text(encoding='utf-8-sig'), 'Unrelated loader edit')
    lines = [s.strip() for s in restored.splitlines() if s.strip() and not s.lstrip().startswith('--')]
    require(re.fullmatch(r'F_itemInfoMerge\(tbl_override,\s*true\)\s*(?:--.*)?', lines[-1]),
            'Official overrides must remain last')
    tracked = [before, checked, args.before_data_ini, CLIENT / 'DATA.INI']
    packages = []
    for package, filename, table in PACKAGES:
        reviewed = ROOT / 'client-patch' / package / 'SystemEN' / filename
        installed = CLIENT / 'SystemEN' / filename
        require(installed.read_bytes() == reviewed.read_bytes(), 'Installed fragment differs: ' + filename)
        tracked.extend((reviewed, installed))
        packages.append('{file='+json.dumps(filename)+',table='+json.dumps(table)+
                        ',path='+json.dumps(windows_path(reviewed))+'}')
    items = {}
    for record in renewal_records(ROOT, 'db/item_db.yml'):
        if record['Id'] in IDS:
            items.setdefault(record['Id'], {}).update(record)
    require(set(items) == IDS and len(IDS) == 35, 'All 35 effective server identities required')
    prelude = 'BEFORE_LOADER='+json.dumps(windows_path(before))+'\n'
    prelude += 'CHECKED_LOADER='+json.dumps(windows_path(checked))+'\n'
    prelude += 'PACKAGES={'+','.join(packages)+'}\nEXPECTED={'
    prelude += ','.join('['+str(i)+']={name='+json.dumps(items[i]['Name'])+
                        ',slots='+str(items[i].get('Slots',0))+'}' for i in sorted(IDS))+'}\n'
    hashes = {str(path): digest(path) for path in tracked}
    env = {k:v for k,v in os.environ.items() if k not in {'LUA_INIT','LUA_PATH','LUA_CPATH'}}
    result = subprocess.run([str(args.lua.resolve()), '-'], cwd=CLIENT,
                            input=(prelude+LUA).encode('ascii'), capture_output=True, env=env, timeout=60)
    require(result.returncode == 0, result.stderr.decode('cp949', errors='replace'))
    report = json.loads(result.stdout)
    require(report['result'] == 'PASS' and report['added_records'] == 35, 'Incomplete Lua proof')
    require(hashes == {str(path):digest(path) for path in tracked}, 'Input mutated during read-only test')
    report.update(checked_loader=str(checked), before_loader=str(before),
                  loader_is_active=checked == (CLIENT/'SystemEN/itemInfo.lua').resolve(),
                  checked_loader_sha256=digest(checked), lua_runtime_sha256=digest(args.lua),
                  input_sha256=hashes, added_ids=sorted(IDS),
                  scope='Actual Lua loader and deep merge; not game rendering, packets, or charging')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
