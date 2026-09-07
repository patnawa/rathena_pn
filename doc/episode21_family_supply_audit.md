# Mandel family supply audit — 2026-09-06

## Status and scope

The 2026-09-06 section below records the frozen original findings. The reviewed
Mandel repair is now installed locally at SHA-256
`3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb`
and has an installed-source native regression receipt. This document does not
claim SQL crash consistency, reconnect persistence, or graphical client proof.

The reviewed Renewal include graph contains 900 scripts and 14 include files.
`npc/scripts_custom.conf:227` enables
`npc/custom/episode21/FamilyReputation.txt`. Its only service is
`Mandel#ep21_reputation` at `jor_mbase,219,315`; its separate helper
`EP21_AllFamilyReputation` requires all reputation IDs 13–19 to reach 1000.
No enabled source outside this file writes `EP21_SupplyUnlocked`,
`EP21_SupplyFamily`, or `EP21_Supply_Daily`.

## Prioritized findings

### 1. A hand-in crossing 04:00 records yesterday's completion period

`FamilyReputation.txt:40` captures `EP21_DailyKey` before both `next` and
`select` in the active-contract branch. Line 80 writes that old value after
payment and rewards. An ordinary character can:

1. Accept a contract and open its delivery dialogue shortly before 04:00.
2. Choose delivery shortly after 04:00, well within the normal NPC timeout.
3. Pay ten materials and receive the first reward, while the script records
   the preceding period.
4. Reopen Mandel, accept another contract, and complete it in the new period.

Both completions occur after the same reset. This violates the script's
one-completion-per-period promise without concurrent dialogues, privileged
register changes, or unusual items. Fixing the shared clock helper alone does
not repair this cached-key defect: compute the current key and recheck today's
completion after the final menu response, before payment. Use that same fresh
key for the final marker. Keep an unfinished contract across midnight/reset;
there is no current expiry or automatic cancellation policy to replace.

There is also a separate shared-helper prerequisite under root review.
`MysteriousGhostShip.txt:15–16` combines one `gettimetick(2)` timestamp with
three independently sampled `gettime` components. `script.cpp:11430/11476`
and `date.cpp:72` onward reach separate `time(nullptr)` calls. Even an ordinary
second boundary can produce a key differing by one second; minute/hour/day
boundaries can produce larger discrepancies. Thus equality can fail even
without a 04:00 dialogue crossing. A single captured timestamp must supply
all calendar components, preserving the existing local 04:00 timestamp-key
representation. The installed single-snapshot helper and its independent
native proof passed 173,268 fixed cases, 117 stable controls, 45 exact-original
defects, and 867,714 assertions. Receipt SHA-256:
`bdf9288a91f73b223025cb5f6b5bb980470e9ef4f5ca27c9ff302bfe3831288e`.

### 2. Reaccepting a completed family never restores its active quest

A successful delivery leaves its quest in `Q_COMPLETE` (lines 78–79).
On a later day, accepting the same family assigns `EP21_SupplyFamily` and
unconditionally calls `setquest` on the retained quest (lines 95–96).

Actual `quest_add` (`quest.cpp:590–602`) rejects every already-present quest,
including completed ones. `completequest` moves the row into the completed
tail; it does not delete it (`quest.cpp:849–876`). `setquest`
(`script.cpp:21809`) reports the error but returns script success, so Mandel
still claims the new contract was registered. Delivery remains possible via
`EP21_SupplyFamily`, but the quest journal is not restarted, and acceptance
logs an error on every repetition. This is a reachable journal/state defect,
not a claim that the reward becomes permanently blocked.

Before a new acceptance, erase the selected quest only if it is completed,
then add it only if absent. An already-active selected row can be retained
as a recovery case without duplicate `setquest`. Do not erase unrelated
family quests, invent a cooldown quest, or change the chosen family/quest
mapping. With the current pure callbacks, this short acceptance sequence
needs no intermediate dialogue yield.

### 3. Every click adds another native quest-marker condition

Mandel's line 18 runs at normal dialogue entry, including locked, review,
cancel, and already-completed visits. It is not an `OnInit` declaration.
`buildin_questinfo` (`script.cpp:21701–21788`) parses a fresh condition and
unconditionally appends it to `nd->qi_data`. Only the NPC ID in the map's
`qi_npc` vector is deduplicated. No automatic parser relocation or
player-display reinitialization occurs on this command path.

Consequences of ordinary repeated clicks are:

- Parsed conditions accumulate until the NPC is unloaded. False conditions
  require the native loop to inspect all accumulated copies; this is not
  merely a duplicate packet display.
- The first lazy registration changes the map's owner-vector size. Characters
  whose `qi_display` was initialized before that registration fail the exact
  size guard in `pc_show_questinfo` (`pc.cpp:15412`) until reinitialization.
  The service itself still works, but native marker evaluation can be skipped.

Move this one registration to Mandel's `OnInit`, terminated with `end`, and
leave normal dialogue entry free of registration. Test actual startup/owner
registration and repeated clicks, not just a regex asserting one source site.
Other `jor_mbase` owners have the same lazy registration shape; changing only
Mandel cannot certify that the whole map has stable owner membership.

There is also a small marker-timing issue: native `completequest` refreshes
quest markers before Mandel writes the final daily/family variables; closing
does not itself refresh them. An explicit `questinfo_refresh` after the final
markers is a narrowly related UI correction. It does not replace the daily
payment guard or fix neighboring owners' size mismatch.

### 4. Conditional native-valid inventory loss, not an established ordinary producer

The existing `checkweight(1001618,10)` is fresh after the last yield, but
`pc_checkadditem` (`pc.cpp:5729`) examines the first same-ID row without bound,
rental, unique-ID, or card metadata. `getitem` creates a plain item, while
`pc_additem` (`pc.cpp:5991`) requires those fields to match and uses the first
compatible row. A concrete native-valid inventory counterexample is:

- Every allowed slot is occupied; one contains an account-bound voucher and
  none contains compatible plain vouchers.
- Another contains eleven selected materials, so paying ten does not free it.
- Weight is valid and the bound voucher stack has room according to the
  ID-only test.

The preflight succeeds, ten materials are removed, and the plain voucher
grant fails for lack of a slot. `getitem` returns builtin failure without
ending the script (`script.cpp:7738–7743`, `run_func:4135`); reputation and
the daily marker are still applied. Moving the daily write after `getitem`
does not repair this. A failing first compatible full stack also cannot be
bypassed by a later compatible row or empty slot.

However, the current voucher producers inspected here create plain items.
The selected item-group graph has only five voucher entries, all in
`AEGIS_103537`, subgroup 6, at indices 34–38. They grant 41–45 vouchers without
`Bound`, `Duration`, `Named`, or `UniqueId` overrides. Enabled literal NPC
voucher grants are the Episode 21 plain `getitem` sites. No ordinary
voucher-metadata producer was established. This is a supported-state
transaction defect deserving defensive coverage, not a demonstrated
player-only exploit or a loss path for unchanged plain-only inventories.

If included in the repair, use a purpose-scoped exact plain-voucher preflight
before payment. Mirror native ascending first-compatible semantics, require
`1 <= getinventoryslots() <= MAX_INVENTORY`, and reserve an empty allowed slot
only when no compatible stack exists. Do not transfer input metadata to the
reward. Preserve the current conservative prepayment capacity policy instead
of assuming a consumed material row will free a slot. Existing Final Battle
and Biosphere helper shapes supply precedent, but their identity allowlists
and callback gates do not cover Mandel automatically.

## Existing contract, items, and acquisition

These are current project declarations, not newly verified official rates.
Every completion consumes exactly ten units, grants ten vouchers and invokes
`add_reputation_points` with 100. No Zeny, quantity input, multi-recipe debit,
or signed-16-bit quantity overflow is involved.

| Family | Reputation | Quest | Material ID / effective name | Example enabled source |
| --- | ---: | ---: | --- | --- |
| Gaebolg | 13 | 17782 | 1001629 Jormungandr Catechism | Yormi 22353/22354, `jor_tmple1`; also Yosters |
| Nerius | 14 | 17783 | 1001648 Ice Star Candy | Ice Seahorse 22318, `jor_raise2` |
| Heine | 15 | 17785 | 1001639 Flashy Accessory | Yortus 22360/22361, `jor_tmple2` |
| Lugenburg | 16 | 17786 | 1001637 Snake Shape Dagger Piece | Yormi 22353/22354 and Yordos 22356/22357, temple maps |
| Walter | 17 | 17787 | 1001646 Fluffy Core | Cliolima 22320, `jor_raise2` |
| Wigner | 18 | 17784 | 1001642 Guano | Seawind 22313, `jor_raise1/2` and `jor_base` |
| Richard | 19 | 17788 | 1001645 Stiff Limb | Letterster 22316, `jor_raise1` |

The ordered Renewal item graph contains exactly one row for each of these
seven IDs and voucher 1001618, all in `db/re/item_db_etc.yml:98185–98444`.
The materials have `AegisName: aegis_<ID>`, native Weight 10, Type Etc, and
BuyingStore enabled. The voucher is `Ep21_Wigner_Ticket`, named
`Wigner Premium Exchange Ticket`, Type Etc, BuyingStore enabled, with native
default Weight 0. All eight have default Buy/Sell 0, no item-specific stack
cap, GUID generation, equipment locations, scripts, autoequip, or trade
restrictions. No missing current item identity blocks a contract.

`db/import/mob_db.yml` supplies 21 relevant monster rows, each appearing
once in the selected mob graph. Their configured material rates are 800
for the listed field creatures/Yormi, 500 for Yortus/Yordos, and 300 for
Yosters' catechisms. `FieldMonsters.txt:9–78` enables the ordinary field
sources and their stronger `jor_sklf1/2` variants. These are raw database
rates, not a claim about final player-specific drop probabilities.

All quests 17781–17790 exist once in the selected quest graph. The unlock
and seven family rows have only ID/title, no objectives, drops, or time
limits. 17789 has `TimeLimit: 4h`, but neither it nor 17790 is used by this
service. Do not introduce them to replace its existing persistent daily key.
One display-data discrepancy is confirmed: Wigner's selected quest 17784 is
titled `Nerius Supply Request`, the same as 17783. Correcting authoritative
quest/client naming needs separate evidence; changing the identity or recipe
to match this apparent title error would be unjustified.

`db/import/reputation.yml` defines 13–19 with range [-1000,1000] and permanent
variables `REP_EP21`, `REP_EP21_Nerius`, `REP_EP21_Heine`,
`REP_EP21_Lugenburg`, `REP_EP21_Walter`, `REP_EP21_Wigner`, and
`REP_EP21_Richard`. Actual `add_reputation_points` (`script.cpp:28059`)
clamps to that range. Consequently a family at 950 gains 50, and a family at
1000 gains zero while still receiving vouchers. Preserve that existing
cap/reward policy; adding a cap-based refusal would change the economy.
Gaebolg alone updates the compatibility mirror `EP21_Reputation` afterward.

## Current callback and state boundary

The enabled graph has 30 `jor_mbase` declarations and 16 literal quest-info
sites in 16 owners. All 16 registrations currently occur on dialogue entry,
not `OnInit`, so these are static site counts, not a claim that a fresh map
already has 16 owners or that each owner has only one native condition.

| Source | Owners / conditions |
| --- | --- |
| `Progression.txt:202,219,229,239,284` | Resistance Soldier and Tris/Tan/Lee: quests 23241–23243 and `EP21_ResistanceStep` |
| `Progression.txt:299,408,434,443,468,488` | Tris/Ivan/Lehar/Lyriq/boat: quest reads, `EP21_Alberta_Complete`, `EP21_GaebolgComplete`, `EP21_CultComplete` |
| `GimliInfiltration.txt:158` | Tris: quests 17763/17766 |
| `BlackHairedBeast.txt:56,161` | Tris: `EP21_GhostShipUnlocked` and `EP21_MainComplete`; Nadoyo: quest 23254 |
| `FamilyReputation.txt:18` | Mandel: `EP21_MainComplete`, supply daily register, `EP21_DailyKey` |
| `SideDailies.txt:71` | Tris: quests 17773/17776 |

The complete user-function closure is `EP21_MainComplete`,
`EP21_GaebolgComplete`, `EP21_CultComplete`, `EP21_QuestInRange`,
`EP21_GhostShipUnlocked`, and `EP21_DailyKey`. These functions have one
enabled definition each and read quests/registers/time; they do not grant or
consume items, write permanent state, or yield. The union of direct quest
IDs and literal range queries is 60 identities, all defined. Mandel's seven
contract quests plus unlock quest add eight separate fixture identities.

Actual synchronous paths requiring this closure are:

- `delitem -> pc_delitem -> weight status -> pc_show_questinfo`;
- `getitem -> pc_additem -> weight status -> AG_GET_ITEM -> pc_show_questinfo`;
- `setquest`, `erasequest`, and `completequest` each refresh quest info.

The first `delitem` count pass checks that the entire ten-item debit exists
before deleting; quantity ten fits its native signed field. `countitem`
excludes rentals, whereas ID-only deletion can consume them; this is existing
policy, not a new shortage when ten non-rental units remain available.
The relevant Etc items cannot ordinarily be equipped. Weight50/Weight90
have no Script or CalcFlags in current data. Voucher Sell 0 supplies ARG0=0
to all seven current Get_Item thresholds (100 through 150000), so none can
complete from this grant; their condition scripts are pure comparisons.
Deferred achievement reward scripts are not automatic grant callbacks.

The reputation setter uses the actual loaded permanent registry, then a
notification function; neither introduces another script callback. No
current callback above can change Mandel's selected family, payment,
voucher capacity, or completion register between its final checks and
commit. `achievement_check_condition` saves/detaches/restores the outer
script's attachment (`achievement.cpp:875`); native tests must execute that
path rather than silently replacing the loop with success.

Original safeguards worth retaining are the initial campaign gate, fresh
post-menu material count and capacity check, explicit review/cancel/leave
branches, one persisted active family, and the daily completion check.
`EP21_MainComplete` is monotonic in the current enabled writer inventory:
only true assignments were found, and no enabled erase/change of completed
18360. `npc_click` rejects another active NPC dialogue (`npc.cpp:2216`);
continuation checks NPC identity and proximity (`npc.cpp:2292`). Native
`select` ends on Escape, and the packet parser rejects zero/out-of-range
selections (`script.cpp:5226`, `clif.cpp:13334`). Thus a synthetic second
simultaneous Mandel dialogue, arbitrary menu result 255 continuing the body,
or mid-menu campaign reset is not an established ordinary exploit here.
Fresh domain/access/map/family/daily checks are still reasonable defensive
hardening after each actionable yield, but must not be misreported as
reproduced native packet bypasses.

## Historical repair and native-test checklist

Implement the single-clock shared helper only after its separate review;
keep Mandel's transaction patch scoped to fresh post-menu state, completed
quest recycling, one-time marker registration, and optionally exact plain
voucher capacity. Preserve all seven arrays, ten-unit rates, reputation
cap, acquisition, cancellation, overnight pending contracts, and local
04:00 reset representation. Keep the final checks/payment/reward/marker
sequence free of an intermediate yield. Do not add quantities, prices,
invented refunds, or a crash-atomicity guarantee.

A new runner should use the actual complete extracted NPC plus its user
functions, real parse/run/select, native quest DB/log, eight item definitions,
native delitem/getitem/checkweight, and actual loaded registry/reputation
DB. Use the document exchange fixture's clean registry load flags and
artifact/source binding, and the material fixture's real nested QuestInfo
adapter. Add a newly reviewed `jor_mbase` dependency gate rather than
reusing the `ba_in01` or Final Battle map closure as if it were equivalent.

Required cases include:

- All seven accept/deliver/repeat-next-day cycles, exact debit/reward/quest
  and mirror changes; reputation -1000, 0, 900, 950, 999, and 1000.
- Fresh unlock, denied access, completed today, overnight active contract,
  review, cancel, leave, Escape at both menus, and reaccepting active or
  completed journal rows without duplicate-quest errors.
- Actual final-yield clock change across 04:00, and exact original-source
  controls that reproduce the stale marker and repeat-quest error. Keep
  the shared helper's clock-race proof distinct from this stale-local test.
- Zero through nine materials, exactly ten, eleven, split native stacks,
  voucher amounts 29990/29991, no voucher plus free/full slots, plain
  compatible stacks, and explicit metadata-separated native inventories.
  Label the last class as native-valid defensive cases with unproved
  ordinary metadata acquisition; no malformed inventory claim.
- Real OnInit registration, repeated ordinary clicks, reload/unload-relevant
  cleanup, lazy neighboring owner growth, wrong-size `qi_display` skip and
  native reinit. Assert both a transaction-time increase in nested callback
  count and restored outer RID/locals, not just separate condition probes.
- Each of the 16 current condition sites true and false with real helper
  calls; distinguish these outcomes from exhaustive short-circuit path
  coverage. Pin all 60 referenced quests and the eight service quests.
- All unmodified original failure controls must remain hash-exact. Fresh
  ASAN/UBSAN builds, denied networking, diagnostic rejection even on exit 0,
  explicit clean allocator teardown, and source/executable/support-object
  checksums before/after any accepted reuse are required.

World lookup, transport, logging, character-server saves, and deterministic
clock interception must be explicit boundaries. Actual VM/registry/inventory
proof would not establish SQL crash consistency, reconnect persistence,
real-client marker delivery, or a live player walkthrough.

## Historical pre-clock/pre-QI source checkpoint

Raw and LF-normalized SHA-256 are identical for these reviewed NPC/data
files. The parent identified the current completed deployment baseline as
`777fb94c5`; this audit did not perform a Git or live-server check.

| Source | SHA-256 |
| --- | --- |
| `FamilyReputation.txt` | `d49ef88c4d7b2ef1a16fccff22f0f13aa82bf790c9a6a92a42ef66b4d8c39f17` |
| `MysteriousGhostShip.txt` | `b6baf2dfdb5d2bc47a67a4a8f31b002eb46fbec2a9f66de963047c18e250b70d` |
| `Progression.txt` | `0def3b01c11e6302d8d6133db2fe05713a2c10f320ce244f59f7c0ea1b6453d3` |
| `BlackHairedBeast.txt` | `40e9f25f960a1cb3b73a7c757ba63167b65d38d7830ceb442fef758feb76e151` |
| `db/re/item_db_etc.yml` | `385e14a6ce2a943ee847cc6ae876390cc4aeac484e77491eaf83962f44cac137` |
| `db/import/quest_db.yml` | `55d75cfefaaabc4d08c774ef95c56332b16648237331764e8a86283530ae0c43` |
| `db/import/reputation.yml` | `83cace0cda525350959195af10f80c67db1f3ea5154cd48ac4051f0a571e3272` |

## 2026-09-07 installed repair and native proof

The reviewed artifact remains at
`../episode21-family-supply-native-20260907/candidate.txt`; those exact bytes
are installed in `npc/custom/episode21/FamilyReputation.txt`. Its raw and
LF-normalized SHA-256 is
`3e6217fa35f57cc1969810938b523d2970b4982e6ce0c383aefea26a2d266aeb`.
Its exact QI-only inverse is
`c72019f51033d96054c277d952d993a45a68062da9f7b4ab99ee76edde2a9605`.
The runner reverses all five source substitutions byte-for-byte to that input, then
uses `episode20_21_questinfo_migration.py` to reconstruct the exact earlier
family source
`d49ef88c4d7b2ef1a16fccff22f0f13aa82bf790c9a6a92a42ef66b4d8c39f17`.
This proves that the already-reviewed one-owner `OnInit` relocation remains
present and unchanged.

The five exact source substitutions implement four behaviors:

1. Immediately after the active-contract delivery menu resumes, call
   `EP21_DailyKey` again, reject a completion already recorded for that fresh
   key, and use the same fresh value for the later commit. There is no yield
   between this recheck and payment.
2. On acceptance, erase the selected quest only when its native state is
   complete, call `setquest` only when that selected row is absent, and persist
   `EP21_SupplyFamily` afterward. An existing active row is retained.
3. After `EP21_Supply_Daily` receives the commit key and
   `EP21_SupplyFamily` is cleared, call `questinfo_refresh()`. This gives the
   marker condition its first evaluation against the completed state.
4. Replace the ID-only `checkweight` approximation with a purpose-scoped
   `L_EP21VoucherCapacity` preflight. It predicts the exact plain
   `getitem 1001618,10` stacking identity (bound, expiry, UID, and four card
   fields), allowed inventory slots, and the 30,000 stack maximum before any
   material debit. The call replacement and local helper insertion are the two
   source substitutions for this behavior.

The arrays, seven family/quest/material mappings, ten-unit debit, ten-voucher
grant, 100-point reputation award, Gaebolg compatibility mirror, reputation
cap policy, cancellation/review/leave paths, initial unlock, and `OnInit`
statement are exact source invariants. Source controls accept both LF and CRLF
forms of the QI-only source and final overlay and reject nine content/shape
mutations. Effective item, quest, and
reputation records are projected from the pinned selected Renewal database
graph, not handwritten substitutes.

`tools/ci/episode21_family_supply_test.py` and its C++ driver compile the actual
`pc.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp`, `achievement.cpp`,
`quest.cpp`, and allocator with ASAN/UBSAN. The accepted run uses the actual
Mandel body, real `select`/`next` resumptions, native inventory debit/grant,
loaded permanent character registry and dirty flags, native quest partition
and mutations, reputation database/clamping, and real QuestInfo evaluation.
Socket/connect/bind/listen syscalls are denied by the kernel fixture. The
production daily-helper bytes are pinned, while its clock acquisition is
replaced at the explicit native-VM boundary with a deterministic key so a menu
can cross the reset reproducibly; the helper's timestamp arithmetic remains a
separate clock-runner responsibility.

Accepted candidate coverage is 82 cases, 24,140 assertions, and 283 actual
Mandel marker-condition executions. It covers all seven mappings, reputation
starts -1000/0/900/950/999/1000, all seven accept/deliver/reaccept-next-period
cycles, active-row recovery, campaign denial, initial unlock, the reset-crossing
delivery, and a same-period reopen refusal. Every successful candidate delivery
ends with the real marker condition false and the display inactive.

The voucher-capacity cases compare the script preflight directly with native
`pc_additem`. They cover compatible voucher stacks at 29,990 and 29,991, no
voucher with a free or full slot, the first compatible stack inside or outside
the currently allowed slot range, contracted and maximum slot ranges, and
bound/card/UID/expiry-separated rows with both free and full inventories.
Transaction-level cases prove refusal occurs before material debit when only an
incompatible metadata row exists in a full inventory, while the corresponding
free-slot cases complete. Refine and random-option differences are deliberately
ignored because the native stackable-item comparison ignores them. These are
native-valid defensive states; this audit did not establish an ordinary
producer for every metadata variant.

The complete QI-only baseline source is also executed as a required negative
control: 6 cases, 611 assertions, and 15 marker evaluations reproduce two
completions after one reset, the stale visible marker until a manual refresh,
and native `quest_add` errors for both completed and already-active rows. The
candidate emits zero such errors; the current controls observe exactly two.

The installed-source receipt is
`../episode21-family-native-label-v2-20260907/receipt.json`, SHA-256
`ecf1cd43623af59b920f5d1d40957fecd1c8970124bdc1e5f12b2592f7e2a795`.
The executable SHA-256 is
`6f6a4a754e42d64e34fd21a5b7933b357e17512be207a3b943776365909a2d08`.
All retained source/header, object/library, executable, fixture, and output
hashes are in that receipt; three in-memory artifact-binding mutations and
seven zero-exit diagnostic mutations fail closed.

The self-contained `episode21_family_supply_overlay.py` owns the five exact
forward/reverse anchors and the complete current/candidate hashes. The QI
migration's public `pair` remains QI-only: a separate fail-closed `active_pair`
canonicalizes only an exact complete Family overlay before reconstructing the
exact `c720...` QI source and then the exact `d49...` pre-migration source. Its
controls accept four LF/CRLF QI-only/overlay forms, reject five overlay/isolation
mutations, and reject passing transaction bytes directly to the QI-only pair.
The independent 125-owner native lifecycle also passes through this interface:
fixed 6,216 assertions and 274 condition calls; old 4,964 assertions; 500 clicks
per phase; zero errors, failures, sanitizer findings, or allocator leaks. Its
installed-source receipt is
`../episode-qi-native-label-v2-20260907/receipt.json`, SHA-256
`efbffc1de3831db33015f92ecc94975f0e919675f15a1128c997c98722e41700`.

This evidence does not establish SQL crash consistency, a save/reconnect
cycle, real packet serialization or client icon rendering, a real world/map
lookup, a live-player walkthrough, an ordinary producer for every defensive
voucher metadata state, or deployment by itself.

## Live deployment addendum — 2026-09-07

The exact `FamilyReputation.txt` candidate was later deployed and read back at
raw SHA-256 `3e6217fa...`. The isolated candidate and live POST map startup both
passed, all guarded SQL rows remained stable, and the offline GM retained every
Episode 21 reputation at its configured maximum. The native fixture boundaries
above still apply to player walkthrough, packet rendering, reconnect, and crash
atomicity. See the
[deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
