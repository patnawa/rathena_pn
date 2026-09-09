# Instance access audit - 2026-09-09

The catalog in [instance_access_manifest.json](../tools/ci/instance_access_manifest.json) maps 65 unique instance guide categories to the effective Renewal database, enabled NPC files, public entrance candidates, current Warper labels and installed client map receipts. Language variants are deduplicated. Category pages and reward guides are not treated as missing instances.

The reviewed import graph contains 915 enabled NPC files and no missing imports. All 100 instance definitions have an enabled script reference and NPCs on their instance maps. All 120 unique client base maps have GAT/GND/RSW resources; `1@ch1b` resolves through an intentional alias. Existing Chapter 2 maps use deliberate compatibility terrain. These checks establish access/resource presence, not encounter completeness or a played-through instance.

The reviewed Warper now has 68 destinations covering 60 implemented guide groups. Five other guide pages describe absent content, an overview or rewards. All implemented guide groups have a route to their local entrance; this does not claim complete equivalence with another server's mechanics. Several difficulties share one entrance. The Chapter 1 overview and its repeatable guide are counted separately as pages but not as two missing instances. Hidden Flower Garden includes story ID 58 and daily Security Area variants 59/60; only the gated daily entrance is exposed.

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

## Content that is not implemented

| Content | Verified local gap | Feasibility boundary |
| --- | --- | --- |
| Alice Twisted Madness | No instance definition or script; the documented `1@alice_mad` map triple and boss ID 30239 are absent. | A faithful maze and coordinated party encounter need missing custom geometry, boss data and encounter logic. A generic entrance would not supply the content. |
| Bioresearch Laboratory | No matching instance/entry script or Unknown Swordsman monster definition. The documented reward items exist. | The guide establishes entry requirements and rewards but not a complete map/spawn/encounter specification. Do not equate this with Bagot Laboratory or a normal field dungeon. |
| Tower of Trials | No instance implementation; reward IDs 50208/50209 are absent. | Existing tower maps and instance scripting could support a separately designed version. Exact custom spawns, skill/stat scaling, resistance-cap enforcement, trial rules and weekly account reward semantics need implementation and validation. |

A distinct Endless Cellar mode is not verified by the existing Endless Tower implementation. Instance Antiquity is a reward reference, not an instance.

## Regression checks

Run `python tools/ci/instance_access_manifest_test.py`. It checks that reviewed instance IDs/names/maps remain in the effective database, their scripts remain enabled and referenced, and all reviewed Warper destination labels remain present. The installed-client map receipts are explicitly outside the clean-checkout test's claim.

Refresh the catalog after a route batch is finalized. Entrance candidates include source paths and line evidence; cloaked story NPCs are not automatically safe public destinations. Client assets do not imply server implementation, and menu labels do not imply that gameplay prerequisites may be skipped.

External category snapshots, exact reference URLs, entry extraction and client resource evidence remain outside Git in the instance coverage audit directory. No client or server assets were modified by this catalog audit.
