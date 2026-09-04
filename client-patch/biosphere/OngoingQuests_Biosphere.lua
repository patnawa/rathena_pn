-- BEGIN RATHENA_PN BIOSPHERE QUEST PATCH
-- Append after the closing brace of SystemEN/OngoingQuests.lub.

local function BiosphereQuest(id, title, description, summary, cooldown)
	QuestInfoList[id] = {
		Title = title,
		IconName = "ico_dq.bmp",
		Description = { description },
		Summary = summary or ""
	}
	if cooldown then
		QuestInfoList[id].CoolTimeQuest = 1
	end
end

-- The translated kRO file points the four original hunts at an obsolete
-- location. Research Admin Beta is at ba_in01 252,353 on this server.
for _, id in ipairs({ 17608, 17609, 17610, 17611, 17612, 17613, 17614, 17615 }) do
	if QuestInfoList[id] and QuestInfoList[id].Description then
		for line, text in ipairs(QuestInfoList[id].Description) do
			QuestInfoList[id].Description[line] = string.gsub(text, "ba_in01,309,44", "ba_in01,252,353")
		end
	end
end

local depth1 = {
	{ "Fire", 16739, 16740, 16741, 16742, 1001330, "Deep Fire Specimen", "BIO_LAVA_TOAD", "Deep Lava Toad", "BIO_FIRE_FRILLDORA", "Deep Fire Frilldora", 286, 114 },
	{ "Ice", 16743, 16744, 16745, 16746, 1001332, "Deep Ice Specimen", "BIO_ANOLIAN", "Deep Anolian", "BIO_KAPHA", "Deep Kapha", 286, 116 },
	{ "Earth", 16747, 16748, 16749, 16750, 1001331, "Deep Earth Specimen", "BIO_STING", "Deep Sting", "BIO_WOOD_GOBLIN", "Deep Wood Goblin", 286, 118 },
	{ "Storm", 16751, 16752, 16753, 16754, 1001333, "Deep Storm Specimen", "BIO_DRAGON_TAIL", "Deep Dragon Tail", "BIO_LITTLE_FATUM", "Deep Little Fatum", 286, 120 },
	{ "Purification", 16755, 16756, 16757, 16758, 1001335, "Deep Purification Specimen", "BIO_HOLY_FRUS", "Deep Holy Frus", "BIO_HOLY_SKOGUL", "Deep Holy Skogul", 283, 114 },
	{ "Corruption", 16759, 16760, 16761, 16762, 1001336, "Deep Corruption Specimen", "BIO_SKEL_ARCHER", "Deep Skeleton Archer", "BIO_SKEL_SOLDIER", "Deep Skeleton Soldier", 283, 116 },
	{ "Soul", 16763, 16764, 16765, 16766, 1001334, "Deep Soul Specimen", "BIO_EMPATHIZER", "Deep Empathizer", "BIO_PRAY_GIVER", "Deep Pray Giver", 283, 118 },
	{ "Venom", 16767, 16768, 16769, 16770, 1001337, "Deep Venom Specimen", "BIO_PINGUICULA_D", "Deep Pinguicula", "BIO_POM_SPIDER", "Deep Pom Spider", 283, 120 }
}

for _, q in ipairs(depth1) do
	local admin = "<NAVI>[" .. q[1] .. " Research Administrator]<INFO>ba_in01," .. q[12] .. "," .. q[13] .. ",0,101,0</INFO></NAVI>"
	BiosphereQuest(
		q[2],
		"Depth 1 " .. q[1] .. " Sample Research I",
		"Collect 30 <ITEM>[" .. q[7] .. "]<INFO>" .. q[6] .. "</INFO></ITEM> on Biosphere Depth 1 and report to " .. admin .. ".",
		"Collect 30 specimens"
	)
	BiosphereQuest(
		q[3],
		"[Standby] Depth 1 " .. q[1] .. " Sample Research I",
		"This daily assignment can be accepted from " .. admin .. " again after the cooldown.",
		"Resets at 4 AM",
		true
	)
	BiosphereQuest(
		q[4],
		"Depth 1 " .. q[1] .. " Research II",
		"Defeat 80 <NAVI>[" .. q[9] .. "]<INFO>" .. q[8] .. ",0,0,3,-222,1</INFO></NAVI> and 80 <NAVI>[" .. q[11] .. "]<INFO>" .. q[10] .. ",0,0,3,-222,1</INFO></NAVI> on Biosphere Depth 1, then report to " .. admin .. ".",
		"Hunt 80 of each monster"
	)
	BiosphereQuest(
		q[5],
		"[Standby] Depth 1 " .. q[1] .. " Research II",
		"This daily assignment can be accepted from " .. admin .. " again after the cooldown.",
		"Resets at 4 AM",
		true
	)
end

local regular = {
	{ 900100, 900101, "Biosphere - Soul", "Soul" },
	{ 900102, 900103, "Biosphere - Venom", "Venom" },
	{ 900104, 900105, "Biosphere - Temple", "Temple" }
}

for _, q in ipairs(regular) do
	BiosphereQuest(
		q[1],
		q[3],
		"Defeat 100 monsters inside the " .. q[4] .. " habitat, then report to <NAVI>[Research Admin Beta]<INFO>ba_in01,252,353,0,101,0</INFO></NAVI>.",
		"Hunt 100 monsters"
	)
	BiosphereQuest(
		q[2],
		"[Standby] " .. q[3],
		"This daily assignment can be accepted from <NAVI>[Research Admin Beta]<INFO>ba_in01,252,353,0,101,0</INFO></NAVI> again after the cooldown.",
		"Resets at 4 AM",
		true
	)
end

BiosphereQuest(
	900106,
	"Depth Abyss Extended Research",
	"Defeat 3,000 monsters on Biosphere Depth 2, then report to <NAVI>[Depth Abyss Research Manager]<INFO>ba_chess,25,13,0,101,0</INFO></NAVI>.",
	"Hunt 3,000 monsters"
)
BiosphereQuest(
	900107,
	"[Standby] Depth Abyss Extended Research",
	"This daily assignment can be accepted from <NAVI>[Depth Abyss Research Manager]<INFO>ba_chess,25,13,0,101,0</INFO></NAVI> again after the cooldown.",
	"Resets at 4 AM",
	true
)

-- END RATHENA_PN BIOSPHERE QUEST PATCH
