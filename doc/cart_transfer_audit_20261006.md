# Inventory and cart transfer audit — 2026-10-06

Baseline: `c98cdad867f2e068fc319cd84c75c83ee1eda901`.

Seven focused native reproductions failed before repair:

- A cart withdrawal granted the inventory item before removing its cart source,
  so quest conditions observed both copies during the transfer.
- A pending bank save allowed inventory-to-cart insertion, then blocked inventory
  deletion, duplicating ten items.
- `@stockall` had the symmetric problem: it added ten inventory items while a
  pending save blocked the cart deletion.
- A failed cart withdrawal returned success, so bulk callers counted skipped
  items as transferred.
- A negative cart deletion increased a stack from ten to eleven.
- A native insertion of `MAX_AMOUNT + 1` created an oversized cart stack.
- `@stockall` ran callbacks after each row, exposing an incomplete batch.

Repairs:

- User-facing cart transfer functions now share the packet handler's activity,
  transaction, cart-presence and map fences. The core add/delete functions still
  permit mutations owned by an active transaction's apply phase.
- Cart insertion validates positive bounded quantities, known active item IDs,
  cart counters, full-width weight arithmetic, per-item stack caps and capacity.
- Cart deletion validates its index, positive quantity, counters and weight before
  mutation. Login cleanup can still remove unknown zero-weight IDs and legacy
  oversized stacks.
- Inventory-to-cart validates canonical metadata and source weight before adding.
  It defers quest/grant callbacks until source deletion succeeds and restores the
  cart snapshot if source deletion fails.
- Cart-to-inventory defers callbacks until cart deletion completes and returns
  false on every failed insertion, so `@stockall` reports skipped amounts exactly.
- `@stockall` uses one delivery scope for the full batch and refuses while another
  UI, transaction or reserved mail capacity owns the actor.
- Amount lookup validates null actors, indices and quantities before array access.

Validation:

- `cart_transfer_native_test.py`: 244 fixtures and 1,328 assertions under ASan
  and UBSan. It drives actual PC functions, actual 0x126/0x127 packet handlers,
  the extracted production `@stockall` command and real quest VM conditions.
- Coverage includes 31 blocked actor/map/cart states across direct, command and
  packet paths; all transaction apply states; malformed indices, quantities,
  counters and weights; all 26 unequal stack-identity variants; equipment GUID,
  refine, grade, card and option roundtrip; capacity, item stack and weight caps;
  partial transfers, type filtering, cleanup compatibility and complete removal.
- Client transport, map-flag lookup, objective/log notifications and actor/NPC
  lookup are explicit boundaries. A syscall filter denies network access.
- Existing trade, guild storage, personal storage and preparation native suites,
  source release checks, optimized map build and complete private map/NPC startup
  run before production cutover.

Rendered client behavior and abrupt process/host failure between packet receipt
and in-memory transfer are outside this audit's verification. No client asset or
database schema change is required.
