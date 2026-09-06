# Actual Shadow Gear Enchanter dialogue VM test

This runs the unchanged `Shadow Gear Enchanter#grademk` body from
`npc/custom/grademk_services.txt` through the actual current `parse_script`,
`run_script`, and `run_script_main` implementation. Its `mes`, `next`, `select`,
`close`, `close2`, and `item_enchant` builtins execute in the real VM. It does not
reimplement or interpret the NPC in Python.

Run from the repository root on Linux/WSL, after a local Linux map-server build:

```sh
python3 tools/ci/run_native_shadow_service_test.py
```

The runner requires Python 3, PyYAML, g++, and the map-server support objects and
libraries. It freshly compiles `src/map/script.cpp`, `src/common/malloc.cpp`, and
`tools/ci/native_shadow_service_test.cpp` with `PACKETVER=20260219`, ASAN, UBSAN,
and sanitizer recovery disabled. Objects and the binary live in a temporary
directory; `--build-dir /tmp/shadow-vm-review` can preserve them for inspection.
Neither the production source nor the shared VM runner is edited.

## What passed

Four actual dialogue paths and 162 native assertions passed on 2026-09-06:

- **Cancel:** both real Next suspensions, the actual three-option menu, Cancel
  resolving to 2, a `CLOSE` pause, and termination after acknowledgement. No
  enchant window request.
- **Open:** both Next suspensions, menu selection 1, a `STOP` pause from `close2`,
  and execution after acknowledgement. The actual `item_enchant` builtin finds
  group 128 in the native container and calls the outbound-UI boundary exactly
  once with the attached player and group 128.
- **Escape:** the actual select cancellation value 255 terminates without
  entering either close branch and without requesting a window.
- **Alitea:** added menu selection 3 closes the dialogue before requesting group
  166 exactly once. Existing Open=1 and Cancel=2 positions are preserved. The
  message distinguishes ordered selectable Alitea enchants from group-128
  upgrades and explains that neither service has a reset.

At every dialogue pause and completion, the complete synthetic inventory is
byte-identical and zeny remains unchanged. Explicit payment and item-deletion
callbacks also confirm no script-side deduction was attempted. Dialogue output
contains the retention/downgrade and no-reset warning. Player script attachment
and the menu-wait flag are cleaned up. The instrumented code reports no sanitizer
error, and the rAthena allocator reports no memory leaks.

The verified source hashes are:

| Source | SHA-256 |
| --- | --- |
| `npc/custom/grademk_services.txt` | `95df0c2011b90dd1c7bf50f3a53b3ff93aa9bd5bda4af9f9e5278f97cdb9d436` |
| `src/map/script.cpp` | `6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d` |
| `src/common/malloc.cpp` | `064496e9722eeb1486178a6663aaa375ed7f312717986b9f41df3b69fd3a4a70` |

Each run prints fresh hashes and requires exactly one completion marker for
each path. A nonzero process exit, sanitizer failure, missing marker, missing
effective group 128/166, or synthetic inventory identity mismatch fails the runner.
Allocator warnings and sanitizer diagnostics fail even if process exit is zero.

## Exact boundaries and limitations

| Component | Tested implementation or explicit boundary |
| --- | --- |
| NPC body | Extracted verbatim from the current production file by balanced-brace scanning that ignores strings/comments; the complete body is parsed, including the unexecuted `OnInit` label |
| Parsing, expressions, menu resolution, pauses, continuation, builtin dispatch, player attachment | Fresh production `script.cpp` |
| `item_enchant` builtin | Fresh production builtin; native `item_enchant_db.exists` lookup executes for the selected group |
| Enchant DB contents | Minimal in-memory group-128/166 existence records; the runner first verifies that the effective Renewal imports contain group 128 with 14 targets and group 166 with four targets |
| Player and NPC lookup | One explicitly attached in-memory synthetic player; no world NPC or persisted account |
| Dialogue messages/buttons/menu | Outbound callbacks capture exact recipient and contents; no packets or rendered client |
| `@menu` write | Explicit transient-registry callback accepts only the real select builtin's `@menu` write and records its value; no persistence |
| Next/close acknowledgements | Driver resumes the real VM with the state transitions used by `npc_scriptcont`; the packet/proximity/world handler itself is not executed |
| `clif_enchantwindow_open` | Request callback only: records player/group and ordering; does **not** emulate its weight check, packet delivery, or `item_enchant_index` assignment |
| Payment/deletion APIs | Failure-returning callbacks record any unexpected invocation; resource snapshots independently detect changes |
| Map registry and event dequeue | Disabled explicit test boundaries, with no SQL initialization or queued world events |

The UI callback deliberately leaves `item_enchant_index` at zero and the test
asserts this. A successful group-128 **request** is not presented as a successful
client-window opening or activated enchant session.

Before initializing test subsystems, the executable installs a mandatory Linux
seccomp filter denying socket creation, connect, bind, and listen. It verifies
all four operations fail with `EPERM`; filter installation failure aborts the
test. Normal map/login/char startup is never invoked. Synthetic account and
character numbers exist only in memory.

Existing support objects satisfy other link dependencies and may predate the
current source. This is not a fresh whole-server integration build. Native YAML
parsing, NPC spawning/placement and OnInit titles, client packet routing, weight
rejection, actual enchant charging/outcomes, acquisition, relog persistence, and
live player access are not proven by this test. The separate group-128 recipe
regression checks client equality and the compiled upgrade selector; neither
test replaces an actual client playthrough.
