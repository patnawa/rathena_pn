# Four-area audit — 29 September 2026

This pass covers all four areas in `Server-Development/NEXT-JOBS.md`. Three
parallel agents handled economy, combat and client consistency; the parent
handled card transactions and combined validation. Confirmed defects have local
fixes and regression evidence. Nothing was deployed, published or installed.

## Findings and changes

| Area | Result |
| --- | --- |
| Item-changing NPCs | Fixed card-removal fee loss across a dialogue pause, stale final confirmation, and bound-material refund conversion. Legacy armor enchanting now refuses metadata it cannot preserve and revalidates after its progress bar. Mayomayo refuses payment records that its plain-item refund cannot reproduce. |
| Shops and currencies | Fixed price multiplication/narrowing, duplicate-row partial delivery, market index truncation, and barter material callbacks. Covered both NPC shops and the cash-shop button. The shared cash-payment helper rejects negative prices and caps Kafra payment at the total price. |
| Skills and stone combinations | Expanded physical damage-stage and partial combo-removal tests. No new gameplay defect reproduced within this scope. |
| Client/server item consistency | Prepared 184 verified weight-tooltip corrections. All 6,660 statically obtainable IDs have metadata and referenced inventory/collection art. Candidate remains uninstalled. |

The two legacy enchanting fixes intentionally restrict unsupported inputs:
Apprentice Craftsman refuses bound, rental, favorite, damaged, graded or
random-option armor; Mayomayo requires plain, permanent, unbound payment stacks
without metadata. These restrictions prevent data loss; they do not add native
support for preserving every modern field through the old reconstruct/refund
services. Players receive an explanation before payment.

## Verification

| Test | Scope/result |
| --- | --- |
| Card-removal NPC and native payment | 247 cases / 7,990 assertions, actual VM, all random outcomes, real quest conditions, metadata, refusals and legacy API compatibility |
| Apprentice Craftsman | 15 cases / 840 assertions, actual VM, progress-bar changes, metadata refusals and full 200-slot inventory |
| Mayomayo | 35 cases / 1,511 assertions, actual VM, metadata refusals, plain payment success/refunds, mixed/split stacks and full inventory |
| Shop transaction handlers | 30 cases using extracted production handlers plus real inventory/payment code, including native in-memory cash registry records |
| Costume combat | 518 cases / 12,004 assertions, 46 independent numeric cases, 472 output smoke cases; four deliberate mutations rejected |
| Refinement/reform/grading | 110,000 refinement outcomes, 13 payment guards, 27 reform cases, reform reply/reconnect protocol, grading handlers and eight certificate checks passed |
| Additional regressions | Bank arithmetic: 500,000 plans; Fashion: 105 cases / 5,324 assertions and five mutation rejections; progression: four tests; mentor: 18; combat bindings: six; skill balance: 48 checks; Dimension runtime: 110 assertions |
| Client overlay | Native Lua exact-delta comparison: only 184 identities / 352 description callbacks change; all 27,133 client identities and other callbacks preserved |

New NPC/shop regressions are registered in the full release-check suite. The
standalone portable runners were exercised on the final isolated candidate,
alongside the release-runner's own 13 tests. `final-tests.json` retains their
exit codes. `final-build.json` binds source hashes to the candidate map binary;
`final-startup.json` records parsing/startup against disposable SQL on an
internal Docker network. Production SQL was not used for those tests.

Final combined validation passed: **327 transaction cases**, the release-runner
tests, the complete candidate map build and isolated startup. The closing
read-only check found all seven production services healthy and the recorded
production source/binary baseline unchanged (`final-health.json`). The installed
client loader still matches its pre-audit hash. No rollout was performed.

The validation image cannot link AddressSanitizer. UBSan was used for the
supported native/extracted checks; no ASan pass is claimed. Some pre-existing
source probes explicitly skip optional client integrations.

Evidence directory: `Server-Development/queued-audit-20260929`. The isolated
combined source/build is `/app/pn-queued-audit-20260929/candidate`. Client evidence
is in `Server-Development/client-consistency-20260929`. Early failed runs and
fixture/environment repairs are retained separately from final receipts.

## Remaining audit work

1. **Bank fixture blocker resolved in follow-up.** Bank and RODEX fixtures now
   match current integration interfaces. The complete isolated release gate
   passes 52 checks with working sanitizers and a fresh map build. See the
   [follow-up evidence and coverage limits](bank_release_gate_20260929.md).
2. **Transaction persistence and output callbacks received a follow-up audit.**
   [Failure evidence and local delivery repairs](transaction_persistence_audit_20260929.md)
   cover callback interleaving and incompatible output stacks. SQL stock writes
   and asynchronous pet delivery have confirmed unresolved defects requiring
   durable receipts. Split-metadata barter allocation and other legacy
   socket/crafting NPCs still need targeted runtime coverage.
3. **Dynamic acquisition and economy graphs.** The literal-price scan found no
   simple profit candidates in 3,403 listings across 925 enabled scripts.
   It does not prove arbitrary reward/reset/currency-conversion graphs. The
   client acquisition tracer still has 199 unresolved dynamic grant lines.
4. **Full combat and visual acceptance.** Damage-stage and status tests do not
   cover every skill's final attack pipeline or cooldown interaction. Druid
   crown historical alias fixtures were unavailable. The prior visual evidence
   remains 37 of 52 costume effects, with 15 unconfirmed; no new rendering pass
   closes those gaps.

Full-catalog client discrepancies outside static acquisition remain reported,
not deleted or filled with guessed assets. No fresh live inventory query was
used to decide whether those items are owned by players.

## Detailed reports

- [Card removal](card_removal_transaction_audit_20260929.md)
- [Armor enchanting](armor_enchant_transaction_audit_20260929.md)
- [Mayomayo payments](mayomayo_payment_audit_20260929.md)
- [Shops and currencies](shops_currency_audit_20260929.md)
- [Skills and stones](skills_stones_followup_audit_20260929.md)
- [Client item consistency](client_item_consistency_audit_20260929.md)
