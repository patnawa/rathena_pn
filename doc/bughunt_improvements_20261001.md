# Bug hunt and improvements — 1 October 2026

Requested scope: all five recommendations from the October 1 review. Baseline
commit: `091107357`. This ledger records the implemented fixes, their current-
source evidence, and the remaining design limits. Existing untracked EM analysis
and screenshots are unrelated and preserved.

| Area | Required outcome | Current evidence |
| --- | --- | --- |
| 1. Dynamic rewards/economy | Inventory unresolved grants; trace exchange/reward/reset cycles with costs and eligibility; reproduce and fix confirmed defects; validate effective output definitions | Lexical and typed audits cover dynamic grants; the combined recipe/vendor model has an independently verified no-growth certificate over 7,424 actions and 2,274 resources; the hidden Eden item was reproduced and fixed |
| 2. Legacy socketing/crafting | Exercise real NPC transactions across dialogue, capacity, metadata and disconnect boundaries; fix reproduced loss/duplication | Three socket paths recheck payment, retain equipment binding, select permanent inputs, and refuse transaction-lock races; all 111 recipes and 11,437 native cases pass |
| 3. Combat | Extend final damage, timing, autocast and equip-transition coverage; remove dependence on missing historical Druid fixtures | Reproducible crown fixtures remove the external dependency; 89,733 crown executions plus the expanded final-damage/timing/autocast/equip pipeline pass under sanitizers |
| 4. Post-commit progression | Classify callbacks; reproduce crash/retry effects on durable progression; implement idempotent recovery where required | Achievement snapshots, shop progression, rewards, item/taming acquisition, and logout now use bounded fail-closed state plus durable SQL/token acknowledgements; native, real-SQL, restart, and live crash proofs pass |
| 5. Mixed gameplay performance | Measure combat/instance/save/purchase workload, contention, latency and receipt growth; improve demonstrated bottlenecks without weakening transaction guarantees | Mixed world/shop runs reconcile exactly; lossless receipts reduce payload storage 98.915%; the staggered four-buyer control completes all 1,440 attempts while retaining the existing concurrency fence |

The final checkpoint binds these results to the current source, clean binaries,
focused regressions, real MariaDB tests, and a live crash/retry realm. The final
deployment checkpoint below records the subsequent Docker rollout and disk
cleanup; earlier no-deployment statements describe historical checkpoints.
User-deferred rendered client journeys remain separate.

## Environment

- Windows Python lacks PyYAML; created a task-local virtual environment at
  `OPS/bughunt-20261001/venv` with PyYAML 6.0.3.
- WSL cannot start (`HCS_E_HYPERV_NOT_INSTALLED`). Investigating the established
  isolated Linux validation host rather than changing machine virtualization.
- Initial portable commands stopped at missing dependency before exercising
  project logic; these are environment failures, not game defects.

## First evidence checkpoint

No changes have been committed, published or deployed. The five-area objective
remains active. `Server-Development/README.md` and `NEXT-JOBS.md` now point to the
authoritative September 30 deployment checkpoint instead of presenting completed
pet/barter work as the next task.

### Acquisition tooling

The actual acquisition test reproduced a false obtainable item from
`mes "example: getitem 5,1;"` and missed bound/rental/expression grants. A shared
lexical scanner now skips strings/comments, retains complete nested expressions,
recognizes bound and rental command variants, and inventories unresolved grants
inside item and achievement scripts as well as NPCs. Scope-local candidate
analysis uses the same scanner. Regression checks cover literal, dynamic,
parenthesized, nested, bound/rental, dialogue and item/achievement paths.

Fresh source traversal: 926 active NPC files; 6,919 traced item IDs; 291 unresolved
grant calls/lines; six cataloged lines. Scope-local NPC analysis finds 3,037 grant
sites, 279 dynamic sites and 103 sites with unknown dependencies. These are
different scopes and are not interchangeable counts. Candidate sets are
overapproximations, not proof of acquisition or a profitable cycle.

The acquisition run used the September 30 `rune-client-candidate-v4` metadata and
resource inventory, with their hashes recorded in the report. It found no missing
metadata among traced IDs against that snapshot; it is not a fresh installed
client export or visual acceptance. Detailed evidence is in
`OPS/bughunt-20261001/acquisition.json` and `dynamic-rewards.json`.

### Socket transaction regression

`Func_Socket` and `Func_Socket2` checked balance/materials, suspended at `next`,
then deleted inputs and charged without checking again. Native VM probes passed
the ordinary path, then failed with `script_set_reg: failed to set param 'Zeny'
to -200000` after changing the balance during that suspension. Both now recheck
all advertised costs before the first deletion.

Command in the isolated candidate:
`python3 tools/ci/socket_transaction_test.py --build-dir /evidence/sockets-bound`.
Result: 104 cases for each function, 4,990 assertions total, no script errors or
reported memory leaks. All 100 random buckets preserve existing success/failure
boundaries and exact fees; additional cases cover changed Zeny, materials,
equipment and cancellation. The fixture executes production script/inventory
code with explicit registry, packet and visual-world doubles and kernel-denied
networking. Existing map link objects are used; this is not a fresh server build,
SQL crash test or end-to-end disconnect proof.

Red/green logs and bound input receipts are in `OPS/bughunt-20261001`; originals
remain under `/app/pn-bughunt-20261001`. Both acquisition and socket tests are
registered in `release_checks.py`; the full gate has not yet been rerun.

### Crown fixture and native coverage

The clean isolated runner reproduced a missing external
`/audit-item-aliases-20260906/.../itemdbnametbl.lub` dependency. The locally retained
original matches its pre-existing pinned SHA-256. Extracted its 23 independent
aliases into `tools/ci/fixtures/druid_crown_aliases.json`, pinned the subset hash,
and verified it against the original via `--alias-source`. No aliases were
inferred from expected item IDs. Clean checkouts now use that fixture. The runner
also links the available Zstandard SONAME instead of requiring a missing
development symlink.

Final isolated command:
`python3 tools/ci/druid_missing_crowns_test.py --require-import --native-build-dir /evidence/crowns-native2`.
It passed 23 items, 11 combos, 89,733 executions and 8,169,504 assertions, with
ASan/UBSan and kernel-denied networking. Twelve recorded source inputs match the
current local files after canonical LF normalization. Six relevant production
translation units were freshly compiled; unrelated support objects are reused.
This restores the existing numerical/bonus coverage, not every final skill
damage or real packet-driven equip transition.

## Next work, without reducing the five-area scope

1. Trace the fresh dynamic inventory into cost/eligibility/reset-aware conversion
   paths; validate suspicious cycles through the real VM. The 291 unresolved
   calls are still unknowns, not confirmed defects.
2. Extend socket fixtures to metadata, capacity, the separate Hat of the Sun God
   branch, and disconnect/persistence. Expand into the remaining legacy crafting
   services. The current stale-check repair does not claim those guarantees.
3. Add final damage, cooldown/autocast and packet equip-transition coverage using
   the restored reproducible combat environment.
4. Build a crash-boundary probe for `intif_parse_ShopCommitted` through
   `PcItemDeliveryScope` and `pn_shop_committed_effects`. Current source invokes
   acquisition achievements and market experience after durable assets; classify
   persistence and replay before choosing a recovery design. Default `shop_exp`
   is zero; no claim of live experience loss is made.
5. Create a separate labelled mixed-load realm: connected combat/instances,
   saving and purchases together; compare idle and loaded latency, refusals and
   receipt growth. Preserve the existing user-handoff rendered realm. Host free
   space was 4.8 GiB after native fixture builds, so size new fixtures first.

All task containers observed for this checkpoint are terminal. Failed fixture
attempts are retained as failures (initial socket world-effect boundary and
missing Zstandard linker alias), separately from passing regression evidence.

Final focused Linux checks all passed: acquisition scanner, dynamic reward
candidate analyzer, typed reward catalog, release-runner regressions and retired
token reward probe. Results/log hashes are in
`OPS/bughunt-20261001/portable/report.json`. The broad Windows reward discovery
attempt had passed the Python tests but failed to import the retired-token
native runner because Windows has no `g++`; its Linux rerun above passed.
`git diff --check` passed. These focused checks do not replace the outstanding
full release gate or the remaining audits.

## Second evidence checkpoint — achievement persistence and recipe costs

The previous goal turn was progress: its worktree changes and retained tests
were revalidated before this continuation. No production changes or publication
have occurred. This checkpoint is additional progress, not completion of items
1–5.

### Achievement snapshot failure

An exact-handler regression reproduced this sequence: the first achievement
UPDATE fails, the handler continues its old-row deletion loop, and successful
deletions overwrite the failure flag before sending a success ACK. Existing
achievement rows can disappear. The map-side negative ACK handler independently
failed to mark the snapshot dirty again, so a failed save could go unretried.

The character writer now validates snapshot identities, uses a reconnect-disabled
SQL transaction, locks the character's achievement rows/gaps, and verifies InnoDB
while holding the table metadata lock. Read, update, insert, delete and commit
errors cannot report success or leave a partial snapshot. Invalid lengths and
duplicate IDs are rejected. The map marks failed saves dirty for retry and never
lets an older success ACK clear newer dirty progress. Its unaligned character-ID
read is replaced with `memcpy`, as exposed by UBSan in the handler fixture.

New installations use InnoDB for `achievement`. Existing installations require
`sql-files/upgrades/upgrade_20261001_achievement_atomicity.sql` before rollout;
custom configured achievement tables need the same conversion. This migration
has only run against disposable SQL. The writer deliberately refuses a
nontransactional table instead of silently claiming atomicity.

Verification:

- `achievement_persistence_test.py`: exact save/ACK handlers, eight injected
  failure boundaries, malformed records and dirty-state retry, ASan/UBSan.
- `achievement_sql_test.py`: freshly compiled production character writer,
  real MariaDB triggers failing the first/second UPDATE, INSERT and DELETE;
  malformed read schema; nontransactional table refusal; exact successful and
  repeated snapshots; other-character isolation; empty-snapshot deletion.
- Two simultaneous native writers both completed with one whole final snapshot,
  never a mixture. An observed sleeping trigger placed the database kill after
  an earlier update but before commit. Restart preserved the old snapshot, and
  the retry succeeded.
- The first database-image attempt failed at TLS connection setup. The final
  test uses the established pinned MariaDB image and retains that earlier failure
  separately. Final SQL evidence is `/app/pn-bughunt-20261001/achievement-sql-final`.
- The focused handler test is registered in the release gate; the actual SQL
  fault/concurrency/restart runner is added to the provisioned CI SQL job.
- Final `make -j2 char map` completed successfully in the isolated candidate.
  Collected SQL logs, focused sanitizer log, build log and source binding summary
  are under `OPS/bughunt-20261001/achievement-final/`. The collection verified
  1,481 available source/header inputs against the SQL receipt, accepting exact
  bytes or CRLF-to-LF normalization. This is not a full release-gate result.

This repairs a progression-persistence prerequisite. It does **not** recover a
callback that never ran after a purchase committed, guarantee offline retries
after player teardown, or add session ownership to the old achievement wire
protocol. Those remaining seams still require the item-4 crash/replay work;
passing this save suite cannot close that row.

### Cost-aware recipe screen

The Rune Seal catalog previously described its input only as prose and emitted
one costless-looking record. A failing regression required all 44 real card costs.
The generated catalog now has 2,724 recipes, still 342 output IDs, with 44 explicit
one-card-to-one-seal exchanges. The four catalog tests pass.

`tools/audit_economy_cycles.py` conservatively uses each possible random output's
maximum. It repeatedly removes a recipe whose net consumed resource cannot be
replenished by any remaining modeled recipe. The recorded elimination certificate
removed all 2,552 repeatable recipes; 172 finite account/tier rewards are separately
excluded. Thus this **closed recipe model** has no self-funding combination, even
at optimistic random yields. Six independent tests cover positive cycles, hidden
material/Zeny costs, one-time eligibility, rare/byproduct output, lossy cycles and
unpriced grants. Retained cycles would be reported as runtime-review candidates,
not proven exploits.

Evidence: `OPS/bughunt-20261001/economy-cycles.json`, with catalog and source hashes.
This is not a whole-economy safety claim: vendor buy/sell paths, reset transitions,
uncataloged grants and player reachability are still outside that closed model.
Continue expanding those inputs and reproduce any candidate in the real VM.

## Third evidence checkpoint — separate socket path and recipe matrix

The preceding goal turn made progress: it collected source-bound SQL/build
evidence and updated this ledger. This turn reproduced another real NPC defect
and expanded the socket regression's production-recipe coverage.

The Hat of the Sun God branch in `socket_enchant2.txt` does not call either shared
socket function. Its last `next` occurred after checking the 200,000,000 Zeny,
two Gold and hat requirements. Running the entire actual Leablem NPC body and
changing the balance at that pause caused item deletion followed by
`script_set_reg: failed to set param 'Zeny' to -200000000`.

Added a fresh balance/material/hat check immediately after that suspension and
before the random roll or any deletion. The regression walks the real menu path,
checks all 100 random buckets (exactly 90 successes), confirms the exact fee,
tests each of the three resources changing at the final pause, and checks
cancellation leaves the whole inventory and balance unchanged.

The runner now loads every literal active recipe call from both source files,
excluding comments, and loads the corresponding effective item definitions.
For each of 111 configured recipes (73 Seyablem, 38 Leablem), all 100 random
outcomes exercise the actual shared script function and linked inventory code.
Assertions check target/material removal, output counts and exact Zeny charges.
These checks validate implementation against declared recipe parameters; they
do not independently establish that the parameters match external official data.

Final run: 11,412 cases, 333,117 assertions, no script errors or reported memory
leaks. Kernel networking is denied; registry, packet, visual world and equipment
callbacks remain the existing fixture doubles. This is not an end-to-end client,
equipment transition, sanitizer, disconnect or SQL persistence claim.

The intermediate recipe-runner failures were harness setup errors: a commented
placeholder was parsed, a tools import path was missing, and a dynamic rapidyaml
key needed explicit conversion. These are retained separately from the actual
NPC red case. Final container `pn-bughunt-socket-recipes4-20261001` exited 0.
Source-bound receipt, both per-function logs and the red/green logs are collected
under `OPS/bughunt-20261001/socket-final/`; six local source files were verified
against that receipt. The existing full release-gate socket entry runs this
expanded matrix. No production changes were made.

Still pending in area 2: exact item selection and bound/rental metadata behavior,
capacity changes, disconnect persistence, and remaining legacy crafting paths.
The broader five-area goal remains active.

## Fourth evidence checkpoint — binding and rental selection

The previous turn was progress: it fixed the separate Sun God stale-payment
path and exercised every declared recipe. This turn extended the inventory
invariants rather than interpreting that matrix as metadata coverage.

Two failing real-VM checks established defects:

- Account-bound equipment was deleted and recreated as an unbound output.
  `socket-metadata-red` failed `socket output retains equipment binding`.
- With rental and permanent material stacks sharing an ID, `countitem` accepted
  the permanent quantity but plain `delitem` consumed the rental stack first.
  After the equipment selection/binding fix, `socket-metadata2` independently
  failed `rental materials untouched`.

`npc/other/Socket_Functions.txt`, loaded before both legacy NPC files by the
common script configuration, selects permanent inventory entries. It retains
the previous preference for ordinary unequipped/unrefined/uncarded equipment,
then the first remaining permanent copy. Callers capture the selected index and
binding after the final dialogue pause and remove that exact item. Successful
replacement uses `getitembound` with its binding. Required materials are removed
from permanent entries, including split stacks; deletion failures terminate the
attempt. The same helpers cover the separate Sun God branch. Advertised
destructive refine/card conversion semantics and chance/fee parameters remain.

Final native run: 11,437 cases and 333,930 assertions, no script errors or reported
memory leaks. New cases cover every nonzero binding type, rental/permanent target
and material mixtures, rental-only refusal, split materials, preserving a refined
alternative while selecting an ordinary differently bound copy, and a bank
transaction lock arriving at the final pause. Sun God additionally exercises
bound outputs and mixed rental/permanent hat/Gold with split permanent Gold.

One intermediate fixture attempt set a shop pending flag. The real VM correctly
deferred the script into its sleep queue, which the simple dialogue driver did
not resume; teardown reported the leftover queue node. That is retained as a
fixture-lifecycle failure, not asserted as a production memory leak. The final
lock-refusal case uses the bank lock, which reaches the inventory deletion guard
without the shop script scheduler. Shop deferral/recovery still needs its own
scheduler-aware test.

Evidence: `OPS/bughunt-20261001/socket-metadata/`, including both original failing
logs, final logs, input receipt, and eight locally verified source/config hashes.
Final container `pn-bughunt-socket-metadata4-20261001` exited 0. As before, these
are real script/inventory operations with packet, registry, equipment and visual
world doubles, not full server persistence or equipment callback coverage.
Capacity changes, unequip-script side effects, disconnect durability and other
crafting families remain open. Nothing was deployed.

The current isolated candidate also passed `release_checks.py --phase source`:
50 checks, including database YAML syntax, 48 focused regression runners, and
client asset compatibility. Container `pn-bughunt-source-gate-20261001` exited 0;
report and all per-check logs are collected under
`OPS/bughunt-20261001/source-gate/`. This phase does not run the full native test
set or map-server startup, and must not be presented as full release acceptance.

## Fifth evidence checkpoint — real mixed workload and storage cost

The previous turn made progress by fixing binding/rental selection and passing
the source gate. This turn provisioned a separate `pn-bughunt-mixed-20261001`
internal Docker network, fresh disposable SQL, and the current candidate. No host
ports were published. The existing rendered user-handoff realm and production
containers were not changed. Fresh login/char/map/web startup and handshake passed
without error diagnostics, including the new achievement-table migration and
socket helper import. The source binding is
`a14c3d284ad4e16a545b57774ce7f9dc7e1402850553e5b5adbe3aaa17e29395`.

`OPS/bughunt-20261001/stage_mixed_realm.py` derives the established realm builder
with recorded replacements for task paths, names, pinned image, labels and the
achievement migration. `mixed_workload.py` runs two protocol processes in the
game container's network namespace. Its guarded adapters change only the realm
names/import path and omit the artificial 75-second SQL stall from the shopping
driver. Both use the same candidate and captured runtime identity; source binding
is checked before and after. Game resources were capped at 1.5 CPUs / 3 GiB,
database at 1 CPU / 768 MiB, on a host shared with unrelated services.

The shopping process runs one buyer for 15 seconds and four synchronized buyers
for 360 seconds, one attempt per buyer per second. Simultaneously, the world
process enters the actual private Damage Lab instance, performs three real
60-second nonzero-damage attack runs, verifies persistent history after relog,
interrupts another run, and exercises full-inventory pet recovery, character
switching, and duplicate claims. Their measured process overlap was 310.97
seconds; all three world cases passed. This is protocol/server/SQL evidence,
not rendered acceptance or correctness of every combat formula.

Results:

- 375 total successful purchases; each buyer's exact wallet and inventory delta,
  stock decrement and successful receipt count reconciled in SQL.
- The four-buyer phase had 360 successes and 1,080 explicit busy refusals.
  Success latency p50/p95/p99/max was 8.87/11.43/16.23/23.68 ms. Refusal p99
  was 0.37 ms. Synchronized contention is deliberately adversarial; it is not
  evidence that ordinary staggered traffic is limited to one purchase/second.
- Worst **individual complete one-minute window** p99 was 25 ms timer lateness,
  5 ms dispatch, 23 ms shop acknowledgment and 24 ms shop completion. These
  are maxima of window percentiles, not pooled workload percentiles.
- Receipts grew by 378 rows and 22,662,612 payload bytes: 375 market purchases
  plus the three recovered pet grants. Every stored request is 59,954 bytes.
  Physical table allocation is recorded separately and excludes other DB logs.
- A read-only SQL `COMPRESS`/`UNCOMPRESS` probe across those actual receipts
  produced 109,058 bytes total (279–843 bytes each), with 378 byte-exact round
  trips. This demonstrates substantial lossless storage opportunity. No stored
  receipt was changed, and no codec has been implemented yet.

Evidence is collected locally under
`OPS/bughunt-20261001/mixed-load-1790834624892120927/`,
`world-proof-1790834625634349492/`, and `mixed-startup-report.json`.
The workload directory contains adapter hashes, process start/end records, 56
Docker-stat samples, all final server logs, SQL reconciliation, raw protocol
artifacts via the world directory, a metrics summary, and the compression probe.
Both completed task containers were stopped successfully with their data and
evidence retained. The original rendered realm and production remain untouched.

Next area-5 improvement: bound and version a lossless receipt codec while
preserving raw historical receipts, byte-exact replay identity and fail-closed
corruption handling. Add native and real-SQL retry/fault tests before repeating
the mixed workload and comparing payload size and latency. A staggered-arrival
control should distinguish synchronized lock contention from ordinary throughput.
No concurrency fence has been weakened; no production rollout is claimed.

## Sixth evidence checkpoint — lossless receipt encoding

The preceding goal turn was progress: it established the live mixed-load baseline
and measured receipt storage cost. The character-side receipt writer now uses
`shop_receipt_codec.hpp` to store a lossless zlib representation when it is smaller
than the immutable request. The wire protocol, replay key, request validation,
SQL transaction boundaries and concurrency fences are unchanged.

Encoding contract:

- Raw historical rows remain byte-exact comparisons when stored length equals
  the known request size. This takes precedence even if their prefix resembles
  the new magic. Incompressible new requests also remain raw.
- Compressed rows are strictly shorter and start with `PNRZ`, version byte 1,
  three reserved zero bytes, and the original size as little-endian uint32.
  The remaining bytes contain exactly one zlib stream, using best-speed encoding.
- Decoding requires a known version, exact trusted request size, stream end,
  exact output length and no trailing input, then compares every decoded byte.
  Its allocation is bounded by the trusted request size (maximum 1 MiB), never
  by a size taken from the stored blob. No digest substitutes for equality.
- Corrupt, truncated or unsupported receipts return the existing Retry outcome;
  they cannot acknowledge success or reapply an old asset snapshot.

No schema migration is required, and existing rows are not rewritten. This is
backward-compatible reading by the new writer/reader, **not** downgrade-compatible
reading by old character binaries: old code refuses a shorter compressed receipt.
Rollout must therefore coordinate character-server versions. A rollback to an old
reader requires verified offline decoding of newly compressed rows first; merely
replacing the binary is not a supported replay-compatible rollback. Nothing has
been deployed, and no historical production receipt has been changed.

The sanitizer codec runner covers 250 deterministic sparse/incompressible
requests, changed-byte identity, every truncation, single-bit mutations, unknown
versions, trailing data, historical magic collisions and a forged short header
whose stream expands beyond the trusted output buffer. The initial real SQL run
also passed new raw/encoded replay and corrupted-row preservation cases alongside
the existing stock, asset, fault, race, restart and crash tests. That first run
freshly compiled the included SQL implementation but reused the earlier linked
character wire handler; a rebuilt-handler run and post-change mixed workload
are required before claiming final acceptance of this improvement.

### Rebuilt handler and measured result

The codec sanitizer runner and `make -j2 char map` both completed successfully.
After that build, `receipt-codec-sql-final` passed using the current character
objects, including the real inter-server handler. Its main SQL run reported
17,725 checks; additional wire, concurrent buyer, database restart/kill, pet,
mail and point-asset cases also passed. Collected SQL evidence verifies 2,888
current local source inputs against the receipt and records the pinned toolchain
and database images. Evidence: `OPS/bughunt-20261001/receipt-codec-sql-final/`.

A fresh `pn-bughunt-codec-20261001` realm then passed all four-server startup
checks and repeated the same single/four-buyer and real combat/relog/recovery
workload. The source binding is
`22058cca41457a4276aca81645986b9d124ce90258b33a5503aaf1cfa06e9811`.
Both processes passed, overlapping for 310.96 seconds. All wallet, inventory,
stock and successful-receipt totals reconciled again.

| Measure | Raw baseline | Encoded candidate |
| --- | ---: | ---: |
| Successful purchases | 375 | 375 |
| Total new receipts, including pet claims | 378 | 378 |
| Stored payload bytes | 22,662,612 | 245,834 |
| Average bytes per receipt | 59,954 | 650.35 |
| Four-buyer success p99 | 16.23 ms | 13.35 ms |
| Four-buyer busy refusals | 1,080 | 1,080 |
| Worst complete-window timer p99 | 25 ms | 23 ms |

This is an exact **98.915% payload reduction** for equal receipt counts with all
replay information retained. Best-speed zlib intentionally differs from the
earlier SQL COMPRESS feasibility probe, which used a different compression level.
Latency figures are descriptive single-run observations on a shared host, not a
statistical speedup or capacity claim. Worst-window internal shop ACK p99 was
27 ms, versus 23 ms before; no broad latency improvement is claimed.

The synchronized four-buyer burst still favors some callers: post-change buyer
success counts were `[299,46,30,0]` (including the single-buyer phase). The fence
was not changed by compression. A staggered-arrival control remains necessary
before deciding whether/how to improve admission fairness; reporting only the
successful latency would conceal this limitation.

Evidence: `OPS/bughunt-20261001/codec-load-1790835697124426959/`,
`world-proof-1790835697856018455/`, `codec-startup-report.json`, and
`receipt-codec-comparison.json`. The comparison checks equal receipts/purchases,
all world cases, overlap duration and the measured storage reduction, and hashes
both input summaries. Both codec realm containers were stopped with evidence and
data retained. Production and the original rendered handoff realm remain untouched.

Finally, the updated source gate passed all 51 checks, including the newly
registered sanitizer codec runner. Report and logs are collected under
`OPS/bughunt-20261001/codec-source-gate/`. Full native release-gate acceptance
and the other open rows remain required before the overall five-area goal can
be considered complete.

## Seventh evidence checkpoint — arrival control and Eden market

The staggered control retained the encoded candidate, resource limits, purchase
cadence and overlapping world workload, changing only four-buyer start offsets
to 0/250/500/750 ms. All 1,440 four-buyer attempts succeeded with zero refusals;
including the single-buyer phase, totals were 1,455 purchases and 1,458 receipts.
Buyer successes were `[375,360,360,360]`. Four-buyer p99 was 12.27 ms, maximum
23.08 ms. The world workload passed all three cases with 309.97 seconds of
overlap, and SQL balances, items, stock and receipts reconciled exactly.

Stored payload totaled 949,657 bytes (651.34 bytes per receipt). Worst complete
window p99 was 25 ms for timers, 5 ms for dispatch, 13 ms for shop ACK and 22 ms
for completion. These remain single-run shared-host measurements, not capacity
or statistical performance guarantees. The earlier synchronized refusals do not
establish a one-purchase-per-second capacity limit; fail-fast admission still
provides no fairness guarantee for synchronized callers. No fence was weakened.

Evidence: `OPS/bughunt-20261001/stagger-load-1790836367290616104/`,
`world-proof-1790836368024552196/`, and `stagger-startup-report.json`.
The control used source binding
`22058cca41457a4276aca81645986b9d124ce90258b33a5503aaf1cfa06e9811`, before
the Eden source correction below. Both stagger realm containers were stopped
successfully after the probes; their evidence and data remain retained.

An exploratory static vendor screen found a malformed Eden market declaration:
`970:12000:20:7136:7000:20` silently hid item 7136 because the native parser
accepted the first three fields and advanced to the next comma. On the isolated
realm, a real market-open packet returned `[971,972,970,7135]`. Replacing the
separator with a comma and reloading the NPC produced
`[971,972,970,7136,7135]`, including item 7136 at 7,000 Zeny with stock 20.
Evidence: `OPS/bughunt-20261001/eden-market-red/` and `eden-market-green/`.
This packet probe occurred after the completed load run and is separate evidence
for the later NPC change.

`eden_market_catalog_test.py` now rejects malformed literal entries and checks
that the Eden replenishment targets exist. Its two tests pass and it is registered
in the source gate. Opening quantities and later replenishment quantities may
legitimately differ; equality is not asserted. The release-check runner's own
13 tests pass, and the previous complete source gate had 51 passing checks;
the expanded complete 52-check gate has not yet been run.

After correction, the exploratory vendor screen parses 3,372 offers, excludes
32 NoSell offers and finds no direct buy/resell profit candidates within its
static Zeny shop/market model. It models native effective price fallback/guard
behavior and conservative discount/overcharge rates. This does not cover dynamic
prices, recipe-plus-vendor combinations, resets or arbitrary callbacks, and is
not a whole-economy safety proof. Evidence: `OPS/bughunt-20261001/vendor-screen.json`.

Finally, `.gitignore` now explicitly includes `src/custom/shop_receipt_codec.hpp`;
the existing broad custom-source ignore would otherwise omit this required new
header from an ordinary add. No changes have been committed or deployed. The
five-area goal remains active, especially its durable callback, expanded combat
and broader crafting/economy cases.

## Eighth evidence checkpoint — durable progression and final validation

This final checkpoint supersedes the pending-status statements in the earlier,
chronological checkpoints.

The post-commit investigation reproduced the remaining achievement loss windows
and closed them without weakening the purchase transaction. Achievement loading,
mutation, rewards, shop callbacks, item acquisition, taming and logout now share
bounded, versioned, fail-closed state and explicit acknowledgements.

The character writer preserves durable achievement IDs unknown to the current map
database and merges counters, completion and reward state monotonically. The map
records how many unknown rows were omitted and reserves their wire capacity before
accepting new progress. Both sides enforce the same maximum row count derived from
the largest protocol header. Oversize loads, snapshots, deferred-event queues and
malformed dynamic packet lengths disconnect or refuse the operation instead of
silently truncating state. Script mutations require a loaded snapshot, cannot
remove persisted rows or reduce counters, and are refused while an item/taming
capture or reward is unresolved.

Shop progression companions now contain only rows changed by planned success
callbacks. Their immutable before/after images are part of the durable receipt
identity. Character SQL locks the current rows, accepts only the recorded baseline
or a strict monotonic ancestor, merges opaque rows, and commits the purchase,
progression and receipt together. A newer or divergent touched row receives a
durable terminal `ProgressionStale` receipt. Unchanged rows compacted out of the
frame can advance concurrently and remain untouched. Staged companions are bound
to their authenticated map connection, bounded, and purged when that connection
closes.

Successful item consumption and taming now capture their Zeny, item-delivery and
taming achievement events into the same prepared snapshot. The live state is
suppressed only after durable preparation; a failed preparation replays the
captured events once. Reward rows and reward mail commit atomically. While a reward
is pending, other achievement events defer into the bounded queue; the exact ACK
replays them before invoking the success callback.

Logout uses dedicated `0x30a5`/`0x38a5` request and acknowledgement packets with
account, character, snapshot version and generation identity. The map retains the
achievement snapshot and final character status until that exact ACK arrives. An
ordinary achievement ACK cannot release logout state. Reconnect sends the retained
online-user state first, then the tokenized snapshot or cached final save. The
packet contract and compact `0x3099` shop semantics are recorded in
`doc/packet_interserv.txt`; the conservative script mutation rules are recorded in
`doc/script_commands.txt`. Fresh schemas and the October 1 upgrade use InnoDB and
the required pet identity index.

Final current-source validation:

- A pinned clean `make clean && make -j2 login char map web` passed. SHA-256 values
  are `e03340f6…` (login), `6ab83bd6…` (char), `d49efe52…` (map), and `33ed9ef1…`
  (web). The progression SQL report independently binds the same four binaries to
  source digest `f81bd332d30936ee794c67833c4f77583786b206b021946e39f3c80f3cca6390`.
- The source release phase passed all 53 checks, including 314 YAML files, durable
  schema alignment, achievement, receipt, inter-server recovery, economy, Eden,
  acquisition, combat-related and client compatibility regressions. Its report
  SHA-256 is `4a6df6561058656efbd67e5fac16f375057798d64933bad8e123b3c27e1f0194`.
- The isolated achievement MariaDB suite passed all six modes and 1,219 native
  checks across ordinary/reward writes, two writers, killed transactions and
  restart verification. The shop/recovery MariaDB suite passed its real asset,
  wire, race, restart and crash cases; the progression section alone reported
  5,723 checks, including stale terminal receipts, strict-ancestor folding,
  opaque-row preservation, compact-row concurrency and all write rollback points.
- A fresh internal crash realm bound source digest
  `6b25846125f04e4bda513f59a7865f82550e2e4fde767ebf3e9ad70efdfdcb3a`
  to the same four clean binaries. The probe stopped the map while character SQL
  slept before receipt insertion, observed no early receipt, then verified wallet
  debit, item 504, the receipt and achievement 220023 committed atomically while
  the map remained stopped. It killed the map before ACK consumption, restarted,
  performed a real relog and obtained the byte-identical durable snapshot
  `f1c5ccd4a90514e6a074d6acb4e91e509ac7cd9c5e313d06be7eb85cb40e1d40`
  with one debit, item, receipt and achievement. The isolated containers, network
  and database volume were removed after evidence collection. An earlier attempt
  was blocked before purchase by a full validation-host filesystem and is retained
  separately as environment-failure evidence.
- Focused sanitizer runners passed 159 shop-capture assertions, 18 item/taming
  cases, achievement persistence and reward ordering, inter-server recovery,
  compact wire framing, receipt codec and durable schema checks.
- Fresh native combat evidence passed 6 cases and 3,297 assertions with 624 equip
  and 600 unequip acknowledgements. The crown matrix passed 89,733 executions and
  8,169,504 assertions. The two socket paths passed 11,437 cases and 333,930
  assertions. Their memory/sanitizer reports are clean and networking was denied.
- The regenerated combined economy certificate was independently rebuilt and
  verified over 7,424 actions, 2,274 resources, 1,834 tight inequalities and 998
  source inputs. Its exact model digest is
  `b76e0b8c609cdc3d43169a6861fe80b926429cf7d1644715a2f75f6223de5bcb`.

The final review found no release blocker in this scope. The following limits are
recorded for the next bug-hunt rather than hidden by the passing checks:

- Quest, pet, storage and inventory logout saves were queued once before the token
  snapshot without retention. This transport-loss window is addressed by the
  follow-up logout journal below.
- Ordinary map-change achievement saves remain untokenized. A late negative ACK
  after the old map releases the session can allow stale state on the destination.
- The final character-status writer currently sends its ACK even if
  `char_mmo_char_tosql` reports an error; achievement durability is already
  established, but unrelated status fields remain exposed.
- Reward row and mail durability is atomic, while custom reward-script side effects
  run after the ACK and cannot be replayed after commit followed by disconnect.
  The currently audited reward scripts have no unsafe durable side effects.
- Progression capture assumes achievement condition scripts have no durable side
  effects outside captured achievement and argument state. Rental purchases and
  configurations with nonzero `shop_exp` intentionally retain their post-ACK
  callbacks.

No commit, publication, schema change on a live database, or deployment was made.

## Follow-up: retain dependent logout saves across character-link loss

The next selected bug was loss of non-achievement logout saves. Reconnect retried
the retained achievement/final-status stages after the live session had already
discarded quest, companion, status and other data. If logout began while the
character connection was absent, those serializers were skipped entirely. The
production-path regression failed before the fix with
`logout reconnect lost dependent save type=1 expected=21 got=0 initially_offline=0`.

Logout now serializes its dependent replacement-save packets into an owned,
bounded in-memory journal before teardown, including when initially offline.
The serializers cover inventory/cart, dirty personal/premium storage and registry
values, quests, active companions, status effects, cooldowns and bonus scripts.
Reconnect advertises retained ownership, replays the exact packet bytes and sends
the `0x2b30` stream barrier. The character server validates the owning connection,
account, character, version and generation before echoing `0x2b31`. Only that
matching reply releases the journal and allows achievement/final-status saving.
Retries after the barrier reply cannot overwrite assets with an older snapshot.
The journal also works when achievements are disabled. Shared guild storage keeps
its existing reconnect path that saves the current shared cache.

Validation evidence is in `OPS/logout-fix-20261001` in the parent Data2026 workspace
and `/app/pn-logout-fix-20261001` on the validation host:

- Red and green production-path tests exercise 60 combinations of initial
  connectivity, achievement availability and packet-loss boundary, including a
  consumed barrier with a lost reply. They check identity rejection, duplicate
  replies, no replay after acknowledgement and bounded journals under ASan/UBSan.
- Six actual dependent serializers produce byte-identical connected/captured
  payloads with the real protocol structures; captured bytes survive mutation of
  their original source data.
- The incremental `make -j2 char map` build passed in the pinned validation image.
  All 54 source release checks passed, including the new logout regression.
- A private realm with disposable SQL passed two real-client cases: kill the
  stopped character process after logout has queued its inventory/quest saves,
  and log out while the character connection is already absent. The latter also
  recovered dirty character variables. After reconnect, SQL contained exactly
  7 then 14 red potions, registry values 13 then 26, and one then two quest rows.
  Real relogs saw those same values, and subsequent ordinary logouts preserved
  them without duplication. `live-report.json` records the cases and binary
  hashes. Its final diagnostic scan was rerun after correcting an overly broad
  classifier that had treated informational SQL connection messages as errors;
  the original classification result is retained separately.

This closes the loss of packets captured by the logout save, not every persistence
failure. The barrier proves ordered delivery to synchronous character handlers;
it does not establish SQL success in legacy handlers, successful forwarding to
the login server, or survival of a map-process crash. Map and character binaries
must be updated together because they share the new packet contract.

An exploratory live test also reproduced a separate pre-logout registry gap:
script completion calls `intif_saveregistry`, clears update flags when enqueuing,
and may lose that ordinary save if the link drops before character processing.
The logout journal cannot recover values that were no longer dirty when capture
began. Evidence is retained as `live-prelogout-registry-gap*`; this remains a
candidate for the next fix, alongside negative SQL acknowledgement handling.
Fixing it requires an acknowledgement/fencing design for ordinary registry saves;
blindly replaying old values could overwrite a newer committed transaction.


## Docker deployment and disk cleanup checkpoint

The user subsequently requested Docker deployment, Git publication, and removal
of unnecessary files including obsolete backups. Production deployment completed
on **1 October 2026 at 11:50:37 UTC / 18:50:37 Bangkok**. The existing login,
character, map, and web containers now run the four validated binaries; FluxCP
was restarted after the game services. Database and proxy containers were not
restarted. All seven production services are healthy, original restart policies
are restored, and the temporary admission fences have been removed.

Evidence committed here:

- [Release receipt](evidence/bughunt_deployment_20261001/release-receipt.json)
  binds the manifest, binary hashes, backup, cleanup and production verification.
- [Full release gate](evidence/bughunt_deployment_20261001/full-report.json):
  all **83 checks** passed, including isolated startup. During qualification,
  the delivery fixture needed to mark its authenticated player's achievement
  cache as loaded; all 31 delivery cases then passed under sanitizers. No server
  binary changed for that fixture correction.
- [Production health report](evidence/bughunt_deployment_20261001/health-report.json):
  the unchanged default health thresholds pass, including services, recent logs,
  runtime metrics, backup restore verification and disk capacity.

The rollout installed all four binaries together because this change set updates
both the durable shop Commit contract and the map/character logout barrier.
It applied only `upgrade_20261001_achievement_atomicity.sql`: `achievement` is
now InnoDB, the legacy reward audit contains zero rows, and no unfinished global
point barriers were present. The fresh-install schema was not imported.

A fresh locked database dump was restored and checked in an isolated temporary
MariaDB container before production files changed. The retained archive is
`/app/rathena-database-backups/ragnarok-20261001T114814365976Z.sql.gz`;
its SHA-256 is recorded in the receipt. Immediate rollback files and their
metadata remain under `/app/pn-deploy-20261001/deployment/before` and the adjacent
manifest. This is a pre-migration database backup, not an automatic live restore
instruction: a later restore must account for any subsequent player activity.

The strict row check initially held admissions closed because the fixed Eden
catalog created its intended market row (`para_alc10`, item 7136, price 7000,
stock 20, flag 0). Comparing every other market row against the pre-cutover hash
proved that this was the sole difference. All 54 other checked player and
receipt tables were byte-for-byte unchanged; existing market rows were unchanged.
The original paused report and the exact catalog-delta verification are retained
with the deployment evidence. No broad data-check exception was introduced.

At the user's cleanup request, the deployment removed 2,819 obsolete compiled
build files (9,881,299,822 bytes) and 12,298 old backup files
(6,251,640,568 bytes), including two obsolete September 7 full snapshots. The
remaining source trees and diagnostic records in the old build area were
preserved. Nineteen completed bughunt test executables were compressed and
verified by decompression/hash before their uncompressed copies were removed,
saving another 1,648,866,358 bytes. Exact path/hash inventories remain in
`/app/pn-deploy-20261001`. Current game files, database storage, active validation
realms, the latest verified database backup and immediate rollback files were
retained. Final available space was **19,380,523,008 bytes** (about 18.05 GiB).

Detailed logs and sanitized deployment reports are mirrored to
`OPS/deploy-20261001/evidence` in the parent workspace; live database contents and
private configuration files were not copied into this repository. The ordinary
pre-logout registry gap, negative SQL acknowledgement limits and deferred rendered
client journeys documented above remain separate follow-up work.
