# Episode audit status — 2026-09-06

The audit is **in progress**. A clean database/script load is not proof that every
episode, combat encounter, reward, or client enchant action works end to end.

## Verified in this follow-up

- `tools/audit_episode_integrity.ps1 -StrictContent` passes locally: 898 enabled
  scripts, 99 instance definitions, 29,504 item identities, 3,215 monster identities,
  and 1,696 custom monster-skill rows. These are integrity checks, not playthroughs.
- The map-server compiles locally and in the production Docker image. The isolated
  Docker candidate completes `map-server --run-once` without script/database errors
  or memory leaks (the existing root-user warning remains).
- `tools/ci/inventory_enchant_test.cpp` executes 40 assertions against the helper
  actually used by `modifyinventoryenchant`; it also passes AddressSanitizer and
  UndefinedBehaviorSanitizer. It checks both Star Signet enchant positions, full
  item-record preservation, maximum unsigned unique ID, stale card/identity
  rejection, physical-slot protection, and invalid equipped/unidentified/stacked
  items. It does not exercise client packets, logging, or script material charging.
- The Grade Workshop Star of Spell Lv3–5 service now uses an in-place, unique-ID
  and complete-card-snapshot guarded mutation instead of delete/recreate. Existing
  prices and material quantities are unchanged. Rejection does not charge costs.
- Temporal Tina now checks inventory capacity before selling tickets, rechecks the
  ticket after confirmation, and synchronizes Episode 19's final progress variable.
  Selecting an already-cleared Episode 19 also repairs the legacy missing variable
  without consuming a ticket. Episode 21 campaign completion is recognized by the
  Gimli and Ghost Ship access helpers. No historical rewards are granted.

## Known unresolved coverage

- The active client enchant data (`nebula_upgrade_v2.grf`'s `EnchantList.lub`) has
  separate `AddPerfectUpgradeEnchant` entries. The server currently handles ordinary
  upgrades via `CZ_REQUEST_UPGRADE_ENCHANT` (0x0b9d), with no separately verified
  newer perfect-upgrade request path. Converting a recipe to server `Upgrades`
  does not establish that the client's corresponding button sends this request.
- Crown group 164 has 90 weighted upgrade recipes. Its 20 guaranteed Lv6→7/Lv7→8
  recipes overlap existing source enchants and have not been wired as a distinct
  choice. Do not replace random recipes with guaranteed ones or silently change
  their costs. Protocol/client verification or an explicitly exposed equivalent
  service is still required.
- Parser fixtures in `npc/test/native_equip_safety.txt` only test accepted command
  syntax during a test-config load; they are not player-attached runtime tests.
- The restored healer and workshop need actual client interaction checks. The
  wider 99-instance corpus still needs encounter, party/solo, cooldown, failure,
  disconnect/re-entry, reward, and equipment-effect playthrough coverage.

## Reproduce the focused behavioral test

From the repository root on Linux (or WSL):

```sh
g++ -std=c++17 -Wall -Wextra -Isrc tools/ci/inventory_enchant_test.cpp -o /tmp/inventory_enchant_test
/tmp/inventory_enchant_test
```

Do not treat the audit's `PASS` or a successful `map-server --run-once` as a
completion claim for the full gameplay audit.
