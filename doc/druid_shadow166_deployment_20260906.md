# Druid Shadow 166, crown metadata and Episode 21 deployment

Deployed 2026-09-06 at 16:27 ICT (`09:27:34Z`) to the authorized Docker host.
This is a bounded verified batch, not a claim that every episode or the complete
Druid class has been playtested. Earlier Druid/Karnos/Alitea class work remains.

## Runtime changes

- Eleven new Alitea Shadow/Soul items, two exact Master-partner job overrides,
  seven native set combinations and eighteen original-client group-166 recipes.
  The three new fragments are imported once each in Renewal mode.
- `pc_isItemClass` recognizes trait-era expanded jobs for Fourth item filters.
  Job, level, sex and broken-item restrictions remain independent and intact.
- Existing Shadow Gear Enchanter at `grademk 40,184`: Open=1 and Cancel=2 retained;
  Alitea option=3 opens group 166 after dialogue closes. No NPC-side item grants,
  charges or new equipment distribution economy.
- Hyper Novice Sky Rune Crown/Napalm Sword combo now registers Chain Lightning
  damage against its canonical skill key. Conditions and amounts are unchanged.
  Cardinal's existing parent/dummy entries were tested and left unchanged.
- Three Episode 21 scripts receive the reviewed Tris report, party portal,
  one-shot wave and suspended-dialogue guards. Rewards are unchanged.

Detailed evidence is in `druid_shadow166_audit.md`,
`enchant_target_metadata_audit.md` and `episode21_encounter_flow_audit.md`.

## Verification

| Check | Result |
| --- | --- |
| Shadow 166 focused suite, all external/native options | 10/10 pass, no skips; 184,027 assertions and 2,662 refine-triple cases |
| Actual equipment eligibility | 159 jobs x 128 masks = 20,352 cases; no prior eligibility lost, exactly 160 intended additional combinations |
| Actual crown item/combo VM | 44,205 executions / 221,352 assertions; both canonical and dummy damage queries and all refine/grade/learned-level gates |
| Actual Shadow NPC VM | Four paths / 162 assertions; exact group routing, cancellation and unchanged inventory/zeny |
| Actual Episode 21 VM | 43 cases / 252 assertions; hash-exact previous scripts reproduce 89 failures |
| Full enchant unittest discovery | 58 pass, 13 optional external/native skips; focused new native tests run separately without skips |
| Final strict integrity audit | 899 enabled scripts, 99 instances, 104 imports, 65 wired fragments, 54 walkable arrivals; no errors or content warnings |
| Fresh candidate build and final startup | `make -j4 server` and `map-server --run-once` succeed; no parse/runtime errors; clean allocator teardown |

Native tests use ASan/UBSan and mandatory network-denial boundaries. They do not
replace a live client playthrough. Candidate warnings are only the pre-existing
root-user and Compose-version/orphan notices; no orphan containers were removed.

## Scoped server installation and recovery

Baseline Git revision: `53df51fe3b662b973853eee8763500e7f4ee86ca`.
Archive `druid-shadow166-reviewed-20260906.tar.gz` contains exactly twelve runtime
source/data/NPC paths, with SHA256
`88607b8629000ec80e1d1e81c87dfcf75928317145395b33fea5dca6b38ffb88`.
The associated manifest rejects unrelated live/candidate changes and verifies
exact installed bytes. Pre-install and post-install checks both pass.

Only the newly built `map-server` binary changed. Its SHA256 is
`1c1613645c63df940fff728cfd0615058d94d47d230ee05e99cfd559d59d3a30`.
Previous map binary SHA256:
`0d08784ce13733bd2c5fa3703ec2daf19b7adc1cca67e83696de23d14ab9a742`.
Login/char/web binaries remain byte-identical to the prior deployment.

Recovery artifacts, retained outside Git under `/app/rathena-deploy-backups/`:

- `pre-druid-shadow166-20260906.tar.gz`: 34 MiB scoped original files/map binary.
- `pre-druid-shadow166-20260906.sql`: 22 MiB consistent database snapshot.
- Reviewed archive/manifest, deploy script, build/startup/deploy logs.

Login was stopped first and zero online characters verified again before stopping
map/char/web. SQL was backed up before file installation. The guarded deployment
has scoped source/binary rollback; it does not automatically restore or overwrite
SQL. New unused fragments may remain after rollback with original imports restored.

All four existing containers restarted successfully, reported ready, remained
running without restarts, and map authenticated to char and reported online.
Post-start logs contain no errors, only the existing root warning. The GM remains
group 99; all fifteen previously set reputation values were rechecked unchanged
at their configured maxima. No account/database mutation was made in this batch.

## Client installation and retained checkpoints

Only two reviewed loose Lua fragments and their two file/table import pairs were
added. DATA.INI, all original GRFs, Chapter 2 GRF and executables are unchanged.
Existing import order and final override merge are preserved.

| Loader checkpoint | SHA256 |
| --- | --- |
| Before crowns: `../client-before-enchant-crowns-20260906/SystemEN/itemInfo.lua` | `6db791e0ea302b71a6cc13d068ef774f2d0c8cb885ceea96b3fd8626a5672811` |
| After crowns/before Shadow: `../client-before-druid-shadow166-20260906/SystemEN/itemInfo.lua` | `6f26c390128be7b6620bf7727f85b47d0151af56db6eb8275578ad93511b7333` |
| Actual active loader after both | `5a3f33728795cc6278183688746b2e6efdd96ca1ced28364a7c55699611f1797` |

Both backups also retain unchanged DATA.INI. Crown fragment SHA256:
`8ab7c0eafa30bf95918f24c2a799eb2d63ef18fe0a6884d2c61d21cd2bcf2772`.
Shadow fragment SHA256:
`f77e9c0f329f2ae8f455a056dfb5cdffffbda605d849639eae2fd042b0551393`.

The actual active loader passed crown verification immediately after that step:
exactly twelve one-slot records added, all prior fields deeply preserved. The
Shadow installation then passed its own actual-active verification: 26,854 prior
records preserved, exactly eleven zero-slot additions, 26,865 final records,
601,146 recursive comparisons. Descriptions agree with reviewed server effects;
Shadow icons deliberately use existing generic art, not claimed official artwork.

Reproduce the active eleven-record check:

```sh
python3 tools/ci/druid_shadow166_client_install_test.py --before-loader ../client-before-druid-shadow166-20260906/SystemEN/itemInfo.lua
```

The earlier strict crown/Chapter-2 checkpoint verifiers must explicitly select
their retained after-loader following later additions. Their output labels that
loader non-active. The active full-list reference test still executes all 164
groups and preserves all 7,193 original callback records (7,298 patched total).
It uses the inspected unprotected 2025 slot fallback, not a claimed protected-2026
native engine. Remaining missing metadata falls from 43 to 27; 28 fallback queries
remain because one missing identity is used across groups.

## Remaining audit work

All 164 client enchant group IDs now exist on the server, but full initial-enchant
comparison deliberately still exits nonzero: 13 target-list differences involving
27 missing server equipment identities, plus ten intentional custom Biosphere
outcome differences. Probability totals are valid. All 1,269 ordinary and 444
guaranteed upgrade recipes match, with 24 server-only custom recipes preserved.

Gimli stage-aware reconnect routes and Ghost Ship once-per-run/participation
policy remain separate findings. No automatic cleared-instance destruction or
reward policy was introduced. Actual visual NPC/client checks, combat playtests,
charging and persistence are still required. The broad audit goal remains active.
