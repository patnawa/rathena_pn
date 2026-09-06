# Chapter 2 native enchant compatibility patch

Installation update: this patch and the four material records were installed
in the active local client on 2026-09-06. See the
[deployment receipt](../../doc/chapter2_gear_deployment_20260906.md) for scope,
backups, verification, and limits. The generator itself remains offline-only.
Candidate measurements below precede the separate two-item Druid Gear server
integration, which reduces unresolved server identities from 40 to 38 and
reported audit differences from 26 to 24.

This directory contains reviewed configuration and a clean-room item metadata
fragment, not the generated GRF binary. The generator writes a new
artifact directory under `server-work` only. It does not modify active GRFs,
DATA.INI, loose client files, server data, or loader imports.

See `doc/chapter2_native_client_coverage.md` for the five-GRF load-chain audit,
the exact 84 missing name/ID mappings, and the native UI protocol evidence.

## Contents and preservation contract

The generated `chapter2_native.grf` has exactly two unencrypted GRF-v2 entries:

| Resource | Additions | Preserved |
| --- | --- | --- |
| data/luafiles514/lua files/Enchant/EnchantList.lub | Groups 167–171; 22 targets; 58 exact perfect initial recipes; required caution text | Every byte of the original active nebula list remains an unchanged prefix |
| data/luafiles514/lua files/ItemDBNameTbl.lub | 84 reviewed server name-to-ID aliases | All 5208 old mappings, original instructions/constants, and the complete original ItemDB_To_ItemID function prototype |

The item-name resource stays **32-bit little-endian Lua 5.1 bytecode**, not
plaintext. The generator inserts three literal instructions per alias before
the original function declaration, appends constants, and inserts synthetic
zero line numbers without altering existing debug entries. It does not
decompile/recompile the original lookup function or introduce a new lookup
implementation. Native Lua execution tests load the actual resulting bytes
directly: there is no architecture/header conversion in the test.

The existing active EnchantList_f helper remains untouched. Its CheckFile
contract requires SetCaution even when reset is disabled. Each new group gets
the explicit client-only text recorded in the manifest: "Choose carefully:
enchant reset is unavailable for this equipment." This is clean-room UI text,
not an additional server charge or gameplay rule.

All five groups' prices, quantities, identities, eligibility, slot order, and
disabled reset state come from the effective recursive Renewal server imports.
The manifest pins their normalized semantic SHA-256 and all three original
client input hashes. Source drift, unsupported raw fields, existing names or
IDs, scope changes, and schema mismatches refuse generation. A changed server
recipe requires a separate review and manifest update, not an automatic repin.

## Build and test

Run from the repository root with Python 3 and PyYAML. Under Windows these
commands can be prefixed with `wsl -d Ubuntu --exec`.

```sh
python3 tools/ci/build_chapter2_native_patch.py \
  --enchant-list '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub' \
  --helper '../audit-client-enchants-20260906/new/data/luafiles514/lua files/Enchant/EnchantList_f.lub' \
  --output ../chapter2-native-new-review

python3 tools/ci/chapter2_native_patch_test.py \
  --luac ../roenglishre_ch2_probe/Tools/luac.exe \
  --lua ../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe -v
```

The output directory must not exist. The write boundary is anchored to the
script's repository location; changing the read-only `--root` argument cannot
move it into the active client. No overwrite option or installer is provided.

The tests cover independent GRF extraction, deterministic output, exact
bidirectional comparison of all five groups, unchanged old initial/upgrade
and probability tables, all old/new identity mappings, byte-exact preservation
of the original function, source/schema/collision refusals, and optional
independent compiled Lua parsing plus actual Lua execution. The real execution
test calls ItemDB_To_ItemID for every old/new name and verifies the original
MessageBox plus zero return for an unknown name. Skipped executable tests are
not runtime proof: pass both explicit executable paths for the full run.

The test runtime used here is the matching Win32 Lua 5.1.5 package from the
[LuaBinaries project](https://sourceforge.net/projects/luabinaries/files/5.1.5/Tools%20Executables/lua-5.1.5_Win32_bin.zip/).
It is a local verification dependency, not bundled into the client or repository.

| Verification dependency | SHA-256 |
| --- | --- |
| Downloaded lua-5.1.5_Win32_bin.zip | c831a26d9c2280adf594a33690324de5de3cf6fb75f26b3753bae00812bcf162 |
| lua5.1.exe | a45f0f8376d3059a8bc79a4d6d07536cc1d9dec429852cfbc1f899ee96d6cd88 |
| lua5.1.dll | fbbe7ee073d0290ac13c98b92a8405ea04dcc6837b4144889885dd70679e933f |

Independent archive validation can additionally use the existing C# extractor:

```powershell
& tools/grf_v3_extract/grf_v3_extract.exe `
  ../chapter2-native-verified-20260906/chapter2_native.grf '.*' `
  ../chapter2-native-independent-extract-20260906
```

## Reviewed candidate, not installed

The corrected candidate is `../chapter2-native-verified-20260906/`:

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| EnchantList.lub | 1067270 | 4adcfaa537969e89622540acd302863dd653bb0f5d4a9211c6088f2a4f9fe203 |
| ItemDBNameTbl.lub | 283347 | 9c1bf7474d9869528b10c9383d3613d2491961b81adc9f0976065cddb043b301 |
| chapter2_native.grf | 143000 | 09a60d6b3da391c7b45160338ccc887357ad836e680b6829426353fbfd160541 |

Verification on 2026-09-06: **18 focused tests passed, no skips**, with the
explicit native Lua parser and interpreter above. The independent C# extractor
read exactly two entries and reproduced both payload hashes in this table.
Full initial-enchant comparison found 163 shared groups (164 client / 163
server), no server-only groups, and only pre-existing missing server group 166.
All client/server probability totals remained valid. The same 40 unresolved
server identities remain, alongside 26 existing issue rows: one missing group,
13 target lists, two perfect requirements, and ten custom Biosphere outcome
tables. No Chapter2 comparison issue remains.

The earlier `../chapter2-native-candidate-20260906/` is an intentionally retained
**rejected draft** without mandatory caution metadata. Do not install it. It
is useful only for demonstrating the helper validation failure and its fix.

## Four separate material display records

`SystemEN/itemInfo_Chapter2Materials.lua` defines only IDs 1002700 and
1002751–1002753 in `tbl_chapter2materials`. The original active itemInfo import
chain lacks these four records. Labels/type/weight follow existing server
records; the Blue/Red/Yellow Paper translations remain upstream TODOs.
Descriptions add no effects, drop rates, prices, or acquisition claims.

Both display resource fields use the known generic **EpisodClear20** asset.
Read-only indexing independently confirmed its item BMP, collection BMP, SPR,
and ACT in the supplied original data.grf. This is the same fallback already
used by the project's Druid item metadata; it is not official Chapter 2 art.

This fragment is deliberately separate from the two-entry GRF and is not
loaded automatically. Any later installation should add one file/table pair
to the existing multi-itemInfo imports, preserving every previous entry and
its relative order and keeping the existing override merge last. Do not
replace the existing Chapter2, Druid, Fashion, ZeroCell, or custom metadata.

## Remaining limits

Native Lua/registration tests and independent extraction are offline evidence,
not a live protected-2026-client purchase test. Actual UI display, icon use,
charging, success application, and reconnect persistence still need a scoped
live check after installation; offline verification does not prove gameplay.

Do not treat unrelated client-only group 166 or the 40 pre-existing unresolved
server item identities as fixed by this patch. No server balance, equipment
effects, or new acquisition route is included.
