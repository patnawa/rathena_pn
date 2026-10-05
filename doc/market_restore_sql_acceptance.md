# Market restoration SQL acceptance

`tools/ci/market_restore_sql_test.py --candidate /built/candidate --evidence /new/evidence`
runs the production market SQL loader, restoration callback, SQL writers and
stock-refresh body against disposable MariaDB. It links the candidate's actual
common `Sql`/`SqlStmt` implementation and records source, native archive, binary
and toolchain hashes. NPC lookup, the item catalog, DBMap and poisoned allocation
are explicit fixture boundaries.

The 49 probe processes cover quantities 0, 1, 37, INT32_MAX and -1 under four
allocation patterns; repeated restoration without duplicate entries; fresh
process restoration; five database restarts; and existing unlimited/zero/limited
catalog policy. The concurrent case holds an independent transaction's stock
row lock, exercises the actual pending-purchase writer/delete guards, reloads
the old committed stock, commits the other transaction and exercises the actual
acknowledgement stock-refresh function before releasing the fence. Memory and
SQL must both converge from 7 to 6. Reload need not be globally prohibited.

This complements `market_restore_test.py` (26 ASan/UBSan allocation cases) and
`shop_recovery_inter_test.py` (acknowledgement and fence lifetime). It does not
start a complete map process or execute a player's purchase. The independent
transaction is a controlled SQL writer, not the character-server commit handler.
Full shop SQL recovery and final map startup remain separate release gates.

The intermediate 29 September run passed all 49 probes and five database restarts.
Its report SHA-256 is
`070b8db968398126c656f11fc18e5b419e19abb15f71fb5b3a6201ef0e57943f`;
the remote evidence directory is
`/app/pn-improvement-delivery-20260929/market-proof-1790692713878446903/proof-1790693016830654653`.
Later source changes require a fresh report rather than relabeling this receipt.

A read-only production query at 2026-09-29 14:37:31 UTC found 246 persisted market
rows and zero rows with `flag & 1` (runtime-added entries). Therefore no currently
persisted runtime-added exposure was observed. This does not establish future
absence or make the initialization defect safe. The query made no changes.

## Existing unfinished acceptance inventory

These tests already belong to `release_checks.py`'s full gate; preserve them when
adding new suites:

| Changed service | Required test | Relevant behavior |
| --- | --- | --- |
| Wise Old Woman | `card_removal_transaction_test.py` | Final equipment/card/payment revalidation, fee and outcome committed together, exact metadata retained on technical abort, quest callbacks cannot observe partial payment |
| Apprentice Craftsman | `armor_enchant_transaction_test.py` | Identity rechecked after progress, unsupported metadata refused, advertised ordinary success/destruction and full-slot replacement |
| Mayomayo | `mayomayo_payment_test.py` | Marked or mixed payment records refused before mutation, split plain stacks charged/refunded exactly, weapon metadata and final-dialogue changes guarded |

The combined Rune client candidate v4 includes the existing 184 weight records
and Sealed Drake tooltip work. Its Lua/export/asset checks are metadata evidence;
the staged private client and pending five-journey rendered plan are separate.
Neither native NPC tests nor client file presence prove native rendering.
