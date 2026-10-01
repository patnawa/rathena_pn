# Costume combat/status audit — 2026-09-28

The isolated native run passed **517 cases and 11,939 assertions**: 45 independently specified numeric/combo cases and 472 unique-output smoke cases covering all 497 material/output catalogue pairs. Four deliberately incorrect private fixture variants were rejected. No gameplay defect was reproduced in this scope, so this audit changes no stone scripts or combo database entries.

## What actually runs

`tools/ci/costume_combat_native_test.py` builds the dedicated C++ driver against actual map-server objects. Fresh UBSan-instrumented code includes `pc.cpp`, `script.cpp`, `itemdb.cpp`, `status.cpp`, `battle.cpp`, and `malloc.cpp`. Native item/combo parsers, combo linking/discovery, `status_calc_pc_sub`, script bonus handlers, magic `battle_calc_cardfix`, and Renewal `skill_vfcastfix`/`skill_delayfix` execute. Native skill/job databases load their current Renewal imports. The skill timing code and other support objects are native linked objects, not replaced arithmetic models.

The fixture supplies a level-1 Novice, explicit costume inventory positions and card slots, a neutral unresistant player target, and transport/registry/world lookup boundaries reused from the established isolated VM harness. Synthetic learned skills use the native permanent-granted flag so skill-tree rebuilding preserves the deliberately supplied test levels. No game server or SQL connection starts; seccomp and the container deny networking. Hat effects use the native unit effect vector with login-time packet suppression.

## Independent numeric cases

The expected values are hand-transcribed from official descriptions summarized in `costume_source_verification_20260928.md`, `costume_taiwan_reference_20260928.md`, and `costume_stone_thai_reference_20260928.md`. The earlier 138 script-derived arithmetic fixtures are not used as an oracle.

- 13 Taiwan/baseline cases: anniversary standalone/combined bonuses; current and legacy Range Middle combinations; missing-middle and unrelated armor Expert Archer controls; anniversary delay/cast combinations. Full delay set supplies 3% from its three stones plus its existing 2% set bonus, then 3% anniversary base and 5% anniversary combo: 13% total. The official Taiwan client descriptions for 29053–29055 independently confirm that pre-existing 2% bonus.
- 24 Korean threshold cases: Ranger II upper delay and lower size bonuses at skill levels 0/1/2/9/10; Shinkiro II/Kagerou II lower CRI and melee combo at those levels; Night Watch II/Rebellion II lower fixed-cast/size combo at those levels; Sorcerer II upper fixed cast at Psychic Wave 0/1/4/5.
- 8 Thai cases: all seven Purified variants and Festa Supreme. These check all-class damage categories, ATK/MATK, HP, HIT, ranged/critical damage, traits, delay, and Greed availability where applicable.

Controlled native magic-cardfix hits start at 10,000 damage. Examples: anniversary alone yields 10,300; 22nd anniversary yields 11,100; both yield 11,400; Purified ordinary/Ultimate yield 11,000/12,000. Soul Strike Lv1 timing is checked through the real native timing functions, with one millisecond tolerance for existing floating-point truncation. At fixture INT/DEX=1 it has 469 ms total cast; the 23rd stone produces 351 ms; the anniversary/cast-stone combination produces 258 ms. Damage/cardfix is a real battle stage, not a full skill attack simulation.

## Exhaustive output smoke coverage

Each of the 472 distinct output enchants is placed in its catalogue category's actual slot. Native status calculation runs twice; the entire observed snapshot must remain identical. Actual native UnEquipScript then runs, the fixture removes its card and rebuilds combos, and native status recalculates. All observed numeric fields, skill-attack bonuses, Greed state, combo count, cast/delay/damage results, and native hat-effect IDs must return to the bare-player baseline. This covers the new 28 Korean and seven Taiwan outputs as part of the complete catalogue traversal.

This smoke pass proves script execution, repeat-calculation stability, and cleanup for the supplied isolated state. It does not independently prove every stone's advertised maximum-level number, every possible multi-stone/equipment combination, proc activation, player class interaction, or client rendering. The 22nd anniversary halo and excluded Red Flame visual remain outside any claim of verified visual behavior.

## Sensitivity checks and evidence

Private generated YAML is mutated in four separate runs: anniversary magic +3 becomes +4; a Ranger every-two-level divisor becomes three; Purified magic all-class becomes all-race; the Range three-stone combo is removed. Every run fails the independent native-result assertion. These are harness sensitivity tests, not discovered production bugs. Repository/live databases are never changed by them.

The final run reported:

```text
COSTUME_COMBAT_NATIVE_OK cases=517 assertions=11939
COSTUME_COMBAT_MUTATIONS_OK rejected=4
Memory manager: No memory leaks found.
```

No native script warnings/errors or UBSan diagnostics occurred in the passing run. `costume_combat_native_evidence_20260928.json` records the tested sources, databases, fixture/catalogue hashes, counts, and boundaries. Full results, inputs, native log, and four failing mutation logs are retained under `Server-Development/gameplay-refine-audit-20260928/combat-build/`. Earlier release evidence remains unchanged.

Reproduce on the isolated configured Linux candidate with matching map link objects:

```sh
LIBRARY_PATH=/tmp python3 tools/ci/costume_combat_native_test.py --build-dir /audit/combat-build
```

The validation used `improvement-validation:20260914`, network disabled, two CPUs, and a 3 GB memory cap. Do not link while another build rewrites the shared support objects.
