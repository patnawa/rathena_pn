# Episode 18 Gudra: Recording Note hand-in source audit

Date: 2026-09-06. This is the historical pre-repair source audit after the
document-exchange batch `777fb94c5`. The defects below describe frozen original
source `50d771b5...`; the reviewed repair is now installed locally at
`8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe`.

## Conclusion

Three defects in the frozen original are confirmed from the enabled NPC and
native source:

1. The global capacity check tests 20 Amethyst Fragments even when the branch
   will issue one Recording Note or award only three/four Amethyst Fragments.
   It can admit a Note grant that cannot fit, and reject a daily reward that can.
2. The first completed-story hand-in lacks the lost-Note recovery present in
   the other three recovery branches. A normally issued Note can be sold to an
   ordinary shop for zero Zeny; after all three stories are complete, this
   leaves the first hand-in stuck at a failing `delitem`.
3. Both reward checks precede real dialogue suspensions. Ordinary party loot
   can change the recipient's inventory while the dialogue is open. A newly
   full plain Amethyst stack causes the eventual `getitem` to fail after the
   Note and quest completion have already been consumed. The VM then continues
   with the remaining EXP/reputation/progression operations.

These are not evidence that every dialogue interruption corrupts a quest.
Missing input at `delitem` terminates the script before subsequent quest/reward
mutations. The confirmed partial-completion loss is the separate output-failure
case. This initial audit performed no native execution; the subsequent
installed-source proof is recorded in `episode18_gudra_transaction_audit.md`.

## Frozen pre-repair transaction policy

`npc/re/scripts_athena.conf:250` enables `npc/re/quests/quests_18.txt`.
`Folklorist Gudra#ep18` is on **wolfvill,61,170**, not `ba_in01`.
The audited block is `quests_18.txt:7545` through its `OnInit` ending at 7774.
Every branch first tests `checkweight(1000405,20)` and `ep18_main >= 36`.

| Branch | Current committed operations, in order |
| --- | --- |
| First acceptance | After the last `next`, set quests 16551/16552/16553, grant one Note, close. |
| First stories incomplete | If no Note, grant one; retain the current story records. |
| First hand-in, quest 16554 active | Delete one Note; complete 16554; set cooldown 16559; grant 20 Amethyst; EXP 7,769,124 / 3,000,000; reputation 3 +100; existing main-story progression check. |
| Daily acceptance | After the last `next`, set quests 16555/16556/16557, grant one Note, close. |
| Daily stories incomplete | If no Note, grant one; retain the current story records. |
| Daily hand-in, quest 16558 active | Recover a missing Note before dialogue; after the final `next`, delete one Note; erase 16555 through 16558; set 16559; reputation 3 +30; determine cap bonus; existing main-story progression check; grant 3 + bonus Amethyst; EXP 18,252,408 / 2,000,000. |

The first reward's delete/grant are at 7646/7649; the daily delete/grant are at
7742/7757. The five existing Note grants are at 7610, 7614, 7712, 7716, 7721.
There is no corresponding missing-Note guard in the first hand-in case.

Dinar, Amira, and Shanina complete the three respective story records and set
the hand-in quest once the other two are complete. These story NPCs do not
require possession of the Note. A repair must not reset or repeat their story
completion merely to recover the missing item.

The effective quest records 16551 through 16558 have no item drops/targets or
timed side effects. Quest 16559 has `TimeLimit: 4h`: without the duration `+`
prefix, the native quest parser treats this as the next local **04:00**, not
four hours from purchase. Preserve its existing absent/active/expired handling.

The effective reputation record is ID 3, variable `RepPointsWolf`, with minimum
-5000 and maximum 5000; `REPUTATION_EP18` resolves to 3. The daily bonus is
calculated **after adding 30**, so fresh reputation 4969 yields 4999 and three
items; 4970 yields 5000 and four. Already capped reputation still awards four.
Do not introduce a new reputation-cap rejection. The existing transition is
`ep18_main == 36` and post-award reputation at least 1000: set main state 37
and quest 18082. First and daily reputation amounts must remain +100/+30.

## Effective item definitions and ordinary reachability

The ordered Renewal item imports produce these complete relevant definitions:

| ID | Identity | Relevant explicit fields |
| --- | --- | --- |
| 1000405 | Ep18_Amethyst_Fragment | Etc; BuyingStore enabled. |
| 1000408 | Ep18_Recording_Note | Etc; NoDrop, NoTrade, NoCart, NoStorage, NoGuildStorage, NoMail, NoAuction. |

Neither record specifies Weight, Buy, Sell, Script, EquipScript, UnEquipScript,
GUID, AutoEquip, or an inventory stack override. Native item defaults therefore
give both weight zero and buy/sell value zero. The ordinary `getitem` output is
identified, unbound, non-rental, UID zero, with four zero card fields. This is
native/default evidence, not a proposal to add or change an economy field.

The Note does **not** have `NoSell`. `pc_can_sell_item` (`pc.cpp:1261`) permits a
normally issued unbound, unequipped, non-favorite Note for an ordinary player
with the existing trade permission. `clif_selllist` excludes negative prices,
not zero prices, and `npc_selllist` accepts and deletes a zero-price sale.
For example the enabled ordinary shop `Vegetable Gardener#ra` is declared at
`npc/merchants/shops.txt:268` (Rachel,65,80); that file is enabled by
`npc/scripts_athena.conf:152`. No shop purchase of a Note is being asserted.
The enabled-source search found no other item grant/box/drop route for the Note.

Thus selling the Note is an ordinary source-supported way to lose it, without
GM commands or malformed inventory. Once all three first stories have set
16554 active, Gudra reaches the unguarded first-hand-in `delitem`; its failure
ends the script. The three story records remain completed, but this branch
cannot recover the Note. Selling while the first stories are still incomplete
is recoverable through the existing incomplete-story branch.

There is also an ordinary failed-issuance route: fill all allowed slots, leave
a plain Amethyst stack with room for 20, and hold no Note. The global check
passes by seeing an existing Amethyst stack. Acceptance sets all three story
quests, but a new Note has no slot and fails. Story progress can nevertheless
reach the same unrecoverable first hand-in. The daily counterpart can recover
later; initial issuance still should not claim successful delivery on failure.

## Capacity and stale-dialogue counterexamples

`checkweight` delegates existing-stack capacity to `pc_checkadditem`
(`pc.cpp:5729`). That helper selects the first same-ID row without considering
the output's bound/rental/UID/cards identity. Actual `pc_additem` (`pc.cpp:5991`)
matches ID, bound, zero expiry, UID and all four card fields. The **first
compatible** stack's overflow is terminal; it does not fall back to a later
stack or a newly freed empty slot. Its native amount cap is 30000.

An ordinary, all-plain inventory suffices for the stale-check defect:

1. Hold the required Note and a plain Amethyst stack of 29980. The initial
   check for 20 passes on either hand-in path.
2. Pause at a real `next`. With loot distribution enabled for the party, another
   same-map party member picks up 20 player-dropped Amethysts. The native random
   recipient may be the conversing player; that stack now reaches 30000.
3. Resume Gudra. The Note is deleted, quest/cooldown changes commit, and the
   Amethyst grant fails at the full first compatible stack. A freed Note slot
   does not help. The first reward loses 20 items; the daily loses three/four.

`party_share_loot` (`party.cpp:1298`) checks same map, life state and the configured
idle rule, but has no `npc_id` receiver exclusion. Current
`conf/battle/party.conf` sets `party_item_share_type: 0` (random distribution,
including player drops) and `idle_no_share: no`. The Amethyst is ordinarily
droppable. This is a reachable random outcome, not a claim that every pickup
selects this recipient. The recipient need not perform a prohibited action:
self-pickup during a dialogue is blocked by `pc_cant_act`; the party receiver
path is distinct. Native party execution remains an intended test, not a test
already performed for this audit.

The daily false rejections are simpler: 29997 plain Amethysts can accept the
three-item reward below the bonus boundary, and 29996 can accept the four-item
reward at/above that boundary. The global 20-item check rejects both.

A second, no-pause counterexample is a same-ID bound row with room for 20,
followed by a full plain row. The ID-only check accepts while the plain grant
fails. This is a native-valid inventory-state test; no ordinary bound-Amethyst
producer was established here, so it must not be presented as an observed or
proved ordinary acquisition route.

The `getitem` failure branch (`script.cpp:7738`) reports failure without setting
the script state to `END`; `run_func` logs that return and the VM continues.
In contrast, insufficient `delitem` (`script.cpp:8649` onward) closes the script
and sets `END`. Neither builtin provides transactional rollback of prior quest,
item, or reputation effects.

## Callback closure that the repair proof must cover

This audit used the existing read-only ordered DB/NPC collectors and
comment/string-aware source scanner: 900 enabled NPC files and 14 include
configuration files. On literal `wolfvill` NPC declarations there are **72
questinfo registrations owned by 35 NPCs**: 71 registrations in `quests_18.txt`
and one in `npc/custom/episode19/quests_19.txt`. The latter is the Grey Wolf
Villager's Episode 19 availability condition.

The reviewed wolfvill conditions read quest state, `ep18_main`, `ep19_main`,
BaseLevel, and item counts through `isbegin_quest`, `checkquest`, and `countitem`.
They do not mutate inventory or reputation or yield. Wolfvill duplicates of
the two dummy NPC bodies, `#contest1`, and `Half Flower#EP18_R01` add no questinfo
registrations. The flower's separate player interaction does not run simply
because a questinfo condition is evaluated. The enabled-function scan found no
function containing questinfo registration/refresh; no dynamic duplicate or
UNPC_MAPID producer was found. The reviewed unitwarp/refresh source inventory
did not establish an additional wolfvill registration route. These are pinned
current-content findings, not a general script-language safety proof.

The synchronous closure is broader than a material's item Script:

- `pc_delitem` and `pc_additem` can update weight/status and call
  `pc_show_questinfo`; `pc_additem` also dispatches `AG_GET_ITEM` achievement
  conditions. Weight50/Weight90 currently have no item-mutating Script or
  calculation flags; both transaction items currently have zero weight.
- `setquest`, `erasequest`, and `completequest` invoke native quest operations
  and then `pc_show_questinfo`. The native condition evaluator detaches the
  current script, runs the condition, and restores it. The correct fixture
  must initialize the real 35-owner/72-condition display list and reject an
  empty or mismatched list; otherwise native count guards can silently skip it.
- The effective achievement graph has 361 records. All seven `AG_GET_ITEM`
  conditions are pure ARG0 thresholds. The two items' zero sell values make
  their current ARG0 value zero, so these conditions do not complete here.
  The engine still sets/clears `ARG0`; do not claim that no registry is written.
- EXP may level the player. `pc_gainexp` can reach base/job level changes,
  status calculation (including other equipment/card/combo scripts), questinfo,
  ten goal-level conditions, fourteen goal-status conditions, and recursively
  twenty achievement-level conditions. Their reviewed condition expressions
  are read-only. Achievement completion bookkeeping is not an automatic claim
  of the achievement's reward script.
- Base/job level events use `npc_script_event`; with an active NPC dialogue,
  `npc_event_sub` queues other NPC events rather than running an arbitrary
  event inline. No enabled literal OnPCBaseLvUpEvent/OnPCJobLvUpEvent handler
  was found. This does not justify replacing actual EXP with a no-op in a test
  and then claiming full native level-up coverage.
- Native quest updates set save flags, alter/compact quest records and send
  packets; configured quest saves involve persistence transport. A normal
  native-call success proof is not a database/network crash-atomicity guarantee.

Normal player dialogues require loaded registries. A repository search found
no enabled direct `RepPointsWolf` assignment outside the reputation interface;
the Episode 18 native additions preserve the effective bounds from valid raw
state. The native getter clamps its result while addition uses the raw registry
before its own clamp: privileged SQL/GM-corrupt out-of-range raw state must be
reported as outside the normal loaded/in-range proof, not silently assumed
equivalent to the displayed getter value.

The source review above was a purpose-scoped preflight. Subsequent work added
the fail-closed wolfvill callback gate and native proof. The current gate passes;
installed-source receipt SHA-256 is
`fe40476cceba69a88b0ae094a8a32e8e726d42b86113909748ade9ca2d377c7d`.

## Historical repair design

Keep all prices, item definitions, reward amounts, reputation, EXP, story
progression, and daily reset policy. Do not change generic `checkweight` or the
engine. Replace the global wrong-item check with a purpose-scoped exact plain
grant preflight shared only if the selected companion service has the same
explicit contract.

1. Before each initial Note issuance, after the last dialogue yield, recheck
   wolfvill, access, and the relevant initial quest state; check space for one
   plain Note before setting the three story quests. For all recoveries use
   the same Note-capacity check. Add the missing first-hand-in recovery without
   altering completed stories or quest 16554.
2. Immediately before either hand-in mutation, recheck wolfvill, access, fresh
   expected story/hand-in/cooldown state, and at least one Note. Recompute the
   daily quantity from fresh reputation **after the intended +30 clamp**.
   Do not carry a pre-dialogue reward quantity or cached inventory snapshot
   into this interval. Reject with no payment/reward progression if state is
   no longer eligible; do not manufacture a quest state to force success.
3. Snapshot inventory once and simulate the one Note debit plus plain reward
   grant. Ordinary `delitem` prefers the first ascending same-ID row with
   equip=0, refine=0 and four zero cards; if none exists it selects the first
   same-ID row in its fallback pass. This item is Etc, so no pet exception is
   involved. It does not require a matching bound/expiry/UID/options value.
   Credit a released slot only when that selected row contains exactly one
   unit. This avoids an unnecessary rejection when handing in the Note itself
   creates the only valid empty slot.
4. For the output, match the first ascending plain-compatible row using the
   actual `pc_additem` identity fields, enforce the native amount/stack and
   allowed-index limits, and do not fall back on first-match overflow. With no
   compatible row, require an allowed empty row after the simulated debit.
   Use the existing native `getinventoryslots` accessor and `MAX_INVENTORY`,
   not a guessed fixed slot count. Do not add refine/identify/attribute/options
   filters that native plain-output stack matching ignores.
5. Commit without another yield. Preserve the existing operation ordering
   wherever the scoped current-callback proof permits it. Do not rely on
   compensation by a later `getitem` or on a failed builtin ending the script.
   No Note duplicate should be created if it is already present.

This helper is implementable at NPC level for the two pinned Etc definitions
and valid positive-amount inventory/cache. It should reject invalid arguments
and unknown identities, not pretend to handle arbitrary equipment, rentals,
GUID-producing definitions or future scripts without re-review. The current
zero item weights make before/after-debit weight identical; a reusable helper
must state its weight policy rather than silently generalize that fact.

## Historical native-regression checklist

- Parse and execute the actual Gudra body and helper, with real loaded
  reputation/quest/item definitions and native inventory builtins. Bind all
  production source and retained object hashes. Keep old-source controls
  separate from fixed-source assertions; label already-safe controls honestly.
- Reproduce both ordinary stale reward failures at a real dialogue pause using
  native party loot distribution; prove fixed first/daily paths keep the Note,
  quests, cooldown, EXP and reputation unchanged on the fresh capacity refusal.
  Do not merely mutate an array and call that an ordinary-party proof.
- Reproduce the original failed initial Note delivery after story-quest grants;
  test both acceptances and all recovery branches. Prove a lost first Note is
  recovered without resetting any completed story or hand-in record. Include
  the ordinary shop sellability/deletion route and a source-bound story-NPC
  transition sequence, or explicitly limit the narrower fixture's claim.
- Cover exact stack maxima, missing/new slot, first-compatible full with a free
  slot, bound/UID/card variants, Note amount one versus multiple, preferred vs
  fallback Note deletion, allowed-index limits, ordinary fields ignored by
  stack matching, and all unknown/invalid helper arguments. Keep the complete
  valid-inventory range and configured VM loop limits in scope.
- Cover daily reputation 4969/4970/4999/5000 and valid negative bounds, first
  reward always 20, +100/+30 and the 1000 progression boundary, absent/active/
  expired 04:00 cooldown, cancellation, repeat calls, and fresh map/access/
  material/quest changes across each relevant yield.
- Execute all actual wolfvill conditions with real nonempty 35-owner / 72-entry
  initialization and per-commit callback counters; reject wrong-size/reinit
  skipped-loop controls. Execute all seven current item-achievement conditions
  and characterize ARG0 cleanup and loaded registry dirty flags accurately.
- Exercise representative native EXP level transitions, equipment/status
  effects, goal-level/status/achievement conditions, and quest reindex/save
  bookkeeping, or clearly report any fixture boundary. Gate the exact enabled
  graph, reputation/quest/item/achievement/weight definitions and dynamic QI
  producer inventory, with mutator/new-registration negative controls.

This historical plan did not itself authorize installation. Local source
installation was subsequently authorized and completed.

## Independently read raw source identities

| File | SHA-256 |
| --- | --- |
| `npc/re/quests/quests_18.txt` (pre-repair raw) | `50d771b504dfa2eafefcbafee9680eebdadfc2edc08e21428051aebb2dc78b0f` |
| `npc/re/quests/quests_18.txt` (installed raw/LF) | `8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe` |
| `src/map/pc.cpp` | `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403` |
| `src/map/script.cpp` | `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1` |
| `src/map/party.cpp` | `9286c5b0fd8b5b27329dcb13305ad352d721cb2a6e4a1dbaba40147eb7d90d6a` |
| `src/map/quest.cpp` | `e4fb9b081712da9be44c39bb6f0e22e075ffd21d6fdfdd4f42f3433a1de8a684` |
| `db/re/item_db_etc.yml` | `385e14a6ce2a943ee847cc6ae876390cc4aeac484e77491eaf83962f44cac137` |
| `db/re/quest_db.yml` | `0beb2cb314f8a73d614957d47d44316911560a4fbe79379eb0819ef930bf2754` |
| `conf/battle/party.conf` | `33fa6fd3bd8a4e3ca8f47d5960daf03199ce359a092f9bfb6a7b569992cca041` |

These identify local source inputs only. No live binary equivalence, persisted
state range, remote startup, SQL result or all-gameplay coverage is asserted.

## Live deployment addendum — 2026-09-07

The historical source-audit boundary above remains accurate for that proof.
The repaired `quests_18.txt` was subsequently included in the exact 13-file
release, validated in an isolated native startup, deployed, and read back from
the live server at raw SHA-256 `8d3af9e9...`. The POST map startup and guarded
SQL checks passed. Full artifact, receipt, rollback, and runtime evidence is in
the [2026-09-07 deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
