-- BEGIN RATHENA_PN QUEST COMPATIBILITY PATCH
-- Append after the closing brace of SystemEN/OngoingQuests.lub.

local function PNCompatibilityQuest(id, title, description, summary)
	QuestInfoList[id] = {
		Title = title,
		IconName = "ico_nq.bmp",
		Description = { description },
		Summary = summary
	}
end

PNCompatibilityQuest(
	900200,
	"Gusli",
	"Receive the traditional Gusli from <NAVI>[Aged Stranger]<INFO>mosk_fild01,86,104,0,101,0</INFO></NAVI> on Whale Island and learn how to play it.",
	"Learn to play the Gusli"
)

PNCompatibilityQuest(
	900201,
	"Gusli",
	"Return to Moscovia and play the Gusli for the Csar in the palace.",
	"Perform for the Csar"
)

PNCompatibilityQuest(
	900202,
	"Shafka",
	"Bring 20 Nine Tails, 10 Yarn, 10 Soft Silk, 20 Sea-otter Fur, and 1 Spool to <NAVI>[Irina]<INFO>moscovia,211,93,0,101,0</INFO></NAVI>.",
	"Gather materials for a Shafka"
)

-- Chapter 2 uses local compatibility quest IDs because the matching late-kRO
-- OngoingQuests data is not present in the supported client bundle.
PNCompatibilityQuest(
	27101,
	"Chapter 2 - Fragments of the End",
	"Answer the World Tree's call, aid Kindlebrook and Shelter Jemis, investigate Ragsroot, and stop the Phantom of Nyrholt.",
	"Complete the Chapter 2 story"
)

PNCompatibilityQuest(
	27102,
	"Chapter 2 - Break the Shadow Jail",
	"Defeat 20 Shadow Jailers while following the expedition beyond Kindlebrook.",
	"Defeat 20 Shadow Jailers"
)

PNCompatibilityQuest(
	27103,
	"Chapter 2 - A Dormant Matter",
	"Defeat another 20 Shadow Jailers for One's research into the dormant matter.",
	"Defeat 20 Shadow Jailers"
)

PNCompatibilityQuest(
	27104,
	"Chapter 2 - Distorted Ragsroot",
	"Defeat 100 creatures in Distorted Ragsroot and report the result to the expedition.",
	"Defeat 100 Distorted Ragsroot monsters"
)

PNCompatibilityQuest(
	27110,
	"Kindlebrook Noodle Soup",
	"Defeat 10 Dragon Clams in Volund Plains and bring 5 Crispy Dragonclam Meat to Tamarin.",
	"10 Dragon Clams and 5 Crispy Dragonclam Meat"
)

PNCompatibilityQuest(
	27111,
	"Hotter! Even Hotter!",
	"Defeat 10 Flame Bulls in Volund Plains and bring 5 Burning Hide.",
	"10 Flame Bulls and 5 Burning Hide"
)

PNCompatibilityQuest(
	27112,
	"Hot Pup! Even Hotter Pup!",
	"Defeat 10 Logidogis in Volund Plains or Volund Forest and bring 5 Old Snacks.",
	"10 Logidogis and 5 Old Snacks"
)

PNCompatibilityQuest(
	27113,
	"Dream of Perpetual Motion",
	"Defeat 15 Dudulsons, 15 Garaksons, and 15 Tuktaksons in Primordial Flame for Nedim.",
	"Defeat 15 of each Primordial Flame target"
)

PNCompatibilityQuest(
	27114,
	"Retrieve 10 Hal Puffs",
	"Collect 10 Hal Puffs from the hand monsters in Primordial Flame and return them to Lutem.",
	"Collect 10 Hal Puffs"
)

PNCompatibilityQuest(
	27115,
	"Jemis' Parts Delivery",
	"Defeat 10 Alpurings and 10 Icy Blasts, collect 5 Ice Jelly and 5 Frost Shards, and recover the marked Frozen Material for Karillon.",
	"Recover and deliver Jemis' repair materials"
)

PNCompatibilityQuest(
	27116,
	"Heat Preservation Equipment Research",
	"Defeat 10 Fire Cotton in Volund Forest and bring 3 Fluffy Embers to Samra.",
	"10 Fire Cotton and 3 Fluffy Embers"
)

PNCompatibilityQuest(
	27117,
	"Even a Handful of Mana",
	"Gather 10 Phantom Herbs from Grass Piles in Distorted Ragsroot and return them to Devries.",
	"Gather 10 Phantom Herbs"
)

PNCompatibilityQuest(
	27118,
	"I Understand, So Calm Down",
	"Defeat 100 monsters in Distorted Ragsroot and report to Gregor.",
	"Defeat 100 Distorted Ragsroot monsters"
)

PNCompatibilityQuest(
	27119,
	"Phantom of Nyrholt",
	"Enter Phantom of Nyrholt with your party and defeat the Snapdragon Phantom.",
	"Clear Phantom of Nyrholt"
)

-- END RATHENA_PN QUEST COMPATIBILITY PATCH
