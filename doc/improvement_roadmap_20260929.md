# rAthena PN improvement roadmap — 29 September 2026

Review baseline: local branch `codex/server-improvements-20260919`, HEAD `ce00fd541`, including the pre-existing uncommitted work. This is a source/documentation investigation, not a fresh production inspection or runtime certification. No game code, configuration, database or deployment was changed. Earlier test results below are attributed to their recorded runs; no native suite was rerun for this review.

The strongest next step is to complete reliable reward delivery and turn the existing progression tools into actionable player guidance. This project already has durable stock purchases, bank protections, extensive native tests, restore-verified backups, a signed client updater, an office directory, a progression guide, a weekly board and a damage lab. Build on these investments.

## Recommended order

| Priority | Work | Why now | Relative scope |
| --- | --- | --- | --- |
| 1 | Market restoration initialization regression | A concrete unsafe source pattern remains outside the new purchase protocol | Small repair; focused native/SQL validation |
| 2 | Durable pet entitlements | Previously reproduced delivery defects remain open | Large, coordinated map/char/schema work |
| 3 | Make release evidence automatic and scope-aware | SQL proof is separate; workflow branch triggers differ from the deployed branch | Medium |
| 4 | Correct barter allocation and post-exchange capacity | Valid exchanges can be refused by current source logic | Medium |
| 5 | Actionable onboarding and instance readiness | Extend working player tools with exact next steps | Small-to-medium pilot |
| 6 | Resolve dynamic reward catalogs and require rendered journeys | Static item coverage and native tests have explicit limits | Medium, incremental |
| 7 | Measure transaction delays and map responsiveness | A process can be healthy while purchases are stalled | Medium; measure before redesign |
| 8 | Persist reproducible damage-lab comparisons | Useful player-facing differentiation on existing infrastructure | Small-to-medium |

Effort labels express relative complexity, not calendar estimates. Production exposure, concurrency and player frustration were not measured here.

## 1. Fix market restoration before extending shop features

**Source-confirmed unsafe pattern; no new runtime reproduction.** In `src/map/npc.cpp:4957`, restoring a dynamically added market entry expands the array, assigns name and price, then tests the new element's `qty` before assigning it. `npc_item_list` provides no default initializer (`src/map/npc.hpp:34`), and `RECREATE` uses reallocation (`src/common/malloc.hpp:96`). The branch then sends the entry to SQL. Thus its decision depends on newly allocated, uninitialized storage.

The same pattern is described in the first-party [upstream issue #10103](https://github.com/rathena/rathena/issues/10103), opened September 15, 2026 and shown open when checked for this review. Its reported production outcome belongs to that reporter, not this server. The local source independently establishes the pattern; active exposure depends on runtime-added market entries surviving into restoration.

**Proposed change:** Initialize the entire new entry and assign persisted stock deterministically. Preserve the separate existing-entry semantics for unlimited stock; do not copy an uninitialized sentinel test into the new-entry branch.

**Acceptance:** Restore a persisted runtime-added item under deliberately different allocator fill patterns; quantity must be identical in memory and SQL. Cover restart, reload, zero stock, supported unlimited semantics and concurrent-purchase fencing. Check current production exposure read-only before deciding repair rollout urgency.

## 2. Make pet rewards durable and recoverable

**Confirmed outstanding work, supported by prior deterministic probes and current source.** The pet reply includes account/class/pet IDs, but no intended character or request ID (`src/char/int_pet.cpp:126`). `pet_get_egg` resolves the current account session (`src/map/pet.cpp:1376`) and requests deletion when inventory addition fails (`:1403`). `pet_create_egg` ignores dispatch success (`:684`). The [existing audit](transaction_async_audit_20260929.md) records duplicate callbacks, failed dispatch, capacity changes and character-switch failures, with precise fixture limits.

**Player promise:** A paid reward remains claimable after full inventory, disconnect or restart, and can be delivered only once to its intended owner.

**Implementation:** Persist payment/consumption, pet identity and an entitlement atomically. Carry account, character, request identity and output ordinal across the protocol. Replace the ambiguous boolean result with explicit not-a-pet/rejected/pending outcomes. Keep undelivered entitlements rather than deleting the pet as compensation. A Recovery Desk can expose pending entitlements after this durable mechanism exists; a UI alone cannot fix delivery.

Cover all producers identified in the audit, including taming, script grants, boxes, mail and paid rewards. Stock-backed pet purchases currently fail before payment; retain that protection until the new path is proven.

**Acceptance:** Real isolated-SQL crash/retry tests; repeated/altered requests; full inventory; character switching; transfer followed by retry; concurrent saves; multi-egg outputs. No duplicate pet/output, wrong-character delivery or paid reward loss. Do not treat the diagnostic `--known-failures` mode as release acceptance.

## 3. Make the strongest tests part of the normal release contract

**Confirmed wiring gap.** `tools/ci/shop_recovery_sql_test.py` supplies real SQL proof, but is absent from the test lists in `tools/ci/release_checks.py:25` and the checked workflow files. The PN workflow builds tools/map and invokes that runner (`.github/workflows/pn_release_checks.yml:33`). The SQL harness separately requires character objects and a validation image (`tools/ci/shop_recovery_sql_test.py:21`, `:95`). Simply appending the filename to the existing list would not provision its dependencies.

The PN workflow's push trigger targets `main` (`.github/workflows/pn_release_checks.yml:6`); CodeQL's push trigger targets `master` (`.github/workflows/analysis_codeql.yml:12`). The current recorded deployment branch is `codex/server-improvements-20260919`. PR/manual triggers also exist, so this is not a claim that CI never runs. Automatic coverage of direct release-branch pushes is the gap.

**Implementation:** Add a separately provisioned SQL recovery job, build the required character objects, pin its reproducible toolchain/database inputs, and require it for relevant release candidates. Align release-branch triggers. Bind server binaries, migrations, client overlays, test reports and deployed state to a single machine-readable manifest.

Keep capability results explicit: native logic, actual SQL recovery, rendered client acceptance and deployment attestation are different evidence. The [shop deployment receipt](shop_purchase_recovery_deployment_20260929.md) already explains why the 55-check workspace gate differs from the 52 applicable checks in the pushed shop-only release. Automate that distinction instead of reducing everything to one green check count.

**Acceptance:** A clean checkout can reproduce the required jobs; a deliberate SQL transaction fault blocks release; a direct release-branch push triggers validation; missing mandatory evidence is blocked rather than passed. Reconcile the existing local NPC fixes and 184-item tooltip candidate as separately scoped releases, preserving their uncommitted work.

## 4. Use one inventory plan for barter eligibility and execution

**Source-supported false-refusal cases, not newly executed tests.** The stackable material loop accumulates the whole requirement into the first matching stack and immediately fails if that stack is too small (`src/map/npc.cpp:3268`). It does not continue distributing that requirement across other eligible stacks. Capacity is tested against current free slots before consumed inputs free their slots (`:3402`). The later stock-purchase planner already simulates consumed inputs and outputs (`src/custom/shop_map.inc:27`), but cannot rescue a request rejected by earlier checks.

**Player examples:** An exchange requiring 10 materials should succeed with eligible stacks of 4 and 6. A full inventory should permit an exchange that completely consumes a material stack and puts its one output in that freed slot, when all weight/metadata rules also permit it.

**Implementation:** A deep inventory-planning module accepts the inventory, consumption rules and outputs, then returns either a complete immutable plan or a precise refusal. Both eligibility and commit preparation use that result. Keep bound/rental/favorite/refine policy explicit; do not solve allocation by silently broadening which items may be consumed.

**Acceptance:** Split stacks, overlapping requirements, freed slots, incompatible metadata, weight limits and multi-output carts. Every rejected plan leaves inventory and money unchanged. Cover limited and unlimited barter paths.

## 5. Give players a concrete next objective

**Existing interface to extend:** `PN_GuideNext` gives broad advice below level 200 and delegates in-progress Chapter 1 to the quest window, while Chapter 2 has detailed state-aware guidance (`npc/custom/main_office/services.txt:191`). The Instance Desk reports reservations but delegates all access/cooldown checks to entrances (`:283`).

**MVP:** Ten onboarding/Chapter 1 states that show one exact next objective and destination, plus five instances displaying ready/missing prerequisite/cooldown/reserved. Add reward-material tags for players pursuing an equipment goal. Use shared read-only eligibility logic with the actual entrances; always revalidate at admission.

**Acceptance:** Guide results agree with native quest/admission behavior at level, cooldown and party boundaries. Viewing guidance mutates no progression or economy state. Measure time to find a valid objective and wasted entrance trips in a small trial before claiming retention gains.

## 6. Close content evidence gaps incrementally

The latest [client audit](client_item_consistency_audit_20260929.md) validates metadata/resources for 6,660 statically traced obtainable IDs but leaves **199 dynamic grant lines unresolved**. These are unknowns, not 199 proven defects. Rune Tablet computed rewards provide a concrete starting point (`npc/custom/rune_tablet/services.txt:276`).

**MVP:** Generate typed output catalogs from authoritative recipe definitions for Rune Tablet and two other dynamic services. Verify actual grant families through isolated native fixtures, then check their effective server/client definitions. Keep unresolved paths visible. Extend the catalog into a currency/item conversion graph later, with repeatability, reset, binding and random-outcome conditions; literal-price checks alone do not establish absence of economic loops. Suspicious graph cycles require runtime confirmation.

Complement this with five rendered candidate journeys: onboarding, complete encounter/reward, party reconnect/re-entry, shop/storage feedback and changed client visuals. The [rendered checklist](rendered_acceptance_20260914.md) remains incomplete, and the latest audit records only 37/52 costume effects visually confirmed. Asset presence and native tests cannot certify rendering.

**Acceptance:** Deliberately missing resources and incorrect declared outputs fail checks. Each journey records candidate hashes and expected/actual results. Report blocked, failed and passed separately.

## 7. Observe stalls before changing concurrency

`src/custom/shop_inter.inc:7` implements a process-wide stock-purchase fence. Retries occur every second (`:21`); a failed stock refresh keeps the operation pending (`:68`). This is a deliberate correctness tradeoff, not measured evidence that the server is currently slow. The health checker inspects process/log/backup/disk state (`tools/admin/health_check.py:53`) rather than transaction completion or map-loop latency.

**MVP:** Track oldest pending operation, commit/ack/refresh latency, retry count, busy refusals, map timer lateness and receipt-table growth. Use bounded metric labels; keep request identities in restricted diagnostic records rather than high-cardinality metrics. Let operators inspect why an operation is waiting.

**Acceptance:** An isolated slow/failed SQL scenario produces a visible stalled-operation signal while the process remains healthy. Establish p95/p99 and queue baselines under representative load. Only then consider per-character/per-stock coordination with canonical lock ordering and independent stock-cache reconciliation. Never unlock or refund an uncertain commit merely because a timeout expired.

## 8. Turn the damage lab into a reproducible comparison tool

The existing lab and notebook already support experimentation, but the notebook stores free text and six base stats (`npc/custom/main_office/services.txt:295`), and lab results use temporary character state. Preserve a small run history containing equipment/refine/enchant fingerprint, target settings, buffs/traits, elapsed time and server build identity. Show repeated-run median/spread and flag incomparable setups. Keep its documented target limitations visible rather than presenting it as a boss simulator.

**Acceptance:** Three saved runs survive relog; interrupted runs are excluded; different target configurations cannot be mislabeled as an A/B comparison. This is a player-value hypothesis, with no measured demand yet.

## Architectural follow-ups

- **Versioned durable receipts:** Shop receipts store the packed request bytes and compare `sizeof` plus `memcmp` (`src/custom/shop_sql.inc:35`, `:52`); the request embeds `item[MAX_INVENTORY]` (`src/custom/shop_commit.hpp:32`). Record an explicit schema/protocol version and define compatibility across item-layout upgrades. Measure receipt growth before compression/archival. Old receipts protect against replay; do not delete them by arbitrary age.
- **Durable post-commit effects:** Purchase data is durable, but achievement/equipment callbacks are not a durable outbox ([documented scope](shop_purchase_recovery_20260929.md)). Classify effects into reconstructible UI/status, durable progression and arbitrary scripts. Add idempotent event receipts only for effects with a defined replay contract; do not replay arbitrary scripts after a crash.
- **Focused transaction module:** Bank, storage, mail, paired transfers and shops each contribute pending/applying flags to `pc_transaction_pending` and `pc_transaction_locked` (`src/map/pc.hpp:1174`). After pet delivery provides another concrete consumer, consider concentrating request identity, retry and pending-state policy behind a small interface. Preserve operation-specific rules; avoid a broad rewrite of unrelated combat/NPC code.
- **Upstream fix tracking:** Maintain a reviewed fix ledger against [official upstream advisories](https://github.com/rathena/rathena/security/advisories) and applicable issues. Existing advisories include login, character-slot, party-booking and RODEX findings; their presence does not prove this fork is vulnerable. Verify patch applicability and local regression evidence before integration. The market restoration match above shows the practical value of this routine.

See [supporting gameplay research](improvement_research_20260929.md) for fuller player-feature evidence and acceptance criteria. Start with market restoration investigation, pet recovery and release automation; pair that engineering work with a small onboarding pilot so players see immediate benefit.
