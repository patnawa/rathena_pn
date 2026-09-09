-- Original translation works of zackdreaver: https://github.com/zackdreaver/ROenglishRE
-- Continuated by llchrisll at https://github.com/llchrisll/ROenglishRE
-- This file can be distributed, used and modified freely
-- This file shouldn't be claimed as part of your project, unless you fork it from https://github.com/llchrisll/ROenglishRE
-- Credits to Neo-Mind for the code it was originally based on.
-- For further information, please visit https://llchrisll.github.io/ROTPDocs/guides/customs/#multi-iteminfo-support.

-- Load the splitted function file
require("SystemEN/LuaFiles514/itemInfo_f")

-- Load the translation file
dofile("SystemEN/LuaFiles514/itemInfo.lua")

------------------------------- Load additional files -------------------------------
----------------------- like custom items, overrides and others ---------------------

-- Place all files in the "SystemEN" folder, the rest will be automatically added.
ImportFiles = {
	"itemInfo_C.lua", -- custom items
	"itemInfo_ZeroCell.lua", -- Zero Cell
	"itemInfo_Chapter2.lua", -- Chapter 2 Flame Branch
	"itemInfo_Fashion.lua", -- Fashion Points
	"itemInfo_DruidItems.lua", -- Druid-family compatibility items
	"itemInfo_Chapter2Materials.lua", -- Native Chapter 2 materials
	"itemInfo_DruidGear.lua", -- Druid Clock Tower tuning enchants
	"itemInfo_EnchantTargets.lua", -- Existing server crown metadata
	"itemInfo_DruidShadow166.lua", -- Alitea Shadow equipment and Soul enchants
	"itemInfo_DruidMissingCrowns.lua", -- Reviewed crowns and required Sky weapons
	"itemInfo_DruidMissingWeapons.lua", -- Reviewed Druid equipment
}
-- Just define the table postfix, 'tbl_' will be automatically added
-- Make sure the table names are unique, like tbl_kro, tbl_jro, etc.
-- Note: The "tbl_override" is handled separately at the end.
ImportTables = {
	"custom",
	"zerocell",
	"chapter2",
	"fashion",
	"druiditems",
	"chapter2materials",
	"druidgear",
	"enchanttargets",
	"druidshadow166",
	"druidmissingcrowns",
	"druidmissingweapons",
}

---------------- Additional Configs for translation file ----------------
-- Display origin server based on translation file's Server argument
-- 0 = disable/1 = in Item Name/2 = top of description/3 = bottom of description
DisplayServer = 3

-- Defines how the item id will be shown in item name, doesn't take effect in other settings
TagStart = '('
TagEnd = ')'

-- Define the colour in which the Server Name should be shown (affects official)
-- Format: '^<RRGGBB>'
-- '' = same color as "Server: " (^0000CC = blue)
-- '^FFFFFF' = white
ServerColour = '^FF0000'

-- Show ItemID at bottom (affects custom items as well)
-- 0 = disable/1 = top of description/2 = bottom of description
DisplayItemID = 2

-- Display a database link at bottom of description (true/false)
DisplayDatabase = true

-- Remove the Weight lines for all items (true/false)
RemoveWeight = false

---------------- Additional Configs for custom items ----------------
-- Display server name
-- 0 = disable/1 = in Item Name/2 = top of description/3 = bottom of description
DisplayCustomServer = 0

-- Defines how the item id will be shown in item name, doesn't take effect in other settings
CustomTagStart = '['
CustomTagEnd = ']'

-- Server Name for your custom items
CServerName = 'Custom'

-- Define the colour in which the custom Server Name should be shown (custom items)
-- Format: '^<RRGGBB>'
-- '' = same color as "Server: " (^0000CC = blue)
-- '^FFFFFF' = white
CServerColour = '^00FF00'

-- Database link for custom items, like fluxcp (true/false)
DisplayCustomDB = false

----- Table for Database Links -------------------
ItemDatabase = {
	["Divine-Pride"] = {
		Name = "Divine-Pride.net",
		URL = "https://www.divine-pride.net/database/item/"
	},
	["iRO"] = {
		Name = "iRO Wiki",
		URL = "https://db.irowiki.org/db/item-info/"
	},
	["Custom"] = {
		Name = "Database",
		URL = "http://127.0.0.1/?module=item&action=view&id="
	}
}

---------------- DON'T TOUCH THE LINES BELOW unless you know what you are doing ----------------
require('SystemEN/LuaFiles514/rotp_f')

-- Loop through each file in the "ImportFiles" table and load them
for _, v in ipairs(ImportFiles) do
	dofile('SystemEN/'..v)
end

-- Loop through each table in the "ImportTables" table
-- and merge them into the main table "tbl"
for _, v in ipairs(ImportTables) do
	F_itemInfoMerge(_G['tbl_'..v])
end

F_itemInfoMerge(tbl_override, true) -- official overrides
---------------------------------------------------

-- Reviewed enchantment repairs; only the declared records are overridden.
dofile("SystemEN/itemInfo_EnchantRepair.lua")
F_itemInfoMerge(tbl_enchantrepair, true)

-- Reviewed episode resource compatibility.
dofile("SystemEN/itemInfo_ClientCompat.lua")
