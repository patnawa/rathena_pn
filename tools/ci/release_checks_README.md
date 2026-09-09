# Release validation

The `PN release validation` workflow runs on relevant changes to `main`, pull
requests, and manual dispatch. It builds Renewal, loads the configured active NPCs
and databases on a disposable GitHub runner, and runs the focused regressions.
It does not enable dormant NPC scripts or connect to the production server.

Linux dependencies: Python 3 with PyYAML, GCC/G++ with AddressSanitizer and UBSan,
make, MySQL client development headers/libraries, zlib, PCRE, and OpenSSL.
The full gate additionally requires the current candidate's `make map` objects,
common and third-party archives. Build using
packet version `20260219`, as used by the Chapter 1 native VM fixture.

Quick source checks (YAML syntax and focused regressions):

```sh
python3 tools/ci/release_checks.py --phase source --report /tmp/source-checks.json
```

Before deployment, build the candidate in a separate directory with disposable
SQL databases and isolated ports/network. Run the following only inside that
isolated candidate directory; the runner launches its map server itself:

```sh
python3 tools/ci/release_checks.py --startup-log /tmp/candidate-startup.log --report /tmp/release-checks.json
```

Only continue deployment when the command succeeds and the full report says
`passed: true`. The runner rejects missing readiness markers, server errors,
fatal errors, and sanitizer failures even if map-server exits successfully.
The startup log is overwritten by this invocation. The runner fingerprints the
candidate source, database, NPC, configuration, tools, client patches, and map executable before
validation, rejects file changes during validation, and records both candidate
and startup-log hashes in the report. Keep build artifacts and reports outside
those fingerprinted directories. Keep the build, startup log, report, and
deployment manifest together. Any subsequent candidate change requires a new run.

The full gate includes native status reload packet boundaries, 256 RODEX states,
storage/composer transitions, equipment bonus boundaries, deterministic rental
durations, Aquila patch targeting/timing, equipment upgrade acquisition and
recipe agreement, and 20 Chapter 1 protection cases. Database YAML syntax alone does
not establish schema or script correctness; native map-server loading supplies
that additional check. Neither suite substitutes for player-client acceptance
tests of quest completion, combat timing, or persisted character state.

The gameplay extension adds 25 reform transaction cases, 110,000 refine outcomes,
episode party progression, backup failure handling, native instance admission and
reward claims, Episode 21 dialogue races, and entrant checkpoint recovery. Native
instance tests use the real script VM with explicit world/network boundaries;
they are not a complete player-client playthrough. Their pre-fix modes reproduce
the repaired failures without depending on Git history.
