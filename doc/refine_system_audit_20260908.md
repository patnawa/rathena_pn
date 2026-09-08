# Refinement system audit — 2026-09-08

## Result

The Renewal refinement database is internally consistent across **160 target refine levels in nine equipment categories**. Two confirmed handler defects were corrected: an off-by-one success roll and allowing equipped/switch-registered items to be modified without refreshing their cached bonuses. Refinement now requires unequipped inventory gear; the player receives an explanation if the item is equipped or registered for switching. Requests made during trading, vending, buying-store use, or storage are rejected before payment.

This is a source/database and deterministic transaction audit. It is not a claim that every graphical client workflow has been played through. Native server compilation and live rollout status belong to the combined release report.

## Effective rules

`db/refine.yml` selects `db/re/refine.yml` under Renewal, then `db/import/refine.yml`. The inspected import contains only commented event examples and no active Body: event rates are **not enabled**. No economy values were changed.

| Equipment | Maximum | Guaranteed normal upgrades through |
|---|---:|---:|
| Armor level 1 | +20 | +4 |
| Armor level 2 | +20 | +3 |
| Weapon level 1 | +20 | +7 |
| Weapon level 2 | +20 | +6 |
| Weapon level 3 | +20 | +5 |
| Weapon level 4 | +20 | +4 |
| Weapon level 5 | +20 | +3 |
| Shadow armor / weapon | +10 | +4 |

Each YAML `Level` is the **target** refine level. The parser converts it to an internal zero-based key; `findLevelInfo` uses the current refine to obtain the next attempt and `findCurrentLevelInfo` subtracts one for current bonuses. Thus +7 → +8 uses the `Level: 8` recipe, not `Level: 7`.

For normal armor level 1, target +5/+6/+7/+8/+9/+10 success rates are 60/40/40/20/20/9%. The database stores rates out of 10,000. The UI shows integer percentages; current configured rates are whole percentages. Ordinary equipment offers Blacksmith Blessing protection for targets +8 through +14 with amounts 1, 2, 4, 7, 11, 16, 22. Shadow categories have no blessing requirement and therefore cannot request this protection through the native UI. Breaking and downgrade behavior is selected from the chosen ore recipe; neither is inferred merely from the ore's display name.

## Confirmed fixes

1. `cost->chance >= rnd() % 10000` wrongly succeeded on the boundary. A configured 0% could succeed once in 10,000 rolls and every nonmaximal chance gained 0.01 percentage points. The handler now uses `rnd() % 10000 < cost->chance`.
2. An equipped target could be refined through a forged/native request while the cached combat bonuses remained unchanged. Success and downgrade only changed `item.refine` and sent a result packet. Both selection and commit now reject equipped and equipment-switch targets. Script-based refiners that deliberately operate on equipped items retain their existing behavior.
3. The request handler now prevents inventory mutation during other economic interfaces. This guard does not rely on the client hiding buttons.

## Transaction review

The commit handler checks the open UI, inventory bounds/data, identification, broken state, current refine recipe, target equipment state, selected ore membership, blessing eligibility and quantity, and funds. Costs and refine are recalculated from the current inventory slot on every request; a packet cannot reuse an earlier cheaper recipe. A stale packet intentionally addresses the current slot, so this is not a persistent unique-ID selection lock. Forged or delayed requests cannot bypass the current recipe checks.

After validation, the non-yielding map-server handler deducts zeny and materials, then rolls success. Blessings are consumed on either outcome and prevent both destruction and downgrade on failure. Without protection, breaking is tested first and downgrade only applies if the item survives. Maximum level and unsupported categories return no recipe. Database inspection found no duplicate ore material IDs within an attempt and no ore/blessing identity collision; consequently the present recipes cannot double-book a material stack. The code has no generic refund for a hypothetical internal deletion failure after payment; normal single-threaded processing plus current distinct recipes make that unreachable through ordinary valid inventory requests, rather than proving arbitrary future custom recipes safe.

`feature.refineui` is on in the repository. Main Office's Master Refiner (`pn_train,32,50`) duplicates `Master Refiner#grademk` (`grademk,42,184`) and opens the native UI. Renewal Vestri and the ordinary town refiners also use this interface when enabled. Legacy enriched/HD script routes remain imported; their percentage comparisons use strict `> rand(100)`, unlike the corrected native off-by-one. Blessed Refiner and Ticket Refiner imports are commented out and are not presented as active services.

## Reproducible validation

Run with Python 3, PyYAML and g++:

```sh
python3 tools/ci/refine_transaction_test.py
```

The test resolves the Renewal/import database and checks category coverage, contiguous refine levels, rate bounds, nonnegative prices, blessing bounds, and unambiguous ore selection. It then extracts the **actual C++ request handler**, compiles it with deterministic inventory/payment/RNG doubles under UndefinedBehaviorSanitizer, and executes 110,000 roll outcomes, 13 rejection paths, insufficient-funds/max-refine cases, and failure destruction/protection/downgrade branches. Result: PASS. The inventory and packet transport doubles are deliberately small; this is not an integration test of SQL persistence or client rendering.

Before claiming full gameplay acceptance, exercise the live client for normal success, protected failure, unsupported equipment, insufficient materials, re-equipping after success, and switching registration rejection. Verify persistence after relog with disposable test equipment. These actions should never be performed on a player's valuable equipment as an audit experiment.

Primary implementation reference: [rAthena refinement packet implementation](https://github.com/rathena/rathena/blob/master/src/map/clif.cpp). The local fork and its effective configuration determine this audit; compatibility with another private server's economy was not assumed.
