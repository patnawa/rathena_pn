# Group 128 Shadow enchant restoration

Prepared 2026-09-06 as a separate data change. The initial staged pass added:

- `db/import/shadow_group128_enchant.yml`
- `tools/ci/shadow_group128_enchant_test.py`
- this document

The main review subsequently wired one Renewal import and added the clearly
labeled Shadow Gear Enchanter at `grademk,40,184` in the already-enabled
`grademk_services.txt`. The NPC opens native group 128 after confirmation, warns
about retention/downgrade outcomes and disabled reset, and does not independently
charge or grant anything. All costs remain in the native database. The overlay
is ignored by `/db/import/*` and must be explicitly included in Git.

Three structural service tests verify unique import/group, confirmation/cancel
ordering, no script-side charges, enabled NPC file and collision-free coordinates.
The integrity audit validates the approach at `grademk,40,181` and connectivity
from the workshop arrival; it also now covers the existing Sratos approach.
The initial proposed position 34,184 was rejected by the collision test because
Sratos already occupies it; that position was never deployed. Native-loader,
deployment and client-interaction evidence are separate from these source tests.
The native loader and backed-up deployment passed at 15:18 ICT; see
[deployment receipt](allocator_shadow_deployment_20260906.md). Actual client
selection, material charging and relog persistence remain unverified.

## Exact scope and source

Restores only group 128, which is commented out at
`db/re/item_enchant.yml:29673` with an old unsupported-fields note. All 14 target
items, the material, six basic-stat enchants, and 30 Shadow upgrade levels
already exist in the effective server item imports. No placeholder item is
needed.

The source of recipe behavior is the user-supplied active client, not a claim of
official Gravity balance or acquisition policy:

| Input | SHA-256 |
| --- | --- |
| `../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub` | `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d` |
| `../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub` | `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496` |

Group 128 is declared at EnchantList lines 10221-10324. The original compiled
client name table resolves the six localized stat labels by numeric ID. The
unchanged, fail-closed `ClientItemNames` resolver is used by the regression; no
supplementary alias map or fallback is introduced.

The target families are Full Power, Full Rate, and Full Spell armor/shoes, plus
Focusing, Stout, Centering, and Witty pendant/earring pairs: 14 targets total.
Slot order is 3 then 2 (the native zero-based card-slot indices). Minimum refine
and enchant grade are zero; random options are permitted. Existing item equip
requirements remain unchanged. Reset is disabled with `Chance: 0` and no material
charge.

There are 12 selectable initial recipes, not random initial distributions:

| Slot | Outcomes | Essence per selection | Zeny |
| --- | --- | ---: | ---: |
| 3 | `Strength3`, `Dexterity3`, `Inteligence3`, `Agility3`, `Vitality3`, `Luck3` | 5 | 0 |
| 2 | `Shadow_ATK_1`, `Shadow_MATK_1`, `Shadow_CRI_1`, `Shadow_CAST_1`, `Shadow_HP_1`, `Shadow_SP_1` | 7 | 0 |

The sole material is the existing `S_Enchant_Essence`, ID 1001253. Each of the
six Shadow categories has four ordinary random upgrades, for 24 recipes total:

| Source level | Essence | First result / weight | Second result / weight | Zeny |
| --- | ---: | --- | --- | ---: |
| 1 | 2 | level 1 / 30000 | level 2 / 70000 | 0 |
| 2 | 3 | level 1 / 40000 | level 3 / 60000 | 0 |
| 3 | 5 | level 2 / 50000 | level 4 / 50000 | 0 |
| 4 | 8 | level 3 / 60000 | level 5 / 40000 | 0 |

Retention and downgrade results are preserved exactly. Level 5 has no outgoing
recipe. There are no guaranteed upgrades and no additional initial stat levels.

## Why the existing engine can represent this data

- `ItemEnchantDatabase::parseBodyNode` in `src/map/itemdb.cpp` accepts the
  `TargetItems`, eligibility, `Order`, `PerfectEnchants`, and `Upgrades` fields.
  New records require target items, order, and slots; this overlay provides all
  three. Slots 3 and 2 are valid native indices and do not overlap the targets'
  native card slots.
- Its `RandomUpgrades` parser rejects unknown or duplicate outcomes, zero
  weights, totals other than 100000, and simultaneous deterministic `Upgrade`.
  Each of the 24 declarations satisfies these restrictions. The material
  quantities fit the native 16-bit and maximum-stack constraints without capping.
- `select_enchant_upgrade` and `select_enchant_upgrade_result` in
  `src/map/enchant_upgrade.hpp` implement separate ordinary/guaranteed request
  selection and ordered weighted outcomes, including the source itself or a
  lower-level enchant. The actual header functions were compiled and tested,
  not rewritten in Python as a probability simulation.
- `clif_enchantwindow_upgrade` in `src/map/clif.cpp` calls these selectors,
  resolves the existing source enchant, checks costs, selects a roll from
  1 through 100000, then applies the selected item. It has no rule requiring the
  result to have a higher enchant level. The perfect initial handler follows
  the configured first empty slot and exact requested initial enchant.
- The native reset parser allows chance zero, and reset requests reject a
  disabled reset. No engine changes are needed for these declarations.

These are source inspection and compiled-selector evidence. This pass did not
run the entire native YAML loader, network handlers, or inventory transaction
against a live map server, and did not test a Windows client window.

## Standalone regression results

The test suppresses only this overlay when reading the baseline, then appends
its body through the existing initial and upgrade overlay readers. It never
edits `db/item_enchant.yml`. It fails on any independently existing group 128,
so an overlapping implementation cannot silently be overwritten.

Nine tests passed with all optional checks enabled and no skips:

- Exact schema, 14 targets, 12 initial recipes, 24 random upgrades, all costs,
  outcome weights, slot order, eligibility, and disabled reset.
- All 51 item dependencies exist at distinct IDs; target ShadowGear slots do
  not overlap enchant slots and all enchant dependencies are Enchant cards.
- Complete initial-group equality and canonical upgrade equality against the
  pinned active client; no group-128 dependency is unresolved.
- Every other initial group, ordinary upgrade, and guaranteed upgrade is
  unchanged after the standalone overlay. Reapplying is semantically idempotent
  in the model; native repeated enabled targets would still warn, so import once.
- ASAN/UBSAN execution of the real native selector across every one of 100000
  draws for all 24 distributions: 2400000 draws, exact frequencies, invalid-roll
  rejection, and ordinary/guaranteed request-mode isolation.

Reproduce from the repository root in a Linux/WSL environment with Python 3,
PyYAML, and g++:

```sh
python3 tools/ci/shadow_group128_enchant_test.py \
  --client '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub' \
  --native -v
```

Without the client paths or `--native`, those respective checks are explicitly
skipped. Repository-only tests are not a replacement for the full command.

## Read-only NPC and acquisition trace

No direct group-128 `item_enchant` call was found in repository NPCs or usable
item scripts. The variable group selections in `grademk_services.txt` select
Varmundt 16-19/52-55, Constellation 7-13, and seasonal 117-124/142, not 128.
The episode-20 variable selection uses 88 plus its menu choice, not group 128.
Thus this pass found no intended existing service entry point to activate merely
by loading the group. It did not add an NPC or choose a new service location.

Existing acquisition declarations were inspected without changing them:

- `S_Enchant_Essence_Box_3` (102485) calls
  `getgroupitem(IG_S_ENCHANT_ESSENCE_BOX_3)` from
  `db/re/item_db_usable.yml:77540`. That item group grants three of the existing
  essence item. Several package/lucky groups also declare essence rewards.
- `MAIN_LUCKY_BOX_` declares the six Full Spell, Centering, and Witty targets in
  its subgroup 6. `Main_Lucky_Box_` (102701) calls that reward group from
  `db/re/item_db_usable.yml:78086`.
- No direct NPC name/numeric-ID references for the 14 target items or essence
  were found. The item-group scan found no direct target reward entries for
  the other eight targets (Full Power, Full Rate, Focusing, Stout).

An item-group declaration and a box script are not proof of player access to
the box. Box supply, any indirect acquisition system, and live deployment state
were not established. No drop rates, purchases, material pricing, or reward
economy were inferred. The subsequent custom workshop entry point enables users
who already have the equipment/materials to access group 128 without inventing
new purchases or drops. Native-loader and actual client-window/material charging
tests are still required before claiming end-to-end gameplay verification.
