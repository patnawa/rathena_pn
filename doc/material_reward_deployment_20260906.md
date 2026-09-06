# Material exchanges and Final Battle reward capacity

Date: 2026-09-06. This is a scoped continuation of the episode audit, not a claim
that all episode content or graphical gameplay has been verified.

## Runtime scope

Only three NPC files change from commit
`7154bfc83e7a0161b9769c40cea0681126e283c9`:

| File | Exact installed-candidate SHA256 |
| --- | --- |
| `npc/custom/varmundt_biosphere_quests.txt` | `edec07ec166f5f8ae4a7e90ed52e38a5f9d76cf5781d139754a4ebd9a1ba0a7a` |
| `npc/custom/varmundt_biosphere_depth.txt` | `6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988` |
| `npc/custom/episode21/FinalBattle.txt` | `3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1` |

Omega's 17 and Depth Ellie's 24 material recipes now reject invalid numeric
input and recheck current access, resources, weight and exact native output
capacity before payment. Large ingredient totals are deleted in positive chunks
within the native amount limit. Costs, menu order, outputs and access policies
are preserved. Equipment Ellie, crowns and Abyss conversions are separate paths.

Final Battle's two crystal checks now reserve capacity for the complete plain
reward batch using native first-compatible stack rules. Refusal preserves the
saved roll and claim eligibility. Reward probabilities, daily policy and grant
order are unchanged. The inventory scan remains within actual script budgets.

No engine, binary, item/monster database, client assets, credentials or player
records are changed by this batch. The existing live-only item/pet overrides and
unbuilt `suicidebombing.cpp` source are preserved exactly.

## Verification

| Check | Result |
| --- | --- |
| 41 material routes, native VM/inventory/payment and real QuestInfo callbacks | 1,516 cases / 788,683 assertions; 89,424 checked nested conditions |
| Genuine original material failures | 100 cases / 25,787 assertions |
| Final Battle native crystal/helper/achievement tests | 566 cases / 26,174 assertions |
| Genuine original Final Battle losses | 3 cases / 188 assertions |
| Existing crown regression | 4,152 cases / 200,773 assertions |
| Existing Abyss conversion regression | 225 cases / 49,497 assertions, plus original/getter controls |
| Existing equipment-switch regression | Fixed 103 / 8,097; original 101 / 6,659; exact separate old bounds failure |
| Source rejection controls | Broad default 17, preserved-live expected snapshot 21, material 29, Final Battle 15, conversion 9 |
| Strict integrity audit | 900 NPC scripts, 110 DB imports, 71 wired fragments, 99 instances, 56 route assertions; zero errors/warnings |
| Existing party tests | 8 passed |
| Candidate server startup, project and preserved-live data variants | Both passed; 3,610 OnInit NPCs; only expected root-privilege warning |

All native suites required their applicable source/data gates before and after.
Normal candidate processes have clean allocator and ASan/UBSan output. Original
failure children accept only explicitly counted historical diagnostics.
Material, Final Battle and equipment-switch scoped production objects were
freshly built during this batch; final fixture-only follow-ups reused exact
objects and relinked. Crown/conversion final runs used exact-source retained
binaries. This is not full-server sanitizer coverage.

See [material transaction details](biosphere_material_transaction_audit.md),
[material callback evidence](biosphere_material_callback_audit.md), and
[Final Battle details](finalbattle_reward_capacity_audit.md) for explicit world,
network, status-notification and persistence doubles. No crash/SQL atomicity or
connected-client playthrough is claimed.

## Operational evidence

The exact three-file archive SHA256 is
`44edbff5057e86aa82aea946aa81d7b013c90ebc7e1b9662a26b7f3ada3e4e7d`.
The broad gate's reviewed NPC section is
`45b9138a14adaaae85d96a30b9a45da3cf5808577144680f7cf663c3dd1183b7`.
Project and explicit preserved-live profiles keep their independently reviewed
engine/database/closure pins. Full manifest comparison found only these three
authorized NPC source changes; 3,688-file membership and import graphs remained
unchanged. Collection of expected-live overrides is not installed-live validation.

The deployment script and runtime/SQL backups stay outside Git under
`/app/rathena-deploy-backups/material-reward-artifacts-20260906/`.
The local companion is `../material-reward-artifacts-20260906/`.
The first `candidate-only.json` is historical preparation evidence explicitly
marked pending; a separate release receipt records completed native checks.

Live installation succeeded at **2026-09-06T13:41:31Z (20:41:31 Bangkok)**.
The deployment verified exact old files, all four binary hashes, the three
preserved live files, current script limits, zero online characters, stopped
game services and zero persisted bonus scripts. It took an owner-only SQL
backup and replaced only the three NPCs. All four actual live source gates
passed, including both new scoped validators with the explicit live profile.
All four services were running, not restarting, with restart count zero; all
four startup logs passed. Map startup reported 3,610 OnInit NPCs and online
readiness. A separate post-deployment query again found zero online characters
and zero persisted bonus scripts. No rollback was needed.

The unchanged map binary remains
`63e554b5829bce76d50b0edb1dacd84ff6bc200393c3f43d821647d05ea7d98a`;
its build/deployment provenance is in the preceding equipment-switch receipt.
The live broad manifest verified 3,688 files and canonical SHA256
`f3b296241b9c5a5a08bc92bb0b3f85af9f1b9d0cf01714112661cb1b16fcc6a3`.
Source validators retain their honest false flags for independent live-world,
persisted-text or deployed-binary attestations; operational checks are recorded
here separately rather than changing those meanings.

Independent release review found all five copied native receipts identical to
their originals and bound to the fresh corresponding manifests. Material's
three raw runtime hashes, five exposed executable hashes, thirteen material
fixtures, four native output hashes and all 24 release artifacts matched.
The crown receipt exposes no executable hash; none is claimed for that check.

| Retained operational artifact | SHA256 |
| --- | --- |
| `release-checks.json` | `8a5b79b65983c0d5bdd6773a14d204791cbbbf48d5829db4718e0e5e187dece2` |
| `material-reward-deploy-20260906.sh` | `f197340f10b0fb9717b2276263d30d35a22299de59265e8590f3360397f88e2d` |
| `runtime-manifest.json` | `8909113ab8d28ec03af22dca331103f5171fe4a597bbea751ffeebbd44256fbf` |
| `pre-npcs.tar.gz` | `ea01dcbea0870dc2bd72e563e1cd2dea3837944666e2d0d68cf3cf8e8148b511` |
| `pre-npcs.sql` | `ce9c0e28777f3f6d9b6b27fd30c617280ae2a7a09e7d941e9f29c2c1ee8c74e7` |
| `deployment.log` | `5f20efafe99784ead15fa7a012ee8bf2122ecfcdc69953da881b2399ff16e194` |
| `startup-project.log` | `3149ea5f84e594b5cf1c59daf61f2f42309ba9e996b11aef749d93d6c28bffb8` |
| `startup-preserved-live.log` | `fe623bb24865fa3d2d3d9004e3556aaa80d5634a73fcee3aed9ba0ca36bd4d1d` |
| `post-rathena-login.log` | `9e24340217b54e6bc1741fee4900bf5476e937d606082e47891f8d05a135f009` |
| `post-rathena-char.log` | `f4a3e2ae3cce4b41bbef02843005cd2af73ab562b91f4f7d8ca1adaa86426c52` |
| `post-rathena-map.log` | `a893c37bad47c33920c1edbf52df42ed7bb241c594f9b0fd7fa3a884461833d8` |
| `post-rathena-web.log` | `eb9758aead71fff99a3982fd06f8eaef5691236376a40b8e2ae6861b824e0a39` |

Rollback stops/verifies every game service before restoring only the exact old
three-file backup, checks old/preserved hashes before restart, and never
overwrites SQL over potentially newer player data. Backups remain available;
no player data was deleted or restored.

All seven startup/deployment logs were copied back and their SHA256 values
matched the remote files. The transfer archive contained exactly those seven
regular log files, SHA256
`a1b81cf98fb93406c8c61322ccb613185c050192cd75ab0f2cafabeb8a8db36d`.

The next source-only finding is documented separately in
[the document-exchange follow-up](biosphere_document_exchange_followup_audit.md).
That reputation exchange is unchanged by this deployment; its proposed fix and
native cases remain future work, not part of the passing material recipe tests.
