# Reliability and player tools — October 2026

This release implements the four reliability repairs and five player features
requested in the October 2 review, against baseline
`a5efdec854371b0ac807a1ac9a80e71e4b3dbccd`.

## Implementation

| Area | Result |
| --- | --- |
| Registry persistence | Versioned acknowledgements, immutable scoped retries, and transactional receipts retain changes across disconnects without replaying stale wallet state. Account-global, account-local and character registries are covered. |
| Character saves | The writer checks and locks its six InnoDB participants, commits the complete replacement transaction, and reports failure without advancing cached state. Final-save acknowledgement requires a matching payload and current map owner. |
| Map transfers | Registry, achievement and status acknowledgements gate transfer so destination maps receive committed progress. Retained ownership is advertised before replay after reconnect. |
| Shop fairness | A bounded FIFO queue admits 32 pending requests and revalidates stock, capacity and payment at admission. Stock contention and a full queue have distinct responses. |
| Progression guide | 180 Chapter 1 entries provide the next objective and navigation. Seven instance readiness paths show actual requirements and cooldowns. |
| Equipment planner | Eight persistent equipment goals use loaded barter recipes, inventory and accessible personal storage counts, and real NPC/monster acquisition locations. Unavailable holdings remain unknown; eligibility/refine requirements remain distinct from quantities. |
| Damage lab | Boss-class targets, physical DEF/RES, magic MDEF/MRES and three reusable presets retain all nine settings. Damage checks exercise the actual combat implementation. |
| Purchase history | Owner-scoped immutable purchase details commit in the same SQL transaction as the purchase. History includes costs, outputs and current pet recovery status. Existing receipts are not replayed or backfilled. |
| Party readiness | Each member's prerequisites, cooldowns and original-roster reentry are evaluated independently. Busy, unavailable and offline states are explicit. Original-roster checks cover the three Chapter 2 instances. |

## Deployment status

Deployment to the existing server completed on 3 October 2026. Its source,
effective runtime identity and four running executables match the verified
Linux package. Login, character, map, web, database, reverse proxy and FluxCP
service checks pass. The task maintenance fence is removed and the original
restart policies are restored.

Actual post-install packet smoke passed login, the shipped guide's next-objective
navigation and wishlist persistence across logout/login. The new QA account is
disabled and offline. Subsequent LAN checks reached ports 6900, 6121, 5121, 8888
and 8080; FluxCP returned HTTP 200. The four database integrity checks show no
orphaned characters or inventory, invalid inventory amounts or negative zeny.

The operational receipt is `deployment-run/deployment-receipt.json`, SHA-256
`3d7d7ccd5522128c57ae77b29c348d299d11cc703d39b28e0f63b2fc4fafa270`.
Its status is `deployed-with-deferred-visuals`. The unchanged full controller
in `linux-release-controller-2.json` accepts every automated scope but still
fails for the six missing rendered cases listed below. Operational deployment
does not certify those checks.

The implementation is published on `codex/improvements-20261002` as
`2d48ece7550d83d21b10e2d140e50d53a88d9002`. Unrelated EM analysis, probe files
and screenshots remain untouched.

## Exact Linux package

The original validation archive contained CRLF baseline files, whereas the
deployed Linux tree uses LF. Exhaustive comparison identified 5,158 files with
newline-only differences and no content or binary differences. Installing that
archive verbatim would change executable maintenance scripts' shebangs, so the
Linux representation was frozen separately and its automated acceptance rerun.
Original archives and receipts, including failed or interrupted attempts, are
preserved. No old report was relabelled as execution against the Linux package.

Linux source identity:
`f8926fc0d251080697260d213fabb3c619c78d883ac022c31417686ebd1d38d8`.
A fresh build reproduced all four previously tested executables byte-for-byte:

| Executable | SHA-256 |
| --- | --- |
| map-server | `f45e09ee5cef9486e1c6ae147adfaba9bdb4c242f00ab8517c73c55372cbf7fa` |
| char-server | `5c4c5e9297a050cfd58fbb865908f29742e89bef62a35dd740e0d313625e55c5` |
| login-server | `3d478700b845ef3450e16d211c2ce9cfd6fbb3f00e14c013b934a3ea5c80db5c` |
| web-server | `80ab2e858d69b163d7ce9eb3bdb62034433e761d98356756c13f034c094e3d64` |

The package and rebuild proofs are in `linux-package-freeze-1` and
`linux-package-rebuild-proof-1`. Private effective configuration is preserved,
including Docker's character/inter/map import overlays. The credential-bearing
install candidate is never launched as a test fixture.

## Validation

The Linux package passes all 86 native checks, including isolated startup, in
`linux-full-native-3`. Its final SQL/integration aggregate,
`linux-sql-aggregate-1`, passes all 43 named cases and verifies 184 artifact
hashes. The unchanged focused pets/market/release/barter guard accepts these
receipts. `responsiveness-linux-3` passes all six responsiveness cases, with all
case artifacts and input snapshots independently verified. Superseded and
interrupted attempts remain preserved.

The idle and bounded-load baselines ran for 1,865.70 and 1,874.72 seconds,
retaining 30 and 31 complete measurement windows. Four concurrent buyers made
7,200 purchases over 1,800.17 seconds, with observed p99 latency of 56.08 ms.
Across the shopping cases, 7,326 purchases reconcile across wallets, items,
stock and receipts. A deliberate process stop and a 75-second SQL delay were
detected and recovered. The SQL recovery supplement requires a newer empty-queue
sample and the same five-second threshold used to detect the delay.

Completed Linux results include all four SQL suites in `linux-final-sql-2`,
33 actual pet catalog cases across 29 producer families, five world/mail cases,
and seven end-to-end journeys. The journeys exercise registry and lab preset
persistence, wishlist relog, the shipped guide, party readiness, two-map
achievement/registry transfer, four delayed-SQL buyers, and owner purchase
history. Catalog and world tests cover 34 active producer families. Complete
source references and actual preprocessing prove that the remaining legacy
single-item NPC cash path is compiled out for `PACKETVER=20260219`; it is not
claimed as runtime-tested.

Native regression evidence includes 108 character-save assertions under
ASan/UBSan, 263 guide/readiness cases with 2,256 assertions, and controlled lab
damage checks. SQL tests inject write failures, missing/nontransactional tables,
database kills and reconnects, and verify retry, ownership and replay behavior.
Fixture catalog overlays are declared separately from frozen product source;
handler coverage does not certify the appearance of every shipped NPC.

All timed tests use isolated game/database fixtures on the same host as other
validation. Their results do not establish uncontended production capacity.

## Maintenance and recovery

Before stopping writers, two actual process/state observations under staged
ingress isolation established that players and pending save/shop work were
drained. Services stopped in dependency order. SQL backup restoration was
verified in an isolated database, with identical original/restored SQL digest.
Source, executable and private configuration backups are separate and retained.

Three migrations cover character-save atomicity, registry receipts and purchase
history. Eight legacy MyISAM tables were converted to InnoDB. Old row fingerprints
were checked after migration and again when deployment resumed. File replacement
preserves existing ownership and modes, including the Chapter 1 source file's
non-root owner. The server's configuration and existing container definitions
are retained. Recovery never automatically restores a stale database backup.

## Remaining limits

The host filesystem is 97% used, with approximately 3.2 GiB free after deployment.
The retained task directory occupies about 14 GiB. Backup and evidence cleanup
remains a storage follow-up; the passing service checks do not certify disk
capacity.

The six rendered cases remain unverified: `onboarding`, `encounter-reward`,
`party-reentry`, `shop-storage`, `client-visuals`, and `lab-relog-comparison`.
Computer Use initialization succeeds, but app discovery returns native pipe
OS error 2. Automated and packet checks are not substitutes for rendered tests.
The full release controller must therefore remain failed for missing rendered
evidence, even if operational deployment completes under the user's direction.

A separate pre-existing unlimited pet barter issue attempts zero-value cash
audit inserts with type `J`, which the bundled `cashlog` enum rejects. Asset,
payment, receipt and pet identity assertions pass. The relevant implementation
and schema fragments match baseline, and original error logs are retained in
`cash-log-followup.json` and the pet receipts. This issue remains unfixed.

Detailed evidence is retained under `OPS/improvements-20261002` locally and the
corresponding task directory on the server. SQL archives and private
configuration are not published to Git.
