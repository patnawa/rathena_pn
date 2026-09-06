# Final Battle crystal reward capacity audit — 2026-09-06

## Status and scope

**Source finding confirmed for a native-valid separated-stack inventory; no
runtime repair or new regression test has been implemented by this audit.** This
document concerns the two Giant Serpent Crystal claims in
`npc/custom/episode21/FinalBattle.txt`. It is separate from the `pc_delitem`
gear-switch cleanup deployment. No runtime, database, engine, or existing test
file was edited. No live-server, SQL, SSH, or Git action was taken.

The initial read-only pass examined Final Battle, Secret Altar, Silent Sanctuary,
and the Black-Haired Beast bridge. It excluded the separately repaired Gimli
reconnect/Ghost reporting flows and the other agent's Omega/Ellie exchanges.
This is not a whole-gameplay or live-playthrough audit.

The current Renewal include graph has 900 enabled scripts and 14 include files.
`npc/re/scripts_main.conf:49` imports `npc/scripts_custom.conf`; its line 229
enables `FinalBattle.txt`. Instance definitions 149/153/152 in
`db/import/instance_db.yml` use `1@ep21b` for Easy/story, Normal, and Hard.

## Exact claim sites and failure

| Crystal branch | Capacity check | Consumes daily claim | Grant loop body | Clears saved roll |
| --- | --- | --- | --- | --- |
| Normal, NPC declaration line 238 | line 284, `checkweight2(.@reward_item,.@reward_amount)` when count is nonzero | line 289 | line 291 | lines 292–297 |
| Hard, NPC declaration line 302 | line 349, `checkweight2(.@reward_item,.@reward_amount)` | line 354 | line 356 | lines 357–362 |

Lines 241 and 305 reject a later attempt when `EP21_FB_Crystal_Daily` already
equals the current daily key. The persisted roll is deliberately generated before
capacity checking so a rejected player cannot reroll merely by retrying.

A concrete Normal-mode counterexample is:

1. The instance is at stage 20. The character has not claimed today. Its saved
   current-day/mode-1 roll has `ItemMask = 1`, `ExtraMask = 0`, and `BoxCount = 0`:
   exactly one Turquoise Magic Stone, item 1001653.
2. All allowed inventory slots are occupied. The first same-ID row contains one
   account-bound 1001653, with otherwise ordinary metadata. There is no compatible
   unbound 1001653 row. Weight capacity is sufficient.
3. `checkweight2` calls ID-only `pc_checkadditem`, which returns
   `CHKADDITEM_EXIST` for that bound row. No fresh slot is charged, so the check
   succeeds even with zero empty allowed slots.
4. The NPC records today's claim. `getitem` constructs an unbound, non-rental,
   zero-card, zero-UID item. Actual `pc_additem` cannot merge it with the bound
   row and cannot find an allowed empty slot. It returns `ADDITEM_OVERITEM`.
5. `getitem` reports failure, but the VM continues. The NPC clears the saved
   roll and displays its success text. Retrying is rejected by the daily key.
   This path has no compensating mail, floor drop, or preserved pending claim.

Hard has the same mismatch. If a later batch member fails, preceding members can
already have been delivered, so the defect also permits a partially delivered
batch whose remaining reward cannot be retried.

**Acquisition boundary:** account-bound item records are supported native item
states, not malformed inventory. However, this pass did not establish an ordinary
player-only producer of such metadata for these crystal outputs. A read-only
traversal of current Renewal item-group imports found no output entries carrying
`Bound`, `Duration`, `Named`, or `UniqueId` overrides for the 21 identities below.
The crystal itself produces plain items. Do not describe this as an ordinary
player exploit or a demonstrated live loss. No failure of this particular
preflight was established for an unchanged inventory containing only plain
compatible stacks.

## Native source evidence

- `src/map/script.cpp:7560`, `buildin_checkweight2`, obtains empty-slot count and
  sums weight, then calls `pc_checkadditem(sd,nameid,amount)` at line 7633.
- `src/map/pc.cpp:5729`, `pc_checkadditem`, searches by `nameid` alone at line
  5752. Its first same-ID row decides existing-stack capacity. It does not receive
  bound, rental, UID, or card metadata.
- `src/map/script.cpp:7671`, `buildin_getitem`, zero-initializes the item, sets
  `identify = 1` and `bound = BOUND_NONE`, and calls `pc_additem` at line 7738.
  Lines 7740–7743 report the failed add and return `SCRIPT_CMD_FAILURE`; they do
  not set the script state to `END`.
- `src/map/pc.cpp:5991`, `pc_additem`, matches `nameid`, `bound`, non-rental
  `expire_time`, `unique_id`, and all four card fields at lines 6021–6025.
  The **first compatible** row is terminal: an amount cap or inaccessible index
  fails without trying a later row or a fresh slot. If no row matches, the first
  empty index must be below `sd->status.inventory_slots`.
- `src/map/script.cpp:4135`, `run_func`, only logs a builtin's
  `SCRIPT_CMD_FAILURE`; it does not automatically stop or roll back the VM.
  Moving the daily assignment after the grant loop therefore does not by itself
  solve this defect.

These are current-source traces, not a freshly executed native reproduction.
The earlier party-progression test explicitly doubles capacity and grants; it
does not prove native stack identity or batch delivery.

## The 21 output definitions

The ordered Renewal `db/item_db.yml` import graph was read, not just a single
matching source row. The following current records have no overriding definition
in that graph. Weights are native units, not displayed weight. `B` means
`BuyingStore: true`; `D` means `DropEffect: CLIENT`.

| ID | Aegis name | Native type | Weight | Other nondefault grant-relevant data |
| --- | --- | --- | ---: | --- |
| 103512 | Yor_Card_P_Box | Usable | 0 | Container; restrictions/use script described below |
| 1001653 | Ep21_Armor_E_Stone1 | Etc | 10 | B, D |
| 1001654 | Ep21_Armor_E_Stone2 | Etc | 10 | B, D |
| 1001655 | Ep21_Armor_E_Stone3 | Etc | 10 | B, D |
| 1001656 | Ep21_Robe_E_Stone1 | Etc | 10 | B, D |
| 1001657 | Ep21_Robe_E_Stone2 | Etc | 10 | B, D |
| 1001658 | Ep21_Robe_E_Stone3 | Etc | 10 | B, D |
| 1001659 | Ep21_Shoes_E_Stone1 | Etc | 10 | B, D |
| 1001660 | Ep21_Shoes_E_Stone2 | Etc | 10 | B, D |
| 1001661 | Ep21_Shoes_E_Stone3 | Etc | 10 | B, D |
| 1001662 | Ep21_Acc_E_Stone1 | Etc | 10 | B, D |
| 1001663 | Ep21_Acc_E_Stone2 | Etc | 10 | B, D |
| 1001664 | Ep21_Acc_E_Stone3 | Etc | 10 | B, D |
| 1001480 | Brown_Dia | Etc | 300 | Buy 600000; native Sell 300000; D |
| 1001034 | EP19_S_F_1_Extract | Etc | 30 | B, D |
| 1001035 | EP19_S_F_2_Extract | Etc | 30 | B, D |
| 1001036 | EP19_S_F_3_Extract | Etc | 30 | B, D |
| 1001037 | EP19_Gla_Extract | Etc | 30 | B, D |
| 1000812 | Snow_F_Stone1 | Etc, native default | 1 | B |
| 1000813 | Snow_F_Stone2 | Etc, native default | 1 | B |
| 1000814 | Snow_F_Stone3 | Etc | 1 | B |

All 21 are stackable, have no item-specific inventory stack cap, GUID generation,
autoequip, equipment location, `EquipScript`, or `UnEquipScript`. Buy and Sell are
zero except Golden Diamond. `ItemDatabase::parseBodyNode` supplies Etc when Type
is absent and derives Sell as half Buy when only Buy is supplied.

103512 has `Container: true`; its trade restrictions are `NoDrop`, `NoTrade`,
`NoCart`, `NoGuildStorage`, `NoMail`, and `NoAuction`. Its use script is
`getgroupitem(IG_YOR_CARD_P_BOX);`. **Receiving** this Usable item does not execute
that script. None of the other 20 items has a Script or trade restriction here.
The 20 material rows are in `db/re/item_db_etc.yml`; the box is in
`db/re/item_db_usable.yml`.

## Smallest maintainable helper plan

No reusable exact batch preflight was found. `EP20_CheckReward` still delegates to
the same generic capacity commands. The existing Biosphere `L_Convert` block in
`npc/custom/varmundt_biosphere_depth.txt` supplies the reviewed single-output
matching pattern. Reuse its approach, not its zero-sell-value callback assumption.

Add one purpose-scoped function in `FinalBattle.txt`, for example
`EP21_FB_CheckPlainBatch`, and call it from both crystal branches in place of the
two `checkweight2` checks. Pass the reward arrays and count; native array-reference
arguments and `copyarray .@local[0],getarg(...),count` are already supported and
used elsewhere. Keep caller reward arrays unchanged.

The helper should:

1. Accept the legitimate empty Normal batch without requiring room for an item
   that is not being awarded. Otherwise validate positive amounts and the
   reviewed output identities. Aggregate repeated IDs if supported; current
   production arrays already contain distinct IDs.
2. Sum `getiteminfo(id,ITEMINFO_WEIGHT) * amount` against the character's current
   `Weight` and `MaxWeight`. Preserve the existing whole-batch weight policy.
3. Read `getinventoryslots()` and call `getinventorylist` once. Separately count
   **all** occupied indices below the allowed slot limit; do not count only the
   prefix visited while finding one matching item.
4. For each output, find the first compatible plain row in ascending actual
   `@inventorylist_idx` order. Match ID, bound 0, expiry 0, UID string `"0"`, and
   four zero cards. Require its index below the slot limit and its existing plus
   aggregate reward amount at most native `MAX_AMOUNT` (currently 30000).
5. If no compatible row exists, reserve one fresh allowed slot cumulatively for
   that distinct output. Do not permit two new output IDs to claim the same
   remaining slot. Do not search for a later stack after the first compatible
   stack fails.
6. Ignore refine, identify, attribute, grade, random options, favorite, and equip
   metadata when matching these stackable outputs: `pc_additem` ignores those
   fields. Rejecting them would unnecessarily exclude native-supported stacks.

The native getter in `src/map/script.cpp:15762` already exposes the actual allowed
slot count; no further engine API is necessary for these pinned definitions.
Future GUID, stack-cap, type, or autoequip changes require re-review because not
all those properties are exposed by the script getter interface.

Preserve saved-roll construction and retry behavior, both masks, the three
independent Golden Diamond rolls, the guaranteed Hard box, reward ordering, and
the existing daily-key/reset policy. In the current source, the Normal material
thresholds repeat 26473/14568/9000, Hard repeats 40000/23000/14568, each against
`rand(100000)`; extra/diamond thresholds are 19094 and 29619 respectively. These
are existing project behavior, not a new claim of official probability research.
No claim-marker rearrangement, new reward entitlement, price, or acquisition
economy is part of this proposal.

This helper plan covers the **two crystal batches only**. Reusing it for Final
Battle story/daily completion rewards would additionally require definition and
callback coverage for 1001618, 102948, and 103537.

## Required synchronous callback conditions

An exact capacity snapshot is sufficient only while all grants run without an
intervening inventory/weight-capacity mutation or suspension. The existing claim
paths have no `next`, `select`, `input`, `close2`, or sleep between the preflight
and commit/reset. Keep that property, and bind the proof to the actual instance
map rather than an arbitrary invocation of an extracted NPC body.

Actual successful grants call:

`getitem -> pc_additem -> weight notification -> AG_GET_ITEM -> pc_show_questinfo`.

- Weight notification can start/end Weight50/Weight90. Current effective status
  entries have no Script or CalcFlags; their mutual EndOnStart and Weight90's
  StopAttacking flag do not allocate inventory or recalculate item bonuses.
  This must be retained as reviewed source/data evidence, not silently omitted
  by a no-op packet double.
- **Golden Diamond is not a zero-sell-value item.** Its Sell 300000 can complete
  all seven current Get_Item thresholds, 100 through 150000.
  `achievement_update_achievement` recalculates achievement level and may invoke
  the twenty Goal_Achieve conditions. Current conditions are respectively pure
  `ARG0 >= threshold` and `AchievementLevel >= 1..20` expressions.
  `achievement_check_groups` may complete dependent groups, but explicitly skips
  records with a Condition; it does not introduce another arbitrary condition
  class. Achievement completion changes achievement data and `ARG0`, not reward
  inventory or capacity, under this reviewed data.
- `achievement_get_reward` runs reward Scripts only on the later reward-claim
  path. Do not confuse those Scripts with automatically executed completion
  callbacks. `achievement_check_condition` detaches and restores the caller's
  script attachment; exercise this native path when testing the grant loop.
- The enabled graph contains exactly 14 NPC declarations on `1@ep21b`, all in
  FinalBattle. None of those bodies registers quest-info conditions. Its one
  `questinfo` statement belongs to the outside-map daily entrance NPC.
  `pc_show_questinfo` returns immediately for an empty map `qi_npc` vector.
  The reviewed enabled graph had no runtime NPC duplicate/map-setter producer;
  its 33 `unitwarp` statements concern other fixed NPC/map families. A future
  condition registration or indirect NPC relocation needs fresh review.
- No output is autoequipped or a pet egg. The container-use Script is not an
  acquisition callback. No regular equipment, card, combo, or persisted bonus
  script evaluation is introduced by this grant path under the current weight
  status and item definitions.

Pin the ordered achievement DB/import graph, the 21 effective item definitions,
the weight status definitions, and the enabled map/source graph. Hashes preserve
a manual semantic review; a keyword search alone is not proof of arbitrary
script purity. The existing Biosphere evidence machinery can supply graph
traversal, but its conversion-specific identity and zero-sell-value assertions
must not be reused unchanged.

## Intended regression coverage

No tests below are claimed as executed by this audit. A future focused suite
should execute actual NPC/helper bodies with native VM, `pc_additem`, capacity
getters, and actual achievement conditions, while clearly identifying transport,
registry persistence, and map/instance test doubles.

- Reproduce the old bound-only/full-inventory failure in both modes and show
  repaired rejection leaves daily marker, roll key/mode/masks/count, and inventory
  unchanged. Include a failure late enough to expose partial batch delivery in
  the old source.
- Accept bound-first/plain-later inventories when the actual compatible row has
  space. Accept a fresh slot when no compatible row exists. Cover card, rental,
  and UID separation independently.
- Reject a full **first compatible** stack even with a later compatible stack or
  an empty slot; reject its index beyond a shrunken allowed inventory size.
- Check exact-fit and one-slot-short batches with multiple new IDs, sparse
  inventory indices, existing compatible stacks, and valid rows beyond the
  allowed limit. Check weight exact fit and one unit over.
- Cover 30000 stack boundaries, native-ignored metadata, the Normal zero-result
  roll, every individual output, the maximal batch, and Golden Diamond counts
  1–3. If the helper aggregates duplicate IDs, test that behavior explicitly.
- Preserve saved rolls across capacity rejection/retry and close/reopen; ensure
  success consumes the daily claim once. Demonstrate a second party member's
  claim remains independent, without changing the existing party policy.
- Execute Golden Diamond's successful Get_Item completion and resulting
  Goal_Achieve checks, not just a zero-value callback rejection. Confirm no
  inventory mutation or automatic container/achievement reward execution.
- Make dependency negatives reject a new imported item/achievement override,
  a mutating condition, an output GUID/stack-cap/autoequip change, and a new
  instance quest-info producer. Keep probabilities and reward declarations
  source-pinned independently from the helper implementation.

## Why not change generic checkweight semantics

`checkweight` and `checkweight2` receive IDs and amounts, not the binding/rental/
card/UID contract of the later grant. Silently making them check only plain
outputs would not make them generic exact-grant checks. It can produce the wrong
answer for existing `getitembound` call sites: a full inventory with only a plain
stack can appear acceptable for an account-bound grant, while an existing bound
stack that really has room can be rejected by a plain-only check.

For example, `Beta#biosphere_reform` in
`npc/custom/varmundt_biosphere_quests.txt:247` checks capacity before line 251's
`getitembound .@item,1,BOUND_ACCOUNT`. Its additional `countitem` guard is not a
metadata contract for the generic builtin. Leave the generic commands untouched
and make the proposed helper's plain-output scope explicit.

## Source identities and remaining limitations

Raw SHA-256 identities rechecked before writing this document:

| File | SHA-256 |
| --- | --- |
| npc/custom/episode21/FinalBattle.txt | `7f8dc969e03879ec90bc11f01e553a5556beb2d931e155061523ee91cd6026cd` |
| src/map/pc.cpp | `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403` |
| src/map/script.cpp | `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1` |
| src/map/achievement.cpp | `1269a19e22b13d0bcf31b433ff0b9532c2e2e12078ff192d3fa8e14a358b2a43` |
| db/re/item_db_etc.yml | `385e14a6ce2a943ee847cc6ae876390cc4aeac484e77491eaf83962f44cac137` |
| db/re/item_db_usable.yml | `62f9bc98bd2b558b0bb4a2d41e74d00c51a74766d2c9c3c52b07920e5030277e` |

The pc.cpp checkpoint includes the separately reviewed full-depletion gear-switch
cleanup and initial `n >= MAX_INVENTORY` rejection. Those changes are unrelated
to Final Battle's capacity mismatch. This documentation addition implements no
reward-capacity repair or further change to pc.cpp.

The proposed proof is current-content, valid-inventory, synchronous execution
safety. It is not generic transaction rollback, process-crash atomicity across
inventory/registry persistence, live binary equivalence, packet/client UI proof,
arbitrary future script safety, or compensation for historical losses. Live
player holdings and a normal acquisition route for separated crystal-material
stacks remain unverified. Retain these boundaries in any later implementation
or deployment receipt.
