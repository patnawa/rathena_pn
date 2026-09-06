# Remaining three equipment source audit

Date: 2026-09-06. This is a read-only evidence receipt for a later review batch,
not an implementation or installation. No server item, enchant target, price,
acquisition route, client metadata record, import, or asset was changed.

## Current unresolved state

| Client alias | Numeric ID | Missing server enchant target |
| --- | ---: | --- |
| `Frontier_R_Crown_AT` | 401195 | Group 164 |
| `NP_B_Dagger` | 510200 | Group 36 |
| `SC_B_Axe` | 620064 | Group 36 |

The effective Renewal item import walk read 29,607 records and found no record
for any of these IDs. `ClientItemNames.unresolved_details` classifies all three
as `server_item_missing`, with the IDs above, not `client_name_missing`.
The real Lua 5.1 `ItemDB_To_ItemID` function independently returned these exact
three mappings from the original client name table.

The active `SystemEN/itemInfo.lua` loader was executed with the matching Lua 5.1
runtime. Its actual merged `tbl` contained 26,900 records; all three IDs were
`nil`. The active loader SHA-256 was
`f2628f0aa0b39eae1a984ea5ef56f37a03c1912ed8551d499cec7e1e30e51c38`.
Thus three server definitions and three client metadata records remain absent.
There is no need to invent or supplement their existing client aliases.

A fresh effective initial-enchant comparison restricted to groups 36 and 164
found exactly two target-list mismatches: group 36 lacks the two Booster names,
and group 164 lacks the Frontier crown. Neither group has extra server targets
or any other comparison difference. This is two affected groups and three
missing targets, not two missing recipe groups. Unsupported target additions
remain deliberately withheld with their server item dependencies.

## Exact permitted MuhRO evidence

Reference root:
`C:\Users\Alpha\Downloads\Compressed\MuhRO\MuhRO`.

The following rows are from its loose `System` files, not inferred from a
similarly named item or a numeric-ID pattern:

| ID | Literal property row in `itemInfo_muh.lua` | Resource row in `itemInfo_EN_db.lua` | Description row in `itemInfo_EN_db_fallback.lua` |
| ---: | ---: | ---: | ---: |
| 401195 | 13337 | 392938 | 322871 |
| 510200 | 15149 | 345593 | 402585 |
| 620064 | 15773 | 360052 | 425815 |

The actual `System/itemInfo_EN.lua` was loaded and its real `main()` executed
with explicit recording callbacks for item registration and description APIs.
Process-launch APIs were disabled. It completed with `true, "good"` and emitted:

| ID | Display name | Identified resource | Slots | ClassNum | Unidentified name / resource |
| ---: | --- | --- | ---: | ---: | --- |
| 401195 | Frontier Rune Crown (Alitea) | `Frontier_R_Crown` | 1 | 2733 | Unidentified Headgear / `uheadgear` |
| 510200 | Gust Hail Booster Dagger | `NP_B_Dagger` | 0 | 31 | Unidentified Dagger / `uknife` |
| 620064 | Slash ChopChop Booster Axe | `SC_B_Axe` | 0 | 7 | Unidentified Two-Handed Axe / `uaxe` |

These numbers describe the supplied MuhRO loader's output. In particular,
dagger ClassNum 31 is not silently replaced with category 1, nor asserted to be
the correct native animation choice for this project's client.

Its actual merge/description rules use literal `itemInfo_muh.lua` properties,
the identified resource from `itemInfo_EN_db.lua`, and the fallback description
when the main row has no description. The three main rows contain their
identified resource only, so the effect prose comes from the fallback file.
No costume registration was emitted for these three records.

Concise property leads, not approved server definitions:

- Crown: top headgear, DEF 60, armor level 2, one slot, minimum level 265,
  refinable/gradable, displayed weight 100. Its fallback names refine/grade
  effects and a set with item 520055. These are not imported effect rules.
- Dagger: ATK 190, weapon level 4, zero slots, minimum level 100, refinable.
  The fallback describes Gust/Hail, refine, level, and passive-skill effects.
- Axe: ATK 300, weapon level 4, zero slots, minimum level 100, refinable.
  The fallback describes Slash/Chop, refine, level, and passive-skill effects.

Both Booster property rows omit weight. The loader prints a weight description
only when the property is present and printed none for either Booster. Omission
does not establish weight zero. It also does not justify importing an arbitrary
level-scaling cap, price, trade policy, or acquisition route.

MuhRO is the direct source for its own supplied compatibility data; these local
files are not authoritative Gravity item specifications. A comment labelling a
base table "official" does not establish provenance for every per-item property
or for the separate generated fallback. Prior reviewed items had material
differences between these references and Gravity pages; see
[the weapon audit](druid_missing_weapons_audit.md) and
[the crown audit](druid_missing_crowns_audit.md).

## Primary-evidence boundary

The root agent reopened [Gravity notice 8193](https://ro.gnjoy.com/news/notice/View.asp?BBSMode=10001&curpage=4&seq=8193).
Its Frontier vendor addition confirms that addition/existence, not the complete
properties, effects, set rules, or this project's acquisition implementation.
It must not be used to fill the unsupported fields above.

The root's renewed item-detail requests remained unavailable, and no access
bypass was attempted. The prior item audits record the same limitation for the
crown and attempted [510200](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510200&itemSeq=1)
and [620064](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620064&itemSeq=1)
pages. This document does not claim a newly retrieved complete Gravity page.

## Resource and accessory evidence

Active client root:
`C:\Users\Alpha\Downloads\Compressed\Data2026\Data`.

The existing read-only GRF reader was invoked without an output directory over
the complete active `DATA.INI` order: `chapter2_native.grf`,
`nebula_upgrade_v2.grf`, `server.grf`, `english.grf`, `new.grf`, `data.grf`.
Only `data.grf` matched the three identified resource stems. Its index is GRF
version 0x300 with 241,336 entries; all 16 matches have entry type 1.

Exact indexed paths use the following directory/stem combinations; brace lists
enumerate individual files, not inferred alternate resources:

| Directory | Indexed filename(s) |
| --- | --- |
| `data\texture\유저인터페이스\item\` | `frontier_r_crown.bmp`, `np_b_dagger.bmp`, `sc_b_axe.bmp` |
| `data\texture\유저인터페이스\collection\` | `frontier_r_crown.bmp`, `np_b_dagger.bmp`, `sc_b_axe.bmp` |
| `data\sprite\아이템\` | `frontier_r_crown.{act,spr}`, `np_b_dagger.{act,spr}`, `sc_b_axe.{act,spr}` |
| `data\sprite\악세사리\남\` | `남_frontier_r_crown.{act,spr}` |
| `data\sprite\악세사리\여\` | `여_frontier_r_crown.{act,spr}` |

Korean directories above represent the original CP949 GRF name bytes, which the
reader exposes through Latin-1. The indexed uncompressed sizes corroborate the
roles: inventory BMPs 1782/1654/1654 bytes, collection BMPs 22856 bytes each,
ground ACTs 116 bytes each, ground SPRs 1306/1197/1297 bytes, and each worn crown
ACT/SPR 48276/3178 bytes. `MuhRO\MuhRO\muh.grf` separately indexes these same
16 resource paths. No reference asset was copied.

Actual Lua execution of the existing extracted client accessory tables returned
`ACCESSORY_IDs.ACCESSORY_Frontier_R_Crown == 2733` and
`AccNameTable[2733] == "_Frontier_R_Crown"`. Source directory:
`server-work/missing-crowns-client-source-20260906/data/data/luafiles514/lua files/datainfo`.

Important limit: this pass inspected archive indexes, not decoded asset payloads,
and performed no graphical client rendering. Presence, type and length are not
a new decompression, integrity, animation, or in-game visual proof.

An exact-stem search for the MuhRO unidentified resources `uheadgear`, `uknife`,
and `uaxe`, with BMP/SPR/ACT extensions, returned no matches across all six active
GRFs. This does not characterize native missing-resource fallback or every loose
file, but it prevents claiming the full MuhRO registration is already proven
drop-in compatible. No arbitrary unidentified-resource placeholder was chosen.

## RockMMO cross-check

Reference root:
`C:\Users\Alpha\Downloads\Compressed\RockMMO Jormungand 220126\RockMMO Jormungand 220126`.

Its loose `System` text item-info sources had no matches for these IDs or aliases.
The two binary Lua 5.1 tables, `itemInfo_indoor.lub` and `itemInfo_true.lub`, were
executed in a restricted environment and both had all three `tbl[id]` entries
absent. This avoids mistaking a text search of compiled Lua for a numeric-ID
absence proof.

Read-only index searches of `ragnarock.grf`, `local.grf`, `jormungand.grf`,
`sdata.grf`, `data.grf`, `beta.grf`, and `edda.grf` found none of the three
identified resource stems. This is the bounded archive set inspected, not a
claim that every file in the Rock installation was decoded or searched.

## Source hashes and next actions

SHA-256 values for the unchanged evidence files:

| Source | SHA-256 |
| --- | --- |
| MuhRO `System/itemInfo_muh.lua` | `a07836ff04fcc3525dc07f12c03139562ac237fdcc2f47547f1320f310c30fa4` |
| MuhRO `System/itemInfo_EN.lua` | `379a308e3392c3cc385d8a862b9eb05fa4d2ffe3c9f7d7c68804b437decc501b` |
| MuhRO `System/itemInfo_EN_db.lua` | `2834833d0438219b46e03224e19ebe94f0b018c725aa43dab7df94fe5ef4a4e6` |
| MuhRO `System/itemInfo_EN_db_fallback.lua` | `235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a` |
| Original `audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub` | `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496` |
| Extracted `datainfo/accessoryid.lub` | `6667e70ecceb3743939d749c0db1af045be086dce12f4ac7a6e88def7254745c` |
| Extracted `datainfo/accname.lub` | `6e8126b68ea009201318964903ba5adfe75430a0d5f6dbe0266186ab5c286ba2` |

A later explicitly reviewed compatibility patch can use the exact identified
names/resources as evidence, but must resolve unidentified artwork and native
weapon-view behavior before claiming complete client compatibility. Complete
server equipment needs supported properties/effects and the crown's supported
set dependencies; client display metadata alone does not provide those.

Until that evidence or a separately approved compatibility policy is available,
retain all three server and client omissions and the two target-list mismatches
in groups 36/164. Do not manufacture inert items to make an audit pass, weaken
the fail-closed resolver, copy long reference descriptions, invent prices, or
infer an acquisition route from the vendor notice. No implementation, native
gameplay test, deployment, or push is claimed by this receipt.
