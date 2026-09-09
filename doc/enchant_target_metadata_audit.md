# Existing-server enchant-target display metadata

Read-only source audit and narrow client fragment prepared on 2026-09-06;
the parent agent subsequently installed the reviewed package separately.
Scope is exactly the twelve already-defined server crown targets below.
No server definitions, native name mappings, enchant recipes, active client
files, or shared test/generator files were changed.

## Result and evidence boundary

All twelve original group-165 targets lacked records in the supplied client's
original merged SystemEN itemInfo table. Every one has an explicit server
`Slots: 1`, independently matching `slots = 1` in the user-supplied reference server
metadata. The numeric IDs already resolve through the original compiled
ItemDB_To_ItemID; no new identity alias is needed.

A narrow optional fragment supplies their complete server-derived descriptions
and display/slot fields. Fresh **native
Win32 Lua 5.1 execution** of the actual active itemInfo loader and original
F_itemInfoMerge proves the fragment reduces missing target metadata from
43 to 31 while deeply preserving every field of every previously present
metadata entry, including nested description arrays. A negative nested-field
mutation probe proves the preservation assertion is not just table identity.
It then exercises the actual EnchantList_f AddTargetItem slot checks for
these twelve names with the original group-165 slot order, 3. No missing
slot count is replaced by zero or inferred from an adjacent item.

At the isolated crown checkpoint, the remaining 31 missing targets were the
existing-server-identity gaps listed
in `doc/remaining_enchant_coverage_audit.md`; they are not implemented,
renumbered, assigned guessed slots, or hidden by this work. Group 165 itself
included ten such unresolved targets. Later Shadow166 changes are separately
verified by the parent agent and do not expand this twelve-record package.
Thus this fragment does not make
the complete original enchant registry client-validated or prove all native
windows work. The harness's strict missing-metadata stop is not a claim about
the protected game's undocumented C_GetSlotCount fallback.

## Reviewed identity, slot and resource mapping

| ID | Original native/server Aegis name | Card slots | Server View / client ClassNum | Identified resource |
| ---: | --- | ---: | ---: | --- |
| 401055 | `Stardust_Crown_SV` | 1 | 2500 | `Poenetentia_P_Crown` |
| 401056 | `Stardust_Crown_SC` | 1 | 2393 | `Poenetentia_B_Crown` |
| 401057 | `Stardust_Crown_VI` | 1 | 2506 | `Poenetentia_S_Crown` |
| 401058 | `Sky_Rune_Crown_IG` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401059 | `Sky_Rune_Crown_ABC` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401060 | `Sky_Rune_Crown_SH` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401115 | `Sky_Rune_Crown_MS` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401116 | `Sky_Rune_Crown_WH` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401117 | `Sky_Rune_Crown_HN` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401118 | `Sky_Rune_Crown_CD` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401119 | `Sky_Rune_Crown_IQ` | 1 | 2692 | `Midgard_Diadem_JP` |
| 401120 | `Sky_Rune_Crown_SKE` | 1 | 2692 | `Midgard_Diadem_JP` |

All server records are Armor / Head_Top with ArmorLevel 2. The three Stardust
crowns have minimum level 250, defenses 60/65/35 respectively, and Weight 400
(display weight 40). The nine Sky crowns have minimum level 240 and defense
50. The native item parser initializes absent Weight to zero, so their display
weight is explicitly 0, not a guessed value from another server. All twelve
are both Refineable and Gradable; those flags are checked against the server.

The fragment's identifiedDisplayName is the exact effective server Name,
including existing translation variations such as
`Rune Crown of the Sky (Spiritualist)`. reference server's corresponding label is
`Sky Rune Crown (Spirit Handler)`, but both sources agree on ID, slot count
and View. These translation differences are not identity conflicts. Several
server Name fields retain TODO translation comments; this work does not claim
the selected labels are final official Gravity translations.

Detailed skill, grade, refine and set-effect descriptions are clean-room
renderings of the effective server Scripts, not copied reference prose.
The package-local `effects.py` accepts only the reviewed Script vocabulary,
fails closed on unknown commands/expressions/conditions, and renders every
effective effect once. The manifest pins all twelve item Script hashes, all
fifteen combo Script hashes, fifteen partner identities/names/locations, and
the twenty referenced skill identities/display labels/max levels.

Basic displayed name, slots, View, defense, armor level, minimum level,
weight, job/class restrictions and refine/grade capability are checked
against the effective server. Every generated description line is compared
with the actual fragment; changing any descriptive scalar fails verification.
Source changes require an explicit reviewed manifest/description update.

The Stardust crowns have no job/class mask restriction and require level 250.
Eight Sky crowns select their named fourth-class job family at level 240.
401060 is different: `Jobs: Spirit_Handler` maps to the Summoner family and
its record has no `Classes: Fourth` filter. Its description states this actual
restriction rather than inventing a fourth-class-only rule.

## Effect semantics and bounded runtime proof

The actual server operations were audited in `src/map/pc.cpp`,
`src/map/battle.cpp`, `src/map/skill.cpp` and `src/map/script.cpp`:

- Integer refine divisions round down. Grade D/C/B/A and nested refine
  thresholds are cumulative, not mutually exclusive alternatives.
- `bMagicAtkEle` describes the attack's magic element; `bMagicAddEle` and
  `bAddEle` refer to the target's element. These are not conflated.
- The Stardust Grade-A `RC_All` bonuses have matching negative Player Human
  and Player Doram entries; they exclude those player races. The current
  `summoner_race: 11` configuration preserves the Player Doram distinction.
- Fixed cast/cooldown values are milliseconds; variable cast and after-cast
  delay values are percent. Perfect-hit add-rate is percentage points.
- Critical-damage bonus rates are halved for critical skills under this
  Renewal implementation. Non-critical bonuses skip critical attacks and
  skills explicitly flagged `IgnoreNonCritAtkBonus`; descriptions disclose it.
- Each `bAutoSpellOnSkill` rate 1000 is a 100% trigger chance, not guaranteed
  successful execution. The listed learned level, combined crown/right-hand
  weapon refine threshold, both Grade-A requirements, cast level and trigger
  are exact. Existing skill/target/equipment restrictions still apply. No
  unsupported promise of free SP, AP, HP or item costs is made.

Cardinal 401118 contains both `CD_ARBITRIUM` and `CD_ARBITRIUM_ATK` bonus keys.
`pc_bonus2` stores raw keys; `pc_skillatk_bonus` canonicalizes both queries to
the parent through `skill_dummy2skill_id`. Consequently the existing parent
entry already supplies exactly `5 * floor(refine / 4)` percent to both
queries. The redundant raw dummy entry must not be changed to the parent or
double-described. The renderer requires the matching effective parent entry
and renders one Arbitrium bonus.

The Hyper Novice / Celestial Napalm Sword combo previously stored only
`WL_CHAINLIGHTNING_ATK`, making its sum-based damage bonus unreachable by the
canonicalized lookup. The parent agent corrected that one database key to
`WL_CHAINLIGHTNING`; this package changes no server database itself. It pins
the corrected effective Script and rejects a return to the dummy-only key.
The existing Napalm +35% and autocast conditions were not altered.

Fresh isolated **actual rAthena parser/VM execution** passed 44,205 cases and
221,352 assertions with ASan/UBSan and no allocator leaks:

- Cardinal: all refines 0–20 and grades 0–4, 105 actual Script executions.
  Both parent/dummy queries receive the single expected bonus; Framen's
  Grade-C addition is checked independently.
- Hyper: both equipment refines 0–20, both grades 0–4, learned levels 4/5,
  and before/after source keys, 44,100 actual Script executions. This spans
  both sides of every gate, every refine split, sums 0–40, and the exact
  threshold 24. Before correction both Chain Lightning query forms return
  zero; afterward both return `2 * sum` only when sum ≥24 and both grades A.
  Napalm remains +35%; autocast identity, trigger, level 5, rate 1000 and
  learned-skill gate remain unchanged.

`run_native_bonus_vm_test.py` freshly compiled current `pc.cpp`, `skill.cpp`,
`script.cpp`, `clif.cpp`, `malloc.cpp`, and its package-local driver. The
actual script bonus builtins, `pc_bonus2`, bonus storage, alias mapping,
`pc_skillatk_bonus`, grade/refine builtins and learned-skill lookup run
unmocked. The six minimal skill identity/max-level fixtures are checked
against effective server data and parsed by the production SkillDatabase.
Player lookup, NPC/event/map-registry boundaries are explicit doubles;
kernel seccomp denies socket/connect/bind/listen. Unrelated existing local
objects satisfy linker dependencies, so this is not a complete fresh server
build or live combat/autocast execution proof.

Exact exercised Script SHA-256 values:

| Script | SHA-256 |
| --- | --- |
| Cardinal item | ab5a59fd4cbc7683106b49fa2a6ae85e88c45a8efdd94bb6dcacf9483f5cb94f |
| Hyper before key correction | e81151397ddcc8f68bdac108240398cbdafc0c73f19bea567abbd68e33a72cd2 |
| Hyper corrected effective combo | 6bebd23f2c26704398f193356f6df8b665be6b04b2c55e125fc2251713824465 |

## Supplied references and loader semantics

The effective server fields were read by recursive Renewal imports rooted at
`db/item_db.yml`, not by assuming that the first matching YAML block wins.
The twelve definitions are in `db/re/item_db_equip.yml`, beginning with
401055 around line 180257, with the remaining Sky crowns later in that section.
The verifier reconstructs effective records and compares the reviewed fields
by numeric ID before running any Lua.

The user-permitted reference directory is
`C:/Users/Alpha/Downloads/Compressed/ReferenceClient/System/`:

| Input | Purpose | SHA-256 |
| --- | --- | --- |
| itemInfo_reference.lua | Literal slots, names, View; selected records at lines 13318–13330 | a07836ff04fcc3525dc07f12c03139562ac237fdcc2f47547f1320f310c30fa4 |
| itemInfo_EN_db.lua | Explicit identifiedResourceName for all twelve IDs | 2834833d0438219b46e03224e19ebe94f0b018c725aa43dab7df94fe5ef4a4e6 |
| itemInfo_EN.lua | Actual reference server loader shows DATA.slots and DATA.view feed AddItem, resource comes from the main table | 379a308e3392c3cc385d8a862b9eb05fa4d2ffe3c9f7d7c68804b437decc501b |
| itemInfo_EN_db_fallback.lua | Inspected for scope only; effect descriptions are not copied | 235ea192329fba3be4eb9dec0ee76bf866a96efc26b2149a43bd84e47c5f0f7a |

reference server is a user-supplied compatibility reference, not asserted official
Gravity provenance. The current server's explicit slots are authoritative for
this project's actual equipment behavior, with the independent reference
providing corroboration. No decryption was used. A text scan of the supplied
RockMMO loose itemInfo files yielded no literal records for the selected probe
IDs; no RockMMO or compiled-file completeness claim is made.

The reviewed pre-crown client loads SystemEN's base itemInfo, custom,
ZeroCell, Chapter2, Fashion, DruidItems, Chapter2Materials and DruidGear
tables with the original override merge last. The verifier executes the
explicitly selected loader, so later checkpoints are not silently called active.
F_itemInfoMerge defaults to non-overwriting: it only creates a missing ID.
The verifier executes that actual function, rather than reimplementing its
merge rules, and checks every old metadata field deeply against a snapshot
after adding the new `tbl_enchanttargets` table.

Original client evidence is pinned in the optional package manifest:

| Input | SHA-256 |
| --- | --- |
| Active nebula EnchantList | 664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d |
| Original compiled ItemDBNameTbl | 2f4f35e157d25548f236dbe7785a3138f4c7ab23367becf9e1f95b958cc3c496 |
| Active new.grf EnchantList_f | bfee2e2ade0437fbb3a101e93e2e1a81986c3a484e4e96b9ec7fe10d52ac75cb |

## Graphics verification

The four identified resource names above are explicit reference server mappings.
Read-only indexing across the active GRFs in DATA.INI order verified each
has an item BMP, collection BMP, item SPR and item ACT: sixteen matching
entries, all from original data.grf, with no higher-priority override.

The reference's unidentified resource `uheadgear` was not found by the
restricted direct resource-name scan. This does not prove no alias exists.
The fragment instead explicitly uses the already verified generic
`EpisodClear20`, whose same four resource types also exist in data.grf.
Thus the verifier checks twenty effective graphic entries in total.
No graphic files were copied, replaced, generated, or downloaded.

These are verified existing client resources, not a claim that the reused
artwork is the original official art for these crown identities. In-game
worn appearance and protected-client rendering were not tested.

## Optional artifact and verification

The reviewed fragment is
`client-patch/enchant_target_metadata/SystemEN/itemInfo_EnchantTargets.lua`.
SHA-256:
`8ab7c0eafa30bf95918f24c2a799eb2d63ef18fe0a6884d2c61d21cd2bcf2772`.

`client-patch/enchant_target_metadata/manifest.json` contains the exact twelve
reviewed records and source hashes. `verify.py` fails on source/identity/slot/
View/resource/effect/description/scalar drift, a pre-existing candidate ID,
extra fragment identities, any changed old field, an altered missing-target
boundary, or missing assets.
The verifier never writes client or reference files.

Run from the repository root:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py
python3 -B client-patch/enchant_target_metadata/test_effects.py
python3 -B client-patch/enchant_target_metadata/run_native_bonus_vm_test.py
```

On Windows use `wsl -d Ubuntu --exec` before the command. The default runtime
is the previously verified matching native 32-bit Lua 5.1.5 executable under
`../chapter2-lua51-runtime-20260906/runtime/`; no bytecode-width conversion is
performed. Pass `--lua`, `--reference-system`, or `--grf-reader` only to select
explicit equivalent local inputs.

Ten focused regressions pass, including exact complete generated
fragment comparison, basic scalar drift, job/weight defaults, Cardinal
deduplication, rejection of the broken Hyper key, cumulative thresholds,
unsupported statements/conditions/amounts, race exclusions, autocast gates,
and rejection of zero-exit native allocator/sanitizer warnings or missing
clean teardown. The native runner requires both its completion marker and
the allocator's explicit `Memory manager: No memory leaks found.` message.

Candidate mode uses `--before-loader PATH`, defaulting to the selected current
loader. Installed mode requires an explicit backup, for example:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py --installed \
  --before-loader ../client-before-enchant-crowns-20260906/SystemEN/itemInfo.lua
```

It executes the real selected after-loader, requires exactly the new file/
table pair and no other loader source changes, verifies previous import order,
and deeply compares all prior fields with exactly twelve new records.
`--loader PATH` can explicitly select a retained after-crown checkpoint after
later unrelated installations. Output always includes `checked_loader`,
`before_loader`, `active_loader` and `loader_is_active`; a historical loader is
never silently reported as current active installation. Both loaders resolve
their unchanged loose dependencies under the real client working directory.

The parent agent installed the crowns and then separately installed eleven
Shadow166 records. This agent independently reran the retained crown-only
installed checkpoint successfully:

```sh
python3 -B client-patch/enchant_target_metadata/verify.py --installed \
  --before-loader ../client-before-enchant-crowns-20260906/SystemEN/itemInfo.lua \
  --loader ../client-before-druid-shadow166-20260906/SystemEN/itemInfo.lua
```

Result: PASS, `installed: true`, `loader_is_active: false`, exactly twelve new
records, all old fields deeply preserved, 43 →31 missing target metadata.
The strict crown-only check correctly rejects the expanded current loader's
additional Shadow import; this is not silently relaxed or reported as the
historical twelve-record state. Current-client combined coverage is the parent
agent's separate installation receipt.

| Loader checkpoint | SHA-256 |
| --- | --- |
| Before crowns | 6db791e0ea302b71a6cc13d068ef774f2d0c8cb885ceea96b3fd8626a5672811 |
| After crowns / before Shadow166, checked here | 6f26c390128be7b6620bf7727f85b47d0151af56db6eb8275578ad93511b7333 |
| Active after later Shadow166 at closeout | 5a3f33728795cc6278183688746b2e6efdd96ca1ced28364a7c55699611f1797 |

The final hardened native runner was freshly rebuilt and passed again from
`../crown-bonus-vm-proof-20260906/`, with 44,205 VM executions / 221,352
assertions, explicit clean allocator teardown and no warning/error/sanitizer
diagnostics. Artifacts remain outside the repository; no active client or
server deployment was performed by this sub-agent.

Verified candidate result: PASS; twelve targets added only in an ephemeral Lua
process; slot count 1 for all; missing metadata 43 → 31; all original fields preserved;
twelve scoped helper target checks; twenty verified effective graphic entries.
The separate VM effect proof is bounded above. No active installation by this
agent, complete group-165 registration, live enchanting purchase, charging,
combat effect application, worn rendering or reconnect test is claimed.
