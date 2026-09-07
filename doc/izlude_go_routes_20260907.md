# Izlude services and @go route validation

MSCESXi's saved town was `izlude_a`, while the custom Warper and Healer were
only placed in `izlude`. Added uniquely named duplicates to `izlude_a` through
`izlude_d`, retaining the original town services and their existing behavior:

- Healer: 121,150.
- Warper: 134,150.

## Command repairs

The 61 numbered destinations (0-60) are retained. Exact configured map names
now override legacy prefix aliases. This fixes `jor_albe`, `jor_base`,
`jor_crk`, `jor_tail`, and other exact names such as `e_tower`. The longer
`isgardairship` alias no longer selects Ice Castle through the earlier
`isgard` prefix. Unknown names no longer become Prontera through `atoi`,
and malformed/out-of-range numeric arguments are rejected.

The help includes Service Mall (60) and no longer advertises negative memo
destinations, which this handler does not support. This is not an expansion
of @go to every game map. Use the Warper's existing routes for additional
destinations; no quest gates, map permissions, or instance ownership rules
were removed.

## Evidence

`python3 tools/ci/go_routes_test.py` (WSL/Linux, g++ required) checks:

- All 61 map-cache destinations, including nonempty walkable maps for the two
  random-coordinate entries.
- All ten Warper/Healer placements across the five Izlude variants.
- Help coverage for every number 0-60.
- 134 native C++ assertions compiled from the actual command declarations and
  name-resolution body: all numeric routes, all canonical map names, malformed
  input, and selected ambiguous aliases.

The strict episode integrity audit passed: 900 enabled scripts, 300 literal
warp destinations, 57 route regression assertions, zero errors and zero content
warnings. These are source/cache checks, not 300 client-rendered playthroughs.

The map server was built in an isolated copy using the live Docker image and
existing PACKETVER 20260219 configuration. Binary SHA-256:
`89910c7d89d42d816ec676746993e987cbe5025a7cdfda7e386ff40a4a55b916`.
No shared inter-server structures were changed. Deployment backs up the previous
map binary, two NPC scripts, command source and help under
`/app/rathena-deploy-backups/izlude-go-20260907/`.
