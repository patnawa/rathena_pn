# Native gameplay acceptance

28 September 2026, combined build 3. The test ran only against disposable labelled Docker containers on an internal network, using synthetic account 99000031 / character 99000032. All production gameplay NPC/data loaded successfully; all four native binaries reached readiness. Full post-test logs contain no Error, Fatal, runtime-error or segmentation-fault diagnostic.

`doc/evidence/muhro_gameplay_20260928/native-report.json` pins the four binary SHA-256 hashes. Remote complete logs: `/app/pn-muhro-zeny-20260928/gameplay-startup-055913/`.

The native packet driver passed:

- Warp Portal's modern packet lists the save point plus all six memo slots; selecting the sixth memo transports the character from Prontera to Alberta.
- The actual missionary rescue function completes all three quest records out of order, and repeat rescue is harmless.
- Actual skill reset clears mission/reward state and extra memo records; the subsequent list contains save point plus three memos.
- A paid trap consumes one material and returns one on removal. A trap placed using a rental requires no material, and removal after native rental expiry returns no material.
- Actual Master Refiner dialogue consumes one weapon certificate and refines equipped Club 1501 to +15.
- After logout, SQL records contain +15 equipment, no consumed certificate or expired rental, and only three memo rows. The driver polls for stock asynchronous logout/save completion rather than assuming a fixed two-second delay.

Harness: `tools/ci/muhro_gameplay_startup.py`, `tools/ci/muhro_gameplay_live_client.py`, `npc/test/muhro_gameplay_fixture.txt`. The test NPC is mounted only into the isolated fixture's custom script list and is not added to the production configuration. Fixture commands reject every other account/character/name. The startup script refuses existing disposable container/network names, overwrites SQL endpoints with synthetic credentials, publishes no host ports, and removes its own containers/network on completion.

The rescue fixture establishes native rescue and reset semantics; it does not simulate all 200 monster kills or visually render the protected client. Full mission ordering and transaction guards also have separate production-body tests. This acceptance adds no new gameplay behavior and does not establish complete MuhRO latest-ten feature parity.

The six-file gameplay client overlay is staged at `../gameplay-client-release/payload` in the work batch. Lua 5.1 loaded all 26,920 existing/new item definitions and 11,473 quest definitions, including four catalyst items and fifteen missionary descriptions. No installed client file was changed by this validation work.
