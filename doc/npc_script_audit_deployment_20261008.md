# NPC script audit production deployment — October 8, 2026

The NPC audit repairs are live. Production verification completed at 12:46
Bangkok time on October 8, 2026. The deployed implementation is
`f1eacde1805a0e808d0b98d4c0f8ddef151f1be8` on `improvements-20261002`, pushed
to origin. Server-side journals for this cutover record the same tree under its
pre-publication commit ID `f63cd9ee05350e7b96d6993cf82a14bb1664a5d1`. A
[follow-up release](npc_script_audit_followup_deployment_20261008.md) went live
the same afternoon.

The repairs are described in the [audit record](npc_script_audit_20261008.md).

## Validation and cutover

The exact committed payload passed the map-server build and all 105 full native
release checks, including private whole-NPC startup and the new
`npc_audit_20261008_test.py` regression. The candidate was staged from the
previous deployed candidate, confirmed against the commit's parent for every
changed file, and aligned with production's character-server binary and import
map cache. Its fixture database was disposable and private, and the candidate's
original configuration was restored afterwards.

Read-only production preflight found no undeclared native input drift and zero
online characters. The existing guarded cutover then:

1. fenced gameplay ingress;
2. collected a fresh quiescent census;
3. gracefully saved and stopped only the map server;
4. backed up every replaced file;
5. installed 55 committed files, the map binary and the generated runtime manifest;
6. restarted the map server.

Its fresh runtime heartbeat and startup health passed before ingress reopened.

Postflight verified the running executable, every installed file hash, the
backups, the runtime manifest including Docker overlays, and all seven healthy
production services. The map startup log contains no errors. Public status
reports the game online. The signed client feed remained byte-identical
(`client-20261007-patcher`, sequence 2026092909) and its signature verified.
This release changes no SQL schema or client assets.

A read-only review of production item and MVP logs found no use of the repaired
exploits, so no player data was changed.

## Pending and recovery

Rendered client playtesting of the changed NPC flows has not been performed.
Two dialogs now show their own confirmations: Missionary Rosetta's second
mission and Cardron's equipment selection. Gold Point dyes are now
account-bound. Each account receives one more full Lake of Fire reward, because
earlier claims were stored as run tokens.

The deployment and exact backups are retained at
`/app/pn-npc-audit-20261008/production-cutover`. The reviewed adapter supports
`release_deploy.py --rollback`. That rollback rechecks file ownership and
hashes, fences ingress, requires a fresh zero-player quiescent census,
gracefully stops the map writer, restores the saved preimages, verifies
readiness and restores ingress. Inspect the current journal and any later
deployments before invoking recovery.

The [deployment receipt](evidence/npc_script_audit_deployment_20261008.json)
contains only hashes, aggregate health and source identifiers. Release scripts
and local evidence are in `OPS/npc-audit-20261008`; server logs are in
`/app/pn-npc-audit-20261008`.
