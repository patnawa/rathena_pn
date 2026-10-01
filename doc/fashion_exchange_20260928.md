# Fashion exchange repair — 28 September 2026

Fashion Designer stopped its dialogue with `script:run_script_main: infinity loop !` at `pn_style,144,130`. The production log and an isolated native script-VM test reproduced the failure. Checking even one unrelated inventory item rebuilt and scanned all 21 stone boxes, exhausting the existing script limits. Empty-costume recovery exposed the same problem in reverse mapping.

Exact delimited stone membership and slot-specific enchant-to-stone indexes replace those repeated scans. All 366 stone outcomes and first-match recovery mappings remain identical. Script execution limits have not been raised. The dialogue failure was not a game-client crash.

## Player access

Type `@goldpoints` to arrive beside the manager, or `@fashion` to enter the fashion floor, then click the relevant NPC. `@activity` shows balances and access instructions. Travel uses the existing Office escape restrictions.

| NPC on `pn_style` | Coordinates | Service |
| --- | --- | --- |
| Gold Points | 136,130 | Exchange 1 Gold Point for 1 Fashion Point |
| Fashion Designer | 144,130 | Trade ordinary costumes for 15 points, Bio5/Tomb IDs 19961–19974 for 1, or listed stones for 10 each |
| Fashion Recycler | 152,130 | Recover a supported stone for 30 points, 10 Server Coins, or 1,000,000 Zeny |
| Fashion Boxes | 160,130 | Boxes 1–20 cost 50 points; garment second-slot box costs 300 |
| Fashion Catalogue | 128,138 | Buy a supported costume for 150 points |
| Fashion Enchants | 160,138 | Apply a supported stone to a compatible empty costume enchant slot |

Gold accrues at one point per three minutes online, capped at 50. Gold and Fashion Points belong to the login account. The catalogue price of 150 is an explicit PN policy chosen for this release; it does not claim another server's prices. Reselling an ordinary purchased costume returns 15 points, not the purchase price.

The catalogue offers 41 installed costumes in six collections. Eighteen old catalogue IDs have no server item definition and are filtered out. Four additional IDs (20596, 420132, 420091, 410171) lack metadata in the installed English client and are excluded from sale. No placeholder items or replacement sprite identities were invented. Existing PN client assets suffice for this release.

## Exchange safeguards

- Gold conversion permits zero to cancel, asks for confirmation, rechecks the balance and refuses Fashion Point overflow.
- Costume and stone trades re-read the selected inventory index after input, debit that exact record, and credit points only after successful deletion.
- Favorite, bound, equipped, rental, unidentified, damaged, refined, graded, enchanted and random-option items are protected from trade-in.
- Catalogue and box purchases validate balance and capacity, verify item delivery, then deduct points without a dialogue yield between delivery and deduction.
- Opening a box verifies stone delivery before consuming the box. Box outcomes retain the existing uniform compatibility selection.
- Colons in item names are normalized in selection labels so they cannot create extra menu choices.

These operations run within the map-server script event loop. This release does not introduce a durable SQL transaction ledger for point exchanges.

## Verification and deployment

`tools/ci/npc_audit_fashion_test.py` executes the actual NPC bodies and helper functions in the native script VM, with isolated player, inventory-debit, item-delivery, persistence and UI boundaries. It passed **47 cases / 1,658 assertions**, including Gold conversion/cancellation/overflow, successful and refused trade-ins, full inventory, empty recovery, all five enchant selection categories, catalogue purchases, box purchases and opening. Four deliberately broken variants are rejected, including the original stone-scan loop. The native allocator reports no leaks.

Validation used the current production build's headers/support objects and a freshly compiled script VM with UndefinedBehaviorSanitizer. The available Alpine AddressSanitizer runtime failed to link (`struct_sock_fprog_sz`); this release does not claim an ASan run. Default test behavior still requests both sanitizers; `NPC_FASHION_SANITIZERS=undefined` selects the tested fallback. A retained `--build-dir` is intended for diagnosis against unchanged headers/support objects; use a fresh directory after changing those dependencies.

`fashion_exchange_data_test.py` verifies the complete lookup parity and effective catalogue/box data. Four existing Fashion data tests pass for all 366 pairs, new enchant metadata, client descriptions and referenced skills. The broader Druid suite has an unrelated existing expectation of level 275 where current Alitea data allows 285; no job data was changed. Main Office validation passes for all 52 reachable desks. A sandboxed Lua read confirms metadata for all 41 offered costumes in the installed English item table; it is not a visual test of every sprite.

Both Fashion NPC files were activated through an idle, graceful map-only restart on 28 September 2026. Startup was clean, all seven production containers were healthy, other service containers were unchanged, and wallet/bank/Gold/Fashion balances were unchanged. No client update or database migration was required. This validates server deployment and isolated transactions; a post-deployment manual player exchange is a separate acceptance check.

Deployment evidence and rollback preimages are retained under `/app/pn-fashion-repair-20260928/deployment/`. Native test logs are under `/app/pn-fashion-repair-20260928/`. The corresponding local evidence folder is `Server-Development/fashion-repair-20260928/` outside the source checkout.
