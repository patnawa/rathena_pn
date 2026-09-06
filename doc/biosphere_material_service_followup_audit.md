# Biosphere material service follow-up audit

Date: 2026-09-06. Status: **unimplemented, source-only findings**. This receipt
records a bounded read-only review and proposed native regression. No new native
cases described here have run. No NPC, engine, database, import, economy, remote
server, or client was changed. This document is separate from
`biosphere_service_followup_audit.md` and the completed Abyss conversion work.

## Exact scope and unchanged policy

The current depth file has no `L_Fusion` label. The intended fusion callsite is
the complete `Ellie#bio_d1_fusion` NPC at
`npc/custom/varmundt_biosphere_depth.txt:235`. It is not the equipment Ellie at
`varmundt_biosphere_quests.txt:152`, any crown service, or Abyss `L_Convert`.

| Material callsite | Recipes | Current access | Last numeric input |
| --- | ---: | --- | ---: |
| Omega elemental branch, `varmundt_biosphere_quests.txt:72` | 14 | `F_BiosphereAccess`: BaseLevel >=240 only | 102 |
| Omega water branch, same file:112 | 3 | Same Omega policy | 135 |
| Ellie fusion, `varmundt_biosphere_depth.txt:235` | 24 | `F_BioDepthQuestAccess`: BaseLevel >=250 and `ep17_2_main >=33` | 263 |

Total: **41 existing material recipes**. All services are on `ba_in01`. Omega
has no Episode 17.2 story requirement, and neither material service has a
reputation threshold. Reusing the Abyss or crown access predicates would narrow
existing eligibility incorrectly. Preserve all existing menus, Leave/Cancel
choices, recipe order, output identities, and per-unit custom prices.

### All recipe identities

Omega's seven elements have two stages each: ten fragments plus 20,000z produce
one rune; five runes plus 20,000z produce one essence.

| Element | Fragment | Rune | Essence |
| --- | ---: | ---: | ---: |
| Glade | 1000636 | 1000640 | 1001138 |
| Fire | 1000637 | 1000641 | 1001139 |
| Ice | 1000638 | 1000642 | 1001140 |
| Death | 1000639 | 1000643 | 1001141 |
| Soul | 1001181 | 1001182 | 1001185 |
| Venom | 1001179 | 1001180 | 1001184 |
| Temple | 1001177 | 1001178 | 1001183 |

Each magical water consumes five each of common essences
`1001138,1001139,1001140,1001141`, then five of its special essence, and 20,000z.
The original material debit order must remain unchanged.

| Water choice | Special essence | Output |
| --- | ---: | ---: |
| Soul | 1001185 | 1001186 |
| Venom | 1001184 | 1001189 |
| Temple | 1001183 | 1001188 |

Ellie's eight elements have three stages each: ten energy plus 30,000z produce
one crystal; five crystals plus 50,000z produce one rune; ten runes plus 100,000z
produce one essence.

| Element | Energy | Crystal | Rune | Essence |
| --- | ---: | ---: | ---: | ---: |
| Fire | 1001290 | 1001298 | 1001306 | 1001314 |
| Earth | 1001291 | 1001299 | 1001307 | 1001315 |
| Ice | 1001292 | 1001300 | 1001308 | 1001316 |
| Storm | 1001293 | 1001301 | 1001309 | 1001317 |
| Soul | 1001294 | 1001302 | 1001310 | 1001318 |
| Purification | 1001295 | 1001303 | 1001311 | 1001319 |
| Corruption | 1001296 | 1001304 | 1001312 | 1001320 |
| Poison | 1001297 | 1001305 | 1001313 | 1001321 |

These tables preserve current project recipes, not an independently reasserted
official acquisition economy.

## Source-confirmed failure mechanisms

All three payment blocks calculate affordability before numeric `input`, then
use `checkweight`, material deletion, Zeny assignment, and output creation.
They omit a fresh complete preflight after the real VM suspension.

1. Native `input` stores a clamped quantity and pushes -1/0/+1 for below-range,
   valid, or above-range submissions (`src/map/script.cpp:6138`). Ignoring its
   status can turn an invalid request into a clamped purchase. Require status
   zero, not merely a positive stored amount.
2. Native `Zeny` assignment rejects a negative result
   (`src/map/pc.cpp`, `SP_ZENY`). If Zeny falls during input, the current order
   can consume materials before assignment fails. This is material loss, not
   evidence of a free output from stale Zeny.
3. A water recipe has five separate `delitem` calls. Native `delitem` preflights
   only its own item type. Shortage of a later essence after input can leave
   earlier ingredients consumed before native abort. Validate every ingredient
   before deleting any of them.
4. Native `delitem` assigns the request into `struct item.amount`, an `int16`
   (`src/common/mmo.hpp:316`, `src/map/script.cpp:8581`). A x10 request at 3,277
   outputs is 32,770, which cannot fit. On the current signed-16-bit narrowing
   path it becomes negative and the builtin returns success without deletion.
   The x5 counterpart is 6,554 outputs. Further wrapping can become positive:
   6,554 x10 is 65,540, narrowing to 4 and under-debiting. These are source-derived
   expectations for the new NPC negative controls, not newly executed proofs.
5. The displayed maximum is not capped to native output `MAX_AMOUNT=30000`.
   `checkweight` and `getitem` also narrow their requested quantity to `uint16`;
   an oversized request must not be left to these narrower representations.
6. `checkweight` calls `pc_checkadditem`, which chooses the first same-ID entry
   without matching bound/card/UID/rental metadata. Actual `pc_additem` chooses
   the first compatible stack. This mismatch can either refuse a valid output
   or allow payment before output insertion fails.

The existing [Abyss conversion audit](biosphere_conversion_audit.md) provides a
reviewed implementation shape and prior native evidence for shared engine paths.
Its five recipes and empty-service-map assumptions are not coverage of these
41 recipes. This pass did not mutate a live player or prove that trade while a
dialogue is open, ordinary acquisition of large quantities, or every proposed
inventory arrangement is reachable through the graphical client.

## All 56 material definitions and weight classes

The effective Renewal item import walk resolves all 56 distinct identities.
Every one is plain Etc. There are no material Script/EquipScript/UnEquipScript,
equipment locations, item-specific Stack overrides, generated GUID/autoequip
flags, or nonzero buy/sell values. The only present flags are BuyingStore and/or
DropEffect. Native new-item buy/sell defaults are zero; their output Get_Item
achievement argument is therefore zero.

All weights below are **native database units**: 1 means player-visible 0.1,
and 10 means player-visible 1. Inclusive ranges enumerate every ID.

| Native weight | Complete identity set | Count |
| ---: | --- | ---: |
| 1 | 1000636-1000639; 1001177, 1001179, 1001181; 1001290-1001305 | 23 |
| 10 | 1000640-1000643; 1001138-1001141; 1001178, 1001180, 1001182-1001186; 1001188-1001189; 1001306-1001321 | 33 |

Do not reuse a fixture that assigns Weight 10 to every material. Read the actual
output weight with `getiteminfo(...,ITEMINFO_WEIGHT)`, and preserve the existing
conservative check of current Weight plus the complete output weight before
deleting ingredients. No credit for freed input weight or inventory cells is
proposed. Output IDs differ from every input in their respective recipes.

With `MAX_ZENY=INT_MAX`, the 100,000z Ellie recipes have a maximum affordable
quantity of **21,474**, even when materials and carrying capacity permit more.
A successful 30,000-output fixture for these recipes would require an invalid
Zeny balance. The other prices permit a 30,000 ceiling: 600,000,000z at 20,000;
900,000,000z at 30,000; and 1,500,000,000z at 50,000. The proposed maximum remains
the minimum of current affordability, material limits, and 30,000.

## ba_in01 enabled callback closure

The enabled graph contains 900 scripts. Eight enabled source files declare
ba_in01 NPCs; the actual literal QuestInfo registrations on that map belong to
19 NPCs: 16 in `npc/re/quests/quests_17_2.txt`, three in
`npc/custom/episode19/quests_19.txt`. They contain **33 conditions**, not zero.
All inspected ba_in01 duplicates inherit callback-free `dummy_npc` or
`dummy_cloaked_npc` bodies. No dynamic map relocation/duplication was identified
by this bounded source review; it is not a runtime-world inventory.

The complete condition inventory below uses compact notation only for this
receipt: `Q(id)` means `isbegin_quest(id)`, `H(id)` means
`checkquest(id,HUNTING)`, and `P(id)` means `checkquest(id,PLAYTIME)`.
The future fixture must extract the actual original strings and registration
order, not execute these abbreviations. `17.2` and `19` identify the two source
files above.

| Source line | NPC suffix | Condition |
| --- | --- | --- |
| 17.2:375 | `#ep172_bain1-00` | Q(11618) == 1 |
| 17.2:1746 | `#jh5_1` Philofontes | Q(17381) == 1 |
| 17.2:1897 | `#jh5_1` Dien | Q(16440) == 1 |
| 17.2:2102 | `#jh5` Rookie | ep17_2_main == 17 and Q(16442), Q(16443), Q(16444), Q(16445), Q(16453) all == 0 |
| 17.2:2103 | `#jh5` Rookie | Q(16448) == 1 |
| 17.2:2214 | `#jh5_1` Ridsh | Q(16446) == 1 |
| 17.2:2279 | `#jh5_1` Kaya Toss | Q(16447) == 1 |
| 17.2:2423 | `#jh5_1` Tatio | Q(16452) == 1 |
| 17.2:3787 | `#ep172_eln_` | Q(18014) == 1 |
| 17.2:3886 | `#ep172_elmn02` | Q(18015) == 1 and countitem(7110) >= 10 and countitem(7326) >= 10 |
| 17.2:4003 | `#ep172_est01` | Q(18016) == 1 |
| 17.2:4004 | `#ep172_est01` | H(18017) == 0 or H(18017) == 1 |
| 17.2:4005 | `#ep172_est01` | H(18017) == 2 |
| 17.2:4343 | `#ep172_ely01` | Q(8681) == 0 and Q(18018) == 2 and BaseLevel >= 170 |
| 17.2:4846 | `#172ba01` | ep17_2_main >= 9 and ep17_2_bath == 0 |
| 17.2:8017 | `#ba_pw01_02Q` | H(5893), H(5894), H(5895) all == 2 |
| 17.2:8018 | `#ba_pw01_02Q` | H(5897), H(5898), H(5899) all == 2 |
| 17.2:8020 | `#ba_pw01_02Q` | Q(5892) == 2 and P(5896) == -1 and H(5893) == -1 |
| 17.2:8021 | `#ba_pw01_02Q` | Q(5892) == 2 and P(5900) == -1 and H(5897) == -1 |
| 17.2:8023 | `#ba_pw01_02Q` | Q(5892) == 2 and P(5896) == 2 |
| 17.2:8024 | `#ba_pw01_02Q` | Q(5892) == 2 and P(5900) == 2 |
| 17.2:8362 | `#libent_to_library` | Q(11622) == 1 |
| 17.2:9299 | `#ep172_amd01` | Q(18018) == 2 and Q(18019) == 0 |
| 17.2:9302 | `#ep172_amd01` | H(18022) == 0 or H(18022) == 1 |
| 17.2:9305 | `#ep172_amd01` | Q(18024) == 1 and countitem(1000226) >= 10 |
| 17.2:9306 | `#ep172_amd01` | P(18025) == 2 |
| 17.2:9504 | `#ep172_swty` | Q(18019) == 1 |
| 17.2:9505 | `#ep172_swty` | Q(18019) == 2 and P(18023) == -1 and H(18022) == -1 |
| 17.2:9506 | `#ep172_swty` | P(18023) == 2 |
| 17.2:9507 | `#ep172_swty` | H(18022) == 2 |
| 19:487 | `#ep19elly01` | Q(18119) == 1 |
| 19:593 | `#ep19crux01` | Q(18120) == 1 |
| 19:1115 | `#ep19gg01` | Q(18123) == 1 |

These are read-only expressions: the only called builtins are `isbegin_quest`,
`checkquest`, and `countitem`; the native `quest_check` implementation is const
and does not erase, complete, or update quests. Their item predicates reference
7110, 7326 and 1000226, none of the 56 conversion identities.

Nevertheless, `pc_delitem` and `pc_additem` really call `pc_show_questinfo`.
Conditions run through `achievement_check_condition`, which detaches the
transaction script, executes a nested VM, then reattaches the previous script.
The map's `qi_npc` must be populated and `sd->qi_display` sized correctly;
otherwise the native function returns early and a test can silently exercise
no conditions. Each NPC stops at its first true condition, so 33 registered
conditions does not imply 33 condition executions on every deletion.

The seven enabled Get_Item achievement conditions, IDs 220023-220029, compare
ARG0 with 100, 1000, 5000, 10000, 50000, 100000 and 150000 respectively. These
plain outputs have sell value zero, so their actual native Get_Item updates
cannot complete those achievements. The ARG register assignment/cleanup and
nested VM still execute. Reward-claim scripts are a different flow. Current
Weight50/Weight90 notification definitions and the wider reviewed callback
closure also remain dependencies, not reasons to replace the map callbacks.

## Smallest proposed change shape

Keep recipe selection/constants untouched and locally reuse the reviewed Abyss
transaction shape, without changing Abyss `L_Convert` or any crown-service body:

1. Cap the pre-input affordability maximum at 30,000; reject `input` status
   other than zero.
2. After that final yield, require `ba_in01` and revalidate the exact original
   per-NPC access policy. The map check bounds the callback proof; do not add
   reputation or cross-service story requirements.
3. Check current Zeny and every material's complete selected total before any
   deduction. Preserve existing `countitem`/`delitem` input acceptance semantics.
4. Check conservative current Weight plus actual complete output weight.
5. Use `getinventoryslots()` and `getinventorylist` to model the actual plain
   output: same ID, bound 0, expiry 0, UID string `"0"`, and all four cards zero.
   Respect the first compatible native index, terminal stack overflow, and
   inaccessible-index failure; otherwise require an empty accessible cell.
   Do not add identify/refine/attribute/grade/options/favorite matching tests,
   because native `pc_additem` ignores those for stacking.
6. With no further suspension, delete each input in chunks <=30,000, deduct
   the original total Zeny, and create the output once. Keep debit order and
   confirmation wording. No compensating grants/refunds are proposed.

A new ba_in01-specific callback/material gate is required. Do not rebaseline
the Abyss empty-ba_chess gate as if it proved this map. Source pins must cover
all 56 definitions, achievement imports, actual condition strings/registration
order, relevant quest definitions, status callback assumptions, and the enabled
NPC/source closure. Changed callbacks require review, not automatic acceptance.

## Proposed native matrix; not yet run

Use actual extracted Omega/Ellie bodies and access functions, current effective
item/quest definitions, and hash-pinned original full-source negative controls.
Freshly compile relevant production VM, inventory, item, achievement, quest and
allocator units with ASan/UBSan. Deny network access and identify packet/world,
logging and persistence doubles explicitly.

- Exercise all 41 recipes at quantity 1 and ordinary multi-output quantities;
  compare exact output, debit totals/order and unaffected inventory metadata.
- Cover x10 totals at output 3000/3001 and 3276/3277; x5 at 6000/6001 and
  6553/6554; positive-wrap under-debit controls and multiple payment chunks.
- Cover 30000 where affordable, 30001 refusal, and the 21474/21475 affordability
  boundary for 100,000z recipes. Do not fake Zeny above MAX_ZENY.
- Submit zero, negative and above-offered input; test each menu Cancel/Leave
  and actual forced close while numeric input waits. Assert no deductions.
- Mutate Zeny and each ingredient at the real input suspension, including every
  later water essence. Change each access predicate and map separately. Positive
  Omega cases must allow level 240 with incomplete story; positive Ellie cases
  must not require reputation.
- Test exact/insufficient weight for both native weight classes without giving
  credit for consumed input weight. Preserve bound input acceptance and every
  unconsumed field; do not change the native deletion preference policy.
- Test restricted actual inventory capacity; bound/card/UID/rental-separated
  outputs; bound-first/plain-later ordering; terminal compatible-stack overflow
  or inaccessible index; and metadata native stacking deliberately ignores.
  Include paid-output-failure original controls with no slot freed by the debit.
- Register the exact 33 original QuestInfo expressions through the real builtin,
  initialize the 19-NPC map collection and player display state, and execute real
  `pc_show_questinfo` after material deletion and output. Obtain true/false
  condition coverage across valid fixtures, respecting first-true short-circuit
  and differences between Omega/Ellie access eligibility.
- Assert nested VM preservation of the main script/RID, local recipe variables,
  resources and quest state. Execute the seven actual achievement conditions;
  do not substitute an empty map or a no-op achievement callback.
- Require clean allocator teardown and reject sanitizer, allocator and unexpected
  native diagnostics even at exit zero. Add changed-callback, changed-weight,
  missing-QI-registration and wrong-display-size negative controls to prevent
  apparent success through skipped native paths.

## Source provenance

Raw SHA-256 values at review; these runtime files were not edited:

| File | SHA-256 |
| --- | --- |
| `npc/custom/varmundt_biosphere_quests.txt` | `787f6ff54a4c43a60510e4a606dc5bd33a80c502853f113ad6756e257ac5aa5a` |
| `npc/custom/varmundt_biosphere_depth.txt` | `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28` |
| `npc/custom/varmundt_biosphere.txt` | `c8d86d6cb99f0421fc7f4420a780532d153bd1197e97e54a5ca9a7053c564458` |
| `npc/re/quests/quests_17_2.txt` | `12c2595c2cfee3310b3d067e52cc035e8f8976785d42bc3bd1191f7fadf2df2c` |
| `npc/custom/episode19/quests_19.txt` | `3a2d4b79e85e93d295c66089651d7f7f3aa5b7e55f8c58c54bac9ca8f5efc11a` |

This receipt makes no claim of an implemented transaction, passing new native
suite, live-world callback inventory, gameplay validation, deployment or commit.
