# Player experience update — October 7, 2026

## Next upgrade

Use `@adventure` → Equipment goals and materials, choose a recipe, and select
**Pin this recipe as my next upgrade**. `@goal` opens the full plan; the Adventure
dashboard shows up to four material totals and the Zeny requirement. Counts
refresh from inventory and accessible owned storage each time. Busy or failed
holdings queries show unavailable progress. Pinning saves an item and recipe
identity on the character, not cached counts, prices, or eligibility.

The planner searches both loaded barter crafting exchanges and loaded item
reforms. Reform plans show the base equipment, materials, refine range, required
random options, refine change, and card/enchant/grade/option changes. Catalyst
access and consumption still belong to the existing reform interface. Item
totals alone do not establish equipment suitability or access to an exchange.
Legacy scripted crafting outside these databases retains its own recipe menus.

Source lookup connects the existing Alice and Bioresearch material credit tracks
to their real entrances and exchanges. Their original eligibility and daily
limits remain authoritative. Other scripted reward sources are not inferred.

## Prepare for an adventure

Use `@prepare`, choose Farming/Boss/Support, then **Prepare for an adventure**.
The flow checks repairs, previews the exact saved equipment and missing retail
supplies, shows the wallet and cost, and lets you choose an existing entrance or
the Weekly Expedition. Confirmation rechecks the equipment and the exact supply
signature after the menus. Canceling changes no equipment or money.

Equipment applies through the existing native eligibility/restore path. Supplies
use the existing durable purchase transaction. They are separate operations:
if purchase submission fails after equipment applies, the dialog reports that
partial result. An accepted purchase waits for its existing server acknowledgement
before delivery. Navigation neither admits a player nor changes a cooldown.

Native `pnpreppreset(slot, 2)` previews eligibility and publishes temporary
`@PNPrepGearCount` / `@PNPrepGearName$` rows without equipping. Existing actions
0 (apply) and 1 (save) keep their behavior.

## Party requests

Use `@partyboard` or Play with friends. Browse a listing, choose **Request to
join**, select a role, and confirm sharing your character name, job and level.
The listing's leader receives a notification and can choose **Review join
requests**, approve an ordinary invitation, or decline. Applicants still accept
through the normal client invitation. Existing party membership, permissions,
invite preferences, capacity, instance and map locks are checked by native
`party_invite`; `party_addmember` is never used by this feature.

Requests expire after five minutes, logout/character change, joining another
party, listing refresh/removal/expiry, or leadership changes. One character owns
at most one pending request; attempts are throttled to one per 30 seconds during
the session. The queue has 80 slots and is temporary across script reload or
map-server restart. Approval rechecks the applicant and listing revision after
the menu. Failed invitations retain their request until it expires. Readiness
uses the existing instance predicates and party snapshot.

## Weekly Expedition pilot

Use `@challenge` from an unrestricted town at Base Level 100 or above, then talk
to the Expedition Console. This private character instance uses `guild_vs1`;
it cannot replace another personal instance. Choose Casual or Challenge and
clear five waves within ten minutes. Difficulty changes guardian HP and attack.

The Monday 00:00 UTC / 07:00 Bangkok rotation has three modifiers:

1. Relentless: six enemies per wave instead of four.
2. Element Shift: water, earth, fire, wind, then neutral defenders.
3. Danger Zones: every 15 seconds, WEST (`x < 50`) or EAST (`x >= 50`) is announced
   as unsafe after four seconds. Move into the other half. Three mistakes fail.

Death, leaving, disconnection and the time limit fail a running attempt. Return
to the console for free retries. Exact spawned GIDs and owner kill callbacks
advance waves; repeated or unrelated death events cannot advance progression.
After clearing, record completion at the console. Each difficulty awards one
cosmetic stamp per week and retains that week's best completion time. Repeated
clears can improve the best time without granting another stamp. Lifetime stamps
persist. An instance completed in an old week cannot claim a new week's record.

The pilot grants no equipment, items, Zeny, experience or boss credits. Instance
map flags suppress ordinary and MVP drops and death penalties. Damage Lab and
Expedition consoles check their own instance identity before running or applying
map flags. Entry needs no new client assets or SQL schema.

## Verification and rollout

Focused native VM tests cover live pinned holdings and reform requirements,
equipment previews/cancellation, party request ownership/expiry/revision and
ordinary invitation dispatch, and real expedition event state transitions,
rotation, duplicate kills, hazard warnings, timeout and cosmetic replay guards.
World, timer delivery, transport and persistence adapters are explicit in those
fixtures; they do not establish rendered client appearance or combat difficulty.

The source gate includes existing database, route, geometry and compatibility
checks. `weekly_expedition_test.py` is part of the full release gate. Validate
NPC startup against a disposable database before rollout. A release needs the
matching map binary, changed NPC sources and instance definition. Existing
characters and paid transactions require the normal quiescent deployment.

Before wider release, play both difficulties, inspect the console placement,
check the danger-zone text in the current client, and test actual invitation
acceptance and restock acknowledgement. Enemy tuning is a pilot starting point.

Validation on the isolated October 7 candidate passed the map-server build,
all 60 source checks and clean NPC startup against a disposable database.
Focused native results: planner 31 cases / 296 assertions; preparation 42 / 441;
social tools 44 / 1,353; expedition 9 cases / 51 checks. Existing readiness
(270 / 2,372), Damage Lab (19 / 551) and weekly reward regressions also passed.
The final committed payload subsequently passed all 104 full native release
checks and was deployed to production on October 7, 2026. All seven services
passed health verification and gameplay ingress is open. Rendered client
playtesting remains pending for the pilot. See the
[production deployment record](player_experience_deployment_20261007.md) and
[receipt](evidence/player_experience_deployment_20261007.json). Evidence and
packaging tools are in `OPS/player-experience-20261007` in the parent workspace.
