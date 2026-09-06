# Episode 21 checkpoint and report deployment

Deployed to `192.168.10.18`, `/app/rathena`, on 2026-09-06. Service start
timestamp: `2026-09-06T09:54:11Z` (16:54:11 Asia/Bangkok). The deployment
script returned zero after observing map-server readiness.

## Exact runtime scope

Only these two NPC files changed. No binaries, SQL records, client files,
import roots, or equipment drafts were deployed in this batch.

| File | Deployed SHA-256 |
| --- | --- |
| `npc/custom/episode21/GimliInfiltration.txt` | `1b719604835239f32a6e09e5d4367540fafa4fe487a9a13bce0fa3c3c9d64293` |
| `npc/custom/episode21/MysteriousGhostShip.txt` | `b6baf2dfdb5d2bc47a67a4a8f31b002eb46fbec2a9f66de963047c18e250b70d` |

The reviewed archive was `episode21-reentry-claims-reviewed-20260906.tar.gz`,
SHA-256 `cf737d8800acd893e637039a02f7f8aa439b10990070c6bb9b44fcf6b54b99ac`.
The before/after manifest is based on Git checkpoint
`51c8195170e9d7018e3b2d10315c2dd8ab3e6c31`. Live before-state, isolated candidate
after-state, and deployed after-state all passed both file checks. The final
live after-state check was repeated independently after deployment.

Gimli restores the attached entrant to an already-unlocked party checkpoint.
Ghost Ship records one successful story or daily report per character per
instance lifetime. The late-arrival eligibility policy remains unchanged and
awaits the user's separate choice.

## Verification

| Native regression | Cases | Assertions | Failures/errors |
| --- | ---: | ---: | ---: |
| Gimli checkpoint entry and native map-cell checks | 66 | 904 | 0 |
| Ghost Ship report claims | 24 | 99 | 0 |
| Existing three-instance encounter suite | 43 | 252 | 0 |

Each suite freshly compiled its declared production units with ASan/UBSan,
denied network operations, and confirmed leak-free allocator teardown. The
hash-exact old sources produced 72, 12, and 89 expected assertion failures,
respectively. See the individual audit documents for test doubles and limits.

The isolated candidate's map-server `--run-once` loaded the scripts, executed
3,609 `OnInit` NPCs, and exited cleanly. There were no parser/runtime errors;
existing root-user and container-orchestration notices remain.

No characters were online after login admission was stopped. All four services
were stopped before the database snapshot and restarted after the scoped copy.
Post-deployment inspection confirmed `running=true`, `restarting=false`, and
restart count zero for login, char, map, and web. Logs confirmed login/web
readiness and the map-to-char handshake with `Map Server is now online`.
Error/fatal/sanitizer/allocator scans of all four startup logs found only the
existing warning about running as root.

The whole local working-tree strict audit was **not** clean at this checkpoint:
the concurrent equipment task had two intentionally unwired draft import
fragments. Those files were neither candidate-tested nor deployed in this batch.
No NPC error was reported by that scan.

## Recovery artifacts

All artifacts below remain outside Git under `/app/rathena-deploy-backups/`:

- `pre-episode21-reentry-claims-20260906.tar.gz`: previous two NPC scripts.
- `pre-episode21-reentry-claims-20260906.sql`: full `ragnarok` database snapshot
  taken with the game services stopped; approximately 22 MiB.
- `episode21-reentry-claims-reviewed-manifest-20260906.json` and reviewed archive.
- `deploy-episode21-reentry-claims-20260906.sh` and deployment log
  `episode21-reentry-claims-deploy-20260906.log`.
- Candidate log `episode21-reentry-claims-startup-20260906.log`.

The script had a scoped automatic file rollback on deployment failure; it did
not run. No database restoration or unrelated checkout reset occurred.

This is not live gameplay/visual validation, full persistent relog/instance
teardown validation, or completion of the broad episode audit. A separately
discovered unaligned map-cache reader remains assigned for an engine repair;
the Gimli test's exact aligned-record geometry proof is not represented as a
passing full-cache parser test.
