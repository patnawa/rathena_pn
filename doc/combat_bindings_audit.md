# Combat binding audit - 2026-09-06

## Corrected in this pass

- Chapter 2 cards 300768-300771 (Doodlehand, Fingerhand, Tinkerhand, Swifthand)
  now read their own compounded weapon's level with `getequipweaponlv()`.
  Previously all four queried `EQI_HAND_R`, so a card in a level 5 left-hand
  weapon received the level 4 right-hand result, and vice versa. Their 10/20%
  bonuses and all item restrictions are unchanged. The active client descriptions
  in `SystemEN/itemInfo_Chapter2.lua` specify the additional bonus on a level 5
  weapon, not on an unrelated right-hand weapon.
- Odium of Thanatos (20796) can now attempt its existing `NPC_POWERUP` spawn
  skill. The row's `attack` state was unreachable at the only `onspawn` dispatch,
  which occurs in `idle`. Both the active overlay and the maintained Thanatos
  fragment now use `idle`; level, chance, timing and targeting are unchanged.
  It is this monster's only spawn-event row, so no higher-priority spawn skill
  consumes the event first.

## Evidence and regression coverage

The engine's `status_calc_pc` sets `current_equip_item_index` to the card's host
before running each card script, including off-hand weapons.
`BUILDIN_FUNC(getequipweaponlv)` uses that index when called without arguments.
Weapon cards remain compoundable only in matching equipment: `pc_insert_card`
checks the equipment/card location intersection; these four cards have only
`Right_Hand: true`, whereas shield armor uses `Left_Hand`. A dual-wield-capable
weapon's database location is still right hand even when worn in the left hand.

Both `mob_spawn` and `mob_revive` call `mob_setstate(*md, MSS_IDLE)` before
`mobskill_use(..., MSC_SPAWN)`. The latter filters the row's state before checking
its event condition and returns after one successful skill.

Run from the repository root on Linux/WSL:

```sh
python3 tools/ci/audit_combat_bindings_test.py
```

The test requires PyYAML and g++. Six tests check the effective item imports,
effective legacy skill overlay (including clear rows), source/active consistency,
and spawn initialization order. A small C++ probe compiles the actual
`getequipweaponlv` builtin body and `mobskill_use` state-filter block extracted
from the current source. Its 13 assertions use AddressSanitizer and
UndefinedBehaviorSanitizer and cover mixed left/right levels, explicit versus
host-bound lookup, missing equipment, non-weapon hosts, and spawn-state filtering.
Before the data fixes, this test produced five expected failures: the four cards
and Odium's state. After the fixes, all six tests pass.

These are focused source/data binding checks with mocked surrounding engine
services. They do not execute the whole script interpreter, attached-player
equipment calculation, actual monster spawning, or encounter playthroughs.

## Unresolved findings deliberately not changed

- Simulation Juncea (21533) also has an unreachable `attack`/`onspawn` Relieve
  level 9 row. However, its effective monster record already has `DamageTaken:
  10`. `battle.cpp` applies Relieve reduction and then the database multiplier;
  enabling both would reduce incoming damage to 1%, not the existing 10%.
  Resolving the legacy duplicate requires confirming the intended encounter
  reduction; activating the row alone is unsafe.
- Broken Thanatos (20785) has an unreachable spawn Earthquake after its spawn
  summon row. Merely changing the state cannot run both skills during the single
  event; changing it to `afterskill` would also affect later resummons. The
  existing below-80%-HP Earthquake rows remain enabled. No new trigger was guessed.
- Several Episode 19 rows label skill 756 as `NPC_MOVE_COORDINATE`, while the
  engine's numeric ID 756 is `NPC_WIDEBLEEDING2` (move coordinate is 755).
  The text label is informational only; a source label/ID mismatch alone does
  not establish which combat effect was intended, so no IDs were replaced.
- Blaze Mana (Magic Resist), item 314982, uses `bRes`, not `bMRes`; the supplied
  custom client description likewise says RES. Its name/resource suggests a
  possible upstream-data error, but there is no authoritative effect correction
  in this pass, so its gameplay bonus is unchanged.

Read-only checks across the active custom skill overlay found no missing
positive summon/transformation monster IDs, missing nonzero afterskill/skillused
skill IDs, or inverted HP-range conditions. These checks do not prove the
encounters are complete or balanced.
