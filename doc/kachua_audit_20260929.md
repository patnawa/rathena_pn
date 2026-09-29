# Kachua audit and loss-prevention repair — 29 September 2026

## Confirmed failures and repairs

1. The Kachua NPC tested capacity for one Knife, then spent a key and granted a random reward plus a Mileage Coupon. With one inventory slot free, the real NPC/inventory test spent the key without delivering the complete draw. It now checks every possible loaded group outcome before each charge, using the new `checkweightgroup` script command. Ten-draw runs recheck before each key is spent.
2. Direct Kachua Secret Box (23914) and reward key/box (102701) scripts previously ran after automatic item consumption. Both now use Delayconsume with an explicit guarded deduction. Failed capacity checks retain the source item. The normal pool still grants its coupon; the event pool retains its existing no-coupon rule.
3. The native selection-package handler deleted its box before checking whether its selected outputs fit. A real 0x0baf request reproduced this with Ace Card Box and a full inventory. It now checks all selected outputs and stack limits before deleting the source. A last box may free the slot needed for its reward.

Reward pools, rates, quantities, cash-shop prices and existing balances were not changed. The capacity checks are conservative: random groups reserve fresh slots even if an existing stack might merge, so players may need to free additional slots.

## Validation

- 578 native cases and 33,407 assertions passed using the actual script VM, inventory mutation, item groups and selection-package packet handler.
- Exercised all 55 normal-pool and 71 event-pool outcomes individually with exact quantities, 22 selection packages (74 choices), cancellation, insufficient keys, one/zero free slots, weight refusal, full coupon stack, interrupted ten-draw run, forged account IDs, invalid choice IDs and reuse of a consumed box's slot.
- Audited 1,077 reachable item identities through static group/box/package links. All have server/client metadata and inventory/collection artwork; no empty usable item lacked its package definition.
- The apparently empty scripts for 22 selection boxes are intentional: those boxes use item_packages.yml and the native package handler. Their behavior was tested rather than replaced.
- Full map-server build and isolated startup parsing passed. Deployed with no connected players; all services healthy afterward. Other services and currency/point balances unchanged.

The fixtures explicitly replace UI transport, persistence and player lookup boundaries; no live player inventory is used. The audit is not a claim of current official monthly reward-pool parity or an in-game visual check. Key summoning and client-side selection UI were not visually exercised in this pass; the native reward and redemption paths above were executed.

## Reproduction and deployment evidence

Workspace directory: Server-Development/kachua-audit-20260929. `run.py` stages the actual native fixture on the isolated build host; `test_remote.py` compiles against current map objects and runs with networking denied. `driver.cpp` is also preserved as tools/ci/kachua_transaction_test.cpp. `repro-before.log` and `package-repro-before.log` capture the two failing paths; `native.log` captures the passing run. Complete item coverage is in complete-audit.json. Build, startup and deployment receipts are build.json, validation.json and deployment.json.

Rollback snapshot: /app/pn-kachua-audit-20260929/deployment/before.
