# Allocator and Shadow service deployment - 2026-09-06

Deployed at 15:18 ICT with no online characters. Runtime scope is exactly four
files: `src/common/malloc.cpp`, the root enchant import, its new group-128 overlay,
and `npc/custom/grademk_services.txt`. All four core binaries were rebuilt because
they share the allocator. No account data, SQL schema, client file or GRF priority
was changed. Unrelated live source modifications and all prior backups remain.

## Build and readiness

The existing isolated candidate retained packet version 20260219. `make -j4
server` and `map-server --run-once` exited 0. The latter reached ready, executed
OnInit for 3609 NPCs and reported no memory leaks. Its native database/NPC loader
reported no errors; the existing root-user and Compose version/orphan notices
remain. No orphan containers were removed.

The four-entry manifest passed before installation on both live/candidate trees
and matched exact bytes after candidate and live installation. The deployment
stopped login, rechecked zero online characters, stopped map/char/web, captured
the database snapshot, installed scoped sources and matching binaries, and
restarted the four exact existing containers.

At 08:18:56 UTC all four containers started and subsequently reported running,
not restarting, and restart count zero. Login/char/map/web reached ready, and map
authenticated with char and became online. The brief login-disconnect warning
belongs to the intentional stop; post-start warnings are the existing root-user
notice. The scoped restore/start error trap was not needed.

## Recovery artifacts

Under `/app/rathena-deploy-backups/`:

- `pre-allocator-shadow-20260906.tar.gz` (44 MiB): previous scoped sources and
  all four core binaries.
- `pre-allocator-shadow-20260906.sql` (22 MiB): database snapshot after core stop.
- `allocator-shadow-reviewed-20260906.tar.gz` and
  `allocator-shadow-reviewed-manifest-20260906.json`.
- `allocator-shadow-reviewed-build-20260906.log`,
  `allocator-shadow-deploy-20260906.log`, and `deploy-allocator-shadow-20260906.sh`.

Archive SHA256:
`1a511cc6ecd40359f44c1ac39653f191adb34639b1661eeacb1ea13b0cddbc4b`.
Manifest baseline: `78de18356dfb0ebf9011fc3b90daef358eb2b5af`.

Installed binary SHA256 values:

| Binary | SHA256 |
| --- | --- |
| login-server | `4c6e67a12b1c9e89b96eeec8e1d62adfbcd5f7ffee6fb16fa65b4b6dd7ab868d` |
| char-server | `6d07b1170ad144d1e3d7718e39f49868c22fa061df323c15d418366fe295aab4` |
| map-server | `0d08784ce13733bd2c5fa3703ec2daf19b7adc1cca67e83696de23d14ab9a742` |
| web-server | `2e554707bf580b09a4d39d4c47d3d38acc9273903b0ef074dbd72f1cd5487f40` |

## Verification limits

Local strict audit passed with 899 scripts, 99 instances, 29535 item identities,
100 DB imports, 61 wired fragments and 54 walkable arrival/service cells.
Focused tests passed: normal/debug allocator matrices (18774 assertions each),
intentional allocator shutdown (105 assertions each), real inventory script VM
(13 cases/21 assertions, ASan+UBSan), nine group-128 tests including 2.4 million
native selector draws, three structural service checks, 24 upgrade/alias tests,
eight initial-enchant audit tests and ten Druid recipe checks.

The Shadow Gear Enchanter is at `grademk,40,184`. No materials/equipment are granted
by this service; existing native costs remain authoritative. Actual client menus,
charging, relog persistence, sustained allocator load and the broad instance/
equipment/quest corpus are not proven by startup or these focused tests.
The overall goal remains active. Initial audit still has 26 reported issues;
Chapter 2's missing client tables and identities need a separate client patch.
