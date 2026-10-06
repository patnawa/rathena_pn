# Auction handoff and bank audit — 2026-10-06

The native bid handler test reproduced an immediate wallet debit before a durable
result (`sends=1 debits=1 FAIL`). Registration had the same split ownership of
inventory/fees and auction custody. The correction routes both operations through
the existing immutable shop commit, with no local debit before acknowledgement.

## Durable ownership

Auction registration, listing fee, inventory removal and replay receipt commit in
one transaction. Bids commit the wallet debit, listing update or buyout, refund
mail and receipt together. Authoritative inventory is compared with the proposed
after-image so registration cannot leave the listed item in the seller's bag.
SQL validates actor ownership, seller/bid limits, current listing state and expiry.
Registration rechecks transfer restrictions and uses 64-bit fee arithmetic.
Bound, rented, equipped and unidentified items cannot enter this custody format.

Two tagged envelopes reuse the existing fixed-size commit region; historical
receipt sizes remain unchanged. Auction structs are copied through byte storage
to avoid unaligned references in the packed protocol. Both services must upgrade
together. Retired registration/bid packets and old refund acknowledgements are
ignored by dispatch, preventing an old reply from crediting assets again.

Unresolved sessions retain the existing transaction/save fence and retry exactly
the submitted bytes. A terminal result applies the map after-image once. Receipt
replay never overwrites newer SQL state. Auction timers/cache reload from SQL
after commit, including replay after an uncertain result. A missing listing is a
durable rejection with no payment; a SQL failure remains retryable.

## Verification

The native handler regression checks admission without an immediate debit.
The exact map interserver fixture covers both new request kinds, immutable retries,
disconnects, wrong acknowledgements, success/rejection and duplicate replies.
The real SQL fixture covers receipt/debit failures, forged inventory after-images,
departed owners, registration/bid/buyout replay, altered request identities,
self-outbid/excess refund conservation and replay after database/process restart.
Existing settlement, shop, mail and pet recovery tests remain in the full suite.

The separate banker audit found four stale test fixtures, not a confirmed live
balance-loss defect. Updated tests include all current transaction fences and
required character-save table names. A private SQL run passed 84 checks before
and after killing MariaDB during an uncommitted receipt insert; financial rows
were byte-identical after recovery. The arithmetic suite exercised 500,000 plans.
Linked server objects are ordinary builds; only the test driver is instrumented.

Auctions remain disabled. This change does not enable the feature or certify the
rendered auction UI. Verify the actual client journeys before a separate enablement.
