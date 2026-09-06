# Biosphere material callback consistency gate

Date: 2026-09-06. This new auditor is implemented for the existing 41 Omega and
Depth-1 Ellie material recipes on `ba_in01`. It is a manually reviewed
game-content consistency check, **not a general script sandbox**, a native
transaction regression, or evidence of graphical gameplay or deployment.
The separate native transaction suite owns those behavioral tests.

The earlier source findings and unchanged recipe/access policy remain in
[the material service audit](biosphere_material_service_followup_audit.md) and
[the Omega/Ellie follow-up](biosphere_service_followup_audit.md). This gate does
not cover equipment Ellie's destructive equipment selection, or replace the
completed Abyss conversion/crown gates.

## Acceptance and source contract

`tools/ci/biosphere_material_callback_audit.py` exposes:

- `validate(root, profile=None)`: first calls the independent broad callback
  gate's `validate` unconditionally. It then traverses the scoped inputs,
  requires six explicit manual section pins, compares overlapping reads to the
  accepted broad manifest, rereads the additional quest/achievement trees, and
  verifies the broad manifest again. It returns
  `acceptance: REVIEWED_CONTENT_MATCH` only after all checks succeed.
- `collect_evidence(root)`: source inventory for preparing native fixtures.
  It does not run the broad gate or accept manual pins. Its manifest explicitly
  says `UNVALIDATED_COLLECTION`; it cannot be used as the safety gate.
- `negative_controls(root, profile=None)`: mandatory public validation before
  and after deliberate in-memory mutations. It never changes project files.

The CLI's `--collect-only` also emits an explicit unvalidated warning. It cannot
be combined with profile selection or acceptance controls. There is no automatic
acceptance/rebaseline option. A changed source, definition, registration, or
import graph requires renewed human review; matching a few regexes is not
permission to execute changed scripts.

`--profile live-20260906` forwards that exact explicit profile to the broad
validator. It never falls back to it after a project mismatch. A project-root
run is not a live-server check, and collection with the three known preserved
live-source overrides would describe an expected snapshot, not installed files.

The manifest always records `live_world_verified`, `deployed_binary_verified`,
and `graphical_client_verified` as false. Source matching does not attest to a
running binary, injected/historical scripts, runtime NPC placement, persistence,
or a player's actual inventory and quest state.

## Reviewed callback closure

Actual `pc_delitem` and `pc_additem` call `pc_show_questinfo`. On this map its
registry is not empty. The latter invokes the real nested
`achievement_check_condition` VM for each eligible condition. That function
detaches the current script and restores its RID/attachment after the nested
condition. Native `quest_check` is const: HAVEQUEST, PLAYTIME and HUNTING inspect
quest state/time/objectives without granting, completing, deleting, or changing
quests. `checkquest` and `isbegin_quest` call it; `countitem` only counts.

All 33 collected literal expressions call only those three builtins, together
with scalar story/BaseLevel reads and comparisons. Their `countitem` arguments
are 7110, 7326 and 1000226, none of the material identities. The pin covers the
entire body of each of the 19 owners, not just the literal strings: adding a
conditional around registration must also be rejected.

The source traversal records 126 `ba_in01` declarations in eight enabled files,
including 29 static duplicates of `dummy_npc`/`dummy_cloaked_npc`. Both actual
parent bodies only `end;`. The map includes ordinary warps and `warp2` entries;
these are not script-body registration owners. The 19 owners are sixteen in
`npc/re/quests/quests_17_2.txt` and three in
`npc/custom/episode19/quests_19.txt`, holding thirty and three conditions.

The gate also inventories all 1,681 registration-site lines across the enabled
source, all 33 `unitwarp` statements, the empty runtime-duplication/UNPC_MAPID
inventory, and the actual ordered enabled include graph. Previous manual source
review traced the unitwarp destinations to other explicit maps, the same map,
or instance maps; none adds a new NPC condition provider on `ba_in01`. This is
source evidence, not a query of the live world or a claim that a lexical scanner
can resolve arbitrary dynamic map names. The broad gate pins every enabled NPC
and engine source, including code around these inventory sites.

`pc_additem` also supplies the output's native sell value to Get_Item. The seven
selected achievement records 220023-220029 have literal conditions
`ARG0 >= 100/1000/5000/10000/50000/100000/150000`. All scoped output sell values
are zero, so none can complete. The ARG assignment/cleanup and nested VM still
run; do not replace them with no-ops in a native fixture. The twenty Goal_Achieve
conditions only compare AchievementLevel with 1 through 20. Rewards are a
separate claim flow. The actual selected achievement root imports Renewal
before `db/import/achievement_db.yml`; the Prerenewal branch is inventoried but
not loaded. All raw selected files, import order, and ordered records are pinned.

Weight notification calls `pc_updateweightstatus` through the actual SP_WEIGHT
packet branch. Weight50 and Weight90 have no Script, CalcFlags, OnTouch or
UnitMove hooks. They end each other on start; Weight90 additionally stops
attacking. Their full effective records are pinned. A fixture which stubs
`clif_updatestatus` must identify that boundary instead of claiming execution of
the native weight-status branch.

The current valid Etc input/output domain therefore has no reviewed synchronous
callback that changes payment, access, or output capacity or suspends payment.
This supports the separately implemented payment-then-output sequence after its
final exact preflight on `ba_in01`; it does not make arbitrary scripts atomic.

## Materials and quest fixture data

All 56 effective definitions are plain Etc, with no item scripts, locations,
per-item Stack override, generated-UID/autoequip flags, or nonzero Buy/Sell.
The only present flags are BuyingStore and/or DropEffect. Actual weights are
native database units, not displayed weight:

| Native weight | Exact identities | Count |
| ---: | --- | ---: |
| 1 | 1000636-1000639; 1001177, 1001179, 1001181; 1001290-1001305 | 23 |
| 10 | 1000640-1000643; 1001138-1001141; 1001178, 1001180, 1001182-1001186; 1001188-1001189; 1001306-1001321 | 33 |

`materials.records` contains all full effective rows. `materials.recipes`
contains the 41 independently inventoried existing recipes and debit order;
these are project custom costs, not a new assertion of official recipes. That
table is a native-test oracle, not a claim that this auditor parses every NPC
branch into recipes. The broader source gate and native transaction suite
check the actual helper/callers. The output/chunk limits remain 30,000, without
inventing smaller x5/x10 batch caps or bypassing the actual Zeny ceiling.

`questinfo.registrations` contains exact `path`, `line`, `npc`, `header`, `icon`,
`color`, `condition`, original `statement`, and per-owner `owner_order`.
`questinfo.owners` contains nineteen owner rows and their registration counts.
The inventory follows enabled source order and preserves registration order
inside each owner. It does not assert that the server's event-database traversal
executes different NPCs' OnInit events in that same global order. The native
fixture must preserve each owner's order and initialize the actual map/player
structures it uses.

`quests.referenced_ordered_records` contains 38 exact selected input rows;
`quests.referenced_records` contains 36 effective definitions. IDs 18119 and
18120 have existing title-only overrides. Those are the only repeated scoped
rows permitted, so no guessed nested quest-objective merge is used. A fixture
can feed all ordered rows to the real QuestDatabase parser. The complete ordered
selected quest import graph/files are pinned independently of the broad gate.

The HUNTING rows use actual monster-name bindings; quest 18024 additionally has
drop references to `G_EP17_2_HEART_HUNTER`, `G_BELLARE3`, and `Broken_Dollcore`.
There are no scoped Targets.Map fields. A native fixture that seeds minimal
monster ID/name entries for lookup must read those bindings from the real
effective mob database and record that world-data boundary; this gate does not
initialize or certify full monster/combat behavior.

Legacy non-UTF-8 NPC display bytes are preserved with UTF-8 surrogateescape.
JSON uses escaped representation; a fixture must not silently replace those
bytes while claiming an exact original display name.

## Required native checks and independent boundaries

The separate native transaction fixture is responsible for actual input,
inventory insertion/deletion, Zeny, capacity, cancellation, stale access and
resource changes, amount boundaries, metadata, and original failing controls.
This auditor does not substitute presence checks for that behavior.

For callback coverage, register all 33 original statements through the native
questinfo builtin on nineteen fixture NPCs. Ensure map.qi_npc is nonempty and
the player qi_display count exactly matches it; otherwise pc_show_questinfo
returns early. Use actual pc_show_questinfo/reinit and nested VM execution.
Per-owner first-true short-circuit means 33 registrations do not imply 33
executions on every debit. Probe both true/false results across source-derived
quest scenarios and assert main-script locals, RID, resources, inventory and
quest state are restored/unchanged as appropriate. Test wrong display size as
a coverage-negative control. Network, world lookup, and packet doubles must be
listed explicitly; no graphical-client claim follows from these fixtures.

## Reproduction and receipt

From the repository in WSL (Python 3 and PyYAML):

```sh
python3 -B tools/ci/biosphere_material_callback_audit.py
python3 -B tools/ci/biosphere_material_callback_audit.py --negative-controls
python3 -B tools/ci/biosphere_material_callback_audit.py --manifest
```

For an explicitly selected preserved-live tree, add
`--root /path/to/reviewed/tree --profile live-20260906`. This document does not
claim that a live-tree invocation has run. Use `--collect-only` only to inspect
unvalidated evidence while another authorized patch has invalidated broad pins.

An initial attempt properly rejected independently authorized FinalBattle source
drift at a broad check; that run is not a passing acceptance receipt. Following
the final helper review, the scoped inventory changed by exactly one source
location: the existing FinalBattle registration moved from line 75 to 77.
Changing only that integer in the new collected section reconstructs its prior
hash exactly. The literal statement, all ba_in01 members, and the other five
section pins remained unchanged. Only the npc_scope pin was manually updated.

Final public default validation and all **29 in-memory rejection controls
passed** against the reviewed FinalBattle raw SHA256
`3873d72118f6cf83374c445e6891eaf9a79660fec71c5e12b3bb02872e4625f1`
and broad NPC-section pin
`45b9138a14adaaae85d96a30b9a45da3cf5808577144680f7cf663c3dd1183b7`.
The controls reject native callback removal; both weight-class changes;
material script/GUID/sell-value changes; weight-status scripts; removed,
assigned, skipped, nonliteral or reordered QuestInfo; changed duplicate parents;
new map relocations/NPCs; changed Get_Item/Goal_Achieve/objective data;
missing/added/cyclic/escaping/wrong-mode imports; altered evidence/schema; broad
gate bypass in default/live-profile calls; and unknown profiles. Broad acceptance
is required before and after the negative suite, not mocked for successful runs.

The exact graph contains 361 selected achievement rows/effective records, 5,750
ordered quest rows, 900 enabled NPC scripts and fourteen include configurations.
The reviewed gate Python SHA256 is
`bf5ca66e5ca0545842f31af2ef6faa82a830d56503526ab42c11fbeb77324730`.
Fresh default public manifest canonical SHA256:
`1fd322f08eeaaa9adc54fe4390c649e9c0c6dee2c143a348cd22e61644358f14`.
Its accepted broad manifest canonical SHA256:
`0b0b24dd976ab83d66e58b4aedfa4f155e1c07d4dbd783892f148984d9041b5c`.
A separate CLI check confirmed collect-only emits the unvalidated classification
and no PASS text, and rejects combining collection with a live profile.

An additional read-only expected-state collection used only the three known
preserved-live files from `../biosphere-live-callback-drift-20260906/` as in-memory
overrides. The broad named-profile manifest and all six scoped section pins
matched. Expected broad canonical SHA256:
`f3b296241b9c5a5a08bc92bb0b3f85af9f1b9d0cf01714112661cb1b16fcc6a3`.
Unvalidated scoped collection canonical SHA256:
`d961507f034851436bfc385f6c911000abc6307e1adeed71578d32c9009108ab`.
That deliberately did not call public live validation against an installed
tree, and is not described as live deployment verification.

No existing gate, engine, NPC, database, client, or remote file was changed by
this auditor's implementation.

## Document-exchange source-location follow-up

Root subsequently reviewed the separate administrator repair in the Depth file,
raw/LF SHA256 `3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326`.
Its only effect on this material/QI inventory is two declaration line numbers:
Ellie moves from 235 to 253, and Chirp from 274 to 292. Reversing exactly those
two integers reconstructs the complete previous npc_scope section. All other
five section hashes are identical, including all 33 conditions and 19 owners.
The manually reviewed npc_scope pin is now
`82946f4b836ae9dad3e3745fe8c2a19efe5509463b1b650f150ebdd62981ed95`.
No collection/validation/negative-control acceptance logic changed.

This gate still does not include document 1001289 or its reputation database
binding. The new document-specific gate adds those dependencies; matching this
material gate alone cannot certify the reputation exchange.
