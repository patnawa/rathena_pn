# Player battle-stat reports

All player groups can use `@battlestats` (`@bs`) for offense and `@battlestats2`
(`@bs2`) for defense. The commands report the current character only and never
alter stats, equipment, quests or combat statuses.

The reference server battle-stat feature is the functional
reference. Values are calculated from this server's actual fields and formulas;
the reference's illustrative numbers and mislabeled descriptions are not copied.

| Topic | Contents |
|---|---|
| `summary` | Level, HP/SP/AP and main offensive or defensive stats |
| `stats` | All twelve attributes: current total = allocated base + job table + other |
| `race` | Race modifiers, including all-race entries |
| `size` | Small/medium/large modifiers, including all-size entries |
| `element` | Enemy armor element and spell element; incoming attack element reductions on defense |
| `class` | Normal, boss, guardian, battlefield and event class modifiers |
| `groups` | Special monster groups, including Rune Tablet episode groups |
| `skills` | Individual skill damage or damage-reduction modifiers |
| `casting` | Stat-based variable-cast reduction, script adjustments, per-skill cast modifiers, delay and SP cost |
| `drops` | All-target, race, class and item-buff drop modifiers |
| `help` | Usage and interpretation |

Examples: `@bs race`, `@bs2 element`, `@bs stats`, `@bs skills 2`.
Detailed modifier tables omit zero rows and show at most twelve rows per page.
Invalid topics and out-of-range pages produce an error rather than dumping chat.

## Interpretation

These are **current snapshots**, not equipment-only totals. Calculated attributes
and bonus arrays include equipment, cards, Rune Tablets and applicable buffs.
The attribute `other` column is the exact remainder after allocated base and job
table bonuses; it includes effects beyond items and is not labeled item-only.
Right- and left-hand physical modifiers are separate, and ammunition race/size
modifiers are shown independently. Bonuses targeting all races/sizes/elements
are added to their applicable specific row once.

The casting topic follows the configured `vcast_stat_scale` and the native
`sqrt((2*DEX+INT)/scale)` stat reduction. It shows script variable/fixed modifiers
separately. Fixed casting has no general DEX/INT reduction. Actual skill flags,
status effects and per-skill rules still determine the cast. The command does
not call a cast-time evaluation that could consume Memorize or another status.

Weapon ATK and refine ATK are separate native fields. Mastery damage depends on
weapon, skill and target, so the report does not invent one universal total.
Drop modifiers are components, not an absolute probability: each monster/item
has its own table, and server multipliers, caps, VIP and other applicable rules
matter. There is no universal MVP-card drop chance.

Damage tables do not apply a target's defenses, race/element interactions,
skill-specific exceptions, attack-flag-conditioned bonus vectors, damage caps or
map modifiers. They are useful for inspecting the player's current modifiers;
the damage lab remains the place to measure an actual rotation.

## Validation

`python3 tools/ci/battlestats_test.py` compiles the unchanged production report
function against explicit status/job-table/transport doubles with ASan and UBSan.
It checks all-target accumulation, left/right separation, negative reductions,
job/other breakdown, casting units/signs, bounded pagination, malformed input,
drop components and preservation of input state. This is a report-boundary test,
not a test of native damage formulas or client rendering.

The complete modified `atcommand.cpp` also passed a C++17 syntax check against
the repository's actual headers with `PACKETVER=20260219`. A fresh native server
build and reload are required for the new handler and aliases.
