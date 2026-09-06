# Depth 1 document exchange dependency gate

2026-09-06. `tools/ci/biosphere_document_callback_audit.py` is a separate,
fail-closed current-content consistency auditor for
`Depth Research Administrator#bio_d1`, not a general script sandbox. It does
not alter NPCs, source, databases, existing gates, or the active client.

## Acceptance and fixture API

`validate(ROOT, profile=None)` must first accept the complete broad callback
gate, then accept the existing material/ba_in01 QuestInfo gate, then match the
five manually reviewed document sections below. The material gate itself also
invokes the broad gate. The two broad manifests must match. No exception or
default checksum failure selects a different profile or relaxes the policy.

The returned manifest exposes:

- `material`: the complete accepted material manifest, unchanged, for the
  existing native `prepare_callbacks` adapter;
- `document.record`: the effective full document item definition;
- `reputation.ordered_records`: all 13 selected records in native import order,
  plus `selected_record` and `selected_ordered_records` for reputation 6;
- `constants.ordered_records`: all 11 selected constant rows in order, plus
  the exact unique `binding`;
- `engine.files`: nine additional explicit source/header hashes;
- `service`: the exact service/access body hashes and reviewed economic domain.

All acceptance paths invoke both prerequisite gates unconditionally. An explicit
`--profile live-20260906` forwards that same choice to both prerequisites; this
supports the parent's separately reviewed preserved-source profile, but is not
a claim that these local tests inspected a running server or its registry.

`collect_evidence(ROOT)` / `--collect-only` returns
`acceptance: UNVALIDATED_COLLECTION` and prints an explicit warning. It cannot
be combined with profile acceptance or negative controls. There is no
autoaccept, rebaseline, or skip-prerequisite option. Private collectors are
fixture inventory utilities, not acceptance APIs.

## Exact dependency contract

The selected item is `1001289 / Bar_D_Docu_1`, an Etc item with native Weight 1
(0.1 displayed weight). Its complete effective record, including all seven
Trade restrictions, is pinned. It has no script, equip location, stack limit,
or behavior flag. It was not one of the previous material gate's 56 records;
the new gate adds it explicitly rather than assuming coverage by that list.

The economy is the existing custom two-documents-for-three-points exchange.
The last pair still grants one or two points when necessary to reach 5000.
No Zeny, item output, capacity probe, refund mechanism, new acquisition route,
or new reputation requirement is introduced. The service rechecks the original
BaseLevel >=250 and `ep17_2_main >=33` access and `ba_in01` location after its
last input suspension. Current input, documents, and reputation determine the
final order without silently reducing it.

`db/reputation.yml` selects `db/re/reputation.yml` and then
`db/import/reputation.yml`. All three selected source files, import edges and
13 ordered definitions are pinned. Reputation 6 is defined once with variable
`RepPoints6`, minimum -5000, maximum 5000 and generator Visibility `Exist`.
The supported in-range domain therefore allows at most 3334 pairs / 6668
documents in one order, safely below the native signed-16-bit deletion limit.

The reputation Renewal file also declares
`db/re/generator/reputation.yml` with `Generator: true`. The auditor retains
that edge as **unselected**. `YamlDatabase` defaults `shouldLoadGenerator` to
false, and `ReputationDatabase` only enables it under `MAP_GENERATOR`. Native
loading skips an import whenever a Generator flag is present unless both the
loader and flag are true; even a present `Generator: false` is skipped. This
gate targets ordinary Renewal map-server, not generator output. It deliberately
does not claim the skipped file's contents are runtime dependencies.

`db/const.yml` selects `db/import/const.yml`, then
`db/import/zero_cell_const.yml`. All three files and all 11 records are retained.
`REPUTATION_BIOSPHERE_DEPTH1` must be the one plain constant row with Value 6,
not a parameter or duplicated definition. Native constant registration refuses
overwriting an existing binding: this is not modeled as last-row-wins.

New unknown YAML fields/header versions/import keys and unsupported modes or
types are refused. Selected missing imports, cycles and root escapes fail.
Every original selected import and row remains part of the pinned evidence;
there is no target-only projection that hides other definitions. Ordered
reputation rows are exposed to the native parser fixture instead of treating
the Python scalar overlay as a proof of arbitrary future parser behavior.

## Native callback closure

The source review covers `get_reputation_points` and `add_reputation_points`
in `script.cpp`, the registry functions in `pc.cpp`, their declarations and
constant registration, and `clif_reputation_type` / packet layouts. Common
YAML source/header selection semantics are explicitly pinned too; the broader
gate additionally pins all applicable source and configuration files.

For a loaded, valid attached character:

1. `get_reputation_points(6)` reads the numeric permanent character variable
   via `pc_readreg2 -> pc_readregistry` and clamps its returned view.
2. Ordinary native `delitem` removes the already-preflighted document quantity.
   Deletion invokes `pc_show_questinfo`: this is **not** an empty callback path
   on `ba_in01`. The mandatory material gate preserves the real 33 literal
   QuestInfo expressions on 19 owners, their ordered registrations and quest
   dependencies, achievement imports, and Weight50/90 notification closure.
   The document native fixture executes the conditions with the actual VM;
   that behavioral proof is separate from this source consistency gate.
3. `add_reputation_points` reads the raw stored value, adds the selected gain,
   clamps to the exact reputation limits, and calls `pc_setreg2`.
4. For the unprefixed numeric `RepPoints6`, index zero, the native path is
   `pc_setreg2 -> pc_setregistry`. It updates or allocates the native registry
   record and marks its update flag and `vars_dirty`; no array-index callback
   is entered. This synchronous path has no NPC script, yield or SQL call.
5. The builtin sends one `clif_reputation_type` notification. Under the reviewed
   PACKETVER20260219 build it writes packet 0x0b8d with success true and one
   entry containing 64-bit type 6 and signed 64-bit points, then sends to SELF.
   It does not consult reputation-group definitions or run an NPC script.

The native registry setter can refuse writes before character variables are
loaded. This auditor is not a transaction guarantee for an invalid/unloaded
session. It also does not establish SQL crash durability, reconnect behavior,
arbitrary injected persisted scripts or out-of-range privileged registry
edits. In particular, the getter clamps its view while the additive builtin
uses the raw stored value: corrupt values below -5000 are outside the valid
in-range proof, not silently repaired by this patch.

The material gate includes Get_Item/Goal_Achieve conditions for its broader
services, though this document exchange only deletes an item and does not
call the item-add achievement branch. Its conservative inherited dependencies
are retained rather than weakened for this one service. Weight notification
and network/world fixture boundaries remain explicit in the native test's
separate receipt. No graphical client, live registry or running-binary
attestation is asserted in this gate's manifest.

## Source consistency and reproducibility

Source bytes are decoded with UTF-8 surrogateescape and newline-normalized,
preserving all non-newline bytes. Complete section hashes use sorted compact
JSON; public proof consumers must use the exact hash convention stated by
their runner, not assume all receipts use this canonical encoding.

The accepted material manifest's broad hash must match the independently
accepted broad manifest. Scoped reads are checked against the same broad,
material and document file inventories. Reputation, constant, achievement and
quest files outside the original broad inventory are reread using a fresh
Reader, and the full broad manifest is then verified again. These checks detect
changed snapshots during validation; they are not a filesystem lock against an
adversary changing files after validation. Deployment must still bind the
reviewed manifest and runtime source to the installed artifact.

| Section | Reviewed canonical SHA-256 |
| --- | --- |
| document | `900df4193e2cf2ea2c1eab70ccaa83020cdad758565df5deef15ab70cbeafc50` |
| reputation | `ef75e783cbe4231045ce3deff668923486b0dc4f595bc826e409e2db70de9604` |
| constants | `648df3c16e898c0c7bd421def43e406f34b5a828e47f8bdbc11b1489832027fa` |
| engine | `7be371b0e512ebea566716e9a0bec6d297b43b2fcc10ffefd7e5083f58fd0a44` |
| service | `9518354fe151f2d357d32dcdf89301e47a46c1cda14fc8a0a34975a767b11472` |

The reviewed full Depth source is LF/raw
`3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326`.
The service body hash is
`946477abd473a8f90f5282b00194dfb348de7e015e3c2d76c587915c0d14a876`;
the original access helper body hash is
`acb46cf0c981b1115de297491ce8e3d4defb20b9ca797dacb711d7380a6b335b`.
These were reviewed against the actual source, not accepted merely because
collection emitted a checksum. Future changes require a new explicit review.

## Verification

Run from the repository root with Python 3 and PyYAML:

```sh
python3 -B tools/ci/biosphere_document_callback_audit.py
python3 -B tools/ci/biosphere_document_callback_audit.py --negative-controls
python3 -B tools/ci/biosphere_document_callback_audit.py --manifest
```

The default public validation and all **40 negative controls passed** with exit
zero. Actual prerequisite acceptance brackets the controls and the complete
before/after accepted manifests are required to be identical. The controls
include document identity/weight/Trade/callback changes; reputation variable,
bounds and schema changes; constant replacement/parameter/duplicate binding;
missing, additional, disabled, cyclic and escaping imports; Generator and
header schema changes; registry, packet, builtin and import-source changes;
service and inherited QuestInfo mutations; tampered evidence and false live
claims; four late-file drift checks; both prerequisite rejection/explicit
profile-forwarding tests; and unknown-profile rejection.

The frozen gate's raw SHA-256 is
`69ba466e4c4e1cdd9a97497514b4c33fc642170d7c7c93338dd083c9e6e56bfa`.
The runtime Depth source was checked again and remains the exact reviewed
`3b94a446...` bytes. Negative controls use Reader overrides and mock only
explicitly failing prerequisite-order tests; no source file is rewritten.

CLI collection classification is checked separately: collection must emit
unvalidated outer and nested material manifests, the explicit NOT-accepted
warning, no PASS string and all three attestation flags false. Combining
`--collect-only` with `--negative-controls` is rejected by argument parsing.

The separate [native exchange audit](biosphere_document_exchange_audit.md)
owns behavioral case totals, sanitizer/allocator evidence and fixture limits.
This gate does not replace it. No preserved-live acceptance, live registry
inspection, graphical client check or deployment result is claimed by this
local document.
