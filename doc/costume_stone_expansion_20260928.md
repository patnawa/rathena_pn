# Costume stone expansion — 2026-09-28

The reviewed catalog contains **497 physical stone materials**, up from 366,
across **26 boxes and 10 slot categories**. All original physical materials are retained; the obsolete Range Middle output is corrected with legacy compatibility.
The generated NPC tables and exact reverse indexes come from
`npc/custom/fashion_points/stone_catalogue.json`; its provenance keys link the
original catalog, official metadata and regional research.

## Coverage and implementation

| Addition | Physical materials |
|---|---:|
| Previously omitted functional/legacy stat stones, including Greed | 10 |
| Korean class stones from boxes 43 and 44 | 28 |
| Cosmetic effects and footprints | 51 |
| Thai Purified and Festa, normal and 100% variants | 28 |
| Thai Loft | 7 |
| Taiwan anniversary and visual stones | 7 |
| **Total added** | **131** |

The new Korean class stones include 54 combo records. Taiwan adds eight anniversary sets, with one legacy Range alternative and three compatibility records for older Range Middle enchants (66 expansion combo records total). Quoted skill references resolve
against the effective skill database. The source verification report compares
the 28 card scripts and 54 combos against their original Korean descriptions.
The arithmetic fixtures are script-derived examples, not an independent oracle
or proof of runtime combat calculations.

The finite Korean scan contains 439 physical-material candidates, all present
in this catalog. The Thai scan examined 18,447 item records with documented
selectors. This is broad verified regional coverage, not a proof of every
historical release in every Ragnarok region. **Red Flame 1003031 → 315313 is
excluded** until its exact visual effect association is established.

Sources: [official Korean September 7 client patch](http://ropatch.gnjoy.com/Patch/2026-09-07_live_client_3363_3364_1788764913.rgz),
[official Thai September 23 client patch](https://ropatch.gnjoy.in.th/patchfile/2026-09-23_live_client_3565_3567_1790134356.rgz).
See `costume_stone_reference_20260928.md`, `costume_stone_thai_reference_20260928.md`
and `costume_cosmetics_reference_20260928.md` for narrower claims and evidence.

Additional regional spot checks found the familiar class/footprint families in
[iRO Box 37](https://renewal.playragnarok.com/news/eventdetail.aspx?id=1977),
[iRO May 2026 rewards](https://iro.ragnarokonline.com/news/eventdetail.aspx?id=2026),
[Indonesia's January 2026 Box 26](https://ro.gnjoy.id/news/detail/2137), and the
[Taiwan guaranteed-exchange catalog](https://ro.gnjoy.com.tw/notice/Guide_View?id=217680).
These name-level comparisons are not full native catalog scans. Taiwan's
[23rd-anniversary promotion](https://event.gnjoy.com.tw/Ro/ACT_20251013_23rd/Event06)
also confirms a Dark Lord Magic Circle lower costume stone; its IDs are now confirmed by the current Taiwan client, and the implemented visual association is supported by the official illustration and native effect artwork. See `costume_taiwan_reference_20260928.md`.

## Compatibility policy

Use `@fashion`. Upper, middle and lower stat stones use costume slots 1, 2 and 3.
Garment stats use slots 1 and 2. Four visual categories use slot 4; Festa upper
uses slot 2. Purified/Loft garment slot 1 is a documented PN convention because
the official metadata does not publish a numeric slot for them.

Application is guaranteed under existing PN policy, including regional normal
materials whose official servers use a chance. Occupied slots are refused.
Recovery checks all supported locations on a multi-position costume and returns a deterministic compatible material in upper/middle/lower/garment priority when an effect ID is ambiguous. Each equipped inventory record appears only once. It returns
the 100% regional material where two materials share an output. Legacy Loft
25934–25940 retain their existing scripts/types so already equipped items keep
their bonuses; new application uses proper outputs 27427–27433 and recovery
accepts both. Official equipment-specific Loft doubling combos are not added;
none were present in the local database and they were not independently verified.

The existing compensated transaction path preserves the original inventory
record and unrelated equipment metadata. The expansion does not alter account
balances, award points, migrate equipped items or rebuild server binaries.

Range Middle material 25061 now applies official output 310330 rather than
legacy 29048. Existing 29048 cards retain their base bonus, recovery mapping,
and all three native combo bonuses through aliases guarded by the actual
costume-middle card slot. The Taiwan anniversary ranged set supports both
outputs without activating the legacy path for Expert Archer on unrelated gear.

## Client

The regional item-info overlay resolves **996 distinct material, output, legacy recovery and box
IDs**, with 272 full English overrides. Two older entries receive resource-only
repairs. All 94 referenced icon resources (376 inventory/collection/SPR/ACT
files) exist after the candidate merge.

Four explicit icon fallbacks are recorded: Whirlwind material/effect
1001616/313066 reuse the installed footprint icon; legacy Greed/Double Attack
29046/29362 replace nonexistent `Strength` artwork with the official generic
enchant icon. Existing descriptions on those two cards remain intact.

The repair GRF gains 140 resources while preserving its previous 177 entries.
These include Flower Garden, Blossom, Sprout, Kiel, Golden Aura, basic footprints, anniversary/Purified icons, reconstructed Ghost/Camellia artwork,
and two effect tables that preserve all existing definitions. Native Lua and
asset structural checks cover the additions. A subsequent actual-client audit
confirmed visible rendering for 37 of 52 native effect IDs; 15 remain
unconfirmed. See `gameplay_refine_audit_20260928.md` for runtime results and
limitations; this is not an all-effects visual pass.

Eight effect associations, including the Taiwan additions, are **inferences from matching native effect names and/or official illustrations**, not recovered direct card-to-effect tables. Their IDs and evidence are recorded in the validation and cosmetic research files. Red Flame did not meet this evidence standard and remains excluded. The 22nd Anniversary stone implements its current permanent numeric bonuses and combos, but its halo visual remains unverified and is explicitly marked unsupported in its tooltip.

Flower Garden, Kiel and Purified assets come directly from official Thai patches. Basic footprints come from an official Korean patch; Golden Aura and anniversary icons come from official Taiwan patches.
Blossom/Sprout art comes from a patch mirror and matches official indexed paths
and expanded sizes; its bytes have not been independently authenticated against
the encrypted official container. Source URLs and hashes are retained in the
audit workspace manifests. Ghost/Camellia use documented reconstructions from a public asset mirror: all native animation fields round-trip at float32 precision, and BMP RGB/transparency match the mirrored decoded textures. These are not authenticated original file bytes.

## Validation and release evidence

- Generated table parity and five effective item/combo tests pass; all 497 forward type guards pass source-interpreted checks.
- Native NPC VM: 77 cases, 2,367 assertions, UBSan, no memory leaks; four negative
  mutations rejected. Covers slot guards, cost compensation, shared effect
  recovery, Festa slot 2, legacy Loft recovery, and nine multi-position recovery cases.
- Isolated SQL/login/char/map/web startup parses the complete candidate; the
  existing six gameplay smoke scenarios check integration and persistence.
  Those smoke scenarios do not claim combat testing of every new bonus.
- Two fault-injection tests prove temporary-file cleanup and successful rollback after a second-file fsync failure.
- The signed client delta preserves prior manifest entries and executable hashes.
  The guarded production update uses an idle, graceful map-only restart, backs
  up all changed files and checks startup, other containers, balances and points.

Machine-readable candidate, startup, deployment, installation and publication
receipts live in `Server-Development/costume-stone-audit-20260928`. Deployment
and publication are established by their respective receipt flags, not by this
design report alone. Reversible backups are retained there and on the server.

## Completed deployment

The server data update was deployed on September 28, 2026 with a clean, idle
map-server restart. All services were healthy afterward; account funds and
Fashion/Gold Points were unchanged. The installed client passed all 5,642
manifest file checks. Signed launcher release `client-20260928-costume-stones`,
sequence `2026092804`, is published; its signature and all six changed download
objects were verified. The standalone delta is 14,995,582 bytes.

Use the PN launcher to update, restart the client, then open `@fashion`.
Deployment does not remove the documented coverage and visual-verification
limitations above. The previous server files, local client files, and signed
launcher feed remain available as rollback copies.
