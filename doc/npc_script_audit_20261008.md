# NPC script audit — October 8, 2026

## Scope and method

Ten parallel auditors read every NPC script loaded by `npc/scripts_custom.conf`
(about 100 files, 3 MB), plus every hunk the fork changed in official scripts
since the rAthena baseline (`git diff 64fd20c6b HEAD -- npc ":!npc/custom"`,
57 files). Each critical or high finding was then re-traced against the source
and engine before any change. Findings already recorded as known in `doc/`
were excluded unless the documented fix was absent.

`scripts_test.conf` is commented out in both `scripts_main.conf` files, so the
`npc/test` fixtures are not live.

## Engine behaviour the findings rely on

Verified in this fork's source:

| Behaviour | Source |
| --- | --- |
| A paused dialog blocks dropping, trading, equip changes, NPC clicks, bank, RODEX and `@storage`. | `clif.cpp:12140,12596,12215,12300`; `bank_ui.inc:76`; `mail.cpp:406`; `atcommand.cpp:1045` |
| Inventory and cart transfers are **not** blocked during a paused dialog. | `pc.cpp:6854` (`pc_cant_act2` excludes `npc_id`) |
| Before this audit, vending purchases and buying-store sales were **not** blocked during a paused dialog (only opening the list was). | `clif.cpp:14273` vs `14282`, `14294`, `19400`, `19441` |
| `sleep` and `sleep2` both release the player for the wait (`script_detach_state`); only `sleep2` keeps the script attached. | `script.cpp:4510`, `21299` |
| `Zeny -= cost` with too little Zeny fails silently and the script continues. | `pc.cpp:10733` |
| `delitem` aborts the script when items are short, but before this audit it reported success when `pc_delitem` refused during a pending save. | `script.cpp:8742`, `8493`; `pc.cpp:6228` |
| A full-inventory `getitem` returns failure without a floor drop, and the script continues. | `script.cpp:7820`, `4149` |
| `changequest` reports success even when the target quest already exists. | `quest.cpp:645`; `script.cpp:22053` |
| Bound @commands typed during a dialog are queued, and `atcommand_disable_npc` blocks player commands. | `npc.cpp:5171`; `atcommand.cpp:12464` |
| Client packets are not processed while a transaction save is pending. | `clif.cpp:25928` |

## Fixed: critical and high

| # | Severity | Area | Defect | Fix |
| --- | --- | --- | --- | --- |
| 1 | Critical | `episode21/Progression.txt` Ivan, `GimliInfiltration.txt` Maristella | Ivan re-added quest 17752 after the chain advanced. Maristella's `changequest 17756,17757` then failed silently and paid 10 tradeable vouchers plus 5 Gaebolg reputation on every click. Honest players could be left stuck with 17756. | Ivan gates on the whole 17752–17763 range, and his marker matches. Maristella refuses (and clears the stray 17756) when 17757 exists, and pays only after verifying the change. Reinhardt gets the same verification. |
| 2 | Critical | `instances/OldGlastHeimChallenge.txt` Oscar reward | The single account-wide token let two characters on one account alternate between their own instances and claim circlets, antiquity boxes and progression points repeatedly. | A per-instance, per-account claim record is set before the grant, and the claimer must have been admitted to the instance. |
| 3 | High | `instances/HallOfLife.txt` gate | The key was counted before the menu and deleted only after `instance_create`. Moving the key to a cart during the menu gave keyless runs. | Recheck after the menu, delete before creation, refund if creation fails. |
| 4 | High | `instances/FallOfGlastHeim.txt` Oscar room NPCs | Several members holding the dialog each spawned another room population or Cursed King Schmidt MVP (extra EXP and drops). | Stage recheck after the last pause in all five Oscars, and once-per-instance spawn guards. |
| 5 | High | `chapter1/CH1.c` `eventtrigger#EG03` | An NPC-wide `.active` flag was cleared only at the end of a 60-second cutscene. One disconnect blocked quest 19239→19240 for every player until restart. | Progress the toucher first, and make the lock a timestamp that expires after 90 seconds. |
| 6 | High | `episode19/quests_19.txt` Vellgunde | The undefined group `IG_ICE_F_STONE_BOX2` became `getgroupitem(0)`, so every exchange took 35 Snow Flowers and gave nothing. | Use `IG_ICE_F_STONE_BOX` (as the Laphine recipe already does), and require a free inventory row before taking the flowers. |
| 7 | High | `episode21/FieldMonsters.txt` | Five Episode 21 MVPs (Icehorn, Yortus Arbiter, Yortus Bailiff, Jortus Judge, Jortus Executioner) had no delay, so they respawned every 5 seconds with MVP drops. | Explicit 120 ± 10 minute respawn. Retune if a different timer is intended. |
| 8 | High | `episode20/Progression.txt` | The Diving Iwin warped anyone to `jor_twice`, and ungated return portals then walked past every earlier Episode 20 gate (the maze needs step 6, the zone beyond it only step 4). | The Diving Iwin and all nine return portals use `EP20_WarperAccess` on their destination. |
| 9 | High | `jobmaster.txt` | A Karnos could become Alitea at 99/50 through the generic third-class branch instead of the Druid Mentor's 200/70 (with fourth-tier traits and gear). | Druid-line characters are directed to the Druid Mentor. |
| 10 | High | `src/map/clif.cpp`, `src/map/script.cpp` | A vending window opened before an NPC dialog could still buy, and a buying store could still be sold to, while the dialog was paused. Zeny could therefore drop between any script's check and its charge, e.g. the free 5M OSC0005 special enchant and free Artisan Tene enchants. | Vending purchases and buying-store open/sell requests are refused while `npc_id` is set. `delitem` now fails closed when the inventory or cart is transaction-locked. |
| 11 | High | `cities/aldebaran.txt` Kafra reserve lotteries | The prize was rolled before the capacity check, and a failed check spent nothing, so a full inventory could reroll until the jackpot. reserve1 also paused between roll and grant. | Every prize in the tier is preflighted (check-only `F_ReserveLotteryGrant`) before the roll, with no pause between roll and grant. |
| 12 | High | `re/quests/quests_16_1.txt` Dylan | The space check ran only on a failed roll. Filling the last slot from a cart cancelled every failure for free, so badge resets always succeeded. | The space check is unconditional and runs before the roll. |

### Regression coverage

`tools/ci/npc_audit_20261008_test.py` (added to the source phase of
`release_checks.py`) pins the ordering behind each fix. All 12 checks fail
against unmodified HEAD and pass on the repaired tree. They are source-order
assertions only, and don't replace native startup or play testing.

## Fixed: medium

1. **Varmundt Ellie imprint** (`varmundt_biosphere_quests.txt`): consumes only an unequipped, +0, uncarded, ungraded, option-free, unbound, non-rental base copy (`delitemidx`), and charges nothing if that copy can't be removed. `npc/custom/dynamic_reward_catalog.json` was regenerated; only that file's source hash changed, and the recipes are identical.
2. **Geffen Magic Tournament Event 2**: the cutscene counter is script-local, and `$gmt_account_id` is now the instance's `'gmt_account_id`.
3. **Lake of Fire**: the full reward is keyed by the account's last claim date (`gettime(DT_YYYYMMDD)`). Each account gets one extra full claim right after deployment, because old stored values were run tokens.
4. **Sunken Tower**: a rostered member returning to a started run skips the rank-band checks.
5. **Dark Whisper simulated clear**: no pause between the capacity check and the grants. The clear is recorded only after the box arrives, and Root Coins (also Est#2's) use `F_CH1_GiveReward`, which queues undeliverable coins. The fail-check counter no longer jumps to a missing label, offline members wait with `sleep`, and only a clear at or above the highest unlocked level unlocks the next one.
6. **Airship Destruction**: members may re-enter the party's live run. The cooldown gates only new runs, and a character on cooldown can't regain the clear quest.
7. **Late joiners**: Final Battle and Sticky Sea record a character roster at the encounter start and require it at the reward and crystal NPCs. The Sticky Sea start also rechecks its stage.
8. **Episode 20 whiskers**: a zero base pays zero. Pamoshgand checks reward capacity before taking feathers.
10. **Warper**: guild dungeons, the Hall of Abyss, SE/TE guild dungeons and the Tomb of the Fallen are no longer warp targets. `ValidateWarp` rejects them, including a saved Last Warp, and the menus route to the official Gate Managers and Ohno Tohiro.
11. **Weekly Expedition**: kill callbacks and every timer tick sweep tracked guardians that match `killedgid` or no longer exist. Wave slots are cleared at each wave, the console no longer holds a dialog during a run, and an attempt can't start when the room expires within 630 seconds.
12. **Item-script menus** (`pc.cpp`, `script.cpp` `consumeitem`): when a suspended item dialog is freed, it is closed and its pending menu/input state is dropped, unless an NPC dialog already owned it. The Infinite Catalyst Box now opens its menu through a floating NPC event (`addtimer` → `PN Catalyst Box::OnOpen`).
13. **Gold Point dyes** are account-bound (`allow_bound_sell: 0x0`), so they can't be sold. The Gold clock re-arms if a tick was dropped from a full event queue.
14. **Alice Hard mode**: the boss GID is read from `$@mobid[0]`, and the `alice_test.py` double now models `monster` correctly.
15. **Thanatos**: the dummy's GID is captured before sleeping, the skill loop follows the current boss (including Broken Thanatos), the floor-4 fire chain stops once the floor is clear, and the antiquity entitlement clears only when the item arrived.
16. **Twilight Garden**: every escort resume rechecks the story state after its menu. State 10 advances to 11 at once when the Heart Hunters are already dead, in both the resume and the normal event.
17. **OSC0005 and Artisan Tene**: Zeny and materials are rechecked immediately before each charge, and refunds return only what was charged.

## Fixed: low

- Chapter 1:
  - Kafra cart rental charges 700z once a cart is given.
  - Investigators hide per player (`cloaknpc`) instead of globally.
  - Verus believers require the Purification Amulet, with a recheck after their pauses.
  - `#c01ms41` gets its missing `end`.
- Chapter 2: Cardron names the auto-selected equipped item and requires the same copy after the confirmation.
- Geffen Magic Tournament:
  - Coin capacity is rechecked right before each grant.
  - The timeout kills the correct `OnMobDead` label.
- Episode 19:
  - Iwin Patrol gets its missing `break` and correct event labels.
  - Lunch consumes items before its pauses.
  - NPC names are corrected (Comfort, Rehar, Alnagusdagand, Horuru).
  - Friederike checks room for the Purified Core.
- Episode 21:
  - Two door landings are moved outside their trigger cells.
  - Nyar's storages open configured pages 0/100/101.
- Old Glast Heim Challenge: the start rechecks stage and leadership after its menus.
- Exchanges:
  - Constellation Tower and Geffen Night Arena exchanges consume only a plain base copy.
  - The Geffen Night Arena returning check uses a per-instance entry flag.
- Airship Crash: rescue EXP requires the member inside the instance.
- Apprentice Craftsman deletes the validated record (`delitemidx`).
- Wave Mode never warps a jailed character or one on a `nowarp` map at login.
- Main Office:
  - `freeloop` is restored correctly.
  - Practice kills in private lab/expedition rooms no longer count.
  - Lab challenges chosen outside the console are queued and announced.
- Utility NPCs:
  - Missionary Rosetta confirms before assigning and asks before overwriting memo slot 4.
  - Multi-storage waits at most about 10 seconds.
  - Tina's ticket text is corrected.

## Follow-up release (same day)

- **Late joiners**: Ghost Ship reports, the one-time Final Battle story reward and the Sticky Sea story branch now require a roster recorded at their encounter start (`'gs_roster`, `'fb_story_roster`, `'ep20_sticky_roster`). These rosters use `getmapunits`, which walks every unit and compares its map, so players just moved by `instance_warpall` are included.
- **`episode20/Instances.txt` (The Undying)**: same `monster()` misuse as Alice. Lasgand's GID and the Vision GIDs are now read from `$@mobid[0]`, and `immortal_instance_test.py` models the native command.
- **Twilight Garden**: the 8→9 ambush runs as an NPC event (`Almond#amond01::OnAmbush`), so a leader disconnect no longer skips the Heart Hunters.
- **Dark Whisper**: fail checks wait detached and re-attach members each tick, so a member logout no longer stops them. The never-set `counter_started` check is removed.
- **Geffen Magic Tournament Event 2**: the cutscene runs detached. The dialogue and buff are given only if the killer is still online.
- **Smaller fixes**:
  - Ellie names the input item.
  - The investigator cooldown holds across map changes.
  - Ascetic Jeon's quest condition has the intended precedence.
  - Apprentice Craftsman selects the non-rental copy.
- **Test fixtures**: `ghost_ship_report_claim_test.py` and `episode21_encounter_flow_test.py` seed the voyage roster through the VM. `ghost_ship_report_claim_test.py` also adds a refused late-joiner case.

## Not bugs (evidence)

- **Episode 20 multi-choice dailies**: the official client quest data defines a separate `[Standby]` cooldown quest for each option (19163/19165, 18237/18239, 11950/11952). Completing both options per day is official behaviour.
- **The `dic_dun` warper gate**: dungeon access is officially open. The Manuk Teleport Cat, the Dimensional Gap and the plain field portal lead in, and El Dicastes itself is still gated at the city guard. The warper's stricter `dic_dun` check is a deliberate, tested design (`instance_warper_test.py`), not an exploit.
- **Friday Dungeon enchant** clears random options, as its own dialogue warns.
- **Illusion / Horror Toy slot choices** cannot reach physical card slots with the current item database.
- **Restart-safe returning checks** at Tomb of Remorse, Airship Crash and Hall of Life are bounded by the active window and post-menu roster checks. Their gate bodies are pinned by `instance_entry_native_test.py`.

## Exploitation review (production logs, read-only)

Production had 3 characters and none online. `picklog` (all items, August 25 – October 6) and `mvplog` show:

- no Maristella vouchers;
- no Oscar antiquity boxes;
- no Kafra lottery prizes;
- no Snow Flower losses at Vellgunde;
- no Dylan resets;
- no Episode 21 field MVP kills;
- no Alitea characters.

The only Hall Key record is a single administrative grant. No player data needed correction.

## Pre-existing gate state (unchanged by this audit)

- `tools/audit_episode_integrity.ps1` reports the same 41 errors on HEAD and on this tree (Alice and Bioresearch mob IDs, missing Alice mapflags, Episode 20 mob-skill drift, client metadata).
- `gimli_checkpoint_reentry_test.py` pins a `GimliInfiltration.txt` hash that already differs at HEAD. It isn't in the release gate.
- Several hash-pinned tests fail only in a Windows checkout with CRLF working copies; the Linux release gate is authoritative.

## Deployment

Both releases are live and pushed to `improvements-20261002`. Each passed all
105 full native release checks in an isolated candidate, then went through the
guarded map-only cutover with zero players online.

- The critical, high, medium and low repairs went live as `f1eacde18`. See the
  [deployment record](npc_script_audit_deployment_20261008.md).
- The follow-up repairs went live as `957f42ea2`. See the
  [follow-up record](npc_script_audit_followup_deployment_20261008.md).
