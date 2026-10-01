# Native costume visual audit — September 28, 2026

The actual Ragexe client received all 52 supported native effect IDs in a
disposable private realm. Retained screenshots confirm visible rendering for
37 effects; 15 remain unconfirmed. This is not an all-effects visual pass.

Unconfirmed IDs: `96, 257, 258, 227, 153, 173, 176, 4, 15, 16, 17, 18, 23, 51, 244`.
The last six effects appeared briefly and cleared; a blank later frame does not
establish an asset defect. Separate structural validation checked 59 STR
animations and 714 unique textures without structural errors.

The first native process exited unexpectedly after the full cycle, without an
observed error dialog. Its cause remains unknown. The second launch survived
the targeted cycle. A movement helper lost its attached player, leaving moving
footprint coverage incomplete. No production client asset defect was reproduced
or fixed. Red Flame and the Taiwan 22nd Anniversary halo remain unresolved.

The private game/database realm, tunnels, login adapter, current client and
temporary GRF Editor were stopped. Three older elevated private client
processes could not be closed under the current Windows permissions.

Full PNG captures, the per-effect JSON report, contact sheet and cleanup receipt
are retained in `Server-Development/gameplay-refine-audit-20260928` as
`visual-runtime-report.json`, `visual-contact-sheet.jpg`, `visual-frames/`,
`visual-qa/` and `visual-cleanup.json`. Raw captures are audit artifacts rather
than production client payloads.

See [the gameplay/refinement audit](gameplay_refine_audit_20260928.md) for
deployed fixes and Fashion Point box purchase/open validation.
