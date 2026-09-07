# Episode 20/21 quest-info registration audit — 2026-09-06

## Status

The 125-owner QuestInfo relocation is installed across all eleven files.
Current source-only verification passes 44 LF/CRLF positives, 110 content/shape
refusals, four Family-overlay forms, and five overlay-isolation refusals, with
`family_overlay_phase: family-transaction`. The historical 68,937-byte patch
remains reproducible at SHA-256
`fad9d7359ac80514419363b3af482184bca19ddcff2d865d427a87986ca428e4`.
This source-only section does not claim graphical client rendering; the later
live deployment is recorded in the addendum below.

The migration baseline includes root's independently implemented single-snapshot
EP21_DailyKey repair: MysteriousGhostShip SHA-256
58c336e56c135390cf47f98526f598433ec87f3384213ed6f3f4f46bba229965.
The earlier b6baf2df... version belongs to the separate clock defect proof.
The QI migration itself remains transaction-neutral. The independently reviewed
Mandel overlay is installed at `3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb`
and canonicalizes exactly to the QI-only `c72019f5...` source.

## Independent clock-proof review

The original daily-clock driver executes actual parse_script/run_script,
gettimetick/gettime/gettimestr/atoi/date code with deterministic time interception.
It requires exactly four clock reads for the original helper and one for the
fixed helper. Its civil-calendar mktime oracle does not repeat the script's
seconds-of-day arithmetic. The full Bangkok day, reset boundaries, year end,
leap day, UTC, and Kathmandu checks are correctly limited to fixed-offset
behavior; they do not repair the existing fixed-86400 DST policy.

The initial runner accepted insufficiently constrained retained metadata and
only recorded some artifacts after use. Those review findings were sent to root.
The subsequently inspected runner 08e651bf... and driver 34604970... added
trusted retained-build/accepted-receipt pins, exact expected counts, pre-run and
post-run executable/fixture/support checks, and independently known timezone
offset assertions. I independently executed their output/artifact controls:
nine zero-exit output refusals and four artifact refusals passed.

Two final binding tightenings were requested and subsequently observed in the
source: bind the original raw NPC read to its later source snapshot, and freeze
generated fixtures before their first compilation/use. The later driver also
runs every second of both UTC and Bangkok days. The installed-source execution
passed 173,268 fixed cases, 117 stable controls, 45 frozen-original
inconsistencies, and 867,714 assertions. Receipt:
`../episode21-daily-clock-label-v2-20260907/receipt.json`, SHA-256
`bdf9288a91f73b223025cb5f6b5bb980470e9ef4f5ca27c9ff302bfe3831288e`.

## Inventory and native defect

The selected Renewal graph has 900 enabled scripts and 14 include files.
Sixteen enabled files lie under npc/custom/episode20 and episode21. Eleven
contain quest-info statements: 125 sites, 125 distinct NPC owners, 27 static
maps. Episode 20 contributes 71 owners; Episode 21 contributes 54.

Every one of the 125 sites is a standalone literal questinfo statement and
the first executable statement in an ordinary static script NPC in the frozen
migration baseline. That baseline had no `questinfo_refresh` or `showevent`
site in these sixteen files. The installed Family transaction overlay adds
exactly one refresh at `FamilyReputation.txt:89`; it is not one of the 125
registration moves. No affected owner had an existing On* event label.
Exactly one had a non-event label: Mandel's unchanged L_Reputation.

Actual buildin_questinfo (src/map/script.cpp:21701–21788) parses a new condition
and appends it to nd->qi_data on every call. It deduplicates only the owner ID
in map.qi_npc, and does not reinitialize player qi_display. Thus ordinary
repeated clicks grow the condition list; false conditions traverse all copies.
The first lazy registration may change map owner count after player display
initialization, causing pc_show_questinfo's size mismatch early return
(src/map/pc.cpp:15399 onward). Moving registration to startup removes those
dialogue-entry mutations; it does not promise marker refresh for every later
character variable write.

Registration compiles the quoted condition but does not evaluate it against
an attached player. Therefore the preserved literal command can run at OnInit
without requiring the NPC's dialogue RID. Actual condition evaluation remains
per-character later. npc_event_do_oninit invokes the configured startup event
(src/map/npc.cpp:1420), and reload runs fresh OnInit events at line 6139.
Native unloading removes map registration and clears condition data
(npc.cpp:3511/3572, map.cpp:4466); registration cleanup must be tested rather
than represented by resetting a model counter.

All conditions use character quest/register/BaseLevel reads and ten read-only
user-function definitions: EP20_MainComplete, EP20_DailyCanAccept,
EP21_Started, EP21_GaebolgComplete, EP21_CultComplete, EP21_GimliComplete,
EP21_GhostShipUnlocked, EP21_MainComplete, EP21_DailyKey, and the transitive
EP21_QuestInRange. EP20_DailyCanAccept reads its dynamically named permanent
EP20_DailyNext_<quest> variable through getd and reads time; it does not write
that variable. No condition requires NPC-local, instance-local, temporary
character, or global variable scope. The literal expressions, including their
existing boolean precedence and short-circuit behavior, must remain identical.
This is an assessment of migration suitability, not certification of every
underlying quest policy.

The following counts concern these custom owners only. Other enabled content
can register quest markers on the same maps, especially Ice Castle; this
inventory is not a claim that the entire map's native owner count equals the
custom count.

## Per-file and per-map counts

| Source (under npc/custom) | Owners |
| --- | ---: |
| episode20/Progression.txt | 51 |
| episode20/SidesAndDailies.txt | 20 |
| episode21/Progression.txt | 26 |
| episode21/GimliInfiltration.txt | 6 |
| episode21/MysteriousGhostShip.txt | 3 |
| episode21/BlackHairedBeast.txt | 9 |
| episode21/FamilyReputation.txt | 1 |
| episode21/FinalBattle.txt | 1 |
| episode21/SecretAltar.txt | 4 |
| episode21/SilentSanctuary.txt | 2 |
| episode21/SideDailies.txt | 2 |

| Map | Custom owners |
| --- | ---: |
| icas_in | 14 |
| icas_in2 | 3 |
| icecastle | 12 |
| jalbe_in | 2 |
| jor_albe | 4 |
| jor_back1 | 2 |
| jor_back2 | 1 |
| jor_back3 | 1 |
| jor_back4 | 1 |
| jor_back5 | 2 |
| jor_crk | 2 |
| jor_crk_p | 6 |
| jor_maze | 9 |
| jor_mbase | 16 |
| jor_nest | 1 |
| jor_raise1 | 1 |
| jor_root1 | 6 |
| jor_root2 | 1 |
| jor_safty1 | 1 |
| jor_sanct | 8 |
| jor_tail | 1 |
| jor_tmple1 | 4 |
| jor_twice | 4 |
| jor_twig | 6 |
| luna_sf1 | 4 |
| luna_sf2 | 5 |
| mbase_in | 8 |

## Exact installed transformation

The new tools/ci/episode20_21_questinfo_migration.py is read-only. It validates
the enabled import graph, selected file membership, each complete before/after
file hash, all owner counts, literal conditions, and exact inverse recovery.
The five other selected episode files also have fixed unchanged hashes; this
prevents a newly introduced helper/registration from being silently ignored.
It accepts newline-only CRLF-to-LF test views while recording and rechecking raw
active hashes. It neither normalizes other bytes nor writes a runtime file.

For each owner, remove only its unique first literal registration line. Before
the final body brace append:


```text
	end;

OnInit:
	<the exact removed questinfo statement>
	end;
```

The added end is deliberate even after an existing close/end. Nine owners end
with a call to an entrance helper; relying on every such helper to terminate
would let a returned dialogue fall through into OnInit. The exact nine are
Lehar#ep20_canyon_entry, Shaky Wall#ep20_drift, Koko#ep20_sticky_entry,
Disguised Temple Guard#ep21gimli, Maristella Walter#ep21gs,
Nillem#ep21gs_daily, Heine's Tablet#ep21_final, Nyar#ep21_fb_daily, and
Shining Door#ep21. Nothing is inserted inside an existing branch or before
Mandel's L_Reputation label.

The tool's before/after SHA-256 maps are fixed reviewed constants; no rebaseline
command exists. Mixed before/after source batches are rejected. Current
source-only verification commands are:


```text
python3 -B tools/ci/episode20_21_questinfo_migration.py --phase after --self-test
python3 -B tools/ci/episode20_21_questinfo_migration.py --phase after --inventory
```

The `--phase after` verifier checks all eleven complete files and reconstructs
their exact before versions. Other NPCs, functions,
travel, rewards, timers, menus, and declaration coordinates remain byte-identical
in the LF-normalized view. The preserved raw source is checked unchanged while
the tool runs.

## Native registration regression — completed

The installed-source ASan/UBSan proof passed 125 owners, 27 maps, two
parse/startup/click/unload cycles and 500 bounded clicks per phase. The migrated
phase made 274 real condition calls and passed 6,216 assertions; the exact
original controls passed 4,964 assertions. Receipt:
`../episode-qi-native-label-v2-20260907/receipt.json`, SHA-256
`efbffc1de3831db33015f92ecc94975f0e919675f15a1128c997c98722e41700`.

The implemented design follows this checklist:

Reuse the garden_legacy_gate_test.cpp approach: compile current npc.cpp in an
isolated fixture translation unit, initialize its actual private name/event/path
registries, and use actual npc_parsesrcfile, event export/dispatch, questinfo,
script compiler/VM, and npc_unloadfile. Exclude the retained npc.o from linking.
Build a fixture containing exactly the 125 full original or proposed owner
declarations, extracted from the hash-pinned source, plus the required actual
user-function definitions. Preserve their original names, coordinates, view
constants, and complete body text. Do not load unrelated episode monster spawns
or instance-controller scripts merely to test marker registration.

Provide 27 explicitly isolated map nodes and world lookup/packet doubles.
Coordinate handling in that fixture is not a real map-cache walkability claim.
Provide actual known function definitions so condition compilation is genuine;
do not invent always-true functions. Registration can be tested without
executing every condition's quest policy.

Implemented lifecycle cases:

1. Parse genuine originals and prove there are zero new startup registrations.
   Execute each real owner's dialogue entry to the first message, with a clearly
   documented packet-boundary stop before quest/reward/travel code. For 124
   owners that message immediately follows the registration. Horuru#ep20_start
   first calls EP20_SyncStep: load its actual EP20_EffectiveStep and
   EP20_QuestInRange dependencies too, seed a stable loaded character with
   EP20_Step=0 and no relevant quests, and assert the original prefix leaves
   persistent values/update flags unchanged. Do not pretend that potentially
   state-writing helper is part of the pure condition closure. Every
   original click adds one parsed condition; repeated entry adds another, while
   map owner IDs remain unique. This controlled stop tests entry registration,
   not a complete dialogue or actual client continuation.
2. Parse proposed owners and run their actual exported OnInit events with no
   player RID. Require exactly 125 owners, the exact per-map counts above, one
   condition per owner, and unchanged expected icon/color. Prove no dialogue,
   item, quest, reputation, or warp builtin runs during initialization.
3. Repeat the same bounded real dialogue-entry checks: condition counts and map
   ownership must remain unchanged. The exact removed line is absent at entry,
   not merely hidden behind a fixture that never dispatches the NPC.
4. Unload through the native lifecycle and require old IDs/conditions/event/name
   registrations to disappear. Reparse and rerun fresh OnInit; every owner
   returns with a fresh native ID and one condition. Repeat this cycle. Calling
   OnInit twice without unloading is a distinct negative/control state: native
   questinfo is not idempotent and should add a second condition, demonstrating
   the fixture does not silently deduplicate condition objects.
5. Initialize a player display vector with real pc_show_questinfo_reinit. Exercise
   the native wrong-size early return after an intentionally late additional
   owner and prove reinit repairs the size boundary. For owner-condition
   short-circuit tests, use explicitly labeled controlled condition results
   or separate simple true/false probe conditions; do not report those as
   actual truth-table coverage of all 125 production expressions.
6. Preserve the real achievement_check_condition attachment save/restore path
   if condition evaluation is included. Assertions must distinguish actual
   production expressions from condition-result doubles, and direct probes
   from callbacks executed during dialogue/registration.
7. All original/candidate fixture text, source/header closure, retained producer
   receipt, fresh objects, executable, and stdout/stderr need before/after byte
   binding. Run ASAN/UBSAN, deny networking, reject diagnostics even with exit 0,
   and require explicit clean allocator teardown. No SQL save, native client
   packet rendering, whole-server startup, or reward safety claim follows from
   this bounded registration proof.

## Complete owner inventory

Paths below are relative to npc/custom. Line numbers refer to the before
checkpoint, including the separate two-line clock expansion in Ghost Ship.
The tool's --inventory output also includes every exact condition expression.

### episode20/Progression.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 212 | icecastle | Horuru#ep20_start |
| 238 | jor_back5 | Lehar#ep20_gorge |
| 249 | jor_safty1 | Lehar#ep20_refuge |
| 260 | jor_back5 | Miriam#ep20_evidence |
| 271 | icas_in | Leon#ep20_report1 |
| 284 | icas_in | Horr#ep20_signs |
| 301 | icas_in | Torr#ep20_camp |
| 310 | icas_in | Lodge Register#ep20 |
| 319 | icecastle | Suspicious Sign#ep20 |
| 328 | icas_in | Vellgunde#ep20_device |
| 341 | icecastle | Lehar#ep20_canyon_entry |
| 351 | icas_in | Miriam#ep20_canyon_report |
| 360 | icas_in | Lazy#ep20_recent |
| 373 | jor_nest | Lazy#ep20_nestdoor |
| 385 | icas_in | Vellgunde#ep20_site2 |
| 397 | jor_root1 | Rgan Shaman#ep20_infiltrate |
| 415 | jor_root2 | Leize#ep20_hidden |
| 444 | jor_sanct | Bishop Rgan#ep20_sacred |
| 469 | jor_sanct | Device Bishop#ep20 |
| 481 | jor_sanct | Silent Bishop#ep20 |
| 493 | jor_sanct | Pamoshgand#ep20_trust |
| 526 | jor_sanct | Maze Supervisor#ep20 |
| 549 | jor_maze | Saergand Trace#ep20_1 |
| 558 | jor_maze | Saergand Trace#ep20_2 |
| 567 | jor_maze | Shaky Wall#ep20_drift |
| 579 | jor_sanct | Nyar#ep20_drift_report |
| 590 | icas_in | Lehar#ep20_infil_report |
| 603 | icecastle | Diving Iwin#ep20_sea |
| 639 | jor_twice | Kokoro#ep20_icy |
| 657 | jor_twice | Ancient Trace#ep20_1 |
| 666 | jor_twice | Ancient Trace#ep20_2 |
| 675 | jor_twice | Aurelie#ep20_nest_report |
| 702 | jor_maze | Lehar#ep20_rumors |
| 715 | jor_maze | Rgan Rumor#ep20_1 |
| 725 | jor_maze | Rgan Rumor#ep20_2 |
| 735 | jor_maze | Rgan Rumor#ep20_3 |
| 745 | jor_maze | Miriam#ep20_rumor_report |
| 769 | jor_maze | White Cat#ep20_maze_end |
| 782 | icas_in | Leon#ep20_find_lehar |
| 791 | jor_root1 | Lehar's Marker#ep20 |
| 804 | jor_root1 | Lehar's Mark#ep20_1 |
| 812 | jor_root1 | Lehar's Mark#ep20_2 |
| 820 | jor_root1 | Lehar's Mark#ep20_3 |
| 828 | jor_sanct | Lehar#ep20_marks |
| 849 | jor_sanct | Bishop Rgan#ep20_prayer |
| 862 | jor_twig | White Cat#ep20_call |
| 871 | jor_twig | White Cat#ep20_inner |
| 880 | jor_twig | Nyar#ep20_confront |
| 889 | jor_twig | Leon#ep20_sanctuary |
| 899 | jor_twig | Orelli#ep20_final |
| 919 | icas_in2 | Voglinde#ep20_epilogue |

### episode20/SidesAndDailies.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 63 | icas_in | Orlito Joveng#ep20 |
| 104 | jor_root1 | Espionage Cache#ep20 |
| 113 | icecastle | Suspicious Iwin#ep20 |
| 123 | icecastle | Iwin Soldier#ep20_home |
| 137 | icas_in2 | Nadyagand#ep20_home |
| 153 | icas_in2 | Leize#ep20_home |
| 170 | icecastle | Iwin Courier#ep20_delivery |
| 183 | icas_in | Frederike#ep20_delivery |
| 195 | icecastle | Chachako#ep20_delivery |
| 204 | icecastle | Frederike#ep20_search |
| 213 | icecastle | Iwin Courier#ep20_search |
| 222 | icas_in | Iwin Guard#ep20_search1 |
| 231 | icecastle | Iwin Guard#ep20_search2 |
| 240 | jor_back1 | Rorohyu Fluff#ep20_1 |
| 249 | jor_back1 | Rorohyu Fluff#ep20_2 |
| 258 | jor_back2 | Rorohyu Fluff#ep20_3 |
| 267 | jor_back3 | Rorohyu Fluff#ep20_4 |
| 276 | jor_back4 | Koko#ep20_sticky_entry |
| 289 | icecastle | Lalaha#ep20_delivery |
| 333 | icas_in | Toryoryo#ep20_flavors |

### episode21/Progression.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 130 | jor_tail | Shufapa#ep21_start |
| 155 | jor_crk | Nyar#ep21_rift_now |
| 168 | jor_crk_p | Nyar#ep21_rift_past |
| 178 | jor_crk_p | Ivan#ep21_first_meeting |
| 190 | luna_sf1 | Ivan#ep21_safe_arrival |
| 202 | jor_mbase | Resistance Soldier#ep21_prog |
| 219 | jor_mbase | Tris#ep21_arrival |
| 229 | jor_mbase | Tan#ep21_arrival |
| 239 | jor_mbase | Tan#ep21_east_tent |
| 256 | mbase_in | Valdaris#ep21_resistance |
| 266 | mbase_in | Richard#ep21_resistance |
| 275 | mbase_in | Lee#ep21_resistance_in |
| 284 | jor_mbase | Lee#ep21_resistance_out |
| 299 | jor_mbase | Tris#ep21_profit |
| 329 | jor_albe | Nillem#ep21_alberta |
| 408 | jor_mbase | Ivan#ep21_cult |
| 434 | jor_mbase | Lehar#ep21_cult |
| 443 | jor_mbase | Lyriq#ep21_cult |
| 452 | jalbe_in | Lalaila Wigner#ep21 |
| 468 | jor_mbase | Small Boat#ep21_recruit |
| 478 | jor_crk_p | Tan#ep21_recruit |
| 488 | jor_mbase | Ivan#ep21_recruit |
| 510 | luna_sf2 | Ivan#ep21_temple_safe1 |
| 522 | luna_sf2 | Believer#ep21_safe2 |
| 535 | luna_sf2 | Believer#ep21_safe3 |
| 554 | luna_sf2 | Believer#ep21_safe4 |

### episode21/GimliInfiltration.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 85 | luna_sf2 | Disguised Temple Guard#ep21gimli |
| 138 | luna_sf1 | Nadoyo#ep21gimli_report |
| 158 | jor_mbase | Tris#ep21gimli_report |
| 184 | mbase_in | Wilhelm#ep21gimli_report |
| 196 | mbase_in | Reinhardt#ep21gimli_report |
| 216 | jalbe_in | Maristella Walter#ep21gimli_report |

### episode21/MysteriousGhostShip.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 84 | jor_albe | Maristella Walter#ep21gs |
| 127 | jor_albe | Nillem#ep21gs_daily |
| 178 | jor_albe | Wigner Executive#ep21gs |

### episode21/BlackHairedBeast.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 56 | jor_mbase | Tris#ep21_finale |
| 119 | mbase_in | Heine's Tablet#ep21_repair |
| 161 | jor_mbase | Nadoyo#ep21_reports |
| 221 | luna_sf1 | Nadoyo#ep21_crevice |
| 243 | jor_tmple1 | Cult Priest#ep21_serpent |
| 256 | jor_tmple1 | Nadoyo#ep21_temple |
| 283 | mbase_in | Nyar#ep21_plan |
| 303 | jor_tmple1 | Heine's Tablet#ep21_final |
| 317 | jor_tmple1 | Nyar#ep21_after_final |

### episode21/FamilyReputation.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 18 | jor_mbase | Mandel#ep21_reputation |

### episode21/FinalBattle.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 77 | jor_raise1 | Nyar#ep21_fb_daily |

### episode21/SecretAltar.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 8 | jor_crk_p | Suspicious Book#ep21 |
| 20 | jor_crk_p | Shining Door#ep21 |
| 36 | jor_crk_p | Nyar#ep21_altar_route |
| 67 | luna_sf1 | Bushes#ep21_heine |

### episode21/SilentSanctuary.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 18 | jor_twig | Nyar Clone#ep21_sanctuary |
| 58 | jor_crk | Nyar#ep21_complete |

### episode21/SideDailies.txt

| Line | Map | Owner |
| ---: | --- | --- |
| 61 | mbase_in | Reinhardt#ep21_lugenburg |
| 71 | jor_mbase | Tris#ep21_lugenburg |

## Before-file SHA-256 checkpoint

These values are repeated as fixed constants in the verifier; its output includes
all corresponding proposed after-file hashes.

| Source (under npc/custom) | LF-normalized SHA-256 |
| --- | --- |
| episode20/Progression.txt | ab08d7b63804e85aa892e0c315e945f13c37d118af92e58cd3e47d22c0e0c649 |
| episode20/SidesAndDailies.txt | 895e602387edce65bb352cd17e51cbe88e0c5e83e4766bfa8bb08c6e2f5f37d4 |
| episode21/Progression.txt | 0def3b01c11e6302d8d6133db2fe05713a2c10f320ce244f59f7c0ea1b6453d3 |
| episode21/GimliInfiltration.txt | 1b719604835239f32a6e09e5d4367540fafa4fe487a9a13bce0fa3c3c9d64293 |
| episode21/MysteriousGhostShip.txt | 58c336e56c135390cf47f98526f598433ec87f3384213ed6f3f4f46bba229965 |
| episode21/BlackHairedBeast.txt | 40e9f25f960a1cb3b73a7c757ba63167b65d38d7830ceb442fef758feb76e151 |
| episode21/FamilyReputation.txt | d49ef88c4d7b2ef1a16fccff22f0f13aa82bf790c9a6a92a42ef66b4d8c39f17 |
| episode21/FinalBattle.txt | 3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1 |
| episode21/SecretAltar.txt | 866e4c4590b8de634b1e5f78ce5dbf047c0129aabdea5bf0bb9cf04dff4b9022 |
| episode21/SilentSanctuary.txt | cbd658dd184ecce5a15eedf1774c60d62bb3478c11f8238201f5c42a9cbc27bb |
| episode21/SideDailies.txt | 1b305582511432b4fc76d737c907240ac7064dc4cc79f8606632938e869ec09c |

## Reviewed Family transaction overlay compatibility

The migration's `pair(path, data)` remains restricted to the exact before and
after identities above. For an installed `FamilyReputation.txt`, `active_pair`
first accepts only the complete QI-only identity
`c72019f51033d96054c277d952d993a45a68062da9f7b4ab99ee76edde2a9605`
or the separately reviewed complete Family transaction identity
`3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb`.
The latter is exactly inverted to `c720...` by a self-contained, anchor- and
hash-pinned adapter before the QI-only pair reconstructs this table's `d49...`
before identity. Partial, unknown, or directly mixed transaction/QI input fails
closed; the Family transaction edits are not counted as registration edits.

## Live deployment addendum — 2026-09-07

All eleven registration-migration targets, including the final Family overlay,
were installed by the exact 13-file r7 release. Candidate and live startup
validated the POST counts and complete `OnInit` registration lifecycle; every
live file matched its pinned candidate hash after restart. This does not add a
graphical-client claim. See the
[deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
