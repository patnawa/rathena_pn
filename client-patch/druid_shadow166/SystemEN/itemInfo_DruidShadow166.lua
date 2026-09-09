-- Clean-room metadata. Sources and limits: doc/druid_shadow166_audit.md.
-- Existing generic art only; no reference server resource/texture is imported.
local items = {}
local function add(id, name, descriptions)
  items[id] = {
    unidentifiedDisplayName = name,
    unidentifiedResourceName = "EpisodClear20",
    unidentifiedDescriptionName = { "" },
    identifiedDisplayName = name,
    identifiedResourceName = "EpisodClear20",
    identifiedDescriptionName = descriptions,
    slotCount = 0, ClassNum = 0, costume = false
  }
end
local function shadow(id, location, partner, trio)
  add(id, "M. Alitea Shadow " .. location, {
    "Max HP +10 per refine level.",
    "With Master Shadow " .. partner .. ": all six trait stats +2.",
    "With " .. trio .. ": P.ATK +1 and S.MATK +1.",
    "If those three pieces have a combined refine level of 27 or higher:",
    "ignore 50% physical and magical defense of all races, excluding players.",
    "With Master Shadow Weapon and Shield, and all four M. Alitea Shadow pieces:",
    "ignore 20% physical and magical resistance of all races, excluding players.",
    "_______________________",
    "^0000CCType:^000000 Shadow Equipment",
    "^0000CCLocation:^000000 " .. location,
    "^0000CCRequired Level:^000000 200",
    "^0000CCJobs:^000000 Alitea",
    "^0000CCWeight:^000000 0"
  })
end
shadow(1270183, "Armor", "Shield", "Master Shadow Shield and M. Alitea Shadow Armor and Shoes")
shadow(1270184, "Shoes", "Shield", "Master Shadow Shield and M. Alitea Shadow Armor and Shoes")
shadow(1270185, "Earring", "Weapon", "Master Shadow Weapon and M. Alitea Shadow Earring and Pendant")
shadow(1270186, "Pendant", "Weapon", "Master Shadow Weapon and M. Alitea Shadow Earring and Pendant")

local function soul(id, skill)
  add(id, skill .. " Soul", {
    skill .. " damage +2%.",
    "An additional +1% " .. skill .. " damage per 2 refine levels of the enchanted equipment.",
    "_______________________",
    "^0000CCType:^000000 Enchant",
    "^0000CCWeight:^000000 0"
  })
end
soul(314804, "Alpha Claw")
soul(314805, "Frenzy Fang")
soul(314806, "Pinion Shot")
soul(314807, "Quill Spear")
soul(314808, "Glacial Shard")
soul(314809, "Roaring Piercer")
soul(314810, "Terra Harvest")

tbl_druidshadow166 = items
return items
