# Chapter 2 native enchant client coverage

Post-audit update: the resulting reviewed patch was installed on 2026-09-06.
See the [deployment receipt](chapter2_gear_deployment_20260906.md). The findings
below describe the original client before that installation.

Audit: 2026-09-06. Scope: the supplied original five-GRF client and existing
loose SystemEN files. No client, GRF, DATA.INI, server, or shared audit was
changed by this coverage investigation. The separate offline patch generator
is described in `client-patch/chapter2_native/README.md`; it does not install.

## Finding

Groups 167–171 have server recipes and NPC entry points, but **no definition
in the supplied client's native enchant registry**. No loaded supplement was
found. Adding server recipes alone cannot repair this: the server sends an
11-byte UI-open packet containing the group index, not recipes.

The gap comprises 22 target equipment records and 58 selectable perfect initial
enchants. All five groups use slot order 3, 2, 1; minimum refine and grade zero;
random options allowed; reset disabled; no normal or upgrade recipes.
Their existing costs must be copied exactly, not rebalanced.

| Group | Family | Targets | Slot 3 / 2 / 1 recipes |
| --- | --- | ---: | --- |
| 167 | Azure/Blaze armor and robe | 4 | 6 / 6 / 3 |
| 168 | Azure/Blaze manteau and muffler | 4 | 3 / 2 / 6 |
| 169 | Azure/Blaze boots and shoes | 4 | 2 / 4 / 6 |
| 170 | Azure/Blaze shields | 2 | 2 / 2 / 2 |
| 171 | Azure/Blaze accessories | 8 | 6 / 6 / 2 |

Every recipe in a given slot currently has the same costs:

| Slot | Zeny | Flame Coin | Mana Ring | Kindle material |
| --- | ---: | ---: | ---: | --- |
| 3 | 15000000 | 50 | 50 | Blue × 5 |
| 2 | 17500000 | 100 | 75 | Red × 5 |
| 1 | 20000000 | 150 | 100 | Yellow × 5 |

Server sources are effective recursive Renewal imports rooted at
`db/item_enchant.yml` and `db/item_db.yml`, including
`db/import/item_enchant.yml`. The NPC is Cardron in
`npc/custom/chapter2/Chapter2.txt`, at `kin_in01,123,215`.
Its Chapter-2-completion-gated menu calls `item_enchant(167)` through
`item_enchant(171)` on lines 539–543.

## Effective supplied load chain

`../../DATA.INI` SHA-256:
`10b3584271cfb8197c61365f643c851e7b332f444d4cd9399febc4a6b1d595dd`.

| Priority | GRF | Enchant resources present |
| ---: | --- | --- |
| 0 | nebula_upgrade_v2.grf | EnchantList.lub only |
| 1 | server.grf | None |
| 2 | english.grf | None |
| 3 | new.grf | EnchantList.lub and EnchantList_f.lub |
| 4 | data.grf | EnchantList.lub and EnchantList_f.lub |

The resource paths are `data\luafiles514\lua files\Enchant\EnchantList.lub`
and `EnchantList_f.lub`, case-insensitive. Whole resources override lower
resources; duplicate names are not concatenated. Thus the effective list is
the nebula plaintext file; the effective helper is the new plaintext file.
All four higher-priority GRFs also lack ItemDBNameTbl: that resource comes
from data.grf. No loose `../../data` enchant or Chapter2 supplement was found.

The active helper initializes `Table = {}` (line 7). Its
`GetEnchantInfo` returns false immediately when `Table[in_EnchantNum]`
is nil (around line 488), then otherwise calls the native `C_*` registration
functions. `LoadAllData` (line 582) iterates only `pairs(Table)`.
Neither active helper nor active list uses require, dofile, or loadfile.
Both the active nebula list and the lower new list define 159 groups ending
at 166, with no 167–171.

The two lower data.grf files were also examined as ordinary unencrypted Lua
5.1 bytecode, without executing or decrypting them. The base list has 159
top-level CreateEnchantInfo assignments ending at 166, zero child functions,
and no supplementary imports. The lower helper likewise has no imports.
They therefore contain no hidden Chapter2 supplement even before precedence
is considered.

The loose `SystemEN/itemInfo.lua` loads its existing base itemInfo, then
itemInfo_C, ZeroCell, Chapter2, Fashion, and DruidItems in that order and merges
their `tbl_*` metadata through `F_itemInfoMerge`; the override merge remains
last. No SystemEN Lua/LUB file calls CreateEnchantInfo, GetEnchantInfo,
C_AddPerfectEnchant, or C_SetSlotOrder. Item display metadata does not populate
the separate native enchant Table or ItemDBNameTbl.

## Native loader and protocol evidence

Read-only disassembly of the supplied **older**
`../../Ragexe_Server_20250604.exe` shows its enchant initializer loading
EnchantList_f.lub at 0x6446c5/0x6446ef, then EnchantList.lub at
0x644730/0x64475a, then calling LoadAllData at 0x6447e7/0x644801.
There is no supplementary file load in that initializer. Its SHA-256 is
`33d4d9af476b8d24b5954d38d121b2bbe93044681b2945bb9b235fd9b25990cb`.
This corroborates the helper-first/list-second contract; it is not a
disassembly or runtime trace of the protected 2026 executable. No protection
bypass or decryption was attempted.

On the server, `BUILDIN_FUNC(item_enchant)` checks the server database before
calling `clif_enchantwindow_open`. That sends `PACKET_ZC_UI_OPEN_V3`
(header 0x0b9a): int16 packetType, uint8 type, uint64 data. The type is
OUT_UI_ENCHANT and data is the client Lua index. No target, price, material,
or recipe is transmitted. Therefore native windows cannot reconstruct these
58 absent client recipes from the server on opening.

The missing registry entries and dependencies are proven for the provided
files. The exact visual symptom in the protected 2026 client remains unobserved:
this audit did not open or purchase from a live game window.

## Exact native item-name dependencies

The five groups refer to 85 distinct numeric item identities. Original
ItemDBNameTbl has 5208 entries; only `Ch1_Mana_Ring = 1001996` already resolves.
The following **84 names and their numeric IDs are both absent**, with no
alternative original client alias at those IDs. These are exact existing
server canonical identities, not guesses at official client spellings:

```text
Ch2_Azure_Armor = 450531
Ch2_Azure_Boots = 470420
Ch2_Azure_Brooch = 490931
Ch2_Azure_Earring = 490932
Ch2_Azure_Manteau = 480751
Ch2_Azure_Muffler = 480752
Ch2_Azure_Necklace = 490933
Ch2_Azure_Ring = 490930
Ch2_Azure_Robe = 450532
Ch2_Azure_Shield = 460147
Ch2_Azure_Shoes = 470421
Ch2_Blaze_Armor = 450533
Ch2_Blaze_Boots = 470422
Ch2_Blaze_Brooch = 490935
Ch2_Blaze_Earring = 490936
Ch2_Blaze_Manteau = 480753
Ch2_Blaze_Muffler = 480754
Ch2_Blaze_Necklace = 490937
Ch2_Blaze_Ring = 490934
Ch2_Blaze_Robe = 450534
Ch2_Blaze_Shield = 460148
Ch2_Blaze_Shoes = 470423
Ch2_Flame_Coin = 1002700
Ch2_Kindle_Hal_B = 1002751
Ch2_Kindle_Hal_R = 1002752
Ch2_Kindle_Hal_Y = 1002753
aegis_314927 = 314927
aegis_314928 = 314928
aegis_314929 = 314929
aegis_314930 = 314930
aegis_314931 = 314931
aegis_314932 = 314932
aegis_314933 = 314933
aegis_314934 = 314934
aegis_314935 = 314935
aegis_314936 = 314936
aegis_314937 = 314937
aegis_314938 = 314938
aegis_314939 = 314939
aegis_314940 = 314940
aegis_314941 = 314941
aegis_314942 = 314942
aegis_314943 = 314943
aegis_314944 = 314944
aegis_314945 = 314945
aegis_314946 = 314946
aegis_314947 = 314947
aegis_314948 = 314948
aegis_314949 = 314949
aegis_314950 = 314950
aegis_314951 = 314951
aegis_314952 = 314952
aegis_314953 = 314953
aegis_314954 = 314954
aegis_314955 = 314955
aegis_314956 = 314956
aegis_314957 = 314957
aegis_314958 = 314958
aegis_314959 = 314959
aegis_314960 = 314960
aegis_314961 = 314961
aegis_314962 = 314962
aegis_314963 = 314963
aegis_314964 = 314964
aegis_314965 = 314965
aegis_314966 = 314966
aegis_314967 = 314967
aegis_314968 = 314968
aegis_314969 = 314969
aegis_314970 = 314970
aegis_314971 = 314971
aegis_314972 = 314972
aegis_314973 = 314973
aegis_314974 = 314974
aegis_314975 = 314975
aegis_314976 = 314976
aegis_314977 = 314977
aegis_314978 = 314978
aegis_314979 = 314979
aegis_314980 = 314980
aegis_314981 = 314981
aegis_314982 = 314982
aegis_314983 = 314983
aegis_314984 = 314984
```

The same reviewed mapping is machine-readable in
`client-patch/chapter2_native/manifest.json`. Generate names and recipes
from effective server data, compare against this allowlist, and stop on drift
or collision. Do not renumber items or overwrite another name's mapping.

The original ItemDBNameTbl chunk also defines `ItemDB_To_ItemID`, a
zero-upvalue function that returns the mapped ID or displays MessageBox and
returns 0 for a missing name. A table-only plaintext replacement would discard
that behavior. A compatible additive transformation must retain its original
function prototype and existing bytecode/constants, not assume the client
accepts plaintext at this lookup resource.

## Separate display-data gap

Among the 85 recipe identities, 80 targets/enchant outcomes have records in
the already imported `SystemEN/itemInfo_Chapter2.lua`; Mana Ring exists in
the main `SystemEN/LuaFiles514/itemInfo.lua`. The four materials 1002700
and 1002751–1002753 have no itemInfo records in the supplied active import
files (only tipbox references elsewhere). This does not replace the missing
native-name finding; it is an additional item-display dependency.

Server Name fields for the latter materials still carry TODO translation
comments (Blue/Red/Yellow Paper). Do not claim these labels are official.
No unverified icon or sprite/resource name should be invented. The subsequent
offline patch work verified the existing generic EpisodClear20 item BMP,
collection BMP, SPR, and ACT in original data.grf and supplied a separate
four-record fragment in
`client-patch/chapter2_native/SystemEN/itemInfo_Chapter2Materials.lua`.
It follows server labels/type/weight and is not loaded automatically; it must
not reorder existing SystemEN imports or claim official Chapter2 artwork.

## Reproducible artifacts and smallest patch

All paths below are relative to this repository; GRFs were only indexed/read
with the existing `tools/grf_v3_extract/grf_v3_extract.exe`.
The lower data copies were extracted under
`../audit-chapter2-native-20260906/data/`.

| Input | SHA-256 |
| --- | --- |
| Active nebula EnchantList, under ../audit-client-enchants-20260906/nebula/ | 664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d |
| Active new helper, under ../audit-client-enchants-20260906/new/ | bfee2e2ade0437fbb3a101e93e2e1a81986c3a484e4e96b9ec7fe10d52ac75cb |
| Original ItemDBNameTbl, under ../audit-item-aliases-20260906/data/ | 2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496 |
| Lower data EnchantList | 0ac5410fac9b0a92be77f5420cac56c5af9f9074b086d1f7cd6baf8136e1479e |
| Lower data helper | a7e380da53350f605c1a7933a23c21e816f3f89cca437a5bca1766ec5356e143 |
| Loose SystemEN/itemInfo_Chapter2.lua | 54801f37a0ef4b344c806904dc1a5bc28b812ffd12cd1f1dbd96caad6deb912c |

The smallest native patch is a **two-resource overlay**: byte-for-byte
preserved active EnchantList plus five appended groups/58 exact recipes;
and original compiled ItemDBNameTbl plus the 84 reviewed literal assignments,
retaining every original mapping and original lookup function body.
Do not edit the helper, replace full translation GRFs, touch DATA.INI, or
change server balance for this generator. Any later installation must give
this two-entry overlay precedence over the current matching resources while
preserving all existing GRFs and their relative order.

Acceptance requires a deterministic archive, independent extraction matching
both generated files, all 5208 old mappings unchanged, all 84 aliases correct,
original function prototypes unchanged, exact comparison of all five groups
against effective server data, and preservation of every existing initial,
normal-probability, and upgrade declaration. Source hash/schema mismatches
must refuse generation. Offline tests cannot certify a live client purchase;
installation and live UI verification remain separate.
