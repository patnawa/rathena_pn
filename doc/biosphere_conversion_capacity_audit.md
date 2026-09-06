# Biosphere five-recipe conversion capacity audit

Independent read-only review on 2026-09-06, starting from project/live
`bc55ec95d`. Scope is `L_Convert` in
`npc/custom/varmundt_biosphere_depth.txt`, not a generic inventory transaction
API or a change to acquisition economics. The earlier live review had four
preserved differences. Root subsequently reconciled the Garden source and
disabled its legacy gates; three preserved live source differences remain.
Neither the historical four-file overlay nor the remaining three modifies
these eleven item definitions, the Biosphere map scripts, or the achievement
database.

## Recommendation

Keep payment-then-output, but permit payment only after a complete, freshly
evaluated, exact plain-output capacity check and all-resource/access check.
Preserve the existing conservative **pre-payment** weight/space policy: do
not count slots or weight that input deletion might later free. Use the new
narrow read-only `getinventoryslots()` accessor approved by root, rather than
guessing the character's allowed slots or rejecting valid differently bound
stacks. Cap a single output grant only at native `MAX_AMOUNT` (30,000); split
large material debits into chunks of at most 30,000.

This is safe under the reviewed ordinary valid-inventory/current-content
conditions below. It is not a rollback guarantee for arbitrary injected
scripts, corrupt inventory, concurrent administrative mutation, or a future
content change. Actual NPC/helper and native regression results belong in the
implementer's receipt; this document does not claim those tests were run by
the audit agent.

## Unchanged recipes and metadata

| Recipe | Inputs per output | Output | Zeny |
| --- | --- | --- | ---: |
| 1 | 1001550 x10 | 1001552 | 10,000 |
| 2 | 1001551 x10 | 1001553 | 10,000 |
| 3 | 1001552 x10 + 1001553 x10 | 1001554 | 20,000 |
| 4 | 1001554 x5 + 6607 x5 | 1001555 | 30,000 |
| 5 | 1001555 x5 + 6608 x5 + 6755 x5 + 25866 x3 | 1001556 | 50,000 |

All eleven distinct IDs are effective `Etc`, weight 10 in native units, and
stackable. Ordered Renewal imports were followed, including partial-override
inheritance. They have no Script, EquipScript, UnEquipScript, equipment
location, GUID/unique-item flag, auto-equip flag, or inventory-specific stack
cap. Their only active flags are BuyingStore, plus DropEffect CLIENT on
1001550..1001556. The five output definitions have native buy/sell zero.
6607, 6608 and 6755 have Buy 20; their derived sell price does not affect the
output callback. None is a pet egg. No source restriction requiring unbound,
unfavorited, or unique-ID-zero **input** materials was found or should be
invented.

`getitem` creates a zero-initialized item, then sets the output ID,
identify=1 and bound=BOUND_NONE. Thus output cards, expiry and unique ID are
zero. With these definitions it makes one `pc_additem` call for the complete
quantity: it does not automatically distribute a grant over multiple stacks.

## Why the old check is insufficient

`pc_checkadditem` (`src/map/pc.cpp`) checks the first matching **ID**, and even
has a FIXME for omitted card identity. `pc_additem` instead scans indices
0..MAX_INVENTORY-1 for the first match of:

```text
nameid == output
bound == BOUND_NONE
expire_time == 0
unique_id == 0
all four card fields == 0
```

After finding that first compatible stack, overflow or an index at/above
`sd->status.inventory_slots` is terminal. It does not try a later compatible
stack or an empty slot. If no compatible stack exists, it uses the first
empty inventory index and rejects one outside the allowed slots.

Examples: an early small bound stack followed by a full unbound stack can
pass `checkweight` and fail the add after inputs were consumed. Conversely,
an early full bound stack followed by a small unbound stack can be rejected
by the old check even though the native add would succeed.

The native match deliberately ignores database item-row ID, identification,
refinement, attribute/broken state, enchant grade, random options, favorite,
equip and equip-switch fields. The scoped checker must not add these as
extra stack-identity conditions. A valid Etc material cannot ordinarily be
equipped; illegally equipped/corrupt records are outside this proof.

`getinventorylist` exposes every occupied positive-amount row, its true
inventory index, all four cards, binding, expiry and the UID as a decimal
string. It enumerates the full compiled array, not just the currently allowed
slots. It did **not** expose the latter limit. `MAX_INVENTORY` is the compiled
ceiling, not a character's actual capacity; default inventory is 100 with up
to 100 expansion slots for this packet configuration. The approved getter
returns exact `sd->status.inventory_slots`, with failure for a missing player.
A first-ID-only guard or a hardcoded 100/200-slot limit would unnecessarily
reject valid supported inventories and is not the recommended fix. A generic
`checkweight` engine rewrite is not needed for this bounded task.

## Complete preflight and commit sequence

After the last `input` suspension, before the first item/Zeny mutation:

1. Require a positive integer quantity no greater than 30,000, the requested
   recipe's unchanged current maximum from **all** material counts and Zeny,
   and the same required Biosphere access. Recheck that the player is on
   `ba_chess`; the callback conclusion below is map-dependent. Preserve the
   existing recipe/access policy, without adding a new reputation threshold.
2. Compute each full material debit and total Zeny debit in script integer
   arithmetic. Check every material independently using fresh `countitem` and
   check total Zeny. Do not trust the maximum displayed before the input.
   A 30,000-output batch requires at most 300,000 of one material and at most
   1,500,000,000 Zeny; these calculations fit script/native signed-32 limits.
3. Require `Weight + amount * getiteminfo(result, ITEMINFO_WEIGHT) <= MaxWeight`
   using the reviewed output weight 10. This intentionally does not subtract
   future material weight, matching the existing conservative policy.
4. Call `getinventoryslots()` and `getinventorylist`. Validate the returned
   slot limit and find the first compatible output row in true index order.
   If found, require its index below the allowed limit and
   `existing_amount + amount <= 30000`. Stop at this first match even when it
   fails; do not continue looking for another stack or use a free slot.
5. If no compatible row exists, require at least one genuinely empty index
   below the allowed limit. For valid inventory, counting occupied rows whose
   true index is below that limit and comparing with the limit is equivalent.
   Rows outside the limit do not consume an allowed slot, but a compatible
   output there still causes the terminal failure in step 4.
6. With no intervening dialogue/sleep/detach, pay each fully preflighted input
   in chunks `min(remaining, 30000)` until its exact total is consumed. Then
   deduct the unchanged total Zeny and grant exactly `amount` output once.
   Do not replace exact capacity with `checkweight(result, amount)`; that
   function can select the wrong first ID and also falsely reject a valid
   later compatible stack.

The chunking is correctness-critical: `delitem` assigns its argument to
`struct item::amount`, an **int16**, before searching. A large aggregate debit
can wrap negative or small and become a no-op or an underpayment. An arbitrary
3,000/6,000-output cap would avoid this but reject otherwise valid aggregated
material inventories. Chunks preserve all native-supported output quantities.

No output ID is an input ID in the same recipe. Deletions therefore cannot
remove or fill the checked output stack. Valid material deletion only lowers
weight and can free slots; it cannot invalidate a pre-payment capacity pass.
All full debits were counted before mutation, and the callbacks below do not
change the remaining payment inventory. Consequently each bounded `delitem`
has enough materials and the final `pc_additem` remains admissible.

`countitem` excludes rental rows, but native ID-only `delitem` can consume
rental/bound/UID-bearing rows and prefers unequipped, unrefined, cardless
rows before other matching IDs. This audit does not silently redefine that
existing payment preference. Enough counted non-rental material also implies
enough total deletable Etc material. Rental timers do not interleave this
straight-line commit. Bound materials must not be prohibited merely because
the new output is unbound; that is the unchanged recipe behavior.

## Actual synchronous callback closure

```text
delitem (count pass, then delete pass)
  buildin_delitem_delete
    pc_delitem
      log / decrement amount and weight / clear exhausted row
      pc_unequipitem only if the exhausted row was equipped
      client item/weight notification
        pc_updateweightstatus -> Weight50/Weight90 start/end on threshold crossing
      pc_show_questinfo
        current map qi_npc conditions -> achievement_check_condition -> run_script
Zeny assignment
  pc_setparam(SP_ZENY): log, clamp/assign, clif_updatestatus, return
getitem
  pc_additem: exact match / amount / weight / allowed-slot checks, insert
    log, weight/client notification; auto-equip/rental branches absent here
    achievement_update_objective(AG_GET_ITEM)
      current Get_Item condition scripts
    pc_show_questinfo
```

Material type alone does **not** prove absence of callbacks: every native
deletion invokes QuestInfo. Both QuestInfo and achievement conditions are real
script evaluations, with parent RID detachment/restoration; the parser wraps
conditions in `achievement_condition(...)` unless already present.

Weight notification is also not packet-only: `clif_updatestatus(SP_WEIGHT)`
calls `pc_updateweightstatus`, which can start/end Weight50 and Weight90.
Both effective status records have no Script or CalcFlags, no OnTouch/UnitMove
hooks, and only mutually end the other weight status on start. Weight90 also
stops attacking. Thus these threshold changes do not run status calculation,
equipment/bonus scripts or inventory/access mutators. Both complete status
records are included in the scoped gate, in addition to their broad DB pins.

For the reviewed `ba_chess` scope, four static NPC declarations are present:
the Depth Abyss Research Manager, Abyss Researcher, entrance trigger and exit
warp. The two enabled Biosphere files contain no `questinfo`, `showevent` or
`questinfo_refresh` registration. Their called access/purge/quest helper
functions do not register conditions. The full enabled 900-file graph was
searched for dynamic registration/relocation alternatives: there are no
runtime `duplicate`, `duplicate_dynamic` or `UNPC_MAPID` setters. NPC
`unitwarp` destinations are other explicit maps or fixed instance maps
(`1@ch1a`, `1@ch1b`, `1@bamn`, etc.); none installs a QuestInfo NPC on
`ba_chess`. `movenpc` does not change maps. Thus the current scoped QuestInfo
registry is empty, rather than being assumed semantically harmless.

The selected achievement root imports `db/re/achievement_db.yml` and
`db/import/achievement_db.yml`, with 361 active ordered records. The seven
Get_Item records, 220023..220029, contain only `ARG0 >=` comparisons against
100, 1000, 5000, 10000, 50000, 100000 and 150000. `ARG0` is the output's
native sell value, zero here, so none completes or reaches dependent/level
recursion. The twenty Goal_Achieve conditions, 240001..240020, themselves
only read AchievementLevel. Achievement reward scripts run in the separate
reward-claim response, not automatically in `achievement_update_achievement`.
The direct Zeny assignment does not invoke AG_SPEND_ZENY. Temporary ARG0
registry setup/cleanup is unrelated to inputs, Zeny, access or capacity.

Accordingly no script between material debit chunks mutates inventory,
capacity, Zeny or access, and none yields. No input unequip/status-calculation
callback is entered by valid ordinary Etc rows. Future QuestInfo producers,
changed item flags, achievement conditions, or arbitrary administrative
scripts require a new review. The existing crown callback sentinel does not
pin achievement databases; root owns the appropriate explicit regression
pin/rebaseline. Known deployed binary provenance remains a separate receipt,
not inferred from source hashes or the preserved live source profile.

## Required verification cases

The implementer's actual-NPC/native suite should check all five recipes and
all payment identities, not a simplified model in place of native adds:

- Post-input loss of each material separately, insufficient Zeny, lost access,
  changed map, invalid quantity and maximum boundary; all fail before writes.
- Plain merge/new slot, bound-first/plain-later, rental-first/plain-later,
  UID/card-bearing first rows, no compatible row, and a completely full
  inventory. Bound inputs continue to work.
- First compatible stack full with later room/free slot; first incompatible
  stack full with later compatible room; ignored fields such as options,
  refine, identify, attribute, grade and favorite do not cause false rejection.
- Slots at both native allowed boundary sides, first empty outside allowed
  slots, expanded inventory, sparse indices, and incompatible rows outside
  allowed slots. The getter must return the real native field.
- Exact MaxWeight success and one-unit-over failure before payment; do not
  count material-deletion weight or slots as preflight room.
- 30,000 output and 30,001 rejection; aggregate input debits above 32,767 and
  65,535, with exact chunk counts and Zeny/output totals and no integer wrap.
- Actual callback boundary checks: empty `ba_chess` QuestInfo registration;
  current seven native achievement conditions and parent-state restoration;
  fail-closed changed-condition/metadata/source guards, without claiming that
  a stubbed callback world proves arbitrary script purity.

This audit task owns only this new document and the new conversion evidence
gate below. It did not modify an NPC, engine, database, deployment, or existing
test. Root owns the read-only getter and existing broad callback gate; the
class agent owns conversion implementation and its native regression suite.

## Independent executable evidence gate

`tools/ci/biosphere_conversion_callback_audit.py` was additionally authorized
for this audit. Its public `validate(ROOT, profile=None)` first requires the
existing broad callback gate; there is no skip-base or automatic-accept option.
It then independently traverses and pins the missing achievement database
graph, all eleven effective material records and the scoped map evidence.
Every shared source reread must agree with the already validated broad
manifest. The broad gate carries exact engine/NPC source bytes; the scoped
keyword inventories are explicit review evidence, not an arbitrary-script
semantic sandbox.

The three conversion-specific canonical JSON section pins are:

- Materials, complete records, both weight-notification statuses and reviewed
  recipe constants:
  `917a3c4e66bace9f94479cb4cc135e8c3a167d133f19acc0940481f1d4f630ec`
- Achievement files, ordered imports/records and 27 condition texts:
  `893b6aa217ba6c43f8375e00f200a3dc2030e6434037dd37aba5f3252f8c3d8e`
- Scoped NPC declarations, map mentions and relocation evidence:
  `9f0f650cbf9e23baf7a25ea6e62d7769f6aba260204b34092ddc4e058bfe5c38`

The additional three database files and newline-normalized hashes are:

| File | SHA256 |
| --- | --- |
| `db/achievement_db.yml` | `6025a9b11478a570fc47d30f6c053be4ad942cbf001861c0eb2e11cc8b346d12` |
| `db/re/achievement_db.yml` | `b91ee560ffbd716ba784f818413806e8e0fad8e0d2223a34f2267fc3fc082305` |
| `db/import/achievement_db.yml` | `7eb14a7b9dbb3adfac1d565cd4e38abe9e52e5d7cb159b5f7680b531677bf106` |

The achievement graph contains 361 ordered/effective records; their canonical
ordered-record hash is
`f9982c03644af8e48cda3e8dae5a21006d1974c864f3e4193a9d0f05af8e4e20`.
The map evidence records four declarations, zero registrations in the two
Biosphere source files, zero runtime duplicate/map setters, and 33 unitwarp
statements across the enabled 900-script/14-include graph. The reviewed NPC
at this stage has raw SHA256
`40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28`;
root owns updating the broader gate after final runtime freeze.

```text
python3 tools/ci/biosphere_conversion_callback_audit.py --root /path/to/server
python3 tools/ci/biosphere_conversion_callback_audit.py --root /path/to/server --manifest
python3 tools/ci/biosphere_conversion_callback_audit.py --root /path/to/server --negative-controls
```

An explicitly selected `--profile live-20260906` is forwarded to the broad
gate; it is never inferred after a default failure. The conversion-specific
evidence is identical across the reviewed preserved live differences. Both scripts
must be available side by side when copying the gate outside the repository.

The scoped in-memory rejection controls cover an actual Get_Item condition
changed into a material mutator; missing and newly imported achievement data;
a new `ba_chess` QuestInfo registration; a newly enabled map NPC source; a
material Script mutation; a material GUID flag mutation; a weight-notification
status Script mutation; and tampered evidence.
They do not write any checkout or runtime file. The public gate additionally
requires root's reviewed broad source rebaseline before it can pass.

Final verification on 2026-09-06: public CLI `--negative-controls` exited 0,
first passing the mandatory broad gate plus complete scoped validation, then
rejecting all **nine** deliberate scoped changes. Root's current default
engine/NPC section pins begin `85a78b70` / `dd76ad03`; their exact values remain
owned by the broader gate. Final conversion-gate raw SHA256:
`9b8cf2bb95602a2464491547b3d4ee43cab64a0bf2f4c24c56b48ec7fcbe1a76`.
