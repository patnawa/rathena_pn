# Sealed Drake tooltip correction

The client described card 4496 as an armor card with only 50% physical size damage.
The main server's database, read through the running map-server's working directory
on 2026-09-11 UTC, defines a weapon card with 50% physical and magic size damage,
increasing to 75% at weapon refine +15 or higher. No matching item overrides were
found in db/import. Elemental Spirits 540114 has two card slots.

Run `python apply.py <client-directory>` to update only this tooltip. The installer
preserves resource bytes and all other item records, and creates a backup before
editing. Restart the client afterward. Server mechanics and existing items are
unchanged. The effective client Lua loader was checked with Lua 5.1; a rendered
tooltip after restart has not been verified.

The record excerpts retain the original ROenglishRE translation structure. Original
translation credits and notices remain in the installed itemInfo.lua.
