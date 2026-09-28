-- Load after the existing QuestInfoList. Independent missionary quest descriptions.
local function memoQuest(id, title, lines, summary)
    QuestInfoList[id] = { Title=title, IconName="ico_qs.bmp", Description=lines, Summary=summary or "" }
end
memoQuest(16586, "The Path of Missionary Work", {
    "Cardinals and Inquisitors with Warp Portal level 4 can help Missionary Rosetta in Prontera (225,330).",
    "Finish the Umbala and Einbroch missions in either order to expand from three to six memo destinations.",
    "Resetting skills removes the extra slots and resets this quest."
}, "Report to Missionary Rosetta")
memoQuest(16587, "Umbala Missionary Work", {"Speak with Tita in Umbala (127,140)."}, "Help the missing missionaries")
memoQuest(16588, "Einbroch Missionary Work", {"Speak with Suan in Einbroch (178,148)."}, "Find the missing children")
local rescues = {
    {16589, "Jhon", "nif_dun01", 101, 246},
    {16590, "Maria", "nif_dun01", 213, 148},
    {16591, "Blue", "nif_dun01", 160, 183},
    {16593, "Las", "ein_dun03", 148, 221},
    {16594, "Minas", "ein_dun03", 38, 152},
    {16595, "Tiris", "ein_dun03", 234, 20}
}
for _, rescue in ipairs(rescues) do
    memoQuest(rescue[1], "Rescue " .. rescue[2], {
        "Find " .. rescue[2] .. " at " .. rescue[3] .. " (" .. rescue[4] .. "," .. rescue[5] .. ").",
        "Approach to reveal the missing person, then talk to send them home."
    }, "Rescue " .. rescue[2])
end
memoQuest(16592, "Banquet of the Dead - Hunt", {"Defeat 100 Brutal Murderers in nif_dun01 and rescue all three missionaries.", "Return to Tita in Umbala when both objectives are complete."}, "100 Brutal Murderers")
memoQuest(16596, "Einbech Mine - Hunt", {"Defeat 100 Abyssmen in ein_dun03 and rescue all three children.", "Return to Suan in Einbroch when both objectives are complete."}, "100 Abyssmen")
for id=16597,16600 do
    memoQuest(id, "Return to Missionary Rosetta", {"Report to Missionary Rosetta in Prontera (225,330)."}, "Return to Prontera")
end
