#!/usr/bin/env python3
"""Run the real quest loader and verify the reviewed legacy navigation repairs."""
import argparse
import json
from pathlib import Path
import subprocess


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--lua", type=Path, required=True, help="Native Lua 5.1 executable")
    parser.add_argument("--client", type=Path, required=True, help="Installed client root")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    patch = root / "client-patch/navigation_repair/SystemEN/QuestNavigationRepair.lua"
    lua = r'''
-- The game can expose a restricted table library in its quest Lua state.
table.insert = nil
dofile("SystemEN/OngoingQuests.lub")
local function clone(v)
  if type(v) ~= "table" then return v end
  local copy = {}; for k, x in pairs(v) do copy[k] = clone(x) end; return copy
end
local function equal(a, b)
  if type(a) ~= type(b) then return false end
  if type(a) ~= "table" then return a == b end
  for k, v in pairs(a) do if not equal(v, b[k]) then return false end end
  for k in pairs(b) do if a[k] == nil then return false end end
  return true
end
local before = clone(QuestInfoList)
dofile(PATCH)
local repaired = {[8846]=true,[17279]=true,[17280]=true,[14995]=true,[16147]=true,[8886]=true,[8916]=true,[21946]=true}
local count = 0
for id, quest in pairs(QuestInfoList) do
  count = count + 1
  assert(before[id], "unexpected added quest")
  if repaired[id] then
    local copy = clone(quest); copy.Description = before[id].Description
    assert(equal(copy, before[id]), "non-description quest data changed: " .. id)
  else assert(equal(quest, before[id]), "unrelated quest changed: " .. id) end
end
for id in pairs(before) do assert(QuestInfoList[id], "quest was removed") end
assert(table.concat(QuestInfoList[8846].Description):find("jor_sanct,92,139,0,101,0", 1, true))
for _, id in ipairs({17279,17280}) do
  assert(table.concat(QuestInfoList[id].Description):find("yuno_pre,95,71,0,101,0", 1, true))
end
for _, id in ipairs({14995,16147,8886,8916,21946}) do
  local text = table.concat(QuestInfoList[id].Description, "\n")
  assert(not text:find("<NAVI>", 1, true), "inactive route still clickable")
  assert(text:find("This quest is not available on this server.", 1, true))
end
local battle = QuestInfoList[16400]
assert(#battle.RewardItemList == 3)
local rewards = {}; for _, item in ipairs(battle.RewardItemList) do rewards[item.ItemID] = item.ItemNum end
assert(rewards[25786] == 9 and rewards[25787] == 15 and rewards[102571] == 1)
assert(table.concat(battle.Description):find("one random EDDA weapon", 1, true))
local expedition = QuestInfoList[16399].RewardItemList
assert(#expedition == 1 and expedition[1].ItemID == 25787 and expedition[1].ItemNum == 2)
local once = clone(QuestInfoList)
dofile(PATCH)
assert(equal(QuestInfoList, once), "reloading creates duplicate notes or changes")
print("PASS: " .. count .. " real quest records retained; 3 corrected destinations; 5 inactive guides; repeat load safe")
'''.replace("PATCH", json.dumps(patch.as_posix()))
    result = subprocess.run([str(args.lua.resolve()), "-"], input=lua, text=True, cwd=args.client, capture_output=True)
    print(result.stdout, end="")
    if result.returncode:
        raise SystemExit(result.stderr)


if __name__ == "__main__":
    main()
