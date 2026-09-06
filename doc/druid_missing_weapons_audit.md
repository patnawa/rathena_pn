# Remaining Druid weapon audit — 2026-09-06

Scope: the fourteen missing weapon identities in the original client's enchant
targets. Twelve have supported definitions and ten new set memberships in this
delivery. The two Booster weapons remain unresolved; this is not fourteen-item
completion. Shared imports, enchant target-list additions, active client install,
server deployment, and acquisition changes are owned separately by the parent.

## Primary evidence and disposition

All linked Gravity kRO item-library pages were inspected on 2026-09-06. They do
not expose a publication date. Names below are stable server identifiers, not
copied descriptions. Weight is shown in player-visible units; the YAML stores
ten times that amount. All twelve supported weapons are weapon level 5,
refineable and gradable. The client uses newly written English summaries.

| Identity | Primary source | ATK / MATK; weight; slots; level | Result |
| --- | --- | --- | --- |
| 510185 `Repeat_Dagger_AD` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510185&itemSeq=1) | 150 / 0; 90; 2; 170 | Dagger, Karnos family. Cast-time, Sharpen Gust and ranged refine effects. |
| 510189 `Solid_Whinger` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510189&itemSeq=1) | 200 / 0; 110; 2; 220 | Alitea dagger; independent refine divisors 2, 3 and 4. |
| 510190 `Glacier_N_Knife` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510190&itemSeq=1) | 210 / 210; 120; 0; 210 | Alitea dagger enchant chassis; slot conflict discussed below. |
| 510191 `D_Glacier_N_Knife` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510191&itemSeq=1) | 210 / 210; 120; 1; 230 | Alitea dagger enchant chassis with two armor sets. |
| 510193 `Dimen_AT_Knife` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510193&itemSeq=1) | 240 / 0; 180; 2; 250 | Alitea dagger; Quill, critical, ranged and crown-set effects. |
| 520047 `F_Ein_AXE` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=520047&itemSeq=1) | 230 / 0; 200; 2; 250 | Alitea one-handed axe; added SP costs at +9 and +11. |
| 520052 `Axe_Furious` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=520052&itemSeq=1) | 230 / 0; 500; 2; 205 | Alitea one-handed axe; level, refine, grade and boots effects. |
| 590104 `Mocadas_Garz` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=590104&itemSeq=1) | 220 / 230; 120; 2; 250 | Alitea mace; Monolith/Nova, water and size-specific magic effects. |
| 590117 `Hall_Furious` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=590117&itemSeq=1) | 100 / 180; 110; 2; 205 | Alitea mace; Terra, Glacial Shard, water/earth and boots effects. |
| 620056 `Glacier_N_Axe` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620056&itemSeq=1) | 350 / 180; 600; 0; 210 | Alitea two-handed axe enchant chassis. |
| 620057 `D_Glacier_N_Axe` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620057&itemSeq=1) | 350 / 180; 600; 1; 230 | Alitea two-handed axe with two armor sets. |
| 620059 `Dimen_AT_Axe` | [Gravity](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620059&itemSeq=1) | 380 / 0; 400; 2; 250 | Alitea two-handed axe; Frenzy, melee and crown-set effects. |
| 510200 `NP_B_Dagger` | [Attempted Gravity page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510200&itemSeq=1) | Incomplete authoritative metadata | **Not implemented.** |
| 620064 `SC_B_Axe` | [Attempted Gravity page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620064&itemSeq=1) | Incomplete authoritative metadata | **Not implemented.** |

The four Glacier chassis are not inert placeholders: their full published
weapon properties, MATK, eligibility and supported breakability are defined.
There is deliberately no invented script on either Glacier dagger. Only axes
and maces have the primary-described indestructibility bonus; it is battle
breakability protection, not guaranteed safe refining.

### Ten exact new sets

The two [Dimensions weapon pages](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620059&itemSeq=1)
and [dagger page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510193&itemSeq=1)
provide the two `Time_DM_R_Crown_AT` sets. Their conditional second effects require
**both** weapon and crown grade A, regardless of either refine level. The
refine sum reads only the right-hand weapon and upper headgear; cooldown is
reduced by 200 milliseconds. The axe improves Savage Lunge; the dagger improves
Tempest Flap unconditionally and Quill Spear conditionally.

[Furious Circlet (Alitea)](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401176&itemSeq=2)
supplies the two unconditional Furious crown sets: the axe adds 10% physical
damage against all monster races, excluding both human and Doram players; the
scepter adds 10% Glacial Shard damage. The two Furious weapon pages supply the
two existing `FuriousBoots` sets using weapon-plus-shoes refine sums. These four
crown memberships are owned only by this overlay, not duplicated in the
coordinated crown overlay.

The [Dim Glacier Armor page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=450270&itemSeq=2)
and [Dim Glacier Robe page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=450271&itemSeq=2)
cover four further memberships: each new Dim Glacier weapon with the physical
armor/boots/manteau trio or magical robe/shoes/muffler trio. Refine bonuses use
the weapon alone. Grades C, B and A add cumulatively 6, 5 and 4 P.ATK/SMATK and
5 corresponding trait points at each grade. Only the two-handed axe receives
20% damage against every target element at grade A. The dagger is excluded by
the primary list of qualifying two-handed types. The existing equivalent
two-handed and one-handed scripts in `db/re/item_combos.yml` corroborate this
interpretation. No existing common set is edited or copied into another
registration, including the existing independent three-armor-piece bonuses.

## Conflicts, defaults and limits

- `Glacier_N_Knife`: the same primary page says `SLOT:0` in its detailed prose
  but displays 2 in its header and summary table. The original group 31 orders
  all four enchant indexes `3,2,1,0`, with perfect recipes on all four. Two card
  slots would collide with its last two enchant slots in the server's real
  `slot < item->slots` guard. This delivery uses 0, supported by explicit primary
  prose, original recipe structure and the permitted loose MuhRO property.
  The conflicting header is not silently discarded as if it never existed.
- `Solid_Whinger`: the primary divisors are 2 for Quill, 3 for Pinion and 4 for
  ranged damage, unlike the permitted fallback's repeated divisor 2.
- `Hall_Furious`: primary base Terra Harvest is 10%, not fallback 5%; its +11
  refine bonus is Glacial Shard, not another Terra bonus.
- `F_Ein_AXE`: +25 SP for each named skill starts at refine 9; refine 11 adds
  another +25. The permitted fallback layout could imply an unconditional
  penalty, which is not used. Native `bSkillUseSP` subtracts its stored value
  from the requirement, so the implemented values are -25, accumulating to -50.
- Neither primary Glacier dagger page says indestructible. The extra fallback
  clause is not imported. Two-handed armor-set target-element bonuses are not
  extended to the dagger merely because the permitted fallback mentions them.
- `Repeat_Dagger_AD` has primary weight 90 even though the permitted literal
  metadata omits weight. This illustrates why an omitted Booster weight cannot
  safely be interpreted as zero.
- Primary pages do not publish item-specific reach. `Range: 1` is an explicit
  project/native-category choice for these daggers, axes and maces, **not** a
  claim of independently documented reach for each weapon. Native default
  sex, prices and trade behavior are left unchanged; no fabricated price or
  custom restriction field is added. Client `ClassNum` uses native category
  numbers 1/6/7/8, not the supplied MuhRO dagger value 31.
- `Jobs: Alitea` plus `Classes: Fourth` represents the primary Alitea restriction
  with the separately reviewed trait-era eligibility fix already in this tree.
  `Jobs: Karnos` on the Repeat dagger intentionally covers Karnos and Alitea's
  shared native second-job bank (including supported Baby Karnos), without admitting base Druid. No existing job
  maps or common item definitions are changed.

### Booster gap: meaningful alternatives checked

Both original numeric names are verified. Permitted loose fallback descriptions
and literal MuhRO records have substantial effect information, but no explicit
weight. `itemInfo_EN.lua` renders weight only when present; it does not establish
a zero default. Repeated official item-page requests (including alternate
scheme/itemSeq), official-domain name searches, and direct read-only HTTP
requests did not retrieve their complete pages. Gravity's
[2025-12-03 Druid developer note](https://ro.gnjoy.com/news/devnote/View.asp?category=3&curpage=1&seq=4195962)
confirms Booster weapons were planned with the class update, but supplies no
complete per-item rules and is not used as a substitute.

The permitted Rock loose item-info files contain neither Booster ID. The
original compiled `datainfo/iteminfo.lub` executes successfully with the matching
Lua 5.1 runtime under disabled process-launch APIs, but contains neither Booster
nor the disputed Glacier dagger. The current official rAthena
[upstream equipment database](https://raw.githubusercontent.com/rathena/rathena/master/db/re/item_db_equip.yml)
was checked read-only: none of these fourteen IDs is present. Retrieved byte
SHA256: `0ded923764d64ad81ad6fae14880f6c0946d91a91ab931d18cce7d4dbcf75f5f`
(6,155,801 bytes). No unsupported Booster definition or enchant-target addition
is included.

The complete current [Druid PR 9765 patch](https://github.com/rathena/rathena/pull/9765)
was also checked read-only. It contains neither Booster name/ID nor the sampled
Repeat/Furious names; it does not close the missing item-property gap. Patch
SHA256: `252080fe0de57d9fbee86b55d0e96de5944ba06bd24ba9b7bbf1bed1dd899477`
(1,512,228 bytes).

## Identity, resources and integration

All fourteen IDs were absent from effective Renewal items before this overlay.
Original item-name chunk SHA256:
`2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496`.
Original enchant-list SHA256:
`664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d`.
Both are verified by the focused test when source paths are supplied.

The standalone client fragment returns twelve records and exposes
`tbl_druidmissingweapons`. It uses only `EpisodClear20`, an existing generic
artwork fallback, not borrowed weapon art. An index-only scan of all six active
`DATA.INI` archives found the item bitmap (1,782 bytes) and collection bitmap
(22,856 bytes) in `data.grf`; no higher-precedence archive shadows them. No
protected contents were extracted, decrypted or imported.

Parent wiring plan: Renewal import the two new databases after the actual
`400999 Time_DM_R_Crown_AT` and `401176 FuriousCirclet_AT` definitions are ready;
merge the fragment through existing `F_itemInfoMerge`. Add only the twelve
existing original targets to their existing groups:

| Group | Supported new targets |
| ---: | --- |
| 24 | `Solid_Whinger` |
| 31 | `Glacier_N_Knife`, `Glacier_N_Axe` |
| 33 | `Repeat_Dagger_AD` |
| 47 | `D_Glacier_N_Knife`, `D_Glacier_N_Axe` |
| 63 | `F_Ein_AXE` |
| 64 | `Mocadas_Garz` |
| 133 | `Dimen_AT_Knife`, `Dimen_AT_Axe` |
| 147 | `Axe_Furious`, `Hall_Furious` |

Read-only exact-name/ID searches in existing item groups, drops, reform,
barters and NPC scripts found no acquisition route for these twelve identities.
An unrelated `GiganticAxe_Furious` match is not a route for `Axe_Furious`.
Definitions and existing enchant services do not themselves supply equipment.
This delivery does not invent drops, vendors, boxes, prices or acquisition.

## Verification

`tools/ci/druid_missing_weapons_test.py` independently checks complete metadata,
unknown fields/economy absence, exact ten memberships, original numeric names,
the four-slot evidence, and preservation of all old effective items and sets.
`--require-import` additionally checks actual Renewal wiring and both crowns.
`--dependency-items db/import/druid_missing_crowns.yml` permits testing the real
candidate crown definitions before shared integration; no fake dependencies.

The Lua test executes the matching Lua 5.1 runtime and actual installed
`F_itemInfoMerge`, proving twelve exact identities, displayed weights/levels,
slots, category values, idempotence, and preservation of an unrelated record
and pre-existing collisions. It tests the fragment, not active installation.

`--native-vm` freshly compiles current itemdb/script/pc/skill/clif/malloc with
ASan/UBSan and denies network sockets. The fixture loads actual YAML and skill
records, tests all refine values 0–20, all grades, every relevant base-level
boundary, actual supported-job equip eligibility without GM permissions,
minimum-level/broken-item controls, all set-member missing controls, duplicate
set prevention, both grade-A gates, actual SP requirements and cooldown
consumption. These are isolated engine proofs, not live combat, acquisition,
packet charging or map-specific restriction proofs.

Candidate result on 2026-09-06: 7 tests successful, with only the deliberately
unrequested import check skipped before wiring. All source and Lua checks ran.
Native result: **16,380 weapon cases, 110,250 set cases, 1,908 job cases
(159 supported jobs times 12 weapons), 16,824,878 assertions**. No parser
warnings/errors, ASan/UBSan errors or allocator leaks. Exact runtime hashes:

- Weapon YAML: `8d52039af9180351197c0a4b6d0b7f5bdbbac58fc983f8c871ab8194447e48fd`.
- Combo YAML: `78c5279ea232e3573d7a8fcb8ec4328ad20e3fcd0599e7ff6cf5c807a008de71`.
- Client fragment: `407ddc4121f2b06a01e28fa93b8d15353191533c04a029ad20744ed157bd6048`.

The candidate command added `--dependency-items db/import/druid_missing_crowns.yml`
and omitted `--require-import`. The post-wiring command is below.

```powershell
wsl --distribution Ubuntu --cd /mnt/c/Users/Alpha/Downloads/Compressed/Data2026/Data/server-work/rathena_pn_push --exec python3 tools/ci/druid_missing_weapons_test.py --require-import --native-vm --client '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub' --lua '../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe' -v
```
