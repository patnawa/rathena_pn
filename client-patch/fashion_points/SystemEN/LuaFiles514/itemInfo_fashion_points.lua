-- Fashion Points itemInfo metadata fragment.
-- Clean-room metadata from the published item names/effects at
-- https://wiki.muhro.eu/Costume_Enchants_(Fashion_Points)
--
-- This file returns a table. Merge each returned [item_id] record into the
-- client's main itemInfo table; stock clients do not auto-load this filename.
-- Exact proprietary icon resources are unavailable locally, so these entries
-- intentionally reuse safe resources already shipped by this client:
--   boxes    -> Mystical Card Album (item 12246)
--   stones   -> Soul Fragment style resource (item 1001326)
--   enchants -> card/enchant resource (item 312402)

local items = {}
local BOX_ICON = "³°ÀºÄ«µåÃ¸"
local STONE_ICON = "¿µÈ¥ÀÇÁ¶°¢"
local ENCHANT_ICON = "ÇÇ±Ô¾î"

local function add(id, name, resource, description)
	items[id] = {
		unidentifiedDisplayName = name,
		unidentifiedResourceName = resource,
		unidentifiedDescriptionName = { "" },
		identifiedDisplayName = name,
		identifiedResourceName = resource,
		identifiedDescriptionName = description,
		slotCount = 0,
		ClassNum = 0,
		costume = false
	}
end

local boxes = {
	{41090, "Top Box 1", 50}, {41091, "Top Box 2", 50},
	{41092, "Top Box 3", 50}, {41093, "Top Box 4", 50},
	{41287, "Top Box 5", 50}, {41094, "Mid Box 1", 50},
	{41095, "Mid Box 2", 50}, {41096, "Mid Box 3", 50},
	{41097, "Mid Box 4", 50}, {41288, "Mid Box 5", 50},
	{41098, "Low Box 1", 50}, {41099, "Low Box 2", 50},
	{41100, "Low Box 3", 50}, {41101, "Low Box 4", 50},
	{41289, "Low Box 5", 50}, {41102, "Garment Box 1", 50},
	{41103, "Garment Box 2", 50}, {41104, "Garment Box 3", 50},
	{41255, "Garment Box 4", 50}, {41290, "Garment Box 5", 50},
	{41105, "Garment 2nd-slot Box 1", 300}
}

for _, box in ipairs(boxes) do
	add(box[1], box[2], BOX_ICON, {
		"Contains one published costume enchant stone.",
		"One outcome is selected uniformly by server compatibility policy.",
		"MuhRO's original per-item probabilities are not published.",
		"_______________________",
		"^0000CCFashion Point cost:^000000 " .. box[3],
		"^0000CCType:^000000 Container",
		"^0000CCWeight:^000000 0"
	})
end

local physical_stones = {
	{1002625, "Karnos Stone (Garment)", "Awakens Karnos potential in a Costume Garment slot."},
	{1002626, "Alitea Stone (Garment)", "Awakens Alitea potential in a Costume Garment slot."},
	{1002627, "Druid Stone (Top)", "Awakens Druid potential in a Costume Top Headgear slot."},
	{1002628, "Druid Stone (Mid)", "Awakens Druid potential in a Costume Mid Headgear slot."},
	{1002629, "Druid Stone (Low)", "Awakens Druid potential in a Costume Low Headgear slot."},
	{1002630, "Karnos Stone (Top)", "Awakens Karnos potential in a Costume Top Headgear slot."},
	{1002631, "Karnos Stone (Mid)", "Awakens Karnos potential in a Costume Mid Headgear slot."},
	{1002632, "Karnos Stone (Low)", "Awakens Karnos potential in a Costume Low Headgear slot."}
}

for _, stone in ipairs(physical_stones) do
	add(stone[1], stone[2], STONE_ICON, {
		stone[3],
		"Take this stone and a compatible costume to a costume enchanter.",
		"_______________________",
		"^0000CCType:^000000 Enchant material",
		"^0000CCWeight:^000000 0"
	})
end

add(314848, "Karnos Stone (Garment)", ENCHANT_ICON, {
	"Chop Chop damage +10%.", "Feather Sprinkle damage +10%.",
	"Set effects are applied with matching Druid/Karnos head stones.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314849, "Alitea Stone (Garment)", ENCHANT_ICON, {
	"Physical damage to all properties +Sixth Sense skill level%.",
	"Magical damage to all properties +Nature Aid skill level%.",
	"Set effects are applied with matching Karnos head stones.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314850, "Druid Stone (Top)", ENCHANT_ICON, {
	"MATK +3 per Nature's Logic level.",
	"Base ATK +2 per Beastly Nose and Sharp Eyes level.",
	"Wind Bomb and Around Flower damage +15%.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314851, "Druid Stone (Mid)", ENCHANT_ICON, {
	"Ice Cloud, Low Flight and Cruel Bite damage +15%.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314852, "Druid Stone (Low)", ENCHANT_ICON, {
	"Earth Flower, No Mercy Claw and Flicking Tornado damage +15%.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314853, "Karnos Stone (Top)", ENCHANT_ICON, {
	"Water, Wind and Earth magical damage +2% per Nature Vigor level.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314854, "Karnos Stone (Mid)", ENCHANT_ICON, {
	"Melee physical damage +1% per Wolf Instinct level.",
	"_______________________", "^0000CCType:^000000 Enchant"
})
add(314855, "Karnos Stone (Low)", ENCHANT_ICON, {
	"Ranged physical damage +1% per Raptorial Instinct level.",
	"_______________________", "^0000CCType:^000000 Enchant"
})

return items
