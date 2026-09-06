-- Clean-room metadata: doc/druid_missing_weapons_audit.md.
-- Generic existing artwork, not a claim that the original weapon artwork is installed.
local items = {}
local function add(id, name, kind, view, slots, weight, atk, matk, level, jobs, effects)
  effects[#effects+1] = "The listed base-level, refine and grade bonuses are cumulative."
  effects[#effects+1] = "_______________________"
  effects[#effects+1] = "^0000CCType:^000000 " .. kind
  effects[#effects+1] = "^0000CCATK:^000000 " .. atk
  if matk > 0 then effects[#effects+1] = "^0000CCMATK:^000000 " .. matk end
  effects[#effects+1] = "^0000CCWeapon Level:^000000 5"
  effects[#effects+1] = "^0000CCRequired Level:^000000 " .. level
  effects[#effects+1] = "^0000CCJobs:^000000 " .. jobs
  effects[#effects+1] = "^0000CCWeight:^000000 " .. weight
  items[id] = {
    unidentifiedDisplayName = name, unidentifiedResourceName = "EpisodClear20",
    unidentifiedDescriptionName = {""}, identifiedDisplayName = name,
    identifiedResourceName = "EpisodClear20", identifiedDescriptionName = effects,
    slotCount = slots, ClassNum = view, costume = false
  }
end
add(510185,"Repeat Dagger-OSAD","Dagger",1,2,90,150,0,170,"Karnos and Alitea",{
  "ATK +5%.", "Refine +7: variable cast time -10%.",
  "Refine +9: Sharpen Gust damage +25%.",
  "Refine +11: ranged physical damage +15%; Sharpen Gust damage +20%."
})
add(510189,"Solid Whinger","Dagger",1,2,110,200,0,220,"Alitea",{
  "Every 2 refine levels: Quill Spear damage +6%.",
  "Every 3 refine levels: Pinion Shot damage +8%.",
  "Every 4 refine levels: ranged physical damage +4%.",
  "Grade D: CON +2.", "Grade C: Quill Spear damage +10%.", "Grade B: P.ATK +2."
})
add(510190,"Glacier Nature Knife","Dagger",1,0,120,210,210,210,"Alitea",{})
local function glacierSets(twoHanded)
  local text = {
    "With Dim Glacier Armor, Boots and Manteau:",
    "ATK +10 per 3 weapon refine levels.",
    "Weapon grade C: P.ATK +6, POW +5, CON +5.",
    "Weapon grade B: P.ATK +5, POW +5, CON +5.",
    "Weapon grade A: P.ATK +4, POW +5, CON +5."
  }
  if twoHanded then text[#text+1] = "Weapon grade A also adds physical damage against all target elements +20%." end
  text[#text+1] = "With Dim Glacier Robe, Shoes and Muffler:"
  text[#text+1] = "MATK +10 per 3 weapon refine levels."
  text[#text+1] = "Weapon grade C: S.MATK +6, SPL +5, CON +5."
  text[#text+1] = "Weapon grade B: S.MATK +5, SPL +5, CON +5."
  text[#text+1] = "Weapon grade A: S.MATK +4, SPL +5, CON +5."
  if twoHanded then
    text[#text+1] = "Weapon grade A also adds magic damage against all target elements +20%."
    text[#text+1] = "Indestructible in battle."
  end
  return text
end
add(510191,"Dim Glacier Nature Knife","Dagger",1,1,120,210,210,230,"Alitea",glacierSets(false))
add(510193,"Dimensions Nature Dagger","Dagger",1,2,180,240,0,250,"Alitea",{
  "CRI +5; Quill Spear damage +15%.",
  "Every 2 refine levels: ATK +25 and ATK +1%.",
  "Every 3 refine levels: Quill Spear damage +5%.",
  "Refine +7: critical damage +20%.", "Refine +9: CRI +15 and C.RATE +5.",
  "Refine +11: Quill Spear damage +15%.",
  "Grade D: P.ATK +5.", "Grade C: Quill Spear damage +15%.",
  "Grade B: ranged physical damage +10%.", "Grade A: P.ATK +3 per 2 refine levels.",
  "With Time Dimensions Rune Crown (Alitea): Tempest Flap damage +45% and ranged physical damage +15%.",
  "If both this weapon and that crown are grade A: Quill Spear damage +1% per combined refine level; Quill Spear cooldown -0.2 seconds."
})
add(520047,"Flush Grinder Axe","One-handed Axe",6,2,200,230,0,250,"Alitea",{
  "Indestructible in battle. ATK +10%; Chop Chop and Frenzy Fang damage +20%.",
  "Refine +7: ATK +70 and attack speed +10% (attack delay reduction).",
  "Refine +9: Chop Chop damage +15%, Frenzy Fang damage +10%, physical damage against all sizes +15%; both skills cost 25 more SP.",
  "Refine +11: after-cast delay -15%; Chop Chop and Frenzy Fang damage +10%; both skills cost another 25 SP.",
  "Grade D: P.ATK +10.", "Grade C: Chop Chop and Frenzy Fang damage +10%.",
  "Grade B: physical damage against all target elements +10%.",
  "Grade A: Chop Chop and Frenzy Fang damage +10%."
})
add(520052,"Furious Axe","One-handed Axe",6,2,500,230,0,205,"Alitea",{
  "Indestructible in battle. Alpha Claw damage +5%.",
  "Base level 210 or higher: ATK +4% and ATK +40.",
  "Base level 220 or higher: Alpha Claw damage +5%.",
  "Base level 230 or higher: physical damage against all sizes +10%.",
  "Refine +7: melee physical damage +10%.", "Refine +9: Alpha Claw damage +10%.",
  "Refine +10: CRI +10.", "Refine +11: Alpha Claw damage +10%.",
  "Grade D: melee physical damage +15%.", "Grade C: Alpha Claw damage +5%.",
  "Grade B: Primal Claw and Feral Claw damage +10%.",
  "Grade A: ATK +3% and P.ATK +3 per 2 refine levels.",
  "With Furious Boots: Primal Claw and Feral Claw damage +1% per combined weapon and boots refine level.",
  "With Furious Circlet (Alitea): physical damage against all races +10%, excluding players."
})
add(590104,"Muqaddas Garz","Mace",8,2,120,220,230,250,"Alitea",{
  "Indestructible in battle. MATK +5%.", "Glacial Monolith and Glacial Nova damage +10%.",
  "Refine +7: water magic damage +10%.",
  "Refine +9: Glacial Monolith and Glacial Nova damage +10%.",
  "Refine +11: Glacial Monolith and Glacial Nova damage +10%.",
  "Grade D: magic damage against small targets +15%.",
  "Grade C: magic damage against medium targets +15%.",
  "Grade B: magic damage against large targets +15%.", "Grade A: S.MATK +5."
})
add(590117,"Furious Scepter","Mace",8,2,110,100,180,205,"Alitea",{
  "Indestructible in battle. Terra Harvest damage +10%.",
  "Base level 210 or higher: MATK +4% and MATK +40.",
  "Base level 220 or higher: Terra Harvest damage +5%.",
  "Base level 230 or higher: magic damage against all sizes +10%.",
  "Refine +7: water and earth magic damage +10%.", "Refine +9: Terra Harvest damage +10%.",
  "Refine +10: water and earth magic damage +10%.", "Refine +11: Glacial Shard damage +10%.",
  "Grade D: water and earth magic damage +10%.", "Grade C: Terra Harvest damage +5%.",
  "Grade B: Glacial Shard damage +10%.", "Grade A: MATK +3% and S.MATK +3 per 2 refine levels.",
  "With Furious Boots: Glacial Shard damage +1% per combined weapon and boots refine level.",
  "With Furious Circlet (Alitea): Glacial Shard damage +10%."
})
add(620056,"Glacier Nature Axe","Two-handed Axe",7,0,600,350,180,210,"Alitea",{"Indestructible in battle."})
add(620057,"Dim Glacier Nature Axe","Two-handed Axe",7,1,600,350,180,230,"Alitea",glacierSets(true))
add(620059,"Dimensions Nature Axe","Two-handed Axe",7,2,400,380,0,250,"Alitea",{
  "Indestructible in battle. CRI +5; Frenzy Fang damage +15%.",
  "Every 2 refine levels: ATK +25 and ATK +1%.", "Every 3 refine levels: Frenzy Fang damage +5%.",
  "Refine +7: melee physical damage +25%.", "Refine +9: CRI +15 and C.RATE +5.",
  "Refine +11: Frenzy Fang damage +15%.", "Grade D: P.ATK +5.", "Grade C: Frenzy Fang damage +15%.",
  "Grade B: melee physical damage +10%.", "Grade A: P.ATK +3 per 2 refine levels.",
  "With Time Dimensions Rune Crown (Alitea): Savage Lunge damage +60%.",
  "If both this weapon and that crown are grade A: Savage Lunge damage +1% per combined refine level; Frenzy Fang cooldown -0.2 seconds."
})
tbl_druidmissingweapons = items
return items
