# Episode resource compatibility

Fixes missing inventory pictures, item collection pictures and ground sprites for
108 Chapter 2 items. The item loader corrects resource fields on 104 records using
the official September 7, 2026 item table. Names, descriptions, slots, class numbers,
custom overrides and server gameplay are preserved.

The archive adds 116 previously missing resources: 29 inventory pictures, 29
collection pictures and 29 ground SPR/ACT pairs. All are native episode artwork;
there are no placeholder or fallback images. The artwork is from the March 18,
2026 update, acquired from a public plaintext GPF mirror. Its paths and expanded
lengths were checked against the official patch index, including subsequent
patches through September 7. This verifies correspondence, not cryptographic
authentication of the mirror against the encrypted original.

A separate card illustration table adds mappings for cards 300761–300783 while
preserving all 1,390 existing mappings byte for byte. These 23 cards previously
had no illustration mapping. The verified March 18 official table maps each to
the existing `sorry.bmp` illustration placeholder; no new placeholder artwork is
added. These card illustration fallbacks are separate from the 116 native item
assets above. The latest indexed official illustration table is August 5, 2026,
but its encrypted payload could not be read, so the March mapping is not claimed
to be verified against that later revision. Distinct Chapter 2 card illustrations
remain unavailable in the inspected plaintext resources.

The English translation base was compared with upstream commit
`66cdfec631603fda6a90ba4bbe26ab07b5204c84` (August 5, 2026). Its 613 archive
paths already existed; 603 matched. The remaining paths contain local connection,
navigation, quest UI, or newer job/effect support and are preserved. Replacing the
whole translation would remove newer Druid-family skill trees and item callbacks.

Build with `python client-patch/client_compat/build_grf.py`. Validate with:

```text
python client-patch/client_compat/validate.py --client CLIENT_ROOT --lua LUA51_EXE
```

Validation checks GRF round trips, BMP dimensions, SPR pixel runs, ACT sprite
references, resource resolution in every active archive, all 26,907 item
registrations, and preservation of every non-resource item field. Before install,
104 resource records change; repeating validation after install normally reports
zero changes. Preserve the preinstallation report separately.

Clean-checkout CI runs `python client-patch/client_compat/validate.py --assets-only`.
It builds the archive in memory and validates the tracked artwork and binary
formats without installed client files, Lua, or generated artifacts. Both release
gate phases run this check; full client resolution and Lua registration still
require the explicit client and runtime arguments above.
The archive now has 117 entries, including the card illustration table; Python
and native Lua independently check preservation of the existing card mappings.

The client supports only ten archives, in slots 0 through 9. Combine this overlay
with the existing validated navigation archive before installing the supplied INI:

```text
python client-patch/client_compat/merge_grfs.py client_compat.grf navigation_repair.grf --output client_repairs.grf
```

Inputs are listed highest priority first. Install `client_repairs.grf`, both
`SystemEN` files, and the reviewed `DATA.INI` together with backups. The combined
archive preserves all 135 resource payloads from the two repair archives. Do not
also list their separate archives in DATA.INI: `data.grf` must remain in slot 9.
The full loader
is a deployment snapshot: rebase its single final import if the active loader
changes before installation. Never replace an independently modified loader.

The executable and packet version remain February 19, 2026. Lua and file format
validation do not establish visual rendering or support for later executable-only
features. Original game artwork belongs to Gravity; source receipts, mirror
details, official patch indexes and installation hashes are retained in the
external client compatibility audit directory.
