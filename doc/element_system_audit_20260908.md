# Element system audit — 8 September 2026

## Result

**No confirmed defect in the audited elemental table or core attribute helper.**
The effective local Renewal table contains all 400 attack/defense/level combinations,
and its database body equals the [rAthena Renewal table](https://github.com/rathena/rathena/blob/master/db/re/attr_fix.yml)
retrieved on the audit date. The LF-normalized local file SHA-256 is
`3695707520a23bb1e3c3e863703dcf86fc0bb912f9a34f68ad8e480f47ea653d`.
This is compatibility with the audited rAthena implementation, not proof that every
skill interaction exactly matches every official or private server.

No combat balance values or production source were changed by this audit.

## Effective configuration and important behavior

- `src/config/renewal.hpp` enables Renewal. `db/attr_fix.yml` imports
  `db/re/attr_fix.yml`, then `db/import/attr_fix.yml`; the local import has no overrides.
- Rows represent the **attack element**, columns the **target defense element**.
  The selected level belongs to the defender. For example, Water attacks against
  Fire defense use 150/175/200/200% at defense levels 1–4, whereas Fire attacks
  against Water use 90/80/70/60%. These are deliberately asymmetric.
- Neutral attacks against Ghost use 90/70/50/0%. A legacy chart showing 25% against
  Ghost 1 does not describe this server's modern Renewal table.
- Players start with defense element level 1 (`status_calc_pc_`). Monster database
  elements/levels are validated by `MobDatabase::parseBodyNode`; a scan of all
  **3,208 effective local monster records** found no invalid element or level.
  Freeze sets Water 1, Stone sets Earth 1, and Elemental Change applies its supplied
  element and level through the status calculation helpers.
- `attribute_recover: no` prevents negative attribute ratios from healing by default.
  Renewal's current base table contains no negative ratios. The helper's explicit
  flag allows intermediate negative values where its caller requires them.
- `attack_attr_none: 14` intentionally makes the configured non-player categories'
  ordinary attacks bypass this attribute adjustment. A monster hitting Ghost armor
  at full damage is therefore not evidence of a broken Neutral/Ghost table.
- Element resistances from equipment, attack-element bonuses, target-element bonuses,
  skill ignore-element flags, and status effects are additional stages. `@bs` values
  are not a prediction of the final damage multiplier. Renewal status effects such
  as Volcano, Spider Web and Oratio can add to the attribute ratio.

## Executed checks

Run from the repository root on Linux/WSL with PyYAML and g++ installed:

```sh
python3 tools/ci/element_system_test.py
```

Passed with AddressSanitizer and UndefinedBehaviorSanitizer:

- Effective import schema, complete 4 × 10 × 10 base table, ratio bounds, direction
  and level regression sentinels.
- The unchanged production `battle_attr_fix` and `AttributeDatabase::getAttribute`
  implementations, compiled into an isolated fixture: **2,800 damage cases** across
  every table cell, including 0, 1, fractional rounding boundaries and large damage.
- Invalid defense fallback, invalid attack random-element fallback, negative-ratio
  recovery disabled/enabled and the intermediate-negative flag.
- Volcano plus Spider Web, consumption of Spider Web, Oratio, and the two-stage
  Telekinesis Intense rounding path.
- Unchanged production defense-element helpers: Freeze, Stone, Elemental Change,
  and baseline defense-level bounds.

The fixture substitutes status storage, world objects, RNG and skill transport.
It does not pretend to be a whole running map server. Database import checks use
PyYAML; they do not replace native YAML loader/startup validation.

## Limits and remaining acceptance

This audit does not execute every skill, dual-wield branch, status priority, card
combination, or map damage modifier. In particular the source already records an
unresolved physical/left-hand interaction in `battle_calc_cardfix_debuff`; no
unsupported balance correction was invented here. The full weapon/magic/misc
damage pipelines are inspected call sites, not covered by this fixture.

Live deployment parity and native startup results belong in the combined audit
report. To verify actual gameplay, use controlled targets at known defense levels,
remove unrelated bonuses, and compare otherwise identical fixed-element attacks.
Include Ghost 1/4, Fire versus Water, Water versus Fire, same-element immunity,
Freeze/Stone transitions, weapon endows and one element-resistance card. Do not use
a single random physical hit as a deterministic table test because defense, attack
rolls and the Renewal status/weapon/equipment attack split also affect the result.
