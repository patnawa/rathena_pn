# PN quality services

PN Services is at Izlude **140,146** (all variants) and grademk **46,180**.

## Damage lab

Creates an individual 30-minute instance. Configure size, race, defending element
and level, MDEF and MRES. A three-second countdown precedes approximately 60
seconds of HP-loss sampling. Results use actual elapsed milliseconds, not a
hardcoded divisor, and are recorded through `logmes` when script logging is enabled.
The character's last result lasts until logout. Interrupted runs unlock after
75 seconds; leaving the room or killing the dummy invalidates the run.

This is a normal-class, stationary, status-immune Poring dummy with zero hard DEF
and FLEE. Boss-only bonuses, real boss AI, debuffs and encounter mechanics are not
represented. Damage exceeding its two-billion-HP refill buffer invalidates the
run. HP sampling measures net HP loss, not individual hit damage; healing the
dummy invalidates comparisons. Do not use this to certify real Zero Cell DPS.
For A/B gear tests, keep every other item, buff, target option and rotation fixed.
Repeat at least three runs per configuration. No combat results are prefilled.

## GM preparation

GM level 99 only, opt-in confirmation, current character only. Supports existing
Soul Ascetic and Elemental Master jobs at 275/60, using the documented magic
stats/traits. Restores job body appearance. Does not change class, create a
character, grant arbitrary equipment, reset quests, or grant all server skills.
Existing skills remain; spend earned skill points normally. Obtain consumables
from the adjacent Skill Supplies service. Druid's separate progression is not
treated as a level-275 fourth job.

## Access and client checks

The access menu reports Zero Cell, Chapter 2 and Devoured Geffenia prerequisites
without modifying quest flags. Navigation points to Robin, Newt or Guardian El.
Run `python3 tools/ci/client_preflight.py CLIENT_DIRECTORY` for active GRF order,
presence/signatures and executable inventory. Add `--hash-archives` for an exact
archive receipt. This does not establish the EXE packet date or asset coverage.

Manual acceptance before calling a client compatible:

1. Log in with both configured jobs; check selection/map sprites and relog.
2. Test F1-F9 and letter bindings outside chat input with the intended keyboard
   layout. Record client settings and `/bm` state; do not replace EXEs blindly.
3. Visit Chapter 2 exits, ba_mansion and the relevant dungeon entrances.
4. Equip the tested gear; inspect item/enchant text and cast each combo skill.
5. Capture exact error text, map coordinates, skill/item IDs and screenshot.

## Guarded source releases

Run the quality-services test, strict episode integrity audit, and existing
combat/route regressions. Pack only reviewed source files, including ignored
`db/import` additions explicitly. Use `deploy_scope_manifest.py create` against
the last deployed Git reference. Never bundle credentials, SQL dumps or binaries
into this source-release workflow.

Validate that exact archive in an isolated candidate with `map-server --run-once`;
inspect the full startup log, not merely its exit code. Keep that log beside the
manifest. The deploy helper checks a clean/ready log but cannot prove which
candidate produced it; the operator must associate it with the archive hash.

`guarded_source_deploy.py plan --root /app/rathena --backup NEW_BACKUP_DIR
--archive RELEASE.tar.gz --manifest RELEASE.json` is read-only. Before `apply`,
close login admission, confirm zero online characters, back up SQL separately,
and stop the map container. Apply uses the same arguments plus `--startup-log
CANDIDATE.log`; it refuses a running map container, drift, unsafe archive paths,
checksum mismatches, existing backup directories and unclean startup logs.

The helper creates complete scoped backups and a receipt before installing any
file. It deliberately leaves services stopped. Start map, inspect fresh logs for
errors and readiness, check char connectivity, then reopen login admission.
On failure, stop map and run `rollback --root /app/rathena --backup BACKUP_DIR`.
Existing files are restored; added files move to recoverable quarantine. Neither
command restores SQL or binaries. Do not use this helper for database migrations
or engine upgrades. Interrupted apply is recoverable using the saved receipt.
