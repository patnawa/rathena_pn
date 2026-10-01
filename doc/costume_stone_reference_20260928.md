# Costume stone reference audit — 2026-09-28

## Follow-up: recovered official client evidence

The initial website-only limitations below were superseded by a local official-client cache found during the expanded implementation work. Its provenance is the [official September 7, 2026 client patch](http://ropatch.gnjoy.com/Patch/2026-09-07_live_client_3363_3364_1788764913.rgz), recorded by the existing `client-compat-update-20260909/latest-iteminfo-source.json`. The compiled `System/itemInfo_true.lub` was read with the bundled Lua 5.1 runtime, and Korean strings decoded as CP949. The proposal records include the source file SHA-256.

The complete descriptions resolve all sixteen Box 44 output enchant IDs to **315324–315339**, in the same order as physical IDs **1003053–1003068**. They also identify twelve additional missing class materials from Box 43: physical **1002831–1002833, 1002843–1002851** map in that order to enchant **315057–315068**. The two proposals contain 28 physical records, 28 enchant records, and 54 two-item combos, including interactions with older garment stones. They are research proposals; the companion implementation is responsible for installing and testing them.

Supporting artifacts:

- `costume_box44_official_20260928.json`: exact Korean names/descriptions and resources, all 32 physical/output records.
- `costume_box44_proposal_20260928.json`: translated scripts, sixteen pairs, thirty combos, validation of all 37 referenced skill identifiers.
- `costume_box43_official_20260928.json`: exact Korean names/descriptions and resources, all 24 physical/output records.
- `costume_box43_proposal_20260928.json`: translated scripts, twelve pairs, twenty-four combos, validation of all 36 referenced skill identifiers.
- `costume_existing18_official_20260928.json`: exact descriptions for output enchants 314661–314678.
- `costume_client_metadata_20260928.json`: English descriptions for 92 client entries (28 new pairs and 18 existing pairs); resource names preserved as base64-encoded CP949 bytes.
- `costume_new_stone_math_cases_20260928.json`: 138 constant/zero/one/maximum-level arithmetic expectations, covering 28 skill-dependent records. These are expected values, not an in-game execution result.
- `costume_official_material_candidates_20260928.json`: 439 official client materials selected by Malangdo NPC navigation, costume/slot description, and exclusion of boxes. This is a reproducible catalog boundary, not a global completeness proof. Compare against effective imported databases, not only the base item file.

The 439-entry scan additionally located Cherry Blossom and Sprout footprint materials 1002855/1002856. Druid-family materials 1002625–1002632 were absent from the base file but require checking the server's existing imports before treating them as missing. Cosmetics and Thai regional implementation evidence is tracked separately.

Everything below records the earlier, narrower investigation and should be read with this update.

## Scope and conclusion

This is a primary-source reference check for the Fashion Points costume-stone audit. It covers the current Korean catalog, the current Thai release and regional Thai stones, selected international/Japanese evidence, and the local/current upstream rAthena item database. It is **not proof of an exhaustive worldwide catalog**: regions release different catalogs and use different names and systems. No new item effects, client data, or mappings were invented during this research.

The strongest concrete gap beyond the local database is **16 class stones in Korean Enchant Stone Box 44**. They are official items, but their physical item IDs are absent from both the local item database inspected and upstream `master` fetched on this date. Adding only NPC menu entries for those IDs would not implement working stones.

## Latest Korean reference

The official probability catalog lists Box 44 dated 2026-07-14, following Box 43 dated 2026-04-14 and Box 42 dated 2026-01-20. Box 44 was the latest entry found under the official costume-stone category on the audit date. [Official catalog](https://probability.gnjoy.com/RO/LIST1?SearchText=%EC%9D%98%EC%83%81+%EC%9D%B8%EC%B1%88%ED%8A%B8+%EC%8A%A4%ED%86%A4+%EC%83%81%EC%9E%90)

These 16 IDs come directly from the named item links in the official probability table. English class names below are translations; the Korean names and linked physical IDs are authoritative. Individual item detail pages for these IDs returned no readable description, so their complete effects and enchant-card IDs remain unverified. [Box 44 table](https://probability.gnjoy.com/RO/LIST1/PAC/0682)

| Physical ID | Stone | Position |
|---|---|---|
| 1003053 | Windhawk II | Garment |
| 1003054 | Ranger II | Upper |
| 1003055 | Ranger II | Middle |
| 1003056 | Ranger II | Lower |
| 1003057 | Elemental Master II | Garment |
| 1003058 | Sorcerer II | Upper |
| 1003059 | Sorcerer II | Middle |
| 1003060 | Sorcerer II | Lower |
| 1003061 | Shinkiro II | Garment |
| 1003062 | Kagerou II | Upper |
| 1003063 | Kagerou II | Middle |
| 1003064 | Kagerou II | Lower |
| 1003065 | Shiranui II | Garment |
| 1003066 | Oboro II | Upper |
| 1003067 | Oboro II | Middle |
| 1003068 | Oboro II | Lower |

An example exact official target is [Windhawk II, physical ID 1003053](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=1003053&itemSeq=7). The remaining targets use the listed ID with the same URL pattern.

Names alone are unsafe for matching: the local database already contains older “Sorcerer Stone II” upper/middle/lower names for a different class generation. Compare exact physical ID, output enchant ID, position, and effects.

## Thai coverage and regional items

Thailand's official Box 33 update is dated August 13, 2026. Its list includes Hit Physical stones in all five positions (upper, middle, lower, garment, dual), plus Abyss Chaser, Troubadour & Trouvere, and Hyper Novice garment stones. This supports checking all five positions rather than treating a family name as a single stone. [Thai Box 33](https://ro.gnjoy.in.th/update-enchant-stone-box-33/)

Thai Purified garment and Ragnarok Festa upper stones are official regional families. The official transfer system removes them into 100% enchant materials and lists six stat versions plus an Ultimate/Supreme version for each family. [Thai transfer system](https://ro.gnjoy.in.th/costume-enchant-transfer-system/)

The local physical IDs are:

| Family | Physical IDs and variants |
|---|---|
| 100% Purified garment | 1002777 STR; 1002778 AGI; 1002779 LUK; 1002780 DEX; 1002781 INT; 1002782 VIT; 1002783 Ultimate |
| 100% Ragnarok Festa upper | 1002784 POW; 1002785 WIS; 1002786 STA; 1002787 SPL; 1002788 CON; 1002789 CRT; 1002790 Supreme |

These records exist in `db/re/item_db_etc.yml`, but this check found no matching Purified/Festa enchant-card implementations by name. Existing Loft enchants 25934–25940 are not a verified substitute. The official Thai system describes extracting a Loft-enchanted costume into a Purified stone and separately applying Purified enchants, so those families must not be silently conflated. [Purified system](https://ro.gnjoy.in.th/enchant-purified-option/)

## Existing functional candidates omitted from the custom NPC

The following mappings are strongly supported by matching physical-stone position/effect descriptions with the local Aegis names and implemented enchant scripts. This is a semantic mapping check, not a claim that upstream's stock NPC already includes the pairs.

| Physical stone | Output enchant | Evidence |
|---|---|---|
| 25140 Resurrection, lower | 29147 `Resurrection` | Official lower stone grants Resurrection level 1; local card grants `ALL_RESURRECTION`, level 1. [Official item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25140&itemSeq=7) |
| 25207 upgraded SP drain, upper | 29208 `SPdrain2_Top` | Official: 2% chance to absorb 1% physical damage as SP; local script `bSPDrainRate,20,1`. [Official item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25207&itemSeq=7) |
| 25208 upgraded SP drain, garment | 29209 `SPdrain2` | Same official effect, garment position. [Official item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25208&itemSeq=7) |
| 25209 upgraded HP drain, garment | 29210 `HPdrain23` | Official: 2% chance to absorb 3% physical damage as HP; local script `bHPDrainRate,20,3`. [Official item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25209&itemSeq=7) |
| 25210 upgraded HP drain, upper | 29211 `HPdrain23_Top` | Same official effect, upper position. [Official item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25210&itemSeq=7) |
| 1002255 Greed, upper | 314175 `Greed_T` | Local physical/card Aegis position names agree; card grants `BS_GREED`, level 1. |
| 1002256 Greed, middle | 314176 `Greed_M` | Local physical/card Aegis position names agree; card grants `BS_GREED`, level 1. |

Local implementation evidence is `db/re/item_db_etc.yml`; compare the [upstream item database](https://github.com/rathena/rathena/blob/master/db/re/item_db_etc.yml). The Greed official item-detail URLs did not return readable data, so their mapping confidence rests on the source database rather than a fetched official description.

## Visual effects require a separate support check

Costume stones are not limited to items containing “Stone” in their English name. Twinkle upper item 25058 is officially applied in the costume's fourth slot and produces a visual effect. [Official Twinkle item](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=25058&itemSeq=7)

The stock local Malangdo script explicitly marks the fourth-slot enchanter as missing and lists 12 effect materials (25058, 25059, 25136, 25137, 25138, 25176, 25177, 25178, 25205, 25224, 25225, 25226). This is a known implementation boundary, not evidence that these are invalid costume materials. [Upstream stock NPC](https://github.com/rathena/rathena/blob/master/npc/re/merchants/malangdo_costume.txt)

Japanese RO also explicitly describes effect stones attached to costume equipment, using a charm-based acquisition flow. Thus a worldwide claim must address effects and regional systems, not only the Korean stat-stone catalog. [Official Japanese costume event explanation](https://ragnarokonline.gungho.jp/special/ragcolle2020/)

## What can be asserted after the NPC patch

Use a bounded statement such as “all verified functional costume-stone mappings supported by this server's database,” accompanied by the audit count and exception list. Do not claim every stone worldwide until all these have been addressed:

- All 16 Box 44 physical definitions, output enchant IDs, skill/combo effects, and client descriptions.
- Thai regional Purified/Festa effect implementations and exact output IDs.
- Cosmetic fourth-slot/effect materials, including client effect support and slot preservation.
- A complete region-by-region catalog reconciliation beyond the sources sampled here.

The live NPC, deployment state, complete inventory comparison, and runtime checks belong to the companion local audit. This reference file records findings before those fixes and does not itself modify server behavior.
