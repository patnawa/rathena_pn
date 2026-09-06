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

Section pins (SHA256 of canonical JSON, including per-file hashes and graph):

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

## Explicit preserved-live profile

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
