# Isolated native script-VM proof

This runs the real rAthena parser, script VM, player attachment, and
`modifyinventoryenchant` builtin. It is not the Python source-driven interpreter
used by the earlier progression tests, and it never starts a map/login/char
server or connects to production.

Run from the repository root in Linux/WSL after a local Linux map-server build
(requires PyYAML for the existing effective-Renewal import reader):

```sh
python3 tools/ci/run_native_script_vm_test.py
```

The runner compiles **current** `src/map/script.cpp`, `src/common/malloc.cpp`
and the test driver with `PACKETVER=20260219`, AddressSanitizer and
UndefinedBehaviorSanitizer into a temporary directory. Recovery on sanitizer
errors is disabled. `--sanitizer address` retains the earlier ASan-only mode.
The fresh allocator object resolves symbols before the common archive, so an
old allocator object cannot mask the current source. Existing
map-server objects/archives satisfy other link references; the normal server
`main` is replaced using the linker's test-only `--wrap` facility. No engine or
production NPC source is edited to install a test hook. Source SHA-256 values are
printed. `--build-dir /tmp/your-test-directory` retains generated objects/binary
when debugging is needed; no source/object files in the repository are rewritten.
The runner checks the four test records' type/subtype/slot counts against the
effective Renewal item imports before compiling, and refuses metadata drift.

The `.script` fixture in `npc/test/native_vm_inventory_fixture.script` is a raw
script body, not an NPC loader file, and is not added to any production config.
The runner requires its completion marker in addition to successful process exit.

## What executes, and what does not

| Boundary | Implementation |
| --- | --- |
| Script parsing, expressions, function dispatch, arguments, local variables, integer/string conversions, return values, attachment/detachment | Actual freshly compiled `script.cpp` |
| `modifyinventoryenchant` validation and mutation | Actual builtin and production inline `inventory_enchant_candidate`, using real `item` and `map_session_data` objects |
| Enchant lookup | Actual `item_db` container, populated with four small declared test records |
| Player/NPC lookup | Explicit in-memory test player; no world NPC |
| Enchant logs | Test callback records exact before/after item data and quantities; no SQL or log-table write |
| Client inventory refresh | Test callback checks exact arguments/counts; no packet transport or rendered client |
| Map registry persistence and NPC event queue | Explicit disabled test boundaries; no SQL initialization or queued world events |

The driver installs a mandatory Linux seccomp filter before initializing test
subsystems. It denies socket creation, connect, bind, and listen, and verifies all
four operations fail with `EPERM`. Failure to install the filter fails the test.
The synthetic account/character IDs are only fields in an in-memory object and
are never created in a database.

## Verified checkpoint

Thirteen actual builtin invocations pass:

- A valid Star of Spell Lv3 -> Lv4 mutation with unique ID `UINT64_MAX`.
- Rejection of a stale changed-slot snapshot, stale UID, overflowing UID,
  negative UID, trailing UID bytes, wrong item identity, unrelated stale card,
  physical card slot, negative inventory index, empty inventory index, missing
  enchant, and ordinary card used as an enchant.

Twenty-one native assertions additionally check byte-for-byte preservation of
every other item field, exact old/new log snapshots and order, exactly one
inventory refresh pair across all thirteen calls, and final player detachment.
The successful run reports no AddressSanitizer or UndefinedBehaviorSanitizer
error in the instrumented code and the rAthena allocator
reports no memory leaks.

This is a narrow real-VM proof, not a fresh whole-server build. Link-support
objects may predate the current source; other engine behaviors are not tested by
this executable. Inventory-capacity accounting, material/zeny charging, actual
NPC menus, client packets, equipment effects, relog persistence, instance combat,
and the other 99-instance gameplay paths still need integration/playthrough tests.

The initial UndefinedBehaviorSanitizer run failed on alignment violations in the
custom allocator during VM setup. A standalone fresh-source test confirmed the
fault; the allocator was repaired and both sanitizer modes now pass. See
[the allocator evidence and remaining limits](../../doc/native_allocator_alignment_audit.md).
This does not retroactively turn the historical ASan-only run into a UBSan pass,
nor instrument unrelated prebuilt support objects.
