# Player trade audit — 2026-10-06

Baseline: `18ee7c41e4269251e63350b701a502e0da722d2d`.

Four independent native reproductions failed before the repairs:

- `--case repeat`: a receiver with one empty usable slot accepted an offer of
  three items, but adding four more from that same stack left the offer at three.
- `--case metadata`: a ten-item stack with a random option merged into a plain
  five-item stack; the result was fifteen plain items and the option disappeared.
- `--case cancel`: an actor retaining a stale partner ID canceled that partner's
  newer trade, including its epoch and offer.
- `--case callback`: actual quest VM conditions ran while source items remained
  in their original inventories (`safe=0`, four observations).

The predictions were checked independently against the same pre-fix source.
The confirmed causes were repeated slot reservations, incomplete stack identity
in both inventory insertion and trade preflight, unchecked reciprocal trade
state, and synchronous callbacks during delivery.

Repairs:

- Native inventory insertion uses the complete existing `compare_item` identity:
  identification, refine, attribute, expiration, binding, GUID, cards, grade and
  every random option member. Unequal metadata gets a separate inventory slot.
- Trade preflight simulates the actual alternating insertion/removal order,
  validates indices, amounts and canonical item metadata before access, and
  applies character slot limits, configured item caps and weight limits.
- Offer adjustments clamp the additional quantity before weight calculations,
  reserve each row once, and validate the resulting complete offer. The existing
  `trade_count_stackable` configuration policy is preserved.
- Item/Zeny edits, native locks and confirmations verify reciprocal partner
  identity and trade epoch and reject actors already waiting for a transaction.
- Cancellation restores client displays and clears complete offer/bound state
  for the matching trade only. A stale actor cannot clear a partner's newer trade.
  Invitation replacement uses the same cancellation path.
- Both actors have delivery scopes across all item and wallet mutations; actual
  grant/quest callbacks execute only after all source removals and wallet changes.
  Failed item insertion/removal cancels the scopes and uses the existing paired
  rollback before cancellation. Durable paired SQL still owns final completion.

Validation:

- `trade_native_test.py`: 116 actor fixtures, 318 assertions, ASan/UBSan, actual
  trade/PC functions, quest VM and paired snapshot code. Covers the four original
  reproductions, all 26 unequal metadata variants, malformed offers, capacity,
  stack caps, stale/busy actors, duplicate confirmation/cancellation, opposing
  partial transfers and exact `INT64_MAX` wallet delivery. Transport, actor/NPC
  lookup, connection availability and objective/log notifications are explicit
  boundaries. A mandatory syscall filter forbids network access.
- Existing paired SQL receipt/fault suite is now registered in source release
  checks. This uses transaction fault doubles, not the production database.
- Re-run guild storage, personal storage and preparation native regressions,
  source release checks, optimized map build and complete private map/NPC startup
  against an owned disposable MariaDB fixture before production cutover.

Rendered two-client gameplay and abrupt process/host failure during a pending
trade are outside this audit's verification. Native trade packet review and the
authenticated companion remain responsible for player confirmation; no client
asset or database schema changes are needed.
