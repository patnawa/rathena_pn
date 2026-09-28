# Thai costume stone reference — 2026-09-28

## Primary evidence and provenance

This audit verified 14 Thai enchant effects, 14 ordinary physical stones, and 14 guaranteed physical stones directly against the official Thailand client. It does not modify gameplay/source files.

The official [download page](https://ro.gnjoy.in.th/game_download/) links through [thank-you](https://ro.gnjoy.in.th/thankyou/) to the official download host. The still-linked [full client ZIP](https://rofull.gnjoy.in.th/RagnarokOnline.zip) contains `PatchClient/Lua Files/ServerInfoz/ServerInfo_TH.lub`; inspecting its strings establishes `ropatch.gnjoy.in.th`, `/patchinfo`, and `/patchfile`. HTTP Range requests retrieved only ZIP directory and small configuration records (not the multi-GB archive). The [official patch list](https://ropatch.gnjoy.in.th/patchinfo/patch.txt) provides the patch filenames.

Latest inspected official patch: [2026-09-23_live_client_3565_3567_1790134356.rgz](https://ropatch.gnjoy.in.th/patchfile/2026-09-23_live_client_3565_3567_1790134356.rgz). Extracted `System/itemInfo_new.lub` SHA256: `5030847f86483c5c12537ea5336a0321cb93774b07b5bbb5cdf27844421cf9db`. Descriptions and resource strings in this file are UTF-8. Local reproducible evidence is in `Server-Development/costume-stone-audit-20260928/`: original RGZ, `thai-itemInfo_new.lub`, `thai_official_items.json`, `thai_verified_mapping.json`, and `thai_items_extract.lua`.

## Exact mappings

The table matches official material and enchant names/effects from that client. The actual server NPC mapping is inferred from this exact name/effect agreement; server code is not distributed in the client. All 14 output IDs and the 14 ordinary physical IDs were absent from the inspected local `db` records. The guaranteed materials already exist locally in `db/re/item_db_etc.yml`.

| Family | Variant | Ordinary material | Guaranteed material | Output enchant | Enchant resource |
|---|---|---:|---:|---:|---|
| Purified | STR | 1001951 | 1002777 | 313738 | `Barmund_Pow` |
| Purified | AGI | 1001956 | 1002778 | 313743 | `Barmund_Sta` |
| Purified | LUK | 1001952 | 1002779 | 313739 | `Barmund_Crt` |
| Purified | DEX | 1001953 | 1002780 | 313740 | `Barmund_Con` |
| Purified | INT | 1001954 | 1002781 | 313741 | `Barmund_Spl` |
| Purified | VIT | 1001955 | 1002782 | 313742 | `Barmund_Wis` |
| Purified | Ultimate | 1001957 | 1002783 | 313744 | `Spl_Orb` |
| Festa | POW | 1002559 | 1002784 | 314797 | `Dragenergy_Red` |
| Festa | WIS | 1002561 | 1002785 | 314795 | `Dragenergy_Purple` |
| Festa | STA | 1002560 | 1002786 | 314798 | `Dragenergy_Gold` |
| Festa | SPL | 1002562 | 1002787 | 314799 | `Dragenergy_Blue` |
| Festa | CON | 1002563 | 1002788 | 314800 | `Dragenergy_Green` |
| Festa | CRT | 1002564 | 1002789 | 314801 | `Dragenergy_Silver` |
| Festa | Supreme | 1002558 | 1002790 | 314796 | `Auto_Orb_Merchant` |

Physical Purified resource: `Black_Winged_Apostle`. Physical Festa resource: `영혼의조각` (UTF-8 spelling in official metadata; encode appropriately for the active client resource tables). An English-client adaptation may reuse a verified installed stone icon if exact resources are unavailable; do not assume a resource file exists just because metadata names it.

## Effects and combo behavior

All effects below are independently corroborated by the official patch metadata. The [Purified system announcement](https://ro.gnjoy.in.th/enchant-purified-option/) additionally supplies seven item effect images, saved locally for inspection.

Purified ordinary variants: after-cast delay -10%; physical and magical damage against all enemy types +10%. The Thai term is `ทุกประเภท` (all types/classes), not `ทุกเผ่า` (all races). Existing local Loft uses `bAddClass,Class_All` and `bMagicAddClass,Class_All`; those are the corresponding proposed bonus categories. Add the following per variant:

| Variant | Additional effect |
|---|---|
| STR | ATK +100 |
| AGI | HIT +60 |
| LUK | Critical damage +20% |
| DEX | Long-ranged physical damage +10% |
| INT | MATK +100 |
| VIT | MaxHP +20% |

Purified Ultimate: after-cast delay -20%; physical/magical damage against all enemy types +20%; ATK/MATK +100; HIT +60; long-ranged physical damage +10%; MaxHP +20%; critical damage +20%.

Festa ordinary variants all reduce after-cast delay (Thai client calls it Global Cooldown) by 5%. Additional effects:

| Variant | Additional effect |
|---|---|
| POW | POW +5; P.ATK +5; short-ranged physical damage +10% |
| STA | STA +5; P.ATK/S.MATK +5; RES +25; MaxHP +5% |
| WIS | WIS +5; P.ATK/S.MATK +5; MRES +25; MaxSP +5% |
| SPL | SPL +5; S.MATK +5; MATK +10% |
| CON | CON +5; P.ATK +5; long-ranged physical damage +10% |
| CRT | CRT +5; P.ATK +5; critical damage +10% |

Festa Supreme: all six traits +10; P.ATK/S.MATK +10; after-cast delay -10%; MaxHP/MaxSP +10%; short- and long-ranged physical damage +10%; MATK +10%; grants Greed Lv.1. [Official Supreme item image](https://img.gnjoy.in.th/2025/12/Ragnarok-Festa-Stone-Supreme-Upper.jpg)

No combo conditions appear in any of the 14 official output descriptions. Purified is not a rename/alias of Loft 25934–25940. Its base bonuses are doubled relative to local Loft, and the [official 6th-anniversary announcement](https://ro.gnjoy.in.th/6th-anniversary-scroll/) explicitly says selected costumes' Loft doubling does not apply to Purified. Do not include Purified in a Loft-doubling equipment combo.

`thai_verified_mapping.json` includes proposed rAthena scripts, transcribed from these effects. These are implementation proposals to validate against server bonus names/runtime, not official server scripts.

## Equipment positions, slots, and success rates

Festa belongs to costume upper and explicitly occupies displayed slot 2 (zero-based card index 1), including multi-position costumes. The [17 December 2025 patch](https://ro.gnjoy.in.th/patch-update-17-december-2025/) confirms this; its `Festa-Stone_Enc-3.jpg` shows the second of four slots occupied. Ordinary Festa success is 50%; failure destroys the stone and any existing slot-2 enchant. Purified belongs to costume garment, has ordinary success 50%, and is offered by Lace La Zard according to the [Purified announcement](https://ro.gnjoy.in.th/enchant-purified-option/). That page and inspected material descriptions do not specify a numeric Purified slot, so the mapping JSON intentionally leaves it null: choose the server's established garment-enchant slot as a documented compatibility decision unless stronger primary slot evidence is found.

The [transfer system](https://ro.gnjoy.in.th/costume-enchant-transfer-system/) creates the guaranteed materials from the corresponding enchants. Reapplication is 100%, replaces only the designated slot, and preserves unrelated slots. Extraction success is 50% for six ordinary variants and 15% for Ultimate/Supreme; failed extraction preserves costume/enchant and consumes only the transfer ticket. These extraction rates are not application rates for the guaranteed materials.

## Implementation boundary

All 28 physical stone definitions and 14 output IDs are now verified from primary client evidence. Normal physical materials 1001951–1001957 and 1002558–1002564 should be included alongside guaranteed materials if the objective is every verified regional material. These 28 share 14 effects; count materials and effects separately. Runtime bonus scripts, actual client icon-file presence, and Purified numeric-slot compatibility remain implementation verification steps, not reasons to substitute existing Loft effects or fabricate IDs.


## Prepared integration inputs

The audit directory additionally contains `thai_item_records.yml` / `thai_item_records.json` (42 proposed database records) and `thai_client_english.json` (42 English client entries with official resource names). New Aegis names in those inputs are locally chosen implementation names because the official client metadata does not provide server Aegis names; the existing guaranteed-material Aegis names are preserved. These are staging inputs, not installed gameplay changes. The plain Festa material weighs 10; all Purified and guaranteed materials weigh 1 in the official client.

The stock local `npc/re/merchants/malangdo_costume.txt` Lace La Zard implementation assigns garment enchants to zero-based card index 0. Retaining that slot for Purified is consistent with the server's existing garment convention; it remains a compatibility decision, not independently verified numeric slot behavior from the Thai server. Festa zero-based index 1 is explicitly verified and should remain separate from ordinary upper enchants in index 0.


## Finite full-client reconciliation

A second pass extracted all **18,447** official Thai item records into `thai_all_items.json` and reconciled them against the then-current **479** local physical-material catalogue and **439** Korean primary-metadata candidates. The Thai dump contains **365** of the already catalogued physical IDs. Descriptor/name/position searches produced 913 broad candidates before removing equipment, boxes, unrelated enchant materials, and output cards. A second scan covered all items using the five known physical-stone resource names (`영혼의조각`, `발자국이펙트`, `블루크리스탈조각`, `스노우플립`, `Black_Winged_Apostle`); remaining Stone-named records were inspected separately. This found seven additional stat materials and the four cosmetic materials already tracked by the cosmetic audit, with no other verified stat-material omissions in this finite dump.

| Material | Variant | Correct output enchant |
|---:|---|---:|
| 25934 | Loft STR | 27427 |
| 25935 | Loft LUK | 27428 |
| 25936 | Loft DEX | 27429 |
| 25937 | Loft INT | 27430 |
| 25938 | Loft VIT | 27431 |
| 25939 | Loft AGI | 27432 |
| 25940 | Loft Ultimate | 27433 |

The primary metadata distinguishes the physical Loft stones (weight 1, `영혼의조각`) from their output enchants (no weight line, elemental stone resources). Local `25934–25940` incorrectly serve as enchant definitions; preserve their existing type/scripts for backward compatibility with already equipped IDs, add the correct output IDs, and permit physical inputs through the custom NPC. Existing equipped legacy IDs need reverse recovery aliases. `thai_loft_proposal.json` provides seven output item records, seven mappings/legacy aliases, and fourteen English client records. No local Loft equipment combos were found to mirror.

The four remaining cosmetic material IDs are 1000573 Firework Upper, 1001799 Kiel Lower, 1001927 Melody Notes Lower, and 1003031 Red Flame Upper; their effect associations belong to the cosmetic audit. `thai_catalogue_gaps.json` captures this finite reconciliation. It does not assert worldwide completeness or infer a separate Classic server's catalogue from Renewal data.
