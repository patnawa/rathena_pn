# Market maximum Zeny verification — 2026-10-06

Keep the legacy 1M ticket retired. The deployed PMK1 Market already supports
64-bit Zeny prices, independently of the native vending window's narrow price
field. Create the shop normally, then use Market → My shop → Set price → Publish.
The maximum is 9,223,372,036,854,775,807 Zeny; quantity totals and seller bank
capacity are checked before payment. Seller proceeds credit the account bank,
after the configured tax, rather than the seller's carried wallet.

Fresh native tests exercise actual vending purchases at 3 billion, 2^53+1 and
INT64_MAX, exact seller bank saturation, insufficient funds, seller bank overflow,
quantity multiplication overflow and a maximum-price sale with five-percent tax.
The fixture submits the real paired request while isolating transport; it is not
a production purchase. The map functions under test use ASan/UBSan.

A separate disposable MariaDB test links the real character objects and includes
the production paired SQL handler. It verifies high-value wallet/cart/inventory/
bank/listing/receipt atomicity, bank/listing/receipt write-failure rollback, replay
after later state changes, and invalid currency totals. The production database
is never used. Only its test driver is sanitizer-instrumented.

The client number test checks exact parsing/formatting, quantities and overflow.
Its initializer now uses an explicit int64_t array so the same test compiles on
Windows (long long int64_t) and Linux (long int64_t).

No market runtime or currency configuration change was necessary. These changes
add regression evidence for the existing capability. No new rendered-client
acceptance session was performed during this audit.
