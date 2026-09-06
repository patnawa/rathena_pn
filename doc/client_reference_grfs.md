# Client reference inventory — 2026-09-06

The owner supplied these local reference clients under
`C:/Users/Alpha/Downloads/Compressed/`:

- `RockMMO Jormungand 220126/RockMMO Jormungand 220126/`
- `MuhRO/MuhRO/`

They are reference data, not drop-in replacements for the running client's
configuration, custom items, executable, protection libraries, or GRF priority.
No third-party GRF contents or credentials are committed to this repository.

## What is readable

RockMMO's `data.ini` enables `gepard.grf`, `jormungand.grf`, `ragnarock.grf`,
`palette.grf`, `sdata.grf`, and `data.grf` in that order. `ragnarock.grf`'s index
lists enchant tables, an item-name table, and NPC job/sprite mappings. `data.grf`
lists the Grademk map trio. The sampled payloads do not decode as standard zlib,
although the GRF entry flags report type 1. The stock extractor cannot import
these entries. No decryption or executable modification was attempted.

MuhRO's `muh.grf` uses the Event Horizon 0x300 container. Its index lists
`data/grademk.gat`, `.gnd`, `.rsw`, enchant tables, the item-name table, and NPC
job/sprite mappings. The tested Grademk payload starts with `MUHVLT2`, a custom
wrapper unsupported by the standard extractor. Its 60-byte overhead is visible
in the entry, but the wrapper has not been decoded. Archive priority for MuhRO
has not been established, so its base GRF is not treated as effective client data.

Readable loose reference files include RockMMO's `System/itemInfo_EN.lua` (its
header dates that particular file to 2023-08-26) and MuhRO's
`System/itemInfo_EN.lua` loader with `itemInfo_EN_db.lua`,
`itemInfo_EN_db_fallback.lua`, and `itemInfo_muh.lua`. These can support future
ID-based description checks. Their descriptions do not establish server item
effects or enchant costs. Display/resource names are not Aegis identifiers.

## Active-client evidence used for the actual fix

The current Data2026 client remains authoritative for its own enchant UI:

- Priority-0 `nebula_upgrade_v2.grf` provides the compared `EnchantList.lub`.
  SHA-256: `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d`.
- `data.grf` provides `ItemDBNameTbl.lub`, with no matching entry in the four
  higher-priority GRFs and no loose file at the equivalent path.
  SHA-256: `2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496`.
  The non-executing Lua 5.1 reader extracts 5,208 names/IDs. The audit resolves
  these IDs against the effective Renewal item imports rather than guessing
  translations. It resolves 85 aliases in the compared recipes and materials.
- The current base `grademk.gat` is 200×200 cells.
  SHA-256: `f160f011b275939c0bfad0da01e8aa9aadb135744c1c25c793a755fa9fc5b2d0`.
  Counter approach positions x=30,32,36,38,42,44,46,48,50 at y=181 are walkable.
  Server-cache route checks independently cover those positions and the healer
  approaches in Prontera and Malangdo. This is not a rendered sprite/click test.

## Reproduce

Extract only the named files to a diagnostic directory, keeping the owner's
original clients unchanged. Supply their actual extracted paths:

```sh
python3 tools/ci/audit_enchant_upgrades.py /path/to/EnchantList.lub \
  --client-item-names /path/to/ItemDBNameTbl.lub \
  --compare-non-upgrades-ref 2edb3538a
python3 tools/ci/audit_enchant_upgrades_test.py
```

Expected result after the crown correction: 1,245 ordinary and 444 guaranteed
client upgrade recipes across 162 loaded groups, zero recipe differences, zero
unresolved names, and 24 preserved server-only recipes. The synthetic suite has
23 tests. The audit prints hashes of the name table and reports differences; it
does not execute Lua or modify input files. Material overlays follow the native
parser, including amount-zero removal and retention of omitted requirements.

This comparison covers upgrade recipes only. Full item effects, initial enchants,
reset behavior, visual assets, and gameplay still require separate verification.
