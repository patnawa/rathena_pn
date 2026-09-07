# Damage-lab creation crash fix

The first player entry created `0335#000001` from `guild_vs1`, then the map
process received a fatal crash signal. Docker restarted it. This was a server
crash, not a client GRF/packet mismatch.

`map_addinstancemap` only initialized flags through `map_data_copy` when flags
were inherited. With `NoMapFlag: true`, a fresh map slot's flag vector remained
empty. NPC cloning/initialization subsequently accessed that vector. The fix
calls `initMapFlags()` on the non-inheriting path, also clearing recycled slots.
The lab retains neutral flags rather than inheriting guild-vs-guild modifiers.

The old binary reproduced the fatal crash with isolated SQL and an ownerless
test instance. The patched binary completed three create/destroy cycles on the
same reusable map slot, executed OnInstanceInit, checked containment/reward flags
and absence of GVG, then reached ready without errors. The test is deliberately
not enabled in production. To repeat in an isolated candidate only:

```
./map-server --run-once --map-config npc/test/pn_lab_lifecycle.conf
```

Require `PNLAB_LIFECYCLE_PASS 3 create/destroy cycles`, no FAIL/Error/Fatal markers,
and successful exit. Startup-only parsing was insufficient for this bug.
Player entry and a full damage rotation remain separate acceptance tests.

Production rollback files and before/after test logs:
`/app/rathena-deploy-backups/pn-lab-crash-fix-20260907/`.

Old binary SHA-256: `794e0c19707ffd741dd32d3c392d3a4f394f9b5f4509ba6d1263ed21fd47f45d`.
Fixed binary SHA-256: `70846387a9a257ddb190fe850baf9a9471dd171c0982dcbfbe76bb3ba02824be`.
No client assets or player tables were changed.
