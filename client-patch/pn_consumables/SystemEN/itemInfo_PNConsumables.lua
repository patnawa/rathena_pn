-- Correct reviewed consumable text without replacing item identities or artwork.
local function replace_text(id, old, new)
    local item = tbl[id]
    if not item or type(item.identifiedDescriptionName) ~= "table" then return end
    for index, line in ipairs(item.identifiedDescriptionName) do
        local first, last = line:find(old, 1, true)
        if first then
            item.identifiedDescriptionName[index] = line:sub(1, first - 1) .. new .. line:sub(last + 1)
        end
    end
end
for _, id in ipairs({12411, 16267}) do
    replace_text(id, "EXP rate increases to 200% for 15 minutes.", "Experience gained +200% for 15 minutes.")
end
replace_text(12883, "If the character KO'ed in battle, this effect will disappear!.",
    "This effect remains after death until its duration expires.")
