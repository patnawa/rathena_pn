# Biosphere crown callback closure - 2026-09-06

`tools/ci/biosphere_callback_closure_audit.py` is the independent fail-closed
regression gate for the reviewed **current-content** payment-order proof.
It is not a script sandbox, arbitrary-script semantic analyzer, rollback
mechanism, or live persistence audit. It does not modify files or services.

## Reviewed conclusion and boundary

For valid inventory produced by the reviewed current content, the nineteen
supported Time Dimensions Rune Crowns can be mutated before charging the
already-checked rune cost. No callback in the reviewed closure consumes those
runes or changes BaseLevel, `ep17_2_main`, or either Biosphere reputation.
The caller must still recheck access, the originally captured item identity,
and material counts after dialogue and before mutation. It must charge on
native **mutation** success, including same-result rolls and failed re-equip.

This conclusion assumes no injected or historical arbitrary persisted
`bonus_script` text and no malformed/illegally compounded inventory. It does
not generalize to future scripts or another NPC operation. Root separately
owns live persistence and deployment-setting verification. The manifest
always says `live_persisted_text_verified: false`.

## Complete script-entry closure

The reviewed source path is:

```text
modifyequipitem (script.cpp:9968)
  pc_unequipitem(index, 3)
    clear equipment positions and equipment-dependent statuses
    pc_unequipitem_sub: remove affected autobonuses/combos
    status_calc_pc(SCO_FORCE)
    target base UnEquipScript; all four card/enchant UnEquipScripts
  revalidate the complete original item and committed weight
  mutate/log/notify the inventory record
  pc_equipitem
    eligibility checks; possible displaced-item cleanup; register combos
    status_calc_pc(SCO_NONE)
    target base EquipScript; all four card/enchant EquipScripts
  return mutation success even if re-equip failed
```

Both status calculations enter `status_calc_pc_sub` through
`status_calc_bl_` / `status_calc_pc_`. The synchronous script sources are:

- Equipment and pet active autobonus first bodies, restored by
  `pc_delautobonus` / `pet_delautobonus`.
- Regular `Script` on all equipped weapons, armor, shadow gear and
  non-throwable ammunition, not merely on the selected crown.
- Every active combo, every equipped card/enchant, and every random option.
- The item referenced by `SC_ITEMSCRIPT`, active status database scripts,
  persisted `pc_bonus_script` entries, and pet bonus scripts.

`SCO_FIRST` is absent: other equipment/card login-only EquipScripts do not
run here. The selected equipment's ordinary equip/unequip callbacks still
run explicitly. All four card-array entries are inspected, including
synthetic enchants. Combo registration/removal itself does not execute a
separate callback; its script runs during status calculation.

Current contents have no inventory mutators, nonlocal assignments, dynamic
function calls or yielding commands in that closure. `set` and `setarray`
matches operate on `.@` locals. Cards 4365 and 4541 drain SP on unequip;
they do not change items. Source-produced `SC_ITEMSCRIPT` buffs refer only
to bonus-only cards 4047 and 4121. Sixteen source-produced persisted bonus
bodies are bonus-only.

Some active autobonus first bodies heal or start/end statuses. These were
reviewed separately, not classified safe solely from a keyword search.
Refresh ends fourteen current statuses with neither OnTouch nor UnitMove
flags. Only Heat Barrel has the universal RemoveOnUnequip flag; it has no
payment/access event hook. Regular weapon-property enchant status changes,
Endure, Rebound and other reviewed first-body status effects likewise have
no rune/reputation mutation. Autobonus `other_script` is an attack/activation
callback, not the restoration callback executed by status recalculation.

Nested script completion restores the parent NPC state. `status_calc_pc_`
also reattaches that state. The reviewed closure contains no dialogue yield,
detach or sleep. Normal NPC event dequeue schedules a later 100-ms event;
it does not interleave a queued event between the synchronous mutation and
the caller's next charge. Generic status-end code can invoke touch/movement
hooks, so changes to those flags or reachable status paths require review.

## What the executable gate pins

All source bytes except newline form are preserved. CRLF and CR become LF;
legacy non-UTF-8 NPC text is retained losslessly with surrogateescape.
Comments and whitespace otherwise remain significant.

| Component | Reviewed traversal |
| --- | ---: |
| `.cpp`, `.hpp`, `.h` files under `src/map`, `src/common`, `src/config` | 2,739 |
| Selected database files across five Renewal roots | 35 |
| Ordered item / combo / random-option / status / pet records | 29,607 / 5,168 / 249 / 1,060 / 107 |
| Effective item IDs / equipment-card-ammo IDs | 29,583 / 18,185 |
| Effective item script fields of all types | 20,651 |
| Equipment/card/ammo script fields | 14,150 |
| Combo / option / status / pet script fields | 5,168 / 249 / 189 / 95 |
| Enabled NPC include files / NPC script files | 14 / 900 |
| Literal active-bonus first bodies, equipment/combo/pet combined | 1,068 |
| Literal source-produced persisted bonus bodies | 16 |

Database roots are item, combo, random-option, status and pet databases.
The strict traversal rejects missing selected files, cycles, out-of-root
paths, wrong database types and unsupported include syntax. It records
ordered imports, including skipped Prerenewal declarations. Complete raw
file hashes and complete ordered parsed-record hashes protect all effects,
flags, eligibility and membership semantics, not only script strings.

The scalar Script overlay view is explicitly not a general native nested-DB
merge emulator. Full ordered records and source parsers are pinned as well.
Literal-body extraction provides traversal counts only. **The hash equality
to reviewed content carries the proof; a regex does not prove new code safe.**

The entire enabled Renewal NPC tree is included to catch new persisted-bonus
providers without trusting keyword recognition. Tests and documentation are
outside the hash scope. Deployment configuration, runtime CLI overrides and
live stored scripts are checked separately by root, not inferred here.

Initial crown-batch pins (historical; superseded by the re-review below):

- Engine: `8d8b7450ba5c8a1d3850a8bf153ba74bf20936fa366124f9c13e02c50db702e9`
- Databases: `393c5f74fd82616b74a5667818cb4e5443d683a9ce693af1f3fd8d03f22df331`
- NPCs: `1c47017cafe784109cf87b762bb8e5d236bd7627cff47054b05b08a3915ccc1a`
- Script closure: `79513f7035bb459077a3eeae053c0db1f1df5c49c758abbfeab76a513a7f228c`

There is no accept/update/rebaseline flag. Any mismatch requires human
review before constants are changed or an explicit reviewed profile is added.
The frozen crown NPC included in this
review has raw SHA256
`790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df`.

## Runner and host usage

Runner API (raises `ValueError` on failure):

```python
from biosphere_callback_closure_audit import validate
manifest = validate(ROOT)
```

Standalone full validation, with PyYAML:

```text
python3 tools/ci/biosphere_callback_closure_audit.py --root /path/to/server
python3 tools/ci/biosphere_callback_closure_audit.py --root /path/to/server --manifest
python3 tools/ci/biosphere_callback_closure_audit.py --root /path/to/server --negative-controls
```

`--manifest` emits the validated JSON, including every exact relative path and
normalized hash. It does not write a file. A separately saved manifest can
also be checked on a host without PyYAML:

```text
python3 biosphere_callback_closure_audit.py --root /actual/server/root --verify-manifest /path/to/reviewed-manifest.json
```

The stdlib host mode first recomputes the section pins from the supplied
manifest, rejecting tampering, then checks every referenced file and current
engine-file membership. It cannot be made to accept a changed manifest by
editing a declared checksum. The gate is standalone and imports no project
test helpers. `collect()` is evidence collection, not acceptance; callers
must use `validate()` or the validating CLI.

Negative controls use in-memory source overrides only. They inject executable
item/card/combo/option/status/pet and persisted-bonus changes, both base
callback fields, an enabled NPC provider, changed engine bytes, a missing
database, added NPC and Renewal database imports, an import cycle, an escaping
path and a tampered host manifest. No checkout, database or service is changed.

Verification on 2026-09-06: `validate(ROOT)` passed, all **17** deliberate
negative controls were rejected, and the stdlib host verifier matched all
**3,688** files while imports of `yaml` were explicitly forbidden. The
original pre-profile standalone sentinel raw SHA256 was
`20450b03e90d1bdadab60e05157340202832058ac5d315049a45c1a03a3053b7`.

## Initial preserved-live profile (historical crown deployment)

`live-20260906` is an additional, explicitly chosen review, not a fallback
after default validation fails. The default `PINS`, `validate(ROOT)` API and
returned manifest are unchanged. The SHA256 of that default manifest serialized
with `json.dumps(manifest, sort_keys=True)` remains
`1ca5aaad95dc00f6b5e8da9c0680adb228482ee3487ae54ad5bab6f0bd6e9c60`.

Root supplied full read-only snapshots in the sibling
`biosphere-live-callback-drift-20260906` directory on 2026-09-06. The four files
were read completely and compared with the default source. They are existing
live changes to preserve, not changes made by this task. Their raw SHA256 and
newline-normalized SHA256 coincide:

| Actual server path | Required SHA256 |
| --- | --- |
| `db/import/item_db.yml` | `20946df000e0fba0710843bfd676dc895818e96fac6a0f3e2296e04cb39a287b` |
| `db/import/pet_db.yml` | `8c70d5f5b70b905082f3431fc0990ea76c736fb3c4b18713e899764d5297c00a` |
| `npc/re/quests/garden_of_time.txt` | `0505f6c6980642ef05c132652278ed42da06a294c977c00a4e44c2b94125431f` |
| `src/map/skills/npc/suicidebombing.cpp` | `80e5ebd92e5eec5e26eec2f3bfb62cb61f894cd8c94bbed097545b7ee7ec4164` |

The exact four-file review found:

- The item overlay contains nine existing episode tickets: 1000282, 1000284,
  1000285, 1000286, 1000287, 1000288, 1000988, 1001214 and 1001215. They become
  `Usable`, with zero weight and supplied display names. Ordered effective
  inheritance was checked, not inferred from absent fields: these records
  retain the base trade flags and have no regular/equip/unequip scripts before
  or after the override. No later selected overlay changes them. This review
  does not certify the ticket names or functionality as official behavior.
- `EP21_ICESEAHORSE` is a new active pet record; its upstream example is
  commented out, so there is no inherited SupportScript. Its single executable
  script reads intimacy into a local and grants Res/MRes +5 at cordial or +10
  at loyal. It has no inventory/registry mutation, yield or dynamic producer.
  The native parser's missing-script initialization/inheritance was checked.
- Garden's full 1,735 lines include ordinary quest, reward, storage and entry
  actions. Its changed sections rename prison references, implement Lake of
  Fire/Hall of Life entry, replace a lower-head blessing on a failed challenge,
  and expose a fourth vending NPC. Those paths contain inventory operations
  and dialogue but require separate NPC interaction; they are not callbacks
  of crown stat recalculation. The only global function, unchanged `F_mesnavi`,
  returns navigation markup. There is no bonus-script/autobonus/SC_ITEMSCRIPT
  producer or Biosphere access/reputation write. The existing custom instance
  gate timers also disable both legacy prison-name variants. General Garden
  correctness is outside this crown transaction review.
- Suicide Bombing changes only the target-selection condition for monster
  self-destruction on `MF_MD_SELFDESTRUCTION` maps. This combat cast path is not
  reached by the reviewed crown callbacks; no new synchronous script entry or
  producer is added. It remains a preserved **unbuilt source delta**.

All 146 effective item records required by the crown native fixture are
identical across the two profiles, including all nineteen crowns, payment
materials, 100 jewel levels, twenty stat enchants and the physical-card and
unsupported-crown controls. Their sorted-record canonical SHA256 is
`66d5b7cc5af6ebe31a5c54808d917cbf381eb3f7e450025dd114b53b6431b8f2`.
The four-file overlay cannot change the separate reputation import tree:
reviewed IDs 6 and 9 remain `RepPoints6`/`RepPoints9`, bounds -5000..5000,
visibility Exist, in `db/import/reputation.yml`. Root separately verifies
actual live files/settings; the five-database sentinel does not pretend to
pin reputation database files or live values.

All 3,688 file memberships and import graphs are unchanged; exactly these
four normalized file hashes differ, with the other 3,684 identical to default.
The live profile has 29,616 ordered item records and 108 pet records, with 96
pet script fields. All other traversal counts are unchanged. The only script
closure change is the additional bonus-only pet script; all item script,
combo, option, status, dynamic-producer and literal-body hashes are identical.
The live profile independently pins every complete section:

- Engine: `babc23082555cd292e24764ef3571e374b80297d83ef4934771346c8aa354293`
- Databases: `a9a970c11e8c7e30bc3be916237b9e106a6358906c4ad3e537b970a2485a1088`
- NPCs: `f405662667a62c1fe3baecae2eea59de64af2f2a8f3df9875c33fa65f207e32e`
- Closure: `b516348011c7faab98fc5023cb3dd453f5ff67e767a72c37410bb9d8599c0d86`

Explicit API: `validate(ROOT, profile="live-20260906")`. Host commands:

```text
python3 biosphere_callback_closure_audit.py --root /actual/server/root --profile live-20260906 --manifest
python3 biosphere_callback_closure_audit.py --root /actual/server/root --profile live-20260906 --negative-controls
python3 biosphere_callback_closure_audit.py --root /actual/server/root --profile live-20260906 --verify-manifest /path/to/reviewed-live-manifest.json
```

The normal live-profile validation re-parses every selected database and
recomputes the closure. The stdlib verification mode requires the same explicit
profile, validates its full section pins, then compares the actual tree.
There is no profile selection inferred from the manifest or a checksum error.
An expected-after-state manifest collected with the reviewed crown NPC as an
in-memory override is only a deployment plan; it does not prove the old live
NPC has already changed. Actual post-deployment verification is separate.

Source equivalence is not binary provenance. Root reports the current map
binary was built from candidate source at `2c796`, without the preserved
Suicide Bombing delta. This sentinel neither hashes that binary nor claims it
was built from the live source tree. Root must separately pin the deployed
binary/build receipt, verify Renewal/database settings, and verify persisted
`bonus_script` text with services stopped. The profile never changes
`live_persisted_text_verified: false`.

Final profile verification on 2026-09-06: the unchanged default gate passed
all 17 rejection controls. The snapshot-backed live profile passed complete
collection and rejected all 22 controls (the original seventeen, a byte change
in each of the four required live files, and attempted default acceptance of
the live manifest). Four additional independent checks omitted one reviewed
live file at a time and were rejected. Both profile crossover directions and
an unknown profile were rejected. All injections were in-memory and wrote no
runtime files. Final standalone sentinel raw SHA256:
`2387012e979e92283affbcdda6c9f1b3782c9f698e8f13f4f321ddeaca8c2ee8`.

## Conversion and Garden manual re-review, 2026-09-06

The earlier receipt and hashes above describe the completed crown deployment;
they are not claims about the newly changed source tree. Root re-read the
complete authorized runtime diff before changing the gate constants:

- `src/map/script.cpp` only adds/registers the read-only per-character
  `getinventoryslots` getter (raw SHA256
  `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1`).
  It returns the native field and does not invoke scripts or change state.
- The Biosphere NPC changes only `L_Convert`; every byte of the deployed
  crown/helper/access prefix is preserved. Current raw SHA256 is
  `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28`.
  Five recipes are unchanged; complete post-input checks precede bounded
  material debits and output. Its separate achievement/QuestInfo/weight-status
  closure is reviewed and pinned by `biosphere_conversion_callback_audit.py`.
  No dynamic bonus producer or crown status callback is added.
- Garden now incorporates the previously reviewed live source, preserving all
  bytes except the two active legacy declarations: `CLOAKED` to `DISABLED`.
  Raw SHA256 is
  `3e5a98758e70be050a0b61b3d23e3d0a3dae1c45e3729cc1988dd2c60fa0f301`.
  Actual native parser/click/reload tests separately cover these flags. The
  two legacy bodies remain disabled; custom entry controllers are unchanged.
  Legacy non-UTF-8 bytes were preserved without transcoding.

There are now exactly three preserved live differences: the same item overlay,
pet overlay and **unbuilt** Suicide Bombing source described above. Garden is
identical in both new source profiles; it is no longer an exceptional profile
file. No database, graph membership, dynamic producer or closure pin changed.
Both profiles still cover 3,688 unique files, with 3,685 identical across them.

Current project pins:

- Engine: `85a78b70ec48fd28b24c64418f1d715361ad0014f56d9d47ac852b2dfb48dce9`
- Databases: `393c5f74fd82616b74a5667818cb4e5443d683a9ce693af1f3fd8d03f22df331`
- NPCs: `dd76ad03a2fc1a98ac03b9c1085b6e4cae779ca8bb1fa5d49e18d572d62af23e`
- Closure: `79513f7035bb459077a3eeae053c0db1f1df5c49c758abbfeab76a513a7f228c`

Current explicit `live-20260906` pins:

- Engine: `434e6e4aa4274bad80e1e6f125f65750bfa0e13824f2cef14cf3b7bfc5c96c02`
- Databases: `a9a970c11e8c7e30bc3be916237b9e106a6358906c4ad3e537b970a2485a1088`
- NPCs: `dd76ad03a2fc1a98ac03b9c1085b6e4cae779ca8bb1fa5d49e18d572d62af23e`
- Closure: `b516348011c7faab98fc5023cb3dd453f5ff67e767a72c37410bb9d8599c0d86`

The API, normalization, explicit-profile requirement and fail-closed behavior
are unchanged. Source pins do not certify runtime settings, persisted scripts,
running binaries, general transactions or end-to-end episode gameplay.

Fresh re-review checks passed: all 17 default negative controls and all 21
explicit-live controls were rejected (17 common, three exact preserved-file
mutations, and rejection of the live profile by the default gate). The separate
conversion gate rejected all nine scoped negative controls. The crown native
suite passed again against the new engine/NPC: 4,152 cases and 200,773 assertions,
including its genuine old-source failure, with clean ASan/UBSan and allocator
teardown. The changed `script.cpp` was freshly compiled; retained sanitizer
objects were reused only when their exact production/driver source hashes
matched. Generated results are not proof of untested end-to-end gameplay.

## Equipment-switch deletion manual re-review, 2026-09-06

The preceding deployment receipts remain historical. The only subsequent
runtime change is `src/map/pc.cpp`, raw SHA256
`1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403`.
Root and an independent reviewer inspected both changes in `pc_delitem`:

- Reject `n >= MAX_INVENTORY` before the first array access, alongside the
  existing negative-index check. Rejection has no logging, inventory, weight,
  cache, status or packet side effects.
- On full depletion only, call the existing `pc_equipswitch_remove` before
  ordinary unequip and before zeroing the old item. Its existing nonzero-mask
  guard, all-EQI matching-index cleanup, one removal notification and final
  mask clear contain no script entry, callback, yield or status recalculation.
  The notification uses the old mask; partial deletion and ordinary items
  without a switch registration retain their prior behavior. The type flags
  suppress their documented deletion/weight/status operations, not all packet
  notifications. The existing ordinary unequip path is otherwise unchanged.

The fix removes a consumed item's live registration. It intentionally does not
reconstruct already-corrupt caches whose corresponding item has no switch mask,
transfer consumed metadata to replacement gear, or alter exchange economy.
The native regression receipt separately distinguishes real engine/NPC calls
from player/world/transport doubles and does not claim client gameplay proof.

All 2,739 engine file memberships were compared with the prior project and
explicit-live manifests; only `src/map/pc.cpp` changed in each. Current engine
section pins are project
`213aa0d386ab10a268a666095ce2aa2ab9355da8ab44aded401fe61c9f220c2f`
and explicit `live-20260906`
`01a5af6f37959bd526b25cdbe47e48fabef1cc7ace0950570a6b96e369ae925a`.
Database, NPC and closure pins, the three exact preserved live differences,
normalization, profile selection and fail-closed acceptance rules are unchanged.
No generic capacity-checking contract is modified by this change.

Re-review verification passed all 17 default, 21 explicit-live and nine scoped
conversion rejection controls. The dedicated equipment-switch native suite
passed fixed 103 / 8,097 and genuine old 101 / 6,659, with a separate exact old
out-of-range diagnostic. Crown 4,152 / 200,773 and conversion 225 / 49,497
regressions passed with freshly compiled patched `pc.cpp`. Both candidate
startup data variants and the installed explicit-live manifest/gates passed.
See `equipswitch_deletion_deployment_20260906.md` for actual binary provenance,
quiescence checks, backups, and the distinction from full gameplay validation.

## Material services and crystal capacity manual re-review, 2026-09-06

This review follows the completed equipment-switch deployment. It qualifies
source changes for the existing callback closure, not new transaction-test or
deployment completion. Root read the complete three-NPC-file runtime diff and
the native input, inventory, grant and callback implementations before updating
the NPC section pin. No engine or database changed:

- `npc/custom/varmundt_biosphere_quests.txt`, raw/LF SHA256
  `edec07ec166f5f8ae4a7e90ed52e38a5f9d76cf5781d139754a4ebd9a1ba0a7a`:
  Omega's two quantity paths check input status, cap the offered maximum and
  call a new purpose-scoped `F_BiosphereMaterialCommit`. It checks current
  access/location, all payment totals, conservative weight and native-compatible
  plain stacking, then pays in bounded chunks with no further yield. All 17
  recipes are unchanged. Equipment Ellie and all subsequent services are
  byte-identical after newline normalization; Research Beta is unchanged.
- `npc/custom/varmundt_biosphere_depth.txt`, raw/LF SHA256
  `6582a4b1401398f505b695f67e213e1d14f9b9978c875ae24b663623ce3b6988`:
  only Ellie fusion's input/max/payment block changes to that same helper.
  All 24 recipes and the exact level/story policy remain unchanged. Everything
  outside that complete NPC region, including crowns and Abyss `L_Convert`,
  is unchanged. No reputation threshold is added.
- `npc/custom/episode21/FinalBattle.txt`, raw/LF SHA256
  `3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1`:
  one new purpose-scoped, read-only `EP21_FB_CheckPlainBatch` snapshots inventory
  and reserves compatible stack/new-slot capacity cumulatively. Exactly two
  crystal capacity-check callsites change. Removing the helper and reversing
  those expressions reproduces the complete old file. Saved rolls, reward
  ordering/probabilities and daily/reset policy are unchanged.

Neither helper introduces a dynamic bonus provider, item/status script entry,
NPC relocation, quest-info registration or new NPC include. They run only from
the reviewed interactive services; their item operations are not new crown
status-recalculation callbacks. Existing ba_in01 QuestInfo and Final Battle's
Golden Diamond achievement completion require their own focused gates/native
fixtures, not a claim that this broad crown review proves those transactions.

Full project and explicit-live collections were compared with the preceding
deployment manifests. They have the same 3,688 memberships and import graphs;
exactly the three script hashes above differ. All engine, database and script
closure manifest sections are identical to the preceding versions. The common
new NPC section pin is
`45b9138a14adaaae85d96a30b9a45da3cf5808577144680f7cf663c3dd1183b7`.
The three separately preserved live differences and all other section pins are
unchanged. No automatic acceptance/rebaseline option has been added.

Older crown, conversion and switch regressions now use
`biosphere_regression_scope.py` for exact reviewed material-only exceptions.
The omitted fusion region and quests material prefix must match explicitly
pinned old/new bytes; arbitrary changes are rejected. Every remaining protected
byte, actual equipment Ellie/access body, and mandatory broad gate remains
checked. Four LF/CRLF form controls and six mutation controls passed. This is
not a blanket ignored prefix or a relaxation to keyword-only equivalence.

The initial Final Battle helper draft used a scripted output-by-inventory nested
loop. Root identified that up to 21 outputs by 200 physical rows could exceed
the actual 2,048 script jump limit. Before deployment, that draft was replaced
with native `inarray` aggregation/lookups and one ascending inventory pass.
Root also required a separate negative-lookup branch before array access,
without assuming logical short-circuit evaluation in this VM. Native `inarray`
is read-only and returns -1 for an empty/not-found list. The correction changes
no reward or callback policy; native budget/invalid-index cases are a separate
required test, not implied by the source pin. The draft was never deployed.

## Document exchange manual re-review, 2026-09-06

The next change is restricted to Depth Research Administrator's document-for-
reputation block. Root read the complete diff plus native input, reputation
getter/add/setter, registry and packet paths. The input status is now checked;
current map/original access, current reputation/ceiling and document count are
rechecked after the final suspension. The gain is computed from that fresh value
before the existing deletion and native reputation addition. The two-to-three
rate, 5000 cap, and final one/two-point pair are preserved. There is no new NPC
provider, database, engine, item/status script, QI registration or import.

The exact new Depth raw/LF SHA256 is
`3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326`.
Full project and explicit preserved-live expected-state collections were
compared to the preceding deployed material/reward manifests. Exactly that one
script hash changes; every other manifest field and all 3,688 file memberships
remain identical. Both reviewed NPC section pins are now
`9aeeaab191be38b0d36f07c0a55a14b7fae6894f48c8d4e4e5d1d3cbeff330a7`.
Other section pins, preserved-live differences and acceptance rules are unchanged.

This source re-review does not attest to a deployed document fix or to arbitrary
corrupt/unloaded character registries. The new document dependency/native proof
must cover the actual document, reputation/constant definitions, loaded in-range
registry path and ba_in01 QI callbacks. The subnet router was offline at this
initial re-review; expected-source collection was not live validation. Subsequent
recovery and actual deployment checks are recorded separately in
[the deployment receipt](biosphere_document_deployment_20260906.md).

Older regressions use an exact document-region inverse in
`biosphere_regression_scope.py`, rather than ignoring a changed prefix. Both
allowed old/new region hashes are checked; only the uniquely identified reviewed
edits are inverted, and the original region hash is checked again. The complete
old material baseline is still reconstructed identically. Crown/conversion
comparisons protect all remaining bytes. Four general LF/CRLF forms and six
mutation controls, plus two document forms and six document mutation controls,
passed. Behavioral regression results are separate evidence.
