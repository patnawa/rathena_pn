# Reputation parser follow-up — source findings only

2026-09-06. Read-only follow-up while the separate Depth document exchange
release was frozen. No engine edit, generated-client output, native parser
regression or deployment is claimed here.

The reviewed `src/map/pc.cpp` raw SHA-256 is
`1c4dee142c4a34d7bb8065e37c5dc6558f0ff2ad52205756d44481c08e764403`.
`ReputationDatabase::parseBodyNode` at line 347 assigns `INT64_MIN` to a new
row's omitted `Maximum`. The schema at `db/reputation.yml:29` specifies
`INT64_MAX`. The minimal default correction is that constant replacement.
Existing-row overrides already retain limits whose fields are omitted.

The current native-selected import graph has 13 effective reputation entries.
All explicitly provide Name, Variable, Minimum and Maximum, with valid ordered
bounds and no `INT64_MIN` endpoint. Thus **zero current entries are affected**
by the omitted-Maximum defect. Generator visibility-only overrides retain
their existing limits. This is not a reason to declare future definitions safe.

Additional concerns require separate explicit tests:

- The parser has no `Minimum <= Maximum` validation. Existing objects are
  mutated as each field is parsed, so a later rejection can leave an earlier
  field changed. Any validation repair should stage the full candidate record
  and commit it only after success.
- New rows require `Variable`, although the schema documents a `RepPoints<id>`
  default. Do not silently combine a change to variable identity with the
  one-line limit correction.
- Under `MAP_GENERATOR`, `pc_reputation_generate` applies `std::abs` to both
  signed endpoints at lines 471–472. `INT64_MIN` has no representable signed
  positive magnitude. Making the upper default correct does not make the
  already documented lower default generator-safe. The bundled BSON writer
  also cannot represent the unsigned magnitude 2^63 as a signed BSON integer.
  A follow-up should reject unrepresentable export bounds before opening or
  writing output files; no invented client sentinel or economic clamp is
  justified by the inspected code.

Proposed bounded native proof: new-row defaults and each omitted field;
existing-row partial overrides retaining untouched values; equal/inverted,
malformed and out-of-int64 bounds; unchanged object state after refusal;
all 13 current definitions and visibility overrides; and representable BSON
round trips plus controlled unrepresentable-bound rejection. These are planned
cases, **not executed results**. The server binary and all runtime files remain
unchanged by this follow-up.
