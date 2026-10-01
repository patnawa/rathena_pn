# Asynchronous pet delivery audit — 2026-09-29

Four deterministic contract failures remain open. This audit changes no gameplay code, inter-server protocol, database, or enabled shop. It supplies failure probes and a scoped durable-delivery proposal; it does not certify pet rewards as safe.

## Reproduction and boundaries

`tools/ci/pet_async_delivery_audit.py` extracts the exact current `pet_create_egg` and `pet_get_egg` function bodies from `src/map/pet.cpp`, compiles them with ASan and UBSan, and executes explicit transport, catalog, session lookup, inventory and SQL-delete doubles. Production bodies are not rewritten. The doubles record dispatch, additions and deletion requests; they do not execute SQL or sockets. The test is independent of map-server objects.

Commands inside the network-denied Ubuntu `pn-bank-validation:20260929` container:

```sh
python3 tools/ci/pet_async_delivery_audit.py
python3 tools/ci/pet_async_delivery_audit.py --known-failures
```

The normal command exits 1 with four failed safety assertions. The audit-only command exits 0 only when those four defects reproduce and the five positive/control assertions pass; its output explicitly says **not release acceptance**. Do not add this known-failure mode to `release_checks.py`.

The original red execution used the temporary name `pet_async_delivery_test.py`; the final files were renamed to `_audit` to distinguish them from acceptance tests. Evidence and SHA-256 command receipts are in `Server-Development/transaction-audit-20260929/pet_async_delivery_*.{log,json}`. The four-defect red log ends `Contract failures: 4`; the known-failure run repeats all four with no sanitizer finding. These are deterministic body-level probes, not gameplay/network replays.

| Executed probe | Observed result | Limit |
| --- | --- | --- |
| Deliver callback `(account=1,class=1002,pet=42)` twice | Two inventory eggs reference the same pet ID | Repeated internal callback is injected; no client-triggered packet replay exploit was demonstrated |
| Reject `intif_create_pet` dispatch because char server is disconnected | `pet_create_egg` still returns true, with zero new requests sent | Transport result is doubled according to actual `intif.cpp` contract |
| Accept creation with one free slot, then fill the slot before callback | Callback requests deletion of the newly created pet row | SQL DELETE is recorded, not executed; persistence loss follows if that request succeeds |
| Request for character 2, replace live account session with character 3 before callback | Egg is delivered to character 3 | Session replacement is injected; real reconnect timing was not exercised |

Controls: normal request dispatches once; normal callback adds one egg; request is initially accepted before capacity changes; disconnected callback adds nothing and does not delete the row; request is accepted for the original character. A disconnected callback therefore leaves delivery unresolved, rather than providing recovery.

Ranked explanations tested by the probes were absent callback identity/idempotency, conflation of egg handling with transport success, and no capacity reservation or durable delivery record. Source confirms these boundaries: `0x3880` carries only account ID, pet class and pet ID (`src/char/int_pet.cpp:126`); its receiver resolves the current account session (`src/map/pet.cpp:1368`). No char ID, login generation or request ID can be validated there. Dispatch result is ignored at `pet.cpp:684`. Delivery failure deletes the pet at `pet.cpp:1410`.

Restart loss, SQL failure windows and retry recovery remain **source-derived risks**, not executed crash/SQL tests. No observed production loss or exploit is claimed.

## Producer and fallback inventory

There are 13 call sites of `pet_create_egg` outside its definition, plus three direct producers calling `intif_create_pet` without the helper. Line numbers describe this audit snapshot.

| Producer | Current behavior after dispatch or failure |
| --- | --- |
| `npc.cpp:2599` cash shop buy-list; `npc.cpp:2754` legacy cash purchase | Payment occurs first; false means ordinary `pc_additem` fallback, potentially an egg without a pet identity |
| `npc.cpp:2936` normal/market buy-list | Payment/market stock update occurs first; helper result is ignored |
| `npc.cpp:3415` barter | Debits occur first; helper false returns failure, without making asynchronous creation transactional |
| `cashshop.cpp:592` cash shop button | False falls back to ordinary item delivery; true allows purchase flow to continue before callback |
| `script.cpp:7779,7947,24086` getitem/getitem2 family/getrandgroupitem | False falls back to ordinary item delivery; true completes the command before creation callback; metadata on the prepared ordinary item is not carried by the pet creation request |
| `clif.cpp:25162` item package rewards | Box consumed first; helper results ignored |
| `mail.cpp:333` raw egg mail attachment | True marks item received and reduces pending slots before creation callback; false stops processing |
| `mob.cpp:3357` regular mob egg drop | Helper result ignored; ordinary drop path skipped |
| `atcommand.cpp:1531,1618` item/item2 administrative grants | False falls back to ordinary item creation |
| `atcommand.cpp:3337` makeegg direct producer | Checks immediate transport result, but callback still has the same delivery defects |
| `script.cpp:11744` makepet direct producer | Dispatch result ignored; script reports command success |
| `pet.cpp:1349` successful taming direct producer | Monster removed and success shown before unchecked creation dispatch |

A return-value-only fix is unsafe because several callers interpret false as “not a pet egg.” An inventory-only duplicate guard is also insufficient: moving, trading or reconnecting changes that inventory while the durable pet identity remains. Neither shortcut was applied. The generic map reward/output callback audit outside these pet producers is owned by the other transaction audit tracks.

## Enabled catalog exposure

Fresh effective Renewal import traversal and active NPC include traversal are recorded in `transaction-audit-20260929/pet_catalog.py`, `pet-catalog.json` and `pet-acquisition.json`. The scan used the existing typed acquisition tracer, retained source hashes, and inspected 925 enabled NPC files, 29,713 effective items and 2,999 item groups.

- 107 effective pet definitions reference 105 distinct egg IDs; 118 effective item definitions have type Petegg. Definitions lacking a matching pet record are not assumed to create valid pets.
- One valid egg has a static acquisition route: **9123 `Ep17_2_C_Admin1_Egg`**. Active `npc/re/merchants/enchan_sage_legacy_17_2.txt:251` consumes 30 Barmeal Tickets (`1000103`), then line 252 executes `getitem 9123,1`. This establishes configured paid exposure, not a completed VM or gameplay purchase reproduction.
- 69 effective item scripts invoke taming (`pet`/`bpet`); 41 have static acquisition paths. Successful taming therefore also deserves durable output handling. Individual captures were not executed.
- No direct valid pet egg entries were found in the effective cash shop database, no package pet outputs, and no `makepet` lines in enabled NPC files. This does not rule out dynamic shops, scripts, mail or already-held items.
- 199 dynamic grant lines remain unresolved by the static tracer. Prerequisite accessibility, generated grants, exact live stock, player holdings and all nested cash-shop-box routes are not certified.

## Proposed durable repair, following existing transaction patterns

Use the established bank/reserve conventions in `src/custom/bank_protocol.hpp`, `bank_commit.hpp`, `bank_sql.inc`, `reserve_sql.inc` and `sql-files/upgrades/upgrade_20260919_reserve_purchase.sql`: account+nonce+request primary key, authoritative character ownership, payload validation, InnoDB transaction, receipt lookup before applying an old snapshot, and matching acknowledgments. Reuse their transaction/locking conventions; do not repurpose a bank action or old pet packet incompatibly.

1. Add a versioned creation request/receipt containing account ID, intended char ID, session nonce, monotonically increasing request ID, producer kind, egg type/quantity, requested metadata and payment/reward identity. Replace the ambiguous bool API with separate `NotPet`, `Rejected`, and `Pending` outcomes. Callers must never fall back to plain eggs after a rejected pet request.
2. Add an InnoDB pet-grant ledger/outbox keyed by the same account/nonce/request identity, plus output ordinal for multi-egg requests. Store normalized immutable request fields, generated pet IDs, delivery state and owning char ID. An identical retry returns the same receipt; changed payload under the same key is rejected. Enforce unique output identity so retries cannot create another pet row.
3. In one character-server transaction, validate payment/reward eligibility, persist its debit/consumption, create pet rows, and persist the grant/outbox. For paid rewards, merely queuing after an independently saved debit is insufficient. Generalize the reserve commit pattern to the actual currency/material inventory; do not reconstruct metadata on refunds.
4. Deliver to an authoritative durable destination. Either commit egg inventory plus receipt atomically while the character is locked, or keep an outbox entitlement for later inventory/mail delivery. Full inventory, logout and destination failure leave a retryable entitlement, never DELETE a paid pet as compensation. Delivery retries must not replay a stale inventory snapshot or duplicate a mailed/transferred egg.
5. Resume unfinished entitlements on login/reconnect using their original character ownership. Map acknowledgments must match char/session/request, while durable receipts outlive the old session. Integrate pending state with existing transaction locks, item output callbacks, saves and logout so a concurrent save cannot overwrite the committed reward.
6. Migrate pet/inventory/payment/ledger tables to transactional engines as needed after backup and writer shutdown, and upgrade map and char together with explicit protocol compatibility. Keep receipts across rollback; define retention/reconciliation before pruning. Existing orphan pets require a separate reconciliation policy because old packets/rows do not reliably identify the original intended egg owner.

Required implementation scope includes all 16 producers above, inter-server lengths/parsers, char SQL creation and acknowledgment, save/logout ordering, metadata/quantity semantics, and reward script APIs (including the Child Manager payment). Rollout must not silently disable all pet purchases as a substitute for delivery.

Acceptance for that repair must add native caller and real isolated-SQL tests covering identical/mutated retries, failure before/after each commit boundary, disconnect/reconnect/character switch, full inventory, concurrent rewards/saves, transferred egg then repeated receipt, multi-egg requests, metadata, insufficient payments and process restarts. Only after those pass should a repaired pet acceptance suite join the release gate.
