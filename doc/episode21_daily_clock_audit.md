# Episode 21 daily-period clock repair

2026-09-06. Scope is the shared `EP21_DailyKey` function in
`npc/custom/episode21/MysteriousGhostShip.txt`. The repair changes only how the
existing 04:00 reset key is calculated. Quest IDs, rewards, reputation amounts,
cooldowns, dialogue, maps and the callers' payment order are unchanged.

## Finding and runtime behavior

The original helper captured `gettimetick(2)` and then read hour, minute and
second through three independent `gettime` calls. `gettimetick(2)` and each
component ultimately call `time(nullptr)` separately. When a second, minute,
hour or day boundary occurred between those reads, the helper could construct a
key that did not represent any one instant. Two services reached across the same
reset could therefore compare different period keys.

The repaired helper captures one timestamp and derives all three components
from `gettimestr("%H%M%S",7,.@now)`. The current 04:00-local reset formula and
the existing fixed 86400-second previous-day fallback remain intact. This is a
single-clock-snapshot repair, not a reset-time or timezone-policy change.

The live `rathena-map` container reported `UTC +0000` on 2026-09-06. Its 04:00
local reset therefore occurs at 11:00 in Bangkok. The deployment deliberately
preserves that schedule. No DST-policy change is claimed, and the script
engine's signed 32-bit timestamp horizon is outside this repair.

## Native verification

`tools/ci/episode21_daily_clock_test.py` reconstructs the exact complete former
NPC source by reversing only the reviewed helper replacement. It extracts the
actual old and repaired function bodies, then executes them in the real script
VM with native `gettimetick`, `gettime`, `gettimestr`, `atoi`, and a freshly
sanitized `src/map/date.cpp`. Only the `time()` syscall is deterministic while
the script runs. World, transport and transient result storage remain explicit
fixture boundaries; no server, database or network operation is performed.

The proof exhausts every second of a complete 2026-09-06 day in both the live
container's UTC timezone and Asia/Bangkok. It also checks three calendar dates,
thirteen reset/calendar boundary seconds and four possible clock-read split
positions in UTC, Bangkok and Kathmandu. The original helper must remain correct
under stable reads and must reproduce its mixed-read failures. The repaired
helper must make exactly one native clock read per invocation.

The final binary links retained ASan/UBSan production objects from the accepted
Depth document proof, except that `date.cpp` and this test driver are freshly
compiled. Reuse checks every retained source, header, object and archive byte.
The sole source-only exception reconstructs exactly the two former broad-gate
NPC digest strings from their newly reviewed values before comparing the entire
old Python file hash. It cannot relax any compiled C++ source binding. Nine
zero-exit output controls, four frozen-artifact controls and four retained-gate
mutation controls are rejected.

Run from the repository root in WSL:

```sh
python3 -B tools/ci/episode21_daily_clock_test.py \
  --native-build-dir ../episode21-daily-clock-utc-final2-20260906 \
  --retained-document-build ../biosphere-document-native-final-20260906 \
  --source-mode fixed
```

## Verification status

PASS in `../episode21-daily-clock-utc-final2-20260906`:

- 173268 repaired cases and 867714 total assertions.
- 117 stable-read controls preserve the former correct behavior.
- 45 mixed-read controls reproduce the genuine original defects.
- Clean allocator teardown and empty stderr; no ASan/UBSan diagnostic.
- Kernel-denied networking.
- All 1660 current source hashes, 72 frozen artifacts and 66 linked inputs were
  independently rechecked after the run.

| Artifact or source | SHA-256 |
| --- | --- |
| Final `receipt.json` | `4443c8826b2023e646813c13bba80c4d2f482b88a8bb5e7a02ef9f9e89186d15` |
| Native executable | `06df1baa81ed503dd6530df4f333dc8c5034a0afb95a2caac09aef2ddd52f181` |
| Full former NPC, LF-normalized | `b6baf2dfdb5d2bc47a67a4a8f31b002eb46fbec2a9f66de963047c18e250b70d` |
| Full repaired NPC, raw and LF-normalized | `58c336e56c135390cf47f98526f598433ec87f3384213ed6f3f4f46bba229965` |
| Fresh `src/map/date.cpp` | `9cfa51180be76065af0849b95707a1661e28273fcef90cdea0e51dca49ca0f25` |

After the Episode 20/21 QuestInfo and Family transaction installation, the
same complete native matrix was rerun against the final source and repinned
broad callback manifest. It again passed 173,268 fixed cases, 117 stable
controls, 45 original defects, and 867,714 assertions. The current receipt is
`../episode21-daily-clock-label-v2-20260907/receipt.json`, SHA-256
`bdf9288a91f73b223025cb5f6b5bb980470e9ef4f5ca27c9ff302bfe3831288e`;
the current executable SHA-256 is
`feb5703b4e7e1f1b6801196a4e3447e4a333e60fc1726911cb3ccd126ed96b7d`.

## Live deployment

The exact one-file archive was first applied to
`/app/rathena-audit-candidate-20260906`. Both the project-data and preserved-live
startup variants parsed with exactly 3610 `OnInit` NPCs, clean allocator
teardown and only rAthena's known root-user warning. The complete callback gate
and the Final Battle scoped gate passed on that candidate.

Deployment completed at `2026-09-06T15:34:12Z` (22:34:12 Bangkok) from
`/app/rathena-deploy-backups/daily-clock-artifacts-20260906`. Before installation,
the gate confirmed zero online characters, zero persisted bonus scripts and the
valid `RepPoints6` range. All four services were stopped, the former NPC and a
database snapshot were retained outside Git, and only the Ghost Ship NPC was
replaced. The live-profile callback gates then passed before service restart.

Post-startup evidence confirms all four services running, not restarting, with
restart count zero; the map server is online with exactly 3610 startup NPCs and
no unexpected diagnostic. The map binary remains
`63e554b5829bce76d50b0edb1dacd84ff6bc200393c3f43d821647d05ea7d98a`.
The live NPC is the repaired `58c336e5...` source, the container remains UTC,
and a fresh database read again found zero online characters, zero persisted
bonus scripts and `RepPoints6=5000` for the existing GM record.

The release-check receipt is
`56c1e9b4bcb5a70e7b8f2faf24b36c676b9ea129cb1d6abb325487530d859055`;
the deployed archive is
`715d1c2c30b36c234d03c3f62f0e51b9f36680b4fabe7354a71df54aef15d673`.
The recoverable pre-NPC archive is `dd2f01db...`; the SQL snapshot remains only
on the server. This deployment proves source installation and startup health,
not graphical client interaction or a real player crossing the reset boundary.

## Cumulative Episode release deployment — 2026-09-07

The clock repair was redeployed as part of the final eleven-file Episode 20/21
set in the exact r7 release. Live readback matched current
`MysteriousGhostShip.txt` SHA-256 `011f74d5...`, which includes the clock repair
plus the reviewed QuestInfo migration. Candidate and live POST startup passed,
and guarded SQL remained identical across the service restart. See the
[combined deployment receipt](episode20_21_gudra_healer_deployment_20260907.md).
