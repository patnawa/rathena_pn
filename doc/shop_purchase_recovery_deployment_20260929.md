# Shop recovery release — 29 September 2026

Deployment is prepared and validated; this receipt will be completed after live
cutover. See [implementation and limits](shop_purchase_recovery_20260929.md).

The release contains 16 native source/header changes, one migration and rebuilt
map/character binaries. It preserves all live NPC/database files and existing
login/web binaries. The prior card-removal script changes in the audit workspace
are excluded; only the new pending-purchase script fence is added to live.

Validation: 55/55 Ubuntu checks; production-runtime shop30, delivery31, planner26
and acknowledgement suites; real MariaDB rollback/restart/concurrency/ownership
proof using production character objects; both production binaries start cleanly
against isolated SQL. Eight deployment-controller failure-injection cases pass.

| Binary | SHA-256 |
| --- | --- |
| map-server | `d640fecb9be3bf3ad6268953c747487fa7c065b14b936394bdb562b010b36b5d` |
| char-server | `fdffdc0bab08c9bd6f27545216374ba8d4dcb66f8b50c0574faebeaf9e320a7b` |

Operational evidence: `/app/pn-shop-recovery-20260929`, mirrored under
`Server-Development/shop-recovery-20260929`. The source attestation checked 3,357
native inputs and found exactly the 16 intended differences.

Cutover requires no online players, a restore-tested database backup, graceful
map then character shutdown, InnoDB stock-table conversion, and installation of
both binaries. Restart character then map. Verify seven healthy services,
unchanged other five services, running executable hashes and existing financial
data. Code rollback retains current database assets and receipts, including
already-converted InnoDB engines. A post-cutover verification failure is reported
for inspection; it is not treated as successful deployment.
