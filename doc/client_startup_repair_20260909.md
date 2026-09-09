# Client startup repair

The episode asset update incorrectly added an eleventh DATA.INI archive, pushing
`data.grf` into slot 10 beyond the client loader's supported slots 0–9. Core job
and NPC identity scripts then became unavailable. The installed stack now combines
the 117 episode resource entries and 18 navigation entries in `client_repairs.grf`.
All 135 payloads are preserved; `data.grf` is restored to slot 9.

A second user-reported failure identified `SystemEN/QuestNavigationRepair.lua`,
line 280. The game environment did not provide `table.insert`. The repair now uses
direct array assignment. Native Lua reproduces the former nil-function failure
with that function disabled, and the corrected patch loads and repeats safely.
All 11,393 actual quest records pass the same restricted-library regression.

The preflight now rejects an eleventh archive, and the release gate tests the slot
limit, merged resource preservation, priority and malformed-archive rejection.
Actual base NPC identity and job-name scripts load with 5,477 `jobtbl` entries.
Item registrations, Chapter 2 enchant recipes and map collision checks still pass.

Original archives and replaced files remain backed up. Close and reopen the game
to load the corrected archive list and quest script. File and native Lua checks
do not substitute for confirmation from the running game.
