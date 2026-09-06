-- Clean-room Druid Gear enchant metadata; effects follow the permitted MuhRO reference.
-- Uses the existing generic EpisodClear20 resource, without importing artwork.
-- Load as tbl_druidgear through the existing multi-itemInfo loader.
local items = {}
local function add(id, name, skill)
  items[id] = {
    unidentifiedDisplayName = name,
    unidentifiedResourceName = "EpisodClear20",
    unidentifiedDescriptionName = { "" },
    identifiedDisplayName = name,
    identifiedResourceName = "EpisodClear20",
    identifiedDescriptionName = {
      skill .. " damage +5%.",
      "Grade D or higher: an additional +3% " .. skill .. " damage.",
      "Grade C or higher: physical damage against all sizes +10%.",
      "Grade B or higher: an additional +4% " .. skill .. " damage.",
      "Grade A or higher: an additional +6% " .. skill .. " damage.",
      "Grade bonuses are cumulative.",
      "_______________________",
      "^0000CCType:^000000 Enchant",
      "^0000CCWeight:^000000 0"
    },
    slotCount = 0, ClassNum = 0, costume = false
  }
end

add(314269, "Pinion Shot Tuning Device", "Pinion Shot")
add(314270, "Quill Spear Tuning Device", "Quill Spear")

tbl_druidgear = items
return items
