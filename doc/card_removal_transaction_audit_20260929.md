# Card removal transaction audit — 29 September 2026

The active Wise Old Woman charged Zeny, Star Crumb and Yellow Gemstone before a
further `next` dialogue. Ending the interaction there left the fee consumed with
no card-removal outcome. Its technical-abort refund also recreated bound fee
materials as ordinary unbound items. Both failures were reproduced through the
actual NPC script VM and native inventory/card commands.

The NPC now revalidates the quoted equipment identity and cards after the final
confirmation, and makes no further dialogue suspension between fee commit and
outcome. Optional fee arguments on `successremovecards` / `failedremovecards`
validate and consume the payment inside the native operation. Capacity, unequip,
compare-and-swap or payment failures consume no fee, so compensation no longer
reconstructs material records. Existing no-fee command callers retain their
behavior. Advertised costs, random chances and intentional destructive outcomes
are unchanged.

Native payment checks run after equipment callbacks and again after returning
cards, because quest conditions can execute scripts during item additions.
Inventory deletion type bit 8 defers quest-condition refresh until the operation
has completed, preventing observers from seeing a partially deducted fee.
Only distinct unequipped Etc materials are accepted by the optional fee API.

## Evidence

`tools/ci/card_removal_transaction_test.py` compiles current `pc.cpp` and
`script.cpp` with UBSan, then exercises the actual NPC and builtins with synthetic
players and network access denied. The retained run passes **247 cases / 7,990
assertions**:

- All 100 random buckets under both player preferences, with exact advertised
  fees, card returns and equipment outcomes.
- Binding, unique ID, refinement, grade and favorite fields on surviving
  equipment; original bound payment stacks; cancellation, full inventory,
  insufficient weight capacity and repeated use.
- Equipment or payment changes while confirmation is open, failed unequip,
  changed wallet/materials and transaction locks inside equip callbacks.
- Invalid optional arguments and legacy no-fee/no-card compatibility.
- Actual quest-condition VM execution observes committed fee states. A
  deliberately nondeferred deletion is detected as a partial-payment control.
- A quest condition removing the available Zeny after card delivery makes the
  operation roll the returned card back without consuming fee materials.

The old NPC from Git HEAD fails the specific no-suspension-after-debit assertion.
Initial fee-loss and bound-refund logs, final `skills-card.log`, and the baseline
rejection are under `Server-Development/queued-audit-20260929`.

Transport, registry persistence and equipment callbacks are explicit fixture
boundaries. This proves in-process operation and VM suspension behavior; it is
not a SQL crash-durability or rendered-client disconnect test. Existing arbitrary
callback failure during rollback is not a general transactional guarantee.
No live account was used and nothing was deployed.
