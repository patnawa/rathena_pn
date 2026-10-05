-- Load after the existing itemInfo tables. Reuses original client resources.
local function catalyst(baseId, name, description)
    local base = assert(tbl[baseId], "Missing catalyst base item " .. baseId)
    return {
        unidentifiedDisplayName = name,
        identifiedDisplayName = name,
        unidentifiedResourceName = base.identifiedResourceName,
        identifiedResourceName = base.identifiedResourceName,
        unidentifiedDescriptionName = description,
        identifiedDescriptionName = description,
        slotCount = 0, ClassNum = 0, costume = false
    }
end

local function rental(material, traps)
    local description = {
        "Keep this rental in your inventory to waive " .. material .. " skill costs.",
        "Other skill costs still apply.",
        "Expires 30 days after opening the box, including time offline.",
        "Cannot be traded, sold, mailed, dropped or stored.",
        "^0000CCWeight:^000000 0"
    }
    if traps then
        table.insert(description, 3, "Traps placed with this rental return no trap items.")
    end
    return description
end

tbl_infinitecatalysts = {
    [50150] = catalyst(617, "Infinite Catalyst Box 3", {
        "Choose one 30-day catalyst rental:",
        "Infinite Soul Talisman, Infinite Trap or Infinite Special Alloy Trap.",
        "The rental period includes time offline.",
        "Canceling does not consume this box.",
        "^0000CCWeight:^000000 0"
    }),
    [50151] = catalyst(1000563, "Infinite Soul Talisman (30 Days)", rental("Soul Talisman", false)),
    [50152] = catalyst(1065, "Infinite Trap (30 Days)", rental("Trap", true)),
    [50153] = catalyst(7940, "Infinite Special Alloy Trap (30 Days)", rental("Special Alloy Trap", true))
}
