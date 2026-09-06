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

## Supported and withheld outcomes

The server has functional item scripts for 358 of the page's 366 stone/enchant
pairs. The following physical stones and corresponding enchant cards remain
visible in item-info only so existing/recovered items have names:

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

They are not awarded by boxes and the application NPC rejects them. The local
server has none of their referenced skill IDs 6526-6606. Enabling them requires
all of the following, not just a recent executable:

1. Integrate and rebuild a complete Druid/Karnos/Alitea server implementation,
   such as [rAthena PR #9765](https://github.com/rathena/rathena/pull/9765).
2. Install matching client-side job, skill, effect, sprite, and translation
   data. The PR author explicitly states client-side support is not included.
3. Give cards 314848-314855 `SubType: Enchant`, reload the item/combo databases,
   and verify every referenced skill and combo effect in-game.
4. Only after those checks, deliberately remove their deny rule from
   `FP_EnchantSupported` and update these warnings.

PR #9765 reports development against a 2025-12-17 client. The inspected
2026-02-19 executable is new enough by date, but that does not prove the
required client-side class resources are installed.

The 18 catalogue-only IDs missing from the server snapshot are outside this
patch. Their published names are known, but equip positions and view IDs are
not; inventing those values would create unusable or visually incorrect items.
