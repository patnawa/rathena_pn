# Druid weapon acquisition audit - 2026-09-06

This is a read-only follow-up to `druid_missing_weapons_audit.md`. The twelve
supported weapon definitions, ten sets and client fragment remain frozen.
**No acquisition change is implemented or approved by this document.** The two
unresolved Booster weapons remain outside this twelve-weapon scope.

Exact-name and whole-number searches of `npc/` and `db/` found these twelve
identities only in their new definitions, sets and enchant target overlay, not
in a vendor, drop, crafting output or box table. Effective Renewal item,
reform and synthesis imports were also inspected. An enchant service requires
equipment already in the inventory; it does not close an acquisition gap.
This finding concerns the checked project data, not unknown live inventory,
external distribution or GM item-creation commands.

## Prioritized disposition

The priority below orders follow-up research by the specificity of existing
evidence and the availability of an existing project service. A known
counterpart price is **not** presented as an exact Druid price.

| Priority | Exact new output(s) | Existing project counterpart | Evidence still required before implementation |
| --- | --- | --- | --- |
| 1 | 590104 `Mocadas_Garz` | Slab synthesis pattern; currently commented out | Druid tablet identity/usable behavior, output quantity and consumption/success rules; reachable tablet/material supply. Three exact material counts are primary-sourced below. |
| 2 | 510190 `Glacier_N_Knife`, 620056 `Glacier_N_Axe` | Maram's working Glacier shop | Exact Nature-weapon shop membership and price, or an explicitly approved project decision to use the existing 150,000-Zeny family price. |
| 3 | 520052 `Axe_Furious`, 590117 `Hall_Furious` | Tunkarom's working Furious coupon barter | Druid membership in this coupon. The inspected official list explicitly omits both. Coupon supply also needs an end-to-end check. |
| 4 | 510185 `Repeat_Dagger_AD` | Frasa's working OS reform and Elyumina's random OS weapon group | Missing 510184 base weapon; Druid reform/material/retention rules and base-output distribution. |
| 5 | 520047 `F_Ein_AXE` | Existing amplification blueprints/reform records | Missing 520046 base axe and 105210 blueprint; exact Druid material counts and preservation/consumption rules. Counterpart costs differ. |
| 6 | 510191 `D_Glacier_N_Knife`, 620057 `D_Glacier_N_Axe` | Forr enchant and Furnace extraction, neither creates weapons | Exact Druid drop membership, monster IDs and rates; inspected official regional source gives only family-level maps/method. |
| 7 | 510189 `Solid_Whinger`; 510193 `Dimen_AT_Knife`, 620059 `Dimen_AT_Axe` | Solid counterpart lottery records; Dimensions counterpart refine/enchant records | An authoritative ordinary acquisition route and complete output/cost rules; no matching project creator was located. |

All primary links below were opened on 2026-09-06. Gravity's kRO item-library
pages are undated; dates are given where an update article publishes one.
Summaries are newly written, not copied third-party descriptions. Regional
iRO behavior is labeled and is not silently promoted to current kRO Druid
behavior.

## 1. Garz: exact materials, incomplete service chain

The [Gravity Garz tablet page, item 105211](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=105211&itemSeq=6)
explicitly identifies a recipe for Mocadas Garz and requires:

| Material identity in this project | Exact primary count |
| --- | ---: |
| 1001201 `Mocadas_Gold` - Bishop's Relic | 100 |
| 1001202 `Mocadas_Water` - Bishop's Holy Water | 100 |
| 1001200 `Crystal_Of_Pollution` - Contaminated Crystal | 100 |

The page also supplies tablet weight 125. Its displayed database price
100,000 is **not evidence of an NPC selling the tablet for that price**. It
does not establish output quantity, success chance, tablet consumption or
where the tablet is obtained. The three material records exist in the
effective item database (`db/re/item_db_etc.yml:95670` onward).

Concrete counterpart targets:

- `db/re/item_db_usable.yml`, records `Mocadas_Slabs1` through
  `Mocadas_Slabs23` (IDs 102040-102062), invoke `laphine_synthesis()`.
- `db/re/laphine_synthesis.yml:4585` onward contains their recipes only as
  comments. Effective Renewal synthesis has no active `Mocadas_Slabs1`
  record. The commented Slabs1 recipe uses the same 100/100/100 materials,
  corroborating the primary counts but not providing an active route.
- `db/re/item_reform.yml`, `Mocadas_Refine_Box`, takes an existing Mocadas
  weapon and improves its refine. It is not a creation recipe.

Neither item 105211 nor a registered Garz tablet recipe/reward exists in
effective items/synthesis. The original compiled item-name table has no
105211 mapping, so a new `Mocadas_Slabs24` identifier must not be guessed.
The permitted loose reference server record calls it Slate Garz but has an empty
description; this is not missing server recipe evidence. No material drop
entry was located in the checked effective mob data. A supported material
list alone therefore does not make Garz obtainable.

## 2. Glacier: complete counterpart shop, Druid membership unproven

`npc/custom/episode19/quests_19.txt:17155` defines
`ep19_glacier_melee` with seventeen counterpart weapons at 150,000 Zeny each.
The existing `Maram#ep19trader` at `icas_in,180,61` starts at line 17158,
requires `ep19_main >= 11`, and reaches the shop through Weapons, then the
melee/ranged-weapon selection (`callshop` at line 17199). It is included by
the project's custom NPC configuration.

The official [iRO Episode 19 equipment guide](https://www.playragnarok.com/news/updatedetail.aspx?id=405&p=2),
dated 2025-09-16, independently documents Maram at those coordinates after
the Ice Castle audience quest and the same seventeen counterpart weapons
at 150,000 Zeny. Its list includes neither Nature knife nor Nature axe.

The narrow future code target is this existing shop's output list, preserving
its quest gate, old products and prices. Two entries at 150,000 would be an
explicit family-price project choice unless newer exact Druid evidence is
obtained; this audit does not authorize that choice. Existing Antiquity
random-output groups are a separate economy and should not be expanded as a
shortcut: adding products changes old probabilities.

## 3. Furious: one-coupon counterpart barter, explicit source omission

`npc/re/merchants/cashmall.txt:333` defines Tunkarom at `itemmall,41,55`.
Menu option 5 reaches `barter_mall_coupon_5` at line 370. The loaded barter
in `npc/re/merchants/barters/cashmall.yml:1479` has 38 outputs. Every output
requires item 1001809 `aegis_1001809`, with no additional Zeny price. Native
`src/map/npc.cpp:682`-697 defaults a new omitted required-item `Amount` to
one; purchase processing multiplies this count by the output quantity.
Thus the existing counterpart charge is exactly one coupon per weapon.

The [Gravity Furious weapon coupon page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1001809&itemSeq=7)
identifies Tunkarom and enumerates 38 exchangeable products, but includes
neither the new Druid axe nor scepter. Its generic family wording cannot
override that explicit omission. `GiganticAxe_Furious` is a different item,
not a match for `Axe_Furious`.

The narrow future target is two new indexes in this existing barter only
after membership is sourced or explicitly approved as project behavior.
Keep all 38 old entries unchanged. No coupon distribution route was proved
by the coupon's item record or description; a usable exchange must not be
confused with an established source of coupons.

## 4. Repeat: a two-stage chain with the base identity missing

The [Gravity Repeat Dagger-OS page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510184&itemSeq=1)
confirms the base weapon, and the [OSAD page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510185&itemSeq=1)
calls the supported weapon its upgraded version. Original client names map
510184 to `Repeat_Dagger_OS`. That base record is absent from effective
Renewal items.

Frasa in `npc/re/merchants/rgsr_in.txt:61` at `rgsr_in,115,169` offers OS
weapon reform and invokes `item_reform("OS_Weapon_Reform")` at line 113.
`db/re/item_reform.yml:5653` contains sixteen counterpart transformations,
each requiring a +7-or-higher base weapon without cards and:

| Existing counterpart material | Count |
| --- | ---: |
| 1000430 `Weapon_Stone_1` | 70 |
| 25669 `EP17_1_EVT02` - Mysterious Component | 200 |
| 25723 `EP17_1_EVT39` - Cor Core | 40 |

These records apply `RandomOptionGroup: Group_0` and omit `ChangeRefine`;
Frasa explicitly says the refine is retained. None accepts Repeat Dagger.
The corresponding base-weapon service is Elyumina in
`npc/re/merchants/enchan_illusion_17_1.txt:286` onward: it consumes one
25668 Damaged Weapon and fifty 25669 Mysterious Components, then calls
`getgroupitem(IG_EP17_1_SPC01)` at line 338. The random group at
`db/re/item_group_db.yml:42113` also excludes Repeat.

A future extension therefore needs both a fully supported base weapon and
its acquisition, not merely a new OSAD reform output. The inspected primary
item pages do not supply the Druid material counts, refine/card/option rules
or random-group probabilities. The exact counterpart recipe above is an
implementation target, not a claim that Gravity applies it to Repeat.

## 5. Flush axe: exact blueprint identity, nonuniform counterpart costs

The [Gravity amplification blueprint page, item 105210](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=105210&itemSeq=6)
names the Grinder Axe as the enhancement target and supplies weight 125,
but no required-material counts. The original client names establish
`Amp_Blueprint20` for 105210 and `Ein_AXE` for base weapon 520046. Both are
absent from effective Renewal items. Do not substitute `Amp_Blueprint19`,
which is an existing different blueprint, or infer a vendor from the
library's displayed 100,000 price.

The concrete family target is `db/re/item_reform.yml:7198` onward,
`Amp_Blueprint1` through `Amp_Blueprint19`. Costs demonstrably differ:

| Existing counterpart | Weapon_Stone_3 | Mjo_Energy | Mjo_Treasure | Shadowdecon | Ein_DYNITE |
| --- | ---: | ---: | ---: | ---: | ---: |
| `Amp_Blueprint1`: `Ein_1HDAGGER` to `F_Ein_1HDAGGER` | 75 | 75 | 30 | 75 | 75 |
| `Amp_Blueprint2`: `Ein_1HMAGGER` to `F_Ein_1HMAGGER` | 100 | 100 | 50 | 150 | 100 |

Both examples use `ChangeRefine: -20` and `RandomOptionGroup: Group_0`.
Neither cost tier or retention policy may be chosen by analogy for the
Druid axe. Existing Flush equipment boxes are also counterpart outputs,
not proof of a new Druid box or its supply. The permitted loose reference server
blueprint description is empty; it does not close these recipe gaps.

## 6. Dim Glacier: published drop method, not exact Druid drops

The official [iRO Dim Glacier update](https://renewal.playragnarok.com/news/updatedetail.aspx?id=411&p=1),
dated 2025-10-01, describes obtaining this weapon family as monster drops in
`jor_dun01`, `jor_dun02`, `jor_ab01` and `jor_ab02`. It lists counterpart
weapons, not the two Druid Nature variants, and publishes no numeric drop
rates for them. This is evidence for the regional family acquisition method,
not a supported new mob-drop table.

Existing `npc/custom/episode19/episode_19.txt` has Forr at `icas_in,188,60`
and the Incineration Furnace at `icas_in,192,57`. Forr enchants equipment or
exchanges materials; the Furnace calls the Dim Glacier extractor. The
`D_Gw_Extractor` recipe at `db/import/laphine_synthesis.yml:113` consumes
existing Dim Glacier weapons and rewards extraction materials. Likewise,
`EP19_DGW_Refine` at `db/re/item_reform.yml:6542` improves an existing
weapon's refine. None of these operations creates a weapon.

No native creation/drop route for the sampled existing Dim Glacier axe and
knife counterpart was located in the checked effective mob/item-group data.
A future change needs exact monster identities, Druid output membership,
rates and any shared-drop distribution policy. Adding them to an extractor
or refine list would be separate compatibility work, not acquisition.

## 7. Solid and Dimensions: do not invent a family merchant

The [Solid Whinger](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510189&itemSeq=1),
[Dimensions Nature dagger](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=510193&itemSeq=1)
and [Dimensions Nature axe](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620059&itemSeq=1)
primary pages support the gear definitions, not an acquisition recipe.

Searches for existing `Solid_Dagger` located lottery output in
`db/re/item_group_db.yml:124044`, group `AEGIS_105031` (ROS Gold Capsule),
including multiple refine/grade tiers. That is not proof of an ordinary
crafting vendor. Extending such a random pool alters the old output odds.
The `Dimen_MT_Axe` counterpart is referenced by equipment, set, enchant and
refine definitions; no corresponding NPC, barter, mob or item-group creator
was found. A same-item refine result is not initial acquisition.

There is therefore no defensible exact new service/price record to propose
for these three yet. Further primary recipe/service evidence is required;
a nearby episode NPC or a similarly named weapon is insufficient.

## Scope and follow-up verification

Read-only alternatives included exact identity searches, effective Renewal
imports, enabled counterpart NPC paths, original client item aliases,
permitted loose reference server/Rock metadata, and direct official item/update pages.
Metadata descriptions are not server NPC/synthesis code. No protected GRF
contents were extracted or decrypted. No third-party server's economy is
claimed as Gravity behavior.

Before any approved acquisition implementation, verify the complete chain:
input sources and dependencies, exact service output, material/currency
charging, insufficient-inventory/weight handling, cancellation, repeat use,
refine/cards/options/grade handling, and preservation of all old recipes and
random odds. Native item bonus/equip proofs and enchant target import checks
already delivered do not prove any of those acquisition transactions.
