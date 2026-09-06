# Remaining enchant identities and group coverage

Read-only audit, 2026-09-06. This records the working-tree state after the
31-item Druid compatibility definitions and initial-enchant overlay. Only this
document was added by this pass; no item data, recipes, NPCs, client files,
acquisition routes, or effects were changed.

## Result

All 40 names still unresolved by `ClientItemNames.unresolved_details` are
`server_item_missing`: the active client already supplies a positive numeric ID,
but the effective Renewal item imports have no identity at that ID. There are
**zero `client_name_missing` results**. The 40 comprise 31 target-equipment
identities and nine enchant identities. Twenty-seven of the target identities
occur in shared client/server groups; the remaining four belong to missing
server group 166. These are distinct names, not target occurrence counts:
`Time_DM_R_Crown_AT` appears in two groups.

Every one of the 40 ID/name pairs independently matches the pinned
[ROenglishRE author repository's ItemDBNameTbl](https://github.com/llchrisll/ROenglishRE/blob/66cdfec631603fda6a90ba4bbe26ab07b5204c84/Additions/data/luafiles514/lua%20files/ItemDBNameTbl.lub).
This is high-confidence identity evidence, **not official Gravity gameplay-data
provenance** and not evidence for equipment effects, restrictions, or acquisition.
Do not synthesize item definitions from these IDs alone.

## Reproducible inputs

Paths are relative to the repository root. The original client name table was
read without alteration or supplementary aliases; it also already contains all
31 previously integrated Druid orb/stone IDs.

| Input | Bytes | SHA-256 |
| --- | ---: | --- |
| `../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub` | 1056907 | `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d` |
| `../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub` | 278965 | `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496` |
| ROenglishRE `Additions/data/luafiles514/lua files/ItemDBNameTbl.lub`, commit `66cdfec631603fda6a90ba4bbe26ab07b5204c84` | 141741 | `686e6d9d8cffb49b8e149c6dc81cb564893412e893956c562df860e97395368c` |

The active EnchantList is CP949 plaintext; the active item-name table is Lua 5.1
bytecode with 5208 literal mappings. The pinned author's plaintext table was
decoded as CP949 and its literal name/integer pairs compared to every unresolved
detail: 40/40 matched. The effective server identity map comes from recursive
Renewal imports rooted at `db/item_db.yml`, not a filename-only search.

## Missing equipment identities

All entries below are existing client target declarations. IDs are verified
against both item-name sources above; the group column is from active EnchantList.

| Client name | Item ID | Enchant group(s) |
| --- | ---: | --- |
| `Solid_Whinger` | 510189 | 24 |
| `Glacier_N_Knife` | 510190 | 31 |
| `Glacier_N_Axe` | 620056 | 31 |
| `Repeat_Dagger_AD` | 510185 | 33 |
| `SC_B_Axe` | 620064 | 36 |
| `NP_B_Dagger` | 510200 | 36 |
| `D_Glacier_N_Axe` | 620057 | 47 |
| `D_Glacier_N_Knife` | 510191 | 47 |
| `F_Ein_AXE` | 520047 | 63 |
| `Mocadas_Garz` | 590104 | 64 |
| `Time_DM_R_Crown_AT` | 400999 | 132, 164 |
| `Dimen_AT_Axe` | 620059 | 133 |
| `Dimen_AT_Knife` | 510193 | 133 |
| `Axe_Furious` | 520052 | 147 |
| `Hall_Furious` | 590117 | 147 |
| `FuriousCirclet_AT` | 401176 | 148 |
| `Frontier_R_Crown_AT` | 401195 | 164 |
| `Sky_Rune_Crown_SHC` | 401171 | 165 |
| `Sky_Rune_Crown_AG` | 401172 | 165 |
| `Sky_Rune_Crown_BO` | 401173 | 165 |
| `Sky_Rune_Crown_TR` | 401174 | 165 |
| `Sky_Rune_Crown_AT` | 401175 | 165 |
| `Sky_Rune_Crown_DK` | 401216 | 165 |
| `Sky_Rune_Crown_EM` | 401217 | 165 |
| `Sky_Rune_Crown_SS` | 401218 | 165 |
| `Sky_Rune_Crown_NW` | 401219 | 165 |
| `Sky_Rune_Crown_SOA` | 401220 | 165 |
| `S_AT_Armor` | 1270183 | 166 |
| `S_AT_Shoes` | 1270184 | 166 |
| `S_AT_Earring` | 1270185 | 166 |
| `S_AT_Pendant` | 1270186 | 166 |

## Missing enchant identities

| Client name | Item ID | Group / slot / declaration |
| --- | ---: | --- |
| `Gear_AT1` | 314270 | 24 / 2 / `AddPerfectEnchant` |
| `Gear_AT2` | 314269 | 24 / 2 / `AddPerfectEnchant` |
| `AT_Soul_AC` | 314804 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_FF` | 314805 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_PS` | 314806 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_QS` | 314807 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_GS` | 314808 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_RP` | 314809 | 166 / 2 / `AddPerfectEnchant` |
| `AT_Soul_TH` | 314810 | 166 / 2 / `AddPerfectEnchant` |

The nonsequential `Gear_AT1`/`Gear_AT2` mapping is literal, not a transcription
error. Their recipe declarations are known, but effects remain outside this
audit. A valid ID alone is insufficient to add an inert placeholder item.

## Whole-group coverage

There are 159 client groups and 162 effective server groups, with 157 shared.
The former initial comparator iterated only that intersection, silently omitting
client-only groups 128 and 166. The former upgrade comparator also skipped
recipes whose group was not loaded by the server.

The main agent corrected both diagnostics during this audit. Current initial
comparison explicitly emits `missing-server-group` for 128 and 166 and lists
server-only groups. Current upgrade output explicitly reports missing recipe
group 128 and exits unsuccessfully even though shared-group upgrade comparison
has zero recipe issues. Thus the historical silent gap is now reported; it has
**not** been repaired in the game data by this audit.

| Client-only group | Target entries | Selectable initial recipes | Normal grade tables | Ordinary upgrades | Guaranteed upgrades |
| --- | ---: | ---: | ---: | ---: | ---: |
| 128 | 14 | 12 | 0 | 24 | 0 |
| 166 | 4 | 18 | 0 | 0 | 0 |
| Total previously omitted | 18 | 30 | 0 | 24 | 0 |

The full client has 2337 selectable initial recipes, versus 2307 in shared
groups. It has 1269 ordinary upgrade recipes, versus 1245 in loaded groups, and
444 guaranteed upgrades, all in loaded groups. These are unique parsed recipe
keys, not counts of eligible equipment or all possible outcome draws.

Current initial output contains 27 issues: two missing groups, 13 target-list
mismatches, two missing `Gear_AT1`/`Gear_AT2` recipes, and ten intentionally custom
Biosphere normal-outcome distributions. All 339 effective server probability
tables and all parsed client tables sum to 100000. The 40 unresolved names are
separately reported; they are not 40 additional initial-comparison issue rows.

### Group 128: dependencies already resolve

`db/re/item_enchant.yml:29673` retains a fully commented group 128 with a legacy
note that some fields are unsupported. The active client contains 14 target
items: Full Power, Full Rate, and Full Spell armor/shoes, plus Focusing, Stout,
Centering, and Witty earring/pendant pairs. All targets, materials, initial
enchants, and upgrade outcomes resolve to existing server identities.

Its slot order is 3 then 2, minimum refine/grade are zero, random options are
allowed, and reset is disabled. Slot 3 has six level-3 basic-stat enchants at
five `S_Enchant_Essence` each; slot 2 has six level-1 Shadow ATK/MATK/CRI/CAST/HP/SP
enchants at seven essence each. All have zero zeny cost.

Each Shadow category has four client random upgrades (24 total):

| Source level | Essence | First outcome / weight | Second outcome / weight |
| --- | ---: | --- | --- |
| 1 | 2 | level 1 / 30000 | level 2 / 70000 |
| 2 | 3 | level 1 / 40000 | level 3 / 60000 |
| 3 | 5 | level 2 / 50000 | level 4 / 50000 |
| 4 | 8 | level 3 / 60000 | level 5 / 40000 |

All upgrade zeny costs are zero. These client declarations include retention or
downgrade outcomes and must not be flattened into a simple success percentage.
Current engine random-upgrade support makes this a dependency-complete candidate
for a subsequent bounded restoration review. An NPC search found no literal
`item_enchant(...128...)` call; indirect calls and deployment-specific routes were
not exhaustively traced, so this is not proof that no route exists.

### Group 166: eleven missing identities

Four missing `S_AT_*` targets and seven missing `AT_Soul_*` enchants block this
group. The client has 18 initial recipes: slot 3 offers six level-3 trait-stat
enchants for one essence each; slot 2 offers five existing generic Soul enchants
for three essence and seven missing AT Soul enchants for five essence. All are
zero zeny. There are no normal distributions or upgrade recipes.

Slot order is 3 then 2, minimum refine/grade are zero, and random options are
allowed. `SetReset` explicitly has `Enabled=false`; its dormant numeric chance,
price, and material fields must not be treated as an enabled paid reset. No
literal `item_enchant(...166...)` NPC call was found; indirect routes remain
outside this pass.

### Server-only Chapter 2 groups

Groups 167-171 are absent from the compared active client EnchantList but have
native server recipes and explicit `item_enchant(167)` through `item_enchant(171)`
calls at `npc/custom/chapter2/Chapter2.txt:539` through line 543.

| Group | Equipment family | Target entries | Selectable initial recipes |
| --- | --- | ---: | ---: |
| 167 | Azure/Blaze armor and robe | 4 | 15 |
| 168 | Azure/Blaze manteau and muffler | 4 | 11 |
| 169 | Azure/Blaze boots and shoes | 4 | 12 |
| 170 | Azure/Blaze shields | 2 | 6 |
| 171 | Azure/Blaze accessories | 8 | 14 |
| Total | | 22 | 58 |

These 58 recipes have no counterpart in this client file and cannot be called
client-validated by the shared-group comparison. Other client Lua loading,
overrides, and actual window behavior were not traced here. This is a concrete
coverage risk, not a claim that all five native windows are proven unusable.

## Verification and safe continuation

The following read-only commands were run from the repository root using WSL
Python 3 with PyYAML. Both correctly exit 1 for the documented remaining gaps:

```sh
python3 tools/ci/audit_initial_enchants.py \
  '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
python3 tools/ci/audit_enchant_upgrades.py \
  '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub'
```

The upgrade report has zero shared recipe issues and zero unresolved upgrade
names, but explicitly records group 128 and its 24 missing-group ordinary
recipes. It also reports 24 server-only ordinary recipes in loaded groups;
those were not changed or reassessed in this pass.

The smallest dependency-complete next data review is group 128: verify supported
native semantics and intended NPC exposure before restoring the exact client
recipes. Independently trace the client loading path for Chapter 2 groups
167-171. For the 40 missing item identities, obtain effects and equipment
restrictions from adequate sources before defining items or adding their target
eligibility. No acquisition costs, drop rates, item effects, or placeholder
equipment were inferred here. This parser/source audit is not a live client
interaction, charging, or runtime equipment-script test.
