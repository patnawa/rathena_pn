# Depth 1 document exchange: follow-up source audit

2026-09-06. **Read-only findings and proposed tests; unimplemented.** No runtime,
gate, test, database or client file was changed for this audit. No native exchange
case, live-player action, deployment or Git operation was performed. The frozen
Omega/Ellie material repair is separate and does not repair this exchange.

Scope: `Depth Research Administrator#bio_d1` at `ba_in01,292,104`, in
`npc/custom/varmundt_biosphere_depth.txt:202` (body through line 232).

## Current contract and dependencies

- Two item `1001289` documents buy three Depth 1 reputation, with the final pair
  allowed to grant only one or two points to reach the existing 5000 cap.
- Original access is `F_BioDepthQuestAccess`: BaseLevel >=250 and
  `ep17_2_main >=33`. It does not require pre-existing reputation or Zeny.
- Effective item import resolution identifies `1001289` as `Bar_D_Docu_1`, plain
  Etc, native Weight 1. The effective row is in `db/re/item_db_etc.yml:96065`,
  reached through `db/item_db.yml`; its trading restrictions remain unchanged.
- `db/import/const.yml:9` maps `REPUTATION_BIOSPHERE_DEPTH1` to ID 6.
  `db/reputation.yml` imports the effective `db/import/reputation.yml:13` row:
  variable `RepPoints6`, minimum -5000, maximum 5000. This effective identity and
  item row were resolved read-only through the existing Renewal import reader.

The NPC reads reputation and document count before `next`, computes its maximum,
then suspends again at `input`. Neither value is revalidated afterward. Its
maximum uses `ceil((5000 - reputation)/3)`, intentionally preserving the final
partially credited pair. Replacing this with floor division would change the
existing contract and strand characters one or two points below maximum.

## Native semantics and concrete findings

`src/map/script.cpp:6138` implements numeric `input` by storing the clamped
submission and returning -1 below minimum, 1 above maximum, or 0 in range. The
exchange ignores this status. At reputation 0 with ten documents, submitting 0
or a negative number therefore selects one pair and charges two documents;
submitting 6 at the offered maximum 5 selects five pairs and charges ten.
`clif_parse_NpcAmountInput` (`src/map/clif.cpp:13375`) copies the packet value into
`npc_amount` before native continuation; it does not perform this range check.
This does not establish how a particular client's Escape button encodes input.

`get_reputation_points` (`src/map/script.cpp:28035`) reads and clamps the current
registry value. `add_reputation_points` (`:28059`) independently reads the current
registry value, adds the requested amount, clamps to the definition's bounds,
and uses `pc_setreg2` before sending the reputation packet. It does not return
the actual awarded delta to the script. The NPC instead calculates and reports
its gain from the old pre-Next `.@rep`.

These are exact source-derived original controls to execute in a future native
fixture, **not cases run during this audit**:

| Initial reputation | Selected pairs | Reputation at resume | Original document debit | Original actual credit | Original message |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 4990 | 4 | 4999 | 8 | 1 | 10 |
| 4000 | 10 | 5000 | 20 | 0 | 30 |
| 4990 | 2 | 4995 | 4 | 5 | 6 |
| 4999 | 1 | 4000 | 2 | 1 | 1 |

The first two rows consume complete redundant pairs; the third misreports the
credit; the fourth undercredits the same pair because the stale one-point clamp
persists even after the character has room for a full three-point credit.

These stale-state outcomes are deterministic if the value changes during either
dialogue suspension. This audit does not claim an ordinary player can open two
NPC conversations concurrently or identify a normal repeatable gameplay route
that changes `RepPoints6` while waiting. Privileged variable editing exists in
`ACMD_FUNC(set)` (`src/map/atcommand.cpp:10277`); actual permission/command policy
and live use were not tested. Native state injection is the appropriate isolated
regression boundary, not evidence of a demonstrated live exploit.

If documents alone become insufficient, the current single `delitem` is **not**
the earlier multi-material partial-payment defect: `buildin_delitem_search`
(`src/map/script.cpp:8416`) completes its count pass before deleting. For a valid
inventory, insufficient documents cause the builtin to fail and terminate the
script before reputation is added (`:8581`). A fresh count check would give a
clean refusal instead of this native error, but no partial document loss is
asserted for that case.

No large-quantity fix is needed here. With a valid reputation in -5000..5000, the
largest offered batch is 3334 pairs, requiring 6668 documents. This is safely
within `item.amount`'s signed 16-bit representation (`src/common/mmo.hpp:316`).
There is no output item or Zeny payment, so introducing the material helper's
output-slot checks, weight preflight or 30000-debit chunk loop is unnecessary.

## Smallest preservation-compatible patch proposal

Keep the existing dialogue, initial offered maximum, ratio and cap. After the
numeric input, with no further suspension before payment:

1. Require `input(.@amount,1,.@max)` status zero; otherwise close without a debit.
2. Revalidate the original access predicates. A `ba_in01` location check may be
   included as a defensive service-boundary check, not a newly claimed movement
   exploit: native `npc_scriptcont` already checks NPC identity and proximity
   (`src/map/npc.cpp:2292`) before normal packet continuation.
3. Read current reputation, calculate current room and the same ceiling limit.
   Refuse if already capped or if the selected pair count exceeds this new
   limit. Do not silently shrink a user's selected order or debit redundant pairs.
4. Require current document count >= `2 * .@amount` before deleting anything.
5. Calculate `gain = min(3 * .@amount, 5000 - current_rep)` from the fresh value.
   Preserve `delitem` followed by `add_reputation_points`, then report this gain.

This accepts a still-valid selection after a reputation decrease, giving the
correct current credit; it preserves one/two-point final pairs. A rise that still
fits the new ceiling can also succeed with the correctly clipped final credit.
No new rate, refund, automatic quantity conversion or acquisition route is needed.

The no-yield segment still crosses native `pc_delitem -> pc_show_questinfo`.
ba_in01's 19 owners/33 conditions must remain in the mandatory callback closure.
The completed material fixture's condition true/false proof is useful prior
evidence, but is not an exchange test and does not exercise all OR arms. A future
exchange gate must additionally pin this exact document definition and reputation
definition/variable binding and the native reputation read/write/packet path.

`pc_setreg2` routes this variable to `pc_setregistry` (`src/map/pc.cpp:11652` and
`:11531`). With a valid attached, loaded character registry and current definition,
there is no script callback or yield in that reputation write. This proposal is
not a general rollback guarantee for a missing definition, unloaded/corrupt
registry or malformed inventory. The native setter can fail when variables are
not loaded; do not manufacture a refund policy or claim that failure impossible
without a separately reviewed requirement.

## Proposed native regression matrix

Use the actual extracted NPC and `F_BioDepthQuestAccess`, real `input`, `delitem`,
`get_reputation_points`, `add_reputation_points` and reputation database parsing.
Retain the exact original source as a hash-pinned negative control. World indices,
packet delivery and any registry persistence double must be named explicitly;
do not replace the reputation builtin with a simulated award function.

- Valid reputations -5000, 0, 4997, 4998 and 4999; one pair and maximum batches;
  maximum 3334 pairs at -5000; exactly one/two/three-point remaining credit.
- Initial cap 5000 closes without input/payment; insufficient 0/1 documents;
  odd document totals leave the unused document intact.
- Input 0, -1, offered maximum +1 and INT32_MAX: original clamping purchases
  versus repaired no-debit refusal. Test actual forced close separately from a
  numeric value; do not label numeric zero as a proven client Escape packet.
- Inject every tabled reputation change at both `next` and numeric-input yields;
  verify exact documents, final registry delta, packet value and success text.
- Increase reputation enough to invalidate the selected quantity: refuse without
  silently changing the order. Decrease reputation: keep the requested quantity
  and calculate current credit.
- Remove one required document while waiting: original single-builtin failure
  without a partial debit versus repaired clean refusal; test split valid stacks.
- Change level/story during input and exercise the defensive wrong-map path,
  distinguishing direct VM continuation from native proximity-checked packets.
- Exercise the real ba_in01 callback loop during document deletion; preserve
  caller state/RID and all unrelated inventory, quest, Zeny and reputation fields.
- Require clean ASan/UBSan and allocator teardown; count only expected original
  failures, reject unexpected diagnostics even at exit zero, and validate the
  reviewed callback/source/data closure before and after the proof.

## Source evidence checkpoint

Hashes are from the inspected local files, not a live-server attestation. The
exchange-region hash is LF-normalized from its exact declaration through the
blank line before `// Eight-element material fusion.`; it excludes that comment.

| Source | SHA-256 |
| --- | --- |
| Complete frozen depth NPC file | `6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988` |
| Exchange declaration/body region | `974d5863217c92a71aafa7cea4bff7b8dbf0356d8c139068d8e7f95da87de632` |
| `src/map/script.cpp` | `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1` |
| `src/map/pc.cpp` | `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403` |
| `db/import/reputation.yml` | `83cace0cda525350959195af10f80c67db1f3ea5154cd48ac4051f0a571e3272` |
| `db/import/const.yml` | `8c30093c01b623ac7eb814d0f6c2cb660442a90820600003952581910b57f734` |
| `db/re/item_db_etc.yml` | `385e14a6ce2a943ee847cc6ae876390cc4aeac484e77491eaf83962f44cac137` |
