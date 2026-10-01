# Regional costume stone item-info overlay

Load `SystemEN/itemInfo_CostumeStones.lua` after the existing item-info imports,
then call `F_itemInfoMerge(tbl_costumestones, true)`. Do not replace the main
item-info table: other custom overlays must remain loaded.

The signed September 28 costume-stone update also merges the reviewed missing
artwork and effect tables into `client_repairs.grf`. Installing only this Lua
file is insufficient for those effects. The complete delta and asset hashes are
in `Server-Development/costume-stone-audit-20260928` and the signed launcher feed.
See `doc/costume_stone_expansion_20260928.md` for provenance and limitations.

`build_client.py` in that audit directory generated this overlay from the
reviewed official metadata JSON, Thai English translations, installed base
metadata and the repository's effective item database. CP949 resource bytes
are preserved as decimal Lua escapes. It executes the actual item-info loader
with native Lua5.1 and an overlay path resolver to verify every catalog ID.
