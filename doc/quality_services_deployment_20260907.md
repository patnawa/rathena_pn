# Quality services release — 2026-09-07

Installed six scoped runtime files on `/app/rathena`, with unchanged engine and
client binaries. PN Services is available at Izlude 140,146 (five variants) and
grademk 46,180. See [service instructions](quality_services.md) for limitations.
The existing Healer auto-identification change is included in this source batch.

Archive SHA-256:
`dee4283bc886fefdfae07f2aab4acae754b5c4e02a47128c73689ca84c663f22`.
Baseline Git revision: `e53922276`.

Verification completed:

- Strict episode integrity audit: 903 scripts, 100 instance definitions, no
  integrity errors or content-completeness warnings.
- Quality-services static contracts, walkable coordinates, client-format
  fixtures, scoped deployment/rollback, drift and safety-gate regressions.
- All 61 @go destinations; 134 native resolver assertions.
- Skill Supplies catalog and six placements.
- Card/combo audit: 5,719 card/enchant records, 5,168 combos, 10,504 script fields;
  native autobonus probability predicates at five rates.
- Active client: seven configured GRFs present with recognized signatures.
- Actual map-server `--run-once` against a disposable, network-isolated MariaDB
  with empty schemas and copied live NPC/database content: ready, no errors.
  This validates loading/OnInit, not attached-character dialog execution.
- Production: login admission closed, zero online characters reconfirmed, SQL
  dump and scoped source backups retained, map restarted. Fresh startup reached
  Map Server online without errors; six exact hashes verified; login reopened.

Backups: `/app/rathena-deploy-backups/quality-services-20260907/`, containing
receipt, original files, candidate log and live startup log. The separate SQL
dump is `quality-services-before-20260907.sql` in the parent backup directory.
The disposable test DB container was stopped; no live player tables were edited.

Still requires player acceptance: opening all menus, creating/re-entering a
private lab, a 60-second cast rotation, relog/interruption recovery, and opt-in GM
preset application. No measured gear winner or full client compatibility is
claimed. The lab currently uses a normal-class dummy, not boss mechanics.
