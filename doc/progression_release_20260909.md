# Progression and navigation release — 9 September 2026

Eight server database files and three client patch files were deployed with backups. The existing production executables were retained. Fresh login, character, and map startup logs reported readiness without errors, and every deployed file was checked against its candidate SHA-256.

## Changes

- Added the Frontier Claw Axe reform from a +10–20 Encroached Axe, its tuning item, and acquisition in both existing daily Singularity reward pools. The reform retains refine, grade, cards, and random options.
- Added six direct perfect level-one choices for the second seasonal Signet enchant. Server and client agree on the 40-million-zeny recipe and materials. Corrected the associated reset success rate to 100% and Token of Life Grace cost to 10.
- Gave both Aquila variants a two-second autonomous Max Pain cast. The separate scripted two-second warning sequence is preserved.
- Corrected quest 8846 to `jor_sanct,92,139` and quests 17279/17280 to `yuno_pre,95,71`.
- Removed misleading navigation links from unavailable quests 14995, 16147, 8886, 8916, and 21946. Their journals now explain availability. This does not implement the missing quests or enable the overlapping, inactive episode script.
- Added a GitHub release workflow and local release gate, including candidate fingerprinting, native regressions, YAML parsing, and a fresh map-server startup check. New equipment imports are explicitly retained by Git.

## Validation

The isolated Ubuntu candidate passed all 10 full release checks, including 20 native protection cases with 137 assertions, status packet boundaries, 256 RODEX state combinations, equipment bonuses, rental duration, Aquila timing, and equipment recipes. A fresh map startup loaded the databases and scripts successfully. Ubuntu test executables were not installed on the Alpine production services.

Separate protocol tests used the production Alpine executables against an empty, isolated SQL schema with synthetic accounts. One hundred saved status records preserved all fields over fresh connections and a character-server restart. An authenticated client session entered the protected map, stayed while protection was active, and was evacuated after expiry. It also verified that storage blocks the RODEX composer, the composer blocks storage, and closing each restores access. No mail was sent. Test containers and their temporary database were removed after evidence collection.

Native client Lua retained all 11,393 quest records and validated the changed guides, including repeated loading. The complete navigation audit checked 3,966 links with zero structural or NPC-name mismatches. Enchant comparison covered 164 groups and the native helper executed 7,304 callbacks without diagnostics. The rebuilt GRF's three entries matched their source bytes.

These checks do not establish a complete visual quest playthrough or Aquila combat test. Native desktop automation was unavailable; rendered client acceptance remains outstanding. Protocol checks cover the tested transitions, not every possible gameplay sequence.

The release workflow runs after these changes are committed and pushed. Deployment evidence and backup receipts are retained outside the repository; no credentials or external reference branding are included here.
