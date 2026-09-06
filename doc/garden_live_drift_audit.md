# Preserved Garden of Time and self-destruction drift

Read-only review, 2026-09-06. No NPC, engine, database, client, server, or snapshot
was changed. This document is a source/engine-semantics audit, **not** a native VM,
combat, client visibility, or live-player test. The supplied snapshots establish
file contents, not which historical binary compiled them or unsnapshotted live
mapflags. Recommendations below are for a separately reviewed batch.

## Findings and preservation decision

Keep the live Garden file intact for the current deployment. Do not replace it
with the local stub, load both copies, or enable its legacy instance gates.
The enabled custom gates already suppress both legacy spelling variants.

1. The four entrance declarations have distinct unique names and coordinates;
   the legacy/custom pair for each dungeon is adjacent. There are also no
   duplicate NPC unique names across the three reviewed files. Their entrance
   policies and state are incompatible if the legacy gates are re-enabled.
2. The live Hall entrance contains a genuine destructive equipment
   reconstruction. It is dormant after the custom gate's startup timer disables
   it; the supplied source does not establish current gameplay reachability.
3. The live Garden file fixes two local omissions: inconsistent gate names and
   the missing fourth vending-machine reveal. Whole-file replacement would
   discard those fixes along with the user's legacy implementation.
4. The live self-destruction condition changes target selection on flagged maps,
   not whether the caster dies. The local enum survives, but there is no named
   export/setter or Garden usage in the inspected local source tree. Its live
   activation and actual deployed-binary history remain unproved.

## Entrances, ownership, and suppression

The existing import-graph reader (`biosphere_callback_closure_audit.npc_graph`)
found 900 enabled script files and exactly one include each for Garden,
LakeOfFire, and HallOfLife. The relevant includes are
`npc/re/scripts_athena.conf:229` and `npc/scripts_custom.conf:203,207`, reached
from `npc/re/scripts_main.conf`. An in-memory replacement of Garden by the live
snapshot found zero duplicate script/duplicate-declaration unique names across
these three files. This was declaration accounting, not a native NPC load test.
This does not assert that every NPC footprint is disjoint: for example, Hall's
start and post-clear reward NPCs intentionally share 54,55 in `1@ba_go`.

| Route | Live unique name | Position in `t_garden` | Current owner |
| --- | --- | --- | --- |
| Legacy Lake | `Dimensional Prison#1` | 158,235 | Live Garden |
| Custom Lake | `Lake of Fire#pn_gate` | 159,235 | `LakeOfFire.txt:192` |
| Custom Hall | `Hall of Life#pn_hol` | 172,235 | `HallOfLife.txt:138` |
| Legacy Hall | `Dimensional Prison#2` | 173,235 | Live Garden |

The local stub instead declares `Dimension Prison#1/#2` at the same legacy
positions. Custom Lake lines 195-202 and Hall lines 268-275 run a one-second
startup timer and disable **both** `Dimension` and `Dimensional` variants when
present. No named `enablenpc`/`hideoffnpc` reopening of these gates was found in
the inspected NPC tree.

Native `npc_enable_target` (`src/map/npc.cpp:1036`) gives `disablenpc` both
`OPTION_HIDE` and `is_invisible=true`. Player-specific `cloakoffnpcself` changes
only the cloak presentation; it does not clear either disabling state.
`npc_click` rejects `OPTION_HIDE` (`npc.cpp:2225`), and the touch/event paths
also skip disabled NPCs. Therefore the Garden story's later uncloaking does not
undo custom suppression. Do not mistake a cloak-option packet for proof that
the legacy entry body becomes clickable.

The timer is not an immediate declaration-time exclusion. The legacy scripts
remain `script(CLOAKED)` before that timer and could also return after a
single-file manual reload that does not rerun the custom timer. This is a
maintenance/startup boundary, not a demonstrated player exploit. Loading both
Garden files is worse: most names collide, and `npc_parsename` renames duplicate
unique names (`npc.cpp:3703`); callbacks and suppression then need not address
the intended copy. There is no reason to load both.

### Why re-enabling legacy gates is not a harmless alternate entrance

`db/import/instance_db.yml:80-94` supplies only one definition per name: Lake
(201, `1@f_lake`, 23,192) and Hall (202, `1@ba_go`, 53,42), each 3,600 seconds.
Both entrance implementations create/enter those same custom-controller maps.

- Live Lake (snapshot lines 983-1038) sets quest 12611 and clears 12613-12616,
  but never writes the custom roster reservation or `PNLake_ActiveInstance` /
  `PNLake_ActiveToken`. Custom creation writes the instance run token and
  per-character reservations; custom entry writes active participation and the
  selected boss quest (`LakeOfFire.txt:63-92,176-183`). Both the start seal
  (212) and reward exit (354) require active instance/token equality. A
  legacy-created/entered run does not satisfy that contract; a generated
  controller `run_token` alone does not register players. The legacy quest
  timer also is not the custom `PNLake_NextEntry` midnight policy.
- Live Hall (snapshot lines 1042-1148) charges a key on **each character's
  successful entry**. Custom Hall charges one leader key on successful
  reservation and writes instance `'party_id` / `'leader_aid` (255-263).
  The custom start seal requires the instance party and leader (287).
  Legacy creation does not initialize that ownership. Custom entry also
  handles protection synchronization and account pending/reward state
  (142-223), none of which is initialized by the legacy entry body.
- Custom Hall completion requires clear state, a start-registered account,
  valid synchronized protection, no prior run decision, and no weekly lock
  (820-858). Lake's exit has separate run/account claim tokens (346-394).
  Merely entering the shared map or setting old quest IDs is not equivalent
  to these reward predicates. This audit does not allege a proven free-reward
  bypass.

The effective Hall quest import matters: `db/quest_db.yml:66` loads
`hall_of_life_quest_db.yml`, overriding 12619 to **one minute** and 12612 to
**Friday 09:45**. The live legacy dialogue still says one hour; the upstream
base's one-hour/Monday values are not the effective Hall policy. Quest 12617
is untimed. Do not migrate account state by inferring failure or weekly
eligibility from that quest alone.

## Legacy barrier reconstruction and metadata

In the live Hall gate, unfinished quest 12617 triggers a failure branch even
before instance ownership is checked (1085-1120). It validates equipped item
420231 and fourth-card range 312453-312472, then erases the quest. Above level
one it executes:

```text
delequip EQI_HEAD_LOW;
getitem2 .@equip_id, 1, 1, 0, 0, 0, 0, 0, (.@id - 1);
equip .@equip_id;
```

This is not an in-place fourth-card change. Native `delequip`
(`script.cpp:10453`) unequips and deletes the selected inventory item.
`getitem2` (`script.cpp:7776`) starts from `item item_tmp = {}`; this call fixes
identify=1/refine=0/attribute=0 and cards 0-2=0, sets only card 3, and supplies
no random options, grade, bound type, expiration, or original UID. Those old
fields are not preserved. `pc_additem` generates a new UID for this unstackable
equipment (`pc.cpp:6062`). Other per-instance metadata such as favorite and
equipment-switch state is likewise not copied from the old record.

This does **not** remove item 420231's intrinsic database trade prohibitions;
clearing an instance bound field and changing database trade rules are different
things. Nor is every theoretically lost metadata field proved obtainable on
this item in ordinary play. The old item's identity is nevertheless deleted.
The replacement-add failure path has no restoration, and `equip <item ID>`
selects the first inventory match (`script.cpp:17699`), not an exact newly added
record if the player has another copy.

The same branch does not synchronize `#PNHoL_Level`, pending instance/level,
or the account reward lock. The custom controller instead changes card 3 via
`modifyequipitem` with expected item/card guards and retained host metadata
(`HallOfLife.txt:41-78`), and decides failure from pending instance lifecycle
and weekly state (81-103). A standalone replacement of just `getitem2` would
not reconcile those policy differences.

Both Garden variants also contain the existing sealing-stone *reissue* at
local 809-821/live 810-822: it creates a new level-one protection when the
inventory-minus-equipped predicate permits. It does not delete an existing
protection and is not the destructive branch above. Custom synchronization
can restore the existing account level on a valid equipped replacement. Do not
remove this acquisition route or guess a new reissue/level policy in a drift
merge.

## Live changes that a stub replacement would lose

- The completed-quest touch route already references **Dimensional** Prison
  in both files (local 689-690). The local declarations and registration
  scene use **Dimension**, so that completed route targets nonexistent names.
  The live rename makes both declarations and all scene references consistent.
  Removing the legacy declarations outright would again leave named story
  references dangling, even though the custom entrances are separate.
- Live line 1435 adds `cloakoffnpcself("Subspace vending machine#4")` to
  `#v_wp01`, also used by `#v_wp02/#v_wp03`. Machine #4 exists in both versions
  (local 1515/live 1655), starts cloaked, and requires completed quest 12620.
  It opens existing enchant groups 142/118/120/122/124. The local reveal list
  contains only machines 1-3 and the complaint terminal. Preserve the fourth
  reveal; this audit did not execute client visibility or those enchant UIs.
- The live-only changelog and full gate implementations are preserved user
  changes, not evidence of official behavior. No other behavioral diff was
  found in the line-ending-normalized comparison.

## `MF_MD_SELFDESTRUCTION` source drift

The suicide-bombing snapshot differs in one `if` expression. It adds
`(md != nullptr && map_getmapflag(src->m, MF_MD_SELFDESTRUCTION))` to the
existing summoned-sphere/non-versus exception. Let `E` mean `BCT_ENEMY`, plus
`BCT_SLAVE` when `alchemist_summon_setting & 8` is nonzero:

| Caster/map | Current local target mask | Live source with flag off | Live source with flag on |
| --- | --- | --- | --- |
| Non-mob, non-versus | E | E | E |
| Non-mob, versus | BCT_ALL | BCT_ALL | BCT_ALL |
| Marine sphere, non-versus | E | E | E |
| Marine sphere, versus | BCT_ALL | BCT_ALL | E |
| Other mob, either map type | BCT_ALL | BCT_ALL | E |

This is `NPC_SELFDESTRUCTION` (skill 173), not Mechanic `NC_SELFDESTRUCTION`.
`battle.hpp:60-73` defines these masks. `battle_check_target` treats ordinary
mobs as friends of other ordinary mobs and `BCT_ALL` as all otherwise
attackable characters/traps/icewall (`battle.cpp:8166-8196`). Thus the intended
inference is to stop ordinary exploding mobs indiscriminately hitting allies
on opted-in maps; the bit-8 slave exception also applies there. It is not an
instance lifecycle/destruction toggle, is not automatically true merely because
a map is instanced, and does not add immunity for players. Both versions retain
the target hiding check, temporary caster block removal, and final
`status_zap(src, sstatus->hp, 0, 0)` after successful re-addition.

Current local `map.hpp:703` retains enum value 81, but `script_constants.hpp`
does not export it. The complete local `src/npc/db/conf` named search found only
that enum. `map_getmapflag_by_name` resolves through script constants
(`map.cpp:4634`), so a normal named mapflag declaration is not supported by
that source snapshot. Numeric `setmapflag` or a runtime-computed value could
still set valid enum 81 (`script.cpp:14185`, `map.cpp:4731`); no direct literal
81 setter was found. This does not exhaust arbitrary dynamic expressions,
plugins, console commands, or unsnapshotted live scripts.

`initMapFlags` defaults the flag to zero. Instance copying copies the flag
vector unless `NoMapFlag` suppresses that copy (`map.cpp:2820,3816,5307`). No
Garden script enables the flag, and none of the inspected Garden boss/illusion
skill rows or direct custom-script calls casts skill 173. Hall's sanctuary
marker 20562 has no mob-skill row in the inspected text databases. There is no
concrete Garden interaction established here. Do not describe the current
candidate as preserving the live condition merely because the enum remains;
conversely, do not claim a proved combat regression without an activated flag.

## Smallest preservation-compatible next batch

1. Establish the preserved live Garden file as the reviewed source baseline,
   retaining its coherent names, story, reissue, and machine #4 reveal. Never
   load local and live variants together. Keep custom controllers as the only
   entry/reward authority; do not merge the old key/cooldown/failure policy into
   them.
2. If closing the startup/single-file-reload gap is desired, a narrowly reviewed
   change can make the two legacy declarations `script(DISABLED)` from load
   time, preserving names/coordinates and their dormant source bodies. Retain
   both alias suppression checks. No legacy functional rewrite or removal is
   required for normal custom entry. If an alternate legacy entry is actually
   wanted, first centralize/delegate to the custom entry functions; restoring
   only the barrier mutation is insufficient.
3. Keep the suicide-bombing snapshot in the preservation record and reconcile
   that single condition separately with the actual live mapflag exports and
   setters. Restoring the guarded condition with its existing enum would not
   alter flag-off behavior, but enabling a new exported flag or selecting maps
   is a separate gameplay decision; do not invent those opt-ins here.

Before implementation is called verified: execute both alias/declaration
variants through real NPC startup/reveal/click semantics, including single-file
reload; show exactly one operational entry per dungeon; cover custom-created
and legacy-created instances, participant tokens/party initialization, failed
entry without deductions, weekly locks, and same-ID multiple protection copies.
Any enabled barrier mutation needs field-exact metadata and native-refusal
tests before quest/account updates. A restored skill condition needs actual
target checks for flag on/off, versus/non-versus, sphere/normal/non-mob,
bit 8 on/off, caster death, and instance flag-copy behavior. None of those
runtime tests was run in this audit.

## SHA-256 evidence (raw file bytes)

Paths are relative to the repository except the supplied sibling snapshots.
The Garden files contain legacy non-UTF-8 bytes; the comparison preserved them
with surrogate escaping. Do not accidentally rewrite their encoding while
reconciling the reviewed line differences.

| File | SHA-256 |
| --- | --- |
| `npc/re/quests/garden_of_time.txt` | `2ad1160409c13f73326790995cabac7ab8359443f0e5f32ba9569aad5c56dd91` |
| `../biosphere-live-callback-drift-20260906/garden_of_time.txt` | `0505f6c6980642ef05c132652278ed42da06a294c977c00a4e44c2b94125431f` |
| `npc/custom/instances/HallOfLife.txt` | `8c08ddd788654e4671a943984e74fc360fd98993d297e2d6a0d0f59bb4059b5c` |
| `npc/custom/instances/LakeOfFire.txt` | `32d37c3e9edbf5450571d3c23948bbb9599ef39a189781827369d29b8c3a4889` |
| `src/map/skills/npc/suicidebombing.cpp` | `2a861f3e29080281d5d87d9015b64afcf7e863a360e8965c20c7f1fe21c91142` |
| `../biosphere-live-callback-drift-20260906/suicidebombing.cpp` | `80e5ebd92e5eec5e26eec2f3bfb62cb61f894cd8c94bbed097545b7ee7ec4164` |
| `src/map/map.hpp` | `cb7ada73689d650ac9a531077876763c73a1564de1c0bd6e19c073ad4fc26258` |
