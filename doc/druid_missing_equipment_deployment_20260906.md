# Missing equipment and map-cache integration

Deployed to `192.168.10.18`, `/app/rathena`, on 2026-09-06. Service restart
timestamp: `2026-09-06T10:21:08Z` (17:21:08 Asia/Bangkok). Deployment returned zero.
This is a bounded extension of the previously installed Druid/Karnos/Alitea
implementation, not completion of the broad episode/gameplay audit.

## Runtime scope

- 12 sourced missing Druid weapons, 12 sourced crowns and 11 required Sky weapons.
- 21 new set memberships: 10 weapon-owned and 11 Sky crown/weapon sets. The four
  Time/Furious crosssets occur only in the weapon overlay.
- 25 original-client target bindings for 24 unique items across 12 existing
  enchant groups. The Time Alitea crown belongs to both 132 and 164.
- Bounded, alignment-safe map-cache file loading/parsing in `src/map/map.cpp`.
  No map-cache file or GRF geometry was changed.

Six new database fragments are imported once each in Renewal mode through the
three existing item/combo/enchant roots. All prior root entries retain order.
The target-only overlay changes no prior item target, recipe, price, normal
weight, reset, refine/grade condition or upgrade. It preserves all 164 groups,
1,293 effective ordinary upgrades and 444 effective guaranteed upgrades.

Item properties/effects and explicit reach assumptions are documented in
`druid_missing_weapons_audit.md` and `druid_missing_crowns_audit.md`; the parser
and malformed-file boundaries are in `map_cache_parser_audit.md`.

## Verification

| Check | Result |
| --- | --- |
| Weapon native VM | 16,380 weapon cases, 110,250 set cases, 1,908 job cases; 16,824,878 assertions |
| Crown/Sky native VM | 89,733 executions; 8,169,504 assertions |
| Full map-cache native regression | 1,408 cases; all 1,313 actual records; 5,492 assertions |
| Gimli with the bounded parser API | 66 cases; 904 assertions; all eight entry/checkpoint cells pass |
| Original Gimli helper, same new native binary | 72 expected assertion failures, confirming checkpoint defect detection |
| Actual import and old-data checks | Supported items/sets wired once, no existing item/set replaced; all target-only invariants pass |
| Strict content integrity | 899 enabled scripts, 110 imports, 71 wired fragments, 99 instances, 54 walkable arrivals; zero errors/warnings |
| Candidate server | `make -j4 server` and `map-server --run-once` return zero; 3,609 OnInit NPCs; leak-free shutdown |

Native tests freshly compile their explicitly declared production units with
ASan/UBSan and deny socket operations. They use documented world/UI boundaries,
not a live player. Candidate startup loaded all six new fragments with exactly
12/11/12 item records, 11/10 combo records, and 12 target overlay records. Its only
warning was the existing root-user warning; no parse/runtime error was found.

The integrated weapon source/Lua/import rerun passed six tests and skipped the
optional native rebuild; the unchanged native inputs had already passed the
separate complete run. The crown integrated check also passed after both sets of
overlays were wired. No optional skip is represented as native execution.

Full reference comparison now has exactly two target-list differences (Booster
group 36 and Frontier group 164) plus ten pre-existing custom Biosphere
normal-outcome differences. Its initial-enchant command deliberately exits
unsuccessfully for these remaining differences. All 339 probability tables have
valid totals; all 2,395 initial selectable recipes remain present. The upgrade
comparison passes: 1,269 original ordinary and 444 guaranteed recipes match,
with 24 existing server-only ordinary upgrades preserved.

## Client installation

Two reviewed loose fragments and four loader import lines were added. DATA.INI,
GRFs, executable files, old import order and final override merge are unchanged.
`tools/ci/druid_missing_client_install_test.py` executes the actual loader in
Win32 Lua 5.1, checking every nested prior field and all exact new records.
Result: **26,865 old records preserved, 35 additions, 26,900 total, 602,841
recursive comparisons**. Its negative deep-comparison control also passes.

The complete original reference helper still has 7,193 unchanged callbacks;
the Chapter 2 extended list has 7,298 callbacks and all 164 groups load. Missing
target metadata decreased from 27 to three. This uses the explicitly documented
2025-reference slot callback model, not protected 2026 native-client execution.

| Artifact | SHA-256 |
| --- | --- |
| Pre-install loader | `5a3f33728795cc6278183688746b2e6efdd96ca1ced28364a7c55699611f1797` |
| Installed loader | `f2628f0aa0b39eae1a984ea5ef56f37a03c1912ed8551d499cec7e1e30e51c38` |
| Crown/Sky fragment | `3cfd17437eb6de75b93d8cbb546628aa426daca6e326c2a0923548250891983f` |
| Druid weapon fragment | `407ddc4121f2b06a01e28fa93b8d15353191533c04a029ad20744ed157bd6048` |
| Unchanged DATA.INI | `45ce90be1cbc6e2af1ebc492ec0ac38713281103d87114deac566b2f344ec025` |

Backup: `../client-before-druid-missing-equipment-20260906/` contains the prior
`SystemEN/itemInfo.lua` and DATA.INI. Reproduction from the repository under WSL:

```sh
python3 -B tools/ci/druid_missing_client_install_test.py --before-loader ../client-before-druid-missing-equipment-20260906/SystemEN/itemInfo.lua --before-data-ini ../client-before-druid-missing-equipment-20260906/DATA.INI
```

## Deployment and recovery

Git baseline: `196dac1c0b85764719377769395ba4b4177119b1`.
The archive `druid-missing-equipment-mapcache-reviewed-20260906.tar.gz` contains
exactly ten runtime source/data files: map.cpp, three database roots and six new
fragments. Archive SHA-256:
`faa6ba10b3198ba2b9ee4b209a7bc1ff95e615e4e7fd1dc5aa2ac087b0b6fe84`.
All ten before-state and candidate/deployed after-state manifest checks pass.

Only map-server's binary changed, from
`1c1613645c63df940fff728cfd0615058d94d47d230ee05e99cfd559d59d3a30`
to `2c796cf83da3468f781c71d67efd918839dbc0171de512580962d34d80583c39`.
Login/char/web binaries are byte-identical to the previous deployment.

Zero online characters were verified, then checked again after login admission
stopped. Map/char/web were stopped before a full database snapshot. Retained
artifacts under `/app/rathena-deploy-backups/`:

- `pre-druid-missing-equipment-mapcache-20260906.tar.gz`: about 34 MiB, previous
  three roots, map.cpp and map-server binary.
- `pre-druid-missing-equipment-mapcache-20260906.sql`: about 22 MiB, full database.
- Reviewed archive/manifest, `deploy-druid-missing-equipment-mapcache-20260906.sh`,
  `map-cache-build-20260906.log`, and the matching startup/deployment logs.

Automatic scoped rollback was available but not needed. It would restore the
old roots/binary and leave new fragments unused; it never restores SQL or deletes
unrelated files. The existing dirty server checkout was not reset or pulled over.

Post-deployment checks independently revalidated all ten files. All four services
reported running, not restarting, with restart count zero. Login/char/web reported
ready; map authenticated to char and reported online. Log scans found no errors,
only existing root-user warnings. The GM remains group 99, and all fifteen
previously configured maximum reputation values were rechecked unchanged.

## Remaining work

Booster weapons 510200/620064 and Frontier crown 401195 remain absent because
complete authoritative definitions were unavailable. Item-specific weapon reach
remains an explicit project-default assumption. No new acquisition recipe,
vendor, drop, item distribution or NPC menu is included in this batch. Missing
Grademk service entry points and the Alitea omission in the custom Biosphere
crown crafter are separate follow-up tasks. Actual gameplay, rendering, autocast
targeting and full persistent instance/reconnect behavior remain unverified.
