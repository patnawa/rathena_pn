# Shop recovery release — 29 September 2026

**Deployed successfully at 19:23:35 Bangkok / 12:23:35 UTC.** All seven services
are healthy; the other five service identities/processes remain unchanged.
See [implementation and limits](shop_purchase_recovery_20260929.md).

The implementation and tests were pushed in Git commit `abaa1e34e` on
`codex/server-improvements-20260919`. This receipt completes the live record.

The release contains 16 native source/header changes, one migration and rebuilt
map/character binaries. It preserves all live NPC/database files and existing
login/web binaries. The prior card-removal script changes in the audit workspace
are excluded; only the new pending-purchase script fence is added to live.

Validation: 55/55 Ubuntu checks; production-runtime shop30, delivery31, planner26
and acknowledgement suites; real MariaDB rollback/restart/concurrency/ownership
proof using production character objects; both production binaries start cleanly
against isolated SQL. Eight deployment-controller failure-injection cases pass.

The 55-check workspace gate also covered three preceding NPC audit candidates
that were excluded from this release. The pushed shop-only gate registers the
52 applicable checks. Unrelated workspace changes were preserved.

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

## Verified live result

- Map and character services restarted gracefully with the exact hashes above.
- All 41 pre-existing financial/storage groups match before shutdown and after
  restart. Final verification covered 42 groups including the new empty receipt
  table. No players were online during cutover or verification.
- `char`, `inventory`, `acc_reg_num`, `market`, `barter`, `sales` and
  `pn_shop_commits` all report InnoDB. Existing stock quantities are preserved.
- Refreshed LAN health passed at `2026-09-29T12:23:49.299491+00:00`.
- No production player purchase was performed as a smoke test. Pet entitlement
  recovery remains outstanding; new stock-backed pet purchases fail before
  payment.

## Backup and rollback

Restore-tested backup (142 tables):
`/app/rathena-database-backups/ragnarok-20260929T122301477362Z.sql.gz`.

Archive SHA-256:
`cfddb96bab469e4eb1e73e6e441783db70ff0528878d5e705c1950c9cbeabcab`.

Prior binaries/sources and ownership/modes:
`/app/pn-shop-recovery-20260929/deployment/before`.

For a code rollback, require no online players, stop map then character and
verify both stopped. Restore the manifest's prior files and ownership/modes;
remove only release-added files identified by a null preimage. Start character
then map and recheck readiness, running hashes and health. Keep the migrated
InnoDB schema and current receipts/player data. No rollback or database restore
was needed during this deployment.
