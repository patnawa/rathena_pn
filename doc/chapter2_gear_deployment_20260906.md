# Chapter 2 client and Druid Gear deployment - 2026-09-06

This scoped receipt does not claim every episode, skill, item, instance, or
protected-client screen works perfectly. The broader audit remains active.

## Server changes

Three runtime files were installed into the existing dirty `/app/rathena`
checkout, preserving unrelated changes:

- `db/import/druid_gear_enchants.yml`: Gear_AT2 314269 (Pinion Shot) and
  Gear_AT1 314270 (Quill Spear), using permitted reference server effect evidence.
- `db/item_db.yml`: exactly one Renewal-only import of that overlay.
- `db/import/druid_item_enchant.yml`: two group-24 slot-2 perfect recipes,
  zero zeny and 150 each ClockTower_Gear, Shadowdecon, and Zelunium.

No eligibility, normal distributions, reset settings, existing recipes,
upgrades, drops, vendors, accounts, or engine binaries changed in this batch.
The Spring Regulator already opens group 24. Solid_Whinger remains a separate
missing equipment definition; this update does not invent that weapon.

The archive `druid-gear-reviewed-20260906.tar.gz` has SHA-256
`9c0ac1befb7dc12ab7b4729356bfb73fe6e9f71611e98c7e2d3ed4229500a936`.
The three-path manifest compares normalized baseline blobs from
`55aaa7c94dcad5e61188c320843f1cfeecf07c05` or exact incoming hashes and refuses
unrelated drift. Production and candidate passed before checks; both passed
exact after checks following their installations.

The existing packet-20260219 candidate ran `map-server --run-once`: 3,609 OnInit
scripts, ready state, no parser errors, no allocator leaks. The existing
root-privilege warning remains. No engine rebuild was needed for this data batch.

Deployment stopped login and rechecked zero online characters before stopping
the remaining services. Scoped data and a SQL dump were backed up. All four
services started about 08:43:08 UTC / 15:43:08 ICT and were verified running,
not restarting, with restart counters zero. All reached ready; map authenticated
to char and reported online. A char/login disconnect warning during the planned
stop was followed by reconnection. No post-start error was observed. All four
binary hashes remained unchanged from the allocator deployment.

Retained under `/app/rathena-deploy-backups/`:

- `pre-druid-gear-20260906.tar.gz`: existing scoped data, about 3.5 KiB.
- `pre-druid-gear-20260906.sql`: about 22 MiB; private, never committed.
- `druid-gear-reviewed-manifest-20260906.json`.
- `druid-gear-candidate-startup-20260906.log` and
  `druid-gear-deploy-20260906.log`.
- `deploy-druid-gear-20260906.sh`: failure rollback restores scoped files and
  services; the new overlay becomes unused without deleting any source file.

Read-only SQL confirmed all 15 previously configured maximum GM reputation
values remained intact. No player data was edited for testing.

## Client changes and recovery

Active client: `C:/Users/Alpha/Downloads/Compressed/Data2026/Data`. No game process
was running during installation. New `chapter2_native.grf` is DATA.INI priority
0. Original nebula_upgrade_v2/server/english/new/data GRFs retain that relative
order at priorities 1-5; none was modified. The new standard unencrypted GRF has
only the two reviewed native EnchantList/ItemDBNameTbl paths. It preserves the
entire old list prefix, 5,208 old mappings and the compiled lookup function.
Additions: groups 167-171, 22 targets, 58 exact server recipes, caution text,
84 aliases. The rejected no-caution draft was not installed.

Two new loose fragments add six metadata records: four Chapter 2 materials and
the two Gear enchants. Unique file/table pairs were appended after druiditems;
every existing import and the final override merge remains. Both use verified
generic EpisodClear20 assets, not new official artwork. Existing fragments were
not overwritten.

| Installed file | SHA-256 |
| --- | --- |
| DATA.INI | 45ce90be1cbc6e2af1ebc492ec0ac38713281103d87114deac566b2f344ec025 |
| SystemEN/itemInfo.lua | 6db791e0ea302b71a6cc13d068ef774f2d0c8cb885ceea96b3fd8626a5672811 |
| chapter2_native.grf | 09a60d6b3da391c7b45160338ccc887357ad836e680b6829426353fbfd160541 |
| SystemEN/itemInfo_Chapter2Materials.lua | bcb1cdd1e74111466ad00f6c8f7a7c4b65406da904fbe592deee65c8aa922da7 |
| SystemEN/itemInfo_DruidGear.lua | be2c7814b76537f8285c37dddffb8bec99ba14ac772dc348b1920d490e5580ab |

Prior DATA.INI and SystemEN/itemInfo.lua remain in
`server-work/client-before-chapter2-gear-20260906/` at matching relative paths.
Their hashes are respectively
`10b3584271cfb8197c61365f643c851e7b332f444d4cd9399febc4a6b1d595dd` and
`3bcef815049208711949e10a0bf975c90292f3cfed9d58749c5317b26838c065`.
Restoring those two files disables the new resources without deleting them.
Check for subsequent user edits before restoring, and close the game first.
A new client launch is needed to consume the installed load order.

## Tests and limits

- 18 Chapter 2 generator tests passed without skips, including actual Win32
  Lua 5.1 execution of all 5,292 lookups and independent compiled Lua parsing.
  Independent C# extraction reproduced both reviewed payload hashes.
- The read-only installed-client verifier independently extracted the actual
  active GRF, checked both payload hashes and original archive order, and
  confirmed only four appended loader lines. Actual Lua execution of the
  backed-up and installed loaders preserved all 26,836 previous item records
  recursively and added exactly six reviewed records (26,842 total;
  600,211 comparisons). It does not invoke native item registration or the game.
- The actual helper reproduced five rejected-draft caution errors. Corrected
  appended groups passed CheckFile and registered 58 exact recipes and 22
  targets through GetEnchantInfo/LoadAllData. All 931 complete original prefix
  callbacks remained unchanged, including a rerun after installation.
- The active metadata lacks 43 original target slot definitions. The strict
  fixture stops equally on original and patched full lists; this is a coverage
  limit, not evidence the game aborts. Its native unknown-item fallback remains
  unobserved. New groups use a byte-exact append and resolved target metadata.
- All ten integrated Gear tests passed without skips: fresh script/pc/skill/
  clif/allocator under ASan/UBSan, 20 item/grade/copy cases, 220 native assertions,
  no leaks or sanitizer findings. Original recipe properties/upgrades preserved.
- The original Druid recipe suite was extended with an explicit two-recipe
  allowlist while retaining its original six-group/63-recipe assertions. All
  ten focused checks passed with original client inputs. General enchant test
  discovery passed 53 checks with eight optional checks explicitly skipped
  (61 discovered); those skips are not presented as executed native tests.
- Unchanged Shadow Gear NPC dialogue passed native VM Open/Cancel/Escape tests:
  111 assertions and no script-side inventory/zeny mutation. This proves a
  window request, not a rendered window.
- Strict episode integrity passed with zero warnings: 899 enabled scripts,
  99 instances, 101 imports, 62 runtime fragments, 54 walkable arrival cells.
- Initial-enchant comparison: 164 client groups, 163 server, 163 shared,
  no server-only groups; 2,377 shared perfect recipes, 2,395 across all client
  groups. Valid probability totals. Still 24 differences: group 166 missing,
  13 target-list gaps, ten intentional custom Biosphere outcome tables.
  Missing server identities decreased from 40 to 38; these are not fixed here.

The computer-use skill could not inspect Windows because its native helper pipe
was unavailable. No alternative GUI input was attempted. Rendered menus, icons,
purchases, damage, resource charging, acquisition, and relog persistence remain
live verification work. Static tests do not equal 99 completed playthroughs.
