# Catalyst client metadata

Copy `SystemEN/itemInfo_InfiniteCatalysts.lua` into the matching client folder.
After the existing `tbl` item definitions and merge functions have loaded, append:

```lua
dofile("SystemEN/itemInfo_InfiniteCatalysts.lua")
F_itemInfoMerge(tbl_infinitecatalysts, true)
```

The fragment copies resource names from existing item IDs 617, 1000563, 1065 and
7940. It adds no sprite or texture assets and does not modify the original items.
Server definitions are in `db/import/infinite_catalysts.yml`.
