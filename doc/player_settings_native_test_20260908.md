# Player Settings and Kill Counter native VM audit

The actual `npc/custom/player_commands.txt` function and event bodies execute in
the production script parser and VM. The harness freshly compiles current
`pc.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp`, and allocator sources with
AddressSanitizer and UndefinedBehaviorSanitizer, linking the remaining existing
Linux map-server objects. It does not start a server; socket access is denied.

Run from a Linux build checkout:

```sh
python3 tools/ci/player_settings_test.py
```

Result on 2026-09-08: **36 scenarios, 499 assertions passed**, no sanitizer error
or allocator leak. The runner caches compilation using source, compiler flags,
and header hashes; script bodies are read from the checkout on every execution.

Coverage includes character/account precedence, explicit Off versus unset,
repeated login dispatch, clearing overrides, cancel paths, every boolean setting,
and native input bounds for autoloot at -1, 0, 37, 100, and 101. Account inheritance
uses saved rows loaded into a second character's real native registry.

Kill Counter checks registration in the first/last and intermediate slots,
status and bare-command output, signed-32-bit maximum count display, isolated
and complete resets, replacement semantics, missing/excess arguments, malformed
and oversized IDs, unknown monsters, and invalid slots. Incrementing credited
kills is covered separately by `tools/ci/player_killcounter_test.py`; the VM test
also requires that the obsolete script kill event is absent to prevent counting
the same credited kill twice.

The VM audit found and fixed a parser defect: `checkre` is a one-argument Renewal
configuration query, not a regular-expression matcher. Slot validation now uses
length and numeric bounds. Monster IDs use a bounded digit-character scan before
numeric conversion and the existing native monster lookup. These builtins do not
require optional PCRE support.

Atcommand execution and UI/message transport are recorded boundaries. This test
proves exact explicit command dispatch, not native command permission checks or
network delivery. Real registry operations are exercised, but a char-server SQL
save/reload roundtrip is not. The separate native toggle tests and server startup
checks complement this harness.
