# Durable stock purchases — 29 September 2026

Market, limited-stock barter and cash-sale carts now commit their complete
inventory, payment, stock and receipt in one character-server transaction.
Previously a failed stock write could leave a paid barter with missing outputs,
or restore already-sold market/sale stock after restart.

## Scope and behavior

The map server builds a pure inventory plan and sends an immutable request on
packet `0x3098`; the character server replies on `0x3898`. Both services must be
upgraded together. Client, login and web protocols are unchanged.

The character service validates current map ownership, table engines, balances,
item counter and locked stock rows. It writes all participating changes and the
full request receipt atomically. Identical retries resolve that receipt without
reapplying snapshots; altered payloads under the same identity cannot commit.
Stale stock or ownership produces a durable rejection. SQL/commit errors remain
pending until the same request can resolve its outcome.

No payment, item addition, script callback or purchase success is emitted while
waiting. Existing player state is flushed first on the same ordered map/char
stream. Pending saves, logout, item/currency mutation and equipment changes are
fenced. Attached scripts retain their stack and resume through the native timer;
cross-player script targets refuse access while the purchase is pending. Rental
checks retry after the fence. Equipment-break effects cannot mutate the staged
snapshot. After committed acknowledgement, caches and UI update once, followed
by the ordinary item callbacks and merchant experience award.

Stock purchases are serialized to one outstanding operation per map process.
Additional attempts fail before charging and can be retried. Stock writers are
fenced while that operation is unresolved. Acknowledgement handling refreshes
stock from SQL rather than retaining pointers into reloadable NPCs. A failed
refresh retains the fence. An exhausted sale remains at durable quantity zero,
removing the old fallible last-unit deletion from the purchase path.

The path applies to the entire cart when it contains a limited market/barter
row or a cash-sale row, including accompanying unlimited rows. Ordinary shops,
entirely unlimited carts and script-controlled transactions retain their
existing paths. Custom stock table names fail closed; this release uses the
default `market`, `barter` and `sales` tables.

Pet outputs and consumed pet materials are rejected before payment in the new
path. Pet creation needs a separate durable entitlement protocol. Unrelated
held eggs are preserved. Existing conservative barter capacity/allocation
checks remain; this change does not claim to fix every false refusal.

## Persistence and recovery

Apply `sql-files/upgrades/upgrade_20260929_shop_purchase.sql` while map and
character writers are stopped, after a restore-tested backup. It preserves
quantities, converts the three stock tables to InnoDB and creates
`pn_shop_commits`. Runtime checks additionally require transactional character,
inventory and account-registry tables. Live read-only inspection found those
three player tables already InnoDB and all three stock tables MyISAM.

A map crash before submission leaves no purchase. A committed transaction
survives map loss in the normal player/stock data; a surviving pending map
retries identical bytes. Cached character records are evicted after an
uncertain commit or old committed receipt, so authentication cannot reuse a
stale snapshot. Same-process relog is fenced until the pending request resolves;
the character handler independently checks the owning map connection.

Inventory/payment/stock recovery is durable. Arbitrary EquipScript and
achievement effects are post-commit notifications, not a durable outbox: a
process crash between commit and callback may omit those effects. They are not
replayed on relog. No exactly-once guarantee is claimed for arbitrary scripts.

For code rollback, stop both writers and restore both prior binaries/sources.
Retain InnoDB tables, current player data and receipts. Do not restore an old
database or prune receipts as part of ordinary rollback.

## Validation

- The original diagnostic reproduced ten correctness failures before the fix.
- Pure planner: 26 cases, including packed item layout, metadata, GUID counters,
  pet refusal and no mutation before submission.
- Native shop controls: 30 cases; native delivery: 31 cases, including actual
  script timer pause/resume while a purchase is pending.
- Exact map acknowledgement handler: all three purchase kinds, immutable
  retry, stale/duplicate acknowledgements, rejection and failed stock refresh.
- Real MariaDB: 15 engine guards, 18 injected write failures, atomic multirow
  purchases, durable rejection, changed-payload refusal, cache recovery,
  database crash/restart and two concurrent buyers of the final unit.
- Character handler: real FIFO buffers and SQL ownership/malformed-frame tests.
- The SQL suite passed with both Ubuntu and production Alpine native objects.
  Alpine checks use UBSan; supported Ubuntu fixtures also use ASan.
- Full Ubuntu release gate: **55/55 passed**, with clean map startup. Both
  exact production Alpine binaries passed isolated startup after importing the
  existing wallet ABI and new shop migrations into the disposable database.
- Coordinated deployment controller: eight offline injected success/failure
  cases passed, including partial installation and startup rollback.

See [SQL evidence and limits](shop_recovery_sql_test_20260929.md) and
[map acknowledgement evidence](shop_recovery_inter_test_20260929.md). A discarded
committed acknowledgement models reply loss; the suite does not cut a live TCP
socket at the exact COMMIT boundary. The actual database crash occurs before
receipt insertion. Deployment status and final gate results belong in the
separate deployment receipt.
