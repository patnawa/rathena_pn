# Gudra installed transaction repair: source and native proof

Date: 2026-09-06, finalized 2026-09-07. The reviewed candidate is installed
locally. Current raw source SHA-256 is
`8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe`;
the frozen original raw SHA-256 is
`50d771b504dfa2eafefcbafee9680eebdadfc2edc08e21428051aebb2dc78b0f`.
The earlier findings and ordinary reachability evidence are in
`doc/episode18_gudra_handin_audit.md`.

## Exact candidate scope

`tools/ci/episode18_gudra_transaction_test.py` owns a deterministic transformation
of the entire frozen original. It makes ten reviewable substitutions only
inside `Folklorist Gudra#ep18`. The runner accepts either exact endpoint,
reconstructs the other by exact inverse/forward transforms, and never installs
files itself. Every substitution has an inverse, and the reconstructed original must be byte-identical
after LF/CRLF normalization. Non-newline edits fail closed. The emitted files
are `before.txt`, `after.txt`, `candidate.diff`, and `proposal.json` in an
explicit artifact directory outside the repository.

| Source identity | SHA-256 |
| --- | --- |
| Whole original, LF-normalized | `5f0b2af75840828f4cac5377dd1c0a5fd368fa0d1d612a4fb9a0a449707d5926` |
| Whole proposed candidate, LF | `8d3af9e9418e085c84338973df576689fa267bc6c62f0cc4a9be482c7d3002fe` |

The proposal removes the incorrect global 20-Amethyst capacity gate and checks
the actual branch output. All existing Note issuance/recovery sites gain exact
capacity protection, and the missing first-hand-in recovery is added. Initial
quest issuance checks capacity before setting the three story quests. Original
unconditional one-Note issuance on new acceptance is preserved, including when
an existing Note is already held; only recovery is conditional on absence.

Both hand-ins recheck current `wolfvill`, `ep18_main >= 36`, their exact normal
story/hand-in states, absent cooldown 16559, and a current Note after the final
dialogue yield. The daily branch retains the original earlier expiration
cleanup, then expects no cooldown at commit. The first branch cannot ordinarily
have 16559: that cooldown is initially created only after completing 16554;
daily paths require 16554 already complete. Checking absence also prevents an
injected cooldown from causing duplicate `setquest` after payment.

All EXP calls, +100/+30 reputation additions, reward amounts, quest mutations,
operation ordering, 1000-reputation main-story progression and 04:00 daily
policy remain unchanged. The preflight daily amount is computed from fresh
reputation at the **post-+30** boundary: 4969 permits three items, 4970 permits
four. The original post-addition calculation and grant still execute unchanged.

## Exact inventory contract

The new local `L_GudraPlain` subroutine supports only the two pinned zero-weight
Etc identities, amounts 1 through 30000, and optional one-Note debit when the
output is Amethyst. It rejects unknown arguments, invalid allowed-slot counts,
or drift from the zero-weight premise. It reads `getinventoryslots()` and
`MAX_INVENTORY`, then snapshots inventory once. No caller uses the snapshot
after the first actual grant.

The native contract is deliberately not reduced to first-same-ID behavior:

- `pc_additem` matches ID, bound value, zero expiry, UID and all four cards.
  Refine, identify, attribute, random options, favorite and equip are not stack
  comparison fields. The first compatible row is terminal on overflow or a
  row index beyond the currently allowed slot cap; an available empty slot
  cannot rescue that failure. The native search scans the physical inventory,
  including beyond the allowed cap, before deciding.
- A plain `delitem Note,1` searches ascending rows, preferring unequipped,
  unrefined and cardless Notes. With none preferred, it uses the first same-ID
  row in its fallback pass. It does not add bound/expiry/UID/options filters.
  Because the Note is Etc, pet-deletion exceptions do not apply.
- The helper credits an empty slot only if the actual selected Note row is
  inside the allowed slot cap and has one unit. A multi-unit Note stack does
  not release a slot. A full plain Amethyst stack still fails even when the
  Note would release a slot. This preserves otherwise valid full-inventory
  hand-ins without inventing a pre-payment empty-slot restriction.

The input premise is a valid native inventory/cache with positive amounts and
the current two item definitions. `getinventorylist` then visits ascending
physical indices. This is not a generic capacity API for arbitrary equipment,
GUID-producing or scripted definitions, and generic `checkweight` is unchanged.

## Source gate and callback scope

`tools/ci/episode18_gudra_callback_audit.py` is a separate fail-closed public gate.
It requires the existing broad source/content gate, plus independently reviewed
section pins for these items, weight statuses, reputation/constants, ordered
quest and achievement DB/import graphs, relevant native/config files and
wolfvill registration/declaration/duplicate/dynamic-producer evidence.

The source inventory includes 900 enabled NPC files, 14 include configurations,
**72 wolfvill conditions / 35 owners**, and 44 local duplicate declarations.
Seventy-one conditions come from Episode 18 and one from Episode 19. Conditions
only query the reviewed quest states, item counts and scalar variables. The
duplicate parent bodies add no conditions; the global registration/refresh and
unitwarp inventories are pinned. No dynamic duplicate or UNPC_MAPID producer
is accepted. This source scanner identifies dependencies and rejects drift;
it is not a proof engine for arbitrary scripts.

The gate includes the ordered **361-achievement** source graph and exact seven
Get_Item conditions. The native fixture loads and executes those seven, whose
ARG0 thresholds are false for both current zero-sell-value items. Item mutations
still execute native achievement ARG0 setup/cleanup. Quest additions, deletions
and completion invoke the real wolfvill `pc_show_questinfo` loop, not a no-op
substitute. Its wrong-size display guard and native reinitialization are tested
explicitly so an accidentally empty fixture cannot claim callback coverage.

Nine source negative controls reject: added Note material script, changed Note
sale flag, mutating item-achievement condition, mutating existing wolfvill
condition, added wolfvill registration, duplicate parent acquiring a condition,
new enabled NPC source, new achievement import, and changed native stack matcher.
`--collect` is always labeled `UNVALIDATED_COLLECTION`, even if the current
section hashes happen to match. It cannot update or accept new pins.

After installing Gudra, the pre-Family-overlay QuestInfo section was
`a8ec137e0b9d2aeb42640539e55a7fd1ef455d1d2941a46f515d4f1696508e51`.
The installed Family overlay changes it to
`cc70ca09d9974c9860b3b258be864ce35fcd29989f838d2d3859943357712409`.
A recursive comparison proved the sole semantic addition is
`FamilyReputation.txt:89 questinfo_refresh();`; the unchanged registration only
moved from line 111 to 149. All other Gudra sections are byte-identical.

Normal dialogue admission and loaded/in-range raw `RepPointsWolf` are the
registry contract. The getter clamps displayed values while addition uses the
raw value before clamping; arbitrary out-of-range privileged SQL/GM state is not
silently folded into the normal-state proof. No live registry query or persisted
bonus-script inspection was performed by this subtask.

## Native fixture and explicit boundaries

`tools/ci/episode18_gudra_transaction_test.cpp` is combined with the exact tracked
crown fixture's transport/world boundary prefix. The runner freshly compiles or
reuses only source/header/compiler-flag/object-hash-identical sanitizer objects
for pc, script, itemdb, clif, achievement, quest and malloc. All remaining link
objects and libraries, the resulting executable, generated fixtures, runner,
driver, inherited helpers and source headers are hashed in the receipt.

The proof executes actual Gudra and all three unchanged story NPC bodies,
native select/Next suspension and resumption, loaded persistent numeric
registry reads/writes and dirty flags, actual inventory helpers/grants/deletion,
native ordered quest parsing and mutation/partition/save flags, and original
wolfvill registrations. Sixty-seven actual quest definitions and their real
monster identity dependencies are loaded. All 72 conditions receive true and
false probes derived from their expressions and real quest/registry state.

The fixture observes item-achievement execution at the actual cross-object
`run_script` entry. An internal call within achievement.cpp to its own condition
evaluator may bypass a linker wrapper; counting only that wrapper was rejected
during preparation. The current counter records the real seven condition VM
entries and their false results, without rewriting condition scripts.

`pc_gainexp` is an **explicit recorded boundary**. The real getexp builtin
receives the original operands, applies the configured fixture quest rate 100,
and calls the boundary with the unchanged base/job amounts and flag 1. The
fixture has no homunculus. It does not claim to execute EXP level-up,
equipment/status recalculation or full level-up gameplay. Weight notification
packet/status transport is similarly explicit; the source gate covers its
current Weight50/Weight90 dependency. Native quest save requests are recorded
at the chrif transport boundary with CHARSAVE_QUEST enabled, not sent to SQL.

World lookup, client packets, logging/persistence transport and transient `@`
register storage are explicit doubles; persistent `RepPointsWolf`, `ep18_main`
and achievement ARG0 route through the real loaded registry. No networking is
allowed by the fixture's kernel boundary. None of this proves remote save,
reconnect, crash atomicity or graphical-client behavior.

The stale-inventory regression changes inventory at genuine Next suspensions;
these are **injected-pause fixtures**. The ordinary party-loot receiver route
remains separately source-derived evidence, not claimed as a real party pickup
executed here. Likewise actual Note removal plus all three story NPCs proves
the lost-item story/recovery sequence; normal zero-Zeny shop sellability is
source-grounded separately, not a fabricated native shop/network test.

## Coverage and reproducible invocation

The candidate suite covers normal first/daily rewards, all valid reputation
boundaries around progression/cap, one/multiple Notes, acceptance and hand-in
yield changes, missing-Note recoveries, preserved preexisting-Note issuance,
active/expired cooldown, cancellation, unknown/invalid helper arguments,
allowed physical-slot limits and full inventories under actual configured
655360-command / 2048-jump limits. It includes 108 direct comparisons of the
exact candidate helper with real native one-Note deletion plus `pc_additem`.

The seven genuine-original controls are one ordinary successful story chain,
two stale-capacity reward-loss cases, one wrong-item initial-delivery failure,
one unrecoverable first Note, and two already-safe daily refusals caused by the
overlarge check. The latter two are not mislabeled as material loss. Exactly
three getitem errors and one delitem error (with their four warnings) are
expected only in the original run; any additional zero-exit diagnostic fails.

Development execution is explicitly `UNVALIDATED_NATIVE_TEST`; `--prepare-only`
only compiles. Accepted proof omits both switches and requires the public gate
before and after execution with identical manifests. The final installed-source
artifact is `../episode18-gudra-native-label-v2-20260907`.

```text
python3 -B tools/ci/episode18_gudra_callback_audit.py --negative-controls
python3 -B tools/ci/episode18_gudra_transaction_test.py --proposal-dir ../episode18-gudra-native-label-v2-20260907 --native --object-cache ../episode18-gudra-native-installed-repinned-20260907
```

The object-cache argument is optional; it never permits a changed object,
source, header set or compiler flags. LF and CRLF original forms produce the
same candidate; three non-newline source controls, seven zero-exit diagnostic
controls, and three retained-artifact controls are mandatory in the runner.
The runner itself does not mutate the runtime, gates, or engine.

## Completed installed-source receipt

The final public-gated run completed with identical full callback manifests
before and after, exact runtime/source/artifact readback, clean ASan/UBSan and
one clean allocator teardown per process:

| Native mode | Cases | Assertions | QuestInfo condition calls | Actual item-achievement VM entries |
| --- | ---: | ---: | ---: | ---: |
| Installed repaired source | 416 | 791285 | 53237 | 1120 |
| Genuine original controls | 7 | 30203 | 1917 | 14 |

The final `receipt.json` SHA-256 is
`fe40476cceba69a88b0ae094a8a32e8e726d42b86113909748ade9ca2d377c7d`;
its callback-manifest hash is
`742ea7a49abe96ad3897deacd825cc11fcb378c1cfc75a5102ed36408f640327`.
The native executable hash is
`03760e377c3af9d0dc3fd3bb7e7b272059ca52f4cbe322a09bafb22d2dcae041`.

The current public gate and all nine source negative controls also pass. The
runner additionally passed its mandatory LF/CRLF and three source-rejection
controls, seven diagnostic-rejection controls and three artifact-binding
controls. The candidate includes successful native fallback Note deletion,
bound/UID/expiry-independent preferred Note selection, metadata/cache equality,
and reputation additions crossing to exactly zero with real dirty flags.

The repaired source is installed locally and the current runner accepts both
exact endpoints. The installed-source gate and native receipt pass. This remains
a bounded local proof, not SQL persistence, reconnect, graphical-client, or
crash-atomicity evidence.

## Live deployment addendum — 2026-09-07

That bounded-fixture statement does not describe the later deployment. The
exact repaired `quests_18.txt` was deployed in the reviewed 13-file r7 release;
live readback matched `8d3af9e9...`, the map reached the exact POST structure,
and SQL remained identical across stop/install/start. This establishes deployed
source and startup health while retaining the fixture's reconnect, graphical
client, and crash-atomicity boundaries. See the
[deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
