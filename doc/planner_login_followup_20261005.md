# Planner and login follow-up — October 5, 2026

The Equipment Planner now counts character-owned storage using `char_id` and
account-owned storage using `account_id`. Its dialog clears previous pages,
shows public NPC names, retains save/remove feedback, and restores recipe
context after source lookup. The implementation is commit `e708d8e76`.

Login roster decoding now reads the sender's 32-bit user count at offset 4 and
account IDs at offset 8. The previous decoder read IDs at offset 6 and could
create phantom online accounts. Malformed lengths/counts are rejected before
state changes; partial packets wait for completion. Offline cleanup now updates
stored entries and advances the iterator before erasing them. These fixes are
commit `a489cd279`.

## Validation

The planner's actual holdings tests passed 32 cases with 302 assertions. The
NPC VM checks passed 22 cases with 255 assertions. Actual SQL/VM replay verified
20 inventory vouchers, 30 eligible storage vouchers, and 50 missing vouchers.
The login regression exercises actual source across eight cases; all eight
failed against the old implementation and passed against the repaired source.

The frozen candidate source identity is
`f29a5d321deb2afe655eea4a1dfbc1f9b1840ae7f6a9446c74e1b450b4a8504d`.
Its map executable is
`ecb6a078e52562bba572377017cf90c72e29137077777d3b923abdd613397093`;
its login executable is
`ef1bb877d8e57ea354b0aa515fb35dbf9fbb8b611e86f8b3e31bd4c8847bc27d`.
The character and web executables remain unchanged.

Private runtime verification passed without restarting its processes. A fresh
read-only process observation verified the runtime identity and an empty
map/character/login census before removing the private admission fence. Its
receipt SHA-256 is
`2f45f7867d846fc4850710329fb2f49f4f589e9dc831016204e31c058d67620b`.
Evidence is retained under `OPS/improvements-20261002` outside this source repo.

## Deployment status

Production deployment completed on October 5. The source and executables above
are installed, and the loaded production runtime identity was verified as
`8a2e7ea74e07050d81cd8a2e4a62c756a6ddaa8ce32279ee859d4ab3d985639d`.
All seven production containers are healthy. Player admission is reopened;
ports 6900, 6121, 5121, 8888, and 8080 are reachable from the operator PC.
The original `unless-stopped` restart policies are restored.

Map and login were restarted for their new executables. Character was also
restarted to refresh its cached login address after the host reboot; character
and web executable identities remain unchanged. Current map/character/login
connections, a fresh empty census, and the in-process runtime were verified
before reopening. No SQL migration was performed.

Two deployment log retrieval/framing issues were repaired through separate
guarded recovery helpers. Earlier failure records and backups were preserved.
The final production report SHA-256 is
`642813990a535d99dd1a8f59e322ebfb3015472fc5e07045f1e5dd64fdcdc818`.
Its initialized-runtime receipt SHA-256 is
`39b5353c93a5802a6f079d773f091bca146b413c7553022da2d25bc1a2efb1c3`.

Current-build in-game visual acceptance remains pending. Automated and runtime
checks do not replace those observations. The historical full controller result
and its acceptance limits remain unchanged. The previously accepted Private
Damage Lab audit was not repeated.
