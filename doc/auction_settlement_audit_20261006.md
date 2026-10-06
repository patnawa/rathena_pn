# Auction settlement audit — 2026-10-06

The actual character-server expiry path deleted an auction after delivery mail
failed. A MariaDB trigger reproduced this deterministically: the old code left
zero mail rows and removed the auction (`retained=0`). The corrected path keeps
the complete auction (`retained=1`) and schedules a retry.

## Changes

- Expiry, seller cancellation, early close and buyout write item mail, currency
  mail and auction deletion in one transaction. Cache removal follows commit.
- Ordinary bids commit the prior buyer's refund and replacement bid together.
  Failed writes preserve the previous cached buyer and price. Known rollback
  sends the existing full-bid failure refund acknowledgement.
- Buyout overpayment is durable refund mail, committed alongside seller payment
  and delivery. The success acknowledgement cannot refund it a second time.
- Settlement requires InnoDB for auction, mail and attachments. New databases
  use InnoDB for auctions; existing databases require the accompanying upgrade.
- A missing terminal row retires stale cache without delivering assets again.
  SQL errors retain custody. Expiry retries after ten seconds.

## Validation

The isolated MariaDB harness links the production character-server objects.
It exercises failed header, attachment, seller-payment, auction deletion and
bid-update writes; complete rollback; expiry retry and callback replay; unbid
returns; wrong-owner cancellation/close; failure and success acknowledgements;
manual replay; metadata preservation; outbid refunds; buyout payments and excess
refunds; and refusal of MyISAM settlement. Packet FIFOs remain local to the test.
The full recovery suite also retains previous mail, shop and pet regressions.

Production rollout requires auctions disabled, zero auction rows, no online
characters and stopped map/character writers before the scoped InnoDB conversion.
The operational journal preserves the table definition and records conversion.
InnoDB is backward compatible with the old binary and is retained on binary
rollback. No player data is cleared by the upgrade.

## Remaining enablement blocker

Keep `feature.auction: off`. This is a settlement fix, not certification of the
complete auction economy. Registration and bid debits still use a legacy
map-to-character protocol without durable request identities or replayable
receipts. An indeterminate bid commit is logged for reconciliation and is not
automatically refunded: inventing a refund could duplicate currency. A dropped
acknowledgement or process crash across that handoff remains an enablement blocker.
Add durable admission/debit receipts and restart/reconnect reconciliation before
enabling auctions. Rendered client auction journeys have not been verified.
