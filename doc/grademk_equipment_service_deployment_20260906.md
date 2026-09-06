# Grademk equipment-menu deployment

Deployed 2026-09-06 at `10:30:06Z` (17:30:06 Asia/Bangkok) to
`192.168.10.18`, `/app/rathena`. Deployment returned zero.

The new explicitly custom **Equipment Enchanter at Grademk 28,184** opens the
existing Flush Einbech (63), Muqaddas (64), Furious weapon (147), Furious crown
(148) and Sky Rune crown (165) interfaces. It does not distribute equipment,
charge resources, or change any recipe. All 123 effective target identities
resolve. The pre-existing Healer, grade NPC, other workshop services and group
24's regulator item are unchanged.

Only two runtime files were installed:

| File | Deployed SHA-256 |
| --- | --- |
| `npc/scripts_custom.conf` | `c6c52cb780c4367296cdc1738fa47f8cd1b61db582c6ca6180673fd366ce9b73` |
| `npc/custom/grademk_equipment_enchants.txt` | `62a4c017d9637653a14d2032aa9591aebde2f32e72c05ef66bd786679c88c1a7` |

The loader adds exactly one line after the existing Grademk services import.
No binary or client file changed from the equipment/cache deployment.
The pending Biosphere crown transaction changes were not included.

## Verification

- Fresh native ASan/UBSan script VM: five Open choices, Cancel and Escape;
  seven paths, 174 assertions, zero errors/failures and leak-free shutdown.
- The same native executable also passed with the post-wiring extracted body.
- Four map cells are walkable; the entrance connects to the standing cell
  while avoiding existing NPC coordinates. Proposed 34,184 was rejected because
  Sratos already occupies it. The new 28,184 position has no enabled collision.
- Integrated strict audit at the pre-deployment snapshot: 900 enabled scripts,
  110 database imports, 71 wired fragments, 99 instances, zero errors/warnings.
- Candidate `map-server --run-once`: zero exit, 3,610 OnInit NPCs, clean shutdown.
  Startup diagnostics contain only the existing root-user warning.
- Exact before/candidate-after/deployed-after two-file checks pass. All four
  services restarted and reported running, not restarting, restart count zero.
  Independent post-deployment checks confirmed readiness and no new errors.

The native test ends at the real builtin's outbound UI-request boundary; it
does not claim client transport, clicking, selection, material charging, or
visual gameplay verification. Details are in `grademk_equipment_service_audit.md`.

## Recovery

Baseline Git revision: `5e139a5552e2fd4d4cf72b1e9501a4609b5b0c3d`.
Archive `grademk-equipment-service-reviewed-20260906.tar.gz` SHA-256:
`a43086881591160408716fd4924be98ef11a82af2d0d519cce3a989030151429`.

Retained outside Git under `/app/rathena-deploy-backups/`:

- `pre-grademk-equipment-service-20260906.tar.gz`: previous NPC loader.
- `pre-grademk-equipment-service-20260906.sql`: approximately 22 MiB full snapshot.
- Reviewed archive/manifest, `deploy-grademk-equipment-service-20260906.sh`,
  and matching startup/deployment logs.

Zero online characters were verified after login admission stopped; the other
game services stopped before the database backup. Scoped rollback was available
but not needed. It restores the old loader and leaves the new script unused;
no SQL restoration, broad checkout reset or unrelated deletion occurred.
