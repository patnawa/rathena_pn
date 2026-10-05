-- PN account-bank metadata. The translated base omits item 12781 entirely.
-- Define it before applying descriptions so the client can register the ticket.
-- Reuse the existing coupon artwork (CP949 bytes); no new GRF is required.
-- Preserve resource identifiers when an installation already defines the item.
if not tbl[12781] then
    tbl[12781] = {
        unidentifiedResourceName = "\196\237\198\249",
        unidentifiedDescriptionName = { "" },
        identifiedResourceName = "\196\237\198\249",
        slotCount = 0,
        ClassNum = 0,
        costume = false,
        Custom = true
    }
end

local descriptions = {
    [6024] = {
        "Retired currency. Use direct Zeny instead.",
        "Existing diamonds were converted to Bank Zeny during maintenance.",
        "^0000FFConversion value:^000000 499,000,000 Zeny",
        "Use ^0000FF@bank^000000 or ^0000FFCtrl+B^000000 to open Wallet & Bank."
    },
    [12781] = {
        "Retired currency. Use direct Zeny instead.",
        "Existing tickets were converted to Bank Zeny during maintenance.",
        "^0000FFConversion value:^000000 998,000 Zeny",
        "Use ^0000FF@bank^000000 or ^0000FFCtrl+B^000000 to open Wallet & Bank.",
        "^0000CCWeight:^000000 0"
    }
}
for id, lines in pairs(descriptions) do
    if tbl[id] then tbl[id].identifiedDescriptionName = lines end
end
tbl[12781].identifiedDisplayName = "1M Zeny Ticket"
tbl[12781].unidentifiedDisplayName = "1M Zeny Ticket"
