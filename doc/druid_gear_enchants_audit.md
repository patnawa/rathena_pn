# Druid Gear enchant compatibility — 2026-09-06

The isolated item overlay `db/import/druid_gear_enchants.yml` defines the two
missing Clock Tower tuning enchants. Effects are **reference-compatible**;
this is not a claim of independently verified official Gravity balance.

## Identity and permitted effect evidence

The original active client `ItemDBNameTbl` maps `Gear_AT1` to **314270** and
`Gear_AT2` to **314269**. The reversed numeric order is intentional. Both match
the pinned [ROenglishRE author name table](https://github.com/llchrisll/ROenglishRE/blob/66cdfec631603fda6a90ba4bbe26ab07b5204c84/Additions/data/luafiles514/lua%20files/ItemDBNameTbl.lub).
The prior [coverage audit](remaining_enchant_coverage_audit.md) records that
independent name-table comparison and full source hashes.

Effects were read directly from the user-permitted loose file
`C:/Users/Alpha/Downloads/Compressed/ReferenceClient/System/itemInfo_EN_db_fallback.lua`,
lines 301340–301373, SHA256
`235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a`.
No protected GRF was opened or decrypted. `itemInfo_EN_db.lua` only gives the
shared resource name `Gear_AT` for these IDs (lines 315452–315457), not effects or
Aegis identities; its SHA256 is
`2834833d0438219b46e03224e19ebe94f0b018c725aa43dab7df94fe5ef4a4e6`.
English server display labels are compatibility labels following existing
Tuning Device items, not asserted official names.

| Item | Skill | No grade | D | C | B | A |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| `Gear_AT1` / 314270 | Quill Spear, `AT_QUILL_SPEAR` / 6588 | 5% | 8% | 8% | 12% | 18% |
| `Gear_AT2` / 314269 | Pinion Shot, `AT_PINION_SHOT` / 6586 | 5% | 8% | 8% | 12% | 18% |
| Both, independently of skill bonus | Physical damage against every size | 0% | 0% | 10% | 10% | 10% |

The skill thresholds add 3%, 4%, and 6% at D, B, and A respectively; grade C adds
the separate size bonus. No refine threshold is specified. The scripts preserve
these two bonus categories instead of combining them into one damage percentage.

## Native semantics

- `getenchantgrade()` defaults to `EQI_COMPOUND_ON`, reading
  `current_equip_item_index` in `src/map/script.cpp`. It reads the host equipment's
  grade in enchant execution context. `src/common/mmo.hpp` orders none/D/C/B/A
  as 0/1/2/3/4. The source describes `Grade >=`, so thresholds are cumulative.
- `bonus2 bSkillAtk,"skill",value` resolves the symbolic name through
  `skill_name2id`; the effective Renewal skill database contains the exact two
  IDs above. `pc_bonus2` stores cumulative skill attack bonuses and
  `pc_skillatk_bonus` returns them for the canonical skill.
- `AT_QUILL_SPEAR_S` / 6589 maps to `AT_QUILL_SPEAR` in
  `skill_dummy2skill_id`. Registering only the parent applies the bonus once to
  both variants. Pinion Shot has no separate enhanced entry.
- `bonus2 bAddSize,Size_All,10` is the established native physical-size bonus,
  matching existing Gear items such as `Gear_DN1`. `pc_bonus2` adds it to the
  appropriate weapon's `addsize[SZ_ALL]`; weapon card-fix calculations in
  `src/map/battle.cpp` consume the all-size entry in addition to any specific
  size entry. It is neither a flat ATK bonus nor magic-size damage.

Both entries are `Type: Card`, `SubType: Enchant`. No location mask, job mask,
price, weight, trade rule, drop, vendor, cooldown, or refine requirement was
inferred. Omitted item properties retain native defaults.

## Exact shared wiring

This bounded subtask creates the item overlay, its focused test, this document,
and the client fragment described below. It does not edit shared database roots
or recipe imports. The parent integration adds exactly one
`db/import/druid_gear_enchants.yml` import with `Mode: Renewal` to `db/item_db.yml`.
The focused `--require-import` check subsequently confirmed that import and the
two recipes below in `db/import/druid_item_enchant.yml`.

The parent deployed these three scoped data files and the new client fragment
on 2026-09-06; see the [deployment receipt](chapter2_gear_deployment_20260906.md).
The final integrated rerun passed all ten tests without skips, including the
220-assertion native sanitizer test.

The active CP949 `EnchantList.lub`, SHA256
`664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d`, lines 4242–4243,
declares the following two perfect/selectable initial recipes in group 24, slot 2:

```yaml
- Id: 24
  Slots:
    - Slot: 2
      PerfectEnchants:
        - Item: Gear_AT1
          Price: 0
          Materials:
            - Material: ClockTower_Gear
              Amount: 150
            - Material: Shadowdecon
              Amount: 150
            - Material: Zelunium
              Amount: 150
        - Item: Gear_AT2
          Price: 0
          Materials:
            - Material: ClockTower_Gear
              Amount: 150
            - Material: Shadowdecon
              Amount: 150
            - Material: Zelunium
              Amount: 150
```

Append/merge only those recipe keys into the established Renewal recipe overlay.
The client does not declare either item in group 24's normal outcome tables.
All three material identities already exist in server data. The separate
`Solid_Whinger` target-identity gap is outside these two enchant definitions;
this work does not invent that weapon or alter group eligibility, reset, slot
order, normal costs, normal probabilities, or upgrades.

## Client fragment

`client-patch/druid_gear/SystemEN/itemInfo_DruidGear.lua` exports `tbl_druidgear`
and returns the same table. It contains exactly IDs 314269 and 314270, with names
matching the server records. Descriptions explicitly state the base damage,
each cumulative grade threshold, the separate physical all-size bonus, type
Enchant, and weight 0. They make no acquisition claim.

Both identification states use the already-known generic `EpisodClear20` resource.
The fragment does not assume that reference server's `Gear_AT` artwork exists in the active
client, and includes no artwork. The separate parent-owned installation step
copies this fragment to `SystemEN`, adds `itemInfo_DruidGear.lua` to `ImportFiles`,
and adds `druidgear` to `ImportTables` in the existing loader.

The actual Win32 Lua 5.1 runtime executes the fragment and the active client's
unmodified `F_itemInfoMerge` function against synthetic base tables. The test
checks both record identities, all description strings, resource fields, zero
slots/class, non-costume metadata, preservation of unrelated data and idempotent
merging. It also checks the real default merge contract: an existing same-ID
record is retained. The parent installation must therefore verify the final
merged metadata; this synthetic merge check does not claim an installed loader
or rendered tooltip was tested.

## Verification

`tools/ci/druid_gear_enchants_test.py` checks the two identities, metadata scope,
loaded skill IDs, and absence of conflicting effective definitions. It also
checks the exact effective group-24 recipes and compares all original settings
against a baseline that suppresses only the shared overlay's group-24 record;
only the two new perfect-recipe keys may differ, and upgrades remain identical.
Optional
paths independently verify the exact permitted descriptions and both original
active-client ID/recipe declarations. `--require-import` verifies the eventual
single Renewal item import after the shared wiring is integrated.

Run the complete source and native checks from the repository root in WSL/Linux:

```sh
python3 tools/ci/druid_gear_enchants_test.py --native-vm --require-import \
  --client '../audit-client-enchants-20260906/nebula/data/luafiles514/lua files/Enchant/EnchantList.lub' \
  --client-item-names '../audit-item-aliases-20260906/data/data/luafiles514/lua files/itemdbnametbl.lub' \
  --effect-reference '/mnt/c/Users/Alpha/Downloads/Compressed/ReferenceClient/System/itemInfo_EN_db_fallback.lua' \
  --lua '../chapter2-lua51-runtime-20260906/runtime/lua5.1.exe' -v
```

The `--native-vm` mode uses the existing isolated native-VM fixture's setup and
world-boundary doubles, with mandatory kernel socket/connect/bind/listen denial.
It freshly compiles the current `script.cpp`, `pc.cpp`, `skill.cpp`, `clif.cpp`, allocator,
and generated test driver with AddressSanitizer and UndefinedBehaviorSanitizer.
It parses the actual YAML scripts and executes both against a synthetic attached
player at all five grades, with a different-grade second inventory item to check
the host selection. One and two applications check 20 item/grade/copy cases,
physical all-size accumulation, unrelated-skill isolation, and enhanced Quill
Spear's canonical bonus lookup. The probe asserts no inventory refresh or log
mutation. Other prebuilt engine objects only satisfy link dependencies.

Verified on 2026-09-06: the initial seven tests passed with all three external paths and
`--native-vm`; no tests were skipped. The native completion marker reports
**20 item/grade/copy cases and 220 assertions**. AddressSanitizer and
UndefinedBehaviorSanitizer reported no errors, and the native allocator reported
no leaks. The probe required fresh `clif.cpp` as well as `skill.cpp` because the
working source references a newer packet helper absent from the older link
support object. This was resolved only in the temporary test build.

After the shared item/recipe integration and client-fragment addition, the
expanded suite passed nine checks with `--require-import`, all three source paths
and `--lua`. The unchanged native-VM test was explicitly skipped on that second
run; its earlier 20-case/220-assertion pass remains the native evidence. Neither
item script nor the native test driver was changed in the client/recipe follow-up.

This does not start a server or perform full equipment calculation, rendered
client interaction, combat damage packets, recipe charging, inventory
persistence, material acquisition, or deployment. These boundaries are not
claimed as verified by the isolated VM test.
