# Abyss Researcher conversion hardening

2026-09-06. Scope: only `L_Convert` in
`npc/custom/varmundt_biosphere_depth.txt`, plus its dedicated native regression.
The parent separately added the read-only `getinventoryslots({char_id})` engine
builtin and owns callback gates, deployment, and the older crown regression.
Every byte before `L_Convert` is preserved relative to the reviewed
`bc55ec95dc341b43f458a71160ecd75b0f33c9d2` NPC. The old recipes, order, Leave
choice, output identities, and per-unit custom economy are unchanged.

## Confirmed original failures

The original NPC computed materials/Zeny maximum before the numeric `input`
suspension, then used `checkweight`, sequential material deletion, Zeny
assignment, and output creation without a fresh transaction-wide preflight.

- All five recipes: reducing Zeny to one below the selected cost during input
  consumes all materials, then native `Zeny -= total` fails with -1. The script
  ends without output and without changing the insufficient current Zeny.
- Recipes 3-5: reducing a later material during input permits earlier material
  deletions before native `delitem` aborts on the later shortage. No Zeny or
  output follows. Native `delitem` preflights only its own item type, not the
  entire multi-material recipe.
- `input` status was ignored: native below/above-range values are clamped, so
  an invalid submission could purchase a clamped quantity.
- `delitem` narrows its request into signed `item.amount`; a x10 request at
  3277 outputs is 32770, beyond int16. Aggregate separated inventory stacks can
  reach this condition. This is not a claim that ordinary live acquisition,
  trade during a dialogue, or the necessary carrying capacity was demonstrated.

The regression pins and executes the original full NPC in a separate native
process. The stale-Zeny finding is **partial material loss, not free output**.
An isolated fixture changes state at the real VM input boundary; no live player
state is mutated for the reproduction.

## Preserved five recipes

Amounts below are per one output. All eleven identities resolve through current
Renewal imports as Etc, Weight 10, without item-specific stack overrides,
autoequip, generated unique IDs, or item scripts. This is project-custom economy,
not an independently asserted official acquisition recipe.

| Recipe | Inputs, in debit order | Zeny | Output |
| --- | --- | ---: | --- |
| 1 | 1001550 x10 | 10,000 | 1001552 Abyss_Magic_Jewel |
| 2 | 1001551 x10 | 10,000 | 1001553 Time_Dimension_Jewel |
| 3 | 1001552 x10; 1001553 x10 | 20,000 | 1001554 Abyss_Rune_Ore |
| 4 | 1001554 x5; 6607 x5 | 30,000 | 1001555 Abyss_Rune |
| 5 | 1001555 x5; 6608 x5; 6755 x5; 25866 x3 | 50,000 | 1001556 Time_D_Ma_Rune |

Other materials: 1001550 Abyss_Jewel_Fragment; 1001551 Time_Dim_J_Fragment;
6607 Temporal_Crystal; 6608 Coagulated_Spell; 6755 Polluted_Spell;
25866 Spell_Of_Time. All outputs have native sell value zero.

## Implemented transaction

The displayed maximum still uses current Zeny and all materials, additionally
capped at 30000 outputs, the native `MAX_AMOUNT`. It is **not** reduced to
3000/6000 outputs: larger supported aggregated inputs are debited in chunks of
at most 30000 each. Every material's complete batch amount is preflighted before
the first chunk. Total price is at most 1.5 billion Zeny, within signed int32;
per-material need is at most 300000, represented in script integer arithmetic.

After a successful, in-range input and before any deduction, the NPC checks:

1. Current location `ba_chess`; original BaseLevel >=250, story
   `ep17_2_main >=33`, and Depth-1 reputation >=2000.
2. Current complete Zeny and all complete material requirements.
3. Current pre-payment weight and exact native output stack/slot capacity.

No Depth-2 threshold is added. In particular, `S_Access(0)` is deliberately not
used because it would exclude previously eligible negative Depth-2 reputation.
The map check scopes the synchronous callback proof to the actual service map.
There is no dialogue suspension between this final preflight and the original
payment-then-output sequence.

Weight remains conservative: existing Weight plus full output weight must fit
MaxWeight before input consumption. No credit is taken for consumed input weight
or slots. `getinventoryslots()` reads the attached player's actual allowed slot
count, not `MAX_INVENTORY` or an assumed default expansion size.

The exact native capacity algorithm is reproduced from `pc_additem`, rather than
the ID-only `pc_checkadditem` approximation:

- Scan inventory entries in native increasing index order for the first
  compatible output: same ID, bound 0, expiry 0, UID string `"0"`, all four
  cards 0. Require that first match's index be within actual allowed slots and
  its amount plus output be <=30000. A terminal first-match failure is not
  bypassed by a later fitting stack or an empty cell.
- When no compatible match exists, require an empty accessible inventory slot.
- Do not add identify/refine/attribute/grade/options/favorite predicates: native
  stack matching ignores those fields. A bound-first/plain-later inventory is
  valid if its first *compatible* stack fits.

The output identity differs from every input of its recipe, so synchronous
material deletion does not invalidate a preflighted existing output stack.
New output may occupy an earlier cell freed by input deletion; all other
inventory metadata must remain unchanged. Refusal performs no deduction, so no
refund is required or attempted.

## Conditional callback guarantee

This is a current-content transaction proof, not a generic atomic inventory API.
The required `biosphere_conversion_callback_audit.validate(ROOT)` runs before
and after the native proof and unconditionally calls the broader reviewed source,
Renewal import, and enabled-NPC gate. It additionally pins the achievement import
graph, exact eleven item definitions, Weight50/Weight90 status records, and the
service-map QuestInfo assumptions. Its nine negative controls reject changes
such as a Get_Item condition that deletes materials or a new `ba_chess`
QuestInfo registration. See `biosphere_conversion_capacity_audit.md` and
`biosphere_callback_closure_audit.md` for the independent closure review.

- Real `pc_delitem` calls `pc_show_questinfo`. The enabled service-map QuestInfo
  collection is empty; the native fixture executes that empty loop rather than
  replacing `pc_show_questinfo`.
- Real `pc_additem` calls `achievement_update_objective(AG_GET_ITEM)`. The seven
  current pure ARG0-threshold conditions execute for native sell-zero output and
  complete no achievement. Reward claim scripts are a separate flow.
- Plain valid Etc input does not invoke equip/unequip scripts. Zeny assignment
  has only the reviewed log and client notification side effects.
- The weight notification can start/end Weight50/Weight90; their pinned current
  definitions have no Script, recalculation, OnTouch, or UnitMove callback.
  That side branch is independently source/data-audited, not claimed to be
  executed by the fixture's client notification double.

Future changed callback content, injected persisted script bodies, illegal item
metadata that breaks native inventory invariants, or a moved service require
fresh review. The gate fails closed; there is no automatic rebaseline mode.

## Native regression and limits

Run from repository root in WSL:

```sh
python3 -B tools/ci/biosphere_conversion_transaction_test.py \
  --native-build-dir ../biosphere-conversion-native-proof-final-20260906
```

The dedicated driver is appended to the existing crown fixture's explicitly
tracked boundary helpers; the old helper source is not edited. The runner freshly
compiles production `script.cpp`, `pc.cpp`, `itemdb.cpp`, `clif.cpp`,
`achievement.cpp`, and `malloc.cpp` with PACKETVER20260219, ASan and UBSan. Other
server support objects are linked as existing support. The exact full NPC and
access function are parsed and executed by the real script VM. Kernel network
access is denied. No SQL, server, or client is contacted.

Native execution covers actual input status, inventory-list and allowed-slot
builtins, material count and deletion, `pc_additem`, Zeny assignment, empty
QuestInfo traversal, and the seven parsed achievement callbacks. Transport/UI,
logs, character-registry persistence, world-ID lookup and queued-world-event
delivery are explicit doubles. Material and output metadata are compared
byte-for-byte. No real multiplayer scheduling, client packet parsing, or secure
timeout timer is claimed: the cancellation case invokes actual
`pc_close_npc(...,2)` while input waits and verifies cleanup/payment preservation.

The getter's registered no-argument and optional-character forms also run through
the actual VM. Direct calls to the same compiled builtin verify raw capacities
0, 1, and MAX_INVENTORY; optional character success without a RID; and, in a
separate expected-diagnostic process, exact -1 pushed value and
SCRIPT_CMD_FAILURE for unknown character and missing attached player. The world
`map_charid2sd` index is explicitly doubled; the getter and its lookup selection
are not. The missing-player case also asserts native script END.

Cases cover all five recipes; quantities 1, 2, 3000, 3001, 3220, 3276, 3277,
6000, 6553, 6554, 10000 and 30000; invalid values; output-cap refusal;
stale Zeny; each material shortage before multi-chunk payment; each access
predicate and map changing; cancellation; exact/deteriorated weight and slot
capacity; binding, UID, card and rental output separation; native-ignored
metadata; bound-first/plain-later ordering; terminal overflow and inaccessible
first-compatible stacks. Positive cases deliberately use Depth-2 reputation
-100 to prove the old access policy is preserved.
All five recipes additionally pay from character-bound input stacks, retaining
every remaining bound stack field while consuming exact amounts.

The runner requires a clean allocator teardown and rejects ASan/UBSan, allocator,
and unexpected native diagnostics even at exit code zero. The two negative child
processes permit only their exact expected errors/warnings; seven synthetic
zero-exit diagnostic controls additionally prove the output guard fails closed.
Each final process's stdout and stderr is retained beside its binary and receipt.

An initial driver run incorrectly modeled cancellation by setting VM END alone;
it correctly failed the uncleared input-wait flag assertion. The fixture now
uses the actual forced-close function. The original failure process additionally
needed the explicit world-ID lookup double when native error reporting attempted
to describe its NPC source. Neither fixture correction changed runtime code.

Final fresh-build run passed:

- Candidate: **225 cases, 49,497 assertions**, no native errors or warnings.
- Original partial-payment child: **8 cases, 177 assertions**, exactly the five
  stale-Zeny and three later-material failures above, with their expected native
  diagnostics retained.
- Getter negative child: **2 cases, 51 assertions**, exact failure return and
  pushed -1 for both missing-player forms.
- All three processes: clean allocator teardown, ASan/UBSan clean. Seven
  zero-exit diagnostic controls passed. Mandatory callback gates matched before
  and after execution; tracked runtime and fixture source hashes stayed equal.

Artifacts: `../biosphere-conversion-native-proof-final-20260906/receipt.json`,
the retained native executable, generated exact-source/YAML fixtures, and
`candidate`, `original`, and `getter-missing` stdout/stderr text files.
Receipt SHA-256:
`00268ccadd47a4ceacbcae27fee2f60503162a3a2982e4447309e2a6d13ddeaf`.
Native executable SHA-256:
`21573eadbbebd3017e4d6527bbc1b58f4bf2f9335d62a38daf5530ec6ff6c1fa`.
Conversion callback manifest serialization SHA-256:
`908c5c41ff62cc3cffc7b33dd32e1b82253a4eb75b1fafffa04f020afb98f25c`.
The final directory freshly compiled all six production units; earlier iterative
development-directory runs are not substituted for that receipt.

## Frozen runtime provenance

- Original complete NPC SHA-256:
  `790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df`.
- Patched complete NPC SHA-256:
  `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28`.
- Preserved pre-`L_Convert` prefix SHA-256:
  `efb7f0e494fa2eb818de329ac1da638f004f24070a3f6220aa48263f22a1444c`.
- Current source `script.cpp`, including parent's read-only getter:
  `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1`.
- Current `pc.cpp`:
  `d02a7ac0a42c3ab768075580dba3b01a8941c1cac4e0997895fa24ba262fda8d`.
