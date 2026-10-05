# Gameplay, content and operational improvement research — 29 September 2026

Read-only review of current local source and recent first-party evidence. No production inspection, new gameplay test, deployment or code change was performed. References below are repository-relative `file:line` anchors. Historic receipts establish what those runs covered, not the state of the currently running server. Acceptance targets below are proposals, not measured results.

The project already has an office, searchable service directory, progression guide, weekly practice board, damage lab, extensive native fixtures, acquisition tracing, verified backups and a health timer. Rebuilding those would waste effort. The strongest opportunities extend their incomplete boundaries.

## 1. Turn the Instance Desk into a personal readiness board

**Confirmed gap:** `npc/custom/main_office/services.txt:283` reports only the current personal/party reservation, then sends players to individual entrance NPCs for requirements and cooldowns (`:290`). This is distinct from the existing daily board, which covers Biosphere hunts, Phantom and Raised Land (`:240`). The existing instance catalog test verifies names, maps, enabled scripts and warper labels, not agreement between a player-facing eligibility result and admission (`tools/ci/instance_access_manifest_test.py:48`).

**Idea:** Display five high-value instances as “available”, “missing prerequisite”, “cooldown”, or “already reserved”, with remaining time and a navigation target. Add reward/material tags so players can find the encounter relevant to an equipment goal. Extract read-only eligibility functions from the actual admission logic; entrance NPCs must still revalidate before charging or entering. Avoid a separately maintained copy of each gate.

**MVP:** Five instances spanning level, story, party and time gates. Native fixtures cover immediately before/after each boundary, including one disconnected member and a party leader change. All board results match admission preconditions; viewing the board changes no quests, inventory, balance or reservation. In a small usability trial, measure time and failed journeys needed to find one currently available encounter; set a reduction target after collecting a baseline.

**Expected value / uncertainty:** Less wasted travel is plausible, but this review did not measure player confusion. Medium effort; high product value if players run multiple instances.

## 2. Fill the early-game and Chapter 1 guidance gap with actionable objectives

**Confirmed gap:** `PN_GuideNext` gives the same general preparation message to every player below 200 (`npc/custom/main_office/services.txt:192`); once Chapter 1 has started, it delegates to the quest window (`:204`). Chapter 2 already has much more useful stage-by-stage navigation and distinguishes a cleared encounter from an unreported quest (`:218`). Milestone/build advice exists too, but is broad (`npc/custom/main_office/weekly_practice.txt:20`).

**Idea:** Extend the existing guide with one concrete next objective, its exact blocking prerequisite and a safe route for the early progression bands and Chapter 1. Start with a small curated path supported by the currently active scripts. Offer a “working toward” equipment goal that explains its first obtainable material and destination. Keep alternative builds possible; do not prescribe one mandatory class/build or grant progression through the guide.

**MVP:** Ten representative new/returning-player states, including started-but-unreported Chapter 1 objectives. Each produces an actionable destination verified reachable in the installed client and consistent with quest state. A new player can identify the next task without a GM for at least 8/10 staged cases. Read-only guide interactions preserve all progression and economy state.

**Expected value / uncertainty:** Better onboarding is a hypothesis; source establishes the asymmetry in guidance, not a retention problem. Small-to-medium effort and a sensible first player-facing project.

## 3. Close dynamic item acquisition blind spots before broad catalog cleanup

**Confirmed gap:** The fresh audit reports client resources for all 6,660 statically traced obtainable IDs, but leaves 199 dynamic grant lines unresolved (`doc/client_item_consistency_audit_20260929.md:21`, `:54`). The tracer explicitly records dynamic grant expressions without resolving their IDs (`tools/audit_item_acquisition.py:96`). Real examples include Rune Tablet reward arrays and computed outputs (`npc/custom/rune_tablet/services.txt:276`, `:313`, `:331`, `:369`). Consequently, “no static path” cannot mean “unobtainable”.

**Idea:** Add typed output catalogs for the highest-use dynamic services and derive acquisition metadata from their authoritative recipe arrays. Supplement them with isolated native fixtures that record actual granted item IDs, then validate effective client metadata, slots, weights and decoded artwork. Promote this to a change-based gate for recipes, rewards and client overlays. Preserve unresolved status where the analyzer cannot prove a path.

**MVP:** Resolve the Rune Tablet output families plus two other dynamic services; every declared output must exist in the effective item DB and loaded client metadata. Exercise at least one real successful grant per output family. Reject a fixture with an intentionally missing icon or mismatched weight. Report resolved/unresolved counts and evidence type separately; no false “catalog fully verified” result.

**Expected value / uncertainty:** This addresses a demonstrated coverage gap, not 199 proven broken rewards. Do not restart the already prepared 184-item weight repair (`doc/client_item_consistency_audit_20260929.md:3`); reconcile and ship that candidate through the normal release path separately. Medium effort.

## 4. Make rendered journey acceptance a small recurring release requirement

**Confirmed gap:** The existing 30-scenario checklist explicitly remains incomplete and distinguishes native fixtures from a Ragexe playthrough (`doc/rendered_acceptance_20260914.md:3`). The 28 September native acceptance improved coverage but explicitly did not simulate all mission kills or render the protected client (`doc/muhro_gameplay_native_acceptance_20260928.md:18`). The 29 September item audit still cites only 37/52 costume effects as visually confirmed (`doc/client_item_consistency_audit_20260929.md:60`). These are documented evidence boundaries, not proof that gameplay is broken.

**Idea:** Maintain a small golden set of rendered journeys: new-player progression, one complete boss/reward loop, party reconnect/re-entry, storage/shop feedback, and one current client-overlay visual check. Bind captures and pass/fail results to candidate server/client hashes. Use native fixtures for preparation and invariant checks, with owner-assisted client execution where native control is unavailable. Retain the broader checklist as rotating coverage.

**MVP:** Five completed journeys on one disposable candidate with screenshots/video and expected/actual outcomes, including the real reward claim after a full encounter. A release report clearly distinguishes blocked, failed and passed. Rerun the impacted journeys after client overlays or corresponding gameplay changes; never substitute packet-only evidence for rendered acceptance.

**Expected value / uncertainty:** High confidence in the need for a clearer acceptance boundary. Medium recurring effort; prioritize this before adding more content whose complete player flow remains untested.

## 5. Measure responsiveness, not only process health

**Confirmed boundary:** The current health checker evaluates Docker state, recent error/disconnect counts, verified backup age, disk capacity and backup timer (`tools/admin/health_check.py:53`, `:92`). Its service pass is `ready and failures == 0` (`:69`). Those are useful checks but do not measure map-loop responsiveness. The timer dispatcher already calculates timer lateness and treats delays above one second specially (`src/common/timer.cpp:357`, `:377`). This does not establish that the server currently lags.

**Idea:** Add a lightweight histogram of map-loop/timer lateness and request latency for a narrow gameplay path, with bounded labels and no account IDs. Retain a short trend alongside the existing health result so a slow-but-running map service is visible. Use an isolated load scenario representing party combat, script activity and saves before choosing production thresholds. The goal is to establish capacity and detect regressions, not to prematurely retune combat.

**MVP:** Record p50/p95/p99 latency and timer-delay counts over a repeatable 30-minute idle/load comparison. Inject a deliberate stall into a disposable fixture and detect it while the process remains running. Prove bounded storage and acceptable sampling overhead. Establish alert thresholds from the observed baseline rather than inventing a universal concurrency target.

**Expected value / uncertainty:** Medium effort; useful operational visibility, with actual performance bottlenecks still unproven.

## 6. Upgrade the existing damage lab into a reproducible build notebook

**Confirmed limitation:** Lab results live in temporary character variables (`npc/custom/quality_services.txt:154`); the notebook saves three free-text notes and six base stats (`npc/custom/main_office/services.txt:295`). The dummy has zero hard DEF/FLEE and is status-immune (`npc/custom/quality_services.txt:126`, `:131`, `:134`). These intentional limits are already documented in `doc/quality_services.md:5`; the lab should not be described as a real boss simulator.

**Idea:** Save a compact run record with target configuration, skill/build label, equipment/refine/enchant fingerprint, relevant traits, elapsed duration and server build identity. Compare repeated A/B runs and flag changed target/buff assumptions. Add configurable hard DEF only if native target behavior can be verified; clearly label unsupported encounter mechanics. Do not add automatic loadout swapping as part of this MVP.

**MVP:** Persist three runs per configuration across relog, display median and spread, and refuse to label different target settings as comparable. Demonstrate a controlled equipment change produces a reproducible measurement difference while invalid/interrupted runs stay excluded. Preserve the existing zero-reward lab behavior and cosmetic weekly practice integration.

**Expected value / uncertainty:** Small-to-medium effort; strong fit for a development server and player build experimentation, with demand to validate through actual use.

Suggested sequence: complete a small rendered acceptance baseline, ship actionable onboarding guidance, then add the readiness board and dynamic-output checks. Add responsiveness metrics before promising population capacity; build-lab history is an optional differentiator after those foundations.
