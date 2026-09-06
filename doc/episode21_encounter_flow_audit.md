# Episode 21 encounter flow audit - 2026-09-06

This bounded pass covers the enabled `GimliInfiltration.txt`,
`MysteriousGhostShip.txt`, and `BlackHairedBeast.txt`. It does not claim a full
Episode 21 playthrough. The earlier `episode_party_progression_audit.md` covered
Episode 20 passages and Secret Altar's party reward lock, not these encounters.
Existing campaign-completion compatibility helpers are retained.

## Proven defects repaired

| Failure | Narrow repair |
| --- | --- |
| Tris's arrival branch accepted completed quest 16818. Completion leaves that quest in the log, so every later conversation returned from this branch before processing quests 23253, 23255, or 18345. Once `changequest` removed 23249, the same branch could re-add it. | Test `isbegin_quest(16818) == 1`. The existing no-later-progress compatibility branch still starts 23249 for a legacy completed Ghost Ship character. |
| Four Ghost Ship internal passages warped only the interacting character, advanced the shared stage, and disabled themselves. Another party member could not follow; re-entry also returned to the original deck with its forward passage disabled. | Keep unlocked passages enabled. Only the first crossing advances the stage and triggers the next encounter/NPC. Every later crossing uses the original destination without changing progress or restarting combat. |
| Two members could both open a shared progression dialogue before either acknowledged `close2`. A late acknowledgement could overwrite a newer shared stage, repeat a relocation, or spawn another captain. | Recheck the expected shared stage after `close2` in 25 dialogues: 15 Gimli and 10 Ghost Ship. A stale continuation terminates before its shared side effects. |
| Four Gimli wave-start dialogues deliberately leave the stage unchanged during combat. Thus a post-dialogue stage check alone cannot stop two already-open conversations from requesting the same wave. The controller previously accepted both events. | Add the instance-local `gimli_wave_stage` latch to all six initial Gimli wave-start labels. Set the latch before any spawn request. The controller's existing death-label stages and all spawn definitions remain unchanged. |

All monster IDs/counts/locations, reward quantities, capacity checks, reputation,
experience, entrance policies, mapflags, and existing travel destinations are
unchanged. No official quest behavior, economy, acquisition, or balance has been
inferred. No database, engine, client, shared audit, or unrelated NPC was edited.

## Engine and dependency evidence

- `npc/scripts_custom.conf` enables each of the three files exactly once.
  `db/import/instance_db.yml` defines Gimli as instance 147, entering
  `1@mdtem,266,174` with additional map `2@mdtem`; Ghost Ship is instance 148,
  entering `1@wtgs,86,306`. Both retain their 3,600-second lifetime.
- `src/map/instance.cpp::instance_enter` resolves the database entrance map and
  substitutes the entrance coordinates for `-1,-1`. Both entrance helpers use
  those arguments. A successful re-entry does not restore a previous deck or room.
- `src/map/script.cpp::buildin_close2` changes the real VM to `STOP`.
  `src/map/npc.cpp::npc_scriptcont` changes that state to `RUN` after the close
  acknowledgement. No shared-stage reservation occurs automatically during the
  pause. `npc_enable_target` hides/disables an NPC but does not cancel all other
  players' already-suspended script states.
- `src/map/script.cpp::buildin_isbegin_quest` converts an absent quest to 0 but
  preserves the completed value 2. `quest_update_status` keeps completed quests
  in the log; `quest_change` replaces the old active quest with the new one.
  The actual native quest functions, not a Python quest model, reproduce Tris's
  original interception and the repaired transitions.
- The relevant quest identities are present in `db/import/quest_db.yml`:
  16818 is Return to Lunaforma; 23249-23255 are Black-Haired Beast; Gimli and
  Ghost Ship's per-character finish transitions retain their existing IDs.
- `instance_warpall` iterates the currently present instance maps. It is not a
  persistent relocation for offline or absent members. `mobcount` counts living
  matching monsters; existing last-monster stage checks were retained.

## Native regression

From the repository root under WSL, after a local Linux map-server build:

```sh
python3 tools/ci/episode21_encounter_flow_test.py --build-dir ../episode21-flow-native-20260906
```

The test freshly compiles `script.cpp`, `quest.cpp`, `malloc.cpp`, and its generated
C++ harness with AddressSanitizer and UndefinedBehaviorSanitizer. Existing support
objects satisfy linking only; normal server startup is not called. The harness
extracts 38 production NPC/event bodies without rewriting their statements.

The repaired source passes **43 cases, 252 assertions, zero failures/errors**, with
no memory leaks reported:

- 21 shared stage-changing dialogues with two actual suspended VM states;
- four additional same-stage Gimli wave dialogues held until after combat;
- six Gimli initial-wave labels, including wrong-stage rejection, exact original
  monster counts, and repeated same-stage event rejection;
- four Ghost Ship passages, including premature access rejection and repeated
  crossings at every subsequent stage through 22;
- five Tris scenarios covering the first arrival/report, later report branches,
  legacy entry, capacity rejection, and reward idempotency;
- three per-character finish variants, each exercised by two members and retried:
  Gimli story, Ghost Ship story, and Ghost Ship daily.

The runner also verifies that existing spawn, travel, and reward declarations
match the original source. Its `--pre-fix` option reverses only this pass in
memory and requires an exact match to all three recorded original file hashes.
It does not read Git history or rewrite the worktree. To reproduce the historical
failure using the already freshly built executable:

```sh
python3 tools/ci/episode21_encounter_flow_test.py --pre-fix --prepare-only --build-dir ../episode21-flow-before-20260906
../episode21-flow-native-20260906/episode21_encounter_flow_test ../episode21-flow-before-20260906
```

The last command intentionally exits 1: **43 cases, 218 assertions, 89 failures,
zero parser/quest errors**. The original passages never issue subsequent movement
requests, so 34 conditional destination-field assertions are not reached. This
accounts for the assertion-count difference; it is not a sanitizer failure.
Running `--pre-fix` without `--prepare-only` instead performs a fresh negative build.

The shared `tools/audit_episode_integrity.ps1 -StrictContent` run reached the
episode route/content checks with zero warnings. At this concurrent checkpoint it
failed solely because the separately owned, in-progress Shadow 166 fragments
`druid_shadow166_combos.yml`, `druid_shadow166_enchants.yml`, and
`druid_shadow166_items.yml` were not yet imported. This is **not** recorded as a
clean whole-repository audit, and those imports were not changed by this pass.

## Exact source checkpoints (SHA-256)

| File | Before | Repaired |
| --- | --- | --- |
| GimliInfiltration.txt | `04ebc9fcc713b20ebaa778f47c1840182525cf6fd97505ca9d04c6386f221edf` | `f0c554a83ae877ea0fcd8e76cddda5fb987b7363c47fec26874c6cb499245ff6` |
| MysteriousGhostShip.txt | `32036b3d303e75f6b3f34474ed9c5fa416387b5a6687f7d73bd43e8e7a788e0a` | `24bbfe0d1b6351c396ccd3477e7e29c343948afc5efa90f5b6808aa89fe14a4c` |
| BlackHairedBeast.txt | `cde09b114a9eda4c2f055298155ef4a4233b7169b6a8c97245f738091e6ce152` | `40e9f25f960a1cb3b73a7c757ba63167b65d38d7830ceb442fef758feb76e151` |

Fresh core source hashes: `script.cpp`
`6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d`,
`quest.cpp` `e4fb9b081712da9be44c39bb6f0e22e075ffd21d6fdfdd4f42f3433a1de8a684`,
and `malloc.cpp`
`064496e9722eeb1486178a6663aaa375ed7f312717986b9f41df3b69fd3a4a70`.
Generated build artifacts include a native-source fingerprint. `--reuse-build`
fails unless those source hashes and fixture metadata are unchanged.

## Boundaries and remaining work

The parser, VM control flow, `close`/`close2`, instance register reads/writes, and
quest commands are native. Quest DB entries are minimal identity fixtures checked
against the actual import, not a complete DB-loader/objective/timing test. Quest
packets and character-registry persistence are explicit doubles, with character
saving disabled. Movement, instance-map/NPC-name lookup, NPC visibility, spawn and
event delivery, capacity, items, experience, reputation and access helpers are
explicit recorders. Event-label bodies run separately; the real monster death
dispatcher and event queue are not simulated as successful integration.

The only rebound builtin entries are enumerated in the test; the native table
declaration is checked against the current engine before compilation. The harness
uses two isolated in-memory players and a real instance register container, not
live accounts. Kernel seccomp rules deny socket, connect, bind, and listen, and
the denial is tested. Actual `npc_scriptcont` proximity checks, packets, collision
paths, instance creation/destruction, monster combat, and in-game UI remain outside
the proof. This pass did not deploy or use SSH/Git.

Further findings deliberately not changed:

- **Gimli checkpoint re-entry remains incomplete.** Its party-wide scripted
  relocations protect currently present members, but its first cross-map portal
  disables at stage 8. Re-entering on `1@mdtem` during stages 8-12 cannot use that
  passage; later room/map relocations have the same absent-member limitation. A
  separate stage-aware checkpoint/route pass is needed. This patch does not claim
  general Gimli reconnect recovery and does not invent new warp destinations.
- **A cleared Ghost Ship can remain reusable.** The shared entrance helper reuses
  an existing matching instance, while the finish NPC accepts a newly active daily
  quest at stage 22. Thus a story-to-daily or reset-boundary reuse can reach the
  daily completion branch without a new battle. Deciding fresh-run ownership and
  party-safe reset policy needs a separate pass; immediately destroying a cleared
  instance would strand other members. No cooldown/reward policy was changed here.
- These scripts have no NPC timers, sleeps, or custom instance-destruction hooks.
  Cleanup is delegated to the existing instance lifetime/idle timer machinery:
  `instance_destroy` removes instance maps/NPCs, cancels timers, and destroys its
  register DB. That engine path was traced read-only, not exercised by this test.
- Characters already carrying duplicate legacy repair quests due to the old Tris
  branch are not automatically migrated or compensated. Ordinary progression now
  advances, but historical quest-log repair requires separately reviewed rules.
