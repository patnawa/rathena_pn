-- Loads a built achievement table in real Lua 5.1 with the client's loader API
-- stubbed, then requires every row, existing and arcade, to register correctly.
-- usage: lua5.1 verify.lua <achievement table> <expected arcade rows>
local seen, resources, tabs = {}, {}, {}
function CreateAchievementInfo() return true, "" end
local function ok() return true, "good" end
function AddAchieve(id, title, summary, details, score) seen[id] = { title = title, score = score }; return true, "" end
setmetatable(_G, { __index = function(_, name)
	return function(id, a, b)
		if name == "InsertAchieveInfo" or name == "InsertAchieve" then seen[id] = seen[id] or {} end
		if name == "SetAchieveIDByTab" then tabs[id] = a .. "/" .. b end
		if name:find("Resource") then resources[id] = true end
		return true, "good"
	end
end })
local chunk = assert(loadfile(arg[1]))
chunk()
assert(type(achievement_tbl) == "table", "achievement_tbl is missing")
local result, message = main()
assert(result == true, "main failed: " .. tostring(message))
local total, arcade = 0, 0
for id, row in pairs(achievement_tbl) do
	total = total + 1
	assert(tabs[id], "row did not reach SetAchieveIDByTab: " .. id)
	if id >= 300000 and id < 301000 then
		arcade = arcade + 1
		assert(row.score == 0, "arcade row has a score: " .. id)
		assert(next(row.reward) == nil, "arcade row has a reward: " .. id)
		assert(row.title and row.content.summary and row.resource[1].text, "arcade row is incomplete: " .. id)
		assert(tabs[id] == "6/0", "arcade row is in the wrong tab: " .. id)
	end
end
assert(arcade == tonumber(arg[2]), "expected " .. arg[2] .. " arcade rows, found " .. arcade)
print("PASS total=" .. total .. " arcade=" .. arcade)
