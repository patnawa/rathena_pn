# Vending and Buying Store Purchase Audit — 2026-10-06

## Scope

This audit followed native item and money mutations through vending and buying-store purchases, including packet validation, inventory projection, cart removal, callback ordering, paired persistence, and pending-transaction behavior.

## Fixed defects

- Batched delivery callbacks until the complete purchase state is settled. Quest and achievement scripts can no longer observe an item in both the recipient inventory and the source inventory or cart.
- Replaced partial stack comparisons with the same complete item identity used by `pc_additem`, covering refine, enchant grade, random options, cards, binding, expiry, and unique ID.
- Removed the older item-ID-only vending capacity check, which rejected valid metadata variants and miscounted batch slot use.
- Verified vending cart counters, loaded item metadata, source quantities, source weight, listing bounds, packet pointers, and unaligned packet fields before mutation.
- Verified that cart removal actually consumed the sold quantity. If it cannot, the paired transaction rolls back instead of duplicating the item.
- Moved buying-store budget clamping behind both actors' transaction locks. A rejected concurrent request no longer changes the live purchase order.
- Added bounds and canonical metadata checks for buying-store listings and seller inventory.

## Native regression evidence

`tools/ci/market_purchase_native_test.py` compiles the real vending, buying-store, inventory, quest VM, and paired persistence paths with ASan and UBSan. Its fixture denies network syscalls and checks:

- vending and buying callbacks see settled stock, listings, and wallets;
- a full stack with different metadata does not block a valid new stack;
- a pending transaction preserves the original buying budget and assets;
- invalid cart accounting cannot duplicate a sold item;
- successful purchases create matching durable pair snapshots.

The focused suite completed 10 native cases and 36 assertions with no sanitizer findings or memory leaks.
