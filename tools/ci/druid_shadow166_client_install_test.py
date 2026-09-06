#!/usr/bin/env python3
"""Read-only actual Lua verification of eleven installed Shadow 166 additions.

Requires an explicit pre-install loader backup. This checks the real active
loader, not only a candidate fragment; no game/rendering proof is claimed.
Run after the separate data/effect/recipe and native-parser regression suite.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess

from chapter2_client_helper_test import windows_path
from chapter2_gear_client_install_test import import_arrays, require
from audit_enchant_upgrades import renewal_records

ROOT = Path(__file__).resolve().parents[2]
CLIENT = ROOT.parent.parent
FILE = 'itemInfo_DruidShadow166.lua'
TABLE = 'druidshadow166'
FRAGMENT = ROOT / 'client-patch/druid_shadow166/SystemEN' / FILE
IDS = set(range(314804, 314811)) | set(range(1270183, 1270187))

LUA = r'''
assert(_VERSION == "Lua 5.1")
os.execute, io.popen = nil, nil
local function copy(v)
  if type(v) ~= "table" then return v end
  local r = {}
  for k, c in pairs(v) do r[k] = copy(c) end
  return r
end
local comparisons = 0
local function same(a,b)
  comparisons = comparisons + 1
  if type(a) ~= type(b) then return false end
  if type(a) ~= "table" then return a == b end
  for k,v in pairs(a) do if not same(v,b[k]) then return false end end
  for k in pairs(b) do if a[k] == nil then return false end end
  return true
end
local function count(t) local n=0; for _ in pairs(t) do n=n+1 end; return n end
local probe={name="old",lines={"one","two"}}
local snapshot=copy(probe); probe.lines[2]="changed"
assert(not same(probe,snapshot),"negative deep comparison control")
dofile(BEFORE_LOADER)
local before = copy(tbl)
local old_files, old_tables = copy(ImportFiles), copy(ImportTables)
for id in pairs(EXPECTED) do assert(before[id] == nil,"must not overwrite old metadata") end
dofile(CHECKED_LOADER)
assert(#ImportFiles == #old_files+1 and #ImportTables == #old_tables+1)
for i,v in ipairs(old_files) do assert(ImportFiles[i] == v and ImportTables[i] == old_tables[i]) end
assert(ImportFiles[#ImportFiles] == "itemInfo_DruidShadow166.lua")
assert(ImportTables[#ImportTables] == "druidshadow166")
for id,entry in pairs(before) do assert(same(entry,tbl[id]),"old record changed: "..id) end
local added={}
for id,entry in pairs(tbl) do if before[id] == nil then added[id]=entry end end
assert(count(added)==11)
local environment={}
local chunk=assert(loadfile(REVIEWED_FRAGMENT)); setfenv(chunk,environment); chunk()
local reviewed=assert(environment.tbl_druidshadow166)
assert(count(reviewed)==11 and same(reviewed,added),"installed records differ from reviewed fragment")
for id,entry in pairs(added) do
  assert(EXPECTED[id] and entry.identifiedDisplayName == EXPECTED[id])
  assert(entry.slotCount==0 and entry.ClassNum==0 and entry.costume==false)
  assert(entry.identifiedResourceName=="EpisodClear20" and entry.unidentifiedResourceName=="EpisodClear20")
end
print('{"result":"PASS","previous_records":'..count(before)..',"checked_records":'..count(tbl)..
      ',"added_records":11,"recursive_comparisons":'..comparisons..',"old_metadata_unchanged":true}')
'''


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before-loader', type=Path, required=True)
    parser.add_argument('--loader', type=Path, default=CLIENT / 'SystemEN/itemInfo.lua')
    parser.add_argument('--lua', type=Path, default=ROOT.parent / 'chapter2-lua51-runtime-20260906/runtime/lua5.1.exe')
    args = parser.parse_args()
    before, checked = args.before_loader.resolve(), args.loader.resolve()
    require(before != checked, 'A separate pre-install backup is required')
    old, new = import_arrays(before), import_arrays(checked)
    require(new['ImportFiles'] == old['ImportFiles'] + [FILE], 'Only the new final file import is allowed')
    require(new['ImportTables'] == old['ImportTables'] + [TABLE], 'Only the matching final table import is allowed')
    restored = checked.read_text(encoding='utf-8-sig')
    for value in (FILE, TABLE):
        restored, n = re.subn(r'^\s*"' + re.escape(value) + r'",[^\n]*\n', '', restored, flags=re.M)
        require(n == 1, 'Exactly one new import line is required')
    require(restored == before.read_text(encoding='utf-8-sig'), 'Loader changed beyond two import lines')
    executable = [line.strip() for line in restored.splitlines() if line.strip() and not line.lstrip().startswith('--')]
    require(re.fullmatch(r'F_itemInfoMerge\(tbl_override,\s*true\)\s*(?:--.*)?', executable[-1]), 'Override merge must remain last')
    installed = CLIENT / 'SystemEN' / FILE
    require(installed.read_bytes() == FRAGMENT.read_bytes(), 'Installed fragment bytes differ from reviewed source')
    items = {}
    for record in renewal_records(ROOT, 'db/item_db.yml'):
        if record['Id'] in IDS:
            items.setdefault(record['Id'], {}).update(record)
    require(set(items) == IDS, 'All eleven effective server identities must exist')
    for item_id, record in items.items():
        expected = ('Card','Enchant') if item_id < 1000000 else ('ShadowGear',None)
        require((record.get('Type'),record.get('SubType')) == expected and record.get('Slots',0)==0,
                'Server type/subtype/slots drift')
    values = {'BEFORE_LOADER': before, 'CHECKED_LOADER': checked, 'REVIEWED_FRAGMENT': FRAGMENT}
    prelude = '\n'.join(k+'='+json.dumps(windows_path(v)) for k,v in values.items())
    prelude += '\nEXPECTED={' + ','.join('['+str(i)+']='+json.dumps(items[i]['Name']) for i in sorted(IDS)) + '}\n'
    tracked = [before, checked, installed, FRAGMENT, CLIENT/'DATA.INI']
    hashes = {str(path): digest(path) for path in tracked}
    env = {k:v for k,v in os.environ.items() if k not in {'LUA_INIT','LUA_PATH','LUA_CPATH'}}
    result = subprocess.run([str(args.lua.resolve()),'-'], cwd=CLIENT, input=(prelude+LUA).encode('ascii'),
                            capture_output=True, env=env, timeout=60)
    require(result.returncode==0,result.stderr.decode('cp949',errors='replace'))
    report = json.loads(result.stdout)
    require(report['result']=='PASS' and report['added_records']==11,'Incomplete native Lua report')
    require(hashes == {str(path):digest(path) for path in tracked},'Input changed during read-only verification')
    report.update(before_loader=str(before),checked_loader=str(checked),
                  loader_is_active=checked==(CLIENT/'SystemEN/itemInfo.lua').resolve(),
                  fragment_sha256=digest(FRAGMENT), checked_loader_sha256=digest(checked),
                  lua_runtime_sha256=digest(args.lua), added_ids=sorted(IDS),
                  scope='Actual Lua loader/deep merge; no game rendering, packets, or charging')
    print(json.dumps(report,indent=2))


if __name__ == '__main__':
    main()
