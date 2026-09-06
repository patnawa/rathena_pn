# Ghost Ship: once-per-character voyage reports

2026-09-06. This pass fixes reuse of one cleared instance for multiple expedition
reports. It does not yet change eligibility for characters arriving after the
captain dies; that separate policy question was presented to the user.

## Evidence and repair

The active entrance helper reuses an existing matching solo/party instance.
At stage 22, the finish NPC previously accepted any active story or daily quest.
A character could finish the story, obtain the daily quest, re-enter the same
cleared ship and obtain a daily completion without another battle. Keeping an
instance across the daily reset allowed the same repeated use on later days.

The project's [MuhRO reference](https://wiki.muhro.eu/Mysterious_Ghost_Ship)
describes a daily expedition with a captain battle followed by its reward.
It does not specify late-arrival enrollment or a per-character participation
cutoff, so this fix does not invent one as purported official behavior.

`Maristella#ep21gs_finish` now stores successful report claimants in the
instance-local `'gs_reported` array. The entries are character IDs, not account,
session, party or persistent instance-ID markers. A character can receive one
story **or** daily completion per instance lifetime. Successful `changequest`
postconditions are checked before adding the claimant; no roster mutation follows
a failed transition. No yield occurs between the check, transition and recording.

Repeated claimants receive an explanation and can still leave. Other characters
keep their independent claims. The NPC is not globally hidden and the instance
is not automatically destroyed while party members need their reports. Only the
owner's existing close/create controls are mentioned; no new reset operation or
automatic party-wide relocation is introduced.

Nillem's capacity checks, item/experience/reputation amounts, report quests and
04:00 daily-period calculation are unchanged. Claiming the in-instance report
does not award items itself. A full inventory at Nillem leaves the existing
pending report available for later collection.

## Native verification

`tools/ci/ghost_ship_report_claim_test.py` extracts the actual current finish and
Nillem NPC bodies without rewriting. It reuses the explicit isolated boundaries
and build routine from `episode21_encounter_flow_test.py`, freshly compiling
`script.cpp`, `quest.cpp`, `malloc.cpp` and its own generated driver with
ASan/UBSan. Kernel seccomp denies socket, connect, bind and listen. The native
parser, VM, `getcharid`, instance registers, `inarray`, `getarraysize`, assignments,
quest operations and dialogue pauses execute in production code.

Result: **24 cases, 99 assertions, zero failures/errors**, clean sanitizer and
allocator teardown. Coverage includes all stages 0-21 rejecting early completion,
an unquested visitor not consuming a claim, story-to-daily replay, changed session
identity with the same character, another character's separate claim, reconnect
semantics, a fresh instance reusing the same numeric ID, inventory-capacity
rejection at the daily report, exact unchanged rewards, daily-reset replay and
two characters suspending at `close2` before their acknowledgements.

The exact source at Git `51c8195170e9d7018e3b2d10315c2dd8ab3e6c31`, SHA256
`24bbfe0d1b6351c396ccd3477e7e29c343948afc5efa90f5b6808aa89fe14a4c`,
reproduces **12 assertion failures** in the same 24 cases/99 assertions, with zero
parser/quest errors and clean teardown. This negative result establishes that
the test detects the old behavior, not merely a self-consistent new fixture.

```sh
python3 -B tools/ci/ghost_ship_report_claim_test.py --build-dir ../ghost-ship-report-native-20260906
python3 -B tools/ci/ghost_ship_report_claim_test.py --pre-fix --prepare-only --build-dir ../ghost-ship-report-before-20260906
../ghost-ship-report-native-20260906/episode21_encounter_flow_test ../ghost-ship-report-before-20260906
```

The last command intentionally exits 1. A fresh negative build is also available
using `--pre-fix` without `--prepare-only`. Run under WSL/Linux from the repository.

Exact boundaries: world movement, NPC visibility, outbound packets, registry
persistence, capacity, rewards and access/time helpers are explicit doubles.
The daily period is a controlled input, not a test of wall-clock/timezone code.
Later daily quest acceptance is represented by native `quest_add`; the entrance
conversation is not executed by this report-only test. Reconnect tests preserve
the character identity while changing the synthetic session identity; no network
relogin occurs. Fresh-run cleanup executes the same register/array destruction
operations used by `instance_destroy`, not its complete map/NPC/timer machinery.
No live accounts, SQL or game process are used by this harness.

## Existing encounter regression maintenance

The previous 43-case encounter suite still runs current source and passes all
252 assertions. Its hash-exact pre-fix reconstruction now starts from the pinned
repaired checkpoint, so separately audited later changes cannot corrupt the
historical comparison. Its spawn/travel/reward invariant remains active everywhere
except exactly `EP21_EnterGimli`, whose new entrant routing has its own native and
map-cell tests. The suite now frees native instance-array metadata between cases,
matching the production cleanup, and rejects allocator/runtime/sanitizer warnings
even if the process exits zero. No prior behavioral case was removed.

Live visual verification, actual instance teardown/reconnect, persistent quest
saving, and the separately requested late-arrival policy remain outside this
bounded proof. The broad episode audit is not complete.
