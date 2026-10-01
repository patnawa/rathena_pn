# Client/server item consistency audit — 29 September 2026

Prepared a client-only candidate correcting 184 obtainable items' displayed weights
to the current effective local Renewal database. Nothing was installed or published.
Examples: Silver Arrow 0.2 → 0.1, Blue Gemstone 3 → 0.1, Bomber Steak 50 → 5,
Cake Hat 10 → 100, and Rib of Jormungand 100 → 1000. These are presentation fixes;
server balance and item behavior are unchanged.

## Fresh scope and results

- Parsed the actual installed `PN-Client/SystemEN/itemInfo.lua` loader under native
  Lua 5.1 and all its overlays: 27,133 client identities registered.
- Parsed the current repository's Renewal item import chain: 29,713 effective IDs.
  Source SHA-256 values are retained. This is a local source audit, not a fresh
  production database snapshot.
- Indexed all ten installed DATA.INI archives in ascending priority, retaining
  241,211 winning archive resource paths. Checked both identified/unidentified
  inventory icons and collection bitmaps. Archive presence does not certify image
  decoding, visual correctness, executable loose-file priority, or rendering.
- Re-ran acquisition tracing across 925 active NPC files and 2,999 item groups:
  all 6,660 IDs with statically traced source paths have client metadata and their
  referenced icon/collection archive entries. No slot mismatch has a traced path.
- The full catalogue has 5,127 absent bitmap references across 2,036 client IDs,
  51 slot mismatches, and 4,126 server IDs without client metadata. None has a
  static obtainable path in the checked configuration. No catalogue entries were
  deleted and no substitute artwork was introduced.
- The broad weight scan produced 1,001 differing lines; 196 have acquisition
  paths. Twelve belong to descriptions containing multiple weight sections,
  including recipe output equipment and box contents. They were excluded because
  those secondary weights are not necessarily defects. The remaining 184 have
  one unambiguous weight line, matched exactly by the candidate overlay.

## Candidate and verification

`client-patch/pn_item_weights/SystemEN/` contains the new `itemInfo_PNWeights.lua`
overlay and the installed loader with one appended import. Before publication,
merge that import with any newer loader changes and confirm the audited database
weights still match the intended server. Baseline installed loader SHA-256:
`a4cdf2fa12c723febe35a1ad66a6621af6382bea47507fe09a0f7900088136ca`.

The complete native Lua callback export was compared before/after. Exactly 184
item identities and 352 identified/unidentified description callbacks change;
all other callbacks, all 27,133 identities, resources, slots, effects and package
metadata are preserved. Comparison is against the explicit expected line
replacement map, not only a count of changes. Candidate execution passes.

Reproduce from `Server-Development/client-consistency-20260929/` with `audit.py`
then `prepare.py`; use the existing `gameplay-refine-audit-20260928/python-deps`
directory on PYTHONPATH. Evidence: `audit.json`, `acquisition.json`, `export.txt`,
`candidate.txt`, and `validation.json` (individual changes and payload hashes).

## Remaining boundaries

There are still 199 dynamic grant lines that the static acquisition tracer cannot
resolve. No live player holdings were queried. Absence of a static path does not
prove an item can never be obtained. This pass does not certify arbitrary combat
description text against every server script branch.

The earlier [costume visual audit](costume_visual_audit_20260928.md) remains the
native visual evidence: 37 of 52 effects visibly confirmed, 15 unconfirmed;
Red Flame, Taiwan 22nd Anniversary halo and moving footprints remain open.
This pass does not turn archive presence into a native visual pass. Previous
consumable, Kachua and costume repairs are preserved by the callback comparison.
