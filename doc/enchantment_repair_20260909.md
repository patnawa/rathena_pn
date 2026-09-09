# Enchantment repair — deployed 9 September 2026

The repair is installed on `192.168.10.18` and in this workspace's game client. The map server restarted successfully, connected to the character server, and reported online. All 16 deployed server files match the tested package; the map-server binary is unchanged. Login, character, database and map containers are running. No map-server errors occurred between deployment and final verification. The existing root-execution warning remains.

## Changes

- Corrected accessory reset probabilities from 21%/81% to the documented 20%/80%.
- Set the twelve original Varmundt Rune equipment resets (groups 16–19) to 100%, in both server and client.
- Corrected ten client Time Dimension crown probability tables to match the server and the reviewed wiki values.
- Added complete definitions and client metadata for missing enchant targets NP_B_Dagger, SC_B_Axe and Frontier_R_Crown_AT, plus the Frontier_AT_Axe set partner. Definitions use the supplied MuhRO client descriptions; existing client archives contain the required art.
- Required 1,500 Biosphere Depth 2 reputation for the alternate crown workshop route.
- Activated ten Shadow enchant books with matching native server option pools and client target lists. Added Shadow currency, Shadow Crates, supplies and recycling through Rayja at `grademk,26,184` and `pn_train,62,60`.

Reference: [MuhRO enchantment category](https://wiki.muhro.eu/Category:Enchantment), [accessories](https://wiki.muhro.eu/Accessories_Enchantment), [Royal Guard Ring](https://wiki.muhro.eu/Royal_Guard_Ring_Enchantment), [Varmundt Rune](https://wiki.muhro.eu/Varmundt_Rune_Enchant), [Time Dimension crowns](https://wiki.muhro.eu/Time_Dimension_Rune_Crown_Enchantment), [Shadow enchanting](https://wiki.muhro.eu/Shadow_Enchanting) and [Shadow gear](https://wiki.muhro.eu/Shadow_Gear).

Shadow book prices are 500,000 / 1,000,000 / 2,000,000 / 3,000,000 zeny, or 1 / 2 / 4 / 6 Nyangvine Fruit, by book tier. Recycling destroys the selected equipped Shadow item and pays 20 Shadow after confirmation and identity checks; bound and rental items are rejected. Books use native Laphine handling and reroll random options while preserving refine, grade and card enchants. Targets must be unequipped and refined +0 through +10.

**Server policy choices:** the reviewed references did not establish exact selection weights or crate contents. Each option in a Shadow line has equal selection weight, and each crate contains exactly 20 Shadow. These choices are displayed to players and are not asserted to reproduce unpublished MuhRO odds.

## Validation

| Check | Result |
| --- | --- |
| Client/server enchant groups | 164 compared; zero reported differences or unresolved names |
| Initial recipes and normal probability tables | 2,395 recipes and 339 tables; zero reported differences |
| Upgrade recipes | 1,269 ordinary and 444 guaranteed client recipes; zero reported differences; 24 existing server-only ordinary recipes retained |
| Native probability, upgrade and inventory helper tests | 96 checks passed with ASan/UBSan |
| Exhaustive normal probability checks | 33,900,000 draws across 339 tables passed |
| Comparator regression suites | 30 tests passed |
| Isolated native map-server startup | New databases, item scripts and NPCs loaded without errors against an isolated schema-only database |
| Native script acceptance | Reset boundary checks and 2,000 Shadow option draws passed |
| Native Lua enchant registration | 7,298 callbacks passed; no diagnostics |
| Native Lua Shadow lists | Ten exact target lists; unrelated entries preserved |
| Installed client verification | Four file hashes and three independently extracted GRF entries matched; all ten book IDs resolve uniquely and register exact targets |
| Live deployment | 16 server hashes verified, map server online, no startup errors |

The temporary test database and network were removed after deployment. No player data was copied into them. Deployment required zero online characters before stopping the map server.

## Client installation and use

The workspace client is already patched. Completely exit and restart the game to load the new GRF and item descriptions.

The [client patch](../client-patch/enchant_repair/README.md) contains the four client files for this server's supplied client build, plus the GRF source tables. Other players using that same build should close the game, back up `DATA.INI` and `SystemEN/itemInfo.lua`, then copy the installation files into their game directory. The included DATA.INI retains this build's existing eight archives and adds `enchant_repair.grf` at highest priority. A different client layout needs its archive list and item-info loader merged instead of blindly replaced.

## Backups and rollback

- Server originals: `/app/rathena-deploy-backups/enchant-repair-20260909T075714Z/files`.
- Local client originals: `server-work/enchant-audit-20260909/client-backup-20260909T075712Z/files` beneath the game client directory.
- Exact paths and SHA-256 manifests: `server-install-receipt.json` and `client-install-receipt.json` in the local audit release directory.
- Server rollback helper: `/tmp/enchant-deploy-20260909.py rollback`; this briefly stops the map server, restores originals, quarantines added files and restarts it. Coordinate rollback with the matching client restoration.
- Client restoration: restore the backed-up DATA.INI and SystemEN/itemInfo.lua, then move the added enchant_repair.grf and SystemEN/itemInfo_EnchantRepair.lua out of the client. Restart the game.

The deployed source package, startup logs, runtime evidence and historical pre-repair audit are retained locally under `server-work/enchant-audit-20260909` beneath the game client directory. This release note supersedes the historical audit's deployment status. Deployment receipts and machine-local rollback artifacts are not included in this repository.

## Limits

This closes the confirmed repair findings and adds the described Shadow enchant supply workflow. It does not certify every mechanic on all 94 wiki page families: the category-wide inventory is not a full semantic comparison. Unmapped costume/chapter items, broader MuhRO crafting vendors, scroll recycling and monster drops remain outside this implementation. New equipment definitions do not establish every acquisition route. No live character playthrough, disconnect/relog persistence test or combat-effect test was performed. Native startup and data agreement cannot establish that the entire game is bug-free.
