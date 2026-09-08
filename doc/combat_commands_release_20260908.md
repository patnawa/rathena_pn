# Combat systems audit and player commands ? 8 September 2026

## Result and scope

The Renewal element table is correct against the reviewed upstream table. Native
refining and grading had confirmed validation/probability defects; the fixes
preserve the configured economy. Player-command additions use PN engine APIs and
existing services. Remaining MuhRO-specific systems are listed in the
[player-command coverage report](player_commands_reference_audit_20260908.md).

## Effective configuration

All nine root, Renewal and import YAML files for attributes, refinement and grading
were compared with the live host after normalizing CRLF to LF. All matched the
checkout. The import files have no active override body. Live configuration has
`attribute_recover: no`, `attack_attr_none: 14`, and `feature.refineui: on`.

| System | Audit evidence | Production change |
| --- | --- | --- |
| Elements | 400 cells; 2,800 native-helper damage cases; all 3,208 effective monster element assignments valid | None required |
| Refinement | 160 levels across nine categories; 110,000 exhaustive random outcomes plus transaction cases | Strict success comparison; inventory-only targets and economic-interface guards |
| Grading | Eight stages and sixteen material options; native selection/commit tests under ASan/UBSan | Commit eligibility, bounds and wide material/chance arithmetic; inventory-only targets |

Native grading retains None?D?C?B?A progression, refine reset on success, and the
selected option's break/downgrade/no-change outcome. Unequip targets and remove
switch registration first. The Office Training floor now has Sratos' existing
Grade Enhancer and Etel exchange. No database rates, materials or prices changed.

## Player settings and tracking

`@settings` and the Office Settings desk save five preferences: autoloot threshold,
EXP messages, zeny messages, skill-delay messages and invitation/deal rejection.
Values can be saved for the character or game account. Explicit character Off
still overrides account On; clearing that override reveals the account value.
Changes apply immediately through idempotent commands. Other characters receive
account preferences on their next login. Manual native toggles are session-only.
No linked master-account model is invented. Clearing both scopes explicitly
restores Off for the changed setting; never-configured login state is left alone.

`@kc <monster ID> <1-5>` replaces a saved character tracking slot and starts its
count at zero. `@kc status` displays all slots; `@kc reset [slot]` clears one or all.
The native death hook counts the existing loot-credit owner once, including mobs
with custom instance/event labels. It excludes script removals, uncredited deaths
and pending rebirth. This is not a party-share counter or necessarily the player
who landed the final hit. Counters saturate at 2,147,483,647.

The layout generator now verifies 52 desk approaches with NPC cells treated as
blocked, preserving connected paths from all three Office entrances. No new
client archive or geometry change is required for these two additional desks.

Account loot profiles use `@alc save 1 Farming`, `@alc list`, `@als 1` and
`@alc delete 1`. Ten versioned account-registry snapshots hold the current rate,
item inclusion list and item-type mask. Loading validates the complete profile
before mutating session preferences. See [profile semantics and limits](account_loot_profiles_20260908.md).

## Verification boundaries

Focused tests execute extracted native handlers or real script VM bodies with
explicit world, SQL and transport doubles. They prove those boundaries, not every
combat skill or rendered UI. Full client navigation overlays, grading/refine
window interactions and real relog persistence still require player acceptance.
The element audit separately records the existing left-hand/card-debuff TODO.

The real Settings/Kill Counter script VM passed 36 scenarios and 499 assertions
under ASan/UBSan, including save/inherit/clear/cancel/login and numeric boundaries.
Native command and loot-profile tests passed their handler boundary checks.

Build, isolated startup and live deployment receipts follow. The Docker workflow compiles all four PN packet-version
20260219 binaries. A separate workflow runs the focused combat and command tests.

## Build and startup acceptance

The final Docker incremental build compiled all three changed map translation
units (`clif.cpp`, `atcommand.cpp`, `mob.cpp`) and linked successfully against the
previous fresh four-server build with `PACKETVER=20260219`. The candidate's
`map-server --run-once` exited 0 with zero errors and resolved every one of the
52 Office desk identities. All source/database/NPC loaders completed.

The first isolated MariaDB 10.11 fixture lacked the TLS support required by the
container client library; it was replaced by the already-installed MariaDB noble
image used for the compatible fixture. The startup's remaining warnings were the
known isolated root/default inter-server settings and empty roulette SQL table.
No test database settings were copied to live.

The candidate also caught a live permission merge collision: the existing
`alootid` alias and new `autolootitem` canonical key resolved to the same handler.
The merge now resolves aliases before adding grants, preserving the existing
behavior without duplicate group entries. A canonical duplicate check was added
to the permission regression test. Final group 0 has 49 reviewed native commands,
with no additional GM or other-character permissions.

## Live deployment

Deployed at `2026-09-08T13:09:18Z` after the database reported zero online players.
Ten runtime/source files and the new map binary were installed. Login and
character services remained running; only map-server restarted. The final map
log has zero errors, confirms character-server authentication, receives maps and
clans, and reports the map server online. No acceptance fixture is imported live.

Rollback backup:
`/app/rathena-deploy-backups/pn-combat-commands-20260908T130917Z`.
It contains the original files and binary, new-file list, source manifest,
receipt, deployment helper and final build/isolated/live startup logs. Binary SHA-256:
`82e1ec46741a372f4dc05341483d50b096ac417b8c4252f8e85571dec824492ea`.

The temporary isolated MariaDB container was removed after verification. No
production SQL schema or character/account registry rows were changed by the
deployment. Settings and profiles create their own persistent rows when used.
For rollback, stop map-server in a zero-player window, restore the backed-up
files/binary, remove only entries in the backup's new-file list, then restart
map-server and verify its character-server connection. The additive preference
registries can remain without changing prior server behavior.
