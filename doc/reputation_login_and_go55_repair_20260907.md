# Reputation login refresh and Constellation Tower command

The GM's saved reputation was already at maximum. An outgoing `0x0b8d`
reputation packet captured during map changes contained all 13 configured IDs:
1/2/4 at 3000, 3/6/9 at 5000, and 13 through 19 at 1000. No SQL adjustment
was needed. The active client's `new.grf` provides matching IDs and positive
limits for these entries; higher-priority archives contain no reputation tables.

The full reputation list was sent only in the `state.changemap` branch of map
loading. It now sends once when either `connect_new` or `changemap` is set, so
fresh connections initialize the window as well. Player-visible verification
of the fresh-login window is still pending; the captured maximum-value packets
preceded the repair and establish the map-change behavior.

`@go 55` was labeled Constellation Tower but selected `clock_01,0,0`, a random
Clock Tower destination. It now selects Oscar's actual entrance at
`e_tower,83,105`, matching Warper option I36. The misleading `clock_01` alias was
removed; the Constellation alias is retained. The destination is walkable in the
live map cache. The strict integrity audit passes with 57 route assertions,
zero errors and zero content warnings.

Both source changes were compiled in an isolated `rathena:local` Docker build.
The resulting map binary retains a 9488-byte `mmo_charstatus`, matching the
repaired character server. Its SHA-256 is
`9649b6277e273a438a7675d8253f72ddfe19cc203139d584f7833e306ed82865`.
The map service was stopped gracefully, the binary and two source files were
installed, and the restarted map server reported online without error/fatal
diagnostics in the checked startup log.

Previous binary and source backups remain under
`/app/rathena-deploy-backups/login-abi-repair-20260907-3IPhou` as
`map-server.before-reputation`, `clif.cpp.before-reputation`, and
`atcommand.cpp.before-go55`. This supersedes the earlier map binary pin.

## Depth 2 portals

The mansion entrance at `ba_maison,242,122` and the onward entrance at
`ba_chess,13,36` were present but used `HIDDEN_WARP_NPC`. Both now use the visible
`PORTAL` sprite and run the same access handler on click or touch. The existing
level, episode completion, reputation checks and Depth 2 buff removal remain
in place. There is no newly added direct route from Depth 1.

Both entrance cells were verified walkable against the live map cache. The GM
has Base Level 275, `ep17_2_main=36` and `RepPoints6=5000`, satisfying the gates.
The previous script was backed up as `varmundt_biosphere.txt.before-portals` in
the same repair directory, and the new script was installed during a graceful
map restart while the GM was offline. The map server returned online without
error/fatal diagnostics in the checked startup log. The strict integrity audit
also passed after the portal changes.
