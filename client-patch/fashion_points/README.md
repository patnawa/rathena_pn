# Fashion Points client metadata

This patch accompanies the server implementation sourced from revision 61305
(2026-07-29) of the published
[MuhRO Fashion Points page](https://wiki.muhro.eu/Costume_Enchants_(Fashion_Points)).
`SystemEN/LuaFiles514/itemInfo_fashion_points.lua` defines the 37 items added by
the isolated server imports: 21 boxes, 8 physical stones, and 8 enchant cards.

## Install for this client

The matching server must be rebuilt with the custom `modifyequipitem` script
command before the Fashion application/recovery NPCs are loaded. This client
patch supplies item presentation only; it does not add server behavior.

The inspected client uses ROenglishRE's multi-iteminfo loader. From this
directory, run:

```powershell
powershell.exe -ExecutionPolicy Bypass -File .\install_client_patch.ps1 `
  -DataRoot 'C:\path\to\Ragnarok\Data'
```

The installer verifies that `SystemEN/itemInfo.lua` is a compatible
multi-iteminfo loader, makes one-time `*.bak-before-fashion` backups, copies the
fragment, creates `SystemEN/itemInfo_Fashion.lua`, and registers the `fashion`
table. It is idempotent and can be rerun after updating the fragment. Close all
game clients before installing, preserve loose-data/GRF priority for
`SystemEN`, and relaunch the client afterward.

For another packaging system, merge every returned entry into the client's
main item-info table. Merely placing the fragment in `LuaFiles514` is not
enough; stock clients do not discover arbitrary item-info fragments.

```lua
local fashion_items = dofile("SystemEN/LuaFiles514/itemInfo_fashion_points.lua")
for id, data in pairs(fashion_items) do
	main_item_info[id] = data
end
```

Adapt `main_item_info` to the build pipeline's actual table. A plain stock
`itemInfo.lua` is commonly a direct table literal, so its packaging step must
splice the 37 records or convert the literal to a named table.

The repository does not contain the proprietary sprites for these IDs. The
fragment intentionally uses the existing ASCII-safe `EpisodClear20` resource
as a fallback icon. Replace only the resource-name strings after obtaining
exact redistributable resources.

## Supported outcomes

All 366 published stone/enchant pairs are enabled with the Druid integration.
The eight newly enabled pairs are:

| Physical stone | Enchant card | Dependency |
| ---: | ---: | --- |
| 1002625 | 314848 | Karnos garment skills |
| 1002626 | 314849 | Alitea garment skills |
| 1002627 | 314850 | Druid upper skills |
| 1002628 | 314851 | Druid middle skills |
| 1002629 | 314852 | Druid lower skills |
| 1002630 | 314853 | Karnos upper skills |
| 1002631 | 314854 | Karnos middle skills |
| 1002632 | 314855 | Karnos lower skills |

The referenced skills are now implemented, all eight cards explicitly use
`SubType: Enchant`, and the former deny rule is removed. Boxes and the application
service include them while retaining the existing inventory/transaction guards.
The updated metadata no longer labels these outcomes unavailable.

See [Druid integration evidence and limits](../../doc/druid_integration.md).
The active client's job IDs, skill IDs, visible tree entries and descriptions
are checked separately from in-game effects. Transformation rendering and every
fashion skill/combo bonus still need player-attached verification; enabling
the data is not a claim that those gameplay tests have passed.

The 18 catalogue-only IDs missing from the server snapshot are outside this
patch. Their published names are known, but equip positions and view IDs are
not; inventing those values would create unusable or visually incorrect items.
