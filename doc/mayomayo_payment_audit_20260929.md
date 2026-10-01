# Mayomayo payment preservation — 29 September 2026

Fixed a reproduced metadata-changing refund in `npc/re/merchants/enchan_mal.txt`.
On technical equipment-mutation refusal, the NPC previously refunded deleted
payment with plain `getitem`. Native VM execution with an injected unequip
refusal turned one bound Silvervine into one unbound Silvervine on reset and
15 bound E-grade coins into 15 unbound coins on enchant. The selected weapon
remained unchanged. This demonstrates the unsafe rejection path; acquisition of
those bound currencies and a normal player-triggerable unequip refusal were not
established.

The NPC now rechecks the current payment count and all inventory rows of the
selected currency immediately before debit. It refuses bound, rental, favorite,
equipped, unidentified, damaged, refined, graded, card-bearing, random-option or
unique-ID-bearing payment records. Random-option values/parameters are also
checked even if the option ID is zero. The player receives an explanation and
instructions to put incompatible currency copies in storage. A mixed inventory
is refused even when a plain stack alone would suffice: the existing deletion
operation chooses stacks internally, so no unsupported record may be eligible.

The guard covers the common enchant path (including its defensive destruction
branch) and reset separately. Plain payments retain the existing service, price,
random enchant selection and technical-refusal refund. This is a conservative
metadata restriction, not a new atomic multi-resource transaction API. Existing
callback/reentrancy limitations outside this demonstrated refund defect remain.

## Verification

`tools/ci/mayomayo_payment_test.py` and its C++ driver execute the actual NPC body,
real item definitions and native inventory operations in an isolated script VM.
No world/account database starts; network calls are denied. Transport, player
lookup, registry persistence and equipment callbacks use explicit fixture
boundaries. The helper's unequip failure is injected at that boundary.

35 cases / 1,511 assertions pass with clean allocator shutdown:

- Twelve unsupported metadata forms, each through reset and enchant, preserve
  the full inventory and money and never reach native equipment mutation.
- Normal reset/enchant succeeds; technical refusal refunds the exact plain
  currency amount and preserves the weapon's complete item record.
- Mixed bound/plain inventories are refused; split plain stacks pay/refund the
  correct combined amount; payment removed during final confirmation is refused.
- A 200-row inventory filled at the final confirmation, with currency in the
  last row, completes without script execution-limit errors or filler loss.
  The preexisting entry capacity check still applies to initially full bags.

The same regression fixture rejects the pre-fix NPC at its first inventory
preservation assertion. Reproduce on a built Linux tree with
`python3 tools/ci/mayomayo_payment_test.py`; use `--source before.txt` for baseline.
Existing compiled core objects are reused, so this is not an all-core sanitizer
certification. No assertion is made that every imaginable callback mutation is
atomic.

Work artifacts in `Server-Development/client-consistency-20260929/`:
`mayomayo-repro.log` (two original rejection reproductions),
`mayomayo-payment-before.log`, `mayomayo-payment-after.log`,
`run_mayomayo_payment.py`, and the retained baseline `mayomayo-before.txt`.
No production deployment was performed by this task.
