# Druid-family initial enchant recipes - 2026-09-06

This overlay adds **63 selectable initial-enchant recipes** for the verified
Druid-family item definitions and rebalances one Gray Wolf normal distribution.
It does not add new equipment targets, vendors, item drops or client metadata.

## Source and identity provenance

Recipes and weights were parsed directly from the active client's extracted
`data/luafiles514/lua files/Enchant/EnchantList.lub`, whose header is dated
2026-03-22. The 1,056,907-byte plaintext file has SHA256
`664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d`.
No material amounts or zeny prices were inferred from another server.

The original active `ItemDBNameTbl.lub` has 5,208 literal mappings, SHA256
`2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496`.
Existing localized names were resolved through its numeric IDs to the effective
server Aegis names using the unmodified shared `ClientItemNames` resolver.
The original table already contains all 31 verified Druid name/ID pairs; the
earlier unresolved results came from their missing server item definitions,
not missing client aliases. The regression checks every original pair against
the independently verified identities in
[druid_item_compatibility.md](druid_item_compatibility.md). No name-table patch
or supplementary resolver is required. Other unknown names remain unresolved.

The item effects themselves are explicitly reference-compatible; adding
client-exact recipes does not convert their provenance into independently
verified official Gravity item balance.

## Recipe scope

Every listed recipe below has zero zeny cost except the group 1 recipes.
Ranges are inclusive.

| Group / slot | Enchant outcomes | Materials per selected result |
|---|---|---|
| 1 / 1 | Wolf_Orb_Skill_52-54 | 10,000,000 zeny and Ep18_Amethyst_Fragment x2,500 |
| 137 / 1 | Wolf_Orb_Skill_52-54 | Sp_Amethyst_Fragment x1 |
| 44 / 1 and 2 | Automatic_Orb99-101 | BarMealTicket x25 |
| 26 / 1 | Ice_F_Orb_Skill_55-57 | Correspondingly numbered Ice_F_Stone_Skill_55-57 x1 |
| 47 / 3 | Glacier_F_Orb_192-210 | EP19_S_F_1_Extract / 2_Extract / 3_Extract x10 / 15 / 25 |
| 47 / 2 | Glacier_F_Orb_192-210 | EP19_S_F_1_Extract / 2_Extract / 3_Extract x15 / 20 / 35 |
| 31 / 1 | Glacier_F_Orb_192-201 | Snow_F_Stone1 / 2 / 3 x25 each, plus the materials below |

Additional group 31 materials:

| Orb suffix | Additional materials |
|---|---|
| 192 | Sharpened_Cuspid x150; Posionous_Canine x150 |
| 193 | Claw_Of_Desert_Wolf x300 |
| 194 | Feather x150; Feather_Of_Birds x150 |
| 195 | Bill_Of_Birds x150; Golden_Feather x150 |
| 196 | Blade_Of_Pinwheel x300 |
| 197 | Ice_Piece x150; Ice_Heart x150 |
| 198 | Lantern x150; Brilliant_Jelly x150 |
| 199 | Cloud_Piece x300 |
| 200 | Grit x300 |
| 201 | Starsand_Of_Witch x150; Browny_Root x150 |

`Posionous_Canine` is the existing server's exact Aegis spelling.

## Gray Wolf probability change

Group 1, slot 1, grade 0 now has exactly the active client's 58 outcomes:

- Wolf_Orb_R_Reject_1: 1,830 of 100,000.
- Wolf_Orb_R_Reject_2: 500 of 100,000.
- Wolf_Orb_R_Reject_3: 100 of 100,000.
- Wolf_Orb_Force and each Wolf_Orb_Skill_1 through 54: 1,774 of 100,000 each.

The previous 55 outcomes are retained. The old weight of 1,876 for Force and
the first 51 skill orbs changes to 1,774, and Reject 1 changes from 1,848 to
1,830, accommodating the three new outcomes while retaining a total of 100,000.
Other grades/distributions, normal enchant prices/materials and success rates
are unchanged. In particular, no custom Biosphere weights are touched.

## Preservation and regression tests

The new `db/import/druid_item_enchant.yml` import is Renewal-only. It contains
only group IDs, slot IDs, the 63 new `PerfectEnchants` entries, and the complete
Gray Wolf grade-0 `Enchants` table. It does not specify targets, eligibility,
order, reset configuration, normal costs, upgrades or guaranteed upgrades.

Run repository-contained checks:

```sh
python3 tools/ci/druid_item_enchant_test.py
```

For the full active-client comparison, supply both extracted files explicitly:

```sh
python3 tools/ci/druid_item_enchant_test.py \
  --client '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
```

All ten tests pass with those files. Without the external files, the two
client-specific tests are explicitly skipped, not counted as client validation.
The checks cover exact recipe costs and client equality, scope/identity/schema,
the 58 Gray Wolf outcomes, all 339 effective probability totals, and idempotent
overlay application. They compare parsed state with only this new import
suppressed and prove every other modeled initial-enchant setting and every
ordinary/guaranteed upgrade recipe is unchanged, including server-only recipes.

## Remaining runtime work

The optional itemInfo metadata fragment is separate from this server overlay.
The unmodified active `ItemDBNameTbl` already resolves all 31 identities now
that their server definitions exist; no alias patch is needed for these items.
The three Ice stones still need an evidence-backed acquisition path; none is
invented here.

Candidate build/startup and backed-up deployment passed at 14:56 ICT; see
[deployment receipt](druid_recipes_monolith_deployment_20260906.md).
Client window visibility/selection, actual charging, inventory updates, relog
persistence and attached-player item effects must still be tested.
The overlay does not claim to resolve unrelated missing equipment targets or
newer item-dependent recipes outside these 31 verified identities.
