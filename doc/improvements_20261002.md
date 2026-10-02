# Reliability and player tools — 2 October 2026

Requested scope: all four bug-hunt recommendations and all five player features
from the October 2 review. Baseline: `a5efdec854371b0ac807a1ac9a80e71e4b3dbccd`.
This is an active implementation ledger, not a completion or deployment claim.
Existing untracked EM analysis and screenshots are unrelated and preserved.

| Work | Required result and acceptance | State |
| --- | --- | --- |
| Ordinary registry persistence | Versioned, acknowledged saves retain changes through disconnect/retry without overwriting newer transactions; real SQL, relog, account/character scope, deletion and stale-reply tests | Implemented; native replay, split-database migration/crash tests and all three scopes across real logout/login pass |
| Character save success | SQL failures cannot release retained final state; retry must converge without partial replacement loss; native fault injection and isolated SQL/reconnect proof | Native and real SQL crash/retry, isolated save/relog journeys and the final full release gate pass |
| Map-transfer achievements | Destination sees committed progress, including delayed/failed acknowledgements and transfer/reconnect boundaries | Implemented; retained transfer/registry/achievement/status ordering tests and an actual two-map roundtrip pass |
| Shop fairness | Measure synchronized buyers; implement and verify fair admission with bounded pending work, unchanged payment/delivery guarantees and capacity/stock revalidation | FIFO 32 implemented; native queue, real SQL stock/crash, four delayed real buyers and 30-minute shopping load pass; 7,326 purchases reconcile exactly |
| Complete progression guide | Cover all Chapter 1 steps and expand instance readiness; exact objectives, missing requirements, cooldowns and navigation agree with admission behavior | Implemented: 180 curated Chapter 1 guide entries and seven instance readiness paths; native script VM suite passes 263 cases / 2,256 assertions and real next-objective dialog passes; rendered acceptance pending |
| Equipment wishlist and farming planner | Select equipment goals; persist wishlist; show authoritative recipes, held materials across inventory/storage, remaining costs and real acquisition destinations | Implemented for loaded barter exchanges: eight saved goals, inventory/access-controlled storage counts, real NPC and monster destinations; native and SQL fixtures plus actual wishlist logout/login pass; rendered acceptance pending |
| Damage lab extensions | Boss-class targets, physical DEF/RES controls and reusable presets; identities and saved comparisons include all settings; verify actual damage behavior | Implemented; actual script VM/combat damage and all nine preset fields across real logout/login pass; rendered journey pending |
| Purchase and recovery history | Owner-scoped recent purchases with costs, outputs, delivery/recovery state and support references; no duplicate delivery or disclosure to another player | Implemented: immutable purchase details recorded in the same SQL transaction, owner-filtered history and current pet delivery state; rollback/replay/ownership and expanded SQL fixtures plus the real committed-receipt dialog pass; rendered acceptance pending |
| Party readiness panel | Member prerequisites, cooldowns and original-roster reentry status agree with actual admission; unavailable/offline state is explicit | Implemented for all seven shared readiness paths, including original-roster checks on the three Chapter 2 instances; native multi-player/cooldown/caller restoration and real ready/missing/busy/offline member dialogs pass; rendered journey pending |

## Validation and delivery

Use deterministic production-path probes before repairs, then native and real
SQL/crash tests where persistence is claimed. Build a fresh isolated candidate,
run the applicable full release gate, and exercise the changed player journeys.
Keep source checks, fixture results, rendered acceptance and deployment separate.
Do not infer that previous rendered-test deferrals certify these new features.
No production change or publication has been made for this work.

## Character-save checkpoint

The original production writer returned success after an injected SQL error.
The actual final-save handler separately acknowledged that failed save and marked
the character offline. Both failures were reproduced by
`python3 tools/ci/char_save_persistence_test.py --case writer-failure` and
`--case ack-failure`, with one changed scalar and one failed write.

The writer now locks and checks its six InnoDB participants, commits the complete
replacement transaction, and propagates failures without advancing its cache.
The final handler requires matching payload and current map ownership and sends
success only after that commit. Reconnecting maps already advertise retained
ownership before replaying their saves. Fresh-install schema and the separate
`upgrade_20261002_character_save_atomicity.sql` migration cover the companion
tables. The actual upgrade from legacy MyISAM tables passes in the disposable
fixture. No production schema has changed.

Validation so far:

- Native production writer/handler: 108 assertions pass under ASan/UBSan,
  including statement failures, transaction boundaries, retries and ownership.
- A fresh pinned-image `make -j2 char login` completed successfully.
- `char_save_sql_test.py`: production writer with real MariaDB triggers on nine
  write boundaries, six nontransactional-engine refusals, wrong ownership,
  duplicate saves, actual database kill, rollback and restart retry all pass.
  Source, header, object, binary and image identities are recorded in
  `OPS/improvements-20261002/character-sql-1/report.json`.
- Full char/login/map/web builds completed after the registry, transfer and queue
  changes. The transport helper now assigns current mtimes to archive entries;
  earlier incremental builds that only relinked were superseded by actual CXX
  rebuilds. End-to-end journeys, full release gates and deployment remain pending.

## Subsequent validation checkpoints

- `registry-sql-2`: actual split-schema engine migration, real local/global
  registry transactions, exact retry receipts, changed-wallet replay, injected
  write failures, database kill and restart all pass.
- `character-sql-2`: legacy engine migration and the complete character SQL
  fault/crash/retry suite pass.
- `shop-queue-sql-2`: sequential queued admission rebases all three stock kinds;
  five independent buyers receive five available items, the sixth is durably
  rejected. Existing concurrency, global-point, pet and crash suites pass.
  UBSan exposed references bound to packed character IDs; map lookups now copy
  the field into an aligned temporary before binding the key reference.
- `lab-target-red.log`: the original normal dummy produces Guardian class 2,
  not Normal class 0. Correct mode flags fix this. `lab-green` passes 361
  assertions, including the complete console parser, native target class,
  physical damage, all nine preset fields, invalid preset rejection and simulated
  relog retaining character registries. Controlled damage: 30,000 baseline,
  40,000 with boss-only bonus, 16,500 at DEF 400, 18,000 at RES 400.
- `onboarding_readiness_test.py`: 263 cases / 2,256 assertions pass, including
  independent member prerequisites, actual quest cooldowns, original-roster
  reentry, busy NPC preservation, unavailable members and caller restoration;
  generated guide entries agree with actual quest and navigation data.
- `player_tools_test.py`: the latest native sweep passes 17 cases / 170
  assertions. The earlier real SQL fixture passes 26 cases / 184 assertions for
  accessible personal storage, dirty cached holdings, unknown/unavailable
  sources, wishlist dialogs, transaction details and owner isolation. Expanded
  pagination/current-pet-status SQL cases also pass in `final-sql-2`: 29 cases /
  247 assertions. The missing-table SQL error in that log is the deliberately
  injected unavailable-storage case, which returns unknown holdings.
- `shop-history-sql-1`: the production MariaDB suite passes 19,524 normal
  checks and 17,288 queue checks, including atomic history rollback, engine
  refusal, exact currency/net-item changes and one history row per receipt.
  Existing receipt replay, crash, concurrent purchase and pet delivery proofs
  pass. History starts with purchases handled by the new binaries; previous
  receipts are neither replayed nor backfilled.
- The planner searches authoritative loaded barter recipes. It aggregates item
  quantities across inventory and accessible personal storage, including the
  live dirty page, and reports unavailable holdings as unknown. Refine and
  eligibility requirements are shown separately from quantities. Other crafting
  services retain their own recipe interfaces. Acquisition routes use actual
  loaded NPCs or monster spawns; instance/quest sources are not invented.
- `startup-1`: the isolated candidate map server reaches readiness with no
  startup errors. The complete diagnostic native sweep passes. Source checks
  and the sweep have found and
  corrected fixture extraction, registry-boundary, quest metadata and line-ending
  assumptions. Neither a diagnostic sweep nor this standalone startup is a
  substitute for the final unmodified full release gate.
- The metrics producer/health-consumer integration test reproduced the new queue
  field being rejected by the old exact schema. Metrics version 2 and its
  backward-compatible consumer now pass the actual producer-to-consumer test
  and all 11 health tests.
- `final-build-2/report.json`: the final pinned-image build of map, character,
  login and web binaries passes. Candidate source is frozen for the full release
  gate and refreshed SQL proofs. This document is outside the release source
  identity and may continue recording results without changing that identity.

These results include native tests, real SQL fixtures and isolated packet-level
server journeys. They do not certify rendered acceptance or deployment. All nine
implementations are present; final validation and deployment remain in progress.

## Release readiness audit

Read-only server inspection on October 2 finds no conflicting production changes
in the task's source paths: all 92 paths present at that checkpoint match baseline
`a5efdec854371b0ac807a1ac9a80e71e4b3dbccd` after LF normalization. The production
Git HEAD is `e985006171d2eb320ee512a653f4c83aea3d81b6`, but its deployed working
tree contains the baseline overlay. The live repository does not contain the
baseline commit object, so Git HEAD alone cannot establish deployed source state.
The broader runtime inventory finds operational configuration differences and
ignored import files, not additional engine/NPC/database source conflicts; the
database import files match the isolated candidate. Preserve production
configuration and compare hashes again immediately before any installation.
Detailed read-only evidence is `OPS/improvements-20261002/readiness-drift.json`
in the workspace root.

The read-only production schema audit found `acc_reg_str` still using MyISAM.
The registry migration initially omitted it, which would reject account-local
string saves under the new transaction guard. The migration now converts all
three string registry tables; the schema test reproduced the omission and passes
after correction. The real SQL fixture now resets/upgrades all three engines;
its final run in `final-sql-2/registry_save` passes. Production also requires conversion of `memo`, `skill`,
`friends`, `hotkey` and `mercenary_owner`; `char` and the numeric registry tables
are already InnoDB. Neither new receipt/history table exists in production yet.

The current release controller infers all ten release scopes for shared engine
or SQL changes. A deployable bundle therefore needs the exact final candidate's
full gate, required SQL cases, six rendered client cases, and six responsiveness
cases. Idle and load baselines must each cover at least 1,800 seconds. The lab
scope also requires a runtime identity generated after the final binary and
configuration are frozen. Source/binary changes invalidate earlier bindings;
source checks, standalone startup and old rendered deferrals cannot satisfy
these requirements.

Production Docker mounts override the three character/inter/map import files
using `tools/docker/asset`. Runtime identity must describe that effective mounted
configuration, not the empty host-side import placeholders. The existing services
were created from `/app/pn-improvements-20260919/game-prepared.json`; a default
Compose recreation could change operational configuration. The deployment plan
must preserve those definitions and install all three protocol participants
(map, character and login) together after a coordinated drain and backup.

Windows client automation remains unavailable. Node/SDK initialization now works,
but app discovery fails with native-pipe error 2 even after the user enabled the
plugin, selected Try now, and the agent reset/retried. The configured Node and
pipe both exist; the exact native-host routing failure remains unconfirmed.
Rendered acceptance remains unverified.
Packet-level server journeys can proceed independently but cannot be relabeled
as rendered client evidence. No production service, source, binary or schema has
been changed by this work.

## Final queue and server-journey checkpoint

The stock-write guard initially reused the queue-capacity predicate. A regression
against the real interserver handler reproduced an unguarded one-entry queue.
Stock writes now use `pn_shop_stock_busy` (any unresolved request), while buyer
admission uses `pn_shop_queue_full` (32 entries). Native admission/retry tests
pass, and `market-final-3` passes 49 real SQL probe processes and five database
restarts, including guarded catalog writes/deletes during a pending purchase.
Earlier build/gate/baseline artifacts are superseded by the second final build
and its matching evidence; the first timed baselines were stopped explicitly.

The final source identity is
`9fd7e6c78adea75339154cd8066a1fd4ae68589fe84216cef2fb3efc1c3134cb`;
the map binary identity is
`f45e09ee5cef9486e1c6ae147adfaba9bdb4c242f00ab8517c73c55372cbf7fa`.
All four `final-sql-2` suites pass: registry saves, character saves, achievements,
and purchase/history/queue/pet recovery. The original reports preserve explicit
native/SQL fixture boundaries and do not claim rendered producer coverage.

`journeys-final-proof/report.json` records seven passing real server journeys:
three registry scopes and all nine lab preset fields across logout/login;
the exact Chapter 1 next NPC; four independent party readiness states; wishlist
persistence across actual relog; achievement and dirty registry transfer between
two map processes and back; four simultaneous delayed-SQL buyers with one commit
each; and the player's actual committed purchase through the history NPC.
The journey artifacts were downloaded and all 12 artifact hashes verified.

The scoped deployment archive contains 93 implementation files. The repository's
read-only guarded deployment plan passes against production with no conflicting
source drift. Review documentation remains in the workspace/candidate rather
than being installed into the live checkout, which omits some baseline docs.
The final full release gate passes all 86 checks, including isolated startup.
All six timed responsiveness cases now pass in `responsiveness-final-2`.
The idle and bounded-load baselines ran for 1,874.62 and 1,874.41 seconds,
respectively, with 31 complete measurement windows each. A real 165-second
process stop caused the health check to reject the stale heartbeat and recover
after resume. The shopping suite reconciled 7,326 purchases across wallet,
inventory, stock and receipts, including 7,200 purchases over 30 minutes from
four buyers (observed p99 55.27 ms). A deliberate 75-second SQL delay raised an
unhealthy verdict and recovered after commit. These tests shared the host with
other isolated validation and do not claim uncontended production capacity.
The unchanged focused metrics evidence guard passes against the final candidate;
this is not a pass of the complete release controller.

The current-build cash producer applicability review proves that both callers
of the legacy single-item NPC cash adapter are compiled out for the actual
`PACKETVER=20260219` build. Complete source references, generated build flags and
preprocessed parser output are retained in `legacy-cash-review-final-1`. The
review records this entry as unreachable, not runtime-tested; modern NPC cash
purchases have separate actual server evidence.

The second pet integration receipt records 33 passing actual server cases across
29 producer families. It includes full-inventory pending delivery, owner-only
recovery after switching characters, replay safety and a real game/database
restart. The added catalog phase covers cash-button purchases, finite/unlimited
barter, reward groups and packages, item-use boxes, and encoded pet material
retirement. Deterministic catalog overlays are declared and hashed outside the
frozen product source; these tests certify production handlers with fixture data,
not the appearance or coverage of every shipped NPC/catalog. The five additional
world/mail cases now pass: actual player kills and floor pickup, MVP rewards,
taming, party distribution to both members, and raw mail attachment claiming.
They check real SQL acknowledgements, exactly one encoded pet identity per
delivery, replay and actual relog. Together these receipts cover 34 active
producer families; the remaining legacy entry is explicitly compiled out.
The first party attempt preserved a failed fixture assumption: the wrong battle
configuration key left random routing enabled. The corrected fixture verifies
the effective round-robin flag and observes the two different recipients.

An additional pre-existing defect was reproduced in unlimited pet barter:
successful Asset/BarterResponse handling attempts two zero-value cash/kafra
audit inserts with type `J`, which the bundled `cashlog` enum does not accept.
The asset, receipt and pet-identity assertions still pass. All three causal
fragments (request kind, unconditional cash logging, and enum schema) are
identical to the baseline. This separate follow-up is recorded with original
error logs in `pet-journeys-final-proof-v2` and the baseline comparison in
`cash-log-followup.json`; it has not been silently fixed or omitted from evidence.

The first complete controller checkpoint accepts the final native evidence,
responsiveness evidence, candidate binding and effective runtime identity.
That checkpoint correctly remained failed for the then-incomplete SQL producer
aggregate and the six missing rendered cases. Final SQL aggregation is recorded
below. The install candidate is a private offline artifact
containing effective live configuration; it must never be launched as a test
fixture. No production source, schema, binaries or processes were changed.

The earlier access restriction interrupted source transfer; its partial archive
was replaced and both archive hashes verified before extraction. No production
container or database was changed. The user's subsequent instruction explicitly
authorizes deployment to the existing server after validation.

Local evidence: `OPS/improvements-20261002` in the workspace root. Linux validation
uses a separate task directory and the existing pinned validation image; live game
containers and the September 29 rendered handoff realm are not test fixtures.

## Final automated acceptance

`sql-aggregate-final-3/report.json` passes all 43 named SQL/integration cases,
preserving 179 original artifact hashes. Its producer coverage contains 34
actual runtime families and the one source-reviewed unreachable legacy entry.
The unchanged pets/market/release scoped evidence check passes against the
private install candidate. Across the two actual pet receipts, 38 cases pass.

`release-controller-final-1.json` accepts the complete native, SQL and
responsiveness evidence and records only one error: missing rendered evidence
for `onboarding`, `encounter-reward`, `party-reentry`, `shop-storage`,
`client-visuals`, and `lab-relog-comparison`. The final parent Computer Use retry
successfully reset/imported the SDK but again failed app discovery with native
pipe OS error 2. This is a desktop connector failure; full filesystem, network
and SSH access are working. No in-session supported repair was found.

The deployment runbook now checks the complete passed controller before any
maintenance or migration, reserves the source rollback folder for the installer,
and explicitly quiesces/restores FluxCP alongside the game writers. Its SQL,
binary/configuration and source backups use separate paths. All October test
game/database processes are stopped with their frozen evidence retained. The
September rendered realm and production services remain untouched.

All nine implementations and automated acceptance are complete. The goal is
still unfinished: rendered acceptance, production installation and post-deploy
verification remain. This acceptance receipt does not certify publication or
production deployment.
