# Equipment grading audit — 2026-09-08

Reviewed the native grading selection/commit handlers in `src/map/clif.cpp`,
`EnchantgradeDatabase::parseBodyNode`, and the effective Renewal grading tables.
No rates, prices, materials, reset rules or failure outcomes were changed.

## Confirmed defects and bounded fixes

- The commit packet handler did not enforce `flag.gradable`, although selection
  did. Both now call the same validation helper. A client cannot bypass the
  item eligibility flag by sending a commit request directly.
- Both selection and commit could index the chance array using an unchecked
  refine value. Current item identity, amount, identification, broken status,
  refine bounds and maximum grade are now checked before database lookup.
- Equipped or equipment-switch targets could be mutated in place without
  recalculating their active equipment effects. Both handlers now require the
  player to unequip and remove the item from switching first, with an explicit
  message. Trading, vending, buying-store and storage sessions are rejected.
- Catalyst multiplication and combined material amounts used 16-bit storage.
  Requirements now use wide arithmetic, combine shared reagent slots, reject
  amounts above the native stack limit and verify all debits before consumption.
  The target cannot be consumed as its own reagent. Missing/stale reagent data,
  equipped reagents and invalid/disabled catalyst costs are rejected.
- Added catalyst success chance could wrap in 16 bits. It now uses wide
  arithmetic and saturates at 10000/10000. A request for zero catalyst steps
  does not require a meaningless zero-quantity catalyst deletion.
- Configured Zeny prices above the signed native payment limit are rejected
  before any material debit.

## Database findings

The active import file contains no override body. Renewal defines eight upgrade
stages: None→D→C→B→A for level-2 armor and level-5 weapons, with sixteen cost
options. Every configured material and catalyst resolves in the effective item
database. Refine indices, chance values, quantities, prices, break probabilities
and downgrade amounts fit their intended native bounds. Current catalyst costs
and shared-material totals fit one stack.

Success still raises grade once and resets refine to zero. Failure still follows
the selected option's configured break, downgrade or unchanged-item outcome.
Cards, unique ID and other item fields are not rebuilt. Existing per-attempt
material/Zeny costs and the `< chance` probability boundary remain unchanged.

## Evidence and limits

Run `python3 tools/ci/grade_system_test.py` with PyYAML and g++.

The test extracts the **actual current selection, validation and commit
functions** and compiles them under ASan/UBSan. Explicit packet, inventory,
database, random-number, log and client-message boundaries verify direct commit
rejection, malformed/refine/grade targets, unavailable materials, shared reagent
costs, overflow, 6999/7000 success boundaries, saturated boosted probability,
success, unchanged failure, break and downgrade. Rejected preflight cases have
no debit, payment, RNG call or result mutation. Existing cards and unique IDs
remain unchanged on success. The same runner audits the active database rows.

The modified full `clif.cpp` also passed a C++17 syntax check against actual
repository headers with `PACKETVER=20260219`.

This is a native **handler-boundary** proof, not a full server/client session:
packet transport, native inventory callback execution, SQL persistence, item
rendering and UI interaction still require integration checks. The preflight
assumes ordinary synchronous native inventory/payment behavior; no rollback
framework for arbitrary custom callbacks or external state mutation was added.
