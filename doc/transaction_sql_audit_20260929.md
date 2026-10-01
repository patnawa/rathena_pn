# Transaction SQL error audit — 2026-09-29

**Implementation follow-up:** the diagnosed stock purchase defects now have a
durable map/character repair and passing real-database recovery tests. See
[purchase recovery](shop_purchase_recovery_20260929.md). The unresolved wording
below records the original audit baseline, not the current implementation.

**Confirmed unresolved persistence defects. No production code or schema change made by this SQL audit.** Moving a stock write before or after payment, adding a boolean return, or issuing an unverified rollback cannot make the current player/stock commit durable. These findings require a durable purchase protocol rather than a local reorder.

## Executable evidence

Run in a prepared offline Linux build:

```sh
python3 tools/ci/shop_sql_failure_audit.py
```

The diagnostic intentionally exits nonzero when correctness fails. It is named `_audit`, is not registered as a passing release gate, and never treats an unsafe result as a successful regression test. To repeat the captured source baseline after other shop edits:

```sh
python3 tools/ci/shop_sql_failure_audit.py --source doc/evidence/transaction_sql_20260929/npc-purchase-baseline.cpp
```

The fixture compiles the exact `npc_buylist`, `npc_barter_purchase`, and `npc_market_tosql` bodies with real item database, inventory, and Zeny functions. World lookup, transport and SQL are explicit offline boundaries. SQL doubles model success, a failure before a write, and a write that applied but whose acknowledgement was lost. No real database is opened; sockets are denied.

Sixteen executions produced four healthy controls, two internally consistent applied-write/lost-ack market cases, and ten correctness failures. Barter modes 1 and 2 both inject `Sql_Query` failure; they are not separate prepared-statement phases. The market modes distinguish `SqlStmt::Prepare` from `Execute` failure. One-row tests fail the first operation; two-row tests fail the second operation. UBSan reported no diagnostic, and allocator teardown reported no leaks.

All cases start with 1,000 Zeny, 20 units of payment material, and stock 5 for each listing. Each output costs 100 Zeny; barter also costs one material per output.

| Path and boundary | Observed outcome |
| --- | --- |
| Market success, one/two rows | Correct 100/200 Zeny debit, all outputs, memory and persistent stock both 4 |
| Market Prepare or Execute failure, one row | Reports success; debits 100 and grants output; memory stock 4, persistent stock still 5 |
| Market failure on second row | Debits 200 and grants both outputs; memory stock 4/4, persistent stock 4/5 |
| Market write applied but acknowledgement lost | In this specific injected outcome, payment, output, and both stock copies agree; this is counted as consistent, not falsely labelled a failed purchase |
| Barter success, one/two rows | Correct Zeny/material debit, all outputs, both stock copies agree |
| Barter Query failure, one row | Reports failure after debiting 100 Zeny and one material; grants no output; memory stock 4, persistent stock 5 |
| Barter Query failure on second row | Debits 200 Zeny and two materials; grants only first output; memory stock 4/4, persistent stock 4/5 |
| Barter write applied but acknowledgement lost | Still reports failure after payment; grants no/partial output even though the failed operation's durable stock decrement applied |

Evidence: `doc/evidence/transaction_sql_20260929/baseline.log`, `baseline-inputs.json` (source hashes and exact network-denied test command), and `npc-purchase-baseline.cpp` (the three original source sections used for replay).

## Source and schema findings

`npc_market_tosql` returns void, logs Prepare/Execute errors, and cannot tell the caller that stock persistence failed. `npc_buylist` has already charged Zeny, then decrements stock in memory, invokes that helper, and delivers the item regardless of persistence outcome. Restart/reload can restore stale durable stock.

`npc_barter_purchase` consumes materials and Zeny before its per-output SQL `REPLACE`. It decrements the in-memory stock before issuing the query. On failure it returns without restoring payment or delivering the current output; earlier outputs in the same cart may already have been delivered.

The separate cash-shop sale path has the same durability class, established by source inspection rather than an additional native runtime fixture here:

- `cashshop_buylist` sends the output grant and success result before persisting the remaining sale stock. An `UPDATE` failure is only logged; the in-memory count changes while the durable count may remain old.
- When the remaining count reaches zero, it calls `sale_remove_item` and ignores the return value. A failed `DELETE` causes that helper to return before removing the sale or its old stock count. The already-paid output has been delivered, while the last unit can still appear available in memory.

`sql-files/main.sql` declares `market`, `barter`, and `sales` as MyISAM (around lines 876, 97, and 934). A recursive inspection of `sql-files` found no later main-schema or upgrade engine conversion for these tables. The relevant upgrades change column types; the barter creation upgrade also uses MyISAM. Optional `sql-files/tools/convert_engine_innodb.sql` converts market and sales; its MyISAM counterpart can convert them back. These tools do not establish which engine is actually deployed. **Live table engines were not inspected.** Even an actual InnoDB engine would not repair the current absence of a transaction and shared durable purchase receipt.

## Why a small reorder is insufficient

Three hypotheses were tested against the failure evidence: ignored market SQL status predicts successful delivery even on Prepare failure (confirmed); barter debit-before-SQL ordering predicts paid missing outputs (confirmed); multirow writes behaving as one atomic transaction predicts no partial durable counts (refuted by the two-row boundary fixture, and unsupported by the default schema).

Returning an error before delivery after payment causes asset loss. Writing stock before payment can consume stock without a completed purchase. Compensating SQL can fail as well. A failed query acknowledgement cannot distinguish a write that never happened from a committed write. Blindly reverting local inventory after an uncertain remote commit can duplicate assets. `START TRANSACTION` does not provide rollback for MyISAM tables. Fixing only the in-memory count masks the persistence divergence rather than resolving it.

## Concrete follow-up design

Reuse the repository's durable purchase patterns, rather than inventing a separate rollback convention. `src/custom/reserve_sql.inc` checks all participating table engines, combines inventory/points/receipt in one transaction, and recognizes repeat request IDs. `bank_sql.inc` and `pair_inter.inc` demonstrate pending-save locks and why rollback is forbidden after an uncertain submission.

A stock purchase should have an immutable operation ID and complete request digest, staged final inventory/currency snapshots, and a durable receipt owned by the same service that saves player assets. Validate the engines of **all** stock, inventory, currency and receipt tables; a deployment migration and rollback/recovery plan must precede enabling this path. Lock/version the relevant stock and player rows, validate expected state, apply every stock decrement and player mutation, and insert the receipt in one transaction. Replays must verify the digest and return the recorded result without reapplying an old snapshot. Keep the player and affected shop quantities pending while outcome is unknown; retry/query the receipt after reconnect instead of issuing compensating grants/debits. If databases cannot share a transaction, design an explicit durable reservation/recovery protocol; do not call it atomic SQL.

Before rollout, test first/middle/last write failure, transaction/commit failure, acknowledgement loss before/after commit, disconnect/restart between every stage, duplicate/reordered retries, changed payload under the same request ID, stale stock versions, concurrent buyers, and refusal of nontransactional/mixed-engine tables. Verify receipt, inventory, currency and stock invariants after recovery. Sale exhaustion and pet creation need their own durable outcome handling alongside the other agents' in-memory delivery work.

This report records a completed bounded reliability investigation, not a completed durable-transaction repair or a deployment approval.
