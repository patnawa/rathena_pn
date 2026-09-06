# Druid recipe/Monolith follow-up deployment - 2026-09-06

Deployed at 14:56 ICT to the Docker server, with no online characters. The
six-file runtime archive contains the Renewal enchant import, its new Druid
overlay, and four Monolith/Nova/Stomp source files. No other live source files,
account records, client tables or GRF priorities were changed by this batch.

## Verification

- Candidate `make -j4 server` and `map-server --run-once` exit 0, packet version
  20260219. Startup reaches ready, executes OnInit for 3608 NPCs and reports no
  memory leaks. No parser/database errors; warnings are the existing root-user
  warning and Compose version/orphan notices. No orphan containers were removed.
- Six-file manifest checks passed before installation on both live and candidate
  trees and matched exact installed bytes afterward. The live dirty checkout was
  preserved outside this scope.
- All four core binaries were copied from the verified candidate. Login, char
  and web hashes remain unchanged; only map-server changed in this follow-up.
- Login/char/map/web containers started at 07:56:53 UTC, each running without
  restarting and with restart count zero. Map authenticated to char and is online.
- Local strict audit: 899 scripts, 99 instances, 29535 item identities, 99 DB
  imports, 60 wired fragments and 52 walkable service/arrival cells; zero warnings.
- Ten recipe tests; all 339 probability tables and 33.9 million sanitized draws;
  43 source-compiled Monolith checks; 24 upgrade/alias tests; eight initial-enchant
  audit tests; all 31 original-client item identities validated read-only.
- Separate isolated real-VM test: 13 builtin calls and 21 native assertions,
  AddressSanitizer clean. It does not connect to this server. See its explicit
  [test boundaries and allocator investigation](../tools/ci/native_script_vm_README.md).

## Artifact hashes and recovery

Archive: `druid-recipes-monolith-20260906.tar.gz`, SHA256
`10af380edefa4e1d582d5b510bb3f1dfd72fa825fa828527f85d895729c758f7`.
Manifest: `druid-recipes-monolith-manifest-20260906.json`, based on Git
`4ab4f34a3366d2f6a6e479f1ff3d85b9bbd048db`.

Installed map-server SHA256:
`1816c73335d3a0fa171fe459e068239a68ea7b724a96e5130491c2a4116e6838`.

Backups retained under `/app/rathena-deploy-backups/`:

- `pre-druid-recipes-monolith-20260906.tar.gz` (44 MiB): previous scoped files
  and all four core binaries.
- `pre-druid-recipes-monolith-20260906.sql` (22 MiB): database snapshot after
  stopping the core services.
- `druid-recipes-monolith-build-20260906.log` and
  `druid-recipes-monolith-deploy-20260906.log`.
- The archive, manifest and deployment script are retained there as well.

The deployment script has a scoped restore/start error trap; it was not needed.
The newly added recipe file would remain as unused evidence on rollback because
the restored import list would no longer load it. No data or backups were deleted.

## Remaining limits

Client interaction, skill animation/damage, material charging, acquisition for
the new stones and relog persistence remain unverified. The initial-enchant
report now shows 25 shared-group differences plus two entirely absent groups,
rather than silently omitting the missing groups. All 40 remaining unresolved
identities exist in the original client table but lack server records. These
findings are not resolved by this deployment; the broad gameplay audit remains
active.
