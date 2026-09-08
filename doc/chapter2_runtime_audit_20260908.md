# Chapter 2 progression and encounter audit — 2026-09-08

Reviewed `npc/custom/chapter2/Chapter2.txt` and `Instances.txt`, their quest and
instance definitions, and the native status/skill paths used by Phantom circles.
The changes preserve quest objectives, prices, reward quantities and cooldowns.

## Fixed defects

1. The Phantom Gate cleared `CH2_DailyPhantom` before the reward helper checked
   inventory capacity. A full inventory therefore lost earned clear credit.
   The helper now checks active quest/clear eligibility and capacity before
   completing the quest and resetting its clear flag. Both gate and board use
   that single commit path, so failed capacity checks can be retried.
2. The Phantom's 90-second exit grace allowed one character to claim a story
   clear, finish the story, accept the daily and reenter the same cleared run
   for daily credit. An instance-local claimed-character list now permits at
   most one successful credit per original roster member per run. Ineligible
   visitors do not consume a claim; another original member can still return
   and claim during the existing grace period. Delimited character IDs prevent
   substring matches such as character 1 matching character 11.
3. The circle cast `NPC_GROGGY_ON` at level 5, whose configured status lasts only
   five seconds. Native `unit_skilluse_id2` also refuses a new skill while the
   boss is already casting, allowing an existing cast to continue. The circle
   now directly applies the forced 10-second `SC_GROGGY` status. Existing native
   status flags stop movement, attacks and the current cast. The ten timer ticks
   continue to restore original mode, 1% damage intake and the reset position.
4. Guardian El checked final reward capacity before a `next` dialog yield.
   Capacity and already-completed status are now checked after that yield,
   immediately before the completion/reward commit.

## Checks and unchanged behavior

- The mission menu already correctly contained ten missions and Cancel.
  Phantom is option 10 / quest 27119; Cancel is option 11. No menu mapping bug
  exists and no mapping change was made.
- Reservations are party-owned; only the leader creates them. The roster is
  captured at creation, and party/reservation/roster identity is revalidated
  after entry dialogs. Late recruits are excluded. Existing original members
  may enter independently, including during the clear grace period.
- Crossroads requires four four-monster waves before Sollith. Nyrholt/Phantom
  requires four two-monster gates before the boss. Clear labels have guards,
  and the exit remains available for retreat before victory.
- Phantom's active timer resets each second. Cleared instances switch to the
  90-second exit timer. The one-second label does not restart combat for a
  cleared stage, non-Phantom mode or absent boss.
- Repeatable reward cooldown remains four hours from successful turn-in.
- Instance item grants retain the existing mail fallback for full inventories.
  This review does not claim to verify mail delivery/SQL success.

## Regression evidence and limits

`python3 tools/ci/chapter2_progression_test.py` passes eight source-driven tests:
capacity refusal/retry and exactly-once daily commit; the actual Phantom Gate
full-inventory path; final-story capacity changing during the dialog; story to
daily reuse of the same run; original-roster reentry and identifier delimiters;
both other story clear modes; the ten-tick circle/reset lifecycle; and all menu
quest IDs plus Cancel.

The tests interpret the actual selected script bodies with explicit quest,
inventory, instance and unit doubles. They fail on unsupported syntax/commands.
They are **not native VM or client combat tests**. They do not establish actual
mail transport, real inventory stacking, monster AI/pathfinding, party packet
delivery, map cells, disconnect timing or rendered skill effects. Fresh native
script loading and in-client playthrough remain integration checks. Map-cache
geometry and reference-content coverage are audited separately by the team.
