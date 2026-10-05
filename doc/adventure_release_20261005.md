# Adventure and compact hub release — 2026-10-05

Production code revision: `aae2d56011624fdba92bf8038025ddad91f6e292`.
Branch: `improvements-20261002`. Signed client release:
`client-20261005-adventures`, sequence **2026092908**.

## Player access

Update through PNLauncher before visiting the compact Office. Use `@office`
for one hall with 50 NPCs in five groups, and `@adventure` for session goals,
preparation, inventory tools, party recruitment, weekly objectives, boss
material credits, shared builds and lab challenges.

The Grade Workshop retains both existing arrivals and groups 12 services
around an open aisle. The Master Refiner is inside at **38,181**, beside
materials at **42,181**. The Workshop Guide at **30,175** marks service routes.
Existing upgrade prices, success rates, recipes and NPC identities remain.
See [workshop arrangement](grade_workshop.md), [Office guide](main_office.md)
and [adventure services](adventure_services.md).

## Verification

- Final frozen candidate: **91 full release checks passed**, including native
  regressions, database/client source checks and complete NPC startup against
  an isolated database.
- Workshop equipment menu: **14 paths, 890 assertions**, zero errors/failures,
  AddressSanitizer/UndefinedBehaviorSanitizer clean and no native memory leaks.
- Office travel/copy-skill regression: **27 cases, 208 assertions**, sanitizer
  clean and leak-free. Geometry verifies all 50 desks with NPC cells blocked.
- Workshop geometry: all 12 enabled services reachable from both arrivals,
  with NPC and exit-trigger cells blocked; six negative regressions passed.
- Preparation/inventory: **36 scenarios, 363 assertions**. Social/build/lab and
  Adventure routing: **40 cases, 1,210 assertions**.
- Weekly/Alice: **246 checks**; Bioresearch: **241 checks**. Final disposable
  SQL proof includes **8,277 point-asset checks**, including actual Alice and
  Bioresearch material purchase rollback, commit and replay.
- Windows signed updater: apply, readiness, exact rollback, replay protection,
  reapply and unchanged baseline files passed across **5,654 files**. The
  personal client was not modified; the production launcher was retained.

Evidence is retained outside source under `/app/pn-adventures-20261005`:
`full-gate-v3/report.json`, `sql-proof-v4/report.json`,
`evidence/grade-native-v3.log`, `evidence/office-native.log`,
`office-client-verification.json` and `final-validation.json`.
The full gate and SQL proof bind to the same final source and binary hashes.
Test databases were isolated from production and temporary resources removed.

## Production result

The completed cutover journal is
`/app/pn-adventures-20261005/production-cutover/journal.json`.
Actual quiescent map/character/login observations preceded graceful map saves.
Only the map process was restarted; dependency process start times and all
seven production container identities remained intact. No SQL migration was
needed. Existing configuration and unrelated cache records were preserved.

The first attempt stopped at the older deployment adapter's root-owner-only
check for a legacy-owned NPC file. Exact backups restored the previous server.
That attempt is retained in `production-cutover-attempt1`. The scoped atomic
writer then gained explicit owner/hash/link checks and owner preservation;
eight isolated write/restore/refusal cases passed. The health wait covers the
first 60-second runtime heartbeat. The second cutover completed successfully.

The matching client map was published after the new map server passed health,
before reopening gameplay ingress. Public signature, sequence, metadata and
object hashes were verified afterward; game status is online and all seven
production containers are healthy. The effective Office collision map is
100×100 and matches the client assets cell-for-cell.

Final map ELF SHA-256:
`8d7b7395b1481ef0964ff6445cde43be8bce0d7cb0d07a7bc7e0f52ad4ca44a0`.
Published `pn_office.grf` SHA-256:
`510c0b8961e721b4c4ca648ead60a4e6e1438c3ec8b712e8a6f5b2d88fdd8984`.

No rendered in-game walkthrough was performed. Automated results establish
script behavior, walking geometry, purchase persistence and updater integrity;
sprite appearance and the player experience still need a current client check.
This record does not claim a full rendered release-controller acceptance pass.
