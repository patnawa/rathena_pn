# Shop delivery audit — 2026-09-29

Two reproduced in-memory purchase defects are repaired: callbacks could interrupt a multi-output delivery after payment, and metadata-blind capacity checks could authorize an output that native inventory insertion could not accept. This is a bounded callback/preflight repair, not a durable purchase transaction or general rollback mechanism. No deployment or live database changes were performed.

## Reproduced failures

The native fixture exercises ordinary NPC shops, barter, NPC cash shops, and the cash-shop button. A real quest condition script fills remaining inventory slots when it sees the first output without the second. Before the repair, all four routes debited 200 from an initial balance of 10,000 but delivered only the first output. The ordinary and NPC cash-shop paths reported success; barter and cash-button paths reported failure after charging.

A separate full-inventory reproduction places a bound copy of the requested stackable output in inventory. The old generic capacity check treated that copy as available stack space, although native plain-item insertion cannot merge into it. All four routes charged for an output they could not insert. The expanded regressions also cover rental, card-bearing and uniquely identified copies.

The preserved eight-case baseline is [baseline-red.log](evidence/transaction_delivery_20260929/baseline-red.log); its exact command and input hashes are in [baseline-inputs.json](evidence/transaction_delivery_20260929/baseline-inputs.json). The baseline records the pre-repair execution; it is not a replayable source archive.

## Repair

`pc_checkadditem_plain` follows the native plain-item insertion predicate: a matching stack must have the same item ID and no bound, expiry, unique-ID or card metadata. It preserves native first-match and stack-limit behavior, and treats nonstackable and GUID-producing items as new slots. The four purchase routes use this helper for their plain outputs; the generic checker remains unchanged for other callers.

`PcItemDeliveryScope` defers successful-add achievement notifications, auto-equip actions and quest refreshes until the outer purchase scope exits. Scope construction occurs after preflight and before payment/input deletion. Nested scopes join the outer scope, and normal returns and early returns flush notifications for additions that actually occurred. A refresh request made by material deletion is also deferred. Without a scope, the original immediate behavior remains.

The outer scope removes itself before invoking deferred work. Before a delayed auto-equip, it verifies the current inventory index still contains the expected item ID and unique ID with a positive amount. Captured sell values retain their original unsigned width. The scope does not stage inventory, packets, logs or SQL; it does not reconstruct consumed items or refund uncertain outcomes.

## Validation

Run against a prepared Linux native build:

```sh
python3 tools/ci/shop_delivery_native_test.py
python3 tools/ci/shop_transaction_native_test.py
```

The new suite passes 30 cases:

| Coverage | Cases |
| --- | ---: |
| Actual quest-script interruption of two-output carts, four shop routes | 4 |
| Full inventory with bound, rental, card-bearing or unique-ID output copies, four routes | 16 |
| Bound copy plus available blank slot accepts a separate plain stack, four routes | 4 |
| Immediate behavior, ordinary scope, nested scope and early-return scope with native achievement/quest observers | 4 |
| Native auto-equip and EquipScript timing outside/inside a delivery scope | 2 |

The auto-equip controls verify the native equipment mutation and script execution. Combat status recalculation is an explicit fixture boundary double; these cases do not validate combat stats. The achievement condition executes natively and observes delivery timing, but deliberately returns false: achievement completion, reward and persistence are outside its claim. The replacement-index guard for later queued auto-equips is source-reviewed, not separately forced by a regression case.

The existing 30 shop regressions also pass. Its barter callback case now invokes the real quest VM, because a linker wrapper alone cannot intercept calls within the same native compilation unit. No original cases were removed. Both suites finish with zero allocator leaks and no sanitizer diagnostics. The new delivery fixture and its extracted purchase bodies run with ASan and UBSan; the existing shop suite uses UBSan. Linked production objects use the prepared native build and are not claimed to be wholly sanitizer-instrumented.

Final evidence: [delivery-green.log](evidence/transaction_delivery_20260929/delivery-green.log), [delivery-inputs.json](evidence/transaction_delivery_20260929/delivery-inputs.json), [shop-regression-green.log](evidence/transaction_delivery_20260929/shop-regression-green.log), and [shop-regression-inputs.json](evidence/transaction_delivery_20260929/shop-regression-inputs.json). Tests run offline with network denied. Earlier auto-equip fixture initialization crashes were resolved by initializing map flags and isolating combat status calculation; they were fixture setup failures, not reported gameplay defects.

## Remaining limits and next audit work

- SQL stock failures and uncertain acknowledgements still permit inconsistent or partial purchases. See the separately reproduced [SQL audit](transaction_sql_audit_20260929.md). Callback deferral does not repair these outcomes.
- Pet creation/delivery failure is outside this fixture and needs a durable outcome protocol alongside stock/player persistence.
- Barter material selection can reject sufficient inventory split across eligible stacks. Full-inventory exchanges can also be refused even when consuming inputs would free enough slots. These remain allocation/preflight follow-ups; this patch does not redesign them.
- Client barter parsers already reject repeated listing indices and duplicate normalized output IDs. The native purchase function still relies on that caller contract; direct-call duplicate hardening is not added here.
- Other failure paths can still leave partial inventory/payment results. Early-return notification cleanup must not be described as rollback or atomic commit. Concurrent buyers, disconnect/restart recovery, and persistence replay require separate tests and architecture.

The targeted suite establishes the repaired callback-loss and metadata-capacity cases. The parent audit owns the final release gate and build receipt.
