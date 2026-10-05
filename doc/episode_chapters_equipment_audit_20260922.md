# Episode 21 and Chapter 1/2 equipment audit — 2026-09-22

## Result

No confirmed equipment-route defect was found in the inspected server and installed client. The read-only `tools/ci/episode_chapters_equipment_audit.py` check passed 1,506 assertions with zero violations. This is a source, database, and client-resource audit, not a character playthrough or a proof of every combat bonus at runtime.

One audit-tool defect was reproduced and fixed: `tools/ci/chapter2_current_client_audit.py` assumed the installed client was two directories above the repository, so it failed with `KeyError: 'Data'` in this workspace. It now finds the installed `PN-Client` and passes without a manual module override.

## Coverage and evidence

| Route | Checked result |
| --- | --- |
| Episode 21 Gaebolg | All ten barter pieces are defined and targeted by enchant groups 143–146. The three hammer reform tools and their recipe materials resolve. |
| Episode 21 Yorscalp | Both crown barter pieces and ten Gaebolg-to-Yorscalp reform outputs are defined. All twelve pieces are targeted by groups 152–156. The three Yorscalp materials occur in the `AEGIS_103537` antiquity item group; the Final Battle reward grants that box. |
| Chapter 1 | Six Entwined and five Dimension footwear barter pieces are defined and targeted by groups 160–163. `Imperfect_Rune` reform outputs are printed **materials** (`Type: Etc`), not a second set of wearable gear. |
| Chapter 2 | Eleven Azure pieces are sold by the Flame Coin Exchanger; all eleven Azure-to-Blaze pairs keep the same equipment type, location, and card-slot count. Enchant groups 167–171 target all 22 Azure/Blaze pieces. |
| Installed client | All 55 gear IDs across these routes have loaded `SystemEN/itemInfo.lua` metadata. The active `enchant_repair.grf` client tables match the server's initial, upgrade, and perfect recipes for all eighteen inspected groups (143–146, 152–156, 160–163, 167–171). The Chapter 2 native audit confirms 22 targets and 58 exact perfect recipes and validates its installed map walkability. |

The live map container was healthy when checked. Its inspected item, barter, enchant, and reform files matched this checkout by SHA-256. The live `npc/custom/chapter2/Chapter2.txt` differs from the checkout only in the file header and one explanatory comment; the executable NPC body is the same. The live Git HEAD differs from the checkout, so this file comparison is the relevant evidence for the inspected equipment routes.

The Chapter 2 client tooltips are intentionally terse (for example, the Azure armor says its bonuses increase with refine and grade). This audit confirms those IDs and enchant recipes are present, but does not certify that every player-visible stat description fully enumerates each server-side bonus. A mounted-client inspection and equipping/refining representative items on a test character remain the direct way to verify display and combat behavior.

## Reproduction

From the repository root in WSL, with the installed `PN-Client` in this workspace:

```sh
python3 tools/ci/episode_chapters_equipment_audit.py
python3 tools/ci/chapter2_current_client_audit.py
PYTHONPATH=tools/ci python3 -m unittest dimension_equipment_test equipment_progression_test episode21_family_supply_test chapter2_progression_test
```

The four existing suites ran 12 tests and passed. The audit script follows Renewal imports and cumulative enchant overlays; interpreting the final imported patch as a complete replacement incorrectly reports five Chapter 1 Dimension targets missing.
