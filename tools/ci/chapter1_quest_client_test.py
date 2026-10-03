#!/usr/bin/env python3
"""Replay Chapter 1 quest additions through the client's original Lua helper.

Supply the original 32-bit Lua 5.1 helper and matching runtime. Without --patch,
this tests installed data and reproduces the missing-18369 popup. With --patch,
it also proves all previous records survive and repeat loading is harmless.
"""
import argparse
import json
from pathlib import Path
import subprocess
import shutil
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--lua', type=Path, required=True)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--helper', type=Path, required=True)
    parser.add_argument('--patch', type=Path)
    args = parser.parse_args()
    catalog = json.loads((ROOT / 'npc/custom/main_office/chapter1_guide.json').read_text(encoding='utf-8'))
    ids = sorted({row['quest'] for row in catalog['steps']} | {18377, 18379})
    lua = r'''
table.insert = nil -- Match the restricted client quest environment.
local function clone(t)
 if type(t) ~= "table" then return t end
 local result={}; for k,v in pairs(t) do result[k]=clone(v) end; return result
end
local function equal(a,b)
 if type(a)~=type(b) then return false end
 if type(a)~="table" then return a==b end
 for k,v in pairs(a) do if not equal(v,b[k]) then return false end end
 for k in pairs(b) do if a[k]==nil then return false end end
 return true
end
local allowed={}
for _,id in ipairs(IDS) do allowed[id]=true end
local reference
for _,loader in ipairs({"SystemEN/OngoingQuests.lub", "SystemEN/OngoingQuestInfoList.lub",
                       "System/OngoingQuestInfoList.lub", "System/OngoingQuestInfoList_True.lub"}) do
 QuestInfoList=nil
 dofile(loader)
 local before=clone(QuestInfoList)
 if PATCH then dofile(PATCH) end
 dofile(HELPER)
 local descriptions=0
 AddOngoingDescription=function(id,text)
  assert(type(text)=="string", "Invalid Chapter 1 description: "..id)
  if not before[id] then assert(text~="", "Empty added description: "..id) end
  descriptions=descriptions+1
 end
 AddOngoingRewardInfo=function(id,item,amount)
  assert(type(item)=="number" and type(amount)=="number", "Invalid reward: "..id)
 end
 -- The precise rendered trigger comes first, producing the original line4 error.
 local ok,title=GetOngoingQuestInfoByID(18369)
 assert(ok and title=="Call of The World Tree (2)", "Wrong 18369 title")
 GetOngoingDescription(18369)
 GetOngoingRewardInfo(18369)
 local shasha=table.concat(QuestInfoList[18369].Description,"\n")
 assert(shasha:find("Lapine Shasha",1,true) and shasha:find("ygg_fruit,80,122,0,101,0",1,true))
 local gate=table.concat(assert(QuestInfoList[18379]).Description,"\n")
 assert(gate:find("hem_fild,180,263,0,101,0",1,true) and gate:find("Enter",1,true) and gate:find("ch1_sf03 122,255",1,true), "Hazy Gate route differs from real entrance")
 local complete=table.concat(assert(QuestInfoList[18377]).Description,"\n")
 assert(complete:find("completed Chapter 1",1,true) and complete:find("not a new assignment",1,true), "Completion marker became an active objective")
 local fire=table.concat(assert(QuestInfoList[18376]).Description,"\n")
 assert(fire:find("mu_fild01,95,154,0,101,0",1,true), "Land of Fire bypasses its public entrance")
 for _,id in ipairs(IDS) do
  assert(type(QuestInfoList[id])=="table", "Missing server Chapter 1 quest: "..id)
  GetOngoingQuestInfoByID(id); GetOngoingDescription(id); GetOngoingRewardInfo(id)
 end
 for id,old in pairs(before) do assert(equal(old,QuestInfoList[id]), "Existing quest changed: "..id) end
 for id in pairs(QuestInfoList) do assert(before[id] or allowed[id], "Unrelated quest added: "..id) end
 local once=clone(QuestInfoList)
 if PATCH then dofile(PATCH) end
 assert(equal(once,QuestInfoList), "Repeated patch load changed records")
 local scoped={}; for _,id in ipairs(IDS) do scoped[id]=clone(QuestInfoList[id]) end
 if reference then assert(equal(reference,scoped), "Chapter 1 entry point diverged") end
 reference=scoped
 assert(descriptions>0)
 print("PASS "..loader.."; "..#IDS.." server quest IDs; original helper callbacks")
end
if PATCH then
 local custom={Title="Owner's custom Chapter 1 guide",Description={"Preserve custom guide"}}
 QuestInfoList[18369]=custom
 dofile(PATCH)
 assert(QuestInfoList[18369]==custom, "Custom override replaced")
 assert(QuestInfoList[999999999]==nil, "Unknown-ID blanket fallback")
end
'''.replace('IDS', '{'+','.join(map(str, ids))+'}').replace('PATCH', json.dumps(args.patch.resolve().as_posix()) if args.patch else 'nil').replace('HELPER', json.dumps(args.helper.resolve().as_posix()))
    # Run a copy of the real loader graph; never modify a running/installed client.
    with tempfile.TemporaryDirectory(prefix='pn-chapter1-lua-') as name:
        staged = Path(name)
        files = ('SystemEN/OngoingQuests.lub', 'SystemEN/QuestNavigationRepair.lua',
                 'SystemEN/EpisodeQuestNavigation.lua', 'SystemEN/ExtendedMemoQuests.lua',
                 'SystemEN/OngoingQuestInfoList.lub', 'System/OngoingQuestInfoList.lub',
                 'System/OngoingQuestInfoList_True.lub', 'SystemEN/Chapter1QuestInfo.lua',
                 'SystemEN/Chapter1GuideRecords.lua')
        for relative in files:
            source = args.client / relative
            if source.exists():
                target = staged / relative; target.parent.mkdir(exist_ok=True, parents=True)
                shutil.copyfile(source, target)
        if args.patch:
            companion = args.patch.with_name('Chapter1GuideRecords.lua')
            if companion.exists(): shutil.copyfile(companion, staged / 'SystemEN/Chapter1GuideRecords.lua')
        result = subprocess.run([str(args.lua.resolve()), '-'], input=lua.encode(), cwd=staged, capture_output=True)
    print(result.stdout.decode(errors='replace'), end='')
    if result.returncode:
        raise SystemExit(result.stderr.decode(errors='replace'))


if __name__ == '__main__': main()
