# Equipment-switch deletion: native before/after proof

2026-09-06. Root owns the two narrow `src/map/pc.cpp` changes. This task owns
only `tools/ci/equipswitch_deletion_test.py/.cpp` and this audit; no NPC, database,
client, gate, server, or Git mutation is performed by the regression.

## Ordinary reachable stale-cache defect

With equipment-switch enabled, register a Varmundt base armor, garment, boots,
or ring in the switch window before using Ellie#biosphere_equipment. Native
`pc_equipitem(...,true)` writes the inventory item's complete `equipSwitch`
mask and its corresponding `sd->equip_switch_index` entries. Registration is
not ordinary wearing: the item's `equip` field remains zero.

Ellie's ID-only `delitem` prefers unequipped, unrefined, cardless records and
does not exclude switch registration. The old `pc_delitem` decrements amount
and weight, then clears the exhausted inventory record. It invokes ordinary
unequip only when `equip` is nonzero and never cleans the switch cache.
`clif_delitem` only notifies item deletion. Ellie's subsequent `getitem` may
reuse that free index for a new imprinted item whose switch mask is zero,
while the old switch cache continues to reference the index.

This is not dependent on injected scripts, GM equipment bypass, large material
stacks, or a state change during a dialogue pause. The dedicated fixture uses
actual ordinary registration, the actual unchanged Ellie NPC, real native
item deletion and addition, and all fifteen existing exchange outputs.

## Minimal parent correction

Inside the full-depletion branch, call `pc_equipswitch_remove(sd,n)` before
ordinary unequip and item clearing. The helper:

1. Returns immediately for an item with no switch mask.
2. Clears every EQI cache entry equal to the consumed index, including all
   entries belonging to two-hand and multi-slot equipment.
3. Sends one removal notification using the original complete mask, then zeros
   the item's mask. The native transport implementation encodes this as 0xa9a.

The existing quantity decrement, weight decrement, ordinary unequip, item
clearing, item-deletion notification and QuestInfo sequence remain unchanged.
Partial deletion does not enter this branch and retains registration. The
type flags 1/2/4 suppress their documented item/weight/recalculation behavior,
not the independent switch-removal acknowledgement; all eight combinations
are tested.

Root additionally rejects `n >= MAX_INVENTORY` before the first array access.
This is a defensive native API bound, not a claim that Ellie's ID-based search
normally supplies such an index. The fixture tests -1, MAX_INVENTORY and INT_MAX
on fixed code. A separate old-code process demonstrates the exact upper-bound
array violation under UBSan; it is not accepted merely for crashing somewhere.

## Immutable source provenance

- Original Git commit: `50717df9e48a6a762303a05e5d92dcc65092b8bc`.
- Original `pc.cpp` normalized-LF SHA256:
  `416f501be40a1346e9d07ae54a3b4e59dbf8a7780a1dfde3729217d3c9aef250`.
- Reviewed patched raw `pc.cpp` SHA256:
  `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403`.
- Unchanged Ellie source `npc/custom/varmundt_biosphere_quests.txt` raw SHA256:
  `787f6ff54a4c43a60510e4a606dc5bd33a80c502853f113ad6756e257ac5aa5a`.

The runner obtains old source with read-only `git show`, requires the explicit
old normalized hash, and verifies that the normalized current source differs
only by the approved index bound and full-depletion cleanup lines. It also
requires both NPC and access-function sources equal the immutable old versions.
Their raw copied fixture bytes are rechecked against the final checkout.

The root-owned broad callback gate is mandatory before and after proof. There
is no skip or rebaseline option. Object reuse during iterative development is
permitted only when that callback context and the exact production source hash
match; the final proof directory freshly compiles all production units and the
old source. All compiled and fixture source hashes must remain unchanged during
the proof.

## What actually executes

The runner creates two executables: one with old `pc.cpp`, the other with
current `pc.cpp`. Both freshly compile current `script.cpp`, `itemdb.cpp`,
`clif.cpp`, and `malloc.cpp`; PACKETVER20260219 and ASan/UBSan are mandatory.
Other existing map-server support objects are linked, not described as freshly
instrumented source. The driver is appended to the unchanged crown test's
tracked boundary helper prefix rather than modifying that older fixture.

Real code exercised includes:

- Effective item records parsed by the native item database parser.
- `pc_isequip`, class/job/level checks, permission check, equip-point resolution,
  and `pc_equipitem(...,true)` registration. Subjects are level-250 Dragon
  Knights with no permissions, including no USE_ALL_EQUIPMENT bypass.
- Actual two-hand Slayer 1151 and multi-head Goggles 2224, as well as all four
  Ellie base types. All mask cache entries are checked after real registration.
- `pc_delitem`, `pc_additem`, `pc_equipswitch_remove`, script `delitem`, native
  `getitem` and UID generation, Zeny assignment, and the complete unchanged
  Ellie equipment NPC/access function parsed and run by the real script VM.

Explicit boundaries and limits:

- Switch-add/removal transport functions are observed argument doubles. The
  test proves count, target index, original mask, success code, and callback
  ordering; it does not claim wire-byte emission or client rendering. The
  callback also asserts that every matching switch-cache reference has already
  been cleared while the item's original mask still exists.
- Registry persistence, item/Zeny logs, dialogue transport, ordinary item/weight
  notifications, and world lookup/event delivery are fixture boundaries. Native
  intra-`pc.cpp` QuestInfo traversal sees an uninitialized/empty fixture map,
  not the full real ba_in01 QuestInfo registry. Achievements are disabled in
  this narrowly scoped cache test. The unchanged world's payment callback
  closure is not claimed to have been simulated.
- Switch registration does not ordinarily equip the item or run its equipment
  Script. This fixture therefore does not claim a live status recalculation,
  mounted state, or an arbitrary worn-item UnEquipScript transaction proof.
- Kernel network operations are denied. No SQL, running server, active client,
  or deployment is touched.

## Coverage

- Full deletion: six real registration types times all eight type-flag values.
  An unrelated switched item and every unrelated cache entry/metadata record
  are retained; each target cache entry is cleared on fixed code and proven
  stale on old code.
- Partial amount branch: all eight flags, with registration and every other
  item field preserved and no removal notification. This deliberately uses
  a stacked-equipment fixture to exercise the generic amount branch; it is
  explicitly not a claim of ordinary stacked-equipment acquisition.
- Defensive duplicate-cache fixture: every EQI entry points to the deleted
  index despite the narrower visible mask. The helper clears by actual index
  equality, including entries outside that mask. This is a defensive invariant
  test, not an asserted ordinary registration state.
- Unregistered full deletion: all eight flags, no extra switch packet.
- Rejections: negative index, upper bound, INT_MAX, empty cell, zero/negative
  amount, excessive amount and missing item-data pointer. Inventory bytes,
  weight, Zeny, both caches and all metadata pointers remain unchanged.
- All fifteen actual Ellie exchanges: normal unrefined/cardless registered
  base selection, exact prices/runes, consumed-index reuse, fresh output ID and
  generated UID, fresh-output metadata, surviving bound rune fields, unrelated
  switched item fields and all cache entries. Added binding/favorite/option
  fixture metadata verifies preservation boundaries; ordinary acquisition of
  every metadata combination is not claimed. Existing destructive exchange
  semantics remain unchanged rather than silently transferring metadata.
- All fifteen Ellie cancellations preserve inventory, payment and registration.

The original engine process expects the stale cache, so a passing negative
proof means the old defect was actually reproduced. The patched process expects
its absence. The third process invokes exactly old `pc_delitem(MAX_INVENTORY)`
and must fail at the old guard's item-array access. The checker requires the
unique native index/limit marker and exact diagnostic source line/index/type,
and rejects unrelated errors. An expected-abort child has no clean allocator
teardown; both normal processes must have exactly one clean allocator report.

Seven zero-exit warning/sanitizer/allocator negative controls exercise the
normal-process output checker. Source/hash/gate drift, nonzero normal exit,
missing markers, missing clean allocator teardown, and unexpected diagnostics
fail closed. Each child's stdout/stderr is retained with the artifact.

## Reproduction

From repository root in WSL:

```sh
python3 -B tools/ci/equipswitch_deletion_test.py \
  --native-build-dir ../equipswitch-deletion-native-final-20260906
```

Final fresh-build proof passed with the strengthened output/provenance checks:

- Fixed: **103 cases, 8097 assertions**, clean ASan/UBSan and allocator teardown.
- Old stale-cache reproduction: **101 cases, 6659 assertions**, clean ASan/UBSan
  and allocator teardown; the tested old defect is the expected postcondition.
- Separate old bounds process: exactly `pc_old.cpp:6107:47: runtime error:
  index 200 out of bounds for type 'item [200]'`, after its unique native
  `pc_delitem index=200 MAX_INVENTORY=200` marker. Fixed upper bounds reject
  cleanly. No unrelated failure is accepted as that reproduction.
- Seven zero-exit diagnostic negative controls passed; source and raw NPC/access
  fixture hashes, plus mandatory broad gates, matched before and after proof.

Final artifact directory:
`../equipswitch-deletion-native-final-20260906/`.
It contains both executables, original pc.cpp, exact item/NPC/access fixtures,
all three stdout/stderr pairs, build provenance and `receipt.json`.

Receipt SHA256:
`f10154c4eb97c111a071b920f9ed2c03668f269f74279b177d02aa05ab72055f`.
Fixed executable SHA256:
`2db33d700b67da53e575cadacac0d5a6c804a8e2dc9133eb2145a1dd9f793395`.
Old executable SHA256:
`91e247e0c546a8123f3057db329fec67c4a848836a0986392cdbb0574bbf69cd`.
Reviewed callback manifest serialization SHA256:
`b7c4f9fbfcb4731104b92f164bbc3677de44b0086b8d0d73f39996f2b0c68c90`.

The deferred Omega/Depth fusion payment and equipment-selection findings are
preserved separately in `biosphere_service_followup_audit.md`. No NPC correction
or broad inventory-capacity change is bundled into this native cache fix.

## Checkout-newline portability follow-up

Root subsequently corrected only the runner's fixed-source acceptance pin to
use normalized LF content, so an equivalent clean Linux or CRLF Windows checkout
is not rejected merely for newline form. The explicit fixed normalized SHA256 is
`660c0bcc0e9efeaafc65c862fb08d97de8d1dd2648548eeb1714919f247ec70f`.
The immutable old normalized pin and exact two-change comparison still apply;
raw source hashes remain recorded and checked throughout each actual run.
No runtime source bytes or engine gate constants changed in this follow-up.

Four in-memory old/current LF/CRLF combinations passed; four corresponding
non-newline semantic mutations were rejected. The updated runner was rerun in
the new `../equipswitch-deletion-portable-20260906/` directory, retaining the
pre-deployment fresh-build evidence above. It reused five sanitizer production
objects only after matching their exact source hashes and callback context, and
freshly compiled the original `pc.cpp` and combined driver. Both executables
were relinked. This is not described as a second all-units-fresh compilation.

The complete native suite passed again: fixed 103 / 8,097; old 101 / 6,659;
the exact isolated old index-200 diagnostic; seven output rejection controls;
and unchanged source/raw NPC/gate checkpoints. Updated runner SHA256 is
`6b1127199a489073b3112bc904b6b3b537de8b9ce16a4bf547030fff076bb1e0`.
Portable-run `receipt.json` SHA256 is
`f5ee67cc0a2c7bc4cf2f5200c1fe2094dd381a33894a1fd5ca9646ef116a87ca`.

## Material-service batch regression, 2026-09-06

The earlier whole-quests-file preservation assertion predates the independent
Omega material repair. It now accepts only the exact reviewed old/new material
prefix hashes through `biosphere_regression_scope.py`, then compares equipment
Ellie and every following byte exactly. The complete access file is still
unchanged. The broad callback gate remains unconditional; this exception cannot
admit an arbitrary material-prefix edit.

The final three-NPC material/reward tree was retested in the fresh
`../material-reward-equipswitch-final-20260906/` directory. All five scoped
production units, original pc.cpp and combined driver were freshly compiled;
pre-existing unrelated support objects remain an explicit linking boundary.
Fixed 103 / 8,097 and original 101 / 6,659 passed, with the exact separate old
index-200 sanitizer failure, clean normal-process allocator/sanitizer teardown,
seven diagnostic rejection controls and all newline controls. Receipt SHA256:
`b630b71906805ed0d474556cf1a8cc2f5d0e9503717e2157c0b4aa35c1690613`.
