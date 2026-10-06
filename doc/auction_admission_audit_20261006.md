# Auction admission audit — 2026-10-06

Production has `feature.auction: off` and zero auction rows. The bid, cancel and
close packet handlers nevertheless forwarded requests. The bid handler reached
its wallet debit before forwarding. This bypassed both the disabled feature and
the pending durable transaction boundary.

All three entry points now reject disabled auctions or pending transactions
before reading request fields, debiting currency or sending an interserver request.
The feature remains disabled. This is an admission fix, not certification of the
legacy auction settlement, refund or registration paths for enabling auctions.

The regression executes the actual three handler bodies with explicit wallet
and transport spies: all six disabled/busy cases failed before the fix; enabled,
idle controls continue to reach their original path. The test is part of the
source release gate. No schema, protocol or configuration change is required.
