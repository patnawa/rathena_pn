# Costume cosmetic stone reference - 2026-09-28

## Evidence and interpretation

The table reconciles 45 local physical cosmetic materials with already implemented enchant cards. Exact numeric pairs come from the local rAthena physical/card names and scripts, not an official server NPC dump. Korean physical positions and fourth-slot descriptions were additionally checked against Gravity's official September 7, 2026 client `itemInfo_true.lub`, using the existing native Lua 5.1 runtime. Thus mappings are semantic reconciliations supported by matching primary metadata, not claims that official server code was recovered.

Primary metadata source: [Gravity September 7 patch](http://ropatch.gnjoy.com/Patch/2026-09-07_live_client_3363_3364_1788764913.rgz). Cached file: `../client-compat-update-20260909/official-client/System/itemInfo_true.lub`. The patch URL is recorded in `../client-compat-update-20260909/latest-iteminfo-source.json`; the exported description evidence is `../costume-cosmetics-official.tsv` (CP949).

Local implementation source: [item database](../db/re/item_db_etc.yml), [effect enum](../src/map/script.hpp), [hateffect implementation](../src/map/script.cpp). [Official Twinkle web page](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25058&itemSeq=7) independently confirms upper costume fourth-slot attachment.

`K` means the official Korean metadata confirms that physical material's costume position and fourth slot. `R` means a regional material absent from that Korean metadata; local positions remain supported by item naming and the regional evidence below. All table output cards contain paired `hateffect` equip/unequip scripts. Fourth slot corresponds to card array index 3; preserve other card slots when installing an effect.

| Physical | Enchant | Position | Name | Script effect | Evidence |
|---|---|---|---|---|---|
| 25058 | 29041 | Upper | Twinkle Effect | `HAT_EF_LJOSALFAR` | K |
| 25059 | 29040 | Middle | Ghost Effect | `HAT_EF_C_GHOST_EFFECT` | K |
| 25136 | 29142 | Middle | Electric Effect | `HAT_EF_ELECTRIC` | K |
| 25137 | 29143 | Lower | Green Flare Effect | `HAT_EF_GREEN_FLOOR` | K |
| 25138 | 29144 | Middle | Shrink Effect | `HAT_EF_SHRINK` | K |
| 25176 | 29160 | Middle | Blue Aura Effect | `HAT_EF_CIRCLEPOWER` | K |
| 25177 | 29162 | Middle | Shadow Effect | `HAT_EF_KAGEMUSYA` | K |
| 25178 | 29161 | Middle | Pink Glow Effect | `HAT_EF_CHERRYBLOSSOM` | K |
| 25205 | 29144 | Lower | Shrink Effect | `HAT_EF_SHRINK` | K |
| 25206 | 29142 | Upper | Electric Effect | `HAT_EF_ELECTRIC` | K |
| 25224 | 29224 | Middle | White Body Effect | `HAT_EF_WHITEBODY2` | K |
| 25225 | 29226 | Middle | Crimson Wave Effect | `HAT_EF_DOUBLEGUMGANG` | K |
| 25226 | 29225 | Lower | Water Field Effect | `HAT_EF_WATER_BELOW4` | K |
| 1000319 | 300152 | Middle | Angel Blessing Effect | `HAT_EF_Blessing_Of_Angels` | R |
| 1000345 | 300157 | Middle | Time Accessory Effect | `HAT_EF_C_Time_Accessory` | R |
| 1000365 | 310322 | Upper | Magic Feather Effect | `HAT_EF_magical_feather` | R |
| 1000875 | 300385 | Middle | Valhalla Effect Effect | `HAT_EF_VALHALLA_IDOL` | R |
| 1000882 | 29142 | Lower | Electric Effect | `HAT_EF_ELECTRIC` | K |
| 1001069 | 300419 | Upper | Camellia Smoke Effect | `HAT_EF_Camellia_Hair_Pin` | R |
| 1001175 | 311926 | Upper | Romance Rose Effect | `HAT_EF_C_Romance_Rose_TW` | R |
| 1001252 | 29727 | Upper | Rumble Effect | `HAT_EF_RESONATETAEGO` | R |
| 1001615 | 313065 | Garment | Footprint Effect | `FOOTPRINT_EF_BASE` | K |
| 1001616 | 313066 | Garment | Whirlwind Footprint | `FOOTPRINT_EF_STR_BASE` | R |
| 1001617 | 313067 | Garment | Purple Star Footprint | `FOOTPRINT_EF_PURPLESTAR` | K |
| 1001650 | 313068 | Garment | Yellow Star Footprint | `FOOTPRINT_EF_YELLOWSTAR` | K |
| 1001651 | 313069 | Garment | Red Star Footprint | `FOOTPRINT_EF_REDSTAR` | K |
| 1001772 | 313345 | Garment | Puppy Footprint | `FOOTPRINT_EF_DOGFOOT` | K |
| 1001907 | 313558 | Garment | Dumpling Footprint | `FOOTPRINT_EF_DUMPLING` | K |
| 1001908 | 313559 | Garment | Panda Basic Footprint | `FOOTPRINT_EF_PANDA_BASIC` | K |
| 1001909 | 313560 | Garment | Panda Color Footprint | `FOOTPRINT_EF_PANDA_COLOR` | K |
| 1002108 | 313955 | Garment | Blue Butterfly Footprint | `FOOTPRINT_EF_BUTTERFLY_BLUE` | K |
| 1002193 | 314094 | Middle | Blue Fighting Spirit Effect | `HAT_EF_ROS_BlueSpirit` | K |
| 1002194 | 314095 | Middle | Red Fighting Spirit Effect | `HAT_EF_ROS_RedSpirit` | K |
| 1002239 | 314171 | Garment | Footprint (2D) | `FOOTPRINT_EF_DRAGON_FACE_2D` | K |
| 1002240 | 314172 | Garment | Footprint (3D) | `FOOTPRINT_EF_DRAGON_FACE_3D` | K |
| 1002285 | 313956 | Garment | Purple Butterfly Footprints | `FOOTPRINT_EF_BUTTERFLY_PURPLE` | K |
| 1002286 | 314066 | Garment | ROS 2025 Footprint | `FOOTPRINT_EF_VICTORY2025` | K |
| 1002294 | 314212 | Garment | Nyar's Blue Footprints | `FOOTPRINT_EF_NYAR_BLUE` | K |
| 1002295 | 314213 | Garment | Nyar's Purple Footprints | `FOOTPRINT_EF_NYAR_PURPLE` | K |
| 1002296 | 314214 | Garment | Divine Light Footprints | `FOOTPRINT_EF_DIVINE` | K |
| 1002394 | 313957 | Garment | Yellow Butterfly Footprints | `FOOTPRINT_EF_BUTTERFLY_YELLOW` | K |
| 1002623 | 314846 | Garment | Blue Star Footprints | `FOOTPRINT_EF_BLUESTAR` | K |
| 1002624 | 314847 | Garment | Sacred Blue Light Footprints | `FOOTPRINT_EF_DIVINE_BLUE` | K |
| 1002642 | 314813 | Garment | Phoenix Operation Footprints | `FOOTPRINT_EF_PHOENIX` | K |
| 1002822 | 315055 | Garment | Flower Garden | `FOOTPRINT_EF_FLOWER_GARDEN` | K |

## Regional evidence and remaining ambiguity

- [Thailand June 2024 promotion](https://ro.gnjoy.in.th/spend-promotion-june-2024/) names Roar upper, Romantic Rose upper and Valhalla Idol **middle**. For local physical 1000875, use middle despite Aegis name `Valhalla_F_Effect_T`; its local display name agrees with the official regional page. Exact IDs are the local database reconciliation.
- [Thailand April 2024 promotion](https://ro.gnjoy.in.th/spend-promotion-april-2024/) confirms Magic Feather, Baby Camellia and Time Accessory cosmetic materials. The fetched page text does not spell out their positions; local names specify upper, upper and middle respectively.
- [Thailand September 2025 promotion](https://ro.gnjoy.in.th/promotion-spend-promotion-september-2025/) explicitly lists Angel Blessing middle. It also lists **Melody Notes lower** and **Firework upper**. Physical IDs for those last two remain unverified and were not present under matching names in the inspected local database; do not invent IDs or reuse unrelated materials. Firework output enchant 310850 exists with `HAT_EF_firework`. A third-party search lead for Melody item 1001736 is not primary evidence and that ID is absent locally.
- Physical 1001616 / output 313066 is a matching local Whirlwind Footprint pair, but the official Korean client inspected has no physical record. Its local garment classification is not independently confirmed by the official pages reviewed here.

## One additional defined but nonfunctional effect

Physical 1001038 maps by exact local name to enchant 311459 `Evt_20th_Effect`; official Korean metadata confirms middle costume fourth slot. However, enchant 311459 has **no Script or UnEquipScript**. Existing `HAT_EF_EFST_C_20TH_ANNIVERSARY_HAT` is a similarly named effect, but that alone does not prove the enchant uses it. A client item-to-effect table or other primary source must establish that relation before adding the script. Do not present a consumed stone with no verified effect as functional.

## Client compatibility boundary

Server card scripts and constants establish server support; they do not establish visual assets in the deployed client. The inspected cached `effective/.../hateffectinfo.lub` identifies a February 4, 2026 build. It contains a `hatEffectTable`, while footprints are handled separately. Loading it with a placeholder `HatEFID` shows a 20th anniversary hat entry, but does not prove the missing enchant's item association.

The primary patch index in `../client-compat-update-20260909/official-effective.json` records:

- `footprinteffectinfo.lub` and Flower Garden footprint `.str`, `.bmp`, `.tga` assets in `2026-07-15_live_data_3240_3243_1783564439.gsf`.
- `hateffectids.lub` updated in `2026-08-19_live_data_3309_3315_1786504293.gsf`.
- `hateffectinfo.lub` updated in `2026-08-05_live_data_3285_3286_1785390239.gsf`.
- `effecthatitemtable.lub` updated in `2026-04-01_live_data_3116_3120_1774858430.gsf`.

Those records are available from the [official patch feed](http://ropatch.gnjoy.com/PatchInfo/patch2.txt), with patch files under `http://ropatch.gnjoy.com/Patch/`. A file's appearance in an official index is not proof of installation. Flower Garden requires a deployed-client resource audit and a game rendering check before claiming it displays correctly.

The official Korean Shrink physical descriptions also state the effect is disabled in siege/PvP and changes the base body size rather than cart/effect size; Shadow states siege/PvP exclusion. This note has not verified client/server enforcement of those cosmetic restrictions.

## Scope

This supplements the costume stone reference audit. It is a local material reconciliation and selected official regional verification, not an exhaustive worldwide list. No server code or item data was changed by this research.

## Supplemental official client reconciliation

The subsequent full native client scan found two additional Korean footprint pairs absent from the local database: **1002855 -> 313953 Cherry Blossom**, and **1002856 -> 313954 Sprout**. Both physical descriptions explicitly require garment costume fourth slot. Their server constants `FOOTPRINT_EF_BLOSSOM` (257) and `FOOTPRINT_EF_BUD` (258) are present and exported. The official Thai native footprint table uses those same IDs and the matching blossom/bud graphical resources. Exact source entries and machine-readable validation are in [supplemental source JSON](costume_cosmetics_official_20260928.json).

The official Thai September 23 client metadata resolves the earlier regional material ambiguities, including the seven region-specific rows above. It also provides four additional exact pairs: Firework **1000573 -> 310850 upper**, Melody Notes **1001927 -> 313724 lower**, Kiel **1001799 -> 300566 lower**, and Red Flame **1003031 -> 315313 upper**. This supersedes the earlier missing-physical-ID statements. Firework has an existing local effect script. The other three physical/card names match, but exact script-effect associations remain unverified; candidate Melody/Kiel graphical constants in the JSON are explicitly tentative. Source: [official Thai September 23 client patch](https://ropatch.gnjoy.in.th/patchfile/2026-09-23_live_client_3565_3567_1790134356.rgz).

Current `PN-Client/DATA.INI` archives were read again. Its active `hateffectids.lub` and `hateffectinfo.lub` identify February 4, 2026 source builds; `footprinteffectinfo.lub` identifies January 21. Blossom and Sprout definitions exist, but all **18** referenced official artwork paths are missing. Flower Garden's **31** indexed artwork paths are missing too.

A candidate client asset set was recovered under the audit work directory `Server-Development/costume-stone-audit-20260928/cosmetic-client/thai-official`: **34 files / 372858 expanded bytes**, comprising 31 Flower Garden artwork files and three native effect tables. Source URLs, per-file SHA256 and sizes are in `thai-official-manifest.json`. Targeted HTTP range requests fetched only 72912 packed bytes. GRF 1.02 table names and payloads were decoded using the existing GRFEditor Utilities.DesDecryption implementation, then decompressed and checked against official entry lengths. Native Lua execution verified the resulting tables, including Flower Garden effect 291 and Blossom/Sprout 257/258. Nothing was installed.

The effect-table sources are [Thai September 23](https://ropatch.gnjoy.in.th/patchfile/2026-09-23_live_client_3546_3547_1789966713.gpf) and [Thai September 9](https://ropatch.gnjoy.in.th/patchfile/2026-09-09_live_client_3474_3474_1788761221.gpf). Korean latest metadata payloads were also fetched with bounded ranges, but they are Gravity-encrypted type128; the available parser cannot decode them. Their raw payloads and manifest remain under `cosmetic-client/official-raw` for inspection. Current `effectHatItemTable` is a list of costume-equipment IDs rather than an enchant-to-effect lookup, and does not resolve the 20th Anniversary enchant's missing script.


## Final native client evidence and bounded asset recovery

The three unique named native associations are now accepted as explicit **semantic inferences**, not recovered enchant lookup entries: 20th Anniversary **1001038 -> 311459 -> effect176**, Melody **1001927 -> 313724 -> effect173**, and Kiel **1001799 -> 300566 -> effect153**. The physical/card names and positions come from official metadata; the symbols, graphical paths and exported server constants establish each unique native graphical effect. Red Flame **1003031 -> 315313** remains unresolved and must not receive a speculative effect. Its [official collaboration animation](https://ro.gnjoy.in.th/ragnarok-x-lucky-flame-collaboration-event/) does not establish a numeric effect ID.

A fresh full effective PN-Client archive index contains241072 file entries. Melody's native STR and all7 textures are present; 20th Anniversary's STR and all16 textures are present. Each STR was parsed to its exact end (Melody15 layers/78 keyframes;20th28 layers/168 keyframes), and all images decoded successfully. Kiel's referenced STR and artwork were absent. The [official July8,2021 costume launch](https://ro.gnjoy.in.th/malangdo-update-limited-costume/) led to its [official Thai patch](https://ropatch.gnjoy.in.th/patchfile/2021-07-08_live_data_68_68_1625547070.gpf): bounded ranges recovered the native files. Only11 files are required for effect153: one STR and10 textures,4059448 expanded bytes. The STR parses fully (3 layers/12 keyframes) and every image dependency decodes. Exact paths, hashes and provenance are in `cosmetic-client/kiel-candidate-manifest.json`; three-effect checks are in `regional-effect-dependencies.json`.

The final footprint candidate contains51 files/925851 expanded bytes:31 official Flower Garden artwork files,18 Blossom/Sprout files from the matching April15 patch mirror, and2 Lua tables derived from the effective current tables. All current entries are retained, with only Flower Garden additions; native Lua deep comparison passed485 fields. The mirrored18 files match the official index paths and expanded sizes, but are not cryptographically authenticated to primary bytes. All44 footprint images decode; all5 STR files parse completely with present texture dependencies. The original latest Thai tables are retained as evidence only, because replacing current tables wholesale would remove prior entries.

Final resource audit covers all 996 catalogue metadata IDs, 94 distinct resource names and 376 inventory/collection/ground files. Four missing `Black_Winged_Apostle` resources were fetched directly from the [official January16,2025 patch](https://ropatch.gnjoy.in.th/patchfile/2025-01-16_live_client_1523_1523_1736821986.gpf), totaling26224 bytes; both BMPs decode and the SPR/ACT pair passes full indexed RLE and frame-reference validation. After the candidate files and the main agent's explicit four resource fallbacks, **zero resources are missing**. Those fallbacks use the official same-family footprint icon for1001616/313066 and official generic enchant `완력` for29046/29362. Item names, effects and existing descriptions are unaffected by icon fallback. Detailed checks: `full-resource-audit.json` and `icon-candidate-manifest.json`.

All candidate paths and manifests are under `Server-Development/costume-stone-audit-20260928/cosmetic-client`. This subagent installed no files and performed no in-game rendering test.


The full audit of all 51 catalogued cosmetic materials also caught three existing artwork gaps: basic footprint PNGs, Camellia (effect 40), and Ghost (effect 96). The two basic footprint PNGs were recovered directly from the [official August 9, 2024 Korean patch](http://ropatch.gnjoy.com/Patch/2024-08-09_live_data_2171_2173_1723182689.gpf). They total 8,552 bytes and both decode as 128×128 PNGs. The GRF 1.02 records use raw-data flag 0: decrypting yields native PNG bytes directly. `base-footprint-candidate-manifest.json` records hashes and exact paths. Those two remaining effects were subsequently recovered as explicitly documented reconstructions, described below.


## Taiwan supplemental visual matches

The official Taiwan physical/card pairs from the parallel metadata scan add Melody Notes upper **1001736 → 300548**, Released Power lower **1001752 → 313331**, Golden Aura lower **1002293 → 314198**, Digital Space lower **1002585 → 314802**, and Dark Lord lower **1002464 → 300714**. Their native associations are respectively effects **173, 161, 278, 87 and 185**, with semantic or visual inference explicitly recorded in the supplemental JSON. The [official Melody illustration](https://news.gnjoy.com.tw/Uploads2/2024/03/19/c6e38019eadc4a0188046c8fae83b601.png) uses the flat purple sharp, mint clef and orange double-note of effect 173, visibly distinct from effect 188's outlined glyphs. The [official Dark Lord illustration](https://news.gnjoy.com.tw/Uploads2/2025/10/14/5cc1174273e6415e8ba2cb2360556599.png) matches effect 185's blue flame ring and triangular star frames. Neither visual inference is a recovered direct enchant lookup.

Golden Aura's exact native artwork came from the [official Taiwan September 30, 2025 patch](http://twcdn.gnjoy.com.tw/ragnarok/patchfile/2025-09-30_live_client_3460_3467_1758863925.gpf): 55 required files, 26,227,208 expanded bytes (536,978 packed bytes fetched). Its STR parses completely with 17 layers and 63 keyframes; all 54 TGA dependencies decode. `golden-candidate-manifest.json` records exact paths and hashes. Both anniversary item resource families, `22Ani_Robe_D` and `23_Anni_Robe`, needed four icon/ground assets each. All eight exact native files were recovered from official Taiwan October 8, 2024 and October 14, 2025 patches (52,264 bytes); BMP decoding and SPR/ACT RLE/frame-reference checks passed. See `anniversary-candidate-manifest.json`.

The main agent reconstructed the two remaining legacy Ghost/Camellia effects transparently from a LATAM mirror's parsed STR fields and decoded texture images. These nine files are **derived reconstructions, not authenticated original bytes**. Independent review confirmed that the encoder matches the native 124-byte keyframe layout and the gateway exposes raw angle/blend fields; exact-end parsing and independent field-offset checks passed for Ghost (13 layers, 76 frames) and Camellia (15 layers, 66 frames). The reconstructed 24-bit BMPs preserve opaque RGB pixels and magenta-key transparency. The main agent's full round-trip checks and per-file hashes are in `reconstructed-candidate-manifest.json`.

With those candidates, the dynamic audit of every catalogued output script containing `hateffect` reports **zero missing native definitions or referenced STR/PNG dependencies** for the final catalogue of **497 materials, 56 visual mappings and 52 distinct effects**. The validator resolves active Renewal imports in order, applies field overlays, and scans all **996 supported material, enchant, alias and box IDs** from any working directory. It checks all categories, including stat stones carrying visual effects; built-in effect IDs are handled without a STR requirement. `audit_all_effects.py` and `all-cosmetic-effect-dependencies.json` capture this bounded asset validation. An in-game rendering check remains separate.

Final scope limitation: the 22nd Anniversary stone retains its verified numeric bonuses, while its advertised halo remains unsupported because an exact native association has not been established. Red Flame is also unresolved. Asset completeness applies to the 56 implemented visual mappings, not these unsupported visual claims.
