# Character selection connection failure: 2026-09-07

The player's login authenticated and character selection succeeded, but the map
server rejected the character transfer with `chrif_authok: Data size mismatch!
9088 != 9488`. The earlier startup checks did not exercise this transfer and
therefore did not establish successful player login.

DWARF information in the deployed binaries confirmed `mmo_charstatus` was 9088
bytes in char-server and 9488 bytes in map-server. The shared header already
defined the expanded skill capacity. However, `src/char/Makefile.in` overwrote
its complete `COMMON_H` list with only `sql.hpp`, allowing stale character-server
objects after changes to `mmo.hpp`. Retaining the original shared-header list
fixes this incremental-build dependency.

`tools/ci/char_shared_header_dependency_test.py` runs GNU make against the real
template in a temporary fixture. An unchanged object is current; changing
`mmo.hpp` requires rebuilding with the fix and reproduces the stale-object
defect with the old assignment. The SQL-header dependency is also preserved.

The live sources and build dependencies were copied to an isolated directory:
`/app/rathena-deploy-backups/login-abi-repair-20260907-3IPhou`.
A forced char-server build completed in `rathena:local` with networking disabled
and `PACKETVER=20260219`. Its character structure is 9488 bytes, matching the
retained map binary. With zero online characters, the character binary was
installed and login, char, and map services were restarted. The map server
reconnected successfully and reported online.

Binary SHA-256 values:

- Previous char: `6d07b1170ad144d1e3d7718e39f49868c22fa061df323c15d418366fe295aab4`
- Rebuilt char: `d1b37f45c18efc5ed776b9f9163c14a96106b0daac3a134297e0f90e0a09a284`
- Retained map: `63e554b5829bce76d50b0edb1dacd84ff6bc200393c3f43d821647d05ea7d98a`

The previous binary remains at `char-server.before` in the repair directory.
The earlier episode deployment's char binary pin is superseded by this repair.
The fixed Makefile template was also installed on the live host and its generated
Makefile refreshed with `./config.status src/char/Makefile`.

After the restart, the live login log accepted account `admin`, the character
log recorded selection of `MSCESXi`, and the map log recorded `MSCESXi logged in`
with AID/CID `2000000/150000`. No error/fatal or character-data-size diagnostic
appeared in the logs checked since this restart. This verifies the player
transfer that the earlier startup-only checks missed.
