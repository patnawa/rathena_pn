# Shop and currency audit, 2026-09-29

Transaction defects reproduced in isolated native execution and fixed in `src/map/npc.cpp`, `src/map/cashshop.cpp` and the shared `pc_paycash` helper:

- Barter prices were multiplied as uint32 before accumulating into uint64. Four rewards priced 1,073,741,824 each consumed materials and charged zero Zeny. Cast before multiplication now preserves the full 4,294,967,296 charge.
- Barter payment narrowed the total to int32 after deleting materials. Two rewards priced 1.5 billion consumed two materials, failed payment with negative Zeny, and delivered nothing. Payment now retains int64 and charges 3 billion before delivery.
- Ordinary NPC buylist validated repeated item rows independently. With 29,998 Red Potions, two rows of two potions charged four but delivered only two. Repeated normalized target IDs now reject before payment.
- Market catalog indexes were truncated to uint8. Buying catalog index 256 charged the buyer then checked index zero's stock and delivered nothing. The saved market indexes now retain int32.

- The shared NPC cash/item/point-shop purchase path also independently checked duplicate rows, charging four items but delivering two at a near-full stack. Duplicate normalized targets now reject before payment.
- That path accumulated a price total in int32. A synthetic two-item INT_MAX price wrapped to -2; `pc_paycash` added two cash points before the caller reported failure. Totals now accumulate in int64 and reject values above the signed payment API limit before mutation.
- Barter material deletion invoked quest icon scripts before payment. A deterministic callback that cleared the wallet after the first material removal caused payment rejection and lost material. Barter now uses deletion type bit 8 (introduced with this audit's card transaction change) and explicitly refreshes quest icons after delivery. The native deletion body test proves that callback sees the paid, delivered final state for the single-output case.

The separate cash-shop button handler reproduced the same duplicate-output partial delivery and price-overflow failures. It now rejects duplicate IDs across tabs, empty/oversized lists, zero/excess quantities, pending transaction locks, and values that cannot be represented by the signed payment API. Eleven button/shared-payment cases include normal one-item and maximum 99-item purchases; the pre-fix duplicate and price failures are retained in `cashbutton-baseline.log`.

Related input guards reject empty/oversized lists and quantities outside 1..MAX_AMOUNT before narrowing; weight totals and barter slot totals are widened. Barter rejects a pending transaction lock before debiting materials. These guards do not claim a general atomic multi-item transaction implementation.

The native cash-payment helper also accepted a negative price, crediting Cash Points before returning a failure value. A preferred Kafra contribution greater than the price converted the excess into Cash Points: a 100-point purchase with a 1,000-point preference credited 900 Cash Points while deducting 1,000 Kafra Points. The helper now refuses negative prices or locked transactions and caps the Kafra contribution at the price. Both were reproduced using real in-memory account registry records, with no character-server persistence.

Validation: `python3 tools/ci/shop_transaction_native_test.py --build-dir /tmp/pn-shop-test` against prepared Linux map-server objects. Thirty cases cover the final changes with UBSan and sockets denied. The same fixture against HEAD's original npc.cpp reproduces the first four defects plus acceptance of zero/negative buylist quantities. Logs are in `doc/evidence/shops_currency_20260929/`. The fixture compiles the exact transaction bodies under alternate function names and uses real item database, inventory, and Zeny functions. NPC proximity/world lookup, transport, logging and registry persistence are explicit boundaries. The material callback case also compiles the exact `pc_delitem` body; its quest-script boundary is a deterministic injected wallet mutation. Cash arithmetic/duplicate rejection cases run for CASHSHOP, ITEMSHOP and POINTSHOP subtypes. Catalog prices for arithmetic edge cases are synthetic; these tests establish core correctness, not exposure of those exact prices in the enabled catalog.

A separate static scan of the isolated effective server snapshot followed Renewal NPC imports: 925 enabled files, 3,403 literal Zeny shop/market listings, zero candidates where 75% purchase price falls below 124% resale price. This is a literal-catalog screen, not proof about dynamic prices, script rewards, or conversion loops. No active literal OnBuyItemEvent/OnSellItemEvent definitions were found by that scan.

Coverage still open:

- Both cash-shop entry paths are checked for the repaired errors. Successful shared NPC payment variants and split-currency inventory payment are not exhaustively covered.
- Barter SQL stock write failures, asynchronous pet creation, and achievement/quest callbacks between output grants can interrupt a multi-output transaction. Ordinary buylist also invokes grant callbacks. This audit does not establish rollback across these boundaries.
- The barter stack-material matcher uses the first eligible stack; split metadata stacks and full inventory exchanges need a dedicated allocation test.
- Dynamic NPC reward claims, currency conversions, repeatable reward cooldowns and multi-step buy/sell conversion graphs are not exhaustively proved by the literal-price scan.
- Bank service regression fixtures showed source/test drift during this branch; parent audit owns their final verification status. No bank gameplay change made here.

No deployment, live account transaction, network packet submission, or persistence mutation was performed.

Final standalone runner: all 30 cases passed with clean allocator teardown,
using the combined final candidate. Early registry-boundary fixture failures
are retained; the final fixture seeds native in-memory registry records and
checks their values against the wallet, rather than suppressing registry errors.
