# PN service release — 2026-09-08

Deployed to the authorized LAN server on 2026-09-08. The release contains the
three-floor Main Office, Rune Tablet NPC system and native bonuses, expanded
offensive/defensive battle-stat commands, Chapter 2 progression repairs and
the Master Shadow enchant menu. The accompanying audits identify remaining
reference and client coverage gaps; this is not a certification of every quest
or equipment effect.

## Delivered behavior

- `@office` opens the 50-desk hub; its directory searches services and moves
  players to verified approaches. Instance/escape restrictions remain enforced.
- Rune Tablet supports 57 sets, 120 collectible pieces and 136 imprints.
  Pieces and one-time rewards are shared within an RO login account; paid
  tablets, active selection, enhancement and pity belong to each character.
  The interface uses NPC dialogs, not the unsupported native Rune window.
- `@battlestats`/`@bs` and `@battlestats2`/`@bs2` report current offensive and
  defensive values. Detailed topics support twelve-row pagination.
- Chapter 2 preserves full-inventory Phantom credit, prevents reusing a story
  clear as a daily in the same instance, applies the intended ten-second groggy
  status, and rechecks the final story reward after the dialog yields.
- Master Shadow enchant groups 70–88 are reachable through the existing Shadow
  Enchanter, alongside 128 and 166. Existing recipe costs are unchanged.
- Twelve Chapter 2 cache cells were aligned with the installed client walls.

## Verification

| Scope | Evidence |
| --- | --- |
| Docker build | Fresh compile of login, character, map and web binaries, `PACKETVER=20260219`; final battle-stat source compiled and linked incrementally afterward |
| Rune transactions | 208 native cases, 2,840 assertions; actual VM/inventory paths with explicit world boundaries; ASan/UBSan clean |
| Rune data and effects | Seven catalog tests and five bonus/lifecycle tests; all 57 aggregate reward totals cross-checked; 1,831 item dependencies present |
| Office | All 50 desk approaches reachable with NPC cells blocked; asset/source preservation tests; 27 native cases, 208 assertions, ASan/UBSan clean |
| Shadow NPC | All 25 native dialog paths, including 19 Master groups and cancellation; 1,178 assertions; ASan/UBSan clean |
| Shadow group 128 | Nine focused checks, including 24 native distributions and 2.4 million draws |
| Battle statistics | Actual report formatter under ASan/UBSan, arithmetic/pagination/negative-value/read-only cases; whole translation unit compiled against actual headers |
| Chapter 2 | Eight source-driven progression regressions; all 17 map aliases compared cell by cell; 18 client patch tests with Lua parser and interpreter enabled |
| Installed enchants | Chapter 2's 22 targets/58 recipes match; all 21 Shadow enchant groups match current client/server recipes |
| Integrity | 913 enabled scripts, 100 instances, zero integrity errors or content warnings |
| Isolated startup | Final engine loads all scripts; acceptance resolves 50 Office desks, three entrances and 22 Chapter 2 NPC identities; no script errors |
| Live startup | Final login, character and map processes running; character connected to login, map connected to character; zero errors in final startup logs |

The isolated startup's empty roulette SQL table produced its known Apple-fill
warnings. It did not use the live database. Native test boundaries and
source-interpreter limitations are explained in the individual feature reports.

## Deployment and rollback record

Player count was zero before the map/character restart. Twenty-nine runtime
and source files were verified against the release manifest. The live Player
group contained additional existing commands; those were retained and only
`bs2` was added. The repository defaults enable both battle-stat commands.
No private database configuration or acceptance-test NPC was copied to live.

Server backup:

```text
/app/rathena-deploy-backups/pn-office-rune-battlestats-20260908T124156Z
```

It contains original files, the former map binary and import cache, new-file
list, release manifest, build/startup logs and receipt. Deployed map binary
SHA-256:

```text
a931c6d41ea759112a9eff118c81b6f197a3095936b6dbbf36ad0556eb7883a0d
```

Docker hostname addresses are cached by the server processes. Starting character
before the intentionally stopped login service initially caused connection
retries. Final startup used a running login service, then restarted character
and map to refresh their addresses. Both connections were verified afterward.
Use dependency order **login → character → map** on subsequent restarts.

The client patch `pn_office.grf` was installed above the existing seven archives
without reordering them. Its SHA-256 is
`15731977fe222b2f9cefafc634c057ce771a2600bfa9dbdcb4115f48f4f67feb`.
The client's original `DATA.INI` and receipt are under
`server-work/client-before-main-office-20260908T121931Z`.

Before rollback, move characters saved in Office maps to a supported town.
During a no-player maintenance window, restore the backed-up server files and
map binary, remove only files listed as newly introduced, and restore the import
cache. Start login, character and map in dependency order and verify their
connections. Restore the backed-up client archive priority only after characters
no longer require Office maps. Persistent Rune registry values are additive;
this release does not delete existing character or account records.

## Remaining verification and coverage

Rendered client appearance, complete quest playthroughs, live equipment purchases,
combat output and reconnect persistence still require in-game acceptance.
Chapter 2 is a compatibility adaptation with a condensed story, not a complete
copy of the linked guide. The Shadow audit retains 21 missing item-use recipes,
eight absent Druid skill pieces and acquisition/whitelist gaps. Three unrelated
itemInfo slot definitions still prevent the strict full-enchant-registry fixture
from completing. See the linked feature reports for exact IDs and evidence.
