# Skills and stone follow-up audit - 2026-09-29

No new gameplay defect was reproduced in this pass. The maintained native costume test now adds physical cardfix and partial combo removal coverage to the previous magic/status checks.

## Fresh evidence

The isolated native costume run passed **518 cases / 12,004 assertions**, including **46 independent numeric cases**, **472 distinct-output smoke cases**, and rejection of all four private fixture mutations. The run had no UBSan diagnostics, script warnings/errors, or reported native memory leaks.

New checks exercise the real `battle_calc_cardfix` for weapon and projectile attacks against a neutral medium player target. Purified stones produce 11,000 damage from a controlled 10,000-point input (Ultimate: 12,000). Ranger II lower size bonuses cover learned skill levels 0, 1, 2, 9, and 10. Stacking its level-10 15% size category with Purified INT's 10% class category produces 12,650 on both paths. These are physical cardfix stage results: Renewal applies short/long bonuses elsewhere in the attack pipeline, and this test does not claim full final skill damage.

Four Range cases remove the middle card while retaining the upper/lower cards, using both current and legacy middle stones with and without the anniversary garment stone. Native combo discovery/status recalculation must leave ranged bonuses of 6% or 9%, respectively. Reinserting the removed card must restore the entire original observed snapshot exactly. Card removal is an explicit inventory fixture boundary; native packet-driven unequipping is not covered.

## Additional current regressions

- Combat binding tests: 6 passed (native extracted binding probe included).
- MuhRO skill balance: 48 checks, zero failures.
- Dimension autocast runtime: 110 assertions, zero failures (extracted production functions with fixture world boundaries).
- Druid mentor: 18 passed using the current local test, which already accommodates the expanded catalogue and level cap. The older deployed copy still expected 366 pairs and level 275 and failed; this is test drift, not a gameplay finding.
- Druid item database: 6 passed, including its 588 refine cases/native extracted bonus probe.
- Druid gear enchants: 10 collected, 6 passed, 4 explicitly skipped (optional client/native integrations were not supplied).
- Druid crown validation was attempted but remains incomplete: its historical `/audit-item-aliases-20260906/.../itemdbnametbl.lub` fixture is absent from the container. No pass is claimed for it.

## Reproduction and limits

Evidence is under `Server-Development/queued-audit-20260929/skills-*` locally; the isolated remote source and evidence are `/app/pn-queued-audit-20260929/skills-candidate` and `skills-evidence`. `skills-build-evidence.json`, `skills-build-results.json`, and `skills-build-native.log` retain numeric outputs and hashes. No production deployment occurred.

The validation container's ASan runtime cannot link (`__sanitizer::struct_sock_fprog_sz`), so extracted probes used **UBSan only** in private test copies. The costume fixture already defaults to UBSan. Its private link invocation uses `-l:libzstd.so.1` because the image lacks the development linker alias. Production tests retain their original sanitizer/link defaults. Native support objects came from the isolated compiled candidate; costume `pc`, `script`, `itemdb`, `status`, `battle`, and allocator objects are the freshly hashed UBSan versions maintained by its build harness.

This is bounded regression and added numerical coverage, not a proof of every class's combat balance, every autocast proc, every multi-equipment combination, or live network equip transitions. Existing Druid crown cooldown native coverage was reviewed but was not rerun successfully in this pass because of the missing historical fixture.
