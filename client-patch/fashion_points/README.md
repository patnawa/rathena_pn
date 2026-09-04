# Fashion Points client metadata

`SystemEN/LuaFiles514/itemInfo_fashion_points.lua` contains metadata for all
37 server items introduced by the isolated Fashion Points imports: 21 boxes,
8 physical stones, and 8 enchant records.

The standard client does not discover fragments automatically. During client
packaging, load this file and merge its returned table into the main
`SystemEN/LuaFiles514/itemInfo.lua` table before that main table is returned.
For example, in a build/merge step:

```lua
local fashion_items = dofile("SystemEN/LuaFiles514/itemInfo_fashion_points.lua")
for id, data in pairs(fashion_items) do
	main_item_info[id] = data
end
```

Adapt `main_item_info` to the table name used by the client build pipeline. A
plain stock `itemInfo.lua` is usually a direct table literal, so a packaging
tool must splice the 37 records or convert that literal to a named table.

The repository client lacks the exact proprietary resources for these IDs.
The fragment therefore uses existing resources from items 12246, 1001326, and
312402 as safe fallback icons. Replace only the resource-name strings if exact,
redistributable client sprites later become available.

The 18 catalogue-only IDs missing from the server snapshot are not defined by
this package. Their published names are known, but equip positions and view IDs
are not; inventing those would create unusable or visually incorrect costumes.
