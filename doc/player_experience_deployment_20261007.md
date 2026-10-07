# Player experience production deployment — October 7, 2026

The four player improvements are live. Production verification completed at
20:04 Bangkok time on October 7, 2026. The deployed implementation is
`c76c4cf2b0899a0118b573a636031755043d04f2` on `improvements-20261002`, pushed to origin before rollout.

Use `@goal` for pinned upgrade/material progress, `@prepare` for saved gear,
retail supply restocking and destinations, `@partyboard` for join requests and
ordinary invitations, and `@challenge` for the rotating private expedition.
[Player guide](player_experience_20261007.md) describes their rules and limits.

## Validation and cutover

The exact committed feature payload passed the map-server build and all
104 full native release checks, including private whole-NPC startup.
Staging was aligned with the existing production character-server binary and
import map cache before that final run. Its database was disposable and private.
The original staging configuration was restored after validation. The candidate
and deployed runtime source identities are separate: production uses its existing
inputs and three effective Docker configuration overlays.

Read-only production preflight found no undeclared native input drift and zero
online characters. The existing guarded cutover fenced gameplay ingress,
collected a fresh quiescent process census, gracefully saved and stopped only
the map server, backed up every existing replaced file, installed 24 committed
files plus the map binary and generated runtime manifest, then restarted it.
Its fresh runtime heartbeat and startup health passed before ingress reopened.

Postflight verified the actual running executable, all installed file hashes,
backups, every runtime-manifest input including Docker overlays, and all seven
healthy production services. Public status reports the game online. The existing
signed client feed remained byte-identical and its signature verified. This
release changes no SQL schema or client assets.

## Pilot acceptance and recovery

The expedition remains a pilot with cosmetic records only. Native event and
transaction checks do not establish rendered client appearance or combat balance.
Actual client playtesting, invitation acceptance and restock acknowledgement
observations remain pending; no rendered acceptance or full controller
certification is claimed.

The deployment and exact backups are retained at
`/app/pn-player-experience-20261007/production-cutover`. The reviewed adapter
supports `release_deploy.py --rollback`; it rechecks file ownership and hashes,
fences ingress, requires a fresh zero-player quiescent census, gracefully stops
the map writer, restores saved preimages, verifies readiness and restores ingress.
Inspect the current journal and any later deployments before invoking recovery.

[Deployment receipt](evidence/player_experience_deployment_20261007.json)
contains only hashes, aggregate health and source identifiers. Detailed local
and server logs are retained under `OPS/player-experience-20261007` and
`/app/pn-player-experience-20261007` respectively.
