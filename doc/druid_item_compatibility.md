# Druid-family enchant item compatibility - 2026-09-06

This import adds 28 enchant cards and three non-usable enchant materials. It is
an explicitly **MuhRO-reference-compatible implementation**, not a claim that
these effects or display labels have been independently verified against current
official Gravity balance.

## Identity and effect provenance

Exact Aegis-name-to-ID mappings were read from the translation project's own
[ItemDBNameTbl.lub](https://github.com/llchrisll/ROenglishRE/blob/66cdfec631603fda6a90ba4bbe26ab07b5204c84/Additions/data/luafiles514/lua%20files/ItemDBNameTbl.lub),
pinned to ROenglishRE commit `66cdfec631603fda6a90ba4bbe26ab07b5204c84`
(2026-08-05). That file is plaintext, 141,741 bytes, SHA256
`686e6d9d8cffb49b8e149c6dc81cb564893412e893956c562df860e97395368c`.

| Aegis names, inclusive | Exact IDs, inclusive |
|---|---|
| Automatic_Orb99-101 | 314271-314273 |
| Wolf_Orb_Skill_52-54 | 314274-314276 |
| Glacier_F_Orb_192-210 | 314277-314295 |
| Ice_F_Orb_Skill_55-57 | 314296-314298 |
| Ice_F_Stone_Skill_55-57 | 1002350-1002352 |

Effects were independently read by these numeric IDs from the user-supplied
`MuhRO/MuhRO/System` directory, without extracting or importing protected GRFs:

- `itemInfo_EN_db_fallback.lua`, SHA256
  `235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a`:
  enchant descriptions for IDs 314271-314298.
- `itemInfo_EN_db.lua`, SHA256
  `2834833d0438219b46e03224e19ebe94f0b018c725aa43dab7df94fe5ef4a4e6`:
  material descriptions for IDs 1002350-1002352 and reused icon-resource names.

Resource names were **not** treated as Aegis identities. For example, multiple
new items reuse old numbered orb/stone resource names. The new English display
labels are clean-room compatibility labels based on their skills, not asserted
official item names. The pinned ROenglishRE main itemInfo lacked these records;
it was not used as evidence for their effects.

## Exact effect rules

All percentages below are skill-damage bonuses. Refinement comes from the host
equipment through `getrefine()` in the enchant-card execution context.
Threshold bonuses are cumulative.

Three recurring skill sets, in numeric item order:

1. `KR_DOUBLE_SLASH`, `KR_CHOP_CHOP`.
2. `KR_SHARPEN_HAIL`, `KR_SHARPEN_GUST`.
3. `KR_ICE_SPLASH`, `KR_THUNDERING_ORB`, `KR_EARTH_STAMP`.

| Family | Bonus on each skill in the corresponding set |
|---|---|
| Automatic 99-101 | 15%; +3% at refine 9; +7% at refine 11 |
| Wolf 52-54 | 15%; +15% at refine 9; +15% at refine 11 |
| Ice 55-57 | 15%; +15% at each of refine 7, 9 and 11 |

The new Wolf descriptions have **no refine-7 bonus**, unlike some earlier Wolf
items. Their refine-11 total is 45%, not 60%.

At refine 11 the three Ice enchants additionally give 15% to these skill sets,
respectively:

1. `AT_ALPHA_CLAW`, `AT_FRENZY_FANG`.
2. `AT_PINION_SHOT`, `AT_QUILL_SPEAR`.
3. `AT_GLACIER_SHARD`, `AT_ROARING_PIERCER`, `AT_TERRA_HARVEST`.

Glacier 192-201 grant `20 + 10 * floor(refine / 3)`, plus 20 at refine 9 and
another 20 at refine 11. Their single skills in order are Chop Chop, Double
Slash, Sharpen Gust, Sharpen Hail, Feather Sprinkle, Ice Splash, Thundering Orb,
Thundering Focus, Earth Drill and Earth Stamp.

Glacier 202-210 grant `10 + 5 * floor(refine / 4)`, plus 10 at refine 9. Their
skills in order are Glacial Nova, Glacial Shard, Roaring Piercer, Terra Wave,
Terra Harvest, Quill Spear, Pinion Shot, Frenzy Fang, and the three-skill set
Primal Claw/Feral Claw/Alpha Claw. Orb 210 applies the full formula to each of
its three skills.

Enhanced wind/Quill Spear damage variants use the engine's
`skill_dummy2skill_id` parent mapping for equipment bonuses. Items deliberately
register only the parent skill once, avoiding double bonuses if canonicalized.

## Integration and limits

- All 31 IDs were absent from effective Renewal item imports before addition.
  The new file is imported exactly once, with `Mode: Renewal`.
- Enchants have `Type: Card`, `SubType: Enchant`, and no physical card location.
  Materials have `Type: Etc`, no use script, and no direct combat effect.
- No purchase/sale prices, weights, drop rates, trade flags, equipment masks,
  acquisition shops or currencies were guessed. Omitted price/weight properties
  retain the item database's zero defaults.
- No enchant recipes or current-client files are modified by this item import.
  Separate initial recipes are now provided by `druid_item_enchant.yml` (see
  [recipe scope](druid_item_enchant_compatibility.md)); material acquisition is
  still incomplete. Defining an item alone does not make it obtainable.
- Correction: all 31 names and IDs already exist in the original active client
  table (SHA256 `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496`).
  Earlier unresolved results were missing server records, not client aliases.
  `tools/ci/audit_druid_item_names.py` verifies this without altering client data.
- `client-patch/druid_items` supplies optional clean-room item metadata with the
  already-used `EpisodClear20` fallback resource. It contains no proprietary
  artwork and is not installed automatically by the item import. The fragment
  was separately installed in the active client on 2026-09-06 with backups.

Run the focused test from the repository root on Linux/WSL:

```sh
python3 tools/ci/druid_item_db_test.py
```

The test checks exact identities, unique effective imports, types/defaults,
loaded symbolic skills and client labels. A restricted translation of these
item scripts compiles with AddressSanitizer and UndefinedBehaviorSanitizer and
checks all 28 enchant effects at refine 0-20: 588 item/refine cases against
independent expected totals. It also compiles the actual current
`skill_dummy2skill_id` body and checks six enhanced mappings and their six
parent identities.

These checks do not execute rAthena's full script VM, player equipment
calculation, damage packets, native client windows or inventory persistence.
Server startup and attached-player checks remain required before claiming
end-to-end gameplay verification.
