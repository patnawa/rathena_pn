# Player command coverage — 8 September 2026

This is an implementation audit against the [MuhRO player-command reference](https://wiki.muhro.eu/Player_Commands),
not a claim that PN implements MuhRO's private server. Command names below identify
compatibility targets; behavior findings come from this repository's native handlers
and script bindings.

## Existing functionality now available to ordinary players

Group 0 explicitly enables the following native commands. The command itself and
its existing aliases use the same permission check. No GM level, administrative
permissions, other-character commands or broad command inheritance were granted.

| Area | Commands | PN behavior |
| --- | --- | --- |
| Discovery | `commands`, `help` | Enumerates permitted native commands and their available help. |
| Information | `rates`, `uptime`, `exp`, `jailtime`, `hominfo`, `homstats`, `showdelay`, `servertime` | Native current values and session toggles. Server time has the `time` alias. |
| Database | `iteminfo`, `mobinfo`, `whodrops`, `whereis` | Existing native reports. Aliases `ii`, `mi`, `wd`, `wi`. Spawn reports concern regular registered spawns. |
| Travel | `go`, `load` | Existing town/save-point warps with native map and state restrictions. `return` aliases `load`. |
| Character | `kill`, `refresh`, `ksprotection`, `noask`, `showexp`, `showzeny` | Self death (`die`), view refresh, personal KS protection (`noks`), invitation and gain-display toggles. |
| Stats | `bs`, `bs2` | PN offensive/defensive reports with topic and page selection. Existing `battlestats` aliases preserved. |
| Loot | `autoloot`, `autolootitem`, `autoloottype` | Native percentage/item/type inclusion settings. Aliases `al`, `alootid`, `aloottype`. One item-list operation per invocation. |
| Loot profiles | `alootconfig`, `alc`, `alootset`, `als` | Ten named account-wide slots for rate/item/type snapshots. Loading applies to the current session; no blacklist or login-scope suffixes. See [loot profile details](account_loot_profiles_20260908.md). |
| Social | `channel`, `langtype`, `me`, `pettalk`, `homtalk`, `party` | Native channel permissions, enabled message language, roleplay messages and party creation. |
| Vending | `autotrade` | Offline vending remains subject to native vending, map and server restrictions; alias `at`. |
| Guild | `changegm`, `breakguild` | Existing guild-master checks; leader transfer also checks membership, maps, WoE and cooldown configuration. |
| Duel | `duel`, `accept`, `reject`, `leave` | Native consent-based duel flow. |
| Navigation | `navi`, `navi2` | Validates map coordinates and requests client directions; the second command opens the navigation window. No teleport. |
| Inventory | `unequipall`, `clearfav` | Normal unequip restrictions; favorite clearing affects current inventory only. |
| Trigger report | `autospells` | Lists currently calculated automatic-spell bonus entries, trigger conditions and base rates. |

The previous `changedress` and item-backed `resurrect` access is preserved.
`office`, `activity` and `ch1rewards` are existing script bindings and do not require
native command grants. Script binding discovery may differ from the native
`commands` listing.

## Semantic mismatches deliberately not disguised

| Native name | Source finding | Disposition |
| --- | --- | --- |
| `allowks` | Toggles the entire map's KS policy. | Not granted to players; personal protection uses `noks`. |
| `invite` | Invites to a duel, not a party. | Not granted as a party-invitation shortcut. |
| `guild` | Temporarily disables the Emperium requirement. | Not granted; normal client guild creation remains available. |
| `storeall` | Opens storage, unequips equipment and transfers everything it can. | Not exposed as a filtered storage command. |
| `stockall`, `dropall` | Existing bulk operations do not provide the requested full filtering/favorite contract. | Not newly granted. |
| `alootid` | Inclusion list only; no pipe parser or separate exclusion list. | Help documents supported syntax. |
| `langtype` | Native language selection is not a master-account settings hierarchy. | Native behavior documented; no fabricated persistence claim. |

## Dependencies still required for remaining reference commands

Names in this table have no matching complete implementation in the audited
baseline. They must not be advertised merely by adding aliases or group grants.
The integration report records any subsequently completed implementations.

| Area | Reference names | Missing dependency |
| --- | --- | --- |
| Server/event state | `cooldown`, `eventinfo`, `woeseinfo`, `ltp`, `utc`, `mobinforate`, `mobinfomode`, `epstatus`, `spellbook` | Native state reporting or specific event/quest integration. |
| Market | `whobuy`, `wb`, `whobuy2`, `wb2`, `whobuys2`, `whosell`, `ws`, `whosell2`, `ws2`, `whosells2`, `whichnpcsell`, `wn`, `shop`, `market`, `marketkill`, `shopjump`, `sj`, `kickmyvendors`, `kmv` | Shop indexes, filtered search, clone vending, account ownership and safe teleport integration. |
| Personal controls | `bsc`, `lastwarp`, `lw`, `ping`, `daynight`, `lockjob`, `setrates`, `bank`, `garmentlayer`, `hateffect`, `hold`, `noattack`, `hidemobshout` | Correct state hooks and/or protocol handling. `bs` is not silently presented as a replacement for `bsc`. |
| Visual | `hideunits`, `hideunit`, `hideplayer`, `hidepet`, `hidepets`, `hidepet2`, `silentpet`, `silentpets`, `hidewarg`, `hidemount`, `unmount`, `hideabr`, `madomode`, `hidebionic`, `hidehom`, `hideelem`, `hideoutfit`, `hideoutfits`, `hidehateffect`, `hidehateffects`, `expandstyle`, `hideflamegiant`, `noresize` | Per-viewer visibility and appearance/client features. Global hide/disguise commands are not suitable substitutes. |
| Loot/storage | `nodrop`, `nolootid`, `restock`, `getall`, `noshare`, `nosellall` | Inventory acquisition/transfer hooks and exclusion-list storage. |
| Guild/BG | `guildinvite`, `guildskill`, `guildskillinfo`, `leader`, `reportafk`, `order`, `listenbg`, `bgrank`, `joinbg` | Party/guild/BG membership checks, queue/ranking and leadership systems. |
| Events | `dicebet`, `acceptdb`, `declinedb`, `resetdb`, `treasureradar`, `tr` | Specific transactional event scripts and state. |

The integrated script commands `settings`, `killcounter` and `kc` provide the
Settings desk and five persistent character monster counters. Settings covers
autoloot plus four message/invitation toggles at character and RO-account scope;
see [release behavior and test boundaries](combat_commands_release_20260908.md).
`hideconfig` is not implemented. Other reference visual toggles and persistence
scopes remain dependencies, not aliases for unrelated native operations.
Master-account sharing requires an actual verified account-link model; RO account
and character settings alone do not implement that scope. Client `/greymap` and
Discord integrations require their own client/bot deployment.

## Validation

```sh
python3 tools/ci/player_command_permissions_test.py
```

The test rejects duplicate YAML keys, unresolved native grants, unexpected default
player permissions, and the specific mismatched/admin handlers above. It checks
exact alias targets. Deployment-specific `conf/import/groups.yml` and
`conf/import/atcommands.yml` still require review because imports can override
checked-in defaults. Native startup and actual player smoke tests remain separate
from this configuration check.
