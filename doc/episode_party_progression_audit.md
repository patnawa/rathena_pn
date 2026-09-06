# Episode 20/21 shared progression audit - 2026-09-06

This bounded pass fixes three script-level progression defects. It is not an
end-to-end instance playthrough or proof that every episode encounter works.

## Defects and changes

| NPC | Previous failure | Repair |
| --- | --- | --- |
| Canyon Passage (Episode 20) | Disabled itself after moving the solo character to map 2. Re-entering the still-live instance returns to the database entrance on map 1, leaving no usable passage. | Keep the passage usable during stages 2-4. Spawn the second-area wave only when the shared stage is still 2 after the dialog closes. |
| Dimensional Tear (Episode 20) | Disabled itself after moving only its interacting player to the boss map. Other party members and reconnecting players could not cross. Two already-open dialogs could also spawn two bosses. | Keep the passage usable during stages 2-4. Recheck and advance stage 2 after the dialog closes, before the one-time boss spawn. |
| Giant Egg (Episode 21 Secret Altar) | First interaction advanced shared stage 8 to 9 and disabled the reward NPC. Other eligible party members lost their quest advancement/reward, including players making room after a capacity rejection. Even an ineligible visitor could lock the reward. | Keep shared stage 8 and the NPC enabled. The existing per-character quest 18352 -> 18353 transition remains the one-time reward guard. |

Canyon Exploration remains an intentionally solo instance. Its entry policy is
defined in `EP20_EnterInstance` in `npc/custom/episode20/Progression.txt`.
Separated Sanctuary and Secret Altar retain their existing party/solo entry
policies. No entry gates, instance lifetimes, idle timeouts, cooldowns, prices,
reward quantities, spawn definitions, or destination coordinates were changed.

The return-to-first-map behavior is visible in `src/map/instance.cpp`:
`instance_enter` substitutes the instance database's entrance coordinates for
negative coordinates and always resolves the database entrance map. The Episode
20 entry helper passes `-1,-1`. The two cross-map entrances are defined in
`db/import/episode20_instance_db.yml`.

## Regression evidence

From the repository root:

```sh
python3 tools/ci/episode_party_progression_test.py -v
```

Eight tests pass against the changed source. The test reads the selected NPC
bodies from the enabled script files and uses a deliberately restricted,
fail-closed interpreter with explicit map/quest/inventory/dialog test doubles.
It exercises early/wrong-instance rejection, repeated crossing during and after
combat, overlapping dialog confirmations without duplicate spawns, per-character
reward idempotency, full-inventory retry after another member claims, and an
ineligible visitor not locking the party reward.

The repository integrity audit (`tools/audit_episode_integrity.ps1 -StrictContent`)
also passed with zero errors/warnings at this pass's checkpoint: 899 enabled
scripts, 99 instance definitions, and 51 checked arrival cells. This was a shared
working tree with other agents' class-support work in progress, not a frozen
production-release validation. The two NPC diffs pass `git diff --check`.

The same tests against the pre-fix revision reproduce seven failing assertions
(including both portal variants), with no interpreter errors:

```sh
python3 tools/ci/episode_party_progression_test.py --source-ref 4434b57de
```

These are source-driven simulations, not the rAthena script VM. Real map-server
parsing, two-client interactions, reconnects, collision
paths, actual capacity accounting, combat, reward persistence, and other instance
stages still require integration tests. This pass does not establish re-entry
coverage for every other Episode 20/21 portal.
