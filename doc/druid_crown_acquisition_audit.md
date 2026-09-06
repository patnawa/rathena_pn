# Alitea Time Dimensions crown acquisition audit

Audit and scoped NPC implementation, 2026-09-06. Scope: the existing custom `Abyss Researcher#bio_d2` and `F_BioD2ModifyCrown` in `npc/custom/varmundt_biosphere_depth.txt`. The original source is pinned at Git `5e139a555`, SHA256 `c0e59e6497f0bebef17ee98784ac92a988d54d16cacf41be188fb774f0456d09`. Historical line numbers and findings below refer to that original source. This package changes only the approved helper/Abyss Researcher paths, plus its independent tests and this report; no engine, item definition, client, import, Git or deployed state is changed by this subtask.

## Result

`400999 Time_DM_R_Crown_AT` is a complete, active one-slot, zero-weight, level-250 Alitea crown, and the integrated native enchant target groups 132 and 164 include it. The original NPC **could not craft, random-upgrade or stat-reroll it**. The omission was in the NPC's identity lists, not in an Alitea job-level requirement or the synthetic-slot engine. The scoped candidate adds it through one explicit 19-ID array, preserves all 18 original outputs, and keeps Cancel at its original choice 19 by appending Alitea as choice 20.

There are two additional bounded hardening findings: upgrade/reroll confirmations do not retain the selected equipment identity, and access/reputation is not rechecked after dialogue suspension. Neither should be described as a demonstrated ordinary-player exploit: the local Renewal configuration defaults `item_enabled_npc=0`, this NPC never enables items, and normal equip/unequip packet handlers reject equipment changes while such a dialogue is open. GM changes, other server-side scripts, configuration changes or other external state transitions remain relevant.

## Existing access, job and economy semantics

The shared `F_BioDepthQuestAccess` (lines 9–20) requires Base Level at least 250 and `ep17_2_main >= 33`. The Abyss Researcher at `ba_chess,22,16` additionally requires Depth 1 reputation at least 2,000 (lines 442–450). Depth 1 is reputation ID 6 (`RepPoints6`), Depth 2 is ID 9 (`RepPoints9`), and both current reputation definitions cap at 5,000.

The craft menu's class labels are **choices of output item**, not checks of the requesting character's job. Any character satisfying the access and reputation gates may choose any offered crown. Actual wearing is governed separately by the item database and `pc_isequip`. Preserve that distinction: adding an Alitea option should not introduce an Alitea-only craft gate or change existing class masks. The old 18 crowns use their existing job/class rules, including expanded-job `Third` masks where already present; the new Alitea record uses its already-verified Alitea/trait filter.

| Existing action | Depth 2 reputation threshold | Consumed resources / result |
|---|---:|---|
| Craft one crown | 500 | 50 `1001552 Abyss_Magic_Jewel`, 50 `1001553 Time_Dimension_Jewel`, 75 `25865 Jewel_Of_Time_Ore`; one plain, identified, unbound crown |
| Open Dimension weapon interface | 750 | Opens group 133; native recipe costs apply later |
| Open crown interface | 1,500 | Opens group 132; native recipe costs apply later |
| Random-upgrade special jewel | 1,500 | Time Dimension Magic Runes (`1001556`) at the existing level-based cost below |
| Reroll slot-4 stat | 1,500 | 180 Abyss Magic Runes (`1001555`) |

Reputation is a threshold, **not a payment**: these paths do not subtract reputation. Direct crown crafting has no Zeny price. Every listed material has server Weight 10 (one client weight unit); crafting consumes 175 client-weight units and creates a zero-weight crown. The other 18 crowns likewise use the zero-weight default. The custom NPC economy is existing project behavior, not a newly verified official Gravity acquisition recipe. Gravity's item-property page establishes the item, not this custom menu's costs.

Random jewel upgrades support exactly ten contiguous level-1..10 families, all verified as existing `Card/Enchant` records: bases 312719, 312729, 312739, 312749, 312759, 312769, 312779, 312789, 314249 and 314259 (Mettle, Tenacity, Master Archer, Acute, Magic Essence, Spell, Adamantine, Affection, Fierce Attack and Great Conjurer). The synthetic jewel is zero-based card index 1, shown to users as slot 2.

| Starting jewel level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| Success percent | 80 | 65 | 50 | 35 | 25 | 20 | 10 | 7 | 5 |
| Time Dimension Magic Rune cost | 5 | 10 | 20 | 35 | 55 | 80 | 110 | 145 | 185 |

Success adds one level; failure subtracts one level except level 1 stays at 1. Level 10 is rejected before charging. Stat reroll changes only zero-based card index 3 (displayed slot 4), selecting STR/INT/VIT/LUK levels 1..5. For each stat family the exact weights are 11,500/8,000/3,500/1,500/500 out of 100,000; all 20 weights total 100,000. Read-only comparison confirms these match effective group 132's slot-3 normal outcomes at grades None/D/C/B/A. This does not replace the unrelated customized crown distribution tables.

## Finding 1 — missing explicit Alitea identity

- Craft selection at lines 504–507 has 18 classes plus Cancel and computes `400529 + selection`. It can produce only 400529–400546.
- The helper rejects any equipped ID outside 400529–400546 at lines 422–424.
- Random upgrade's inventory scan repeats that range at line 534.
- Stat reroll's scan repeats it at line 611.

The Alitea crown therefore cannot reach the helper, and changing only the menu or only the helper is insufficient. Its intrinsic Slots=1 already permits both synthetic indices 1 and 3; no engine, skill, class-mask or item-definition change is needed for that reason.

Implemented compatible fix: one explicit allowlist of the existing 18 IDs plus **400999**, preserving all original menu mappings/order and appending Alitea **after** Cancel. Output resolves through that array, and the guarded helper uses its same membership predicate for capture, validation and mutation. The range is not widened. Appending one name while retaining `400529 + selection` would produce 400547, the unrelated `2023Seppay_02`; the larger interval would admit hundreds of unrelated identities. Adding Alitea at the same existing project cost is a deliberate extension of the custom economy, not an assertion of a newly sourced official recipe.

## Finding 2 — confirmation snapshot does not protect the selected crown

At lines 544–546 or 621–623, the caller snapshots only three synthetic-card values before `next` and `select`. It does not retain the original item ID, actual inventory index, unique-ID string, physical card, or other identity metadata. After confirmation it charges materials and calls the helper.

The helper then reads the **current** `getequipid` and cards at lines 422–425. It computes which current field differs from the requested final triplet and passes those newly read values to `modifyequipitem` as expected ID/card arguments (line 438). This compares against the state at helper invocation, not the state the player confirmed. The zero-change branch returns success at lines 434–435 without invoking the native validation at all.

A source-level counterexample: start the dialogue on a valid crown whose special jewel is level 2 and whose other two synthetic slots are X/Y. During suspension, an external operation replaces it with another allowed crown carrying level 8 and the same X/Y. A successful old level-2 roll requests level 3 and charges 10 runes. The helper sees exactly one difference, passes the replacement crown's freshly read ID and level-8 card as its expected values, and can change the replacement crown to level 3. The original snapshot is never checked. Same-ID replacements and rerolls are also affected. Conversely, a change to another synthetic slot can make the helper see multiple differences, refuse and refund. Its existing refund comment that this can occur only when the equipment was removed is therefore inaccurate.

The native `modifyequipitem` implementation (script.cpp around lines 9968–10140) is not inherently missing its documented within-call guard: it validates the supplied ID/card, synthetic-slot boundary, equipment type/amount/identify state, special-card metadata and weight; snapshots the entire record before force-unequip; then checks it again after unequip scripts. It preserves UID, grade, refine, binding, expiry, options and unrelated cards. The caller is supplying too-late expected values.

Smallest NPC fix: before suspension, retain the real `@inventorylist_idx`, `@inventorylist_uniqueid$`, item ID, amount, identify/broken state, all four cards and the other displayed/identity metadata needed to cancel changed requests. Store them in local variables/arrays rather than relying on shared `@inventorylist_*` across yields. Immediately after the last confirmation, re-read the equipped record and refuse any mismatch **before payment**, including the legitimate no-change roll path. Pass the original item ID and original selected-card value to the existing builtin and enforce that the intended slot, not an arbitrary differing slot, is the one being changed. Keep no further dialogue suspension between final validation, mutation and payment.

Payment ordering needs a deliberate native test rather than a casual rewrite: current code pays then refunds on helper failure; moving payment after success is attractive but `modifyequipitem` may execute unequip scripts, including physical-card unequip scripts, before returning. All 19 current crown base definitions have no `UnEquipScript`, but the physical card can supply one. The final implementation must prove resource preservation on that boundary too. A failed re-equip after a successful mutation intentionally returns success with the crown safely unequipped; it is not a refund condition.

## Finding 3 — stale access/reputation across dialogue

The NPC reads Depth 2 reputation once at line 450, before the top-level `next/select`. The craft path then suspends twice more; upgrade and reroll suspend at their confirmation. Neither the shared level/story gate, Depth 1 minimum, nor current Depth 2 threshold is rechecked at the final operation/window opening. A decline through an external GM/script action can leave stale authorization. No claim is made that ordinary gameplay normally reduces these values during dialogue.

Smallest hardening: a shared read-only access/threshold predicate called both at menu entry and immediately before crafting, opening native group 132/133, upgrading or rerolling. Preserve thresholds 500/750/1500 and do not consume reputation. Avoid duplicating a subtly different rule in each label.

## Capacity and charging assessment

- Crafting already rechecks all three material counts after its final `select` (lines 513–517), then calls `checkweight(output,1)` before deductions. There is no yield between that check, three deletions and `getitem`. Current native `checkweight` checks both weight and free inventory slots, so zero-weight gear still requires one free slot. It is conservative: a full inventory is refused even if consuming a complete material stack would free a slot. Preserve that existing safe behavior unless a separately tested net-capacity transaction is intended.
- Random upgrade and reroll recheck their current material counts after confirmation. In-place enchant changes require no new equipment slot or weight; a cost refund normally fits because the exact same stack quantity was just removed. However, `getitem` is not guaranteed-success: native add failure raises a script error. If an unequip-side effect consumes freed capacity before failure/refund, the script has no explicit refund-capacity proof. Test that failure path before claiming transactions are lossless.
- Existing refund branches do not reconstruct the crown, avoiding UID/grade/card loss. Retain this in-place approach. Do not revert to `delitem` + `getitem2/4` crown reconstruction.
- Adjacent but outside the requested crown labels: `L_Convert` computes resource/Zeny-derived `.@max`, suspends at quantity `input` (line 713), then checks output capacity without rechecking all current inputs and Zeny before its sequential deletions. External reductions can therefore cause partial payment followed by `delitem` failure or stale Zeny arithmetic. This is a separate material-conversion hardening candidate, not a reason to broaden the crown patch silently.

## Required native regression before any implementation is called complete

Use the actual parsed NPC/function bodies, real Script VM, real item/skill/reputation metadata, and real mutation/charging functions; mock only player/NPC lookup, network packets, persistence and controlled dialogue answers. An expression-only or regex-presence check is insufficient. Deny networking and require clean ASan/UBSan, zero script warnings/errors and explicit allocator teardown.

1. All 19 exact menu outputs, all original ordering, Alitea 400999, Cancel, and refusal of 400547/unrelated headgear. Exercise access boundary values Base Level 249/250, story 32/33, Depth 1 1999/2000, and Depth 2 499/500, 749/750, 1499/1500. Verify another eligible job can still choose a different job's crown, since existing crafting is not job-restricted.
2. Craft exact costs/output/UID defaults, no reputation or Zeny deduction, insufficient each material, zero empty slots versus one slot, and cancellation/no-op resource snapshots at every yield. Inject access/reputation/material/capacity changes during each suspension; all refused paths must preserve inventory, counts, weight and Zeny.
3. Every one of the ten jewel families at levels 1..10, exact costs, success/failure boundaries for `rand(100)`, level-1 failure retention and level-10 no-charge refusal. Preserve all unrelated crown metadata and verify only card index 1 changes.
4. Exhaust all 100,000 stat-roll buckets against the exact 20 outcomes, including roll endpoints and the legitimate same-result case. Charge exactly 180 runes once, with no grade-dependent distortion, and modify only index 3.
5. At every upgrade/reroll pause: remove the crown; replace it with another allowed ID; same-ID/different-UID replacement; same UID at another inventory index; alter any card (including physical index 0), refine, grade, binding, expiry or random option; and change the old selected enchant to the prospective rolled result. Every stale request must refuse before any charge and not touch the new equipment. Include the currently failing level-2→replacement-level-8 counterexample.
6. Native builtin failure, forced-unequip side effects, refund capacity failure, failed re-equip after a successful mutation, and immediate re-entry/repeated confirmation. Prove the final chosen payment ordering causes neither free successful enchants nor consumed resources after a refused mutation.

## Candidate transaction design and verification boundary

The helper now has explicit capture, validate and mutate modes. Capture writes 30 numeric fields into a caller-local array and the unsigned 64-bit unique ID into a caller-local string **before** `next/select`. Validation re-reads the equipped item and compares every stored field without overwriting the original expectation. Mutation supplies the original item ID and original selected-card value to native `modifyequipitem`; same-result rolls also reach native validation. UID zero refuses the paid service rather than pretending to distinguish two otherwise identical legacy records.

The 30 fields are actual inventory index, item ID, amount, equip mask, refine, identification, attribute, grade, four card values, expiry, binding, favorite and five random-option ID/value/parameter triples. `getinventorylist` does not expose the SQL row ID or `equipSwitch`; these hidden fields are **not** claimed to be compared across dialogue. The nonzero UID supplies cross-dialogue identity, while native mutation preserves the complete record and compares it byte-for-byte around its own unequip callback boundary.

UID-zero refusal is a conservative identity guarantee, not a request to repair SQL. Existing native non-stackable load/add paths generate missing UIDs. The parent agent's read-only live audit found one supported crown across inventory/cart/storage/guild storage and zero supported records with UID zero; no SQL repair is needed or performed by this package.

Current access is checked immediately before crafting and paid mutation, and again after `close2` acknowledgement before opening group 132/133. A pure `S_Access` checks the unchanged level/story/Depth 1/Depth 2 conditions; no reputation is spent. Crown crafting retains its three current material-count checks, conservative free-slot/weight check, and uninterrupted deduction/create sequence.

Paid mutation is now attempted **before** deducting its single material cost. Native refusal consumes nothing, removing both capacity-dependent refund failure and bound-material refund reconstruction. There is no dialogue yield between a successful native mutation and the single `delitem`. A failed re-equip after successful mutation still counts as success and charges once, matching the native API's documented result: the modified crown remains safely in inventory.

This payment ordering relies on the separately reviewed **current synchronous callback closure**, not a claim that arbitrary equipment scripts are transactional. That closure includes all equipped-item/card/ammo Scripts and Equip/UnEquip scripts, combos, random options, active status scripts, pet scripts, autobonus bodies, source-produced persisted bonus scripts and `SC_ITEMSCRIPT` producers. `tools/ci/biosphere_callback_closure_audit.py` is an unconditional dependency of the transaction runner, and is checked again after native execution; it rejects drift in the reviewed callback content/source/import graph. Nested-script extraction supplies traversal evidence, not a regex proof of arbitrary script semantics. Historical/injected arbitrary persisted scripts, malformed illegal card placements and future script/data changes are outside that content-specific proof and require re-review. A native API extension would be needed for a generic guarantee across arbitrary material/access-mutating callbacks.

The parent separately queried live `bonus_script` at approximately 10:55 UTC and reported zero rows and zero online characters; it will repeat that check with services stopped before deployment. This operational observation is separate from the offline gate. The gate does not query SQL, certify unknown historical persisted script text, or synchronize a broad checkout onto the live server.

`tools/ci/biosphere_crown_transaction_test.py` compiles the current production VM/item/mutation/material functions and executes the exact NPC/helper bodies under mandatory kernel network denial and ASan/UBSan. Its explicit doubles are player/NPC lookup, registry persistence/transient compiler switch registers, outbound UI/logging and the equip/status world boundary. The actual parser, VM, inventory listing, count/capacity checks, `pc_additem`, `pc_delitem`, and `modifyequipitem` inventory validation/mutation run natively. Controlled equip-boundary doubles inject changes before the native post-unequip comparison; they do **not** execute a whole live status recalculation or establish client packet delivery.

Positive crown fixtures use an explicit ID-to-matching-job setup for all 18 old classes and Alitea, checked against the effective `Jobs` declarations. Their physical card is the headgear-compatible `4365 B_Katrinn_Card`. This keeps ordinary examples within realistic equipment metadata; it is **not** a native `pc_isequip` proof because equip/status is deliberately doubled. A separate eligible level-250 Novice crafting the Alitea output proves the existing crafter still has no invented requesting-job gate.

The exact original source is also executed: a level-2 confirmation followed by same-ID/different-UID level-8 replacement is actually changed to level 3 and charged 10 runes by the old helper. The candidate's pre-yield snapshot rejects that transition. This is a reproducible server-side race fixture, not evidence that ordinary equipment packets bypass the existing NPC item lock.

Every stat bucket is verified by the actual VM cumulative-selection loop with an exhaustive loop input in place of the random assignment; separate full-dialogue tests use the unchanged real `rand` builtin and real standard distribution at each probability endpoint. The RNG fixture loads a standard `mt19937` state and independently checks its first distributed output. It does not replace the production probability function or claim a random-sampling frequency estimate.

## Final frozen candidate receipt

The fresh final run passed with **4,152 native cases, 200,773 assertions and all 100,000 stat buckets**. Both the pre-run and post-run mandatory callback gates passed. ASan/UBSan reported no diagnostics, the script VM reported no errors/warnings, and the allocator explicitly reported no leaks. The original replacement-crown failure was reproduced in the same run.

Run from the repository root:

```text
wsl -d Ubuntu --exec python3 -B tools/ci/biosphere_crown_transaction_test.py --native-build-dir ../biosphere-crown-native-proof-final-20260906
```

This was a new artifact directory: `pc.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp`, `malloc.cpp` and the driver were all freshly compiled. Unrelated pre-existing map objects supply link dependencies only; this is not described as a fresh build of every server translation unit.

The frozen NPC SHA256 is `790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df`. The latest JSON receipt is `../biosphere-crown-native-proof-final-20260906/receipt.json`, SHA256 `8eb0865682bf2d88135562b3fceb82de9b833d681199c95160f971dd0a8bc4ab`. Its callback-manifest serialization SHA256 is `1ca5aaad95dc00f6b5e8da9c0680adb228482ee3487ae54ad5bab6f0bd6e9c60` (`json.dumps(manifest, sort_keys=True)` encoded as UTF-8).

After the fresh compile, root made the runner's source-region extraction portable to Windows CRLF checkouts: only newline form is normalized for the VM input, while receipts retain the actual NPC byte hash. The unchanged six exact-source sanitizer objects were reused and relinked under the updated runner; the complete gated suite passed again with identical counts. The runner SHA256 is `c351fb0d2fee9868fa145d69086b17fae8d3baf1f744aa946d3e6270ca6d91f0`. An attempted strict reuse with the old runner hash correctly refused before execution; no stale-build guard was relaxed.

Covered final paths include all 19 craft outputs and original menu/cancel positions; menu Escape at every relevant selection; exact admission thresholds and external access loss; missing/reduced materials; full capacity initially and during confirmation; 31 exposed-field/identity changes for each paid service; UID-zero refusal; every supported jewel family/level and both success-threshold sides for every crown; every stat endpoint at every grade; and no-charge native failure with bound materials, capacity loss, or changed target. A successful mutation whose re-equip fails charges once and preserves the modified record in inventory. The callback failure fixtures explicitly compare post-callback inventory state rather than attributing injected changes to the NPC.

These are isolated candidate checks. Live callback-source parity, persisted-script state, deployment and actual player/client interaction remain separate operational checks owned by the parent agent. No official acquisition-economy claim is implied.
