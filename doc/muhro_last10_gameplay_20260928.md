# MuhRO latest-ten gameplay audit and implementation

Audited 28 September 2026 against PN baseline `1f4fd4cab`. Releases are selected
by publication date, not the discussion category's reply/activity order. MuhRO
published two different releases numbered 242; both count. Latest is 247, oldest
in this ten-release set is 239. This report covers server gameplay only. Wallet,
client, patcher and website work is tracked by the other workstreams.

This is an independent implementation of published behavior. Missing original
MuhRO source is not a reason to reject a feature. Where public notes omit exact
data, this report distinguishes an implementation decision from a verified
matching value. The current patch does **not** represent complete ten-release
feature parity.

## Delivered gameplay changes

| Change | Local result and verification |
|---|---|
| Imperial Guard, September 2 | Radiant Spear uses 7 POW with Spear Scar, Imperial Cross 8; unbuffed POW remains 5. Cannon Spear's scar addition is 200 per skill level; Banishing Point's is 180 per level. Base-level multiplication is preserved. |
| Genesis Ray, August 17 / September 2 | A GTB-wearing caster can initiate the self-centered player skill. Enemy GTB and recursive self hits retain immunity; NPC Genesis and Deadly Projection behavior are unchanged. |
| Airship Crash, September 18 | Five separate instance-scoped rescue bits; each passenger can be rescued once in any order. Party leader may enter before completing rescues. All five select paid boss 20891; fewer select 21062. Free specimen and seven-stone paid admission remain available. |
| Airship creature balance | Published weak/strong HP are 868,754,928 / 1,578,754,928. Existing other custom monster stats and drop restrictions remain. Two extra NPC locations, (270,303) and (150,250), are independently selected walkable locations; they are not claimed as MuhRO coordinates. |
| Sarah and Fenrir, August 13 | Story Guide at `1@glast 370,304` lets the leader choose Fast Mode before the first scene. Sixty-one dialogue pauses total 6.1 seconds instead of 168.5. This is an independently defined fast-story mode; combat timers, movement, monster spawns, objectives, rewards and cooldowns are preserved. Selection locks at scene start and rechecks leadership/start state after the menu. |
| Headgear, August 13 | Fancy Flower 2207 and Sunflower 2253 become refinable. Pirate Bandana 2287 was already refinable. |
| Market services, September 11/18 | PN service map `itemmall` (@go mall / @go 60) now has unlimited carrying capacity, storage and tool dealer. Combat Rudus (@go 50) is unaffected. Enter/exit and dynamic flag changes recalculate capacity including existing Gym bonuses. Percentage multiplication is widened and saturated to avoid heavy characters appearing light. |
| Master Refiner, August 13 | Existing weapon/armor +15 certificates 6872/6878 now work through the Master Refiner. Equipped item identity, unique ID, refine level, refinability and certificate count are rechecked after confirmation. Local independent policy supports refinable weapon level 5 / armor level 2; costume/shadow slots excluded. |
| Infinite Catalyst Box 3, September 11/25 | Local box 50150 grants one native 30-day rental: Soul Talisman 50151, Trap 50152 or Special Alloy Trap 50153. Corresponding material costs 1000563/1065/7940 are waived only while a nonexpired rental is carried. All other costs remain. Trap deployment snapshots free-material status and all four recovery paths suppress material refunds. No invented price or acquisition source. |
| Awakened Yordos, August 13 | Investigator 22414 now drops `Yordos_Inve_Card`; Judge 22415 drops `Yordos_Judge_Card` instead of the unrelated bailiff card. Existing local card rate 1 and steal protection are used; MuhRO's exact rate was not stated. |

The [September 2 notes](https://dis.muhro.eu/t/patch-notes-242-02-sptember-2026/5253)
specify the four Imperial Guard corrections. The
[August 13 notes](https://dis.muhro.eu/t/patch-notes-241-13-august-2026/5106)
specify refinability, cards and Fast Mode. The
[September 18 notes](https://dis.muhro.eu/t/patch-notes-244-18-september-2026/5331)
specify five rescues; the [Airship walkthrough](https://wiki.muhro.eu/Airship_Crash)
identifies the weak/strong creature IDs and HP. Other walkthrough settings such
as cooldown, exchange prices and research EXP are outside this patch's changes.

## Release-by-release disposition

| Release | Gameplay assessment |
|---|---|
| [247, September 26](https://dis.muhro.eu/t/patch-notes-247-26-september-2026/5391) | Server sale parser and transaction now reject partial/oversized/duplicate/zero-amount batches before deletion; actual 191/200-entry batches pass. No server-side 190-entry limit was found. This does not establish the protected native client disconnect fix; client serialization still requires verification. |
| [246, September 25](https://dis.muhro.eu/t/patch-notes-246-25-september-2026/5384) | Expanded stacks require coordinated client/server/data changes. Three 30-day rental catalyst choices delivered. BG Practice and Happy Hour display still need new local services. Bank/NPC economy revisions are assigned to the wallet workstream. BG gear exclusion lists and practice maps/rules require a defined local catalog. |
| [245, September 19](https://dis.muhro.eu/t/patch-notes-245-19-september-2026/5350) | No local reproduction established for dead-on-login, disappearing Gym bonus or lost quest records. Those notes do not identify faulty fields or affected characters. Do not manufacture data repairs. Vending import and character transfer are other workstreams. |
| [244, September 18](https://dis.muhro.eu/t/patch-notes-244-18-september-2026/5331) | Airship correction implemented. Kunai Splash has no demonstrated local double-ammo deduction: `battle_consume_ammo` clears `arrow_atk` after the inner invocation, preventing outer consumption. Tool vendor and storage added to PN itemmall (@go mall / @go 60), preserving Rudus @go 50 combat routing. |
| [243, September 11](https://dis.muhro.eu/t/patch-notes-243-11-september-2026/5296) | New competitive/event systems remain to implement: seven BG modes with balancing, rental builds, rankings/reporting, MVP betting/stash, seasonal/event revisions, scheduled cash sales and BG notifications. These need explicit local equipment catalogs, persistent state and website integration. Public notes do not provide all Moon Goddess values, rental presets, exclusions, event schemas or costume assets. Shop/NPC moves require matching local routes. Unlimited market capacity delivered on PN itemmall (@go 60), with safe exit recalculation; rentals delivered using native expiry. |
| [242, September 2](https://dis.muhro.eu/t/patch-notes-242-02-sptember-2026/5253) | Four skill corrections delivered; Psychic Stream already has `1750 + 3850*level + 12*SPL`, giving 21000 before SPL at level five. Local level/trait work already targets 285/65 and 120 (`kro_285_progression_test.py`, player configuration). Seasonal reward pools, donation coupon issuance, new stone box contents and custom recolors still require local item/service integrations. |
| [242, August 17](https://dis.muhro.eu/t/patch-notes-242-17-august-2026/5154) | Player Genesis correction delivered using September's clarified scope. Reported periodic lag lacks a local reproduction or published root cause; no speculative scheduler change. Other notes are client/patcher work. |
| [241, August 13](https://dis.muhro.eu/t/patch-notes-241-13-august-2026/5106) | Fast story mode, two refinement flags and two awakened card corrections delivered. Cat Ears Cape already uses costume-low; Drooping Elven Ears already uses costume-mid. `@bs` already reports ASPD. Encroached Sword restriction is absent, but patch notes omit allowed jobs; linked item detail did not resolve during research, so no guessed restriction. Master Refiner +15 certificate service delivered. Other names/costumes and event consumables require further item/catalog work. |
| [240, August 2](https://dis.muhro.eu/t/patch-notes-240-02-august-2026/5063) | Published event reward changes need independent local event implementations. Existing `cluckers.txt` is a GM-configurable random item game, not the coin-stake event described. Several amounts are specified and can be implemented once those services exist; other placements/pools lack exact values. Expanded Warp Portal memo slots and their missionary quest are now delivered: native variable-length warp selection, three base/six rewarded slots, map-specific hunts, six rescues, both mission orders, and reset semantics. |
| [239, July 24](https://dis.muhro.eu/t/patch-notes-239-24-july-2026/5015) | Undead Outbreak movement cannot be ported as a code fix because no equivalent local event exists; it can be built independently. A two-floor market needs two suitable maps, shop placement and route design. PN currently provides the separate service mall. |

## Files and tests

Core commit: `26581bf37`. NPC/data commit: `b4ad037a2`. Market/refiner commit: `291c143b2`. Catalyst commit: `16e1a234b`.

Changed production files:

- `src/map/skill.cpp`
- `src/map/skills/swordman/{radiantspear,imperialcross,cannonspear,banishingpoint}.cpp`
- `npc/custom/instances/AirshipCrash.txt`
- `npc/re/instances/SarahAndFenrir.txt`
- `db/re/item_db_equip.yml`
- `db/import/airship_crash_mob_db.yml`
- `db/import/mob_db.yml`

Executed locally under WSL g++, AddressSanitizer and UndefinedBehaviorSanitizer:

```text
python3 tools/ci/muhro_skill_balance_test.py
MUHRO_SKILL_BALANCE checks=48 failures=0
python3 tools/ci/muhro_airship_rescue_test.py
MUHRO_AIRSHIP_RESCUE checks=2553 failures=0
MUHRO_AIRSHIP_GEOMETRY five unique walkable survivor locations
python3 tools/ci/muhro_item_rules_test.py
MUHRO_ITEM_RULES three refinable headgears and two costume positions correct
MUHRO_MOB_RULES awakened Yordos cards and rescue-selected boss HP correct
python3 tools/ci/muhro_sarah_fast_mode_test.py
MUHRO_SARAH_FAST_MODE 61 story pauses: 168500ms -> 6100ms; encounter operations preserved
```

The first test failed 17 assertions before the fixes, then passed all 48.
It compiles actual production formula bodies and the actual GTB dispatch
predicate against explicit status lookup doubles. The Airship test translates
only script registers/builtins into C++ and exercises the unchanged branch and
bit-operation bodies for all 120 rescue orders, repeat clicks, registration and
all partial masks. It is **not** the native rAthena script VM. Geometry checks
validate GAT walkability, not rendered client navigation or path reachability.
Sarah's test reverses only the documented pacing additions and proves the
remaining encounter script exactly matches the baseline.

Root reports the isolated Docker native build/startup passed for the phase-one gameplay scripts/data. Phase-two/three integration and native startup are coordinated by root and the core build agent. Rendered in-game movement/cast acceptance and deployment remain separate checks. No production files or live characters were changed by this workstream.


## Additional feature verification and integration notes

- `market_weight_test.py`: 10 actual production weight and transition checks under ASan/UBSan; three service coordinates are walkable. Includes leaving the market with excessive weight, Gyms, and dynamic flag removal.
- `refine15_certificate_test.py`: eight actual translated transaction guard cases cover successful refinement, cancel, missing certificate, changed equipment ID/unique ID/refine, removed equipment and disabled refinement.
- `infinite_catalyst_test.py`: 152 actual C++ helper/cost/recovery predicate checks plus six translated box transaction cases under ASan/UBSan. Expired, exact-boundary, permanent, zero-amount and wrong token cases do not waive costs. Unrelated material/SP/AP are unchanged. Missing box, duplicate token, capacity failure and grant failure preserve the box.

Catalyst IDs 50150-50153 are independently allocated local IDs. Rental `Etc` items are supported by this checkout's `rentitem` and `pc_additem` implementations: expiry forces a separate inventory slot even for stackable item types, and inventory packets carry `HireExpireDate`. The older script-command prose claiming stackable items cannot be rented does not match those implementations. Duplicate tokens are refused before `checkweight`, guaranteeing a fresh slot is checked; the native grant remains the final authority. Client display definitions must be included with the release. The selected duration is 2,592,000 seconds and runs offline. Existing administrator/reward systems can grant box 50150; MuhRO acquisition and pricing parity remains unspecified and no public shop was invented.

Remaining substantial server feature priorities are independent event/BG systems, their published reward changes, persistent rankings/betting, and defined equipment/costume catalogs. Absence of MuhRO source is not itself a blocker to implementing those systems; exact unpublished item pools, balance presets and assets remain separate specification gaps. This branch does not claim those systems are delivered.


## Bulk sales and expanded memo follow-up

- `f0a2a8ef0`: malformed and repeated-index NPC sales fail during preflight. `npc_bulk_sell_test.py` executes the actual parser and transaction with inventory doubles: 2,024 assertions passed, including valid 191/200-entry batches and unchanged inventory on invalid batches. Native-client serializer behavior is a separate unresolved check.
- `e506ebb77`: six-slot Warp Portal support and missionary quest. Capacity/recording/offering/final selection/reset test passed 94 assertions; translated real mission/rescue transaction branches passed 252 assertions across both region orders and all rescue orders. Nine NPC positions are walkable. Modern `0x0abe` already supports variable-length destination lists. The legacy four-entry serializer off-by-one is corrected.

The independent quest uses the eligibility, two 100-monster objectives, six rescue locations and reward progression documented by [Extended Memo Slots](https://wiki.muhro.eu/Extended_Memo_Slots) and [Missionary Work](https://wiki.muhro.eu/The_Path_of_Missionary_Work). Cardinal/Inquisitor characters with Warp Portal 4 receive a fourth memo slot on acceptance and six total after both missions. An actual skill reset clears progress and the extra memo records; a points-only query does not. Quest log descriptions reuse the existing client quest loader. Existing SQL memo rows already support the extra records, but the changed common character structure requires rebuilding and restarting every inter-server binary together. Native startup and real-client acceptance remain integration checks.

Client release fragments are isolated under `client-patch/infinite_catalysts` and `client-patch/extended_memo`. Lua 5.1 loaded the complete existing item database plus all four catalyst definitions, and the complete existing quest database plus all fifteen missionary definitions. Installed client files were not modified.
