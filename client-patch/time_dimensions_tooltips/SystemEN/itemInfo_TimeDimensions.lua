-- Reviewed Time Dimensions autocast descriptions, 2026-09-22.
-- Facts and Gravity source links: corrections.json and
-- doc/dimension_other_jobs_audit_20260922.md.
-- Loaded after existing item merges; resources and other fields stay intact.
local repairs = {
  [510139] = {
    {"Casting ^009900Deft Stab^000000 has a chance to auto cast Level 1 ^009900Abyss Dagger^000000.", "Using ^009900Deft Stab^000000 autocasts ^009900Abyss Dagger^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [530054] = {
    {"Casting ^009900Overslash^000000 has a chance to auto cast Level 1 ^009900Overbrand^000000.", "Using ^009900Overslash^000000 autocasts ^009900Overbrand^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [540079] = {
    {"Casting ^009900Diamond Storm^000000 has a chance to auto cast Level 1 ^009900Elemental Buster^000000.", "Using ^009900Diamond Storm^000000 autocasts ^009900Elemental Buster^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(Requires Elemental Buster Level 1 or higher and a summoned high elemental.)"},
  },
  [540080] = {
    {"Casting ^009900Terra Drive^000000 has a chance to auto cast Level 1 ^009900Elemental Buster^000000.", "Using ^009900Terra Drive^000000 autocasts ^009900Elemental Buster^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(Requires Elemental Buster Level 1 or higher and a summoned high elemental.)"},
  },
  [540082] = {
    {"Casting ^009900Dawn Break^000000 has a chance to auto cast Level 1 ^009900Midnight Kick^000000.", "Using ^009900Dawn Break^000000 autocasts ^009900Midnight Kick^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [540083] = {
    {"Casting ^009900Sunset Blast^000000 has a chance to auto cast Level 1 ^009900Noon Blast^000000.", "Using ^009900Sunset Blast^000000 autocasts ^009900Noon Blast^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [550130] = {
    {"Casting ^009900Arbitrium^000000 has a chance to auto cast Level 1 ^009900Adoramus^000000.", "Using ^009900Arbitrium^000000 autocasts ^009900Adoramus^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [550131] = {
    {"Casting ^009900Talisman of Blue Dragon^000000 has a chance to auto cast Level 1 ^009900Talisman of Red Phoenix^000000.", "Using ^009900Talisman of Blue Dragon^000000 autocasts ^009900Talisman of Red Phoenix^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [550132] = {
    {"Casting ^009900Talisman of Blue Dragon^000000 has a chance to auto cast Level 1 ^009900Talisman of Black Tortoise^000000.", "Using ^009900Talisman of White Tiger^000000 autocasts ^009900Talisman of Black Tortoise^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [550133] = {
    {"Casting ^009900Napalm Vulcan Strike^000000 has a 50% chance to auto cast Level 5 ^009900Soul Vulcan Strike^000000.", "Using ^009900Napalm Vulcan Strike^000000 has a 50% chance to autocast ^009900Soul Vulcan Strike^000000 at Level 5."},
  },
  [560060] = {
    {"Casting ^009900First Brand^000000 has a chance to auto cast Level 1 ^009900Second Flame^000000.", "Using ^009900First Brand^000000 autocasts ^009900Second Flame^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
    {"Casting ^009900Second Flame^000000 has a chance to auto cast Level 1 ^009900Third Flame Bomb^000000.", "Using ^009900Second Flame^000000 autocasts ^009900Third Flame Bomb^000000 at the highest learned level."},
  },
  [570062] = {
    {"Casting ^009900Metallic Fury^000000 has a chance to auto cast Level 1 ^009900Reverberation^000000.", "Using ^009900Metallic Fury^000000 autocasts ^009900Reverberation^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [570063] = {
    {"Casting ^009900Rhythm Shooting^000000 has a chance to auto cast Level 1 ^009900Sound Blend^000000.", "Using ^009900Rhythm Shooting^000000 autocasts ^009900Sound Blend^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [580061] = {
    {"Casting ^009900Metallic Fury^000000 has a chance to auto cast Level 1 ^009900Reverberation^000000.", "Using ^009900Metallic Fury^000000 autocasts ^009900Reverberation^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [580062] = {
    {"Casting ^009900Rhythm Shooting^000000 has a chance to auto cast Level 1 ^009900Sound Blend^000000.", "Using ^009900Rhythm Shooting^000000 autocasts ^009900Sound Blend^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [590080] = {
    {"Casting ^009900Explosive Powder^000000 has a chance to auto cast Level 1 ^009900Cart Tornado^000000.", "Using ^009900Explosive Powder^000000 autocasts ^009900Cart Tornado^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [600054] = {
    {"Casting ^009900Hack and Slasher^000000 has a chance to auto cast Level 1 ^009900Ignition Break^000000.", "Using ^009900Hack and Slasher^000000 autocasts ^009900Ignition Break^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [610065] = {
    {"Casting ^009900Phantom Menace^000000 has a chance to auto cast the higher Level learned ^009900Rolling Cutter^000000.", "Using ^009900Phantom Menace^000000 autocasts ^009900Rolling Cutter^000000 at the highest learned level."},
    {"Casting ^009900Rolling Cutter^000000 has a chance to auto cast Level 1 ^009900Impact Crater^000000.", "Using ^009900Rolling Cutter^000000 autocasts ^009900Impact Crater^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(Each autocast skill must be learned at Level 1 or higher.)"},
  },
  [620037] = {
    {"Casting ^009900Mighty Smash^000000 has a chance to auto cast Level 1 ^009900Axe Tornado^000000.", "Using ^009900Mighty Smash^000000 autocasts ^009900Axe Tornado^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
    {"Increases damage of ^009900Axe Tornado^000000 by 45%.", "Increases damage of ^009900Axe Stomp^000000 by 45%."},
  },
  [640049] = {
    {"Casting ^009900Crimson Arrow^000000 has a chance to auto cast Level 1 ^009900Storm Cannon^000000.", "Using ^009900Crimson Arrow^000000 autocasts ^009900Storm Cannon^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [640050] = {
    {"Casting ^009900Rock Down^000000 has a chance to auto cast Level 1 ^009900Frozen Slash^000000.", "Using ^009900Rock Down^000000 autocasts ^009900Frozen Slash^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [650046] = {
    {"Casting ^009900Cross Slash^000000 has a chance to auto cast Level 1 ^009900Shadow Slash^000000.", "Using ^009900Cross Slash^000000 autocasts ^009900Shadow Slash^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [650047] = {
    {"Casting ^009900Red Flame Cannon^000000 has a chance to auto cast Level 1 ^009900Thundering Cannon^000000.", "Using ^009900Red Flame Cannon^000000 autocasts ^009900Thundering Cannon^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [700093] = {
    {"Casting ^009900Crescive Bolt^000000 has a chance to auto cast Level 1 ^009900Sharp Shooting^000000.", "Using ^009900Crescive Bolt^000000 autocasts ^009900Sharp Shooting^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [700094] = {
    {"Casting ^009900Hawk Rush^000000 has a 50% chance to auto cast Level 1 ^009900Hawk Boomerang^000000.", "Using ^009900Hawk Rush^000000 has a 50% chance to autocast ^009900Hawk Boomerang^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [810040] = {
    {"Casting ^009900Only One Bullet^000000 has a chance to auto cast Level 1 ^009900Spiral Shooting^000000.", "Using ^009900Only One Bullet^000000 autocasts ^009900Spiral Shooting^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
  },
  [840032] = {
    {"Casting ^009900Wild Fire^000000 has a chance to auto cast Level 1 ^009900Spiral Shooting^000000.", "Using ^009900Spiral Shooting^000000 autocasts ^009900Wild Fire^000000 at the highest learned level."},
    {"(If you learned a higher Level, it will auto cast that Level instead.)", "(The autocast skill must be learned at Level 1 or higher.)"},
    {"^0000CCType:^000000 Rifle", "^0000CCType:^000000 Grenade Launcher"},
  },
}
for id, edits in pairs(repairs) do
  local item = tbl[id]
  if item and item.identifiedDescriptionName then
    for index, line in ipairs(item.identifiedDescriptionName) do
      for _, edit in ipairs(edits) do
        if line == edit[1] then
          item.identifiedDescriptionName[index] = edit[2]
          break
        end
      end
    end
  end
end
