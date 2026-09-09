# Instance access audit - 2026-09-09

The catalog in [instance_access_manifest.json](../tools/ci/instance_access_manifest.json) maps 65 unique instance guide categories to the effective Renewal database, enabled NPC files, public entrance candidates, current Warper labels and installed client map receipts. Language variants are deduplicated. Category pages and reward guides are not treated as missing instances.

The reviewed import graph contains 917 enabled NPC files and no missing imports. All 102 instance definitions have an enabled script reference and NPCs on their instance maps. All 124 unique client base maps have GAT/GND/RSW resources; `1@ch1b` resolves through an intentional alias. Existing Chapter 2 maps use deliberate compatibility terrain. These checks establish access/resource presence, not encounter completeness or a played-through instance.

The reviewed Warper now has 70 destinations covering 62 implemented guide groups. Three other guide pages describe absent content, an overview or rewards. All implemented guide groups have a route to their local entrance; this does not claim complete equivalence with another server's mechanics. Several difficulties share one entrance. The Chapter 1 overview and its repeatable guide are counted separately as pages but not as two missing instances. Hidden Flower Garden includes story ID 58 and daily Security Area variants 59/60; only the gated daily entrance is exposed.

The new routes include 12 later-episode entrance groups, 16 classic groups and the Chapter 2 Phantom of Nyrholt repeatable entrance. NPCs retain their quest, level, reservation, party and cooldown checks. The route tests maintained separately exercise the gate expressions and destination walkability. No direct entry bypass is added for the unimplemented content below.

The audit also repairs visible Episode/Chapter menu prefixes: `:` is a menu
separator in the script engine, so those labels now use ` - `. An early `end`
no longer prevents the remaining navigation registrations from executing.
Regional access helpers now run only for their own destinations. Previously,
eager boolean evaluation invoked Biosphere and Zero Cell checks during unrelated
warps, displaying misleading access-denial messages. Actual restricted-area
checks remain in place.

The same evaluation rule affected the Immortal controller's offline-member
guard. It now skips disconnected participants before reading their HP, allowing
the encounter clock to continue without querying an absent character.
Airship Crash's party briefing validator also separates its online, attachment
and quest checks. An offline member now returns not-ready and restores the
requesting character instead of aborting the script with no player attached.

Bioresearch Laboratory is now implemented on the native EDDA maps, with Sierra and a Warper route. The earlier audit incorrectly classified it as wholly custom and understated the public walkthrough: the official EDDA foundation and seven-area sequence are available. See [implementation and validation scope](bioresearch_laboratory.md).

Alice Twisted Madness now provides Normal and Hard modes on two original generated maze maps, with a guarded daily entrance, per-character route checkpoints, coordinated Hard entry and Confused Boy's nine material exchanges. Its terrain, encounter tuning and guaranteed final-clear currency are documented adaptations. See [implementation and validation scope](alice_twisted_madness.md).

## Content that is not implemented

| Content | Verified local gap | Feasibility boundary |
| --- | --- | --- |
| Tower of Trials | No instance implementation; reward IDs 50208/50209 are absent. | Existing tower maps and instance scripting could support a separately designed version. Exact custom spawns, skill/stat scaling, resistance-cap enforcement, trial rules and weekly account reward semantics need implementation and validation. |

A distinct Endless Cellar mode is not verified by the existing Endless Tower implementation. Instance Antiquity is a reward reference, not an instance.

## Regression checks

Run `python tools/ci/instance_access_manifest_test.py`. It checks that reviewed instance IDs/names/maps remain in the effective database, their scripts remain enabled and referenced, and all reviewed Warper destination labels remain present. The installed-client map receipts are explicitly outside the clean-checkout test's claim.

Refresh the catalog after a route batch is finalized. Entrance candidates include source paths and line evidence; cloaked story NPCs are not automatically safe public destinations. Client assets do not imply server implementation, and menu labels do not imply that gameplay prerequisites may be skipped.

External category snapshots, exact reference URLs, entry extraction and client resource evidence remain outside Git in the instance coverage audit directory. The Bioresearch follow-up adds server encounter data and synchronizes the client quest reward description.
