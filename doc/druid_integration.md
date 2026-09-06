# Druid / Karnos / Alitea integration

## Provenance and scope

The engine/database implementation is the GPL-licensed rAthena
[Druid pull request #9765](https://github.com/rathena/rathena/pull/9765), pinned to
`92224e78e5f480c648363f07a908ba747436cdff` (2026-08-29). Its exact delta against
`e985006171d2eb320ee512a653f4c83aea3d81b6` was integrated into this project; the
unrelated upstream branch was not merged. The upstream pull request remains
open, so this is a reviewed compatibility implementation, not a claim of
officially complete behavior.

Core coverage:

- Client jobs 4351..4355, both directions of map/job conversion, job names,
  command aliases, script constants, class stat/ASPD rules and equipment masks.
- 84 skill identities (6524..6607): 74 factory-backed active/internal skills,
  nine passive calculation hooks and the internal Flip Flap target entry.
- Skill prerequisites/inheritance for Druid, Karnos, Alitea and baby variants;
  transformations, status effects, charging/combo states, ground units and
  continuous casting support.
- 398 job flags on 205 existing equipment definitions: 193 Druid flags and
  205 Karnos flags. Existing item IDs, scripts and unrelated fields are retained.

Local Garden/Lockon mechanics and all enchant validation/probability/metadata
fixes are preserved. Druid statuses are appended after existing custom statuses
so saved custom status numeric identities do not shift.

## Corrections beyond the reference

1. The upstream enhanced Roaring Piercer factory returned `SkillRoaringPiercer`,
   carrying the ordinary skill ID and damage ratio. `AT_ROARING_PIERCER_S` now
   returns `SkillRoaringPiercerS`. The factory identity audit detects this error.
2. Baby Karnos was mistakenly assigned the fourth-job level-275 experience
   group. Its cap is normalized to level 200, matching normal Karnos; this is
   a server compatibility policy rather than a claim of official baby-class data.
3. The skill-array capacity is raised from this project's 1700 to 1800, covering
   the effective database's 1733 skills without exceeding the shared array.
4. The reference did not add Alitea to the trait-job predicate. It is now
   recognized as an expanded trait job, enabling trait point awards, allocation,
   reset and stat limits. Karnos is also classified as expanded-first. Native
   compile-time assertions catch the original missing classification.
5. Same-lineage job changes now end both Druid transformations before assigning
   the new class/body, preventing their saved old body style from being restored
   after Karnos becomes Alitea. Alitea also skips the lower-stage extended stat
   cap branch and uses the configured trait-era normal-stat cap.
6. All six enhanced `_S` attacks map to their base skill for equipment bonuses,
   skill-damage adjustments and copy references. Apex Phase's Quill Spear bonus
   uses that same normalized base key. The enhanced implementations retain
   their own IDs, damage formulas, status/charge consumption and packets.
   `skill_dummy2skill_id` is not called by cost, cooldown or logging paths.
   Its other callers are heal/damage bonus lookups and Plagiarism/Reproduce;
   these Druid base/variant records do not enable either copying flag.

| Job | Base cap | Job cap |
| --- | ---: | ---: |
| Druid / Baby Druid | 99 | 70 |
| Karnos / Baby Karnos | 200 | 70 |
| Alitea | 275 | 60 |

## Deployment requirement

`MAX_SKILL` changes `mmo_charstatus`, a shared inter-server structure. Build and
deploy compatible **char and map binaries together**; rebuilding all core
binaries is safest. Do not deploy only a new map binary against an old char
server. Check no players are online, back up the prior binaries and scoped
files, then restart the coordinated core services. Candidate parser/startup
testing must inspect logs because `--run-once` can return zero after DB errors.

## Reproducible checks

Run from the repository root with Python 3 and PyYAML:

```sh
python3 tools/ci/audit_druid_integration.py --compare-ref 4434b57de89469fc90a69da75e2e0298cc1fe6e2
python3 tools/ci/audit_druid_integration_test.py
g++ -std=c++17 -O2 -ffunction-sections -Wl,--gc-sections -DPACKETVER=20260219 -Isrc -I3rdparty/rapidyaml/src -I3rdparty/rapidyaml/ext/c4core/src -I3rdparty/libconfig -I/usr/include/mysql tools/ci/druid_identity_test.cpp -o /tmp/druid_identity_test
/tmp/druid_identity_test
python3 tools/ci/audit_druid_integration.py --emit-alias-test | g++ -std=c++17 -O2 -ffunction-sections -Wl,--gc-sections -DPACKETVER=20260219 -Isrc -I3rdparty/rapidyaml/src -I3rdparty/rapidyaml/ext/c4core/src -I3rdparty/libconfig -I/usr/include/mysql -x c++ - -o /tmp/druid_alias_test
/tmp/druid_alias_test
```

The structural audit checks all 84 enum/database identities, factory constructor
identity, active/passive coverage, skill status references, inherited skill-tree
requirements, effective job caps, total skill capacity and scoped equipment
preservation. It does not simulate combat or a player session.

## Remaining verification limits

The reference intentionally marks HP/SP progression, weight/stat constants,
Quill Spear CON scaling, precise Monolith teleport position/range, continuous
cast task indicators and transformation display workarounds as uncertain or
unfinished. These TODOs are retained; no replacement values are invented.

Compilation and startup cannot establish correct client transformation sprites,
hotkeys, skill animations, damage/cost timing, reconnect persistence or official
balance. Those still require player-attached testing on the active 2026-02-19
client. Client data and class progression NPC validation are separate from the
core engine checks described here.
