# Shop purchase SQL recovery proof — 2026-09-29

The real MariaDB suite passed against the new `shop_sql.inc` implementation. No production database was accessed. Evidence is in [evidence/shop_recovery_sql_20260929/report.json](evidence/shop_recovery_sql_20260929/report.json), with the input source hashes and individual normal/restart/crash logs beside it.

## Reproduce

Run on a Docker host using a candidate with Ubuntu-built character-server objects and common/libconfig/rapidyaml archives:

```sh
python3 tools/ci/shop_recovery_sql_test.py \
  --candidate /app/pn-shop-recovery-20260929/test-candidate \
  --evidence /app/pn-shop-recovery-20260929/sql-proof-final \
  --image pn-bank-validation:20260929
```

The runner creates a unique internal Docker network and disposable `mariadb:noble` container, imports `main.sql` plus `upgrade_20260929_shop_purchase.sql`, compiles the runtime, and removes only its own containers/network. It refuses pre-existing fixture names. It connects exclusively to the fixed fixture alias `shop-recovery-db` and database `shop_recovery_probe`, never to application configuration. Evidence must be outside the candidate tree. The compiler container has networking disabled; database access uses only the isolated internal network.

The C++ runtime includes the exact production `shop_sql.inc` and links actual character-server objects, including inventory persistence and common SQL transaction handling. Most SQL cases supply the explicit `current_owner` boolean. A separate wire case calls the real character handler with real in-memory FIFO buffers, online-character ownership records, map-server file descriptors and acknowledgement writer. SQL statements, transactions, rollback, row locks, receipts, character cache and database restart are real. This does not exercise a live map-client purchase, TCP transport or the outer inter-server packet dispatcher.

## Passing coverage

- Market, barter and sale purchases each atomically update two stock rows, output inventory, wallet/cash points, unique-item counter and receipt; barter also consumes material quantity. Unrelated item refine/binding/unique ID/card and unrelated character state survive.
- Fifteen table-engine cases: each kind refuses non-InnoDB character, inventory, account registry, active stock or receipt tables, with no state changes.
- Eighteen real SQL trigger failures: first stock write, second stock write, inventory insertion, character update, currency registry insertion and final receipt insertion for each of the three kinds. Every case rolls back player state and both stock rows; cache values do not advance. Removing the triggers allows the original operation to commit.
- A new request with a wrong map owner creates a durable rejection without changing player/stock state. Stale second-row stock likewise rejects without decrementing the first row; restoring stock later cannot turn that rejected request identity into a purchase.
- Exact committed retries after a discarded acknowledgment and after opening a new SQL connection preserve newer wallet, inventory, stock and cash state. Stale character cache entries are evicted rather than filled from the old request.
- Twelve changed-payload probes (item amount, wallet, stock amount or character identity for each kind) cannot replay under an existing receipt or mutate newer state.
- The final sale unit remains durably represented as zero stock. A receipt retry does not restore or consume it again.
- Two independent runtime processes and SQL connections meet at a database barrier, then compete for the last market stock unit while a trigger holds the first update. Exactly one commits and one receives a durable rejection. The recorded run hit a real MariaDB deadlock; the losing process retried the same immutable request once, then received rejection. Stock ends at zero, total output is one, only the winner pays, and the loser retains its wallet/counter/original inventory. Both resulting receipts replay without changing state.
- The actual character wire handler rejects five ownership failures: missing online record, negative map server, out-of-range map server, mismatched character and mismatched map connection. Six malformed-frame/payload cases cannot commit: short/long declared length, incorrect packet ID, missing nonce and zero/excessive stock count. Correct ownership commits and preserves ACK identity; a departed session can resolve an identical committed receipt without replaying state. Buffers contain complete records; fragmented TCP and outer-dispatch length checks are outside this case.
- Actual clean MariaDB restart plus a new runtime process: an existing committed receipt still resolves after newer state was saved, without overwriting that state.
- Actual MariaDB `SIGKILL` while a trigger pauses immediately before receipt insertion: recovery rolls back the already executed market stock/inventory/wallet writes; a new runtime process retries and commits once. This is an executed database crash, not a doubled SQL error.

Final output includes `SHOP_SQL_PASS 11785`, `SHOP_CONCURRENCY_PASS 1025`, `SHOP_WIRE_PASS`, `SHOP_RESTART_PASS 409`, and `SHOP_CRASH_RECOVERY_PASS 69`. These counters include SQL queries and cell validations; they are **not counts of independent purchase scenarios**. UBSan reported no violation. Expected injected SQL errors appear in the normal log; the concurrency log records the successfully retried deadlock.

## Limits

Concurrency coverage is a two-account, final-unit market purchase, not an exhaustive schedule exploration or every barter/sale contention pattern. The lost acknowledgment case discards a committed result and retries; it does not cut a live socket precisely during COMMIT. The database crash is before receipt insertion/COMMIT, while the clean restart covers already committed receipts. It does not certify live deployment, map-server save/logout ordering, new stock-row initialization, all catalog/collation variants, or asynchronous pet delivery. Native map-path tests and the release gate must cover their own boundaries.

The initial run used existing Ubuntu character objects with the new SQL source included directly. A subsequent full run also passed against the **production Alpine candidate's newly built character objects**, using `improvement-validation:20260914` and pinned `mariadb@sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979`. All five output markers and the successful deadlock retry matched. Separate [production evidence](evidence/shop_recovery_sql_20260929/production-alpine/report.json), command receipt, logs, toolchain image IDs and native object/archive/binary hashes are retained in the `production-alpine` subdirectory. Character objects were held stable during this proof. Re-run if tested SQL/header/migration or character inventory/SQL dependencies change.
