# Maintenance and release completion — 5 October 2026

The [Midgard client release](https://github.com/patnawa/rathena_pn/releases/tag/client-2026-10-05-midgard)
is published with nine assets: three complete-client volumes, the smaller
launcher installer, the bootstrap, installation instructions, checksums and
two manifests. Every GitHub-reported asset size and SHA-256 matches the local
verified package. The public small-package download was downloaded again,
checked and installed into an isolated client with the official offline
OpenSetup ZIP. The source and illustrated README are pushed on
`codex/improvements-20261002`.

## Backup recovery

The October 5 startup backup failed while the database was unavailable. The
service depended on Docker readiness and did not retry; its newest failed
report correctly kept backup health red even with older valid backups present.

`database_backup.py` now waits up to 120 seconds for the database to authenticate
and answer `SELECT 1` before starting a dump. Probe timeouts are bounded and
passwords remain inside the container environment. A timeout publishes a
failed report with the failing stage and exception type, and publishes no dump.
The systemd service retries failures after five minutes with no start-rate
limit. Failure reports remain visible until a new verified backup succeeds.

The regression exercising startup readiness failed on the old implementation
and passed after the repair. All six backup tests and eleven health tests pass.
The service configuration was checked by `systemd-analyze verify`.

The installed service completed a real backup and restored it in a new
networkless container. All 149 tables passed integrity checks. Original and
restored SQL SHA-256 both equal:

`4ac51f495aa1eb8126a0133310fd894661b698a76adaafed7e8913e12c6c00f7`.

The archive SHA-256 is
`150e4a61e97bd25fd0109546cb195d962c1e6f13f804d0c1c54265982672ab06`.
The off-host copy was verified by archive and decompressed SQL hash. The latest
backup is `ragnarok-20261005T071348500012Z.sql.gz`; credentials and SQL content
are not published. The live status page now reports all game services online,
fresh successful maintenance health and a verified backup.

Two root-owned health systemd unit files had mode 0666. Their contents were
preserved and their permissions set to 0644. Previous script/unit files are
retained in `/var/backups/pn-operations/backup-fix-20261005`. Game containers
were not restarted for this maintenance operation.

## Disk cleanup

Local cleanup removed 5,662 files from the obsolete October 3 QA client:
5,364,077,140 payload bytes. Every file was checked against the retained
October 4 QA client or the ZIP preserving its six differing files. The current
QA client and resume evidence remain available.

Another 11,364 disposable release files were removed after publication:
16,114,516,408 bytes of staging, extracted verification copies and uploaded
full-client volumes. Release checksums, manifests, logs, the small installer
and publication evidence remain local. The full volumes are available from
the verified GitHub release. Total removed local payload is approximately
**20.00 GiB**; this includes staging created for the release in this session.
Some empty temporary directories remain because automatic approval review
blocked their removal. Their file contents were removed successfully.

On production, eight inactive metrics fixture `candidate` directories were
archived off-host before removal: idle/load, Linux/final, attempts 1/2 under
`/app/pn-improvements-20261002/evidence/`. Every archived file hash and both
source inventories matched. All Docker mounts, including stopped containers,
and native process references were checked before deletion. The cleanup
removed 52,760 files and recovered **3,920,441,344 free bytes (3.65 GiB)**.

The verified private archive is
`C:/Users/Alpha/PN-Server-Backups/cleanup-20261005-midgard/server-inactive-benchmarks.tar.gz`,
1,149,991,245 bytes. Its SHA-256 is
`cc5483a3c4fa8375a071bdd2927371f772622c8fcabf03e5c5d1c2644be58de6`.
To rerun those archived fixture-specific checks, first restore their candidate
directories from that archive beneath `/app/pn-improvements-20261002`.
Measurement reports, the latest QA candidate, databases, runtime source and
production rollback files were retained. Current production disk availability
is approximately **231.6 GiB**. No game service was restarted for cleanup.

## Remaining client acceptance

The planner/login production deployment was already complete; its
[October 5 receipt](planner_login_followup_20261005.md) supersedes older pause
notes. Completed deployments, migrations and accepted Damage Lab checks were
not replayed.

Six original rendered cases still need genuine in-game observations:
onboarding, encounter/reward, party reentry, shop/storage, client visuals and
lab/relog comparison. The existing detailed checklist is
`OPS/improvements-20261002/readiness-rendered-plan.md` outside this source repo.
This session does not expose the Windows `node_repl` Computer Use tool, so it
cannot complete or certify those GUI observations. The October 4 isolated QA
client is retained, but its stopped private services and expired tunnel must
be revalidated before use. User authentication must be performed by the user.
The historical full-controller result remains incomplete for those receipts;
automated tests are not substituted for them.
