# PN native player convenience commands

Implemented from the requested [MuhRO Player Commands reference](https://wiki.muhro.eu/Player_Commands), using this server's own inventory, navigation, equipment, and bonus APIs. This subset does not assert complete MuhRO feature parity. Command permissions and Settings NPC persistence are configured separately.

| Command | Behavior |
|---|---|
| `@navi prontera 25 30` | Requests client navigation to a loaded map and walkable coordinates. |
| `@navi2 prontera 25 30` | Same destination validation, also opens the navigation window. |
| `@unequipall` | Attempts to remove all currently worn inventory equipment, including costumes/shadow slots. Reports removed and restricted items. |
| `@clearfav` | Clears favorite marks on inventory items and immediately sends each affected client tab update. |
| `@autospells` | Lists currently calculated attack, when-hit, and skill-triggered bonus autospells. |
| `@showexp [on\|off]` | Toggles without an argument; explicit state is idempotent. |
| `@showzeny [on\|off]` | Same explicit/toggle behavior for zeny messages. |
| `@showdelay [on\|off]` | Same explicit/toggle behavior for delay failure messages. |
| `@noask [on\|off]` | Same explicit/toggle behavior for the existing invitation/deal rejection state. |

Explicit state accepts case-insensitive `on` and `off`; invalid or extra tokens do not change the state. Reapplying `on` at login therefore cannot accidentally turn a setting off. These native handlers change runtime state; the separate Settings NPC owns persistence.

Navigation sends directions only. It never warps, changes a quest, or grants access to a restricted map. Client navigation data must know the map and route; a custom map can pass server geometry checks while the client has no route graph for it. Destinations on another map server are currently rejected because this implementation validates locally loaded geometry.

Unequip-all uses the same nonforced `pc_unequipitem(..., 1)` operation as the native client's unequip-all handler. Action restrictions, death, and per-item prohibitions such as Pyroclastic remain effective. It does not remove equipment-switch registrations. A blocked item stays equipped; successful removals use normal status recalculation and acknowledgements.

Clear-favorites changes only the favorite flag of existing inventory items, leaving quantities, equipment, refinements and other attributes intact. The `0x0908` update uses client index `inventory index + 2`; its `favorite=true` packet field selects the **normal tab**, matching rAthena's inverted packet convention. No separate remembered-favorite storage registry exists in this implementation, so MuhRO's additional remembered-storage reset behavior is not claimed. Close other inventory interfaces first.

Autospell output reads the three current bonus vectors, including contributions already merged by status calculation. Rates are per eligible trigger, with 1,000 units representing 100%; it shows base chance rather than simulated damage or an unconditional chance per attack. The output notes arrow-attack and long-physical-hit rate halving. It also exposes trigger skill, battle mask, source item, and random-level flags. Arbitrary `autobonus` scripts, status-based skill autocasts, and conditions that are not currently contributing to these vectors are outside this list. Normal skill/range/map restrictions still govern actual casts.

## Validation

```sh
python3 tools/ci/player_commands_native_test.py
```

The test extracts the actual command handlers and compiles them under UndefinedBehaviorSanitizer with small engine doubles. It checks explicit state idempotence, no-argument toggling, invalid argument immutability, navigation window modes and malformed/boundary coordinates, unavailable actions, nonforced equipment removal, favorite packet indices/flags, repeated clearing, rejection of target-like arguments, and autospell rate formatting. Result: PASS on 2026-09-08.

This harness does not replace a full map-server build or graphical client acceptance. Confirm navigation overlays, item-tab updates, equipment restrictions, and Settings persistence in the running client after deployment. Source registration is in `src/map/atcommand.cpp`; no new cross-player permission is implied by these handlers.

## Persistent kill counter

The `@killcounter` / `@kc` script menu configures five permanent character slots. Counting itself runs once in native `mob_dead`, before the monster-specific NPC event branch. This includes monsters with instance/event labels, which the general `OnNPCKillEvent` script event would miss. The script's increment handler was removed to prevent counting ordinary monsters twice.

Credit goes to the existing `first_sd` loot-priority owner: damage from eligible controlled units is aggregated, and the configured first-attacker loot bonus participates in ranking. This is not necessarily the final hitter or the largest raw-damage dealer, and credit is not copied to party members. Only final deaths with a real source and credited player count; pending rebirth, scripted `killmonster` removal, and deaths without a source are excluded. No quest or monster event dispatch was changed.

`PNKCMob` and `PNKCKills` arrays use `pc_readregistry` / `pc_setregistry`, which maintain permanent registry persistence. Temporary `pc_readreg` / `pc_setreg` would be incorrect for these script variables. Counts stop advancing at the 32-bit display cap, with the check before addition to avoid overflow even if the registry contains an unexpectedly large value.

```sh
python3 tools/ci/player_killcounter_test.py
```

The extracted native helper passes tests for all five slots, labeled instance monsters, empty/nonmatching slots, attribution isolation, excluded death conditions, saturation, and a single call site. The test also verifies that the script no longer has an `OnNPCKillEvent` increment. SQL flush/relogin persistence and actual party combat attribution still need live acceptance; the harness verifies use of the permanent API and correct helper behavior.
