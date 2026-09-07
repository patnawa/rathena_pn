# Episode 20/21, Gudra and GradeMK healer deployment — 2026-09-07

## Result

Release `episode20-21-gudra-healer-13-v2-20260907` was validated and deployed
to `/app/rathena` on `192.168.10.18`. Validation completed at
`2026-09-07T09:29:18.748674Z`; the live deployment receipt completed at
`2026-09-07T09:30:41.033516Z` with result `DEPLOY_PASS` and global target state
`POST`.

The immutable successful artifact is:

```text
/app/rathena-deploy-backups/episode20-21-gudra-healer-13-v2-r7-20260907T092736Z
```

Its mode-0700 run directory contains the validation/deployment receipts,
candidate evidence, exact PRE file backup, and a verified locked SQL dump. No
credential is recorded in this document.

## Exact live file set

The release changed exactly these 13 files. The table records each deployed raw
SHA-256 and byte size.

| Path | Raw SHA-256 | Bytes |
| --- | --- | ---: |
| `npc/custom/episode20/Progression.txt` | `1fb4e3b719877f7a0bf25fe4f22fea06b181b95289f6ee8f8844a1593b26920f` | 42,961 |
| `npc/custom/episode20/SidesAndDailies.txt` | `4f43339e03f7ad71165c4b158e83bd5b468bd0b53813443414df3004af1b0e9a` | 25,557 |
| `npc/custom/episode21/BlackHairedBeast.txt` | `b00fdd28cd0ea4703bbbfdfb8a3f7336f74d38be02b93eddde30029996593b53` | 11,549 |
| `npc/custom/episode21/FamilyReputation.txt` | `3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb` | 5,948 |
| `npc/custom/episode21/FinalBattle.txt` | `d93ccfe36f48cccaf041e15d24c6036cc2fd908afe89103bd5a1dd3924558ace` | 17,595 |
| `npc/custom/episode21/GimliInfiltration.txt` | `cb51fe24b554f896a8702f8cd62430b78e3b52ea7f4e4f640c3c36188bd34273` | 20,795 |
| `npc/custom/episode21/MysteriousGhostShip.txt` | `011f74d5b6abc6833be6dbb70cd19c7b123ed057a4a9bbe677528aa85f9318a6` | 17,772 |
| `npc/custom/episode21/Progression.txt` | `812e3a1e7c5a6ffedb4ca9fbb7a98e5160485cf577d834ed2e97a5e7b367b2b6` | 24,462 |
| `npc/custom/episode21/SecretAltar.txt` | `18b816122fd5a3fbb7e4db0d7efcf9533ecec2ebc799cbd0a8f99c87c87f7970` | 7,568 |
| `npc/custom/episode21/SideDailies.txt` | `e031113a8e040385a6d395187d0c00e45f2239e0dd9197299f2c603758cf764c` | 11,133 |
| `npc/custom/episode21/SilentSanctuary.txt` | `e5551f3cceeefa1b2dbf2ef85743c58a282152a83526743d9e1439a143292a0e` | 5,069 |
| `npc/re/quests/quests_18.txt` | `8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe` | 468,343 |
| `npc/custom/healer.txt` | `76efe16c909ee7d24898fe83fb0d4a8726cd324d858d9b0cc1b7ad08d7c8ed52` | 3,779 |

The live readback matched all 13 hashes after service startup. Four server
binaries also retained their pinned hashes.

## Package and receipt chain

| Evidence | SHA-256 |
| --- | --- |
| Generator | `cbc449304689cafa72ce5608f43c11968df656f052aaab584d7ee12fde822aa6` |
| Server validator/deployer | `7489e8795ae4e980f582ae7dc3df0e5c660454020140377a708ac85d9448ffea` |
| `release-checks.json` | `77ee29730734b3e621289b68c5bfc49866ce822a7b19cf5d00ce84efc26d0d96` |
| Runtime manifest | `49c860a2befff822e3e11a1664e738951e8333b3e8e6d32ec7f5b1cd7ebc5905` |
| Runtime archive | `8f9bb531864fa6169d465c7e5797901bad3e4bafed5f45f4f55996595d4f44f8` |
| Authenticated outer r7 archive | `bd9a5d3ad7485e437e039239ba25f0f9f679a12efef5d5228b48d5164375dd44` |
| Validation receipt | `bcb05c8e9d90d4db48f1cfbb7939f9ebc59309519f90bd78b4d9d054d1b1e82b` |
| Deployment receipt | `d78de0d89a7c7d830a17b92c85b7b4acb411c3fe7c6eea40206b65d0926e2bb0` |
| PRE file archive | `6cf8f0ae2694dc817a7172f01e96c18b17894f7abe37f48642731aa579d53996` |
| PRE backup manifest | `e65c9af68e98dd3c463548a1a61c62153a541966c5a857460090e2a477c99e73` |
| PRE SQL dump | `54f62c6c20c729d1f07f2636e34149ff0c0893ef72d41783d19750921e6cb9e5` |

The SQL checksum sidecar verified the dump. The package verifier accepted only
the exact 13-member runtime allowlist and five pinned native-test receipts.

## Runtime and data postchecks

The isolated candidate and live POST startup each passed structural checks.
The live map log contained exactly one of every required marker:

- 1,282 maps;
- 27,079 NPCs, including 4,330 warps, 374 shops, 22,375 scripts and 4,205
  spawn sets;
- 79,293 cached mobs and zero uncached mobs;
- 3,735 `OnInit` executions;
- one `Map Server is now online.` marker and no error/fatal diagnostic.

The exact `docker_default` membership after deployment was `rathena-login`,
`rathena-char`, `rathena-map`, `rathena-web`, `rathena-db`, and
`rathena-fluxcp`; all six were running and no `ep13v2-*` candidate container,
network, or volume remained.

All guarded SQL snapshots were byte-identical across the stop/install/start
boundary. They recorded zero online characters, zero `bonus_script` rows, 246
market rows, and 46 stable map-registry rows. The exact offline GM identity was
`MSCESXi`, `char_id=150000`, `account_id=2000000`, login `admin`, group 99.
Its complete 15-key reputation set was already present and remains at every
configured maximum:

- `RepPointsOrc=3000`, `RepPointsGoblin=3000`, `RepPointsWolf=5000`,
  `RepPointsIsgard=3000`, `RepPoints6=5000`, and `RepPoints9=5000`;
- `REP_EP21`, `REP_EP21_Nerius`, `REP_EP21_Heine`, `REP_EP21_Lugenburg`,
  `REP_EP21_Walter`, `REP_EP21_Wigner`, `REP_EP21_Richard`, and
  `EP21_Reputation` at 1000;
- `CH2_Reputation=300`.

No reputation write was necessary because the exact live rows were already at
their maxima.

The live source contains exactly one GradeMK healer at
`grademk,24,184` and the warper route to `grademk,34,184`. The retained Druid
implementation also passed its postcheck: `npc/scripts_custom.conf` enables one
`Druid Mentor` at `prontera,153,193`, and the rebuilt Druid-capable map binary
retained its pinned hash through this deployment. Prior GRF/MuhRO comparison
found the required client metadata already present, so this release required no
additional client archive mutation.

Existing live `tools/docker/asset/map_conf.txt` and `char_conf.txt` routing
overlays intentionally differ from the repository's older loopback templates.
They contain the server's advertised route and a non-documented credential.
They were outside this 13-file scope, were not changed, and their effective
shape and cross-file consistency were validated without recording the secret.

## Preserved failed validation artifacts

Every superseded artifact remains preserved and was never edited or reused:

| Revision | Artifact suffix | Fail-closed finding | Live mutation |
| --- | --- | --- | --- |
| r1 | `v2-20260907T082404Z` | valid live SQL-route order differed from the first checker | none |
| r2 | `r2-20260907T082916Z` | valid advertised map/char overlay shape differed from the local template | none |
| r3 | `r3-20260907T083455Z` | market `amount=-1` unlimited-stock sentinel was rejected | none |
| r4 | `r4-20260907T084319Z` | exact isolated-candidate default-credential warning was not yet modeled | none |
| r5 | `r5-20260907T090314Z` | unauthenticated `mariadb-admin ping` raced the temporary init server | none |
| r6 | `r6-20260907T091909Z` | MariaDB TCP advisory prevented exact app-probe output | none |

r1 and r2 failed before a run receipt was created. r3 failed before creating
candidate resources, so its receipt records `candidate-cleanup=not-required`;
r4 through r6 record `candidate-cleanup=complete`. Exact live targets were
reclassified `PRE`, and no candidate Docker resource remained before the next
revision. r7 replaced the ping check with final-PID plus authenticated root/app
probes and scoped the expected isolated-candidate diagnostic to one exact,
ordered three-line block.

## Rollback

The successful run's PRE backup and SQL dump are retained. If this exact release
must be reverted, use only its reviewed rollback mode:

```sh
/usr/bin/env -i PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin \
  /bin/bash -p \
  /app/rathena-deploy-backups/episode20-21-gudra-healer-13-v2-r7-20260907T092736Z/package/deploy_release.sh rollback
```

Rollback restores the exact 13 PRE files and metadata; it does not automatically
restore SQL.
