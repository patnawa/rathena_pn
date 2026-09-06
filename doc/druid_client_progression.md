# Druid client and progression checks - 2026-09-06

The current 2026-02-19 client already contains matching class/skill data; no
RockMMO/MuhRO protected GRF resources were imported for this integration.
The active DATA.INI order is nebula_upgrade_v2, server, english, new, data.
The first two archives contain no matching job/skill tables and the corresponding
loose override paths are absent.

`tools/ci/audit_druid_client.py` checks explicitly extracted files without
executing Lua. The effective `new.grf` supplies the skill IDs, visible tree and
player-job names; `english.grf` supplies skill information/descriptions;
`data.grf` supplies job/NPC identities and NPC resource names.

- Job IDs 4351/4353/4355 match Druid/Karnos/Alitea.
- All 84 server skill IDs 6524..6607 match the active client.
- All 74 visible Druid-family skills have information and description entries.
  Nine internal variants also have information; the internal Flip Flap target
  marker is not a selectable client skill.
- The tool prints SHA-256 hashes for all input tables. The inheritance chunk
  contains metatable/function code, so it is recorded but deliberately not
  executed by the literal reader. Runtime inheritance remains unverified.
- Six synthetic tests verify explicit table references remain read-only and
  unknown globals, mutations, overwrites and calls are rejected. The existing
  23 item-alias/upgrade tests remain passing.

These checks do not prove character sprites, transformation rendering, skill
effects, packet interactions, or in-game item bonuses.

An additional base-GRF inventory finds 32 body SPR/ACT resources: male/female
Druid, Karnos and Alitea (normal and riding), plus Werewolf/Wereraptor forms.
The corresponding body paths have no overrides in english/new; the two smaller
higher-priority archives contain only their already inventoried unrelated files.
This establishes archive presence, not successful sprite decoding or rendering.

## Scoped job-change service

`Druid Mentor` at `prontera,153,193` offers only normal Novice -> Druid -> Karnos
-> Alitea. It does not enable the disabled general Job Master or convert existing
unrelated classes. Requirements are the published Gravity thresholds:
[Novice Job 10](https://ro.gnjoy.com/guide/runemidgarts/popup/jobChangeQuest.asp?jobgamecode=4351),
[Druid Base 99/Job 70](https://ro.gnjoy.com/guide/runemidgarts/popup/jobChangeQuest.asp?jobgamecode=4353),
and [Karnos Base 200/Job 70](https://ro.gnjoy.com/guide/runemidgarts/popup/jobChangeQuest.asp?jobgamecode=4355).

This is explicitly a **custom compatibility service**, not the official Veledor
quest implementation. It charges nothing and grants no quest rewards. It checks
class, levels, unspent skill points and normal/unmounted form before and after
the yielding confirmation, verifies the actual job-change result, and relies on
the engine's existing job-level reset, point handling and save path.

`druid_mentor_test.py` performs source-driven simulations of boundaries, invalid
classes, cancellation, stale state, forms/mounts and engine rejection. It also
checks all 366 effective Fashion stone/enchant pairs and their new skill
dependencies. The eight formerly withheld enchants now explicitly use
`SubType: Enchant`, are awarded/applied normally, and have updated client metadata.
These tests are not attached-player VM execution or relog proof.

## Reproduce

```sh
python3 tools/ci/audit_druid_client.py /path/to/audit-druid-client-20260906
python3 tools/ci/audit_druid_client_test.py
python3 tools/ci/druid_mentor_test.py
```

See [engine integration and remaining upstream TODOs](druid_integration.md).
