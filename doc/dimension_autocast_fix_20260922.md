# Time Dimensions autocast repair — 22 September 2026

The server repair was deployed to the development server at 12:33:25 UTC
(19:33 Bangkok). Startup and health checks passed, the running map executable
matches the verified candidate, and all 34 checked financial/storage groups
were unchanged. No database restore or schema migration was performed.

The Elemental Master equipment bug was reproduced in the production script VM:
equipping a grade-A Time Dimensions crown and book before summoning registered
zero Elemental Buster autocasts. Summoning afterward did not rebuild that bonus.
The same defect affected Elemental Spirits.

## Changes

| Area | Repair |
|---|---|
| EM books 540079/540080 with crown 400536 | Register eligible learned-skill bonuses regardless of summon presence during equipment calculation. At autocast dispatch, require a currently summoned advanced elemental before consuming resources. Summoning, replacing, or dismissing an elemental no longer leaves a stale equipment decision. |
| Elemental Spirits 540114 | Apply the same runtime summon condition. Retain grade A, +12, and learned Buster level 10 requirements and both Diamond Storm/Terra Drive triggers. |
| Shared on-skill autocast range | Measure from the caster to the autocast target. Ground triggers supply no original target, which previously prevented self-centered Buster when range checking was enabled. |
| Shared ground autocast limits | Check the autocast skill's unit limit, rather than the triggering skill's limit. This includes Soul Ascetic's Black Tortoise follow-up. |
| Meister Dimensions Axe 620037 | Remove the equipment-time Mado exclusion. Both Mighty Smash and Axe Tornado explicitly allow Mado; the official item has no mount exclusion. |
| Troubadour/Trouvere 570062/580061 | Correct the grade-A Reverberation set bonus to twice the combined weapon/crown refine, in addition to the existing 45%. |
| Client descriptions | Correct misleading learned-level text and confirmed trigger/skill-label errors through a narrow item-description override. See the other-jobs audit for the coverage and primary sources. |

The server retains learned-skill and grade requirements, proc rates, skill levels,
target selection, and recursion locks. Buster remains centered on the caster;
this change does not extend its damage radius to the targeted ground location.

Primary specifications: [EM book](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=540079&itemSeq=1),
[Meister axe](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620037&itemSeq=1),
[performer violin](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=570062&itemSeq=1),
and [performer ribbon](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=580061&itemSeq=1).

## Regression evidence

The equipment fixture runs actual item/combo scripts in the native VM, calls
production bonus registration, and reads the resulting autocast and skill-damage
bonuses. It covers summon presence, all five advanced elemental identities,
learning, separate crown/weapon grades, refine boundaries, both exact Elemental
Spirits triggers, performer weapons, and mounted/unmounted Meister.

- Final local equipment run: **576 cases / 3,838 assertions**, GCC ASan/UBSan,
  clean native allocator teardown.
- Original live EM scripts: expected one registered Buster, actual zero after
  equipping without a summon.
- Original performer script at weapon +12/crown +11: expected 91%, actual 68%.
- Original mounted Meister script: expected one registered Axe Tornado, actual zero.
- Runtime function regression: **110 assertions / zero failures**. The captured
  original live source fails 37 assertions. Each of the three independently
  reintroduced runtime defects also fails.

The runtime fixture compiles production ground-cast dispatch, on-skill autocast,
cast-type selection, bonus registration, range checking, Buster, Diamond Storm,
and Terra Drive functions. World geometry, resource debit and final damage
delivery are explicit test boundaries. It verifies dispatch and guard behavior;
it is not a rendered gameplay or numerical damage benchmark.

Run under Linux/WSL from the repository:

```sh
python3 tools/ci/dimension_autocast_runtime_test.py
python3 tools/ci/dimension_equipment_test.py --build-dir ../dimension-equipment-proof
python3 tools/ci/bug_hunt.py --output ../dimension-combat-checks --area combat
```

The equipment fixture needs existing Linux map support objects. Captured original
database files can be replayed with `--before <live-before-directory>`; that run
must fail. The fast runtime test is integrated into ordinary release/combat
checks; the full VM fixture is integrated into native combat checks.

## Deployment evidence

Operational artifacts are retained outside the source repository in
`Server-Development/dimension-autocast-20260922`. The candidate is built from a
copy of live server inputs, preserving the live import-combo differences, with
zero reused objects in the pinned runtime image. Isolated startup, candidate
regressions, exact preimages, backups, and post-install identities are checked
by the deployment controller. Final installation status is recorded in that
directory's `REPORT.md` and deployment receipts.

The production build used the pinned Alpine image. Its sanitizer library could
not link the native fixture, so the final candidate regression combined the
Docker host's isolated runtime-function test with the working WSL VM fixture.
All 366 candidate database files were copied and hash-verified, including live
imports. Relevant compiled sources and headers were compared with the candidate;
newline/comment differences and an unexercised NPC skill difference are recorded
in the equivalence receipt. Both original failures and the environment failures
are retained. The final candidate VM run passed 576 cases / 3,838 assertions.
