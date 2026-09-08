# Account loot profiles

Players can keep ten named snapshots of their current autoloot preferences:

```text
@autoloot 5
@alootid +501
@aloottype +6
@alootconfig save 1 Cards and potions
@alootconfig list
@alootset 1
@alootconfig delete 1
```

`@alc` aliases `@alootconfig`; `@als` aliases `@alootset`. The configuration command
also accepts `load <slot>` and `help`. Names are labels, with selection by slot
number 1–10. A save to an occupied slot replaces it. Names are limited to 32 plain
ASCII characters without client color codes.

Profiles contain percentage thresholds, item inclusion IDs, and item-type flags.
They are shared between characters on the same RO login account, through the
existing permanent account registry. They contain no items, currency or equipment.
Loading replaces all three current preferences and correctly reconstructs the
item-list activation flag, including for an empty profile.

The selected profile applies to the current session. Saving or loading does not
select a login default or change the separate Settings NPC defaults. This feature
does not implement exclusion lists, linked master accounts, pipe-separated item
commands, or the reference server's scoped suffix syntax.

## Storage and validation

`#PNALootSet$[1..10]` stores one versioned registry string per profile:
version, percentage in hundredths, type mask, item-array length, item IDs, name.
Version 1 uses the current `AUTOLOOTITEM_SIZE`. `pc_setregistry_str` is deliberately
used instead of temporary `pc_setregstr`, so values take the normal persistent
registry save path. Account registry availability is checked before any access.
As with other account variables, durability follows the server's normal registry
save cycle; the command does not claim a synchronous database commit.

Only one registry write is needed per save/delete. Loading parses and validates
the complete record before changing any active preference. Unknown versions,
out-of-range rates/types/IDs, duplicate IDs, unavailable items and malformed names
leave current preferences untouched. Slots outside 1–10 and surplus arguments
are rejected. No inventory or economy operation runs from either handler.

## Focused test

```sh
python3 tools/ci/aloot_sets_test.py
python3 tools/ci/player_command_permissions_test.py
```

The first compiles the unchanged production profile helper under ASan/UBSan with
explicit registry, item database and message-transport doubles. Cases cover
save/load/list/delete, shared-account access, different-account isolation,
unchanged preferences on errors, malformed/overflow input, corrupt/stale records,
empty profile restoration, unavailable account variables and failed writes.
It validates the handler and serialization contract, not a real SQL reconnect;
native build/startup and live registry persistence acceptance are separate checks.
