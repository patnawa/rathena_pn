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

## Enchantment protocol follow-up

The separate guaranteed-upgrade request and recipe path is now implemented. All
776 ordinary/guaranteed upgrade recipes in the 18 restored workshop groups match
the active client data. The server import restores 444 guaranteed recipes across
loaded groups, including the 20 crown recipes previously omitted. Exact selector
tests, sanitized tests, production-image compilation, and isolated startup pass.
See [protocol evidence and limits](enchant_upgrade_protocol.md).

## Item identity and crown follow-up

- Numeric IDs from the active client's 5,208-entry `ItemDBNameTbl.lub` resolve 85
  localized aliases. This explains 266 of the former 338 recipe differences.
  There are no unresolved names in the compared upgrade recipes/materials.
- The remaining 72 differences were in Biosphere crown group 132: 32 incorrect
  deterministic recipes and 40 missing higher-level recipes. The native window
  now uses the same weighted level 1–10 progression and rune costs as the client
  and the existing Abyss Researcher script menu. Failure retains level 1 or
  downgrades one level; it is not a guaranteed upgrade.
- All 1,245 ordinary and 444 guaranteed client upgrade recipes in 162 loaded
  groups now compare exactly. All 24 additional server-only recipes remain
  unchanged (16 stat upgrades and 8 Fierce Attack/Great Craftsman upgrades).
  Group 132's target items, reset, order, and normal enchants are unchanged;
  all other groups in the modified import are unchanged.
- The comparison now models material overlay/zero-removal, omitted amounts and
  prices, and mutually exclusive deterministic/random replacements. There are
  23 synthetic regression tests for these rules, non-executing Lua-table reading,
  alias resolution, and safe/idempotent recipe generation.
- The restored workshop/Tina/healer scripts match the running Docker server.
  The audit checks 11 service approach cells in addition to 40 episode arrivals,
  including routes from `grademk,38,177` to the nine counter services. The client
  Grademk GAT agrees that the approach aisle at row 181 is walkable; portions of
  row 183 are the counter, not player standing cells.
- `admin`'s character `MSCESXi` still has all 15 saved reputation/mirror variables
  at their configured maxima. No account values were changed in this follow-up.
- RockMMO and MuhRO references were inventoried. Their tested GRF payloads are
  not standard zlib; no protected assets were imported. See the
  [reference inventory and comparison command](client_reference_grfs.md).

## Druid and parallel audit follow-up

The user authorized Druid/Karnos/Alitea integration and parallel agents. The
reviewed PR #9765 delta adds 84 skills and five job variants, with local fixes
for Alitea traits, transformed job changes, enhanced-skill bonuses/factory
identity, and Baby Karnos's cap. A custom Druid Mentor provides the three normal
job transitions at `prontera,153,193`; this is not the official Veledor quest.
All 366 Fashion pairs are now enabled. Existing live-only `@go` help additions
and `MF_MD_SELFDESTRUCTION` were identified during drift checks and preserved.
See [core integration](druid_integration.md) and
[client/progression evidence](druid_client_progression.md).

The parallel episode pass fixes cross-map re-entry/party access in two Episode
20 instances and the shared first-claim reward lock in Secret Altar. Eight
source-driven regression tests pass. Four Chapter 2 cards now inspect their own
host weapon level, and Odium's spawn buff has a reachable state; six combat
tests and 13 sanitized source-extracted assertions pass. See
[party progression](episode_party_progression_audit.md) and
[combat bindings](combat_bindings_audit.md).

The broader local audit now reports 899 enabled scripts, 99 instances, 29,535
item identities, 100 DB imports, 61 local fragments and 54 walkable arrival/service
cells, with zero integrity warnings. The additional 31 Druid-related item
definitions have verified source IDs and supplied-MuhRO compatibility effects;
63 initial-enchant recipes and the Gray Wolf distribution are now wired in a
separate Renewal overlay. All 31 identities already exist in the original client
name table; previous unresolved results meant missing server definitions.
Material acquisition and client interaction remain incomplete. See
[item provenance and limits](druid_item_compatibility.md) and
[recipe checks](druid_item_enchant_compatibility.md).

These remain structural, source-driven and isolated build/startup checks.
Attached-player combat, actual client clicks, relogging and official balance
remain unproven. The broad audit is still in progress.

The next Druid batch adds a live owned-Monolith/range check for Nova and Stomp,
with 43 source-compiled ASan/UBSan regression checks. An isolated real-script-VM
inventory test now passes 13 builtin cases and 21 native assertions; player
lookup, logs and client delivery are explicit test boundaries, with networking
denied by the kernel. This is not an actual client or persistence test. See
[Monolith checks](druid_monolith_runtime_audit.md) and
[native VM limits](../tools/ci/native_script_vm_README.md).

The combined changes were deployed with matching login/char/map/web binaries
on 2026-09-06 at 14:34 ICT, with no players online. All four containers are
running and the map/char connection is online. See the
[deployment receipt and backup details](druid_deployment_20260906.md).
The six-file recipe/Monolith follow-up was deployed at 14:56 ICT; all four core
containers are ready with zero restart counts. See
[follow-up receipt](druid_recipes_monolith_deployment_20260906.md).

The allocator/Shadow follow-up deployed at 15:18 ICT. The default allocator now
provides correctly aligned pooled/large payloads, reads and writes odd-sized tail
guards safely, and verifies the final large-allocation byte correctly. Fresh
normal/debug ASan+UBSan matrices pass 18774 assertions each, and deliberate
shutdown cleanup passes 105 assertions each. The real VM now recompiles the
allocator too and passes with both sanitizers. See
[allocator evidence](native_allocator_alignment_audit.md) and
[deployment receipt](allocator_shadow_deployment_20260906.md).

Group 128 is now imported once and available through the Shadow Gear Enchanter
at `grademk,40,184`. Its 14 existing targets, 12 initial recipes and 24 weighted
upgrades match the original client, with 2.4 million exact native-selector draws.
No equipment/material economy was invented. The map approach, NPC wiring and
native startup pass; actual client interaction/charging remains unverified.

## Initial-enchant and remaining gameplay coverage

The normal-enchant sampler, grade bonus parser/application, and disabled-reset
charging order have now been corrected and covered by 39 compiled behavioral
checks plus 7 new configuration-comparison tests. Explicit zero success/bonus/
outcome/reset rates now parse correctly. Exhaustive testing of all 339 effective
server distributions passes 33.9 million draws with exact declared frequencies.
See the
[initial-enchant/reset audit](enchant_probability_audit.md) for evidence and limits.

- End-to-end client clicks, cost charging, relog persistence, and equipment-effect
  verification remain pending for the new guaranteed-upgrade path. Packet evidence
  comes from the bundled 2025 binary; the active 2026 client needs a runtime check.
- The current initial-enchant/reset comparison covers 158 shared groups, 339 normal
  grade tables and 2319 selectable initial recipes. After the Druid overlay it
  reports 25 shared-group differences: two Gear_AT recipes, 13 equipment target
  lists, and ten deliberately customized Biosphere distributions. The audit now
  reports missing server group 166, for 26 total issues. That group's 18 additional
  selectable recipes bring the full client total to 2337. With group 128 restored,
  all 1269 ordinary and 444 guaranteed client upgrades match, with no missing
  upgrade groups or unresolved upgrade dependencies.
  All 40 unresolved identities are present in the client but missing on the
  server; none is a missing client name. No item IDs/effects have been
  guessed. Compared reset settings and existing initial-recipe costs match.
  The 24 server-only upgrades remain preserved, not proven visible in the client.
  Client clicks and actual material charging remain unverified for corrected
  group 132 native recipes and the corrected normal/reset handlers.
- Chapter 2 groups 167-171 have no definitions in any of the supplied client's
  three enchant-list copies. Their 22 targets/58 outcomes also require 84 missing
  client name mappings. The existing itemInfo metadata does not register native
  enchant tables. A separate offline client-patch generator is being prepared;
  it is not installed and those windows are not yet repaired.
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
