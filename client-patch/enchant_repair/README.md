# Enchantment client patch

Close the game before installation. Back up `DATA.INI` and `SystemEN/itemInfo.lua`, then copy `DATA.INI`, `enchant_repair.grf` and the `SystemEN` directory into the matching server client directory. Restart the game.

The supplied DATA.INI preserves this client's existing eight archives and loads enchant_repair.grf first. For other client layouts, merge the archive entry and the final itemInfo_EnchantRepair loader/merge calls into their existing files instead of replacing them.

`source/` contains the three Lua tables packed into the GRF: EnchantList, ItemDBNameTbl and LapineUpgradeBox. Install this patch together with the enchantment server database/NPC changes in this commit. The item-info overrides require the existing base client definitions and art.

Validation: 164 native enchant groups with no comparison issues, 7,298 Lua callbacks, and ten uniquely resolved Shadow book IDs with exact target registrations. The installed GRF was independently extracted and its entries matched these source files.
