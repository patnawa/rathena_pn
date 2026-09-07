# Episode 20/21 static quest-marker registration lifecycle

## Scope and runtime preservation

The authorized migration moves exactly 125 literal `questinfo` statements from
dialogue entry to each owner's `OnInit`, across 11 enabled custom Episode 20/21
files and 27 static maps. Each move inserts an explicit dialogue `end` before
the event label. The exact whole-file before/after identities and complete
inventory are pinned in `tools/ci/episode20_21_questinfo_migration.py` and recorded
in `doc/episode20_21_questinfo_registration_audit.md`.

The applied source passed `--phase after --self-test`: 44 newline-only positive
controls and 110 non-newline/shape refusals. Four additional LF/CRLF controls
accept the exact QI-only Family source and the exact complete transaction
overlay; five isolation controls reject partial/mutated overlays and direct use
of transaction bytes by the QI-only pair. Inverse reconstruction proves every
QI-migration non-registration byte unchanged. In particular, no Gudra or Mandel
transaction, quest, reward, travel, or daily reset behavior is owned by this
migration.
The preceding independent Ghost Ship clock repair remains preserved in the
before identity `58c336e56c135390cf47f98526f598433ec87f3384213ed6f3f4f46bba229965`.

## Native proof design

The independent `episode20_21_questinfo_native_test.py/.cpp` fixture executes the
actual NPC parser, native event registry and global `OnInit` dispatcher,
`questinfo`, `npc_click`, and `npc_unloadfile`. Its translation unit includes
the current `npc.cpp` to initialize the native private registries without a
server, SQL, unrelated controllers, or periodic world timers. The runner
compiles this unit, `script.cpp`, and `malloc.cpp` with ASan/UBSan, no recovery
or suppression. Retained support objects are bound to the exact accepted
document-test producer, each artifact's bytes, and all 1,645 engine/header
sources. The prior test driver is excluded; prior test logic and broad-gate
acceptance are not inherited.

Every fixture owner is its complete hash-pinned declaration, retaining its
original map, coordinates, name, view, and dialogue body. The original phase
is reconstructed by the exact reversible transformation and must match all
11 original whole-file SHA-256 identities; it is not a simulated broken body.
The fixture exercises two parse/startup/click/unload cycles per phase:

- Migrated startup registers one compiled condition for every owner, before
  player dialogue. Two bounded clicks per owner leave that count at one.
- Original startup registers none; the first and second clicks produce one
  and two conditions respectively. The map's owner-ID list still deduplicates.
- Native unload clears old map memberships, names, world IDs, and events.
  Reparse creates fresh IDs and the subsequent startup recreates one condition.
- Deliberately running candidate `OnInit` twice without unloading produces two
  conditions. This is a negative lifecycle control, **not** an idempotence claim.

The marker loop uses actual `pc_show_questinfo_reinit`, actual
`pc_show_questinfo`, and actual `achievement_check_condition`/script VM. All
125 original conditions run against a loaded empty-story character, with a
wrong-sized `qi_display` skip/reinitialization control. The 13 exact required
user functions include the transitive condition helpers and Horuru's actual
`EP20_SyncStep`, `EP20_EffectiveStep`, and `EP20_QuestInRange`. Clean loaded
persistent registry state is initialized through native load-mode writes;
the bounded prefix must preserve its values, update flags, and dirty bit.
The fixture also performs the native fake-NPC initialization needed by
`achievement_check_condition`; omitting it was rejected by UBSan during fixture
development, not suppressed or classified as a production defect.

## Explicit boundaries

World ID lookup, spatial registration/iteration, spawn/clear/marker notification
and dialogue transport are explicit doubles. Original coordinates are retained
but this test does not establish native map collision or client rendering.
Ten mob-backed NPC view identities (22006, 22301–22306, 22308, 22311, 22366)
are projected as exact Id/AegisName/Name triples from the pinned import DB and
parsed through native `MobDatabase::parseBodyNode`. These deliberately minimal
world records exercise the native view lookup without claiming full monster
stats, skills, drops, availability overrides, or mob-DB import-graph coverage.
The three constant files actually loaded by `do_init_script` are also pinned.
Each click is intentionally terminated at its exact first literal `mes` call,
before quest/reward/travel continuation. Horuru's preceding real synchronization
helper is executed only for the stated empty-story character, not every possible
quest history. No material or reputation transaction is exercised.

Conditions are parsed and run natively for one empty-story state, not exhaustive
truth tables or full live-map callback closure. Only these 125 selected owners
are loaded. Socket/connect/bind/listen are denied by an installed kernel seccomp
filter and tested explicitly. No SQL, packet transport, native client, server
connection, deployment, or live gameplay is part of this proof.

## Receipt

Final installed-source compilation and execution passed in
`../episode-qi-native-label-v2-20260907/`, with exact metadata-bound object reuse and
`family_overlay_phase: family-transaction`:

| Phase | Owners/maps | Cycles | Bounded clicks | Assertions | Native condition calls |
| --- | --- | --- | --- | --- | --- |
| Migrated source | 125/27 | 2 | 500 | 6,216 | 274 |
| Genuine original controls | 125/27 | 2 | 500 | 4,964 | 0 |

The 274 calls are 125 conditions plus a 12-owner display-repair control in each
of two cycles. Original controls intentionally do not evaluate marker conditions;
they prove original startup omission and click-driven accumulation through native
registration data. These are original behavior controls, not 4,964 reproduced
failures. Both processes returned zero, had zero assertion failures/native
errors, empty stderr, and exactly one clean allocator shutdown. ASan/UBSan were
enabled without recovery or suppression. All frozen source and artifact hashes
matched before and after execution. A final migration verification also passed.

The runner separately refused 16 incomplete/diagnostic/exit-status results and
seven altered source/header/object/archive/executable/fixture/producer inputs.
It also accepted four exact LF/CRLF QI-only/final-overlay forms and refused
three altered overlay forms before native execution.
It requires the exact native phase counts, not merely a success-looking line.

Receipt SHA-256:
`efbffc1de3831db33015f92ecc94975f0e919675f15a1128c997c98722e41700`.
Runner SHA-256:
`7a9ee5a4329bf4922b4bb8c71c0124c005865c09a73a267faffd8ef6f57fcaca`.
Driver SHA-256:
`6db66acdd566742e499d6cd1d9d5fa944c50b581ae68ade1a0df7d169e1b5c4f`.
Migration tool SHA-256:
`d9b9f1b5e8f724e4ac0f00244b733287ee688ae95fab1c083ace9e060e29b5e6`.
Self-contained Family overlay adapter SHA-256:
`33001c9b8ba48bc472018434aee4ac65d406b2c79e48689f7b1aa17f5f113ec1`.

The adapter owns all five transaction edit pairs and their full-file hashes.
`active_pair` strips only the exact final overlay to the exact `c720...` QI-only
file, then the unchanged public `pair` reconstructs the exact `d49...` source.
Thus an installed final Family source is accepted without adding its transaction
edits to the 125-registration migration or weakening unknown-source refusal.

Reproduction (Linux/WSL, fresh output directory, accepted retained producer
artifacts available):

```sh
python3 -B tools/ci/episode20_21_questinfo_native_test.py \
  --build-dir ../episode-qi-native-fresh \
  --retained-document-build ../biosphere-document-native-final-20260906
python3 -B tools/ci/episode20_21_questinfo_migration.py --phase after --self-test
```

This is a local native regression receipt, not a deployment or graphical live
gameplay claim. The QI-only transform remains exactly 625 added lines and 125
removed lines across the 11 authorized files. The current Family file also has
the independently reviewed transaction overlay at `3e6217fa...`; `active_pair`
removes that exact overlay before reconstructing the unchanged QI-only inverse.

## Live deployment addendum — 2026-09-07

The native receipt remains intentionally bounded, but all eleven transformed
Episode 20/21 files were later deployed together in the exact r7 release. The
isolated candidate and live map each loaded the expected POST inventory of
27,079 NPCs, 22,375 scripts, and 3,735 `OnInit` executions without an error or
fatal diagnostic. Exact deployed hashes, the validation/deployment receipts,
and rollback material are recorded in the
[deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
