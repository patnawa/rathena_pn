# PN Main Office

The Office now has one compact hall with 50 clearly named service NPCs.
Use `@office` to arrive at `pn_office,50,35`, or `@adventure` to plan a session
from wherever you are. Click Directory to choose a category or search an NPC
name; navigation marks the exact walkable cell beside that NPC.

| Group | Position in the hall | Services |
| --- | --- | --- |
| Everyday | Entrance / south | Healing, storage, reset, card removal, settings, preparation presets, inventory tools, dealer and cafe |
| Jobs and Pets | Left / west | Job changes, Druid mentor, platinum skills, rentals, skill supplies, pet food/care/recovery |
| Adventures | Center | My Adventure, progression, travel, Temporal Tina, instances, party board and boss rewards |
| Equipment | Right / east | Damage Lab, Rune Tablet, repair, enchants, refining, ores, crowns, runes, Constellation, Build Notes and grading |
| Fashion | Back / north | Stylist, Gold Points, designer, recycling, boxes, catalogue, enchants, copy skills and clans |

All desks occupy a 49 by 48 cell area in the existing arena hall resource.
The new `pn_office` map is 100 by 100 cells, replacing the old 220 by 200 lobby
and eliminating floor travel for services. NPC collision and every approach
are checked by flood fill. The old `pn_train` and `pn_style` maps remain
registered for compatibility; entering them returns you to the new hall.
Office save points and old outlying login positions migrate on login.

## Player conveniences

See [Adventure services](adventure_services.md) for the full feature guide.
My Adventure connects story progression, eight equipment wishlist goals,
current instances and cooldowns, preparation, inventory tools, weekly choices,
party recruitment, boss material progress, and public build snapshots.

Existing services retain their fees and eligibility. Card removal retains its
normal risks. Refining and grading require an unequipped inventory item that
is not in equipment switching. `@office`, `@fashion` and `@goldpoints` retain
the existing death, instance, PvP/GvG/battleground and escape restrictions.
The private Damage Lab returns you to the compact Office entrance.

| Command | Result |
| --- | --- |
| `@office` | Compact hall entrance, 50,35 |
| `@adventure` | Session dashboard and all new convenience systems |
| `@fashion` | Fashion group, 49,67 |
| `@goldpoints` | Gold Point Manager, 43,67 |
| `@settings` | Saved character/account login preferences |
| `@activity` | Gold/Fashion balances and earning rules |

Click the service NPC after arriving. Gold Points accrue at one per three minutes online and stop at 50. The manager exchanges them one-for-one for Fashion Points; exchanging below the cap resumes the earning timer. These are login-account balances, shared by that login's characters, not by separate login accounts.

Fashion services include costume/stone trades, stone recovery, enchantment and stone boxes. Boxes 1–20 cost 50 Fashion Points; the garment second-slot box costs 300. The Fashion Catalogue now sells 41 supported costumes at the PN price of 150 Fashion Points each. Ordinary costume trade-ins award 15 points, Bio5/Tomb costumes award 1, and listed stones award 10 each. Favorite, bound and modified items cannot be traded in. See the [exchange audit and service guide](fashion_exchange_20260928.md) for the dialogue-loop repairs and verification.

## Client assets and rollout

Every client must update `pn_office.grf` before visiting the compact hall.
Use the signed launcher's Check for updates. The server and client collision
maps must change together; an old lobby client cannot render the new layout
correctly. Retain the patch while any character is saved in an Office map.

For a fresh build from the owner's original `guild_vs1` and `iz_ac02` assets:

```sh
python3 tools/build_main_office.py build . /path/to/data /path/to/new-build
python3 tools/generate_main_office_layout.py
```

The builder checks dimensions and every walkable cell, changes only the two
RSW alias filename fields, and emits nine GRF resources plus hash metadata.
The source art is retained; the compact layout is original PN placement.
The old three-map aliases remain so existing references do not lose maps.

The fresh-client installer intentionally refuses to replace a different
installed archive. Existing clients upgrade through the signed update and its
rollback backup. For an import-cache migration, pass the exact reviewed old
`pn_office` record SHA-256 to `tools/build_main_office.py merge-cache` with
`--reviewed-office-sha256`; every other collision still refuses replacement.
Unrelated server cache records are preserved.

The actual placement coordinates and grouping are generated in
[`layout.json`](../npc/custom/main_office/layout.json). Load shared services,
new feature scripts and Shadow Supplies before their visible duplicates.
No new map index or SQL migration is required.

Automated VM, collision and asset checks establish behavior and geometry;
a current client session is still needed to judge appearance and usability.
