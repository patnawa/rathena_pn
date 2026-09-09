-- Navigation corrections verified against the live server on 2026-09-09.
-- Loaded after the base quest table and existing compatibility patches.
local fixes = {
  [" aldebaran, 172,162,0,101,0"] = "aldebaran,172,162,0,101,0",
  ["Go back to yuno.gat, 165,153,0,101,0"] = "yuno,165,153,0,101,0",
  ["Morroc,100,31,0,101,0"] = "morocc,100,31,0,101,0",
  ["Morroc,158,162,0,101,0"] = "morocc,158,162,0,101,0",
  ["Morroc,170,75,0,101,0"] = "morocc,170,75,0,101,0",
  ["Morroc,201,61,0,101,0"] = "morocc,201,61,0,101,0",
  ["Morroc,237,73,0,101,0,"] = "morocc,237,73,0,101,0",
  ["Morroc,45,106,0,101,0"] = "morocc,45,106,0,101,0",
  ["Morroc,82,91,0,101,0"] = "morocc,82,91,0,101,0",
  ["Morroc_in,116,101,0,101,0"] = "morocc_in,116,101,0,101,0",
  ["Morroc_in,45,126,0,101,0"] = "morocc_in,45,126,0,101,0",
  ["airplane.gat,236,63,0,101,0"] = "airplane,236,63,0,101,0",
  ["alb2trea.gat,86,109,0,101,0"] = "alb2trea,86,109,0,101,0",
  ["alberta,108,236,0,101,0,"] = "alberta,108,236,0,101,0",
  ["alberta,140,217,0,101,0,"] = "alberta,140,217,0,101,0",
  ["alberta,208,50,0,101,0,"] = "alberta,208,50,0,101,0",
  ["alberta,231,117,0,101,0,"] = "alberta,231,117,0,101,0",
  ["alberta,47,118,0,101,0,"] = "alberta,47,118,0,101,0",
  ["aldebaran, 101,52,0,101,0"] = "aldebaran,101,52,0,101,0",
  ["aldebaran, 107,164,0,101,0"] = "aldebaran,107,164,0,101,0",
  ["aldebaran, 108,166,0,101,0"] = "aldebaran,108,166,0,101,0",
  ["aldebaran, 124,238,0,101,0"] = "aldebaran,124,238,0,101,0",
  ["aldebaran, 124,238,0,101,0 "] = "aldebaran,124,238,0,101,0",
  ["aldebaran, 172,162,0,101,0"] = "aldebaran,172,162,0,101,0",
  ["aldebaran, 174,164 Please come to, 0,101,0"] = "aldebaran,174,164,0,101,0",
  ["aldebaran, 217,212,0,101,0"] = "aldebaran,217,212,0,101,0",
  ["ama_dun01.gat,156,192,0,101,0"] = "ama_dun01,156,192,0,101,0",
  ["ama_dun02.gat,121,47,0,101,0"] = "ama_dun02,121,47,0,101,0",
  ["ama_dun03.gat,152,55,0,101,0"] = "ama_dun03,152,55,0,101,0",
  ["ch1_gef_in.gat,182,191,0,101,0"] = "ch1_gef_in,182,191,0,101,0",
  ["comodo, 297,180,0,101,0"] = "comodo,297,180,0,101,0",
  ["comodo.gat,204,143,0,101,0"] = "comodo,204,143,0,101,0",
  ["dali02.gat,108,94,0,101,0"] = "dali02,108,94,0,101,0",
  ["dali02.gat,111,95,0,101,0"] = "dali02,111,95,0,101,0",
  ["dic_fild01,231,174,0,101,0,"] = "dic_fild01,231,174,0,101,0",
  ["dic_fild02,175,130,0,101,0,"] = "dic_fild02,175,130,0,101,0",
  ["dic_fild02,71,357,0,101,0,"] = "dic_fild02,71,357,0,101,0",
  ["dic_in01,138,188,0,101,0,"] = "dic_in01,138,188,0,101,0",
  ["dic_in01,158,188,0,101,0,"] = "dic_in01,158,188,0,101,0",
  ["dic_in01,245,119,0,101,0,"] = "dic_in01,245,119,0,101,0",
  ["dic_in01,294,276,0,101,0,"] = "dic_in01,294,276,0,101,0",
  ["dic_in01,300,280,0,101,0,"] = "dic_in01,300,280,0,101,0",
  ["dic_in01,39,261,0,101,0,"] = "dic_in01,39,261,0,101,0",
  ["dicastes01,112,248,0,101,0,"] = "dicastes01,112,248,0,101,0",
  ["dicastes01,207,210,0,101,0,"] = "dicastes01,207,210,0,101,0",
  ["dicastes01,249,140,0,101,0,"] = "dicastes01,249,140,0,101,0",
  ["eclage,282,255,0,101,0,"] = "eclage,282,255,0,101,0",
  ["einbech,156,243,0,101,0,"] = "einbech,156,243,0,101,0",
  ["einbech.gat,40,100,0,101,0"] = "einbech,40,100,0,101,0",
  ["gef_tower.gat,153,31,0,101,0"] = "gef_tower,153,31,0,101,0",
  ["geffen 161,83,0,101,0"] = "geffen,161,83,0,101,0",
  ["geffen, 106,63,0,101,0"] = "geffen,106,63,0,101,0",
  ["geffen, 88,35,0,101,0"] = "geffen,88,35,0,101,0",
  ["hugel, 69,121,0,101,0"] = "hugel,69,121,0,101,0",
  ["hugel,209, 109, 0, 101, 0"] = "hugel,209,109,0,101,0",
  ["iz_ac01.gat,45,80,0,101,0"] = "iz_ac01,45,80,0,101,0",
  ["izlude.gat, 164,148,0,101,0"] = "izlude,164,148,0,101,0",
  ["izlude.gat,164,148,0,101,0"] = "izlude,164,148,0,101,0",
  ["izlude_in, 69,116,0,101,0"] = "izlude_in,69,116,0,101,0",
  ["jalbe_in.gat,126,54,0,101,0"] = "jalbe_in,126,54,0,101,0",
  ["jalbe_in.gat,22,63,0,101,0"] = "jalbe_in,22,63,0,101,0",
  ["jalbe_in.gat,29,28,0,101,0"] = "jalbe_in,29,28,0,101,0",
  ["jalbe_in.gat,67,95,0,101,0"] = "jalbe_in,67,95,0,101,0",
  ["jor_albe.gat,148,235,0,101,0"] = "jor_albe,148,235,0,101,0",
  ["jor_albe.gat,151,210,0,101,0"] = "jor_albe,151,210,0,101,0",
  ["jor_albe.gat,167,139,0,101,0"] = "jor_albe,167,139,0,101,0",
  ["jor_albe.gat,190,175,0,101,0"] = "jor_albe,190,175,0,101,0",
  ["jor_albe.gat,191,186,0,101,0"] = "jor_albe,191,186,0,101,0",
  ["jor_albe.gat,192,189,0,101,0"] = "jor_albe,192,189,0,101,0",
  ["jor_albe.gat,195,179,0,101,0"] = "jor_albe,195,179,0,101,0",
  ["jor_albe.gat,211,136,0,101,0"] = "jor_albe,211,136,0,101,0",
  ["jor_albe.gat,239,115,0,101,0"] = "jor_albe,239,115,0,101,0",
  ["jor_back4,101,265,0,101,0"] = "jor_back4,98,258,0,101,0",
  ["jor_mbase.gat,233,277,0,101,0"] = "jor_mbase,233,277,0,101,0",
  ["lasa_sea,135,70,0,101,0,"] = "lasa_sea,135,70,0,101,0",
  ["lasa_sea,18,51,0,101,0,"] = "lasa_sea,18,51,0,101,0",
  ["lasa_sea,197,67,0,101,0,"] = "lasa_sea,197,67,0,101,0",
  ["lasa_sea,28,142,0,101,0,"] = "lasa_sea,28,142,0,101,0",
  ["lasa_sea,74,202,0,101,0,"] = "lasa_sea,74,202,0,101,0",
  ["lasagna 83,206,0,101,0"] = "lasagna,83,206,0,101,0",
  ["lasagna, 101,120,0,101,0"] = "lasagna,101,120,0,101,0",
  ["lasagna, 101,120,0,101,0,"] = "lasagna,101,120,0,101,0",
  ["lasagna, 150,97,0,101,0,"] = "lasagna,150,97,0,101,0",
  ["lasagna, 271,137,0,101,0"] = "lasagna,271,137,0,101,0",
  ["lasagna, 289,285,0,101,0,"] = "lasagna,289,285,0,101,0",
  ["lasagna, 81,201,0,101,0,"] = "lasagna,81,201,0,101,0",
  ["lasagna, 83,206,0,101,0,"] = "lasagna,83,206,0,101,0",
  ["lasagna, 90,202,0,101,0"] = "lasagna,90,202,0,101,0",
  ["lasagna, 90,202,0,101,0,"] = "lasagna,90,202,0,101,0",
  ["lasagna,101,120,0,101,0,"] = "lasagna,101,120,0,101,0",
  ["lasagna,111,214,0,101,0,"] = "lasagna,111,214,0,101,0",
  ["lasagna,150,98,0,101,0,"] = "lasagna,150,98,0,101,0",
  ["lasagna,196,137,0,101,0,"] = "lasagna,196,137,0,101,0",
  ["lasagna,213,254,0,101,0,"] = "lasagna,213,254,0,101,0",
  ["lasagna,224,111,0,101,0,"] = "lasagna,224,111,0,101,0",
  ["lasagna,227,110,0,101,0,"] = "lasagna,227,110,0,101,0",
  ["lasagna,289,285,0,101,0,"] = "lasagna,289,285,0,101,0",
  ["lasagna,81,201,0,101,0,"] = "lasagna,81,201,0,101,0",
  ["lasagna,83,189,0,101,0,"] = "lasagna,83,189,0,101,0",
  ["lasagna,83,206,0,101,0,"] = "lasagna,83,206,0,101,0",
  ["lasagna,90,202,0,101,0,"] = "lasagna,90,202,0,101,0",
  ["lhz_in01,201,152,0,101,0"] = "lhz_in01,132,259,0,101,0",
  ["mal_in01,114,169,0,101,0,"] = "mal_in01,114,169,0,101,0",
  ["mal_in02,179,57,0,101,0,"] = "mal_in02,179,57,0,101,0",
  ["malangdo,125,147,0,101,0,"] = "malangdo,125,147,0,101,0",
  ["malangdo,141,155,0,101,0,"] = "malangdo,141,155,0,101,0",
  ["man_fild01,0,101,0"] = "airplane,33,69,0,101,0",
  ["man_fild01,92,230,101,0"] = "man_fild01,92,230,0,101,0",
  ["man_in01,315,52,0,101,0,"] = "man_in01,315,52,0,101,0",
  ["mbase_in.gat,92,123,0,101,0"] = "mbase_in,92,123,0,101,0",
  ["mid_campin,110,120,0,101,0,"] = "mid_campin,110,120,0,101,0",
  ["mid_campin,168,170,0,101,0,"] = "mid_campin,168,170,0,101,0",
  ["mid_campin,93,114,0,101,0,"] = "mid_campin,93,114,0,101,0",
  ["mjolnir_03,280,252,0,101,0,"] = "mjolnir_03,280,252,0,101,0",
  ["moc_fild12,232,228,0,101,0,"] = "moc_fild12,232,228,0,101,0",
  ["moc_fild12,234,59,0,101,0,"] = "moc_fild12,234,59,0,101,0",
  ["moc_fild17,219,258,0,101,0,"] = "moc_fild17,219,258,0,101,0",
  ["moc_fild18,125,222,0,101,0,"] = "moc_fild18,125,222,0,101,0",
  ["moc_fild18,314,192,0,101,0,"] = "moc_fild18,314,192,0,101,0",
  ["moc_para01,110,79,0,101,0,"] = "moc_para01,110,79,0,101,0",
  ["moc_para01,110,83,0,101,0,"] = "moc_para01,110,83,0,101,0",
  ["moc_para01,110,87,0,101,0,"] = "moc_para01,110,87,0,101,0",
  ["moc_para01,110,91,0,101,0,"] = "moc_para01,110,91,0,101,0",
  ["moc_para01,23,31,0,101,0,"] = "moc_para01,23,31,0,101,0",
  ["moc_para01,25,31,0,101,0,"] = "moc_para01,25,31,0,101,0",
  ["moc_ruins.gat,57,162,0,101,0"] = "moc_ruins,57,162,0,101,0",
  ["mora, 101,53,0,101,0"] = "mora,101,53,0,101,0",
  ["pay_arche,63,109,101,0"] = "pay_arche,63,109,0,101,0",
  ["pay_arche.gat,39,131,0,101,0"] = "pay_arche,39,131,0,101,0",
  ["pay_dun04.gat,120,116,0,101,0"] = "pay_dun04,120,116,0,101,0",
  ["pay_fild03,211,174,0,101,0,"] = "pay_fild03,211,174,0,101,0",
  ["pay_fild03,211,179,0,101,0,"] = "pay_fild03,211,179,0,101,0",
  ["payon,197,122,0,101,0,"] = "payon,197,122,0,101,0",
  ["payon,215,201,0,101,0,"] = "payon,215,201,0,101,0",
  ["payon.gat,190,93,0,101,0"] = "payon,190,93,0,101,0",
  ["payon.gat,241,294,0,101,0"] = "payon,241,294,0,101,0",
  ["payon_in01,187,90,0,101,0,"] = "payon_in01,187,90,0,101,0",
  ["prontera, 100,183,0,101,0"] = "prontera,100,183,0,101,0",
  ["prontera,276,355,"] = "prontera,276,355,0,101,0",
  ["prontera,58,364,"] = "prontera,58,364,0,101,0",
  ["prontera.gat, 268,156,0,101,0"] = "prontera,268,156,0,101,0",
  ["prontera.gat,213,321,0,101,0"] = "prontera,213,321,0,101,0",
  ["prontera.gat,268,156,0,101,0"] = "prontera,268,156,0,101,0",
  ["prt_fild01.gat,101,155,0,101,0"] = "prt_fild01,101,155,0,101,0",
  ["prt_fild01.gat,107,180,0,101,0"] = "prt_fild01,107,180,0,101,0",
  ["prt_fild01.gat,87,198,0,101,0"] = "prt_fild01,87,198,0,101,0",
  ["prt_fild01.gat,95,194,0,101,0"] = "prt_fild01,95,194,0,101,0",
  ["prt_fild01.gat,97,180,0,101,0"] = "prt_fild01,97,180,0,101,0",
  ["prt_fild01.gat,98,171,0,101,0"] = "prt_fild01,98,171,0,101,0",
  ["prt_fild08.gat,358,272,0,101,0"] = "prt_fild08,358,272,0,101,0",
  ["prt_fild08.gat,362,272,0,101,0"] = "prt_fild08,362,272,0,101,0",
  ["prt_maze01.gat,100,27,0,101,0"] = "prt_maze01,100,27,0,101,0",
  ["ra_fild01,368,183,0,101,0"] = "ra_fild03,368,183,0,101,0",
  ["splendide,160 264,0,101,0"] = "splendide,160,264,0,101,0",
  ["tur_d03_i,125,86,0,101,0"] = "tur_d03_i,125,186,0,101,0",
  ["um_in.gat,44,71,0,101,0"] = "um_in,44,71,0,101,0",
  ["veins,238,110,0,101,0,"] = "veins,238,110,0,101,0",
  ["xmas, 204,223,0,101,0"] = "xmas,204,223,0,101,0",
  ["yuno, 150,155,0,101,0"] = "yuno,150,155,0,101,0",
  ["yuno, 195,302,0,101,0"] = "yuno,195,302,0,101,0",
  ["yuno, 282,102,0,101,0"] = "yuno,282,102,0,101,0",
  ["yuno, 343,257,0,101,0"] = "yuno,343,257,0,101,0",
  ["yuno.gat, 142,163,0,101,0"] = "yuno,142,163,0,101,0",
  ["yuno.gat, 150,155,0,101,0"] = "yuno,150,155,0,101,0",
  ["yuno.gat, 165,153,0,101,0"] = "yuno,165,153,0,101,0",
  ["yuno.gat, 175,162,0,101,0"] = "yuno,175,162,0,101,0",
  ["yuno.gat,121,174,0,101,0"] = "yuno,121,174,0,101,0",
  ["yuno.gat,123,152,0,101,0"] = "yuno,123,152,0,101,0",
  ["yuno.gat,128,197,0,101,0"] = "yuno,128,197,0,101,0",
  ["yuno.gat,133,199,0,101,0"] = "yuno,133,199,0,101,0",
  ["yuno.gat,139,154,0,101,0"] = "yuno,139,154,0,101,0",
  ["yuno.gat,139,204,0,101,0"] = "yuno,139,204,0,101,0",
  ["yuno.gat,141,200,0,101,0"] = "yuno,141,200,0,101,0",
  ["yuno.gat,142,152,0,101,0"] = "yuno,142,152,0,101,0",
  ["yuno.gat,153,199,0,101,0"] = "yuno,153,199,0,101,0",
  ["yuno.gat,164,169,0,101,0"] = "yuno,164,169,0,101,0",
  ["yuno.gat,165,153,0,101,0"] = "yuno,165,153,0,101,0",
  ["yuno.gat,175,162,0,101,0"] = "yuno,175,162,0,101,0",
  ["yuno.gat,175,204,0,101,0"] = "yuno,175,204,0,101,0",
  ["yuno.gat,176,155,0,101,0"] = "yuno,176,155,0,101,0",
  ["yuno.gat,179,204,0,101,0"] = "yuno,179,204,0,101,0",
  ["yuno.gat,181,79,0,101,0"] = "yuno,181,79,0,101,0",
  ["yuno.gat,185,179,0,101,0"] = "yuno,185,179,0,101,0",
  ["yuno.gat,185,186,0,101,0"] = "yuno,185,186,0,101,0",
  ["yuno.gat,190,158,0,101,0"] = "yuno,190,158,0,101,0",
  ["yuno.gat,190,189,0,101,0"] = "yuno,190,189,0,101,0",
  ["yuno.gat,193,231,0,101,0"] = "yuno,193,231,0,101,0",
  ["yuno.gat,194,166,0,101,0"] = "yuno,194,166,0,101,0",
  ["yuno.gat,196,188,0,101,0"] = "yuno,196,188,0,101,0",
  ["yuno.gat,198,163,0,101,0"] = "yuno,198,163,0,101,0",
  ["yuno.gat,199,152,0,101,0"] = "yuno,199,152,0,101,0",
  ["yuno.gat,201,176,0,101,0"] = "yuno,201,176,0,101,0",
  ["yuno.gat,208,330,0,101,0"] = "yuno,208,330,0,101,0",
  ["yuno.gat,235,108,0,101,0"] = "yuno,235,108,0,101,0",
  ["yuno.gat,236,147,0,101,0"] = "yuno,236,147,0,101,0",
  ["yuno.gat,247,178,0,101,0"] = "yuno,247,178,0,101,0",
  ["yuno.gat,283,285,0,101,0"] = "yuno,283,285,0,101,0",
  ["yuno.gat,291,116,0,101,0"] = "yuno,291,116,0,101,0",
  ["yuno.gat,301,190,0,101,0"] = "yuno,301,190,0,101,0",
  ["yuno.gat,33,192,0,101,0"] = "yuno,33,192,0,101,0",
  ["yuno.gat,341,258,0,101,0"] = "yuno,341,258,0,101,0",
  ["yuno.gat,355,164,0,101,0"] = "yuno,355,164,0,101,0",
  ["yuno.gat,363,221,0,101,0"] = "yuno,363,221,0,101,0",
  ["yuno.gat,368,164,0,101,0"] = "yuno,368,164,0,101,0",
  ["yuno.gat,89,109,0,101,0"] = "yuno,89,109,0,101,0",
  ["yuno_fild01.gat,119,343,0,101,0"] = "yuno_fild01,119,343,0,101,0",
  ["yuno_fild02.gat,159,235,0,101,0"] = "yuno_fild02,159,235,0,101,0",
  ["yuno_fild03.gat,320,204,0,101,0"] = "yuno_fild03,320,204,0,101,0",
  ["yuno_fild08.gat,206,317,0,101,0"] = "yuno_fild08,206,317,0,101,0",
  ["yuno_fild09.gat,63,108,0,101,0"] = "yuno_fild09,63,108,0,101,0",
  ["yuno_fild11.gat,37,346,0,101,0"] = "yuno_fild11,37,346,0,101,0",
  ["yuno_in01,103,157,101,0"] = "yuno_in01,103,157,0,101,0",
  ["yuno_in03.gat,173,170 Let's go back to, 0,101,0"] = "yuno_in03,173,170,0,101,0",
  ["yuno_in03.gat,173,170,0,101,0"] = "yuno_in03,173,170,0,101,0",
  ["yuno_in03.gat,176,55,0,101,0"] = "yuno_in03,176,55,0,101,0",
  ["yuno_in03.gat,Let's go back to 173,170,0,101,0"] = "yuno_in03,173,170,0,101,0",
}
for _, quest in pairs(QuestInfoList) do
  if quest.Description then
    for i, text in ipairs(quest.Description) do
      quest.Description[i] = text:gsub("(<NAVI>.-<INFO>)(.-)(</INFO></NAVI>)", function(prefix, info, suffix)
        return prefix .. (fixes[info] or info) .. suffix
      end)
    end
  end
  if quest.Summary then
    quest.Summary = quest.Summary:gsub("(<NAVI>.-<INFO>)(.-)(</INFO></NAVI>)", function(prefix, info, suffix)
      return prefix .. (fixes[info] or info) .. suffix
    end)
  end
  if quest.NpcNavi == "Morroc" then quest.NpcNavi = "morocc" end
end
-- The receiving NPC completes quest 11464; Lazy is a different NPC.
assert(QuestInfoList[11464])
QuestInfoList[11464].Description = {"Speak to the <NAVI>Rebellion Guard leader<INFO>rebel_in,162,87,0,101,0</INFO></NAVI> about the party preparations."}
QuestInfoList[11464].Summary = "Talk to the Rebellion Guard leader"
-- Preserve the destination shared by this legacy delivery quest family.
if QuestInfoList[8325] then QuestInfoList[8325].NpcPosY = 19 end
-- Current server NPC locations for the Acolyte and Episode 13.1 guides.
QuestInfoList[1003].NpcPosX = 208
QuestInfoList[1003].NpcPosY = 218
QuestInfoList[10068].NpcPosX = 19
QuestInfoList[10068].NpcPosY = 98

-- Reviewed legacy guide destinations against the actual NPC dialogue.
-- The Tower assigns Sacred Roots workers. The presidential Guard, rather
-- than the entrance Secretary, handles waiting guests and admission.
local reviewedDestinations = {
  [8846] = {"jor_sanct,95,152,0,101,0", "jor_sanct,92,139,0,101,0"},
  [17279] = {"yuno_pre,69,17,0,101,0", "yuno_pre,95,71,0,101,0"},
  [17280] = {"yuno_pre,69,17,0,101,0", "yuno_pre,95,71,0,101,0"},
}
for id, replacement in pairs(reviewedDestinations) do
  local quest = QuestInfoList[id]
  if quest and quest.Description then
    for i, text in ipairs(quest.Description) do
      quest.Description[i] = text:gsub("(<INFO>)(.-)(</INFO>)", function(prefix, info, suffix)
        return prefix .. (info == replacement[1] and replacement[2] or info) .. suffix
      end)
    end
  end
end

-- These legacy quest stages are not implemented by the loaded server scripts.
-- Keep their journal text, but do not route players to unrelated NPCs with
-- the same name. This does not remove or alter character quest progress.
local unavailableGuides = {14995, 16147, 8886, 8916, 21946}
local unavailableNote = "This quest is not available on this server."
for _, id in ipairs(unavailableGuides) do
  local quest = QuestInfoList[id]
  if quest and quest.Description then
    local hasNote = false
    for i, text in ipairs(quest.Description) do
      if text == unavailableNote then hasNote = true end
      quest.Description[i] = text:gsub("<NAVI>(.-)<INFO>.-</INFO></NAVI>", "%1")
    end
    if not hasNote then quest.Description[#quest.Description + 1] = unavailableNote end
  end
end

-- Preserve the five custom records shipped only in the previous fallback table.
QuestInfoList[2300] = QuestInfoList[2300] or {["IconName"]="ico_nq.bmp",["Summary"]="",["Title"]="Quest 2300",["Description"]={[1]=""}}
QuestInfoList[2301] = QuestInfoList[2301] or {["IconName"]="ico_nq.bmp",["Summary"]="",["Title"]="Quest 2301",["Description"]={[1]=""}}
QuestInfoList[2302] = QuestInfoList[2302] or {["IconName"]="ico_nq.bmp",["Summary"]="",["Title"]="Quest 2302",["Description"]={[1]=""}}
QuestInfoList[18360] = QuestInfoList[18360] or {["IconName"]="ico_nq.bmp",["Summary"]="",["Title"]="Episode 21 - Story Gate",["Description"]={[1]=""}}
QuestInfoList[18368] = QuestInfoList[18368] or {["IconName"]="ico_nq.bmp",["Summary"]="",["Title"]="Chapter 1 - Story Progress",["Description"]={[1]=""}}

-- Battle-mode completion rewards; expedition quest 16399 retains its own reward.
if QuestInfoList[16400] then
    QuestInfoList[16400].Description = {
        "Complete all seven combat zones and defeat the Unknown Swordsman. Speak with Sierra in the final chamber to claim all completion rewards, including one random EDDA weapon."
    }
    QuestInfoList[16400].RewardItemList = {
        {ItemID = 25786, ItemNum = 9},
        {ItemID = 25787, ItemNum = 15},
        {ItemID = 102571, ItemNum = 1}
    }
end
