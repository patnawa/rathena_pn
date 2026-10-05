# Costume source verification — 2026-09-28

Read-only source-evidence review of the integrated `db/import/fashion_stone_expansion_items.yml`, `fashion_stone_expansion_combos.yml`, and `npc/custom/fashion_points/stone_catalogue.json`.

## Result

No source discrepancy found for the **28 Box 43/44 physical items, 28 output enchants, 54 combo records, or 28 catalogue positions/pairs** reviewed. Integrated records exactly match the research proposals after whitespace normalization. A fresh reading of every original Korean enchant description also agrees with the base and combo bonuses, skill dependencies, amounts, damage categories, and placement encoded by those proposals.

The original descriptions are extracted in `costume_box43_official_20260928.json` and `costume_box44_official_20260928.json`. The primary source is the cached official `System/itemInfo_true.lub` from the [September 7, 2026 Korean client patch](http://ropatch.gnjoy.com/Patch/2026-09-07_live_client_3363_3364_1788764913.rgz).

Specific checks included:

- Fixed cast reductions use milliseconds: 0.1 seconds is 100; 0.5 seconds is 500. Skill cooldown reductions use the same unit.
- Every-two-level bonuses multiply after integer division, preserving thresholds at odd skill levels.
- Physical damage against all target elements uses `bAddEle,Ele_All`; magic damage against all target elements uses `bMagicAddEle,Ele_All`; damage of specified magic elements uses `bMagicAtkEle`.
- CRI and C.RATE remain distinct (`bCritical` versus `bCRate`). Shinkiro II plus Kagerou II lower grants both skill-dependent CRI and melee damage, while its middle combo grants fixed C.RATE.
- Combos referring to older garment generations use the pre-existing garment Aegis names and do not accidentally refer to the new fourth-class garments.
- Physical weight 100 represents the source's displayed weight 10.
- All new physical/output pairs are in the correct Upper/Middle/Lower/Garment catalogue category. The catalogue contained 451 pairs at review time.

## Three older materials

The official client independently confirms the newly included older mappings:

| Material | Position | Output | Official effect |
|---|---|---|---|
| 6716 | Upper | 4926 | CRI +1 |
| 6717 | Middle | 4927 | MaxHP +50 |
| 6718 | Lower | 4928 | MaxSP +10 |

The output scripts implement these exact values. Their material records are historically `Type: Card` in the base database, while the outputs are enchant cards; a material scanner restricted to `Type: Etc` would miss these three.

## Test-evidence qualification

The 138 entries in `costume_new_stone_math_cases_20260928.json` were **mechanically evaluated from the proposed scripts** and `skill_db.yml` maximum levels. They are useful arithmetic fixtures, but they are **not an independent oracle derived from the original descriptions** and **not runtime tests**. Their status field now says this explicitly.

As separate source-derived threshold examples for any runtime checks:

| Source condition | Skill level | Expected added effect |
|---|---:|---|
| Ranger II upper; one percent delay reduction per two Arrow Storm levels | 1 / 2 / 10 | 0% / -1% / -5% delay |
| Ranger II lower; three percent size damage per two Aimed Bolt levels | 1 / 2 / 10 | 0% / +3% / +15% physical size damage |
| Sorcerer II upper; 0.1-second fixed cast reduction per Psychic Wave level | 0 / 1 / 5 | 0 / -100 / -500 milliseconds |
| Shinkiro II + Kagerou II lower; per two Shadow Dance levels | 1 / 2 / 10 | CRI 0/+1/+5 and melee damage 0%/+3%/+15% |
| Night Watch II + Rebellion II lower; per two P.F.I levels | 1 / 2 / 10 | fixed cast 0/-100/-500 ms and physical size damage 0%/+3%/+15% |

These examples are read directly from the original description statements and evaluated at the displayed input levels; they do not assert any test has run. Runtime behavior, client rendering/assets, NPC transactions, regional Thai effects, and the complete cosmetic catalogue remain outside this source-only review.
