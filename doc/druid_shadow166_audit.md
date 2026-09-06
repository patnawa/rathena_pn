# M. Alitea Shadow equipment / group 166

Audit date: 2026-09-06. This is a bounded implementation of four missing target
items, seven missing enchant cards, seven native set combinations, and eighteen
perfect-enchant recipes. It does not define a new equipment acquisition economy.

## Source hierarchy and evidence

The original active client supplies numeric identity and recipes. Gravity kRO's
item database supplies the four targets' unique effects and properties. The
official Gravity Game Link guide supplies the generic Shadow refinement rule.
The seven Soul effects use the user's explicitly permitted loose MuhRO reference;
they are reference-compatible effects, not independently verified kRO balance.
No protected GRF was decrypted, and no artwork was imported.

Primary pages inspected on 2026-09-06 (short paraphrases, not copied descriptions):

| Source | Relevant evidence |
| --- | --- |
| [Gravity kRO, item 1270183](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1270183&itemSeq=4) | M. Alitea Shadow Armor, Alitea-only, level 200, weight zero, armor location. Shield pair gives all trait stats +2; shield/armor/shoes trio gives P.ATK/S.MATK +1 and, at total refine 27+, 50% physical/magical DEF bypass except players. Full six-piece set bypasses 20% physical/magical resistance except players. |
| [Gravity kRO, item 1270184](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1270184&itemSeq=4) | Corresponding Shoes, same job/level/weight; its own Master Shield pair gives all trait stats +2. |
| [Gravity kRO, item 1270185](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1270185&itemSeq=4) | Corresponding Earring. Master Weapon pair gives all trait stats +2; weapon/earring/pendant trio gives P.ATK/S.MATK +1 and the same total-refine-27 DEF bypass, excluding players. |
| [Gravity kRO, item 1270186](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1270186&itemSeq=4) | Corresponding Pendant and its own Master Weapon pair, all trait stats +2. |
| [Gravity kRO, Master Shadow Weapon 24792](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=24792&itemSeq=4), [Master Shadow Shield 24793](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=24793&itemSeq=4) | Both expressly support primary fourth and second upper-expanded jobs, level 200, weight zero, zero slots. Existing effects are preserved, not reimplemented. |
| [Gravity Game Link, Shadow Gear Equipment, 2020-02-26](https://ro.gnjoy.id/news/detail/244) | Generic per-refine bonus: Shadow Weapon ATK/MATK +1; other Shadow Gear Max HP +10. Maximum Shadow refinement is +10. This regional primary rule agrees with existing native M-class Shadow item scripts. Direct navigation now redirects to the publisher's farewell page; the search index still returned the original dated publisher page and complete refinement section when inspected. No regional acquisition prices or trade rules are imported. |
| [Gravity kRO notice 7326, 2020-10-30](https://ro.gnjoy.com/news/notice/View.asp?BBSMode=10001&curpage=35&search=contents&seq=7326&srhval=%ED%98%84%EC%83%81%EC%9D%B4) | Shadow refinement above +10 was a bug; affected items were restored to +10. Independent kRO cap corroboration. |

`itemInfo_EN_db_fallback.lua` contains all seven Soul descriptions, each expressing
+2% named-skill damage plus another +1% per two refine levels. Its target equipment
records stop at 1270182, so it cannot supply the four new target effects.
`itemInfo_EN_db.lua` has only BK artwork aliases for those four IDs. Those aliases
are not effect evidence. `itemInfo_muh.lua` also lacks their literal records.

Read-only upstream checks included [Druid PR #9765](https://github.com/rathena/rathena/pull/9765)
at head `92224e78e5f480c648363f07a908ba747436cdff` and current official rAthena
equipment data; neither supplied these four records. The primary Gravity pages
closed that gap. No effects were inferred from other classes' resource reuse.

Local evidence paths are relative to the repository unless absolute:

| Evidence | SHA256 |
| --- | --- |
| `../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub` (CP949; original group 166) | `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d` |
| `../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub` (original Lua 5.1 binary) | `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496` |
| `C:/Users/Alpha/Downloads/Compressed/MuhRO/MuhRO/System/itemInfo_EN_db_fallback.lua` | `235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a` |
| Same directory, `itemInfo_EN_db.lua` | `2834833d0438219b46e03224e19ebe94f0b018c725aa43dab7df94fe5ef4a4e6` |
| Same directory, `itemInfo_muh.lua` | `a07836ff04fcc3525dc07f12c03139562ac237fdcc2f47547f1320f310c30fa4` |

The eleven numeric names also agree with the previously pinned
[ROenglishRE name table](https://github.com/llchrisll/ROenglishRE/blob/66cdfec631603fda6a90ba4bbe26ab07b5204c84/Additions/data/luafiles514/lua%20files/ItemDBNameTbl.lub).
See `remaining_enchant_coverage_audit.md` for its separate provenance.

## Exact implementation

`db/import/druid_shadow166_items.yml` has eleven new records and two partial
Master-partner overrides. The four targets are native `ShadowGear`, weight/slots
zero, level 200, refineable, correct equipment positions, `Jobs: Alitea`, and
`Classes: Fourth`. The native `bMaxHP,getrefine()*10` script supplies the generic
non-weapon Shadow refine bonus. No ATK/DEF/grade/trade/drop/binding/acquisition
flags or other unique effects are invented.

| ID | Aegis identity | Effect / native skill |
| --- | --- | --- |
| 1270183 | `S_AT_Armor` | M. Alitea Shadow Armor |
| 1270184 | `S_AT_Shoes` | M. Alitea Shadow Shoes |
| 1270185 | `S_AT_Earring` | Right Shadow Accessory / Earring |
| 1270186 | `S_AT_Pendant` | Left Shadow Accessory / Pendant |
| 314804 | `AT_Soul_AC` | Alpha Claw, `AT_ALPHA_CLAW` 6580 |
| 314805 | `AT_Soul_FF` | Frenzy Fang, `AT_FRENZY_FANG` 6582 |
| 314806 | `AT_Soul_PS` | Pinion Shot, `AT_PINION_SHOT` 6586 |
| 314807 | `AT_Soul_QS` | Quill Spear, `AT_QUILL_SPEAR` 6588 |
| 314808 | `AT_Soul_GS` | Glacial Shard, native spelling `AT_GLACIER_SHARD` 6594 |
| 314809 | `AT_Soul_RP` | Roaring Piercer, `AT_ROARING_PIERCER` 6597 |
| 314810 | `AT_Soul_TH` | Terra Harvest, `AT_TERRA_HARVEST` 6603 |

Soul arithmetic is native integer `2+getrefine()/2`: at host refinements 0..10 it
gives 2,2,3,3,4,4,5,5,6,6,7 percent. The current host item index is used, not another
equipped piece or total set refinement. Enhanced Quill Spear 6589 and Roaring
Piercer 6598 inherit their canonical parents through the existing native mapping.

`db/import/druid_shadow166_combos.yml` registers four separate +2 all-trait pair
bonuses, the two three-piece bonuses, and one six-piece resistance bonus.
`bAllTraitStats` means POW/STA/WIS/SPL/CON/CRT, not STR/AGI/VIT/INT/DEX/LUK.
Each all-race ignore bonus has matching negative `RC_Player_Human` and
`RC_Player_Doram` entries. Native additive race semantics therefore leave both
player races at zero while preserving monster bypass. Both qualifying trios
stack to 100% DEF/MDEF bypass; four pairs stack to +8 each trait. Full-set RES/MRES
bypass is a separate 20%, with no invented refinement condition.

### Required native eligibility correction

Alitea is encoded as `JOBL_THIRD|MAPID_KARNOS`, not raw `JOBL_FOURTH`, even though
it is a trait-era second upper-expanded class. Merely adding `Jobs: Druid` would
set the wrong native job bank, and `Classes: Third` on Master partners would admit
unrelated primary third classes. Neither workaround is used.

Root's bounded engine correction makes the Fourth item-class predicate use
`pc_is_trait_job(sd->class_)`, preserving the old branches and independent Jobs
filter. The two partial records 24792/24793 copy the entire existing Jobs map and
add only `Alitea: true`; all other fields, especially `Classes: Fourth` and
scripts, remain unchanged. This is necessary because native item parsing replaces
Jobs maps instead of merging them.

### Recipes and consumption

`db/import/druid_shadow166_enchants.yml` exactly mirrors group 166:

| Native card index / order | Perfect choices | Zeny | Essence per attempt |
| --- | --- | ---: | ---: |
| 3, first | `M_Pow3`, `M_Sta3`, `M_Wis3`, `M_Spl3`, `M_Con3`, `M_Crt3` | 0 | 1 |
| 2, second | `Nimble_Soul`, `Casting_Soul`, `Critical_Soul`, `Expert_Soul`, `Robust_Soul` | 0 | 3 |
| 2, second | Seven `AT_Soul_*` above | 0 | 5 |

All four targets are allowed; minimum refine and enchant grade are zero, random
options are permitted. There are no normal random tables or upgrade recipes.
Reset is disabled. The original client stores dormant reset success/cost values;
native reset chance/price remain zero and materials absent so they cannot enable
reset accidentally. Zero physical card slots is consistent with Shadow Gear;
the original recipes specifically target native indices 3 then 2.

Read-only native consumption audit: `ItemEnchantDatabase::parseBodyNode` resolves
the exact target, enchant and essence item identities. `clif_parse_enchantwindow_perfect`
checks the opened group and inventory target, chooses the first empty position
in the configured order, checks the requested perfect choice and required funds
and material counts, then charges Zeny/materials through native inventory routines
before logging and replacing the chosen card index. The new data does not bypass
that path. The isolated effects test does not send a packet or claim a live
material-deduction test.

## Access and acquisition boundaries

At initial audit there was no group-166 `item_enchant` entry point, no existing
recipe using these Soul cards elsewhere, and no target/Soul grants in the DB/NPC
search. Existing variable-based service routes were inspected and did not resolve
to 166. Seven Soul definitions alone would not have made the group usable.

Existing `Shadow Gear Enchanter#grademk` at `grademk,40,184` in
`npc/custom/grademk_services.txt` opens group 128. Root owns a bounded menu addition
for group 166, with a separate explanation and cancel path before `close2` /
`item_enchant(166)`. Root added it as option 3 while preserving old open/cancel
options 1/2, and separately verified four actual native dialog paths (162
assertions). It adds no grants or manual fees; native recipes display and consume
their existing essence costs.
The focused data files do not themselves install that menu or active client.

Essence already exists as item 1001253. The existing item 102485,
`S_Enchant_Essence_Box_3`, calls `IG_S_ENCHANT_ESSENCE_BOX_3`, whose `Algorithm: All`
record supplies three essence. This is a verified pre-existing dependency, not a
new material distribution. Existing reform item 103309 handles older skill-shadow
pieces, not these four M. Alitea pieces. Gravity tags the new pieces as Kachua
content, but that tag supplies no exact local selection box/rate or purchase
economy. No target equipment acquisition is invented here; owning the correct
equipment remains a separate prerequisite.

## Client fragment and tests

`client-patch/druid_shadow166/SystemEN/itemInfo_DruidShadow166.lua` returns table
`tbl_druidshadow166`, containing exactly eleven records. It uses existing generic
`EpisodClear20` art, clean-room English names/descriptions, zero slots, correct
Shadow or Enchant type text and weight zero. It includes no acquisition claim.
An index-only check of all six active `DATA.INI` archives found the two generic
resources in `data.grf`: `item/episodclear20.bmp` (1,782 bytes) and
`collection/episodclear20.bmp` (22,856 bytes), both ordinary type-1 entries. None
of the five higher-priority archives overrides them. All eleven records use those
same resources. The existing list tool was called without an output directory;
no resource payload was extracted or decrypted.
Root owns loader wiring/active installation; the test executes the actual Lua
5.1 fragment and unmodified active `F_itemInfoMerge` on synthetic tables, including
unrelated-record preservation, repeat merge and existing-ID behavior.

The focused Python suite compares exact dictionaries rather than permissive
field subsets, preserves every original item field/group/upgrade/combo using
whole-overlay baseline suppression, verifies pinned original client hashes,
and checks optional final Renewal imports.

Its native test freshly compiles current `itemdb.cpp`, `script.cpp`, `pc.cpp`,
`skill.cpp`, `clif.cpp`, `malloc.cpp` and the driver under ASan/UBSan. Existing
support objects only satisfy other link dependencies. Existing fixture doubles
isolate world lookup, logging and UI; mandatory seccomp denies sockets, connect,
bind and listen. It parses actual YAML through native item/combo/enchant parsers,
executes actual item/combo bytecode, exhausts all 1,331 normal-range refinement
triples for each three-piece set, validates player exclusion and additive effects,
and tests actual `pc_isequip` without GM equipment permission.

The equip matrix uses the exact pre-fix predicate from `53df51fe3` as independent
oracle for every valid `pc_jobid2mapid` job and all 128 class masks. It verifies
no former eligibility is lost and new admissions are only trait-era expanded
jobs with the Fourth item flag. Minimum/maximum level, sex, broken-item and Jobs
negative controls remain active. Map restrictions are explicitly bypassed in
this mapless fixture. Invalid/malformed job IDs, live characters, network packets,
native rendered client UI, full status recalculation and actual player acquisition
are outside this bounded proof.

Completed verification: focused suite **10/10 passed, no skips**, with all original
external arguments and `--native-vm --require-import`. Native results: **184,027
assertions**, **2,662** three-piece refine cases, **159** supported jobs times
**128** class masks (**20,352** combinations), exactly **160** newly eligible
trait-expanded combinations, and no former eligibility lost. ASan/UBSan and the
native allocator completed cleanly with no leaks. Full enchant unittest discovery
passed **71 tests** (**58 passed, 13 optional external/native tests skipped**).
The independent old-predicate fixture is SHA256-pinned after normalizing only its
function name, preventing accidental oracle changes from hiding a regression.

Run from the repository in PowerShell (requires the current root-owned native
eligibility correction and three root imports):

```powershell
wsl --distribution Ubuntu --exec python3 tools/ci/druid_shadow166_enchant_test.py --native-vm --require-import --client '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub' --effect-reference '/mnt/c/Users/Alpha/Downloads/Compressed/MuhRO/MuhRO/System/itemInfo_EN_db_fallback.lua' --lua '../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe' -v
```
