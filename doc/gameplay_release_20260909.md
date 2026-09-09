# Gameplay reliability release — 9 September 2026

This release fixes confirmed equipment, episode, and instance defects and adds daily database backup restoration checks. It follows the equipment progression and client navigation release from the same day.

## Equipment transactions

Reform now validates the target, tuning, ingredients and final weight before charging. It rejects switch-registered or stacked targets, binds delayed consumption to the opening item's inventory cell, and updates carried weight after transformation. Explicit NPC recipes do not accidentally consume a stale last-used item. Reform retains the existing inventory cell, so a full inventory does not require an additional empty slot.

Twenty-five source-extracted native handler cases pass under sanitizers. The existing refine suite also passes 110,000 outcomes and its pre-payment guards. Separate real login/character/map sessions against an isolated, empty database tested three TCP-reset cases: opening without confirmation, disconnect after acknowledged completion, and immediate disconnect after sending confirmation. Reconnection showed either unchanged inputs or the complete result, with exact material consumption and preserved refine, cards and unique identity. These tests do not establish durability through every possible server or host crash.

## Episode and instance progression

Thirteen shared dialogue windows in Secret Altar, Final Battle and Silent Sanctuary now recheck encounter state after client acknowledgements. This prevents stale dialogues from rewinding stages, repeating waves or re-enabling old interactions. Admitted returning players use unlocked checkpoints; the second altar door's explicit state prevents recovery beyond a still-disabled NPC. Recovery changes neither party-wide position nor quest/reward state.

OGH Challenge now distinguishes a character's admission to the current live instance from party ownership, blocks cooldown bypass, and preserves the original cooldown on re-entry. Fall of Glast Heim stores admission and reward claims per live instance, avoiding false matches when numeric instance IDs are reused after restart. Airship Crash and Tomb of Remorse recheck the starting character roster after entry menus.

Native VM validation passed 159 episode assertions and 29 instance checks. Removing the fixes reproduces the failures. Existing episode and Chapter 2 source-driven suites passed 16 tests. Six checkpoint cells are walkable; all 20 literal exit destinations reviewed across 11 active custom instance scripts fit their map bounds. World movement, inventory delivery and admission boundaries in native VM fixtures are explicit test doubles.

## Recovery and release checks

The combined release gate passed all 17 checks on a coherent Ubuntu build. The production-compatible Alpine build separately loaded the final active NPC graph successfully. Only the Alpine map executable is deployed. Deployment checks compare source hashes, preserve backups, require no online players, inspect fresh service logs and restore prior files if verification fails.

The database backup test restored all 118 tables in a networkless temporary MariaDB container, checked every table, and reproduced the original SQL SHA-256. Read-only checks found no orphaned character/account, inventory/character, quest/character, storage/account or attachment/mail references, invalid inventory amounts, negative zeny, or duplicate nonzero item identities across active inventory locations. Fresh production logs showed no server errors before this release. See [database recovery](database_recovery.md) for the daily timer and recovery procedure.

## Remaining client acceptance

Desktop automation was unavailable. These results do not claim a rendered playthrough of every episode or boss. Visual acceptance still needs the game client for:

- Party members overlapping finale dialogues and leaving/rejoining after each shared door or arena transition.
- Full-inventory reform and perfect Signet menus, including displayed requirements and outcomes.
- Valid versus excluded instance re-entry, cooldown displays, and once-only rewards.
- Aquila's visible warning/cast and a complete fight.
- Quest navigation clicks and complete quest chains, especially previously unavailable legacy content.

Raw validation evidence, synthetic-session fixtures, deployment receipts, and private database archives remain outside Git. The unrelated equipment-analysis work is not part of this release.
