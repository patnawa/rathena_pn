# PN Main Office

The Main Office brings 52 service desks into three connected service areas.
Use `@office` or the Main Office attendant in Prontera, Izlude (including its
configured variants), or the Grade Workshop. Each floor has a directory with
case-insensitive search and transport to a reachable cell beside the selected
desk. The directory also provides floor travel and a return to Prontera.

| Area | Entrance | Services |
| --- | --- | --- |
| Lobby | `pn_office,100,40` | Healing, storage/save point, stat/skill reset, card removal, job changes, Druid mentor, platinum skills, rentals, supplies, pets, travel and progression |
| Training and equipment | `pn_train,50,35` | Private damage lab, Rune Tablet, repairs, equipment/shadow enchants, refining, ores, Frontier crowns, Varmundt runes, Constellation, build notes, reset and card removal |
| Fashion and lounge | `pn_style,140,140` | Stylist, Fashion Points services, recycling, costume enchants, skill copying, four clans, cafe, healing and storage |

The service concept is inspired by the [MuhRO Main Office](https://wiki.muhro.eu/Main_Office).
This is an original PN arrangement using existing client academy and arena
resources, with independent map names and town flags. The source arena's PvP
flags do not apply to the training floor. It does not contain MuhRO's custom
map artwork, economy, or every NPC from that server.

## Behavior

Existing services retain their original fees, eligibility and consequences.
In particular, the Wise Old Woman's card removal can destroy cards or gear on
failure; moving the NPC does not change that system. Job Master, Stylist and
Rental Service use hidden templates with visible office duplicates. Their
former disabled town placements are not introduced on top of existing NPCs.

`@office` refuses dead characters, instances and maps restricting escape or
participating in PvP, guild war or battlegrounds. The office Kafra can save the
character's respawn point after confirmation. Build Notes stores three short
notes and base-stat snapshots; it is a notebook, not a loadout switcher. Skill
copying goes through the native copy eligibility rules and learned skill caps.
The instance desk reports the current reservation and directs players to the
real instance entrances for their individual requirements and cooldowns.

The damage lab retains its private instance and measurements. Leaving a lab
created from the office returns to the originating lobby or training floor.
See [PN Services](quality_services.md) for the Poring target and measurement limits.

## Client installation

The server cache includes the three alias maps. Every player's client also
needs `pn_office.grf` before visiting the office. Full client assets are not
distributed in this repository. Build the patch from the owner's extracted
`iz_ac01`, `guild_vs1` and `iz_ac02` GAT/GND/RSW files:

```sh
python3 tools/build_main_office.py build . /path/to/extracted/data /path/to/new-office-build
python3 tools/generate_main_office_layout.py
python3 tools/install_main_office_client.py /path/to/client /path/to/new-office-build
```

Close the game before installation. The installer verifies the build hash,
preserves the relative order of existing archives, and saves `DATA.INI` plus
a receipt under the client's `server-work/client-before-main-office-*` folder.
Restore that `DATA.INI` to roll back client priority. Keep the patch installed
while any character is saved or logged out in an office map.

The builder validates dimensions and every walkable cell against the server
cache, patches only the two alias filename fields in supported RSW 2.1 files,
and writes an unencrypted nine-resource GRF with a hash manifest. The layout
generator verifies desk spacing and flood-fill reachability after treating all
NPC tiles as obstacles. Generated coordinates are in
[`layout.json`](../npc/custom/main_office/layout.json).

## Server rollout and checks

Load Rune Tablet and office service definitions before their duplicates in
`npc/scripts_custom.conf`. The aliases are registered in `conf/maps_athena.conf`
and `db/map_index.txt`; their geometry is tracked in `db/map_cache.dat`.
Servers with an overriding import cache should merge the generated alias cache
with `tools/build_main_office.py merge-cache`, preserving other map records.

Restart the character and map servers during a no-player maintenance window
when adding map indexes. Check the native startup log, not just the process
exit status. Isolated startup validation and cell connectivity do not prove
rendered client appearance or a player's complete service transaction.

## Equipment audit and command update ? 8 September 2026

The lobby now includes Player Settings (also `@settings`), and Training includes
Grade Enhancer, sharing Sratos' existing grading and Etel exchange service.
The generated layout verifies all 52 desk approaches with NPC cells blocked.
Native grading and refining require inventory targets: unequip the item and
remove it from equipment switching before selecting it.
