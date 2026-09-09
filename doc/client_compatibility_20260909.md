# Client compatibility update, September 9, 2026

Installed the Chapter 2 resource repair in the active client. Official item metadata
through September 7 supplied corrected resource names on 104 records. The new
archive supplies 116 asset files, resolving the missing inventory, collection and
ground resources for 108 episode items without placeholders.

The maintained English base was already current. Existing custom item descriptions,
equipment fields, newer job trees, connection settings and navigation remain intact.
The February 19, 2026 executable is unchanged to preserve the server packet version.
Asset provenance and reproduction steps are in
[the patch README](../client-patch/client_compat/README.md).

Validation passed before and after installation:

- All 26,907 item registrations load in native Windows Lua 5.1. Before installation,
  exactly 104 resource records changed; all other item fields were preserved.
- All 116 archive payloads round-trip; 58 bitmap layouts and 29 ground sprite/action
  pairs pass format and reference checks.
- The 17 Chapter 2 maps match server collision data; 22 enchantment targets and 58
  exact recipes match server configuration, as do 21 existing shadow groups.
- All 11,393 quest records load, retaining the reviewed navigation fixes.
- Every existing active archive retains its hash and relative priority; the new
  archive takes first priority and contains only previously absent artwork.

A final visual-reference audit also verified all 421 direct model references and
65 ground textures used by the 17 maps. A subsequent archive update preserves
1,390 existing card-illustration mappings and adds 23 official March 18 fallback
mappings for the Chapter 2 cards. These use the existing `sorry` image; no distinct
card illustrations were invented or supplied. The latest indexed table revision
is August 5, but its encrypted values could not be verified. The updated archive
contains 116 artwork files and one card mapping table.

The current-client audit now accepts legacy Korean resource-path bytes and checks
the winning enchantment overlay instead of requiring the historical archive owner.
Its original strict reader behavior remains the default for the older ASCII fixture.

Installation changed four files: `client_compat.grf`, `DATA.INI`,
`SystemEN/itemInfo.lua` and `SystemEN/itemInfo_ClientCompat.lua`. The external audit
directory `server-work/client-compat-update-20260909` contains the before/after hashes,
validation logs, installation receipt and rollback copies of replaced files.

These checks establish data and Lua compatibility. A rendered in-game playthrough
was unavailable, and this update does not implement later executable-only features
or import unsupported server episodes.
