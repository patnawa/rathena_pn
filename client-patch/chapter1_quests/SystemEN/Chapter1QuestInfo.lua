-- PN Chapter 1 quest records, GPL-3.0-or-later.
-- Source: npc/custom/chapter1/CH1.c and db/import/quest_db.yml.
-- Fill only the reviewed missing World Tree chain; preserve custom records.
local function quest(id, title, summary, description)
    if QuestInfoList[id] ~= nil then return end
    QuestInfoList[id] = {
        Title = title,
        IconName = "ico_nq.bmp",
        Summary = summary,
        Description = description
    }
end

quest(18369, "Call of The World Tree (2)", "Introduce yourself to Lapine Shasha.", {
    "Follow the branch to the fruit and introduce yourself to <NAVI>[Lapine Shasha]<INFO>ygg_fruit,80,122,0,101,0</INFO></NAVI>."
})
quest(18370, "Call of The World Tree (3)", "Introduce yourself to Maysel.", {
    "Introduce yourself to <NAVI>[Maysel]<INFO>ygg_fruit,82,120,0,101,0</INFO></NAVI> beside Shasha."
})
quest(18371, "Twisted Land of Darkness", "Investigate the Land of Darkness.", {
    "Walk onto the first <NAVI>[Investigation Point]<INFO>ygg_roots,334,138,0,101,0</INFO></NAVI> in the Land of Darkness and follow the investigation."
})
quest(18372, "To Fallen Geffen", "Meet Ascetic Jeon.", {
    "Meet <NAVI>[Ascetic Jeon]<INFO>ygg_roots,164,231,0,101,0</INFO></NAVI> at the gate to Fallen Geffen."
})
quest(18373, "Fire Resistance Material", "Bring 5 Dragon Scales to Debris.", {
    "Bring 5 Dragon Scales to <NAVI>[Debris]<INFO>ygg_fruit,71,78,0,101,0</INFO></NAVI> for his fire resistance research."
})
quest(18374, "Bad News", "Receive the Muspelheim assignment.", {
    "Speak to <NAVI>[Lapine Shasha]<INFO>ygg_fruit,80,122,0,101,0</INFO></NAVI> again to receive the Muspelheim assignment."
})
quest(18375, "Good News", "Receive fire protection from Debris.", {
    "Receive fire protection from <NAVI>[Debris]<INFO>ygg_fruit,71,78,0,101,0</INFO></NAVI> before entering Muspelheim."
})
quest(18376, "Land of Fire", "Enter the survey camp and speak to Chez.", {
    "Enter through the <NAVI>[Survey Point]<INFO>mu_fild01,95,154,0,101,0</INFO></NAVI>, then speak to Chez at ch1_sf01 190,214 inside the scout camp.",
    "The survey entrance checks your story progress; a navigation link does not grant entry."
})
quest(18377, "Mysterious Being", "Chapter 1 completed.", {
    "You have completed Chapter 1 and reported back to Shasha. This is the completed story record, not a new assignment."
})
quest(18378, "To Ashridge", "Report the Land of Darkness investigation.", {
    "Return to <NAVI>[Lapine Shasha]<INFO>ygg_fruit,80,122,0,101,0</INFO></NAVI> and report the Land of Darkness investigation."
})
quest(18379, "Hazy Gate", "Visit the Hazy Gate reconnaissance point.", {
    "After speaking with Rubiel in Ashridge, visit the <NAVI>[Recon Point]<INFO>hem_fild,180,263,0,101,0</INFO></NAVI>, choose Enter, then speak to Rubiel at ch1_sf03 122,255 inside the camp."
})

-- The same omission affects later Chapter 1 investigations in the guide.
dofile("SystemEN/Chapter1GuideRecords.lua")
