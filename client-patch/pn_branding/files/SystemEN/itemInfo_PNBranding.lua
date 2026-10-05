-- PN presentation cleanup, applied after every item metadata overlay.
-- Remove external-server guide links without changing item identities/effects.
for _, item in pairs(tbl) do
    for _, field in ipairs({"identifiedDescriptionName", "unidentifiedDescriptionName"}) do
        local description = item[field]
        if type(description) == "table" then
            local kept = {}
            for _, line in ipairs(description) do
                local external = type(line) == "string"
                    and line:find("<URL>", 1, true)
                    and line:find("https?://")
                if not external then kept[#kept + 1] = line end
            end
            item[field] = kept
        end
    end
end
