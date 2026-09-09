# MuhRO hotfix comparison: #50–74

Reviewed 9 September 2026 against the local renewal server, including ignored db/import files. Each primary hotfix post was read through web browsing. This is a source and data comparison, not proof that all play paths or client binary changes match MuhRO.

## Confirmed changes

- #51: Liberation Shadow Shoes (24063) already added 2% physical/magical boss damage, but refinement 7/9 added only physical damage. Added corresponding +1/+2 magical bonuses in `db/re/item_db_equip.yml`. Hasty Armor, Rogue Armor, Randgris Card, and both STR Gloves already match the post.
- #52: Chapter 1 resistance was checked at entry but had no expiry ejection. Reported to the coordinating agent for NPC repair.
- #57: Amicitia 2 lacked a nomemo declaration. Reported to the coordinating agent for mapflag repair.
- Supplemental #16: Deviruchi transformation passes ASPD val1=10 in legacy tenths; status script erroneously treated it as +10 flat ASPD. Changed `Mtf_Aspd` to divide by 10 in `db/re/status.yml`; distinct `Mtf_Aspd2` remains unchanged. [Hotfix #16](https://dis.muhro.eu/t/hotfix-16-06-december-2024/2893).
- Supplemental #28/#29 review found item 23582, a three-day fly-wing box, grants 10,080 minutes (seven days). Corrected `E_WING_OF_FLY_3DAY_BOX` to 4,320 minutes in `db/re/item_group_db.yml`. Three Giant Fly one-day group variants already grant 1,440 minutes, with one result each. This is a duration defect found by the comparison, distinct from the post's duplicate-wing symptom.

## Every assigned post

| Post | Disposition |
|---|---|
| [#74](https://dis.muhro.eu/t/hotfix-74-07-september-2026/5280) | Homepage stylist, patcher compatibility and proprietary client rendering/loading/chat fixes. No transferable server patch or implementation detail is published. Client behavior remains unverified. |
| [#73](https://dis.muhro.eu/t/hotfix-73-03-september-2026/5259) | MuhRO seasonal egg pricing/chances and patcher features; no matching local seasonal implementation located. Do not invent prices or probabilities. |
| [#72](https://dis.muhro.eu/t/hotfix-72-13-july-2026/4961) | Reversion of MuhRO palette package. Local assets are a separate client distribution; server changes cannot reproduce this. |
| [#71](https://dis.muhro.eu/t/hotfix-71-05-july-2026/4922) | Custom Mystery Voyage and What-A-Mole event rules. Matching event implementation not found in local custom scripts. |
| [#70](https://dis.muhro.eu/t/hotfix-70-26-june-2026/4883) | MuhRO network/proxy infrastructure and patcher latency display. Settings are unspecified; no blind network tuning. |
| [#69](https://dis.muhro.eu/t/hotfix-69-25-june-2026/4878) | Unspecified latency correction, custom UI/fonts/staff colors/proxy/screens. No evidence-backed server code change can be derived from the post. |
| [#68](https://dis.muhro.eu/t/hotfix-68-21-june-2026/4863) | Client chat preferences, skill labels, FPS and item-count display. Client binary functionality, not database bugs. |
| [#67](https://dis.muhro.eu/t/hotfix-67-09-june-2026/4829) | Client CPU optimization and acknowledged announcement-color issue. No published implementation. |
| [#66](https://dis.muhro.eu/t/hotfix-66-22-may-2026/4754) | Reversal of MuhRO Eden Market prices. These are custom vendor economy rules, not canonical item Buy/Sell prices; left local economy unchanged. |
| [#65](https://dis.muhro.eu/t/hotfix-65-16-may-2026/4728) | Gepard update for Druid Nature Protection. Requires corresponding third-party anti-cheat release, no rAthena patch specified. |
| [#64](https://dis.muhro.eu/t/hotfix-64-11-may-2026/4700) | Gepard compatibility/attack fixes; same third-party limitation. |
| [#63](https://dis.muhro.eu/t/hotfix-63-03-may-2026/4657) | Unspecified client crash repair and wide-description toggle. Not reproducible from server source alone. |
| [#62](https://dis.muhro.eu/t/hotfix-62-02-may-2026/4649) | Gepard crash update, no transferable source details. |
| [#61](https://dis.muhro.eu/t/hotfix-61-26-april-2026/4609) | Turbo configuration support and patch mirror infrastructure; MuhRO patcher-specific. |
| [#60](https://dis.muhro.eu/t/hotfix-60-01-march-2026/4286) | Custom hourly/happy-hour additions are policy. One-handed Encroached Axe 520054 exists in ignored `zero_cell_item_db.yml`; target Frontier Claw Axe 520055 exists in `enchant_repair_items.yml`, but no reform or enchant route references the source axe. Confirmed progression coverage gap, reported; requires a complete tuning item/NPC/material definition, not a guessed substitution for the separate two-handed axe. |
| [#59](https://dis.muhro.eu/t/hotfix-59-28-february-2026/4279) | Encroached equipment and Zero Cell boxes currently have no explicit sale value. Difference confirmed, but intended amounts are absent from the post. No arbitrary economy change. |
| [#58](https://dis.muhro.eu/t/hotfix-58-28-february-2026/4275) | Flower event is custom. No Gate of Hell entry found in Zero Cell skill overrides. DK crown/sword combo exists in `item_combos.yml`; post gives no exact defect, so no speculative rewrite. Encroached weapon enchant-category coverage is missing locally and needs a separate fully sourced implementation. |
| [#57](https://dis.muhro.eu/t/hotfix-57-22-february-2026/4248) | Amicitia nomemo gap reported for repair. Main-account subscription rewards are MuhRO account-policy infrastructure. Existing saved memo rows require separate runtime/database handling if any exist. |
| [#56](https://dis.muhro.eu/t/hot-fix-56-02-february-2026/4170) | Acolytes 20521–20524 are only commented placeholders in local mob DB; the Illusion Labyrinth content needed for the four crafting-material drops is missing. Not a safe four-row patch. [Primary dungeon reference](https://wiki.muhro.eu/Illusion_of_Labyrinth). |
| [#55](https://dis.muhro.eu/t/hot-fix-55-01-february-2026/4164) | Nefeeru costume recolor addition; matching local NPC not found. Optional custom content, not a confirmed defect. |
| [#54](https://dis.muhro.eu/t/hotfix-54-26-january-2026/4136) | Chapter 1 hunt repair has no quest IDs or implementation details. Cannot infer a specific bug merely from the announcement; broader quest integrity checks belong to the coordinating audit. |
| [#53](https://dis.muhro.eu/t/hotfix-53-20-january-2026/4100) | Chapter 1 regional drop adjustments are applicable content differences, but increased rates are unspecified and local monster/item naming differs. Do not guess drop rates or silently change the economy. |
| [#52](https://dis.muhro.eu/t/hotfix-52-20-january-2026/4099) | Protection-expiry bypass confirmed and delegated. Dark Whisper damage reduction has no published target value; left untouched. |
| [#51](https://dis.muhro.eu/t/hotfix-51-18-january-2026/4094) | Fixed Liberation magical refine increments; four other named equipment corrections already present (both STR Glove variants checked). Chapter stone migration, encounter tuning, and lag improvements need specific data/runtime evidence; no bulk player-item conversion attempted. |
| [#50](https://dis.muhro.eu/t/hotfix-50-20-december-2025/3995) | Mystical Amplification has 700 ms variable and 700 ms fixed casting with explicit ignore flags and Sacrament handling in `skill.cpp`. Post does not disclose intended values; no evidence sufficient to change skill balance. |

## Validation and limitations

`bonus_regression.py` extracts the actual edited Liberation script and Deviruchi status expression into a small C++ fixture; tests all refine levels 0–20 for physical/magical symmetry and 2/3/5% expected thresholds, and the legacy ASPD conversion. This checks expression behavior, not the production script VM or combat engine. Full native server startup parsing is performed by the coordinating agent.

The prior EM native-VM executable was tried in an isolated temporary directory, but its glibc/MySQL runtime did not match the host/container. That attempt is explicitly not a passing native test. Artifacts: `liberation-input.json`, `liberation-native.log`; source-extracted replacement: `bonus-regression/bonus_regression.cpp`.

Supplemental Aquila Max Pain issue (#15): two live-compatible import rows have zero cast time, matching the reported symptom, but the published hotfix does not specify the corrected duration. Deferred rather than guessing encounter balance.

No deployment or commit was performed by this worker. Refer to the coordinating report for final installation and broader validation outcomes.

Retained regression drivers are `tools/ci/hotfix_bonus_regression.py` and `tools/ci/fly_wing_rental_regression.py`. The rental regression passed locally using PyYAML: all four groups have one deterministic wing reward with the correct lifetime. Bonus driver execution is coordinated on the Linux host.

Deviruchi unit cross-check: a full ignored-file-inclusive search of db/npc/src found one SC_MTF_ASPD producer (`transform 1109,1200000,SC_MTF_ASPD,10,5`). `pc.cpp` converts each bAspd unit to ten internal units. The [original emulator implementation announcement](https://board.herc.ws/topic/2525-monster-transform-update/) documents Deviruchi as +1 ASPD and +5 HIT. Existing SC_MTF_ASPD2 producers and their status script are unchanged.

Supplemental #33: Signets of Circulation (`grademk_service_enchants.yml` ID142, four seasonal targets) support all 24 trait upgrades (six traits, levels 1–4). Slot 2 has no direct PerfectEnchants list, so the exact MuhRO perfect-second-enchant feature is absent rather than implemented with a POW-only restriction. No guessed perfect cost or rate was added. The separate Star Signet family ID13 has seven slot-2 perfect choices and is not the same family.

## Final follow-up checks

The three changed DB files in the fresh live snapshot are byte-for-byte identical to repository HEAD before this work. The candidate-to-snapshot diff contains only the three described corrections: two added Liberation bonus statements, one rental duration replacement, and one Deviruchi expression replacement. SHA-256 evidence: `recent-live-db-baseline.json`; complete changed-line comparison: `recent-live-db-comparison.json`. These comparisons establish that the candidate does not overwrite an independent live modification to these files.

Aquila remains an explicit unresolved review item. Both the fresh live snapshot and current repository have identical Max Pain rows for monster IDs 21531 and 21588 in `db/import/mob_skill_db.txt`: skill 716 level 5, chance 2500, cast time 0 ms, delay 30000 ms. The [primary hotfix #15](https://dis.muhro.eu/t/hotfix-15-02-december-2024/2867), also saved in `topics/2867.json`, says the instantaneous cast was fixed but gives no replacement cast time, trigger, or other mechanics. A 1000 ms cast would be an unsupported encounter-balance assumption, so neither row was changed. This gap is not counted as fixed or passing.

The Signet slot-2 limitation is likewise not counted as fixed: having all six trait upgrade paths does not establish that direct perfect enchant selection exists. Exact deterministic costs, target levels, and any restrictions must be sourced before implementing that missing feature.
