# Real Chapter 2 client-helper verification

`chapter2_client_helper_test.py` executes the actual supplied active
`EnchantList_f.lub`, original/patched `ItemDBNameTbl.lub` functions, and actual
enchant declarations using the matching native Win32 Lua 5.1 runtime. It does
not rewrite the helper or replace its constructors with a Python interpreter.
No active client, GRF, list, helper, or server data is modified.

Run from the repository root in WSL (or Python on Windows):

```sh
python3 tools/ci/chapter2_client_helper_test.py
```

Defaults use the original extracted client resources, the preserved draft
`../chapter2-native-candidate-20260906`, corrected artifact
`../chapter2-native-verified-20260906`, and the independently obtained runtime at
`../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe`. Keep its adjacent DLLs and
CRT files intact. The script accepts `--lua`, `--draft`, and `--fixed` overrides.
It launches only the resource interpreter, with Lua shell/process-launch entry
points disabled; it does not launch the game, send packets, or install assets.

## Verified result, 2026-09-06

The actual helper reproduced **five draft-only missing-caution errors** from
`CheckFile`, one each for groups 167-171. The corrected exact append includes
`SetCaution("Choose carefully: enchant reset is unavailable for this equipment.")`
for all five groups. Its scoped `CheckFile` succeeds with no diagnostics.

Both the real `GetEnchantInfo` and `LoadAllData` produce identical registration
payloads for the corrected five-group append:

- 22 exact target callbacks, with IDs resolved by the actual compiled
  `ItemDB_To_ItemID` function.
- 58 exact `C_AddPerfectEnchant` callbacks, matching every effective server
  slot, enchant identity, price, material identity, and material quantity.
- Exactly one condition, random-option, reset, caution, and slot-order callback
  per group. Slot order remains `[3, 2, 1]`; reset remains disabled.
- No unexpected normal-enchant or upgrade callback types.

The original helper's `AddTargetItem_Duplicate` behavior is intentional: it
allows sharing a target across groups without consulting/inserting into
`GlobalTargetItemTbl`, but rejects repeated insertion within the same group.
The actual helper was probed with a known target and confirmed both behaviors.
Ordinary `AddTargetItem` still performs the global duplicate check. Do not turn
all cross-group target repetitions in the original list into false positives.

## Honest full-list boundary

The harness obtains `C_GetSlotCount` from the **actual active merged itemInfo**:
it executes `SystemEN/itemInfo.lua`, including its base/custom imports, actual
`F_itemInfoMerge` rules, and final override merge. It resolves target names via
the actual original or patched `ItemDB_To_ItemID` function. It never substitutes
zero for an unknown item or missing `slotCount`.

That strict check finds 43 pre-existing original targets without usable active
metadata. Original, draft, and corrected full-list execution all stop at the
same first missing target, `Solid_Whinger` / 510189. This is a **test-fixture
coverage boundary**, not a claim that the actual game necessarily throws the
same error: the native client's behavior for an unknown item slot query was not
observed and is not guessed.

Before that stop, groups 1-23 are complete. Their 931 actual `GetEnchantInfo`
callbacks remain unchanged between original, draft, and corrected inputs.
The test does not remove or repair partial group 24 and does not claim that
full original or full patched `LoadAllData` succeeded.

To verify the new groups despite that independent baseline gap, the test checks
that each patched resource preserves the entire original list byte-for-byte,
then executes its **unchanged appended bytes only** with a fresh instance of the
actual helper. All 22 Chapter 2 target slot counts resolve from the real active
custom itemInfo. Scoped `LoadAllData` therefore verifies all 58 new recipes
without inventing the 43 missing original target definitions.

All C registration callbacks are explicit recorders returning success. In
particular, the draft passes nil to `C_SetCaution`; the recorder captures this
without guessing how the native callback would react. The real `CheckFile`
failure, not a simulated native callback rejection, proves the draft defect.
Callback ordering through Lua `pairs` is not treated as stable; payload multisets
are compared, while the ordered slot array itself must match exactly.

## Constant provenance

These explicit fixture constants come from the supplied **unprotected 2025
reference executable**, not a protected-2026 runtime assertion:

| Lua global | Value | Double constant VA | Numeric Lua setter call |
| --- | ---: | --- | --- |
| `MAX_SLOT_NUM` | 4 | `0xfe6878` | `0x643bb1` |
| `MAX_GRADE_LEVEL` | 7 | `0xfe6880` | `0x643af8` |
| `MAX_MATERIAL_NUM` | 8 | `0xfe6888` | `0x643c73` |
| `MAX_REFINE_LEVEL` | 20 | `0xfe6890` | `0x643a2d` |

The numeric setter is `0x50be00`. The corresponding four little-endian double
byte patterns are `0000000000001040`, `0000000000001c40`,
`0000000000002040`, and `0000000000003440`. The slot constant also agrees with
`MAX_SLOTS 4` in `src/common/mmo.hpp`, which the test checks. The client grade
limit is **7**, not the server's Renewal grade-enum maximum 4.

The test verifies the executable hash before using this reference fixture:
`Ragexe_Server_20250604.exe` SHA-256
`33d4d9af476b8d24b5954d38d121b2bbe93044681b2945bb9b235fd9b25990cb`.
Reproduce the read-only reference inspection with:

```sh
objdump -d -Mintel --start-address=0x643990 --stop-address=0x643d40 ../../Ragexe_Server_20250604.exe
objdump -s --start-address=0xfe6878 --stop-address=0xfe6898 ../../Ragexe_Server_20250604.exe
```

Source hashes checked on every run:

| Source | SHA-256 |
| --- | --- |
| Active nebula EnchantList | `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d` |
| Original compiled ItemDBNameTbl | `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496` |
| Active new EnchantList_f | `bfee2e2ade0437fbb3a101e93e2e1a81986c3a484e4e96b9ec7fe10d52ac75cb` |

The corrected list at this checkpoint has SHA-256
`4adcfaa537969e89622540acd302863dd653bb0f5d4a9211c6088f2a4f9fe203`.
The separate patch-generator tests verify its complete original-prefix and
bytecode preservation guarantees. This helper test additionally checks the
prefix before isolated append execution and compares callback payloads with
current effective server definitions.

## Exact 43-target active metadata gap

The first 31 target identities from
`../../doc/remaining_enchant_coverage_audit.md` are included. Twelve additional
already-existing server crown identities are marked `server present` below.
The script's JSON output includes every missing name/ID; it fails if the
43-target baseline changes, requiring a reviewed checkpoint update rather than
silently applying a fallback.

| Target name | ID | Additional existing-server target |
| --- | ---: | --- |
| `Axe_Furious` | 520052 | |
| `D_Glacier_N_Axe` | 620057 | |
| `D_Glacier_N_Knife` | 510191 | |
| `Dimen_AT_Axe` | 620059 | |
| `Dimen_AT_Knife` | 510193 | |
| `F_Ein_AXE` | 520047 | |
| `Frontier_R_Crown_AT` | 401195 | |
| `FuriousCirclet_AT` | 401176 | |
| `Glacier_N_Axe` | 620056 | |
| `Glacier_N_Knife` | 510190 | |
| `Hall_Furious` | 590117 | |
| `Mocadas_Garz` | 590104 | |
| `NP_B_Dagger` | 510200 | |
| `Repeat_Dagger_AD` | 510185 | |
| `SC_B_Axe` | 620064 | |
| `S_AT_Armor` | 1270183 | |
| `S_AT_Earring` | 1270185 | |
| `S_AT_Pendant` | 1270186 | |
| `S_AT_Shoes` | 1270184 | |
| `Sky_Rune_Crown_ABC` | 401059 | server present |
| `Sky_Rune_Crown_AG` | 401172 | |
| `Sky_Rune_Crown_AT` | 401175 | |
| `Sky_Rune_Crown_BO` | 401173 | |
| `Sky_Rune_Crown_CD` | 401118 | server present |
| `Sky_Rune_Crown_DK` | 401216 | |
| `Sky_Rune_Crown_EM` | 401217 | |
| `Sky_Rune_Crown_HN` | 401117 | server present |
| `Sky_Rune_Crown_IG` | 401058 | server present |
| `Sky_Rune_Crown_IQ` | 401119 | server present |
| `Sky_Rune_Crown_MS` | 401115 | server present |
| `Sky_Rune_Crown_NW` | 401219 | |
| `Sky_Rune_Crown_SH` | 401060 | server present |
| `Sky_Rune_Crown_SHC` | 401171 | |
| `Sky_Rune_Crown_SKE` | 401120 | server present |
| `Sky_Rune_Crown_SOA` | 401220 | |
| `Sky_Rune_Crown_SS` | 401218 | |
| `Sky_Rune_Crown_TR` | 401174 | |
| `Sky_Rune_Crown_WH` | 401116 | server present |
| `Solid_Whinger` | 510189 | |
| `Stardust_Crown_SC` | 401056 | server present |
| `Stardust_Crown_SV` | 401055 | server present |
| `Stardust_Crown_VI` | 401057 | server present |
| `Time_DM_R_Crown_AT` | 400999 | |

No new equipment effects, slot counts, display metadata, acquisition routes,
or economy were inferred or installed by this pass. Native callback acceptance,
the protected 2026 executable, complete client initialization, and a live window
purchase remain untested.
