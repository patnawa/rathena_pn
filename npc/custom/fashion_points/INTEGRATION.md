# Fashion Points integration

The original 366 revision 61305 materials are retained, including the eight
Druid/Karnos/Alitea pairs enabled by the integrated class implementation.
`stone_catalogue.json` is the reviewed mapping source. Run
`python tools/ci/fashion_stone_catalogue.py --write` after changing it, and
`--check` in validation. The September 2026 expansion adds official Korean and
Thai regional stones and cosmetic fourth-slot support. Its scope and evidence
are documented in `doc/costume_stone_expansion_20260928.md`.

## Server prerequisites

This package depends on the custom `modifyequipitem` built-in implemented in
`src/map/script.cpp`. Rebuild and restart `map-server` from this source tree
before loading either NPC script. An older executable will reject the command
while parsing the scripts. The command is documented in
`doc/script_commands.txt`.

The NPC scripts must load in this order because the enchanter calls functions
defined by the main script:

```yaml
npc: npc/custom/fashion_points/FashionPoints.txt
npc: npc/custom/fashion_points/FashionEnchant.txt
```

The repository already contains those ordered loader entries. It also loads:

- `db/import/fashion_points_box_item_db.yml` from `db/item_db.yml` (26
  `Delayconsume` boxes)
- `db/import/fashion_points_missing_item_db.yml` from `db/item_db.yml` (8
  Druid-family physical stones and 8 functional enchant records)
- `db/import/fashion_points_item_combos.yml` from `db/item_combos.yml` (Druid-family combos)

Use a clean map-server startup to validate YAML and script parsing after
installation. A production restart is preferred over piecemeal reloads because
the package spans item, combo, item-script, and NPC data.

## Runtime behavior

- `Fashion Boxes` at `pn_style,160,130` sells Top, Middle, Lower Visual Effect
  and Garment Footprint boxes for 50 Fashion Points each (menu entries 22–25).
  Their pools contain 8, 14, 10 and 24 materials respectively. Double-click a
  purchased box for one uniformly selected stone; opening does not charge more
  points. The separate Festa Upper Slot 2 Box also costs 50 points.
- `Fashion Enchants` at `pn_style,160,138` applies supported catalogue mappings.
  Use `@fashion` to reach the Fashion area. Application is guaranteed by PN
  policy, including regional materials whose official servers use a chance.
- `Gregio Grumani#FP` at the Fashion area recovers a recognized stone for the
  published choice of 30 Fashion Points, 10 Server Coins, or 1,000,000 Zeny.
- Both services validate the item type, costume location, requested card slot,
  intrinsic socket count, exact cost inventory index, and empty/expected
  enchant. The C++ mutation repeats the item/card checks after forced unequip,
  so an `OnUnequip` script cannot silently redirect the operation.
- The equipment is never deleted or recreated. Its inventory record, unique ID,
  cards outside the selected slot, refine, identify/broken state, binding,
  expiry, favorite flag, enchant grade, and all five random options are retained
  byte-for-byte. Rental and favorite costumes are therefore supported. Forged,
  created, and pet special-card metadata remains deliberately immutable.
- Application debits one exact permanent, non-favorite stone stack, then refunds
  a stone with the same binding if the guarded mutation fails. It requires no
  spare equipment slot or temporary equipment weight.
- Recovery stages and verifies only the returned stone, then charges the exact
  selected payment. A mutation failure removes the staged stone and refunds
  Fashion Points, Zeny, or each exact Server Coin binding group. Recovery needs
  capacity only for its one returned stone.
- The core logs old/new item states as enchant transactions, refreshes the same
  client inventory entry, and attempts to restore its equip position. If a new
  restriction prevents re-equipping, mutation is still successful and the same
  item remains safely unequipped.

All validation and command-failure paths are compensated. The script engine has
no database transaction spanning item award/debit and the C++ mutation, so an OS
or host crash inside those few synchronous commands remains the sole atomicity
limit. There is no player-input yield inside either commit window.

## Regional slot and recovery rules

Upper, middle and lower stat stones use slots 1, 2 and 3 respectively.
Garment stats use slots 1 and 2. Cosmetic stones use slot 4 for their costume
family. Thai Festa upper stones use slot 2. Purified and Loft use garment
slot 1 as an explicit PN compatibility convention. Occupied slots are refused.
Recovery lists each inventory costume once and searches all supported locations.
Ambiguous shared effects use upper/middle/lower/garment priority. Thai normal and
100% materials share an output; recovery returns the 100% material. Range Middle now applies 310330; legacy 29048 remains recoverable with guarded
combo compatibility. Historical
Loft IDs 25934-25940 retain their existing equipped bonuses and are recoverable.

The expansion also imports `fashion_stone_expansion_items.yml` and
`fashion_stone_expansion_combos.yml`. Use a clean map-server restart so all
item, combo and NPC data change together.

## Client prerequisite

The existing item-info overlays remain enabled. The regional release adds
`SystemEN/itemInfo_CostumeStones.lua` to the real item-info merge and adds
missing icons/footprint artwork to `client_repairs.grf`, preserving prior
resources. See the audit report for icon fallbacks, asset provenance and
visual verification limits.

The subsequent combat/recovery/refinement audit is documented in
`doc/gameplay_refine_audit_20260928.md`. Its fixes are deployed. Actual native
client screenshots confirmed 37 of 52 effect IDs; 15 remain unconfirmed. Red
Flame and the 22nd Anniversary halo still lack verified effect mappings.
