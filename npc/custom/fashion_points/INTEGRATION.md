# Fashion Points integration

This implementation follows revision 61305 (2026-07-29) of the published
[MuhRO Fashion Points page](https://wiki.muhro.eu/Costume_Enchants_(Fashion_Points)).
Its 21 box tables contain all 366 published physical-stone/enchant pairs in the
published order. Of those, 358 have functional enchant records in this server
build; the eight Druid/Karnos/Alitea outcomes are deliberately withheld.

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

- `db/import/fashion_points_box_item_db.yml` from `db/item_db.yml` (21
  `Delayconsume` boxes)
- `db/import/fashion_points_missing_item_db.yml` from `db/item_db.yml` (8
  future-class physical stones and 8 withheld enchant records)
- `db/import/fashion_points_item_combos.yml` from `db/item_combos.yml` (withheld
  future-class combos)

Use a clean map-server startup to validate YAML and script parsing after
installation. A production restart is preferred over piecemeal reloads because
the package spans item, combo, item-script, and NPC data.

## Runtime behavior

- `Complete Fashion Enchanter#FP` at `mal_in01,25,113` applies all 358 supported
  mappings. The page publishes no application probability, so application is
  guaranteed by declared compatibility policy; no probability is invented.
- `Gregio Grumani#FP` at `mal_in01,24,120` recovers a recognized stone for the
  published choice of 30 Fashion Points, 10 Muh Coins, or 1,000,000 Zeny.
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
  Fashion Points, Zeny, or each exact Muh Coin binding group. Recovery needs
  capacity only for its one returned stone.
- The core logs old/new item states as enchant transactions, refreshes the same
  client inventory entry, and attempts to restore its equip position. If a new
  restriction prevents re-equipping, mutation is still successful and the same
  item remains safely unequipped.

All validation and command-failure paths are compensated. The script engine has
no database transaction spanning item award/debit and the C++ mutation, so an OS
or host crash inside those few synchronous commands remains the sole atomicity
limit. There is no player-input yield inside either commit window.

## Withheld future-class outcomes

`FP_EnchantSupported` requires `Type: Card`, `SubType: Enchant`, and denies
314848-314855. Boxes therefore never issue these inert outcomes, and manually
granted stones cannot be applied. Recovery remains available for an already
inserted matching card and returns the exact published physical stone.

Do not remove this gate until a complete Druid/Karnos/Alitea server build and
matching client resources are installed. The local server currently contains
none of skill IDs 6526-6606. The still-open
[rAthena PR #9765](https://github.com/rathena/rathena/pull/9765) supplies a
server-side implementation, reports testing with a 2025-12-17 client, and
explicitly does not supply client-side support. Cards 314848-314855 also need
`SubType: Enchant` before activation. See the client README for the full
activation checklist.

## Client prerequisite

Install `client-patch/fashion_points` and ensure its returned table is actually
merged into the main client item-info table. Exact sprites are unavailable in
this repository, so the supplied metadata uses a disclosed fallback icon. The
server scripts can run without that patch, but players would see missing or
incorrect item names/icons for the 37 custom IDs.
