-- Clean-room Druid-family item metadata; not original Gravity artwork/text.
-- Effects follow the supplied reference server reference. See doc/druid_item_compatibility.md.
-- Generic known-safe icon only; does not install official item artwork.
-- Load as tbl_druiditems using the existing multi-itemInfo loader.
local items = {}
local function add(id, name, description)
  items[id] = {
    unidentifiedDisplayName = name,
    unidentifiedResourceName = "EpisodClear20",
    unidentifiedDescriptionName = { "" },
    identifiedDisplayName = name,
    identifiedResourceName = "EpisodClear20",
    identifiedDescriptionName = description,
    slotCount = 0, ClassNum = 0, costume = false
  }
end

add(314271, "Automatic Orb (Double Slash)", {
  "Double Slash, Chop Chop: skill damage +15%.",
  "At refine +9: an additional +3%; at +11: another +7%.",
  "Druid-family reference compatibility item."
})
add(314272, "Automatic Orb (Sharpen Hail)", {
  "Sharpen Hail, Sharpen Gust: skill damage +15%.",
  "At refine +9: an additional +3%; at +11: another +7%.",
  "Druid-family reference compatibility item."
})
add(314273, "Automatic Orb (Ice Splash)", {
  "Ice Splash, Thundering Orb, Earth Stamp: skill damage +15%.",
  "At refine +9: an additional +3%; at +11: another +7%.",
  "Druid-family reference compatibility item."
})
add(314274, "Wolf Orb (Double Slash)", {
  "Double Slash, Chop Chop: skill damage +15%.",
  "At refine +9 and +11: an additional +15% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314275, "Wolf Orb (Sharpen Hail)", {
  "Sharpen Hail, Sharpen Gust: skill damage +15%.",
  "At refine +9 and +11: an additional +15% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314276, "Wolf Orb (Ice Splash)", {
  "Ice Splash, Thundering Orb, Earth Stamp: skill damage +15%.",
  "At refine +9 and +11: an additional +15% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314277, "Glacier Flower (Chop Chop)", {
  "Chop Chop: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314278, "Glacier Flower (Double Slash)", {
  "Double Slash: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314279, "Glacier Flower (Sharpen Gust)", {
  "Sharpen Gust: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314280, "Glacier Flower (Sharpen Hail)", {
  "Sharpen Hail: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314281, "Glacier Flower (Feather Sprinkle)", {
  "Feather Sprinkle: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314282, "Glacier Flower (Ice Splash)", {
  "Ice Splash: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314283, "Glacier Flower (Thundering Orb)", {
  "Thundering Orb: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314284, "Glacier Flower (Thundering Focus)", {
  "Thundering Focus: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314285, "Glacier Flower (Earth Drill)", {
  "Earth Drill: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314286, "Glacier Flower (Earth Stamp)", {
  "Earth Stamp: skill damage +20%.",
  "Every 3 refine levels: an additional +10%.",
  "At refine +9 and +11: an additional +20% at each threshold.",
  "Druid-family reference compatibility item."
})
add(314287, "Glacier Flower (Glacial Nova)", {
  "Glacial Nova: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314288, "Glacier Flower (Glacial Shard)", {
  "Glacial Shard: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314289, "Glacier Flower (Roaring Piercer)", {
  "Roaring Piercer: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314290, "Glacier Flower (Terra Wave)", {
  "Terra Wave: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314291, "Glacier Flower (Terra Harvest)", {
  "Terra Harvest: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314292, "Glacier Flower (Quill Spear)", {
  "Quill Spear: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314293, "Glacier Flower (Pinion Shot)", {
  "Pinion Shot: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314294, "Glacier Flower (Frenzy Fang)", {
  "Frenzy Fang: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314295, "Glacier Flower (Primal/Feral/Alpha Claw)", {
  "Primal/Feral/Alpha Claw: skill damage +10%.",
  "Every 4 refine levels: an additional +5%.",
  "At refine +9: an additional +10%.",
  "Druid-family reference compatibility item."
})
add(314296, "Ice Magic Orb (Double Slash)", {
  "Double Slash, Chop Chop: skill damage +15%.",
  "At refine +7, +9 and +11: an additional +15% at each threshold.",
  "At refine +11: Alpha Claw, Frenzy Fang skill damage +15%.",
  "Druid-family reference compatibility item."
})
add(314297, "Ice Magic Orb (Sharpen Hail)", {
  "Sharpen Hail, Sharpen Gust: skill damage +15%.",
  "At refine +7, +9 and +11: an additional +15% at each threshold.",
  "At refine +11: Pinion Shot, Quill Spear skill damage +15%.",
  "Druid-family reference compatibility item."
})
add(314298, "Ice Magic Orb (Ice Splash)", {
  "Ice Splash, Thundering Orb, Earth Stamp: skill damage +15%.",
  "At refine +7, +9 and +11: an additional +15% at each threshold.",
  "At refine +11: Glacial Shard, Roaring Piercer, Terra Harvest skill damage +15%.",
  "Druid-family reference compatibility item."
})
add(1002350, "Ice Magic Stone (Double Slash)", {
  "Material for Ice Magic Orb (Double Slash), item 314296.",
  "Consumes one stone in the matching supported enchant recipe.",
  "This material does not grant a combat bonus by itself.",
  "Druid-family reference compatibility item."
})
add(1002351, "Ice Magic Stone (Sharpen Hail)", {
  "Material for Ice Magic Orb (Sharpen Hail), item 314297.",
  "Consumes one stone in the matching supported enchant recipe.",
  "This material does not grant a combat bonus by itself.",
  "Druid-family reference compatibility item."
})
add(1002352, "Ice Magic Stone (Ice Splash)", {
  "Material for Ice Magic Orb (Ice Splash), item 314298.",
  "Consumes one stone in the matching supported enchant recipe.",
  "This material does not grant a combat bonus by itself.",
  "Druid-family reference compatibility item."
})

tbl_druiditems = items
return items
