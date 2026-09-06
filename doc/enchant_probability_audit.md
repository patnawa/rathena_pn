# Native initial enchant and reset audit — 2026-09-06

## Confirmed handler defects

The previous `clif_parse_enchantwindow_general` selected a uniformly random
enchant, accepted it using a 0–10000 draw, retried a small number of times, then
fell back to a uniformly random enchant. Database/client item probabilities use
a 100000-point scale. That algorithm did not implement those probabilities and
could select a zero-weight outcome through its fallback.

The replacement partitions a draw from 1 through 100000 using cumulative item
weights. It validates the entire table before charging anything: null/invalid
items, empty tables, and totals other than 100000 are rejected. Zero-weight
entries are never chosen. All 339 effective normal-enchant grade tables currently
total 100000, so this validation does not disable an existing configured table.

Two grade-bonus defects are also corrected:

- The parser read `Chance` from the slot instead of the `EnchantgradeBonus`
  entry. A valid entry with no explicit slot-level `Chance` failed to load.
- The handler summed every grade's bonus, regardless of the selected item's
  grade. It now adds only that grade's bonus and safely caps the result at 100%.

The isolated fixture then exposed a separate validation defect: the shared rate
parser rejected explicit zero values. Normal success, grade bonuses, normal
outcome weights and reset rates now accept the inclusive range 0–100000. Weighted
upgrade outcomes retain their existing positive-only validation. In particular,
an import can now explicitly disable a previously enabled reset with `Chance: 0`.

The client helper's `SetGradeBonus` stores a value indexed by grade and validates
each `SuccessRate + gradeBonus` separately. These are not cumulative bonuses.
The current 103 normal-enchant slot configurations all have 100% base success;
the grade-bonus fixes protect lower-success configurations and future data.

Success/reset draws now use 1–100000 with inclusive comparison. A 0% chance can
never succeed; 100% always succeeds. Reset with chance zero is rejected before
charging its configured price/materials. Previously that disabled-reset check
was after charging. Random/guaranteed upgrades remain separate and unchanged.

The shared item-state check also rejects unidentified, empty/stacked, equipped,
equip-switch, broken, invalid-grade targets and items whose card array contains
creator/forge/pet metadata. Those metadata fields must not be mistaken for
enchant slots or erased by reset. This does not implement a new way to combine
special card metadata with enchant slots.

## Tests and their limits

`tools/ci/enchant_probability_test.cpp` tests the exact helper/types used by the
handlers. Its 39 checks include exhaustive draws for weighted outcomes (including
one-in-100000 and zero-weight entries), success/reset rates of 0, 1, 10000, 50000,
99999 and 100000, current-grade-only bonuses, overflow, invalid distributions,
and item-state rejection. AddressSanitizer and UndefinedBehaviorSanitizer pass.
The same compiled helper also passes every draw in all 339 effective database
tables: 33,900,000 draws with exact declared outcome frequencies, under both
sanitizers. This uses real merged server tables, not just synthetic distributions.

```sh
g++ -std=c++17 -Wall -Wextra -fsanitize=address,undefined -Isrc \
  tools/ci/enchant_probability_test.cpp -o /tmp/enchant_probability_test
/tmp/enchant_probability_test
python3 tools/ci/audit_initial_enchants_test.py
python3 tools/ci/audit_enchant_upgrades_test.py
python3 tools/ci/audit_initial_enchants.py /path/to/EnchantList.lub \
  --client-item-names /path/to/ItemDBNameTbl.lub --emit-probability-fixture \
  | /tmp/enchant_probability_test --database
```

The Python suites have 7 initial-configuration and 23 upgrade/identity tests.
They cover reset/default semantics, material overlays, per-grade probability
tables, selectable initial enchants, whitespace and unsupported declarations,
and synthetic Lua data without running Lua.

`npc/test/enchant_probability.yml` is an isolated parser fixture, **not** a
production import. With the previous binary, the first fixture reproduces
`Missing node "Chance"`. After fixing the lookup, it exposed the separate
zero-value rejection. The three fixtures cover omitted base chance, zero grade
bonus/outcome, a differing base/bonus pair, zero base success, and disabled reset.
To load them in a disposable/candidate container, mount
the fixture over `db/item_enchant.yml` and mount the original root database at
`npc/test/enchant_probability_base.yml`. Run `map-server --run-once`, inspect
errors and the fixture's loaded-entry count, and then run again without those
two mounts. Never add the fixture to normal database imports.

The integrity audit checks that the real handler invokes weighted selection
before charging and rejects disabled resets before charging. These source checks
supplement, not replace, the compiled tests and runtime parser fixture. None of
these tests proves client click handling, actual currency/material transactions,
relog persistence, or combat effects.

## Configuration comparison still requiring follow-up

`tools/ci/audit_initial_enchants.py` reads literal declarations from the active
`EnchantList.lub` and resolves item IDs using the active `ItemDBNameTbl.lub`.
It compares initial rolls, selectable initial enchants, target eligibility,
slot order and reset settings against effective Renewal database overlays.

```sh
python3 tools/ci/audit_initial_enchants.py /path/to/EnchantList.lub \
  --client-item-names /path/to/ItemDBNameTbl.lub --details
```

The first comparison covers 157 shared groups, 339 client normal grade tables,
and 2307 selectable initial-enchant recipes. It reports 89 differences:

- 65 selectable recipes absent from the server, primarily client references to
  newer item names such as `Wolf_Orb_Skill_52`, `Glacier_F_Orb_192`, and
  `Automatic_Orb99`. There are no cost mismatches in the existing compared
  selectable recipes.
- 13 target lists missing 27 distinct client names, including the `AT` equipment
  families and additional Sky Rune Crowns. Their item IDs/definitions still need
  authoritative resolution; IDs and effects must not be guessed.
- One Gray Wolf probability table differs because the client adds three newer
  skill orbs and rebalances the existing weights. Adding only the probabilities
  without those missing item definitions would be incorrect.
- Ten Biosphere crown grade tables include deliberately added POW/CON or
  Fierce Attack/Great Craftsman enchants. They remain unchanged; they are custom
  differences, not automatically missing content.

The resolver also flags 71 distinct names across the full client configuration
(including groups outside the comparison). These are unresolved identities,
not an additional count of proven server defects. Both client and server
probability totals pass. All compared reset settings, slot orders, minimum
refine/grade conditions, random-option allowances and normal-enchant costs match.

The earlier ordinary/guaranteed-upgrade comparison still passes all 1245/444
client recipes. This is not a full enchantment or gameplay completion claim.
