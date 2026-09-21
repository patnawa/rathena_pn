# Account bank audit and deployment — 2026-09-21

The bank SQL handler now checks new transaction arithmetic and original balances on receipt retries. Bank/reserve include dependencies were repaired in the source templates and the live generated Makefiles. The fixes are deployed on 192.168.10.18; all seven containers passed health checks, and financial row hashes were unchanged across deployment.

Verification passed: 500,000 randomized arithmetic cases, expanded production-handler sanitizer tests, 77 actual bank SQL assertions, 31 reserve SQL assertions, real database crash recovery, 15 complete login/character/map scenarios, three native Windows bank suites, and installed Lua/item-art checks. The SQL validation gap was reproduced against the previous production objects before the fix. It was an internal validation gap; a player-facing exploit was not demonstrated.

The complete audit, evidence and rollback instructions are retained in the deployment workspace at `Server-Development/bank-audit-20260921/REPORT.md`. The remote evidence and rollback directory is `/app/pn-bank-audit-20260921`; the source candidate is `/app/rathena-builds/bank-audit-20260921`. These deployment artifacts are outside this Git repository.

The map server remains authoritative for inventory eligibility and capacity. SQL permits positive wallet rewards received while a save is pending; a committed receipt retry acknowledges the operation without applying its older snapshot. Automated checks found no further balance-loss or duplication defects in the covered scenarios, but do not prove the absence of all bugs. Manual rendered gameplay acceptance was not performed.

The character binary changed without an ABI or schema migration. The map binary retains SHA-256 `5e4f65318c7763cbb041879f1147368e42eca0e2048d725c69f308097e5b9381`. The deployed character binary is `c1c7232d9fea62abd3d31129c5131cb65270b01bc2719c53e40e956c0671a97b`. Preserve current player SQL data when rolling code back.
