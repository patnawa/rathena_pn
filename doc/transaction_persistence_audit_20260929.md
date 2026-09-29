# Transaction failure audit — 29 September 2026

**Recovery follow-up:** stock-backed purchases now have an implemented and
tested durable transaction protocol. See [purchase recovery](shop_purchase_recovery_20260929.md).
Pet entitlement recovery remains open. The original findings below are retained
as historical evidence.

Three parallel audit tracks examined SQL stock persistence, synchronous shop
delivery, and asynchronous pet rewards. The audit reproduced real failure paths;
the SQL and pet persistence defects remain open. No deployment, client update,
live purchase, or production database operation was performed.

**Deployment follow-up:** the shop delivery and related payment-validation
fixes are now live. See the [September 29 deployment receipt](shop_delivery_deployment_20260929.md)
for the exact scope, production-runtime checks and rollback. The no-deployment
statement above describes the audit itself.

## Findings and repair scope

| Area | Finding | Status |
| --- | --- | --- |
| Shop output callbacks | A quest condition fills the remaining inventory after the first of two rewards. All four purchase handlers charge for both but deliver only the first. | Local repair defers script-capable item callbacks until the synchronous batch ends. |
| Shop stack preflight | A matching bound stack is treated as available for a plain reward; a full inventory then rejects the paid grant. | Local repair checks the same stack metadata used by native addition. |
| Market/barter SQL | Stock writes fail after payment; barter loses payment with no or partial outputs, while market stock diverges between memory and SQL. | Confirmed unresolved; requires coordinated durable player/stock commits. |
| Pet reward replies | Repeated callback duplicates an egg, failed dispatch reports success, full inventory requests pet deletion, and a late reply can reach another character on the same account. | Confirmed unresolved; requires durable request identity and delivery receipts. |

The delivery scope is not a rollback transaction. It queues successful-add
autoequip and achievement work and coalesces quest refresh until shop processing
ends. It preserves ordinary grants outside a scope and checks output identity
before delayed equip work. SQL or asynchronous failures can still leave partial
transactions. Script-controlled shops and generic crafting/reward scripts are
not certified by this change.

The SQL audit has 16 executions: four healthy controls, two consistent
applied-write/lost-ack market outcomes, and ten failed correctness assertions.
The pet audit has four failed safety assertions and five controls. Both tools
are explicitly named `_audit` and remain outside release acceptance. A
known-failure reproduction is evidence of a defect, never a successful safety
test.

The default SQL schema uses MyISAM for market, barter and sale stock; deployed
table engines were not inspected. Even InnoDB would require actual transaction
and receipt integration. The static pet catalog trace finds an enabled paid
reward: Child Manager consumes 30 Barmeal Tickets before granting egg 9123.
That establishes configured exposure, not an observed production loss. The
trace still has 199 unresolved dynamic grants.

## Validation

The combined candidate passes **53 of 53 release checks**: 50 regression suites,
314 database YAML files, client compatibility assets and isolated map startup.
The new delivery suite passes 30 cases; the existing 30 shop cases remain green.
Native quest, achievement-condition and EquipScript timing are exercised, with
explicit combat-stat, transport and persistence boundaries documented in the
delivery report. The new fixture uses ASan and UBSan; the existing shop fixture
uses UBSan. Linked native objects are not wholly sanitizer-instrumented.

The map server and affected native objects were rebuilt against the previously
verified Ubuntu toolchain. All 5,377 checked source/data/test files match the
local workspace, excluding generated Makefiles and compiler products. The
candidate remained stable throughout the gate. Startup exited zero, reached
readiness and produced no gate-detected errors. This is a map rebuild, not a
new four-server release package.

The candidate is isolated at `/app/pn-transaction-audit-20260929/candidate`.
Evidence is retained in `Server-Development/transaction-audit-20260929`, including
`gate-1790679739/release.json`, per-check logs, `startup.log`, `build.json`,
`local-candidate-match.json` and `delivery-changes.patch`. Validation used an
internal Docker network and disposable SQL with private fixture configuration;
no production database was accessed. Fixture containers and the network were
removed after the gate.

| Artifact | SHA-256 |
| --- | --- |
| Gate candidate | `e06545aa708c7faf8e4d67c3cc2aae56b5782372a7f7d76d0298d9a843a59292` |
| Map executable | `657a6a71654119bcee8292cded0561de478878fabab6b0335d2fa2b6648b19e8` |
| Startup log | `0df426892931f3c75e8814cf26b8165d97d234eb4408f64a41d8d3c1d32f77dc` |

The release-gate pass validates its configured checks. It does not close the
separately reproduced SQL and pet failures, which are intentionally not counted
as successful safety checks.

## Next implementation

Implement a durable purchase receipt shared by stock, player inventory and
payment before expanding into more economy audits. Reuse the existing reserve
purchase and bank transaction conventions: immutable request identity, complete
payload validation, checked transactional table engines, authoritative player
ownership, pending-state save locks, atomic mutation plus receipt, and receipt
lookup after uncertain commits. Pet delivery additionally needs a durable
entitlement/outbox and character/session matching, so full inventory or logout
retains a deliverable reward.

Do not replace these protocols with a write reorder, unverified compensation,
an inventory-only duplicate check, or a pet bool-return change. The detailed
reports specify caller scope, migration requirements, failure injection,
reconnect/restart coverage, and acceptance criteria.

- [Delivery repair and runtime tests](transaction_delivery_audit_20260929.md)
- [SQL failure evidence and durable repair design](transaction_sql_audit_20260929.md)
- [Pet failure probes, enabled exposure and repair design](transaction_async_audit_20260929.md)
