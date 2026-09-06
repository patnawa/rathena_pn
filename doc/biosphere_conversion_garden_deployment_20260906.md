# Biosphere conversion and Garden gate deployment

Deployed `2026-09-06T12:06:45Z` (19:06:45 Asia/Bangkok) to `192.168.10.18`,
`/app/rathena`. The guarded deployment returned zero. All four services were
independently verified running, not restarting, restart count zero, and ready.
The only post-restart diagnostic was the existing root-user warning.

## Exact runtime scope

Three source/NPC files and one map-server binary; no database definitions,
client files, account values, or other binaries are changed by this batch.

| Path | Previous raw SHA256 | Installed candidate raw SHA256 |
| --- | --- | --- |
| `src/map/script.cpp` | `6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d` | `035c218850b1b4ea4d906468ac96af380c36ef0226a279e4b62dd9cda0087fd1` |
| `npc/custom/varmundt_biosphere_depth.txt` | `790c47dd2deb4a46154c63983ca46cff60d34704824bdb3fc8c2a71f544d44df` | `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28` |
| `npc/re/quests/garden_of_time.txt` | `0505f6c6980642ef05c132652278ed42da06a294c977c00a4e44c2b94125431f` | `3e5a98758e70be050a0b61b3d23e3d0a3dae1c45e3729cc1988dd2c60fa0f301` |
| `map-server` | `2c796cf83da3468f781c71d67efd918839dbc0171de512580962d34d80583c39` | `778f8b77b14a2fe70e6da6eb1111ff002a76e03b05876e35cf04b79917e6fc59` |

The read-only `getinventoryslots` getter supports an exact, fresh output
capacity check. The existing Abyss Researcher at **ba_chess 22,16** retains all
five material recipes and costs, rejects stale/invalid input without payment,
and splits large complete material debits into native-safe chunks. Crown
services and their entire source prefix are unchanged.

Garden adopts its already reviewed live source into the project. Relative to
that live source only two active declarations change, `CLOAKED` to `DISABLED`,
closing the old entrance startup/reload gap. Custom Lake of Fire/Hall of Life
controllers remain active and unchanged. Story anchors, vending-machine reveal,
legacy bodies, and the two non-UTF-8 bytes are preserved; no encoding conversion
or legacy reward-policy change is performed.

## Verification

- StrictContent audit: 900 enabled NPC files, 99 instances, 56 route assertions;
  zero integrity errors and zero content-completeness warnings.
- Crown regression on the changed engine: 4,152 cases, 200,773 assertions,
  genuine old-source failure, ASan/UBSan clean and leak-free. The changed engine
  object was freshly compiled; other retained sanitized objects required exact
  source hashes.
- Conversion fresh six-production-unit proof: 225 cases, 49,497 assertions;
  eight original partial-payment failures reproduced (177 assertions), and two
  missing-player getter failure cases (51 assertions). All three processes are
  ASan/UBSan-clean and leak-free; seven zero-exit diagnostic controls rejected.
  Bound input payments and optional character lookup are included.
- Garden native declaration/reveal/click/unload/reparse: 20 cases and 132
  assertions passed; exact original source fails 68 hardening assertions.
  Both processes are sanitizer-clean and leak-free. The portable runner also
  passes without the untracked original snapshot and with an in-memory CRLF
  checkout; original full-source hashes and legacy bytes remain required.
- Default callback gate rejected 17 negative controls; explicit preserved-live
  profile rejected 21. Separate conversion content gate rejected nine controls,
  including changed achievements, weight-status scripts and map QuestInfo.
- Candidate map build succeeded with retained PACKETVER20260219 configuration.
  Only `script.cpp` required compilation; all three other server binaries are
  byte-identical to live. The candidate does **not** compile the separate
  preserved Suicide Bombing source delta.
- Candidate `--run-once` startup passed for project content and with actual live
  item/pet overlays mounted read-only. Both executed 3,610 OnInit NPCs and
  terminated cleanly without allocator leaks. The only diagnostic was the
  existing root-user warning. No old Garden snapshot was mounted over the new
  candidate.

These are scoped native and source/data checks, not a claim that all episode
gameplay, network scheduling, client rendering or multiplayer reward lifecycle
has been tested. Transport/world/persistence doubles and the source-audited
weight-notification boundary are explicit in the native audit documents.

## Preserved live changes and deployment safeguards

The item overlay, pet overlay and unbuilt Suicide Bombing source stay byte-exact
under the explicit `live-20260906` profile. Garden is now shared between project
and live profiles. All 3,688 reviewed source/content files remain covered; the
profile is never selected automatically after a failed default check.

The deployment script checks the exact archive, old/new runtime files, binary
hashes and complete expected live after-state before mutation. It creates a
scoped file/binary backup, stops login admission, requires zero online
characters, stops map/char/web, requires zero persisted bonus scripts, and takes
a database snapshot. Installation changes only the three source files plus map
binary; there is no SQL account mutation or automatic SQL restoration.

ERR/INT/TERM/HUP use scoped rollback. All four services must be verified stopped
before restoration; exact restored hashes and preserved source hashes must pass
before restart. A failure to stop or verify restoration leaves the services
unrestarted for explicit recovery. This path was source-reviewed and syntax-
checked; no failure was deliberately injected into the live server.

Actual deployment verified zero online characters after stopping login
admission and zero persisted `bonus_script` rows with services stopped. The
scoped backup is 34 MiB and database snapshot 22 MiB. Post-install checks matched
all 3,688 profile files, all three preserved source differences, the exact map
binary and the conversion-specific content gate. Login, character, map and web
services independently reported ready on ports 6900, 6121, 5121 and 8888.

Read-only post-restart SQL again showed zero online characters and zero
persisted bonus scripts. All fifteen existing GM reputation values remain at
their configured maximums: Chapter 2 300; both Episode 21 totals and six family
values 1000; Biosphere Depth 1/2 and Wolf 5000; Goblin/Isgard/Orc 3000. No GM or other
account values were modified by this deployment.

## Artifacts

Local sibling directory and matching remote directory:
`biosphere-conversion-garden-artifacts-20260906`, under `server-work` locally and
`/app/rathena-deploy-backups` remotely. Generated archives, SQL snapshots and
native executables are deliberately not committed.

- `runtime.tar.gz`: `e6e2287d8091b2224141b51e69c308b7f91b6be07236da8a5ffc5367bd6b3201`.
- `live-runtime-manifest.json`: `49daa2a88a9507ac532bbe61a9898080e083d1eff207b820b54589e2858db769`.
- `expected-live-callback-manifest.json`: `735e9d05873be6f460b148599d68252dca40a62ca2cf342dd6c9d06e8bff3475`.
- Deployment script: `170ff194d4fd7ccf1bbe2d5dc877fc397a0c330085489db5f49e4ea5d71607b3`.
- Deployment log: `85bc19c1eb9909a5a232dce5ed7144122be18cabe6e740f5f625aecf973ade74`.
- Scoped backup `pre-runtime-and-map.tar.gz`: `10d35a67ca12ca04e8374f5981bebdfc348f91179723a5c5727adcae406beb5b`.
- Database backup `pre-runtime-and-map.sql`: `65d7529c1183402b23e15c03e728326bc428323c18003a32e1d8be6189d997c9`.
- Crown native receipt: `f48fb07af7ff053eaeedc0508034b5f5d8087173a3e2072293db4ad935f45e67`.
- Conversion final native receipt: `00268ccadd47a4ceacbcae27fee2f60503162a3a2982e4447309e2a6d13ddeaf`.
- Garden portable native receipt: `ecb87e03911e42b99212ca2af7900b46923f061304b34c8d7c755b8f65129eb0`.

Git whitespace checking is clean outside the three existing whitespace-only
lines carried verbatim from the preserved Garden source (1051, 1064, 1104).
Those bytes are deliberately not cleaned as part of this two-flag live change.
