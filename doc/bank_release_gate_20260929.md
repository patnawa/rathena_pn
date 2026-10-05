# Bank fixtures and release gate — 29 September 2026

The full release gate passed **52 of 52 checks**, closing the bank-service
fixture blocker from the four-area audit. Changes remain local; no server
deployment or client publication was performed.

## Repairs

`tools/ci/bank_service_test.cpp` now models the current bank UI dependencies:
transaction-pending states, partner/trade metadata, native wallet refresh,
32-bit packet reads, and explicit trade/offline-collection dispatch boundaries.
Existing authentication, fragmentation, malformed packets, balance limits,
duplicate/stale requests, commit acknowledgment, retry, and legacy exchange
rejection checks remain intact. New checks exercise pending-state precedence,
HUD suppression, reciprocal partner identity, routing/replay behavior, and
offline-collection dispatch without changing balances before persistence.

The first full-gate attempt then exposed the same interface drift in
`tools/ci/rodex_operation_test.py`. It now includes mail and paired-transaction
pending states and extracts the actual pending/locked production predicates.
The original 256 RODEX state combinations remain; new checks cover all 16
pending masks, internal applying states, blocked storage opening, and reopening
after pending flags clear.

Both repaired fixtures pass with their configured sanitizers. The bank test
uses AddressSanitizer and UBSan; the RODEX test uses AddressSanitizer. Trade and
offline-collection persistence remain explicit doubles in the bank fixture;
these checks do not certify those separate persistence implementations.

## Validation environment and evidence

Evidence is retained in `Server-Development/bank-release-20260929` and the
isolated host directory `/app/pn-bank-release-20260929`. The candidate contains
the previous four-area repairs and current local source, tools, databases,
NPCs, client fixtures, and SQL documentation assets.

The older Alpine toolchain failed to link even a trivial AddressSanitizer
program. Validation uses Ubuntu 24.04 / GCC 13 with functioning ASan and UBSan,
plus Lua 5.1 explicitly selected through `LUA51`. The complete map object and
archive set was rebuilt for this toolchain using packet version `20260219` and
`--without-pcre`, matching the native fixtures' link requirements. Sanitizers
were not disabled or substituted.

The full gate uses an internal Docker network and freshly initialized disposable
SQL database. All database configuration points at the disposable database.
Candidate reports and logs are outside fingerprinted source directories.
The gate itself must verify a stable candidate digest and isolated startup.

## Final results

- 314 database YAML files parsed; all 49 configured regression suites passed.
- Client compatibility asset validation and isolated map startup passed.
- The startup exited zero, reported readiness, and contained no gate-detected
  errors. The candidate fingerprint remained unchanged across the gate.
- All 5,371 source/data/test files checked match the local workspace; the six
  generated source Makefiles are excluded from that cross-platform comparison.
- The build is a fresh map-server build with its native object/archive set,
  not a new four-server release package.

Final evidence: `bank-release-20260929/gate-1790677898/release.json`, its
`release-logs/` directory, `startup.log`, and `runner.log`. The parent evidence
directory also contains `build.json`, `build.log`, `local-candidate-match.json`,
the original bank compiler failure, and the first gate's RODEX compiler failure.
The separate 42-suite diagnostic run passed but is not used as a substitute
for the final uninterrupted gate.

| Artifact | SHA-256 |
| --- | --- |
| Gate candidate | `85305f5a8ff93d5578f50b79271ddd4d7ca5ea6ead9c74af9733caf1dcb05e6e` |
| Map executable | `18f790b45ccb31b27b1efeb73b2c6452cbabe499a36ca2195be49238b1d27640` |
| Startup log | `4568166ae503ab72b7ef1e8bc9130d3ce5a806d96f1e6f36bf77285037811f5e` |

The next audit remains transaction persistence: SQL stock-write failures,
multi-output delivery, asynchronous pet creation, and output callbacks. The
gate pass does not close those broader behavioral coverage gaps.
