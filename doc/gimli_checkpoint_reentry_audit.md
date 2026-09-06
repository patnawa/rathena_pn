# Gimli entrant-only checkpoint recovery

Verified locally on 2026-09-06. This bounded change modifies only the successful
path of `EP21_EnterGimli` in `npc/custom/episode21/GimliInfiltration.txt`.
It does not change encounter progression, rewards, party-wide movements, entry
eligibility, instance creation policy, timers, or monster definitions.

## Defect and minimal recovery

The external entrance calls `instance_enter` with coordinates `-1,-1`. The actual
engine resolves those coordinates to instance 147's database entrance,
`1@mdtem,266,174`, even after the party advances. Existing route NPCs disable
themselves after transporting the group. The sole inter-map portal at
`1@mdtem,130,36` also disables itself when stage 7 advances to stage 8. Re-entry
therefore cannot simply repeat the already-consumed route interactions.

After a successful native entry, the helper reads the current stage from that
explicit instance ID and warps only the attached entrant to the last group
destination that was already unlocked. These are existing destinations, not new
combat shortcuts:

| Current stage | Entrant destination | Existing transition establishing destination |
| --- | --- | --- |
| 0–1 | `1@mdtem,266,174` | Instance database entrance |
| 2–4 | `1@mdtem,170,154` | `Tan#ep21gimli_start`, stage 1 → 2 |
| 5–7 | `1@mdtem,121,36` | `Escaping Believer#ep21gimli_1`, stage 4 → 5 |
| 8–10 | `2@mdtem,140,125` | `#EP21_GimliPortal`, stage 7 → 8 |
| 11–12 | `2@mdtem,60,125` | `Temple Guard#ep21gimli_inner`, stage 10 → 11 |
| 14–15 | `1@mdtem,80,80` | `Suspicious Place#ep21gimli_b`, stage 12 → 14 |
| 17–18 | `2@mdtem,60,65` | `Wilhelm#ep21gimli_move`, stage 15 → 17 |
| 20–22 | `1@mdtem,252,67` | `Tan#ep21gimli_move`, stage 18 → 20 |

Stages 13, 16, and 19 are not assigned by the script. They and all out-of-range
values preserve the database entrance, as does an unavailable checkpoint map.
No recovery branch writes the stage or wave latch, triggers an event, enables an
NPC, spawns a monster, or moves another character. The original combat gates
remain authoritative, including stage 7's portal and the final stage 20/21 waves.
Recovery does not record or require earlier individual participation: the existing
party entry policy is preserved and the route follows the party's current progress.

## Engine and data evidence

`src/map/script.cpp::buildin_instance_enter` passes the supplied character ID and
explicit instance ID into `src/map/instance.cpp::instance_enter`. The latter
validates ownership, party membership, BUSY state, instance database identity,
entry-map resolution, and successful initial positioning before returning IE_OK.
The new instance-register read occurs only after that success.

`buildin_getinstancevar` returns a reference to the selected instance's register
database. `buildin_instance_mapname` with an explicit instance ID invokes native
`instance_mapid` and generated clone-name lookup, returning an empty string on
failure. Supplying the ID matters: the attached entrance NPC is outside the
instance, and implicit NPC instance context is zero. The final `warp` builtin
targets only the attached player. The VM's ordinary `end` builtin itself sends a
close packet; the regression distinguishes that from a suspended CLOSE error
dialogue instead of treating every close packet as a failure.

Effective `db/import/instance_db.yml` record 147 retains the name, entrance, and
`2@mdtem` additional map. `db/import/quest_db.yml` contains the connected story
states 17764–17769. The level-230/active-17764/not-completed external entry checks
and the report chain are unchanged, protected by whole-file reconstruction.
They are not newly exercised by the entry-helper VM fixture.

## Native regression and map-cell proof

`tools/ci/gimli_checkpoint_reentry_test.py` freshly compiles `script.cpp`,
`instance.cpp`, `map.cpp`, `malloc.cpp`, and its generated driver with ASan+UBSan
and no sanitizer recovery/suppression. It executes the actual complete extracted
helper through `parse_script`/`run_script`, without replacing script builtins.
Native instance ownership validation, instance-register references, cloned-map
resolution, `warp`, cache decompression/cell conversion, and `CELL_CHKPASS` run.

The positive run passed **66 cases / 904 assertions / zero failures / zero
native errors**, with explicit `Memory manager: No memory leaks found.` output.
The runner requires that message and rejects warning/error, allocator corruption,
ASan, UBSan, and runtime-error diagnostics even if process exit status is zero.

Cases cover every stage 0–22 plus -999, -1, 23, 99, and INT32_MAX for both a leader
and ordinary member; no party; member without an instance; creation failure;
wrong instance; non-ready instance; wrong owner; missing entrance map; failed
initial movement; successful new-instance creation boundary; and an unavailable
checkpoint map. Assertions protect exact movement counts and coordinates,
entrant identity, the other member's position, stage/wave/other-instance registers,
inventory, and zeny.

Independent binary parsing selects the first effective record in native cache
priority order: import, renewal, base. All seven exact checkpoints **and** the
entrance are explicitly enumerated; this does not depend on a literal-warp regex.
Every one is GAT 0 and passes actual native `CELL_CHKPASS`. Source map dimensions
are 300×300 (`1@mdtem`) and 200×200 (`2@mdtem`); the native last-row/column exclusion
is also respected. Each movement request repeats the native cell check.

Both effective records are from `db/map_cache.dat`:

| Evidence | SHA-256 |
| --- | --- |
| Base cache | `3d523caa567fdb6330c062824085bb19e1268940bec82fe360c1f14060e5b508` |
| Exact `1@mdtem` record header + compressed payload | `6e4584616875ee99fc4592c7617f648ead385a018fbe8b64d82e94c5b082b2fb` |
| Exact `2@mdtem` record header + compressed payload | `4ea22bebceb82d78e30f95a70c1151ef1382a06c3c97b934cb4f49267492de78` |

The hash-exact previous helper, run in the **same positive native executable**,
produced **66 cases / 832 assertions / 72 expected checkpoint failures / zero
native errors**, with leak-free shutdown. Both missing movement and wrong final
position are detected for each of the 18 advanced reachable stages and two actors.
Failure handling and entrance-fallback tests still pass on the previous helper.

## Reproduction and boundaries

From the repository under Linux/WSL with the local native support objects built:

```sh
python3 tools/ci/gimli_checkpoint_reentry_test.py --build-dir ../gimli-checkpoint-native-20260906
python3 tools/ci/gimli_checkpoint_reentry_test.py --pre-fix --prepare-only --build-dir ../gimli-checkpoint-before-20260906
../gimli-checkpoint-native-20260906/gimli_checkpoint_reentry_test ../gimli-checkpoint-before-20260906/entry_helper.script
```

The last command intentionally exits 1 for the old-source regression. Each
preparation reconstructs and SHA-checks the **entire** previous Gimli file before
extracting the helper, preserving every byte outside the marked recovery block
(allowing the original CRLF representation).

| Source snapshot | SHA-256 |
| --- | --- |
| Previous Gimli at checkpoint `51c819517` | `f0c554a83ae877ea0fcd8e76cddda5fb987b7363c47fec26874c6cb499245ff6` |
| Corrected Gimli | `1b719604835239f32a6e09e5d4367540fafa4fe487a9a13bce0fa3c3c9d64293` |
| Fresh `script.cpp` used for reported result | `6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d` |
| Fresh `instance.cpp` used for reported result | `d85d818ef07b63603ef4c27da17cd0708f2787ccc8f063f4f1cd2867aa8efc95` |
| Fresh `map.cpp` used for reported result | `265b60e0e1a88e3d3b0f3fbdde48e8bb72e585dd6d0c1b1d33a4a37ab5513f00` |
| Fresh `malloc.cpp` used for reported result | `064496e9722eeb1486178a6663aaa375ed7f312717986b9f41df3b69fd3a4a70` |

Player/party/NPC and map-index lookups, final `pc_setpos`, actual map creation,
and outbound UI are explicit doubles. Creation success is a service boundary,
not a claim of real instance creation. The exact decoded source geometry is
shared into the fixture's clone maps; dynamic NPC/skill-wall state is not loaded.
Other native support objects are linked from the local map-server build and are
not all sanitizer-instrumented. Kernel seccomp denies socket/connect/bind/listen;
no real player, server connection, SQL service, or native-client UI is used.

### Separately discovered existing cache-reader alignment defect

The first attempt to feed full cache files into freshly sanitized `map.cpp`
stopped at `map_readfromcache`, line 3682 in the recorded snapshot. Its
`map_cache_map_info*` cast assumes 4-byte alignment, but compressed record lengths
leave subsequent record headers unaligned. The first observed failure was renewal
cache `izlude` at offset 2615 (offset modulo 4 = 3), after `alberta` with compressed
length 2587. The base cache likewise has `alb2trea` at offset 551 modulo 4 = 3.
This is independent of the NPC change and was reported for a separate engine fix.
The map-cache tool reads into aligned stack-local structs, not that pointer cast.

For this bounded NPC proof, the Python reader preserves the exact selected
20-byte record header and compressed payload in an aligned, one-record cache
envelope. Native decompression, geometry conversion, and collision checks then
run unchanged under both sanitizers; no bytes of map geometry are invented or
replaced and no diagnostic is suppressed. This is **not** a passing test of the
original whole-cache reader. The base cache's historical file-size field is also
stale (3056818 versus actual 3110866 bytes); the native reader uses map count and
record lengths, which the independent reader follows. No cache or engine file
was changed by this task.
