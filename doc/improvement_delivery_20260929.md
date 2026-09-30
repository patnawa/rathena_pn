# Improvement delivery ledger — 29 September 2026

Authorized objective: implement roadmap items 1–8 and the unfinished work, test them without requiring user client interaction, deploy to live and push Git. Completion requires current implementation, appropriately scoped tests, deployment evidence and remote Git verification. A partial gate or earlier receipt cannot close this ledger.

Baseline: `ce00fd541` on `codex/server-improvements-20260919`. Existing card-removal, armor-enchant, Mayomayo, combat-test and client-overlay work is integrated; unrelated local analysis is preserved outside staging. No rollout or Git push has occurred yet.

Current frozen source: `ef5350c8ae8522ba1077848716fdfa1caafd0224e9b9339bde29eae7472ce617`.
The last change repairs the real-world lab driver: identify the actual spawned
target, wait for GO, walk into range, verify movement acknowledgement, and require
positive damage before continuing. Fourteen recorded-packet checks passed. The
four production binaries are unchanged from the preceding successful native/SQL
candidate. Earlier bf65 evidence remains under its original binding. The final
native, SQL, world-protocol and sustained performance checks now pass. Rendered
acceptance and deployment remain incomplete. The sections below retain historical
checkpoints; the dated checkpoint immediately below is authoritative for current status.

## Current checkpoint — 30 September 2026

The final `ef5350c8` candidate passed the 71-check native gate
(`musl-gate-1790701965/release.json`), combined SQL acceptance
(`final-sql-ef5350c8/combined.json`, SHA-256
`621b66c13f7cb58b37dc5e505c1461a44a9a08722e691a181db1b4ce0adbd876`),
and the shipped world driver. The latter recorded three positive 60-second lab
runs, persisted comparison/history after relog, preserved interrupted runs, and
verified full-inventory, ownership, partial pet recovery and replay behavior.

All six responsiveness cases passed, with original artifacts copied and verified:
idle and bounded-load baselines (1,942 seconds each), process-stall detection,
single buyer, four buyers for 1,800 seconds, and a deliberate 75-second SQL delay
with health failure and recovery. Four buyers produced 1,800 successful purchases
and 5,400 contention refusals; successful purchase p95/p99 were 11.46/13.76 ms.
This was a cohosted synthetic workload, not a production capacity measurement.
Receipt: `final-responsiveness-ef5350c8/combined.json`, SHA-256
`d127fc572c2d8752680fcd04f1c551e99edb8810fc31a8f78dacd980face6c1a`.

The private Windows client reached character selection on 29 September. On
resuming 30 September, the supported native UI bridge reported its pipe missing,
including after the documented retry and session-reset procedure. None of the
seven required rendered journeys is certified complete. Production has not been
modified, client publication has not occurred, and final release acceptance must
continue to reject missing rendered evidence. Twenty-eight receipt-bound derived
SQL build inputs were staged only into the private deployment envelope; complete
source, binary, configuration and runtime identities remained unchanged.

| Requirement | Implementation | Verification | Live / Git |
| --- | --- | --- | --- |
| 1. Market restoration initialization | Deterministic entry initialization and retained stock restoration | Native poison tests and real SQL/restart suite passed; final binding rerun underway | Pending |
| 2. Durable pet delivery and recovery | All audited producer families, atomic entitlements, ownership/replay, consumed-pet handling and Recovery Desk implemented | Native/SQL suites and actual world recovery passed on prior bound snapshot; final rerun and rendering remain | Pending |
| 3. Automatic release evidence | SQL CI job, release-branch triggers, scoped controller, private configuration separation and deployment guards implemented | Full 71-check gate and SQL receipt validation passed before driver correction; final rerun underway | Pending |
| 4. Barter allocation and capacity | Shared immutable plan handles split stacks, overlapping requirements, freed slots and metadata | Native caller/planner and SQL proofs passed before driver correction; final rerun underway | Pending |
| 5. Guide and readiness | Ten onboarding states and five instances share read-only admission checks | Native state/boundary tests passed; rendered journey pending | Pending |
| 6. Dynamic rewards and client journeys | 2,681 recipes/342 output IDs cataloged; 32 Rune metadata definitions added | Native catalog coverage passed; 193 dynamic lines remain explicit unknowns outside this pilot; rendered journeys pending | Pending |
| 7. Runtime observability | Bounded latency/pending/retry metrics and stall health checks implemented | Partial superseded measurements preserved honestly; final sustained measurements pending | Pending |
| 8. Persistent lab comparisons | Three saved runs, equipment/buff/runtime identities, compatible comparisons and medians implemented | Three nonzero 60-second world runs, SQL relog and interruption preservation passed; final rerun and rendering pending | Pending |
| Existing NPC transaction fixes | Card removal, armor enchants and Mayomayo integrated | Included in passing native gate; final binding rerun underway | Pending |
| Existing client fixes | 184 weights, Sealed Drake tooltip and Rune definitions prepared for signed publication | Effective payload hashes verified; rendered samples and publication pending | Pending |
| Final release | Musl binaries, private deployment envelope, reversible preservation and guarded cutover prepared | All 5,922 nonconfiguration inputs match staged Git blobs/generated templates; complete acceptance still required | Deployment and Git push pending |

See [roadmap](improvement_roadmap_20260929.md) and [supporting research](improvement_research_20260929.md) for the full scope and evidence criteria. Additional architectural follow-ups are assessed alongside the affected implementation; none should silently weaken the listed guarantees.

## Current evidence and continuation state

- Local evidence: `Server-Development/improvement-delivery-20260929/portable-tests.json`, `market-red.log`, `market-green.log`, `metrics-green.log`.
- Isolated remote candidate: `/app/pn-improvement-delivery-20260929/candidate`. `remote.py sync` verifies all current source/data/test inputs and refuses to synchronize while its recorded build container is running. Build and gate handles are in the same remote evidence directory; inspect them before restarting anything.
- Clean toolchain: `pn-improvement-validation:20260929`, built from `tools/ci/Dockerfile.transaction-tests`. SQL proof: `sql-1790686533788038847/report.json` (11,785 atomicity assertions plus wire, concurrency and restart suites). This predates the final metrics rebuild and is evidence for that earlier candidate; final release must rerun it.
- Earlier full-gate map hash: `80b22910aac6d02883da22e46d56b1b5191bf9e78694016195a2040d8d8a50a9`; character hash: `b921181a9394b3d750503b0965b0faf41dda44125a4bc649ac678ba1816295d8`. Full gate handle: `pn-improvement-gate-1790687005168687789-map`. These are intermediate binaries, not a release; the newer pet-foundation build is recorded below.
- The full gate completed **57/57 checks** with clean isolated startup; its fixture container/network were removed by the controller. Evidence is `gate-1790687005/release.json`, mirrored as local `release.json`; `gate-result.json` records the terminal result. The subsequent GUID-stack correction passed the native 38-case shop suite (`shop-1790687331332082099.log`) and 26-case planner test but is newer than this gate/binary. Rebuild and rerun final acceptance after the remaining features land.
- Native Windows automation was initialized through the computer-use skill's `@oai/sky` and returned current desktop windows successfully. No game window was open. Rendered client acceptance has not run.
- **Do not deploy the current partial candidate.** Pet outputs now use durable entitlements in normal/market/barter purchases, but the shared planner still excludes consumed pet eggs; using it for unlimited barter temporarily extends that material refusal. The remaining producer/payment seams and supported pet material semantics must be integrated before release.
- Pet implementation must handle the full producer inventory in `transaction_async_audit_20260929.md`. In particular, putting an entitlement after an independently saved debit does not make payment atomic, and simply sending pet eggs through RODEX moves the problem to attachment persistence. Inspect and integrate each caller's actual consumption/claim seam.

No changes have been committed, pushed or deployed during this implementation pass yet. Existing work remains present. The complete objective remains active.

## Release bundle continuation

`tools/ci/release_bundle.py` now distinguishes the ten declared delivery scopes and
candidate/deployed stages. It requires current source/binary bindings, original
full native and shop SQL reports, named scenario evidence with intact artifacts,
and 30-minute idle/load samples where relevant. Native and SQL runners now stamp
and recheck those bindings. Eleven bundle regression tests and thirteen existing
runner tests passed in WSL. Existing intermediate gate/SQL reports predate these
fields and must be rerun, never relabeled. See `release_evidence_bundle.md` for
schema, configuration treatment and limits. The deployment controller still needs
integration, and scope declarations still require reconciliation with the change
inventory; this is not a completed release gate for all eight work items.

## Pet entitlement SQL foundation

The new `src/custom/pet_entitlement.hpp` / `pet_entitlement_sql.inc`, included by
the real character-server pet translation unit, provide issue/claim helpers that
join an existing reconnect-disabled SQL transaction. They never independently
commit payment. Keys contain account, character, session nonce, sequence and
output ordinal. The versioned immutable output preserves egg metadata, and the
ledger retains claimed identities after transfer or consumption. A pending claim
requires its original incubating pet row; missing identities cannot create ghost
eggs. `Sql_BeginTransaction` now rejects nesting so it cannot silently commit an
outer payment, and `Sql_InTransaction` exposes ownership of the guarded API's
transaction. Makefile prerequisites include the new pet and existing shop `.inc`
files so incremental character/map builds cannot silently reuse those old bodies.

`upgrade_20260929_pet_entitlements.sql` converts the pet table to InnoDB and adds
the ledger. It has **only been applied to disposable fixtures**, not production.
Producer behavior is still the old behavior; no repaired pet-delivery claim is
made until every audited producer is integrated. The map-side paid debit must be
part of the same outer asset transaction, not saved before calling this helper.

Build `pn-improvement-build-1790688498295780039` completed successfully. Map SHA:
`a5c44887d89c5fd2bfb13506c2b7c6f6ffcb4efa18c3f2f018e1fc00d797e14a`; char SHA:
`ac081393b885761e90175e223818a0c61cef7bd2c91222022d4fc93e8dfb7ca4`.
The previous build ended with a compiler error, was corrected, and is terminal;
no build remains running. `build_existing_config_remote.py` regenerates only the
two Makefiles from the configured candidate, then builds map/char.

Real SQL report: `/app/pn-improvement-delivery-20260929/sql-1790688631543683385/report.json`,
mirrored locally as `Server-Development/improvement-delivery-20260929/pet-entitlement-sql-report.json`.
Report SHA: `fbd20f1be170c8b779129b2004bf5dbacf947ad240462c17f63a28364060bc09`.
The original shop atomicity/wire/concurrency/restart suite still passes. Pet tests
execute actual linked `int_pet.o` helpers: 3,934 core SQL assertions, 314 restart
assertions, concurrent creation with one new and one existing result, and a real
database kill between entitlement creation and payment commit followed by recovery.
The fixture wrappers perform the debit/inventory orchestration, so this evidence
does not certify a live shop/script/mail/taming producer. No active SQL fixture or
tool session remains from this run.

Next integration seam: extend the immutable asset commit with versioned pet output
descriptions and claim identities, using these helpers before its receipt/COMMIT.
Claim marking and inventory installation must share that transaction. All 16
producer routes, raw RODEX attachment consumption, multi-output batching, metadata,
consumed pet materials, original-character login recovery and the Child Manager
ticket debit remain required. Do not switch callers to a standalone enqueue after
their existing debit or treat a passing helper suite as closing item 2.

## Pet asset protocol and first producer integration

The next seam above is now implemented. `pn_shop::Commit` version 2 carries pet
output descriptions or claim IDs/slots, while its static assertion keeps the
frame below 65,536 bytes. Old request layouts/versions are rejected. The production
character transaction now creates entitlements or marks claims before persisting
inventory, balances, stock and its replay receipt. A savepoint undoes earlier pet
operations if a later claim conflicts, before recording a durable rejection.
An exact receipt replay still precedes all asset writes and preserves newer state.

The map planner creates deferred pet events (no inventory callback yet), preserves
egg metadata, assigns individual identities, and installs claimed eggs with their
original IDs. The acknowledgement handler skips inventory callbacks for deferred
events and attempts recovery after commit. Recovery selects pending outputs for
the original account/character, plans only what fits, and submits a durable claim.
It is scheduled after character inventory/status loading and exposed to players
through `@petrewards`. Actual login/reconnect and rendered command behavior still
need the live isolated fixture; the timer/SQL boundary tests are not substitutes.

Normal shops, finite/unlimited markets, and finite/unlimited barter pet-output
carts now dispatch these transactions before any debit. Seven new native caller
cases verify the deferred result, exact output plan, stock/material costs, dispatch
refusal, freed-slot case and unchanged live player state. Together with the earlier
38 cases they pass without memory leaks in
`shop-1790689730833643220.log`. An initial fixture cleanup retained an item database
shared pointer too long; releasing fixture references before allocator teardown
removed the diagnostic. The map planner/recovery suite now passes 32 cases and the
acknowledgement suite covers deferred callbacks and Asset cash balance application.

Build `pn-improvement-build-1790689488626205097` is terminal and passed. Map SHA:
`fae505775074ade187de47c5cdfd3edb1c89d62e646b4fb80627080e6d9ca9f6`; char SHA:
`f8975bdbc68192dfd1f60f8869e862179a05bfaad8dc09dfa09b0ebdeb3385be`.
Real SQL report: `sql-1790689599762607644/report.json`, mirrored as
`Server-Development/improvement-delivery-20260929/pet-asset-sql-report.json`.
Report SHA: `74f28cb1ad689aa73ae38971f5f841c9696255acf251e4b01597ed41025b7e11`.
It includes **4,718 assertions against the production asset transaction**, covering
every pet/payment/stock/receipt write failure, old protocol rejection, wrong owner,
stale stock, multi-output creation, claim conflicts, atomic inventory installation,
replay after transfer and stockless material/cash costs. The prior shop and pet
restart/concurrency/crash tests also pass. Subsequent native-caller test edits are
newer than that report's whole-source binding; final acceptance must rerun on the
finished candidate.

Remaining pet integrations: cash/item/point NPC shops (including legacy), cash
button, script getitem/getitem2/group/makepet, package boxes, raw mail attachments,
mob drops, administrative item/item2/makeegg, taming, the Child Manager ticket debit,
consumed pet eggs, and a rendered Recovery Desk. Replace the old ambiguous boolean
API only with explicit handled/rejected/pending semantics across all callers; do
not add ordinary-egg fallbacks. Before rollout, drain pending v1 requests, stop both
map and char writers, retain old receipts, migrate, and start the matching v2 pair.
The combined full gate and complete release scope remain pending. Nothing has been
deployed, committed or pushed during these passes.

### Pet material retirement and parallel implementation

Production SQL now atomically retires a consumed incubated pet alongside its
inventory removal, replacement output, and payment. It locks and verifies the
single owned egg identity, rejects active pets/duplicate identities/pending
entitlements, retains claimed entitlement receipts, and rolls all pet operations
back to the savepoint before durable rejection. The inventory identity index is
installed conditionally by the pet migration. Raw pet materials remain eligible;
encoded pet eggs require a valid retirement descriptor. Barter dispatches material
retirements even without a pet output or limited stock.

Build `pn-improvement-build-1790690258843296181` passed (map
`784bab6fef495447f28a38f44e8ac09ee982d758de63b5429ab30a097da978d0`, char
`e109988fb2b09bd9f0d1f432da8862116e0ded876b83aeac2bcb2a0b76a7f433`).
Native caller suite: 47 cases, no memory leaks, log
`shop-1790690749346930505.log`. Planner suite: 33 cases. SQL report
`sql-1790690739164965400/report.json` includes 2,014 retirement assertions and
previous asset, concurrency and crash tests. Local mirror:
`Server-Development/improvement-delivery-20260929/pet-retirement-sql-report.json`,
SHA256 `19fbc512679ad68ecd7f0ef547cd3e72cdd37d4f34e1773f8779b814139cfa46`.

New getpetreward builtin and Child Manager conversion are local implementation
pending native VM validation; do not count them as accepted yet. The API combines
eligible material consumption and deferred entitlement creation. Child Manager
no longer deletes tickets independently or blocks a durable reward solely on
pre-exchange capacity. Surrounding arbitrary script progression is not atomic.

User explicitly requested ten agents. Runtime permits four concurrent agents
including root, so ten workstreams are scheduled with three active workers and
root integration. First workers own guidance/readiness, lab history, and dynamic
catalogs. Queue is recorded outside the repo in improvement-delivery-20260929/
agent-workstreams.md. No production deployment or git push has happened yet.

### Parallel integration checkpoint

First workers delivered guidance/readiness, persistent lab history, dynamic
catalogs, paid cash/item shop routes, package/admin pet routes, and a staged client
metadata fix. Ten workstreams are scheduled; runtime limits both concurrent and
retained agent threads, so completed live workers are reused for later streams.
Current workers handle mail transaction integration, release contract enforcement,
and isolated runtime metrics. Root retains integration, remaining producer seams,
rendered acceptance, final deployment and push.

Build `pn-improvement-build-1790691783181592434` passed: map
`c3da0049eda9dd1bea83ef4260f2426f196500a2754116f0234b89cf9efb32e1`, char
`6b4c4437d30a0f83f07ee63b25d23507db0528760a8547ec3abd683a46aa4dd0`.
Native results against that intermediate binary pair:
- Shop callers: 61 cases, no leaks (`shop-1790691902287767382.log`).
- Actual Child Manager + getpetreward + getitem/getitembound/getitem2/makepet/
  getrandgroupitem: 18 cases / 247 assertions, no leaks
  (`pet-script-1790691909629757792.log`). Submission is doubled; SQL tested separately.
- Lab history: eight cases /122 assertions, no leaks
  (`lab-history-1790691915303584707.log`); persistence remains a registry double.
- Exact package handler + real planner: eight cases, no leaks
  (`package-pet-1790691949926351567.log`).

Root added explicit pn_pet_grant_result NotPet/Rejected/Pending API and batched
script families through Asset transactions, retaining metadata. This is not a
complete item-use repair: pc_useitem still consumes an ordinary lure/box before
executing scripts, so that upstream debit needs a shared reservation seam.
Point-shop registries, mail (in progress), taming/mob sources, old callback removal,
world recovery/relog and rendered acceptance remain incomplete.

Dynamic catalogs cover 2,681 recipes/342 outputs and expand acquisition to6,976
IDs. A staged Lua client overlay adds32 missing definitions (105616,105617,
105619-105648), incorporates184 weight fixes and preserves SealedDrake. Candidate
is improvement-delivery-20260929/rune-client-candidate-v4; report SHA
`ba20a791d80ae9c8579129e02f683e009b7f33249c4f058032f251df0524cd6a`.
Installed client remains unchanged.193 dynamic lines remain unresolved.

Onboarding native fixture is still under correction (instance fixture state and
registry lifecycle); do not call it passing. Rune exhaustive fixture now uses
separate generated lambdas to avoid extremely slow compilation and binds generated
include bytes to its compile cache; missing decomposition key metadata was fixed
in its fixture. Full exhaustive rerun is pending. Runtime startup found three
missing TAB separators in new lab function declarations; corrected locally and in
private metrics snapshot, restart baseline from zero. This demonstrates why native
extracted-body tests alone do not certify complete NPC loading.

Metrics owns its copied snapshot; handle metrics-handle.json references a real
30-minute idle +30-minute bounded NPC/SQL workload and process/SQL stall probes.
This is not representative player/combat/purchase throughput; report those limits.
No deployment, commits or push during these changes; final all-scope gate pending.

### Latest integrated native results

Build1790692336349854848 passed with mail descriptor and runtime identity startup
hooks: map5107ae2f51472377cd7a1434e5d66e78ff9e42eae6cb47c041c0bb715dc8f1e0,
chara2c54dc93138383453d3da1474b9ce513327bf8f020c433bd158989b880649d0.
Onboarding fixture now initializes INSTANCE_BUSY and the real instance registry,
and read doubles no longer insert missing zero entries. It passes53cases/336
assertions without leaks (onboarding-1790692459772816816.log). Lab updated
fail-closed identity suite passes8cases/126 (lab-history-1790692466269306467.log).
Package/mail/admin exact caller suite passes8+6+7 cases without leaks
(package-pet-1790692452971113849.log). RealSQL mail proof remains pending.

Rune exhaustive native ASan/UBSan suite passes2,842cases/1,648,828assertions without
leaks (rune-catalog-1790692143414603392.log), against earlier buildc3da0049.
Compilation caching includes generatedcasebytes; decomposition inputkeys now
populate actualfixturemetadata. This is intermediate evidence, not final current
sourcebinding after subsequent mail/identity changes.

Renderedworker prepared privatecandidate client/realm scripts while the isolated
metrics baseline continues. No renderedjourney result yet. Installedliveclient,
production deployment, gitcommit and push remain unchanged by this goal.

### Atomic mail acceptance checkpoint

Build1790692560316173181 passed: map
b7881832566b61103a6445679926fb10301fe9a0057579151b32858a843f0bab, char
f6cba0318f054f699cc3819006a8d38fbbfadc86ec21dc25c17d81247dbddf4b.
All mail ITEM extraction now takes the durable path, including ordinary-only and
already encoded pet attachments; legacy map+char ITEM extraction cannot run.
Rollout still requires draining old requests. Native8package+8mail+7admin passed
without leaks (package-pet-1790692705638097469.log). RealSQL suite passed,
including5,926 pet-mail assertions, in sql-1790692705638095549/report.json.
Local mirror pet-mail-sql-report.json SHA
1c8768c021ea30dc0b323001f463f9a0b75ee37309c49d0b75ef4e3195a870f3.
The report includes nativeSQL log artifact hashes under the tightened contract.

Fullreleasegate on this intermediate snapshot is running (rootexecsession49683;
use gate-handle.json and gate_status_remote.py). No sharedcandidate synchronization
while it runs. Workers are now implementing point-shop registry atomicity and
item-use reservation/taming/mob seams, and producing market SQL restart evidence.
Market read-only production observation:246persistedrows,zero flag&1 runtimeadded
rows at2026-09-29T14:37:31Z; exposure reportmarket-exposure.json. No productionwrite.
Metricsbaseline remains independentlyrunning on copied earliercandidate; five
renderedjourneys onlyprepared, notpassed. Nothing committed/pushed/deployed yet.

### Full gate and market SQL checkpoint

The intermediate atomic-mail candidate completed all 65 release checks, with
clean NPC startup, in `gate-1790692785`. Map hash remains
`b7881832566b61103a6445679926fb10301fe9a0057579151b32858a843f0bab`.
Fetched `release.json` SHA256 is
`7a003c999da09043cb641b7715e2c88aa011487b9c89237eb6889a26490731c5`.
This does not certify subsequent point-registry or item-use changes.

Market SQL restoration passed 49 probe processes and five database restarts,
including four allocation poisons, zero/unlimited stock, and reload during a
pending purchase followed by authoritative ACK refresh. Report SHA256:
`070b8db968398126c656f11fc18e5b419e19abb15f71fb5b3a6201ef0e57943f`.
See `market_restore_sql_acceptance.md` for exact production-function coverage
and fixture limits. No broad reload prohibition was needed.

The Main Office now exposes Pet Recovery at lobby (140,60), using the existing
owner-filtered entitlement query and durable @petrewards claim path. The layout
generator and static reachability check pass with 53 desks. Full startup and
world interaction must be rerun on the final candidate.

Actual-world preparation initially reported a missing Damage Lab definition;
this was a search false negative, corrected without a production edit. The
tracked `db/import/quality_lab_instance_db.yml` defines instance 1000 and is
imported by `db/instance_db.yml`. Searches must include ignored import paths.
Metrics collection shares its host with validation jobs and is not an
uncontended performance benchmark. Representative purchase workload, world
recovery/relog, rendered journeys and final release/deployment remain pending.

### World and purchase workload preparation

Added `shop_load_live_client.py` and an explicitly isolated market fixture. The
driver checks the Docker label, exact network namespace and disposable database
before connecting four synthetic accounts. It measures one/four-buyer native
purchase ACK latency, checks persisted balances/items/stock/receipt count, and
injects a 75-second receipt-insert delay to require visible pending health and
recovery. Only Python syntax is verified so far; no shopping runtime pass yet.
The shared bank packet helper now accepts explicit ports, retaining its existing
defaults. The metrics release contract requires these three shopping cases in
addition to the longer idle/NPC workload and process-stall evidence. Both bundle
and controller suites pass 11 tests each; missing shopping evidence is rejected.

World lab/recovery preparation uses QA fixtures under effective `conf/import`,
so runtime identity includes them while the candidate source binding stays
unchanged. A private mixed debug snapshot is being prepared separately from the
shared candidate; its overrides are recorded and cannot certify final deployment.
Its initial startup exposed an older login binary requiring libmysqlclient.so.24
while the validation image supplies .21. The private fixture is rebuilding login
and web, and subsequent integrated builds now rebuild all four server binaries.

The ongoing earlier metrics snapshot completed its idle phase: 1,872.6 seconds,
31 samples, maximum window timer p95/p99 bucket bounds 25 ms and dispatch p95/p99
5 ms, with zero shop operations. These are host-contended intermediate values,
not final candidate performance acceptance. Its bounded NPC/SQL phase continues.

### Intermediate four-service build and actual-world proof

The first four-service build exposed a Makefile dependency-order bug: `$<` for
script.o selected item_use.hpp, producing a header object instead of script.cpp.
Corrected the first prerequisite and removed only that invalid isolated object.
Build1790694492701365475 then passed:
map `c5aa12538a1428f9f554f886dcc0140c732f08a03fc91d610a67671a0fb56bca`,
char `3a2f7b0c9fc2a6da45b191b3b94ac6dea1879e04bd4cb31b40251624f9ba5e3a`,
login `47c55da8b6b13c5bc8155d0be349f2d642e8a0890465f8fae3c96b869d5b5aff`,
web `c51404abc4e438935b58eaacdeaf8e9e2e6de83b169575a6789976e3c3c13665`.

Its SQL run sql-1790694581231445967 failed at the linked login probe with an
unaligned packet store; the fixture and corresponding production variable-offset
reads/writes are being corrected using memcpy. Its gate first stopped because
the extracted ACK fixture lacked the new item-use seam. The fixture now tests
active-scope refusal and exactly-once settlement, passes locally with sanitizers,
and was copied alone into the frozen remote candidate. Gate1790694733083953273
is running against that intermediate snapshot. No final gate pass is claimed.

Private debug world proof passed three real 60-second lab runs through SQL relog,
incomparability checks, interruption without history replacement, and three
entitlements across full inventory/character switching. The actual Recovery Desk
claimed one freed slot; @petrewards claimed the remaining two, with no duplicate
pet or egg after replay. Report SHA256:
`225137f5d092f3d9ebd07fef0a008304f8c873bf75888a63c38f08966898f073`.
This is the earlier mixed debug binary, zero-damage runs and protocol evidence,
not final-source or rendered acceptance. Native client SSO currently disconnects
before character selection; fixture diagnosis is underway, with no user action.

Shopping debug attempt one omitted heartbeats for idle clients/long waits and was
aborted before SQL delay. A corrected external driver is running, preserving
the private candidate source binding and recording its separate driver hash.
Final runtime acceptance remains pending.

Commit preparation must explicitly include new ignored `src/custom` headers/inc
files; ordinary `git status` hides them. Do not include generated Makefiles,
default import templates, private runtime config or unrelated EM gear research.

### Four-service build and integration checkpoint (15:25 UTC)

Build `pn-improvement-build-1790695015150033323` passed all four services.
Map SHA256 `4954143a7e147c08edfbb20d515879a1d993ceb8de92474446d649b7ea862c4e`;
char `20f2d97411ee279f427b677aec12b594e3140a23eaffc8e4131dfb6ef922117c`;
login `26f670979031f6f172d155eb80d67a1502cff2b5656dbc3022299713cb33eee2`;
web `c51404abc4e438935b58eaacdeaf8e9e2e6de83b169575a6789976e3c3c13665`.
Full SQL report `sql-1790695270641221142/report.json` passed, including
point Asset commit, global barrier, global restart and linked login adapter.
Focused point, package, shop and pet-script native tests passed. Two item-use
failures remain recorded: metadata case10 and missing fixture timer initialization.
Their fixes are included in the next checkpoint; acceptance awaits execution.

Bounded shopping debug report `shop-load-1790694552713246508/report.json`
SHA256 `f77740494daa26f3a97075bddd32aa11889dfa8431de8d0476db7c0276cc4762`
passed 125 single-buyer and 245 four-buyer purchases, with735 concurrent busy
refusals. Successful ACK p99 was13.83/15.00ms. A75.01s SQL-delayed purchase
triggered pending-age health failure and then recovered. Persisted wallets,
inventory, stock and receipt counts agreed. This is paced debug-snapshot
measurement, not final-source acceptance or a capacity result.

Snapshot5577 inputs39 changed now builds as `pn-improvement-build-1790696087249701315`.
It includes capture reservation and raw floor acquisition, with new native and
SQL/restart cases. Direct group/rental producer audit and classifier wraparound
follow-up remain under implementation. No production deployment or push yet.

### Raw floor SQL acceptance and archived evidence

Four-service build `pn-improvement-build-1790696087249701315` passed.
Map SHA256 is `55dbf0d0f98dbe2effa2f039d696f6e4aca2f343b102d219edff196daed81326`;
character, login and web hashes match the preceding four-service checkpoint.
The original build receipt is retained locally as
`build-1790696087249701315-result.json` in the delivery operations directory.

Full SQL run `sql-1790696661944640900/report.json` passed, including raw floor
Asset admission (1539 checks) and restart recovery (376 checks). The earlier
floor failure was an unreachable inventory-INSERT fault in the fixture:
entitlement-only admission correctly leaves inventory unchanged. The corrected
proof rejects inventory INSERT/UPDATE/DELETE while committing entitlements,
and retains failure/retry checks at actual writes plus second-output rollback.
All 13 mandatory pet SQL fields are present; all 36 original log hashes verify.
Report SHA256 is `f06a6e3b9476a976e32fddadebf6e235821f49cadfe6207e83f06259337d8b63`.

Downloaded report and logs are archived locally in
`improvement-delivery-20260929/sql-1790696661944640900-logs.zip`, SHA256
`350f40011a33b57cc7608371b34e85a76567d166240ed41608ab32d292cb7484`.
The sibling `sql-1790696661944640900-download.json` records every file hash and
mandatory-field verification. Remote evidence was read without modification.

Focused native run `pet-integration-1790696283082077396` passed floor11 and
metadata20, with verified original logs downloaded locally. Its overall result
remains failed: the item-use native run exited1 at registry-fixture setup and
requires a successful rerun. This milestone is SQL acceptance for its bound
snapshot, not complete final-source, rendered-client or deployment acceptance.

### Rendered fixture routing and placement checkpoint

The isolated native client now reaches the game world. Its executable requests
administrator privileges, which blocked the lower-privilege input bridge.
Launching only the disposable client with process-local RunAsInvoker restored
normal input. No installed executable or persistent Windows setting was changed.
The QA adapter substitutes a disposable fixture login; this is not proof of
native SSO. Private client identity is recorded in the operations directory;
rendered acceptance still awaits the finished server snapshot.

Actual default-camera inspection showed the newly added recovery NPC at140,62
mostly hidden by a foreground wall, despite server-side reachability. The office
generator now places it on the open floor at134,44 with approach134,43. The
53-desk connectivity check passes; final full-sprite/dialog verification remains.

The broad item-use audit classified20,346 effective scripts:20,250 ordinary,
69 pet-supported and27 conservative refusals. Those27 are valid existing
ordinary boxes, so this checkpoint must not deploy as-is. Their grant contract
and bounded-expression handling are being corrected; audit loader diagnostics
also remain to be eliminated before catalog acceptance.


### Production-compatible build and validation checkpoint

Production uses Alpine/musl and MariaDB3. The earlier Ubuntu/glibc validation
binaries cannot replace its existing containers. A fresh build using immutable
image `sha256:b17273c43a92428604e46db95b0bef8fd94041758096dfe0351d13bdd55ca199`
compiled all four servers. The first final link exposed runtime identity's direct
OpenSSL dependency; map/generator Makefiles now link crypto explicitly, and CMake
uses OpenSSL::Crypto. Successful map SHA was
`7bbf39e2be7045773f750f759f8a68adccf747cd99bcf806d87e8f7de84ef0cd`.

The real SQL suite passed on that musl snapshot. Original report SHA is
`7c5b5464223a3feb390e8780a7a06dd10c82e1b3bf2789a14313d56c3fbbf9d2`;
all36 evidence artifacts were downloaded and checksum verified. This includes
pet floor, retirement, mail, global point barriers and recovery fault cases.
It remains an intermediate snapshot: classifier/test fixture fixes follow.

The full native gate encountered a validation-image problem: its GCC15.2 ASan
runtime has the upstream missing struct_sock_fprog_sz definition. The separate
UBSan SQL suite passes; ASan checks are not waived. An isolated sanitizer-runtime
backport is being prepared without modifying production's image or server ELF.
The focused run passed metadata24, point, package, shop and petreward26; item-use
failed because its fixture omitted mapindex initialization, now corrected.
Catalog execution had no compilation errors but32 conservative refusals; a
bounded-analysis widening fix and two regression fixtures await verification.

Old metrics and market-proof trees were archived to Windows with complete
before/after file inventories, transport checksum and extracted-file checks
before removal from the task workspace. The supplemental-evidence-location index
preserves original paths and locations. No production content was deleted.
At the latest read-only health check, all services, backup proof and timer passed;
only disk space failed. Further final measurement/deployment acceptance remains.

### Final native and SQL candidate checkpoint

Frozen source identity is
`bf65a030e5370f8a57808de02b5af9bdf3d74858c152920c6f268a5a2d629c01`;
map binary is `843e1b2559e8269002e161a285dcb0bc2b70fe92e64a10f1b1cd1323f5c56182`.
All four musl binaries passed loader checks inside the actual production image.
The complete release gate passed 71 checks, including isolated startup, in
`musl-gate-1790699936/release.json`. The validation-only sanitizer image contains
the upstream GCC15.2 sanitizer fix; benign, heap-overflow, leak and undefined-
behavior controls demonstrated that the real instrumentation remains active.
No production image or server binary was altered by that toolchain repair.

Final real SQL and market runs passed under this exact binding. The combined
receipt `final-sql-bf65/combined.json` has SHA256
`bd8a71f378b4eb5b7c53ae75c0c8e95bc7b59d6647badce5838aa4c62ce5fa37`.
Its 112 original reports/artifacts were downloaded and hash verified. The receipt
distinguishes real SQL durability from native transport doubles and composed
full-inventory evidence. It does not claim rendered all-producer acceptance.

The preceding focused catalog run, with identical production binaries but an
earlier test-source binding, covered 29,713 effective items and 20,346 scripts:
20,277 ordinary and 69 supported pet scripts, with no unknown classification,
conservative refusal or compilation failures. It remains supplemental evidence
under its original binding; it was not relabeled as the final run.

Curated Git staging initially covered 198 task files and preserved 16 unrelated
local artifacts. Four additional existing files required line-ending-only
renormalization so their committed blobs match the tested canonical source.
All 5,922 nonconfiguration candidate inputs now match staged Git blobs or the
explicit 44 generated import-template mappings. Cosmetic trailing-whitespace /
final-blank-line findings remain recorded; no late formatting edits invalidate
the frozen candidate. Client journeys, sustained measurements, deployment and
remote Git verification are still pending at this checkpoint.
