# Shop transaction deployment — 29 September 2026

The shop delivery, capacity and payment-validation fixes are live as of
**18:20 Bangkok / 11:20 UTC**. All seven services are healthy, the refreshed LAN
health report passes, and 41 financial/storage data groups are unchanged.

## Installed scope

The release replaces `map-server` and four matching source files:
`src/map/pc.cpp`, `pc.hpp`, `npc.cpp` and `cashshop.cpp`. It includes callback
deferral during shop output batches, metadata-aware plain-item capacity checks,
duplicate/quantity/price guards, corrected barter arithmetic and payment order,
and the shared cash-payment validation fixes from the preceding audit.

The candidate was built from the current live tree, preserving its NPCs and
databases. A comparison of 3,352 native input files confirmed that only the four
listed sources changed. The earlier card-removal/enchanting NPC repairs and
client tooltip candidates remain local; this deployment does not publish them.
No schema migration was required. Existing login, character and web binaries
and the runtime image remain unchanged.

## Verification

- Clean map build against the live Alpine runtime, packet version `20260219`.
- Both 30-case native shop suites passed, including real quest, achievement and
  equipment-script delivery timing, with clean allocator teardown.
- This production-toolchain run explicitly used UBSan. Its known broken ASan
  linker was not counted as an ASan pass; the separate Ubuntu audit previously
  passed ASan/UBSan delivery coverage and the broader 53-check gate.
- The exact deployment binary passed startup using the pinned production image
  and a disposable database on an internal network, with no startup errors.
- Five offline deployment-controller cases passed: normal install, startup
  failure, stop failure while running, failure after stopping, and partial copy.
- A fresh SQL backup was restored and checked in isolation: 142 tables passed.
- No characters were online during cutover. Only the map service restarted;
  the other six services retained their identities, processes and policies.
- Installed files and the running executable under `/proc` match the release
  manifest. All 41 checked financial/storage groups match before and after the
  restart, including wallet, inventory, registries, storage, receipts, mail,
  pets and shop stock. No database restore was performed.
- Final LAN health refreshed successfully at 11:21 UTC.

Running map SHA-256:
`d20f8e4a713d017d4b64267593cad4fd2b55c943955e87b49b0f14406437cbda`.

Evidence is retained in `Server-Development/transaction-deploy-20260929` and
`/app/pn-transaction-deploy-20260929`: `manifest.json`, `build.json`,
`native-inputs.json`, `validation.json`, `controller-tests.json`, `backup.json`,
`deployment.json`, `final-health.json`, and validation/live restart logs.

## Rollback

Original files and ownership/modes are retained at
`/app/pn-transaction-deploy-20260929/deployment/before`. The controller refuses to
overwrite an existing rollback directory. To roll back later, verify no online
characters, stop the map gracefully and confirm it is stopped, restore exactly
the five manifest files with recorded metadata, then start the map and verify
health and the running executable. Preserve current player data; an ordinary
code rollback must not restore the old SQL dump.

Restore-verified SQL backup:
`/app/rathena-database-backups/ragnarok-20260929T111952916612Z.sql.gz`.
Archive SHA-256:
`5114237621afe763025d23aa6b3081d02c3460e9fe15f635c1b8c22310f4d8bb`.

The separately documented [SQL stock and asynchronous pet delivery defects](transaction_persistence_audit_20260929.md)
remain unresolved. This release improves synchronous shop delivery; it does not
introduce durable purchase receipts or claim crash/reconnect atomicity.
