# PN client presentation

Apply `files/` after the existing client metadata overlays. The installed item
loader calls `itemInfo_PNBranding.lua` last, after all item tables are merged.
It hides origin-server tags and generated database links, and removes embedded
external guide links from identified and unidentified descriptions. IDs,
names, resource references, equipment effects and costume flags are preserved.

The 21 Fashion box records now describe their equal-chance selection directly.
Their identified and unidentified tooltips no longer compare other servers.
Client notes and metadata comments use PN wording.

The actual Lua 5.1 item loader and `main()` export were executed before and after
the change. All 27,133 items retain identical IDs, names, resources and effects;
only the approved description replacement and origin/link removals differ.
The archive scan inspected 816 text entries across all ten installed GRFs.
No searched private-server brands were found in those archive text entries.
Translation authorship and license notices remain intact.

Published and locally installed as `client-20260929-pn-branding`, sequence
2026092902. Evidence and exact preimages are in
`Server-Development/branding-cleanup-20260929`. The signed updater also retains
the previous release for rollback. Historical development evidence is not part
of the player-facing branding cleanup.
