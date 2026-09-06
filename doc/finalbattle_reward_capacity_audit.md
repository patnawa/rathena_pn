# Final Battle crystal batch repair — 2026-09-06

## Scope and runtime identity

This implements the two Normal/Hard Giant Serpent Crystal capacity checks in
`npc/custom/episode21/FinalBattle.txt`. It does not change the story/daily entrance
rewards, party entitlement, acquisition economy, generic `checkweight` commands,
engine, item definitions, or historical inventory compensation.

Current frozen raw SHA-256:
`3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1`.
Removing the new function and replacing exactly its two calls with the original
`checkweight2` expression reconstructs the entire original source, SHA-256
`7f8dc969e03879ec90bc11f01e553a5556beb2d931e155061523ee91cd6026cd`.
The test performs this exact-byte comparison without invoking Git. Consequently
the saved-roll construction, thresholds, masks, diamond rolls, reward order,
daily key/reset, claim policy, and all unrelated NPC bodies remain unchanged.

The preceding source-only finding and all 21 output properties are retained in
`doc/finalbattle_plain_reward_capacity_audit.md`. Its statement that no repair or
native reproduction existed describes that earlier checkpoint, not this batch.

## Exact capacity contract

`EP21_FB_CheckPlainBatch` takes real array references plus count, copies the given
segments into local arrays, and never modifies caller arrays. It accepts only the
21 reviewed identities, counts 0–21, and positive quantities up to 30000. Duplicate
IDs are aggregated before checking capacity; an aggregate above 30000 is rejected.
The legitimate empty Normal result succeeds before any weight or slot check.

This is the two fixed callers' API: their arrays are named `.@reward_item` and
`.@reward_amount`, distinct from the helper's local destination names. Native
`copyarray`'s same-variable-ID/index no-op test does not compare scope pointers;
an arbitrary caller reusing a destination variable name is outside this scoped
contract. The fixture uses the actual caller names, verifies unchanged contents,
and additionally exercises nonzero source offsets. No generic array-engine change
is hidden in this repair.

For nonempty batches it sums actual item weights against current `Weight` and
`MaxWeight`, reads native `getinventoryslots()`, rejects values outside
1–`MAX_INVENTORY`, and snapshots `getinventorylist` once. The valid-inventory
premise is native nonempty rows with positive amounts, coherent item metadata,
and the builtin's ascending physical-index enumeration. It is not a repair for
arbitrarily corrupt inventory records.

One ascending inventory pass counts **all** occupied allowed indices and records
the first compatible row per output. Compatibility is exactly ID, bound zero,
expiry zero, UID string `"0"`, and four zero cards. A first compatible row outside
the allowed limit or exceeding 30000 after the reward is terminal: it must not
fall back to a later compatible stack or a fresh cell. Every unmatched distinct
output reserves one fresh slot cumulatively. Bound-first/plain-later and
incompatible-only-with-free-room inventories remain supported.

Native matching ignores refine, identify, attribute, grade, options, favorite and
equip metadata for these stackable outputs. The helper deliberately does not add
those filters. Current item definitions have no GUID generation, autoequip,
custom inventory stack cap, equipment location, or equip/unequip callbacks.
103512 is a Usable container whose use script runs only when used, not received.

Native `inarray` handles the at-most-21-ID lookup; its empty/-1 result is checked
in a separate branch before indexing the matched array. The script does not rely
on short-circuit `||` evaluation to protect a negative array index.

The first draft's nested scripted output×inventory scan could exhaust the normal
2048-jump budget. It was replaced before deployment by the single inventory pass;
aggregation also uses native `inarray`. No `freeloop` or raised script limit is
introduced. The fixture reads actual `conf/script_athena.conf` and its import,
asserting 655360 commands and 2048 jumps, and tests full 200-cell inventories with
all 21 compatible output cells only at the end.

## Why the original claim can be lost

Generic `pc_checkadditem` receives ID/amount, whereas `pc_additem` receives complete
item identity. A native-valid account-bound-only stack in a full allowed inventory
can pass `checkweight2` for a plain reward, then fail actual `getitem`. The builtin
failure logs an error but does not end the VM: the original script consumes the
daily marker and clears the saved roll despite no delivery. Hard can deliver its
first reward and fail a later one. A full first compatible plain stack can also
fail despite an earlier nonfull bound row and unused inventory cells.

These are native-valid inventory counterexamples, not proof of an ordinary
player-only acquisition route for such variants or a reproduced live loss. The
earlier read-only item-group audit found no bound/rental/named/UID producer for
these output identities. Plain-only compatible inventories remain supported.
Changing generic `checkweight`/`checkweight2` to assume plain items is inappropriate
for existing `getitembound` call sites; the new function is explicitly scoped.

## Synchronous callback evidence

The preflight is valid only while the grant loop has no intermediate suspension
or capacity/inventory mutation. Receiving these outputs follows native
`getitem → pc_additem → weight notification / achievement update / quest-info`.

- Current Weight50/Weight90 definitions have no Script or CalcFlags. Their
  start/end effects do not mutate inventory or recalculate equipment bonuses.
- Golden Diamond's actual Sell value is 300000. Its grant can complete all seven
  current Get_Item conditions, not merely take their false branches. Completion
  recalculates achievement level and can evaluate the twenty Goal_Achieve
  conditions. Their current expressions are pure value thresholds. Conditionless
  dependent-group completion does not introduce arbitrary condition execution.
  Price derivation occurs in native `ItemDatabase::loadingFinished()`, not merely
  `parseBodyNode`; the fixture executes both, including normal dummy-item setup.
- Achievement reward item/scripts run on the later reward-claim path, not upon
  automatic completion. The native fixture loads the actual Gift_Box definition
  to parse their unchanged metadata; no Gift_Box is automatically granted.
- The enabled graph has 14 NPC declarations on `1@ep21b`, all in FinalBattle, with
  no quest-info registration in those bodies. The daily entrance's quest-info is
  on another map. Current runtime duplication/map-setting producers are absent;
  all 33 enabled `unitwarp` statements were included in the reviewed closure.
  Native `pc_show_questinfo` returns on the empty instance-map `qi_npc` vector.

`tools/ci/finalbattle_reward_callback_audit.py` unconditionally requires the broad
callback gate, then pins the 21 effective item records, Weight50/Weight90, full
ordered achievement/import graph (361 records), all 20 achievement levels/imports,
27 relevant conditions, complete encounter/daily-helper source identity,
instance-map membership, enabled-source traversal counts, and two script-config
files. The broad gate supplies exact current engine/database/include coverage.
Default validation is fail-closed; the separately reviewed explicit live profile
is supported, never inferred from failure. Hash pins retain a manual semantic
review; keyword scans alone are not proof of arbitrary script purity.

## Reproducible checks

Run in the repository's WSL Ubuntu environment:

```sh
python3 -B tools/ci/finalbattle_reward_callback_audit.py --negative-controls
python3 -B tools/ci/finalbattle_reward_capacity_test.py --native-build-dir ../finalbattle-reward-native-20260906
```

The native runner freshly compiles `pc.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp`,
`achievement.cpp`, and `malloc.cpp` with ASan/UBSan and actual parser/VM code.
Retained objects require exact source/header/compiler-flag/object hashes; retained
binary reuse additionally requires unchanged support objects and binary hash.
Artifacts, source fixtures and receipts are outside the repository. Network
socket/connect/bind/listen syscalls are kernel-denied before the fixture runs.

Coverage includes all 21 outputs, plain/bound/UID/rental/each-card/native-ignored
metadata, terminal first-compatible limits, reduced slots, sparse cells,
duplicate aggregation, invalid arguments, nonzero array-reference offsets,
exact weight/amount boundaries, full-inventory script-budget cases, empty Normal,
saved-roll rejection/retry, actual RNG saved-roll reuse, successful-claim retry,
premature stage rejection, and actual Golden Diamond achievement completion and
all Goal_Achieve condition thresholds. Original-source failures run separately.

Final result: **PASS, 566 candidate cases / 26,174 assertions**, plus a separate
original-source process with **3 confirmed loss cases / 188 assertions**. Both
finished with clean ASan/UBSan and exactly one clean allocator teardown. The
original process requires exactly three expected `getitem` errors and three
matching VM warnings; any additional warning/error or missing success/teardown
marker fails validation. The candidate permits no such diagnostics.

All **15 dependency negatives** passed: mutating Get_Item and Goal_Achieve
conditions, missing/new achievement imports, missing level import, output GUID,
stack cap and equip callback, new output import, Weight50 callback, instance
quest-info, new enabled instance NPC, changed reward probability, raised jump
budget, and new script-config import. **11 zero-exit output negatives** proved
the runner rejects missing success/allocator evidence and unexpected error,
warning, sanitizer, loop-limit, or allocator diagnostics, including an otherwise
expected original-failure process.

The final receipt is
`../finalbattle-reward-native-20260906/receipt.json`; separate stdout/stderr files,
exact source fixtures, build/source/header/support-object manifests, and the
executable accompany it. All required source/data gates were revalidated after
native execution. Runtime remains frozen at the identity above.

## Explicit boundaries

Registry persistence, player/world lookup, network packets and logging use
explicit test doubles. Local arrays, helper, grant loops, native capacity/grants,
achievement condition detach/reattach, completion and level calculation are real.
The `clif_updatestatus` transport double stops the weight-status side branch;
its current safety is established by the mandatory source/data gate, not falsely
claimed as execution in this fixture. The empty quest-info world is independently
source-pinned, not a full instance startup or connected-client playthrough.

This is current-content, valid-inventory, synchronous claim safety. It is not
process-crash atomicity across inventory/registry persistence, live persisted
text inspection, deployed-binary equivalence, arbitrary future callback safety,
or automatic rollback/compensation. Deployment and live-state evidence belong to
the root agent's separate reviewed receipt.

## Final checkout-portability and compatibility check

The runner's frozen-source acceptance now uses LF-normalized content while its
receipt separately records raw source identity. Two LF/CRLF positives and six
non-newline mutations passed their controls. Exact-source/header/flag-checked
production objects were retained and the binaries relinked; the complete gated
native suite passed again with unchanged counts. This follow-up is not a second
all-units-fresh build. Runner SHA256:
`66d288260763e502b978a5a128a7f3af9d3c0b3306773338aa499a3b7eea192c`.
Updated receipt SHA256:
`13f2117bf0d9dbaae74a7c25e5b968ee21ecbe9d84bee645f955fbd0405cdb32`.

Eight existing party regressions and the strict integrity audit passed. Existing
encounter tests do not load these FinalBattle crystal bodies; no helper
registration adaptation was needed and unrelated encounter binaries were not
rebuilt for this claim. The new native suite supplies the actual crystal proof.
