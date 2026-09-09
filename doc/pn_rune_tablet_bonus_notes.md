# Rune Tablet bonus implementation

The 57 published sets use stable client set and piece IDs. Numeric effects follow
the retrieved Rune Tablet System reference
(2026-09-08), with cumulative piece thresholds and integer enhancement divisions.
The catalog contains the matching source provenance and player-facing descriptions.
The Chaos set has no defined effect in either the reference or client description;
its activation does not invent a benefit. Eight newer Chapter 1 client sets absent
from the reference are excluded.

## State and recalculation

`PNRTActive` selects one set; `PNRTPaid[setId-1260000]` must be nonzero.
`PNRTLevel[setId-1260000]` is clamped to 0–15 when applying bonuses.
An unlocked `#PNRTPiece[pieceId-1263000]` counts once per applicable set.
Fewer than two pieces grants no effects. All membership reads use function-local
variables, so an equipment recalculation cannot overwrite an open service dialog.

Services call `PN_RT_Refresh` after changing persistent state. A single constant
`bonus_script` invokes `PN_RT_Bonus` on each native stat recalculation. Its flags
are 512 (protected from ordinary clears), 8 (discard on logout), and 1024 (renew
the existing identical script). It does not call `bonus_script_clear` or touch
unrelated bonuses. Login restores it, death/dispel retain it, and daily renewal
keeps continuously connected characters covered. The seven-day duration fits
the engine's 32-bit millisecond duration. Relog rebuilds from persisted state;
no SQL bonus rows are used for the hook.

## Native effects

The Burning Fang effect needs an equipment-independent proc. Stock `autobonus`
requires an equipped item position and is therefore unsuitable for a tablet.
`bPNRuneSPRegenProc` is a recalculated Boolean bonus. A damaging hit in the native
additional-effect path has a 2% chance to start four ticks of 200 SP, one second
apart. Further procs replace the pending sequence rather than stack it. Death or
switching away cancels it. Timer identity prevents a pending old-session callback
from affecting a character who has relogged. The short proc is not persisted.

Episode bonuses use new additive `RC2_PN_EP18` through `RC2_PN_EP21` groups.
The overlay enumerates exactly the existing database Aegis names with the matching
`EP18_`, `EP19_`, `EP20_`, or `EP21_` prefix (37, 40, 27, and 60 records). It changes
only group membership. Existing stats, drops, and other race groups remain intact.
This provides physical and magical modifiers without the ten-monster limit of
`bAddDamageClass`. A reused monster retains its original episode classification;
the modifier is not based on the map currently occupied. Additional episode mobs
must be classified in the overlay when introduced.

The reference's penalties are preserved, including Fusion/Predator elemental
vulnerability, Irritation's critical penalties, and partial-set SP cost increases.
HP/SP damage drains use native per-thousand probabilities (20 = 2%, 10 = 1%).
The 21st Anniversary effect uses the published HP recovery modifier for the two
named item IDs; it does not silently substitute SP recovery for Poring Kombucha.

## Validation

Run `python3 tools/ci/test_rune_tablet_bonuses.py` with `g++` installed. It checks
all 57 sets against catalog membership, all enhancement/piece-count combinations,
representative cumulative thresholds, adverse effects, drains, and exact episode
group coverage. It also compiles the actual native timer functions in a small
session/timer harness to test four-tick recovery, refresh replacement, loss of the
bonus, death, logout, and relog. A fresh map-server build and native script load
are required because the new bonuses and race-group constants are engine changes.
