# Confused Boy exchange audit — 2026-10-05

The nine paid equipment exchanges now use `barter_alice_equipment` in
`npc/custom/instances/alice_barters.yml`. The existing Dali NPC opens the native
extended barter window. Output IDs, Zeny prices and material quantities match
the recipes at parent commit `318c50eb32c4b2f7a52ea8aaa84b0d49710dcceb`.

## Reproduced defects

1. With three occupied inventory slots containing exactly 1 Heavy Chain,
   50 Small Sewing Boxes and 50 Yellow Leaves, Mad Bunny was rejected despite
   all three slots becoming available when costs were consumed. Native VM
   result: output=0, Zeny=5,000,100. With spare slots, the same recipe succeeds.
2. An actual quest-info condition ran during native material deletion and
   observed consumed inputs before the output existed. Its wallet mutation
   could occur in the middle of the old script exchange. Native VM result:
   callback_seen=1, callback_safe=0. This is a callback ordering defect; the
   audit does not claim an arbitrary injected `pc_additem` failure is refundable.

The existing native barter implementation plans the complete inventory after
consuming input costs, pays through `pc_payzeny`, and uses `PcItemDeliveryScope`
to defer quest/achievement callbacks until settlement. The scoped fix reuses
that implementation without changing server C++ or binaries. It also respects
the server's configured favorite-item selling protection. Bound material stacks
remain eligible as in the previous ID-based recipe; unconsumed stack metadata
is preserved.

## Validation

`tools/ci/alice_exchange_transaction_test.py` parses the actual effective item
metadata and barter catalog, executes the actual Confused Boy VM and `callshop`,
and tests the exact production barter, inventory planner, deletion, delivery,
payment and quest-condition code. Actor/NPC lookup, client transport, registries,
achievement notifications and the legacy script wallet setter are explicit
fixture boundaries. Native barter payment itself is not doubled.

- 138 cases and 1,454 assertions with ASan/UBSan; script, inventory, quest and
  memory sources compiled fresh. No unexpected diagnostics or memory leaks.
- All nine exact recipes, post-consumption slot/weight limits, insufficient
  money, each missing material, favorite protection, pending transaction fences,
  surviving material metadata, two-item purchases, invalid quantities, replay
  refusal and combined nine-recipe carts.
- Actual quest-info VM conditions observe complete input/output/payment state
  for each recipe before changing the wallet.
- Alice encounter suite: 91 checks. Earlier doubled exchange assertions moved
  to the new native economic suite; leave/open catalog handoff remains covered.
- Weekly regression, source release checks and private whole-NPC startup are
  release gates. The private startup database uses disposable fixture schemas;
  production gameplay and player databases are not used for test transactions.

The ordinary unlimited barter path is synchronous. These tests prove normal
validation and callback ordering, not durable SQL receipt recovery after a
process crash, or arbitrary native mutation failures. In-game rendering and
manual client interaction remain to be verified by a player.
