# Biosphere document exchange: registry and callback source review

Date: 2026-09-06. Independent read-only review of the frozen Depth Research
Administrator candidate. This document is evidence, not a new runtime gate,
native-test receipt, deployed-state attestation, or repair of other exchanges.

## Conclusion and scope

No new payment/credit loss was identified in the candidate for a normally loaded
character, valid inventory/cache, and raw `RepPoints6` already within its actual
database range **-5000 through 5000**. The post-input map/access/resource and
reputation checks close the stale-dialogue and redundant-pair defects while
retaining the last partial-credit pair. The conclusion depends on the reviewed
current `ba_in01` callbacks, not on a claim that `delitem` is callback-free or that
a failed reputation builtin rolls back payment.

The independent inspection did not modify runtime, engine, gates, or tests and
did not inspect live SQL or execute remote commands. Combat's native fixtures and
root's deployment/persisted-state evidence are separate receipts.

## Frozen source identity

Raw SHA-256, independently re-read at the end of this review:

| File | SHA-256 |
| --- | --- |
| `npc/custom/varmundt_biosphere_depth.txt` | `3b94a4467227b39eefb83ed8ce09314582b8ac9d610ba071ca2f8e385bc6f326` |
| `src/map/pc.cpp` | `1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403` |
| `src/map/script.cpp` | `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1` |
| `src/map/clif.cpp` | `ef8a28c1fd61c4b473301bf2a9cf7a4f7a1831c3fe0fd00c4928034175cf1626` |
| `src/map/intif.cpp` | `b0626e5503b578d45f3ebbaba0b15ca0043c07c6be4eed2ba310baa6c3bf61dd` |

These are source identities, not proof that a live binary was compiled from
them. Existing broad/scoped gates carry additional source, database, and enabled
NPC dependencies; their collectors were used here only for read-only evidence.

## Final preflight and payment interval

The Administrator block at `varmundt_biosphere_depth.txt:202` now performs, after
the last `input` suspension:

1. Successful input status, current map `ba_in01`, and the original
   `F_BioDepthQuestAccess` check (BaseLevel >= 250 and `ep17_2_main >= 33`).
2. A fresh reputation read and validation of the selected number of pairs
   `q` against `1 <= q <= ceil((5000 - rep) / 3)`.
3. A fresh `countitem(1001289) >= 2*q` check.
4. `gain = min(3*q, 5000-rep)`, followed immediately by `delitem 1001289,2*q`
   and `add_reputation_points REPUTATION_BIOSPHERE_DEPTH1,gain`.

There is no dialogue suspension between these checks and the two mutations.
Fresh reputation at 5000 rejects every positive order. At 4999/4998, the final
pair still costs two documents for one/two points, respectively; extra pairs are
rejected instead of silently consumed. Valid negative reputation is not
malformed: the -5000 boundary permits 3334 pairs / 6668 documents and a final
10000-point gain, all within the native amount/integer limits used here.

## Loaded-registry admission and the actual setter

Normal player dialogue admission establishes the registry precondition:

```text
intif_parse_Registers: receive character + local/global account registries
  -> pc_reg_received: vars_ok = true, then state.active = 1
  -> inventory/status loading completes: state.pc_loaded = true
  -> clif_parse_LoadEndAck: rejects incomplete loading, then map_addblock
  -> clif_parse: rejects ordinary packets while sd->prev is null
  -> NPC click/input handlers and the Administrator dialogue
```

Relevant source anchors are `intif.cpp:1481`, `pc.cpp:2354`, `pc.cpp:2429`,
`pc.cpp:14828`, `clif.cpp:10763`, and `clif.cpp:25798`. `vars_ok` is set false
at new-player initialization (`pc.cpp:2214`), not by these payment callbacks.
This is normal packet/login admission evidence, not permission to call a native
builtin against an artificial unloaded test session.

`get_reputation_points` (`script.cpp:28035`) reads the registry and clamps the
returned value. `add_reputation_points` (`script.cpp:28059`) independently reads
the **raw** value, adds its argument, clamps, calls `pc_setreg2`, and sends the
reputation packet. Reputation 6 resolves to plain numeric scalar `RepPoints6`.

`pc_setreg2` (`pc.cpp:11652`) rejects unsuitable variable scopes/types, clamps to
the signed 32-bit range, then routes this scalar through `pc_setglobalreg` to
`pc_setregistry` (`pc.hpp:1577`, `pc.cpp:11531`). For the valid scalar and loaded
session, the latter updates/creates the in-memory numeric registry entry and
marks it dirty. There is no script/event/achievement callback or synchronous
SQL commit in this setter. Its ordinary failure branch is refusal before
registry loading. `clif_reputation_type` (`clif.cpp:24267`) constructs/sends a
packet; it does not execute another script.

Therefore the candidate's in-range addition cannot overflow or be narrowed by
the setter. The builtin's return status is **not** a credited-amount or rollback
API; safety here comes from the established preconditions. Allocation failure,
process death, delayed persistence failure, malformed unloaded sessions, and
administrative mutation are not crash-atomicity guarantees supplied by this NPC.

## Document deletion and synchronous callback closure

The effective item 1001289 is `Bar_D_Docu_1`, Type `Etc`, Weight 1. Its only
other effective fields are its name and NoDrop/NoTrade/NoSell/NoCart/
NoGuildStorage/NoMail/NoAuction trade restrictions. It has no Script,
EquipScript, UnEquipScript, equipment location, GUID, AutoEquip, or stack
override. The effective record was reconstructed through the ordered item
import graph, not inferred from its base file alone.

`countitem` (`script.cpp:7136`, `7225`) counts matching positive non-rental
inventory rows. Ordinary `delitem` (`8350`, `8417`, `8581`) counts first and then
deletes matching rows in its existing two-pass selection order, preferring
unequipped/unrefined/cardless rows. Its plain search can include rental rows:
**these are not identical metadata filters**. A sufficient non-rental count
still establishes sufficient total deletable documents in the valid, stable
inventory, and the candidate preserves the existing deletion policy rather
than inventing a new bound/rental restriction. The amount is at most 6668.

`pc_delitem` (`pc.cpp:6103`) logs and reduces the stack/weight. A depleted row
clears equipment-switch cache references and then the item. The ordinary Etc
document is not equippable, so no equipment, card, combo, or status-calculation
scripts are reached through unequipping it. The switch cleanup itself only
updates cache/item fields and sends its packet. Item-use scripts are not run by
`delitem`. Invalid inventory/cache or forged equip flags remain outside this
normal-state proof; the script-level deletion helper does not propagate every
possible native deletion failure.

The actual callback branches are:

```text
pc_delitem
  -> clif_updatestatus(SP_WEIGHT) -> pc_updateweightstatus
       -> start/end Weight50 or Weight90
  -> pc_show_questinfo(current map)
       -> achievement_check_condition -> nested run_script(condition)
       -> restore the parent script attachment
```

The effective Weight50/Weight90 records have no Script or CalcFlags; their
EndOnStart relation concerns each other. Their reviewed source transition does
not mutate documents or reputation or recalculate equipment scripts. This
branch is source/content evidence; a native transaction fixture which doubles
transport must not claim it executed all weight-status notification code.

Fresh collection of the enabled graph gives **900 scripts / 14 include configs**
and **33 `ba_in01` quest-info registrations / 19 owners**. The owners reside in
`npc/re/quests/quests_17_2.txt` and `npc/custom/episode19/quests_19.txt`. Their
conditions only read quest state/time/objectives, BaseLevel, episode variables,
and counts of items 7110, 7326, and 1000226; none reads or writes item 1001289 or
`RepPoints6`. `quest_check` (`quest.cpp:898`) performs the relevant reads, not a
quest deletion on expiry. The existing scope collector also accounts for
duplicate parents, enabled registration sites, unit warps, and dynamic map/NPC
producers. Merely checking the Administrator's own quest-info declaration would
not establish this closure.

`pc_show_questinfo` (`pc.cpp:15399`) can actually execute the conditions through
`achievement_check_condition` (`achievement.cpp:875`), which detaches and later
reattaches the parent. Current conditions have no payment/registry mutation or
yielding commands. `pc_delitem` does not emit an AG_GET_ITEM achievement update.
Thus no reviewed synchronous intermediate branch invalidates the fresh count,
gain, map, access, registry-load state, or remaining multi-stack deletion.

## Current reputation writers and the raw/clamped limitation

A comment/string-aware command inventory over the enabled graph found **51
`add_reputation_points` sites, zero `set_reputation_points` sites, and zero
literal `RepPoints6` occurrences**. Searching for reputation-named computed
`set`/`setd` writers and the database script content did not identify an ordinary
direct writer of that variable.

All 51 add targets resolve as follows:

| Target | Sites | Source families |
| --- | ---: | --- |
| 3 (`REPUTATION_EP18`) | 19 | Episode 18 quests |
| 4 (`REPUTATION_EP19` or literal 4) | 28 | Episode 19 quests/instances/patrol and Episode 20 progression |
| 6 (`REPUTATION_BIOSPHERE_DEPTH1`) | 1 | This Administrator |
| 9 (`REPUTATION_BIOSPHERE_DEPTH2`) | 1 | Depth Abyss hunt reward manager |
| 13 | 1 | Episode 21 ghost-ship reward helper |
| 13–19, selected from an explicit seven-entry array | 1 | Episode 21 family supply daily |

Starting from an absent scalar (native read returns zero) or an already in-range
value, these examined normal producers preserve the reputation-6 range. This
is not a whole-program proof against arbitrary string-generated variable names,
privileged script execution, GM variable commands, SQL edits, imported legacy
state, or arbitrary persisted script text. No live-registry query was performed
by this reviewer.

The raw/clamped distinction remains real outside that invariant. For raw
`RepPoints6 = -5001`, the getter reports -5000; a one-pair gain of 3 is then added
to -5001, producing -4998, only two displayed points of improvement. More deeply
out-of-range raw values can produce no visible improvement despite payment.
The candidate does not normalize or repair corrupted/privileged stored values.
Neither silent setter substitution nor a broad engine change was added to this
frozen batch.

## Other document/material-to-reputation paths (inventory, not fixes)

Only this enabled Administrator combines a debit of 1001289 with reputation 6.
Other 1001289 sites grant research documents for hunt completions.

An analogous document hand-in exists at `Folklorist Gudra#ep18`
(`npc/re/quests/quests_18.txt:7545`). The Recording Note 1000408 is consumed at
line 7646 before the first reward's +100 reputation-3 call at 7653; the daily
variant consumes one note at 7742 before +30 at 7748. These also change quests
and grant items/experience and are not the same variable-quantity exchange.
Their complete transaction/capacity behavior was **not** repaired or certified
by this review.

`Mandel#ep21_reputation` (`npc/custom/episode21/FamilyReputation.txt:17`) is a
related fixed-quantity material daily: ten selected supplies -> ten vouchers
1001618 plus 100 selected family reputation at line 75, followed by quest/daily
bookkeeping. The supply identities are 1001629, 1001648, 1001639, 1001637,
1001646, 1001642, and 1001645. It is not a 1001289 exchange; generic
`checkweight`/grant and its own callback boundaries need a separate audit.
Other Episode 18/19 material hand-ins also accompany reputation gains. The
51-site writer inventory is not a claim that every indirect gameplay reward
path or every hand-in is fixed.

## Separate dormant parser finding: omitted Maximum

`ReputationDatabase::parseBodyNode` assigns `INT64_MIN` to a **new** row whose
`Maximum` is omitted (`pc.cpp:347`), whereas the schema documents `INT64_MAX`.
This is a source defect for future definitions, left unchanged in this batch.

The current runtime root `db/reputation.yml` imports `db/re/reputation.yml`
(Renewal only) and then `db/import/reputation.yml`. Its **13 effective entries**,
IDs 1–4, 6, 9, 13–19, all explicitly
set Minimum and Maximum. **Zero current entries are affected.** The generator
import under the Renewal file is gated by `Generator: true`; loading generators
defaults false (`src/common/database.hpp:26`, `database.cpp:220`). Even when
selected for a generator run, its four visibility-only records reference
existing IDs and retain their already assigned limits.

Raw identities for this separate finding:

| File | SHA-256 |
| --- | --- |
| `db/reputation.yml` | `0349d019ae57c9c5c40f0a13719000345dee442205038e95f76b9d018b6b8323` |
| `db/re/reputation.yml` | `3f98b479fb165cc06f9a613b952e81f650be38c2a742b69076647404bd2209a9` |
| `db/import/reputation.yml` | `83cace0cda525350959195af10f80c67db1f3ea5154cd48ac4051f0a571e3272` |
