# Installed Chapter 2 and Druid Gear client check

Run from the repository root under WSL:

```sh
python3 tools/ci/chapter2_gear_client_install_test.py
python3 tools/ci/chapter2_client_helper_test.py
```

The first script is a read-only installation validator. It does not edit the
active client, backups, GRFs, or extracted artifacts. Its default paths point at
the supplied client root, `../client-before-chapter2-gear-20260906`, the reviewed
`../chapter2-native-verified-20260906` candidate, and the matching Win32 Lua 5.1.5
runtime in `../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe`. `--backup` and
`--lua` can select another backup location or Lua 5.1 executable; reviewed backup
and artifact hashes remain mandatory. Runtime provenance is recorded in
`chapter2_client_helper_README.md`.

## Checks and observed result

The 2026-09-06 installed checkpoint passes:

- The independently implemented GRF reader from `chapter2_native_patch_test.py`
  checks the installed archive, not the builder's manifest. The archive contains
  exactly the two reviewed resources, with exact payload hashes and byte equality
  to the reviewed extracted candidate. Neither resource has a loose-file override
  at its corresponding client path.
- `DATA.INI` puts `chapter2_native.grf` at priority 0 and preserves the previous
  five archives in order: `nebula_upgrade_v2.grf`, `server.grf`, `english.grf`,
  `new.grf`, `data.grf`. Every configured archive exists.
- The original loader's file/table pairs remain in their original order. Exactly
  two unique pairs are appended: `itemInfo_Chapter2Materials.lua` /
  `chapter2materials`, then `itemInfo_DruidGear.lua` / `druidgear`. Removing only
  these four import lines reconstructs the backed-up loader's normalized lines.
  The final official override merge remains last, including its original inline
  comment. Both installed fragment files equal the reviewed repository bytes.
- The actual Lua 5.1 interpreter executes the backed-up loader, recursively copies
  its merged `tbl`, then executes the installed loader using the real
  `F_itemInfoMerge`. Copying is necessary because the base file reinitializes
  `tbl`. All **26,836 existing records** retain every field recursively; exactly
  **six records** are added, producing **26,842 records**. The run made **600,211
  recursive value comparisons**.
- Added IDs are exactly `314269`, `314270`, `1002700`, `1002751`, `1002752`, and
  `1002753`. Actual fragment-produced metadata equals the installed merged records
  recursively. Each has zero slots, `ClassNum = 0`, `costume = false`, and the
  reviewed generic `EpisodClear20` resources. Effective server imports independently
  confirm that the two Gear records are enchant cards and the other four are Etc
  items, all without equipment slots.

The separate real-helper regression also passes after installation: 931 unchanged
callbacks for complete original groups 1-23 and exactly 58 new Chapter 2 perfect
recipe callbacks. The same 43 original targets lack merged itemInfo metadata.
No 12-crown metadata fragment is installed or required by this checkpoint.

## Pinned artifacts (SHA-256)

| Artifact | SHA-256 |
| --- | --- |
| Backup `DATA.INI` | `10b3584271cfb8197c61365f643c851e7b332f444d4cd9399febc4a6b1d595dd` |
| Backup `SystemEN/itemInfo.lua` | `3bcef815049208711949e10a0bf975c90292f3cfed9d58749c5317b26838c065` |
| Installed `chapter2_native.grf` | `09a60d6b3da391c7b45160338ccc887357ad836e680b6829426353fbfd160541` |
| GRF `Enchant/EnchantList.lub` | `4adcfaa537969e89622540acd302863dd653bb0f5d4a9211c6088f2a4f9fe203` |
| GRF `ItemDBNameTbl.lub` | `9c1bf7474d9869528b10c9383d3613d2491961b81adc9f0976065cddb043b301` |
| Reviewed `itemInfo_Chapter2Materials.lua` | `bcb1cdd1e74111466ad00f6c8f7a7c4b65406da904fbe592deee65c8aa922da7` |
| Reviewed `itemInfo_DruidGear.lua` | `be2c7814b76537f8285c37dddffb8bec99ba14ac772dc348b1920d490e5580ab` |
| Observed active `DATA.INI` | `45ce90be1cbc6e2af1ebc492ec0ac38713281103d87114deac566b2f344ec025` |
| Observed active `SystemEN/itemInfo.lua` | `6db791e0ea302b71a6cc13d068ef774f2d0c8cb885ceea96b3fd8626a5672811` |
| Observed Lua 5.1 executable | `a45f0f8376d3059a8bc79a4d6d07536cc1d9dec429852cfbc1f899ee96d6cd88` |

## Boundaries

This verifies archive installation and actual Lua loader/merge behavior, not
native game rendering, packets, login, or an enchant window. It never calls the
itemInfo native registration `main` functions and makes no network requests;
`os.execute` and `io.popen` are disabled in its Lua harness. This is not an OS-level
network sandbox or an execution mechanism for untrusted Lua.

The backed-up loader and active loader use the same current base and original
import files, whose content was not part of this installation change. This proves
the two loader configurations preserve those records; it is not a historical hash
audit of every pre-existing client file. Archive filenames/order are verified, not
the full contents of the five unchanged older archives.

The strict `C_GetSlotCount` recorder in the separate helper test intentionally
rejects missing target metadata instead of guessing slots. Its shared 43-target
baseline limitation is **not** proof that the native client aborts: the native
client's missing-item fallback is not established. Full native window behavior
still needs an in-game check. Metadata equality also does not prove visual art,
item acquisition, or combat effects; those require their separately scoped tests.
