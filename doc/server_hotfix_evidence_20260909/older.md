# Older reference server hotfix comparison ? 2026-09-09

Scope: public hotfixes #1?24, repository and fresh live snapshot. These are source/data checks, not a full game playthrough. Missing private features and underspecified notes are deliberately not reported as passing gameplay tests. All24 source posts were read from the successfully downloaded Discourse topic corpus.

| Hotfix | Disposition | Evidence / limit |
|---|---|---|
| #1 | Client-only / not reproduced | Costume Shadow Devil resource patch belongs to the client archive; no server defect established from this notice. |
| #2 | Already supported | db/re/pet_db.yml:799 and820 map CHUNG_E / CHUNG_E_ to Tantanmen; item12395 executes pet;. Static item-to-pet linkage is present. |
| #3 | Not reproduced / incomplete specification | db/re/job_stats.yml:7077?7953 defines all expanded fourth jobs including Spirit Handler; pc.cpp job conversions include these classes. The post supplies neither expected HP/SP tables nor a failing skill-point sequence. No speculative class rebalance or private Erundek shop copied. |
| #4 | Not reproduced / needs scenario | pc_calc_skillpoint sums actual learned permanent/replaced skills; no unconditional extra9 grant there. The repeated-reset report has no triggering sequence or reference server implementation to compare; a new-character/job-change playthrough remains necessary. |
| #5 | Not reproduced / needs scenario | Same learned-skill accounting path inspected in pc.cpp:2518. The post provides no incorrect count/example for rebirthed novices, so no global skill-point mutation made. |
| #6 | Server-specific drop change / unresolved parity | Bio5 records and existing imported drops inspected. Notice does not identify item ID or intended rate for Cursed Crystals. No invented item mapping or drop-rate change applied. |
| #7 | No matching permanent state reproduced | db/re/status.yml:4170 models NoConsumeItem as part of Deepsleep; status.cpp:14826 decrements the remaining count and exits to status cleanup. Timer is finite even for sub-2-second durations. Runtime overlapping-sleep scenario not played. |
| #8 | No matching elemental rejection found | Lauda Agnus/Ramus and Renovatio handlers under src/map/skills/acolyte use status application/party iteration; no holy-armor rejection branch in these handlers. No speculative removal of general element rules. |
| #9 | reference server custom feature | Magikarp Fluffring/reroll availability is a private costume-service catalog change; no matching NPC names found. Not a proven defect in this server. |
| #10 | Already implemented | db/re/status.yml:6233?6254 Mtf_Mhp/Mtf_Msp use bMaxHPrate/bMaxSPrate, preserving percentage interpretation. |
| #11 | Mixed server-specific changes | Kafra definitions exist in npc/re/kafras and npc/kafras; notice omits wrong/expected coordinates. Dark Claw blocking Max Pain is explicitly a temporary balance adjustment, not automatically copied. Waifu Garden is a reference server-specific map feature. |
| #12 | Already implemented | db/re/status.yml:8456 Infinity_Drink grants critical/ranged/magic damage, MaxHP/SP percentage and no cast cancel. Item scripts start the matching status. |
| #13 | Current exchange guards present | npc/custom/instances/ConstellationTower.txt:182?220 validates positive fixed material costs and carrying capacity before each exchange. The undocumented legacy exploit cannot be reconstructed, but current paths do not use player-supplied negative quantities. |
| #14 | Already implemented | Speed Potion12016 starts SC_SPEEDUP1 with50; db/re/status.yml:750 applies bSpeedRate from its value. No missing effect found. |
| #15 | Confirmed matching defect; delegated DB fix | Both Aquila21531/21588 NPC_MAXPAIN rows in db/import/mob_skill_db.txt had zero casttime (lines972/1101). Sent to recent_hotfix agent for a nonzero windup and regression; see final root deployment report for chosen duration/status. |
| #16 | Confirmed matching defect; delegated DB fix | Deviruchi12658 passes SC_MTF_ASPD value10; db/re/status.yml:6004 fed it directly to bAspd and pc.cpp:3972 scales again by10. This grants+10ASPD instead of+1. Other SC_MTF_ASPD2 scrolls already use value2 for+2 and must stay unchanged. Delegated targeted correction to recent_hotfix. |
| #17 | Existing compatibility support / incomplete exact parity | db/import/shadow_repair_laphine.yml includes S_Safeguard_Shield among accepted shadow targets. The exact Race Shadow Thumb Box identity is not specified by the post; no assumption that every shadow box should accept it. |
| #18 | reference server item / not mapped | Hero's Heirloom by that name is not identified in local server data. No broad battleground item restriction added to unrelated items. |
| #19 | reference server NPC / not mapped | No Abandoned Sunflower NPC declaration found at wolfvill37,257. This is feature availability in reference server, not evidence to recreate its private costume reward logic. |
| #20 | Mixed custom services | Constellation exchange fixed positive quantities and inventory guards present (same paths as13). No matching Pet Fairy service found; Freyja/Galensis pet availability is not automatically inferred from monster presence. |
| #21 | No server field-width defect found | clif_inventory_expansion_info uses PACKET_ZC_EXTEND_BODYITEM_SIZE expansionSize int16, not uint8. Values above250 are representable; actual client display not tested. |
| #22 | Unspecified / not reproduced | Only generic notice of @hateffect issue, no symptom. Native script hateffect validates effect and unit; no matching custom @hateffect command was found. No guess at a fix. |
| #23 | Partially implemented; explicit missing pet content | Dark Bible and Fruit Set Trap have pet scripts and TameItem mappings. Old Tree's Dew23257 / Stinky Rotten Meat23258 have TODO scripts and commented-out pet records; Beehive Box102723 has no pet script or pet record. These need complete pet content/specification, not merely uncommenting a consuming item. Delicious Earthworm/Fruit Platter translated identities not conclusively mapped. Left documented. |
| #24 | Already implemented in inspected paths | Oscar#oghcm_reward checks completion, per-account token and checkweight2 before grants at OldGlastHeimChallenge.txt:420?448. Four Colors Charm selects SC_FIRE_CHARM_POWER for CHARM_TYPE_FIRE. No matching missing-handler bug found. |

## Additional confirmed fix from #44

reference server hotfix44 matches an actual character-server truncation: map-server saves all statuses while chmapif_parse_askscdata returned at most50. Changed src/char/char_mapif.cpp to allocate for actual row count up to the uint16 packet-length bound. This preserves the existing protocol and avoids false warning at exactly50. Packet length uses an unsigned16-bit cast.

Regression tools/ci/scdata_reload_test.py extracts the production handler and actual status_change_data structure, then exercises SQL/FIFO doubles. Host Linux g++ with AddressSanitizer passed8 counts:0,49,50,51,100,1000,1365,1366. The last case correctly caps at1365 with a warning; every other row and all six saved fields are checked, including64-bit duration, account/character identity and packet size. Full char-server build is coordinated by root. No deployment or commit performed by this agent.

Database fixes #15 and16 are owned by recent_hotfix agent; root report is authoritative for final changes and deployment.

## Additional confirmed RODEX operation fix from #43

reference server hotfix43 describes NPC/storage and mail overlap restrictions. Modern RODEX mail_invalid_operation previously checked only MF_NORODEX. It now rejects NPC dialogue/shop, storage, trade, vending and buying-store states. Its existing callers cover inbox refresh, read, claim, delete, return, recipient lookup, attachments and send. mail_writing is deliberately allowed so composing a message can complete. Classic pre-20150513 mail behavior is preserved.

Normal, guild and premium storage opening now reject mail_writing; premium load rejects it before requesting data and premium open checks again after asynchronous return. Guild asynchronous replies re-enter the guarded guild-open function (intif.cpp:1529). The existing NPC-click parser already rejects mail_writing. NPCs can still open storage legitimately.

Regression tools/ci/rodex_operation_test.py extracts the actual shared guard and normal/premium storage-open handlers. It checks256 combinations of NPC/shop/storage/trade/vendor/buying-store/map restrictions with and without active mail composition, plus NPC-to-storage, composer-to-storage and delayed premium reply transitions. Native execution and full map build are coordinated by root. This patch does not invent an inbox-open flag without a reliable close signal, and does not cancel already-pending character-server attachment acknowledgments (which could otherwise lose items).
