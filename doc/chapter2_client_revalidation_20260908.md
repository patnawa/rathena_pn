# Chapter 2 installed-client revalidation

The current client was checked on 2026-09-08 after the prior Chapter 2, Druid
and item-metadata patches. The fresh audit reads the active `DATA.INI` and
installed patch archives, rather than assuming the original installation's
exact archive order or number of itemInfo imports.

## Geometry repair

All 17 Chapter 2 alias maps were compared cell by cell with the effective
server map cache. `mu_dun01`, `mu_dun02` and `rgs_dun1` each had four mismatches:
`(176,87)` through `(179,87)` were water/walkable in the server cache and walls
in the client GAT. Twelve server cells were corrected to walls. All other
pre-existing map records remain byte-identical; the independent Main Office
addition contributes three new records.

`tools/repair_chapter2_walkability.py` applies this bounded, idempotent repair
to a reviewed cache. It refuses unexpected dimensions or cell types. Back up
the cache and stop the map server before applying it to a deployment. No
client map assets need changing. The full 17-map walkability comparison passes
after the repair.

## Enchant and metadata checks

- All five Chapter 2 groups register successfully through the actual Lua 5.1
  helper: 22 targets and 58 recipes agree with the effective server definitions.
- Installed Shadow groups 70–88, 128 and 166 also match their current server
  recipes. Disabled reset prices are ignored on both sides because those
  reset operations cannot execute.
- The Chapter 2 generator's 18 tests pass with both the independent Lua parser
  and Lua interpreter enabled; no optional tests were skipped in that run.
- The installed native-enchant GRF retains SHA-256
  `09a60d6b3da391c7b45160338ccc887357ad836e680b6829426353fbfd160541`.

The full strict enchant-helper fixture still stops on three unrelated missing
authoritative itemInfo slot records: `401195 Frontier_R_Crown_AT`,
`510200 NP_B_Dagger` and `620064 SC_B_Axe`. These do not affect the scoped
Chapter 2 callback checks, but prevent claiming that this fixture validates
the complete enchant registry. The protected client's unknown-item handling
has not been observed here.

Earlier installation-receipt tests intentionally require their historical
archive order and exact loader additions. They reject the current client after
subsequent patches; those failures are not silently changed into passes. Use
their documented historical-loader options to reproduce the old receipt, and
the current audit for current resource verification:

```sh
python3 tools/ci/chapter2_current_client_audit.py
```

These are resource, recipe, Lua and geometry checks. They do not prove rendered
windows, live purchases, combat timing, or a completed player quest chain.
See the [runtime repairs](chapter2_runtime_audit_20260908.md) and
[reference coverage audit](chapter2_reference_audit_20260908.md) for those
separate scopes and remaining differences.
