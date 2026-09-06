# Omega and Ellie material transaction hardening

2026-09-06. Scope: Omega's two quantity paths in
`npc/custom/varmundt_biosphere_quests.txt`, one purpose-scoped helper in that file,
and only the `Ellie#bio_d1_fusion` body in
`npc/custom/varmundt_biosphere_depth.txt`. This is not the equipment Ellie,
any crown service, or Abyss `L_Convert`. The parent owns shared callback gates,
older regression scope guards, integration and deployment.

The complete 41 recipes, 56 material definitions, price/access differences and
original source-level findings are enumerated in
[the follow-up audit](biosphere_material_service_followup_audit.md).
That source-only receipt is historical and is not a native run receipt.

## Runtime change

All original menus, recipe constants, output IDs, debit order, prices and success
messages remain unchanged. The three callers cap their computed maximum at
30000 and require `input` status zero. Invalid submissions are refused, not
silently converted at the clamped quantity. The existing affordability minimum
is retained: 100000z recipes cannot exceed 21474 outputs under native MAX_ZENY.

`F_BiosphereMaterialCommit` is deliberately limited to these three callers.
Before quantity arithmetic it validates argument count, the Omega/Ellie access
selector, positive output quantity <=30000, allowed material counts, positive
representable Etc IDs, allowed per-unit prices and needs, no duplicate inputs,
and no input equal to output. Exact callsite recipes and the current 56 plain
Etc definitions are additionally pinned by the required callback gate.

After the caller's final numeric input suspension, the helper:

1. Requires the actual service map `ba_in01` and the original per-NPC access:
   Omega BaseLevel >=240 only; Ellie BaseLevel >=250 and `ep17_2_main >=33`.
   It adds no reputation requirement and no story requirement to Omega.
2. Checks current Zeny and every material's complete selected total before
   taking anything. Failure returns false and the caller closes without its
   success message.
3. Preserves conservative pre-payment weight using the actual output weight.
   Seven fragments and sixteen Depth energies/crystals have native Weight 1;
   the other 33 material identities have Weight 10. No released input weight or
   inventory cells are credited to the preflight.
4. Matches native plain output capacity, then performs each original material
   debit in chunks <=30000, the original single Zeny deduction, and one getitem.
   There is no dialogue suspension between final preflight and commit.

There are no compensating grants, refunds, new prices, metadata transfers,
equipment mutations, or changed acquisition routes.

## Exact capacity and validity boundary

The helper uses the actual `getinventoryslots()` capacity and rejects values
below 1 or above exported `MAX_INVENTORY`. It scans `getinventorylist` in native
index order. A compatible plain output has the same ID, bound 0, expiry 0, UID
string `"0"` and all four cards zero. The first compatible stack is terminal:
overflow or an index outside allowed slots cannot be bypassed by a later stack
or an empty cell. Without a compatible stack, an accessible empty cell is needed.

Identify, refine, attribute, enchant grade, random options and favorite are not
added to stack matching because actual `pc_additem` ignores them. Existing
metadata in retained stacks must remain byte-identical, and a new plain output
must not inherit anything from a consumed input cell.

This relies on valid native inventory entries: populated cells have positive
amounts and coherent item-data pointers. `getinventorylist` omits zero-amount
entries while native name-ID occupancy can still see such a malformed cell.
The test does not claim malformed inventory is reachable or that this script
repairs arbitrary corrupted character storage. Oversized allowed-slot count is
defensively refused but is not represented as an ordinary player capability.

## Callback closure and real VM design

The separate `biosphere_material_callback_audit.validate(ROOT)` is mandatory
before and after a final native proof and itself requires the broader reviewed
engine/database/enabled-NPC gate. Its unvalidated collector is usable only for
explicit compile-only preparation, never a successful transaction receipt.

Unlike the prior Abyss map, ba_in01 has 33 literal conditions on 19 enabled NPCs.
The harness extracts the exact original registration statements, preserves
per-owner order, and runs the real `questinfo` builtin to populate the native
map collection. It initializes the attached player's actual `qi_display` with
`pc_show_questinfo_reinit`; an incorrectly sized display list would otherwise
skip the native loop. Reinitialization only allocates display entries: the
separate `pc_show_questinfo` call evaluates conditions. Both behaviors are
asserted, including the native wrong-size early return. First-true
short-circuiting is preserved.

`pc_show_questinfo` and `achievement_update_objective` are not replaced. A
recording wrapper around `achievement_check_condition` delegates to its real
implementation and checks that nested execution restores the previous script
and RID and changes no inventory, Zeny, weight or quest-log data. Source-derived
true/false states exercise every condition individually, plus native full-map
traversal. Seven actual sell-zero Get_Item conditions execute; no achievement
completion or reward claim is fabricated.

The runner feeds all 38 ordered referenced quest rows through the actual native
quest parser, retaining 36 effective records including two title-only overrides.
It provides exact source-backed monster ID/Aegis bindings solely for quest
target/drop lookup, not full monster initialization or combat. The three
ancillary countitem identities are loaded from actual item definitions alongside
the 56 materials.

## Regression structure and explicit doubles

The new driver is appended to the existing crown fixture's tracked boundary
helpers without editing that older source. The runner compiles production
`script.cpp`, `pc.cpp`, `itemdb.cpp`, `clif.cpp`, `achievement.cpp`, `quest.cpp`
and `malloc.cpp` with PACKETVER20260219 and ASan/UBSan. Other server support
objects are linked as existing support; this is not full-engine sanitizer
coverage. Header/source checks prevent stale-source object reuse.

Explicit doubles cover world ID indices, packet delivery, item/Zeny logging,
registry persistence and queued-world event delivery. The transport double for
weight notification stops Weight50/Weight90 execution; the mandatory current
source/data closure, not a simulated status callback, carries that boundary.
No secure timer or client input packet parser is claimed. Cancellation invokes
actual `pc_close_npc(...,2)` while the real script waits for numeric input.

Large-quantity fixtures use multiple positive stacks with distinct UID keys and
adequate synthetic carrying capacity. They do not claim ordinary acquisition
or that every large batch is available to a live player. Kernel socket/connect/
bind/listen denial prevents contacting a server, SQL service or client.

The original negative child reconstructs each entire pre-change NPC file by
inverting only the reviewed modifications, then checks its full original
SHA-256 before use. No Git command or untracked sibling snapshot is needed on a
clean clone. CRLF-to-LF normalization is an in-memory test view; actual raw
source hashes are retained and checked unchanged after execution.

Run from repository root in WSL:

```sh
python3 -B tools/ci/biosphere_material_transaction_test.py \
  --native-build-dir ../biosphere-material-native-proof-final-20260906
```

`--prepare-only` constructs and compiles fixtures without accepting the callback
gate or executing the native suite; it explicitly cannot produce a PASS receipt.
`--reuse-build` requires the existing compiled-source/header inventory to match.
Default per-object reuse requires the exact source, headers and compiler flags;
a new final build directory produces fresh scoped objects.

The runner requires an explicit clean allocator teardown and rejects sanitizer,
allocator and unexpected error/warning diagnostics even at exit zero. Its
test-only ShowError recorder retains the severity prefix, including errors
outside a case or during teardown. The old
failure child permits only its counted expected diagnostics. Seven synthetic
zero-exit failures verify this output guard.

The candidate matrix covers all 41 actual menu routes and these boundaries:

| Cases | Coverage |
| ---: | --- |
| 525 | Valid quantities across chunk, signed-narrowing and output/currency boundaries |
| 205 | Invalid input statuses and above-cap submissions |
| 94 | Current Zeny and each individual material becoming insufficient during input |
| 106 | Changed level, map, and Ellie's story access |
| 41 | Actual forced close during numeric input |
| 82 | Conservative output-weight exact fit and one native unit short |
| 123 | Zero/oversized slot fields and no accessible pre-payment empty cell |
| 285 | Bound, UID, card, rental, ignored stack metadata, terminal-stack order, out-of-range slots, and bound inputs |
| 8 | Menu Cancel, Leave and Escape paths |
| 34 | All 33 source-derived condition true/false states and display initialization/skip behavior |
| 13 | Direct helper malformed-domain rejection without a debit |

The 100 genuine-original controls independently reproduce 41 stale-Zeny material
losses, 12 partially consumed water recipes, 41 negative signed-narrowing debits
that still produce output, three paid-but-failed incompatible-stack outputs,
and three invalid-zero submissions silently buying one output. These are
intentional original-child failures, not permitted candidate behavior.

## Verification status

PASS on 2026-09-06 in `../biosphere-material-native-proof-final-20260906`:

- Candidate: **1516 cases, 788683 assertions, 89424 checked nested conditions**.
- Genuine original failure controls: **100 cases, 25787 assertions, 4653 checked
  nested conditions**.
- Seven synthetic zero-exit diagnostic controls pass. Candidate stderr is empty;
  both native children explicitly report `Memory manager: No memory leaks found.`
  ASan/UBSan report no issue; only the original child's exactly counted expected
  diagnostics are accepted.
- The mandatory callback gate passes before and after, with an identical complete
  manifest. Every raw runtime file and every tracked compiled source/header is
  unchanged during the proof.

All seven scoped production units were compiled fresh in this final build
directory. The final test-only severity-recorder adjustment rebuilt the fixture
and relinked against those exact-source, exact-header sanitizer objects; it did
not change or recompile production source under a different scope. The retained
receipt includes source/header, fixture, output and executable hashes.

| Retained artifact | SHA-256 |
| --- | --- |
| `receipt.json` | `09e4fc3ce1699fb580a60c9dd9c75f6b0f18a73661dfe604c4d18bd9974832a7` |
| `biosphere_material_transaction_test` | `5ad0b313e9ae3d9903b56922617f96f6cf6d54bf808723f9bdde96426ec547e7` |
| Complete callback manifest (canonical sorted JSON) | `6c5bfda2ee7f1415d9d66a84fbbfdc052965869f9e4adb4e4ac30ac8f0536c07` |

This is an isolated native proof, not a live-player/client demonstration,
full-engine sanitizer run, deployment receipt or Git commit. The explicit world,
transport, persistence, status-notification and valid-inventory boundaries above
remain in force. No live player data or server process was accessed by this task.

Frozen runtime SHA-256 (both sources are currently LF, raw equals normalized):

| Source | Original | Candidate |
| --- | --- | --- |
| `varmundt_biosphere_quests.txt` | `787f6ff54a4c43a60510e4a606dc5bd33a80c502853f113ad6756e257ac5aa5a` | `edec07ec166f5f8ae4a7e90ed52e38a5f9d76cf5781d139754a4ebd9a1ba0a7a` |
| `varmundt_biosphere_depth.txt` | `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28` | `6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988` |
