# Project bug hunt — 22 September 2026

The audit fixes and authorized server deployment are complete on the development server. All seven live containers are healthy, and the running map executable matches the tested candidate. This pass covered custom NPC dialogue and inventory operations, native storage commands, offline audit runners, client integrity checks, and health/release tooling. Existing unrelated untracked work was preserved.

## Confirmed defects and fixes

| Area | Failure and consequence | Fix and regression evidence |
| --- | --- | --- |
| Gold Point Manager | `1:1` inside a `select` string added an extra option. About and Leave were routed incorrectly. | Use delimiter-free label text. Execute the actual NPC in the script VM, including Exchange, About, Leave, and close paths. |
| Costume enchant recovery | `Slot N:` added a second option per enchant, so the selected menu index could name the wrong slot. | Use a dash in each label. Verify all five recoverable headgear/garment slot choices through the actual VM. |
| Costume exchange | Fashion Points were credited without checking whether the inventory debit succeeded. | Require successful `delitemidx` before crediting points. Exercise both debit success and failure. |
| Storage cleanup | `@clearstorage` and `@cleargstorage` iterated occupied-item count as though it were the last array index. Items after holes survived. | Scan the complete native storage arrays, skipping empty slots, including retained items above reduced capacity. Both original loops independently fail the new native regression. |
| GRF integrity audit | A corrupt classic GRF payload could pass when its compressed and expanded sizes matched, because the v3 stored-data fallback also applied to v2. | Permit the fallback only for GRF v3. Verify invalid v2 data and valid stored/compressed v3 fixtures. |
| Lua item audit | Registering one item twice could satisfy the item count while another item was never registered. | Reject duplicate item IDs during real Lua 5.1 registration. The original implementation fails the new regression. |
| Health reporting | Missing Docker `StartedAt` aborted the report; truthy strings/numbers could count as successful backups; nonfinite thresholds could disable age checks. | Return a failed service check for malformed metadata, require Boolean success flags, and reject NaN/infinite thresholds before service inspection. Repair outdated test state fixtures and verify restart-aware log windows. |
| Bug-hunt identity | Supplied client files were fingerprinted only initially. Identity errors could leave evidence unfinished. | Capture and compare both server and client inputs at the beginning and end; preserve identity failures and completed results. |
| Bug-hunt execution | Timeouts left descendants running, repeated areas overwrote logs, and inherited `PYTHONOPTIMIZE` disabled assertion-based checks. | Kill the fixture process group on timeout/interruption, deduplicate areas, and clear optimization from the fixture environment. Real subprocess regressions reproduce the original failures. |
| Regression integration | Environment-based Lua tests did not receive `--lua`; quality-service fixtures used invalid GRF headers. | Forward and record `LUA51`, repair fixtures, add the new tests to applicable bug-hunt/release suites, and install Lua 5.1 in release CI. |
| Release gate follow-up | The release runner still inherited assertion-disabling optimization, and a timeout could omit the active check from its evidence. | Keep assertions enabled, log each command, write reports atomically before/after execution, record timeouts/interruption/dependency failures, and terminate descendant processes. Preserve the existing stop-on-failure behavior. |

Primary changes: [Fashion NPCs](../npc/custom/fashion_points/FashionPoints.txt), [storage commands](../src/map/atcommand.cpp), [bug-hunt runner](../tools/ci/bug_hunt.py), [client audit](../tools/client_release_audit.py), and [health checker](../tools/admin/health_check.py).

## Verification

Evidence paths below refer to the retained `project-audit-20260922` directory in the local development workspace. Raw logs, server receipts, and backup locations are kept outside this source repository. The complete operational report is retained there as `REPORT.md`.

| Check | Result |
| --- | --- |
| Broad offline bug hunt | All 25 selected checks passed across transactions, party progression, recovery, combat, and release. Initial/final source and binary identities match. |
| Fashion NPCs, real script VM | 11 paths / 289 assertions passed under GCC ASan/UBSan, with clean allocator teardown. Original HEAD and each independently reintroduced defect were rejected. |
| Native storage cleanup | 18 cases passed under GCC ASan/UBSan; original personal and guild loops each failed. |
| Bug-hunt runner regressions | 10 tests passed on WSL, including actual descendant termination and optimization-environment handling. |
| Client audit regressions | 12 tests passed with the installed Lua 5.1 executable; no Lua tests skipped. |
| Health-check regressions | 8 tests passed, including nine nonfinite-threshold argument combinations. |
| Client preflight / archive-stack regressions | 6 + 6 tests passed. |
| Release builder and mocked deployment controllers | 16 tests passed with `PN_CLIENT_PACKAGES_ROOT` pointing at the retained package workspace. |
| Database backup helper | 3 tests passed on WSL using disposable subprocess fixtures. |
| Quality-service integration | Passed on WSL, including temporary guarded-deployment fixtures. |
| Installed client integrity / Lua loader | 10 GRFs, 242,188 verified payloads, zero archive errors or encrypted/unverified payloads, and 26,916 registered items. |

The installed-client local evidence `client-integrity.json` also records 1,116 overrides/collisions and 2,569 missing-icon candidates. These are review candidates, not demonstrated rendered failures: the audit does not establish loose-file/fallback behavior, asset availability in gameplay, or which items are obtainable.

The broad run completed at 10:44:55 UTC on 22 September 2026. Its local evidence `offline/dashboard.md`, local evidence `offline/report.json`, individual logs, and initial/final identities are retained in the workspace. `git diff --check` passed.

## Server follow-up

A clean candidate was built separately from the running server. The production runtime image was pinned to `sha256:cae30d441d1c6e8f784be1d5525ea0573d9cd680dd21baf077e85908ccfe531b`.

Deployment completed at **11:07:47 UTC / 18:07:47 Bangkok on 22 September 2026**. The installed changes are the map executable, `src/map/atcommand.cpp`, the live-preserving Fashion NPC file, and `tools/admin/health_check.py`. Development audit/release tooling and CI improvements remain in the source checkout and validated candidate. The existing login, character, and web binaries were retained. No schema migration or client install was required.

There were no online players at preflight or restart verification. The controller took a restore-verified SQL backup before installation, checked exact source preimages, replaced files atomically, and verified the running map executable through `/proc`. All **34 financial/storage groups** matched across shutdown, installation, and restart, including wallets, inventory, account/character registries, storage pages, bank/reserve receipts, and mail. Production startup recorded zero errors, and the final health report passed every check. local evidence `deployment-report.json` · local evidence `final-health.json`.

Rollback originals, ownership/modes, and the restore-verified SQL backup are retained in the operator workspace. The deployment controller refuses to overwrite an existing rollback directory. Any later code rollback must preserve current player data; the operational runbook records the exact restore procedure.

The deployment controller also passed three offline tests for rollback behavior and startup-diagnostic handling; a failed stop must be followed by a successful stopped-state read-back before any rollback file writes or restarts.

- All four servers built from zero existing objects with Renewal and packet version `20260219`. There were zero compiler warnings/errors, and all 3,123 native source inputs remained stable. local evidence `followup-build/server-build.json`.
- Fresh login, character, map, and web processes completed startup and the game-server handshake using a disposable database loaded from 21 schema files on a private Docker network. Startup diagnostics were clean. local evidence `startup-evidence/report.json`.
- The exact live NPC candidate preserves existing wording while applying only the three reviewed fixes. Its script VM proof passed 11 cases / 289 assertions and rejected all three reintroduced defects. local evidence `live-fashion-native.log`.
- All 35 source release checks passed, including syntax validation of 311 YAML files. Lua-dependent audit fixtures were separately tested with Windows Lua 5.1; the WSL source run skips those fixtures when `LUA51` is unset. local evidence `release-source/report.json`.
- Focused checks against the production candidate passed: 18 native storage cases, 8 health tests, 10 bug-hunt tests, and 13 release-runner tests. local evidence `candidate-checks-evidence/report.json`.

The native fixtures exercise production functions with explicit world, persistence, UI, or inventory boundaries. This pass did not perform a new SQL crash-durability exercise or interactive rendered gameplay acceptance. GitHub CI itself was not invoked. Client-resource triage did not establish a justified replacement for the missing-icon candidates, so no speculative client assets were installed. local evidence `client-resource-triage.json`.

## Reproduce

From the repository under Linux/WSL:

```sh
python3 tools/ci/bug_hunt.py --output ../new-bughunt-results \
  --area transactions --area party --area recovery --area combat --area release
python3 tools/ci/npc_audit_fashion_test.py
LUA51=/path/to/lua5.1 python3 tools/ci/client_release_audit_test.py
```

The Fashion VM test requires local Linux map support objects and libraries. The standalone storage and ordinary bug-hunt fixtures use temporary build directories. See [the runner guide](../tools/ci/bug_hunt_README.md) for installed-client checks and native options.
