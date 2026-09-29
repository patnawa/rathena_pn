# Practical cash shop consumables — 2026-09-29

User requested useful usable items and no costumes. The resulting shop contains 90 listings:
72 retained kRO-derived boxes and 18 new PN-priced consumables/boxes.
Existing prices and item effects are unchanged. No client patch is needed: all 90 item IDs
are present in the published client item table.

This is a PN selection inspired by Thai RO consumable categories, not a reproduction of
the current Thai cash shop or its regional prices, restrictions, and promotion schedule.
Thai permanent announcements confirm magic amplification scrolls and recovery supplies:
https://ro.gnjoy.in.th/cash-item-update-13-feb-2025/
https://ro.gnjoy.in.th/cash-shop-new-update/
Thai HE Bubble Gum was also offered in a limited 50-pack promotion, which ended March 4,
2026: https://ro.gnjoy.in.th/cash-item-special-package/
The PN HE boxes below contain 10 each, without that expired promotion's purchase limits.

Current Thai-only Full Chemical Protection Scroll, Force/Speed Booster packages,
Golden Angeling Coin packages, Zonda services, and event passes were not added because
their complete corresponding implementations are unavailable locally. Costume packages
and random cosmetic scrolls were excluded. The six prior cosmetic listings removed are:
- CCloth_Dye_Coupon2_Box
- CCloth_Dye_Coupon_Box
- C_Alice_Scroll_Box10
- C_Disguise_Croll_Box10
- C_Mimic_Scroll_Box10
- C_New_Style_Box

## Additions

Prices are Cash Points per purchase. Singles use the same item IDs as the existing box rewards.

| ID | Item | Points | Price basis |
|---|---|---:|---|
| 14539 | Shadow Armor Scroll | 80 | Rounded-up per-unit price from C_S_ArmorBox10 (2400 / 30). |
| 14540 | Holy Armor Scroll | 80 | Rounded-up per-unit price from C_Holy_Armor_S_Box30 (2400 / 30). |
| 14593 | Mystical Amplification Scroll | 26 | Rounded-up per-unit price from C_MP_Scroll_Box50 (1300 / 50). |
| 14600 | Mental Potion | 50 | Rounded-up per-unit price from C_Mental_Potion50_Box (2500 / 50). |
| 12215 | LV10 Blessing Scroll | 22 | Rounded-up per-unit price from C_Blessing_10_Box50 (1100 / 50). |
| 12216 | LV10 Agil Scroll | 22 | Rounded-up per-unit price from C_Inc_Agi_10_Box50 (1100 / 50). |
| 12645 | J Aspersio 5 Scroll C | 22 | Rounded-up per-unit price from C_Aspersio_5_Box50 (1100 / 50). |
| 14536 | Abrasive | 190 | Rounded-up per-unit price from C_Abrasive_Box10 (1900 / 10). |
| 14537 | Regeneration Potion | 60 | Rounded-up per-unit price from C_Regeneration_Box10 (600 / 10). |
| 12832 | Mysterious Water | 50 | Rounded-up per-unit price from C_Myst_Water_Box50 (2500 / 50). |
| 14534 | Small Life Potion | 28 | Rounded-up per-unit price from C_S_Life_Potion_Box50 (1400 / 50). |
| 14535 | Medium Life Potion | 48 | Rounded-up per-unit price from C_Med_Life_Potion_Box50 (2400 / 50). |
| 12578 | Rapid Life Water | 250 | Rounded-up per-unit price from C_Rapid_Life_Water_Box2 (2500 / 10). |
| 12212 | Giant Fly Wing | 16 | Rounded-up per-unit price from C_Giant_Fly_Wing_Box100 (1600 / 100). |
| 12411 | HE Battle Manual | 1600 | PN price: twice the existing 800-point regular single manual. |
| 12412 | HE Bubble Gum | 1000 | PN price: twice the existing 500-point regular single gum. |
| 16267 | HE Battle Manual Box | 16000 | 10 HE manuals at the single-item price; no bulk discount. |
| 16268 | HE Bubble Gum Box | 10000 | 10 HE gums at the single-item price; no bulk discount. |

HE effects retain the server's existing scripts: SC_EXPBOOST or SC_ITEMBOOST, value 200,
duration 900000 ms (15 minutes). Do not combine duration or magnitude claims with the
different Thai hour-long promotional variant. Runtime stacking and map restrictions
continue to follow existing server status/skill rules.

Validation: unique listing IDs; positive prices; loaded nonempty consumable scripts;
HE box contents; published-client item coverage; cosmetic-name exclusion; actual
map-server startup with isolated disposable SQL. Live purchase/use UI remains untested.
