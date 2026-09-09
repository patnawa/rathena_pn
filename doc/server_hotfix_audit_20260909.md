# reference server hotfix comparison and server fixes ? 9 September 2026

Reviewed all 75 numbered public topics in reference server's hotfix category: #1?74, including two different #28 posts. Compared applicable behavior with a fresh snapshot of 192.168.10.18 and the local repository, including ignored imports. This is a source, data and focused execution audit, not a guarantee of bug-free gameplay or a complete quest/instance playthrough.

## Eight fixes

| Fix | Result | Reference |
|---|---|---|
| Saved status restoration | Removed the arbitrary 50-buff reload limit; respects the actual 16-bit packet capacity. | #44 |
| RODEX interaction overlap | Blocks mail operations during NPC/shop, storage, trade, vending and buying-store activity. Blocks storage opening during mail composition, including delayed guild/premium responses. Normal mail composition remains allowed. | #43 |
| Liberation Shadow Shoes | Adds the missing magical boss-damage increments at refine +7 and +9. | #51 |
| Deviruchi transformation | Converts the legacy ASPD value correctly: +1 ASPD, rather than +10. The separate Mtf_Aspd2 status remains unchanged. | #16 and [primary developer explanation](https://board.herc.ws/topic/2525-monster-transform-update/) |
| Three-day unlimited fly-wing box | Corrects 10,080 minutes to 4,320 minutes; existing rentals are not retroactively shortened. | Duration issue found while comparing #28 and #29 |
| Chapter 1 protection expiry | Checks resistance on entry/relog and every second in hem_dun02, ch1_gfn01 and ch1_gfn03; returns expired visitors to their entrances. | #52 |
| Amicitia 2 memo | Adds the missing nomemo flag. Read-only database check found zero existing Amicitia 2 memo rows; no player-data cleanup was needed. | #57 |
| Garden vending savepoint | Stops the savepoint action from falling through into paid storage. Existing live customizations were preserved using a one-line live-specific patch. | Additional defect discovered during #26 comparison |

## Verification

- Fresh character and map server builds succeeded in the server's compatible Docker toolchain.
- Final map-server and character-server startup checks passed in an internal Docker network with an empty schema-only database; no production player records were copied into testing.
- Saved-status production-handler fixture passed eight ASAN cases: 0, 49, 50, 51, 100, 1000, 1365 and 1366 rows. Original code fails the same regression. SQL/FIFO boundaries are test doubles; this does not simulate a rendered player relog.
- RODEX production-handler fixture passed 256 busy-state combinations and storage/composer transitions under ASAN. Already-pending mail-claim settlement remains unchanged; no persistent inbox-open state was invented.
- Actual Chapter 1 NPC body executed in the native script VM under UBSan: 20 scenarios, 137 assertions, with network calls denied and world/timer/packet boundaries replaced by test doubles. No memory leaks reported.
- Source-extracted bonus expressions passed all 21 Liberation refine levels and the Deviruchi conversion under UBSan; these are expression tests, not combat-engine damage measurements.
- Four rental groups passed deterministic-item, quantity and duration checks. Garden patch passed exact-byte and branch-preservation checks.
- Initial isolated startup caught a nonexistent ch1_gfn02 map flag; it was removed before final acceptance and deployment.

## Remaining differences and limits

Public hotfix notes often describe proprietary client, patcher, website or seasonal-event changes without transferable code. Those were classified individually, not marked as repaired or gameplay-tested.

Known review items include Aquila's instant Max Pain (the published notice omits the corrected cast duration), missing one-handed Encroached Axe progression, incomplete Illusion Labyrinth/pet content, and absent direct perfect second-slot Signet enchant support. Unspecified drop rates, NPC sale prices, encounter tuning and bulk player-item migrations were not guessed. See the complete per-post evidence in [older.md](server_hotfix_evidence_20260909/older.md), [middle.md](server_hotfix_evidence_20260909/middle.md) and [recent.md](server_hotfix_evidence_20260909/recent.md).

## Deployment

Final status: **deployed and verified**. All 11 file hashes match; login, character and map services are running and connected. Two earlier attempts automatically rolled back because verification included shutdown logs and did not strip ANSI color codes; both verification issues were corrected before the final successful deployment.

Nine source/data files plus rebuilt map-server and char-server binaries were installed with no players online. The login, character and map services were restarted. Exact outcome and installed hashes are recorded in deployment-receipt-final.json; startup logs and receipts are retained under server-work/hotfix-audit-20260909 in the game workspace.

Backup: /app/rathena-deploy-backups/hotfix-audit-20260909T091450Z. The deployment script can restore all original files if verification fails. The isolated build directory is /app/rathena-builds/hotfix-audit-20260909.

The repository changes are not committed or pushed. Previous navigation edits and unrelated working files were preserved.
