# Biosphere Alitea crown deployment

Deployed `2026-09-06T11:20:11Z` (18:20:11 Asia/Bangkok) to `192.168.10.18`,
`/app/rathena`. The guarded deployment returned zero. All four services are
running, not restarting, restart count zero, and independently reported ready.
Post-restart diagnostics contain only the existing root-user warning.

## Installed scope

Only `npc/custom/varmundt_biosphere_depth.txt` changed at runtime:

- Previous normalized SHA256:
  `c0e59e6497f0bebef17ee98784ac92a988d54d16cacf41be188fb774f0456d09`.
- Installed exact SHA256:
  `790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df`.

The existing Abyss Researcher at **ba_chess 22,16** now crafts, random-upgrades,
and stat-rerolls Alitea crown 400999 through the explicit nineteen-item list.
All eighteen original menu outputs, Cancel at 19, costs, chances, and reputation
thresholds remain unchanged; Alitea is appended at 20. Snapshot checks reject
changed equipment and access before mutation. Failed mutation consumes no runes;
successful mutation is charged once, including a failed re-equip.

No client file, database definition, binary, account value, or other NPC file
was installed by this batch. The adjacent bulk-conversion path was not changed.
Its separately documented resource-preflight finding remains follow-up work.

## Verification and boundaries

- Fresh native parser/VM/inventory/mutation proof: **4,152 cases, 200,773
  assertions, all 100,000 stat-selection buckets**. Original replacement-crown
  failure reproduced. ASan/UBSan clean, no script errors, no allocator leaks.
- Root independently reran the exact-source retained executable with the
  mandatory callback gate before and after; receipt remained identical.
  A later runner-only CRLF portability adjustment was verified by relinking
  the six unchanged sanitizer objects and repeating the entire gated suite.
  Its latest receipt is documented in `druid_crown_acquisition_audit.md`.
- Local strict integrity audit: 900 enabled NPC scripts, 110 database imports,
  71 wired fragments, 99 instances, 56 route assertions, zero errors/warnings.
  Two old text assertions were updated to the stronger original-snapshot
  mutation call and explicit nineteen-ID output mapping; none were removed.
- Candidate startup passed both with project content and with the actual live
  custom item/pet/Garden files mounted read-only: zero exits, 3,610 OnInit NPCs,
  clean allocator shutdown. This second check represents the preserved runtime
  content, not an assumption that the dirty live checkout equals Git.
- All **3,688** expected live-profile files match after installation. The
  expected-before comparison differed only in the intended crown NPC.
- Default callback profile rejected all 17 negative controls; explicit
  `live-20260906` rejected 22, plus four independent omitted-file checks and
  profile crossovers. See `biosphere_callback_closure_audit.md` for scope.
- Zero online characters were verified after login admission stopped. After
  the other game services stopped, `bonus_script` contained zero rows. This
  separate quiescent SQL check closes the historical persisted-text assumption
  for this rollout; the static manifest never claims to query persistence.
- Post-deployment read-only SQL reconfirmed all fifteen existing GM reputation
  values at their configured maxima, with no online characters or persisted
  bonus scripts. No account adjustment was necessary in this batch.

The tests double transport, player lookup, persistence and the equip/status
world boundary. The separately reviewed current callback content supports the
payment ordering; this is not generic transaction atomicity against arbitrary
future/injected scripts. No visual client or live-player gameplay test is claimed.

## Preserved live differences and binary provenance

Initial default-profile verification correctly refused four existing live
differences. They were reviewed independently and preserved byte-for-byte:
`db/import/item_db.yml`, `db/import/pet_db.yml`,
`npc/re/quests/garden_of_time.txt`, and `src/map/skills/npc/suicidebombing.cpp`.
No broad synchronization or checkout reset occurred. The explicit live profile
pins those four exact files and all other dependencies; it is never selected
automatically after a failure. All 146 crown-fixture item records are identical
between project and preserved live profiles.

All four binaries remained exact. The map SHA256 is
`2c796cf83da3468f781c71d67efd918839dbc0171de512580962d34d80583c39`, from the
earlier equipment/cache build receipt. That build used candidate source, not the
preserved unbuilt Suicide Bombing source condition. Source preservation is not
a claim that this extra condition is running. Its conditional impact and the
preserved Garden routes are recorded separately in `garden_live_drift_audit.md`.
Renewal's compiled NPC root and live `db_path: db` were checked, with no bound
map-config override. Reputation/constant imports were compared separately.

## Recovery records

Git baseline: `f4efb83744a5f697d4938f2c699bd278b0d265ef`.
Reviewed one-file archive SHA256:
`fc7b3cffa3d21aca5c9510bac271e7607b664b3f8d07053ddfd0d7359709a06e`.
Expected live-after manifest SHA256:
`10efce7d6be1d358c74a4baf57c6a2e504d27e01e8b16d6204492823ec2a51fff`.

Retained outside Git under `/app/rathena-deploy-backups/`:

- `pre-biosphere-crown-transactions-20260906.tar.gz`: 6.5 KiB previous NPC.
- `pre-biosphere-crown-transactions-20260906.sql`: approximately 22 MiB full
  database snapshot taken with game services stopped.
- Reviewed archive/manifest, expected-after callback manifest, standalone gate,
  both startup logs, deployment log and deployment script.

Deployment script SHA256:
`300505b50da26fa1269888cb1e2bd6964a67f2eade1fe1b772fd74adde439145`.
Gate SHA256:
`2387012e979e92283affbcdda6c9f1b3782c9f698e8f13f4f321ddeaca8c2ee8`.
Scoped automatic rollback was available but not needed. It restores only the
previous NPC; no SQL restore or unrelated deletion was performed.
