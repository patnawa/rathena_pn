# Runtime responsiveness and purchase metrics

The common timer dispatcher emits one `PN_METRICS` JSON record per process approximately every minute. It contains no account, character, request or item identifiers. Histograms use a fixed 13-bin array; recording observations does not allocate memory. Percentiles are bucket upper bounds capped by the observed maximum, not exact quantiles. Histogram observations reset each report; purchase counters are process-lifetime totals.

Measurements:

The shop measurements describe end-to-end scheduling and resolution. They are not exact wire counters or database round-trip timings.

- Timer lateness uses the dispatcher's existing due-time comparison; dispatcher duration samples elapsed time at the end of `do_timer`.
- Shop acknowledgement latency starts when the purchase joins the FIFO queue and ends at its first final committed/rejected result; completion latency also includes successful stock-cache refresh when required. Queue waiting time is included. An admission-capacity rejection before submission is a local final result and also contributes to these histograms, so acknowledgement counts are not exact SQL round-trip counts. Retry responses do not count as final results.
- An unresolved purchase keeps its pending age across reporting windows. Failed refreshes and retry timer attempts are counted. Retry attempts include queued entries still waiting for their turn and do not count only packets sent. A metric never unlocks, refunds, resubmits a changed payload or otherwise changes transaction outcomes.
- `shop_pending` is zero or one for the active queue head. Schema version 2 adds `shop_queue_depth`, the number of queued purchases including that head, bounded from zero to 32. `shop_oldest_ms` is the head's age since enqueue, including any earlier waiting time. `shop_started` advances when a purchase becomes the active head.
- `shop_busy_refusals` counts the instrumented shared-entry refusals when the queue is full; it is not a count of every possible client refusal.

The health checker requires a fresh map-server heartbeat after a 120-second startup grace. A heartbeat older than 150 seconds fails readiness even if Docker still reports the process running. Its recent trend retains at most 15 samples.

`--max-shop-pending-seconds` defaults to 30 as an operational unresolved-purchase threshold. `--max-timer-p99-ms` is optional: choose it after recording representative idle and combat/load baselines. Missing or malformed metrics cannot fall back to an older green sample. The health checker accepts the exact version 1 schema and the version 2 schema so an existing server remains readable while its checker is updated. Version 2 validates the queue bound and that a nonempty queue has an active head. Deploy the updated health script before or together with the version 2 map binary.

Portable validation executes one million observations and reports timing, tests lifecycle/percentile behavior, and injects a stalled purchase into the health checker. That timing is a fixture observation, not a production overhead or capacity claim. A 30-minute idle/load baseline and real-process stall acceptance remain required in the improvement delivery ledger.

`tools/admin/runtime_baseline.py --candidate /path/to/built/candidate --evidence /new/evidence/directory`
runs a disposable map process and MariaDB on an internal Docker network with no
published ports. It copies the input tree before starting, records source and
binary bindings, and removes its containers/network on completion. Allow about
70 minutes. The default pinned database image and validation toolchain must
already be available on the host. Budget 2 GiB for map and 768 MiB for SQL.

The idle phase includes one control-table lookup per second. The bounded load
phase adds 500 script arithmetic iterations and a 20 ms synchronous SQL query
per second. Each phase lasts at least 30 minutes plus a reporting interval.
The report records the maximum per-window p95/p99 bucket bounds; these are not
aggregate percentiles. It then stops the actual map process for 165 seconds,
checks stale-heartbeat rejection using the production health parser, resumes
it and checks recovery. Finally, real five-second SQL sleeps must appear in
dispatcher maximum latency.

This fixture has no connected character server, players or purchases. Expected
character connection errors do not invalidate heartbeat collection, but neither
does successful collection certify clean NPC startup, player combat capacity,
or purchase latency under load. Those require separate acceptance evidence.
The scenario is an operational baseline of bounded NPC/SQL dispatch, not a
substitute for representative gameplay. Run it again against the final candidate;
an earlier binary's report cannot certify a later release.

`tools/ci/shop_load_live_client.py` adds connected-player purchase acceptance in
the labelled disposable improvement realm. It uses native market packets from
one buyer and four concurrent buyers at one attempted purchase per player per
second. Wire success must agree with persisted wallets, inventory, stock and
durable receipt count. Its SQL-delay phase holds a real receipt insert for 75
seconds and requires an unhealthy pending-operation signal followed by recovery.
It records exact observed acknowledgement quantiles and receipt payload growth;
the server's separate histograms retain their bucket-bound semantics. This is a
bounded shopping workload, not a combat or maximum-capacity benchmark. All three
shopping cases are mandatory alongside the longer operational baseline in the
release evidence contract. The fixture file must never enter production NPC
configuration.

### Paced real-player market baseline

`tools/ci/shop_load_live_client.py` accepts `--single-seconds` and
`--four-seconds`; defaults remain 125 and 245 for short debug runs. Each accepts
1–1800 integer seconds, with a combined maximum 1998 seconds to bound added
inventory below 2000 potions per fresh buyer including the delayed transaction.
For final acceptance use `--single-seconds 125 --four-seconds 1800` with the
existing `--realm` and `--evidence` paths, inside the guarded disposable game
network namespace. Durations are recorded in the receipt. Existing heartbeat,
per-account inventory/wallet deltas, stock delta and successful receipt-count
checks remain active. SQL-delay injection follows the normal measured phases.

This is a real-player market workload paced at one attempt per buyer per second,
with four buyers contending for one market. Report successes and refusals
separately. It is distinct from `runtime_baseline.py`'s idle and bounded NPC/SQL
phases. For concurrent final runs, clone the same frozen source/binary binding
into independent namespaces; record cohost activity and effective fixture
hashes, and do not present the resulting latency as uncontended capacity.
