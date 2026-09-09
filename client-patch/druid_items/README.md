# Druid-family enchant item metadata

Optional clean-room names and descriptions for the 31 records in
`db/import/druid_item_db.yml`. Effects follow the supplied reference server reference;
these are compatibility labels, not claimed official translations. See
`doc/druid_item_compatibility.md` for source hashes and gameplay limits.

No active client files are modified automatically.

## Installation with the existing SystemEN multi-itemInfo loader

1. Back up the client's `SystemEN/itemInfo.lua` and any existing
   `SystemEN/itemInfo_DruidItems.lua`.
2. Copy this package's `SystemEN/itemInfo_DruidItems.lua` into the client's
   `SystemEN` directory.
3. Add `"itemInfo_DruidItems.lua",` once to the loader's `ImportFiles` list.
4. Add `"druiditems",` once to its `ImportTables` list. Preserve every existing
   import and table name; do not replace the loader wholesale.
5. Restart the client and inspect a representative item from each family.

For other loaders the fragment also returns its item table, allowing an
explicit merge into that loader's existing table. Do not execute or merge it
twice. Uninstall by removing these two loader entries and restoring the backed-up
fragment, if there was one.

The fragment uses the ASCII-safe `EpisodClear20` generic resource already used
by the project's Episode Clear Ticket and other compatibility patches. No
original Druid item artwork is included. Every record has zero physical slots
and no equipment appearance.

This itemInfo fragment does not add native-enchant `ItemDBNameTbl` aliases,
server recipes, vendors or material drops. The original active client table
already contains all 31 name/ID pairs in `item_name_aliases.json`; earlier
unresolved audit results meant missing server item records, not missing client
names. No client name-table patch or GRF priority change is needed.
Run `tools/ci/audit_druid_item_names.py` with the extracted original table to
verify both identities read-only. Separate server recipes are documented in
`doc/druid_item_enchant_compatibility.md`; client clicks remain unverified.
