# Scoped release evidence

`tools/ci/release_bundle.py` assembles evidence for explicitly named scopes. Its exit
status is nonzero when a required capability, scenario, input or artifact is
missing or different. The JSON output remains available on failure. This is an
integrity check over receipts, not an independent observer of a client or a
production database.

The supported scopes match the improvement ledger: `market`, `pets`, `release`,
`barter`, `guide`, `content`, `metrics`, `lab`, `npc-fixes`, and `client-fixes`.
The release controller derives mandatory scopes by comparing the entire candidate
input inventory (including untracked/import files and deletions) with an immutable
Git baseline. Explicit scopes may add requirements; omitting an inferred scope is
an error. Shared engine, dependency and SQL changes conservatively require every
scope. NPC/database changes require content proof, with additional named scopes
for known services. Unknown input categories require all scopes. Baseline line-ending
differences can broaden scope; they never suppress a requirement.

Use `release_controller.py baseline --repository REPO --ref BASE_COMMIT --output
/evidence/baseline.json` to export the baseline. The baseline must identify the
previous deployed source; selecting the correct deployed baseline remains an
operator responsibility. Its inventory hash detects subsequent alteration but
is not a signature proving where that baseline was deployed. Then use
`release_controller.py check --candidate CANDIDATE --baseline /evidence/baseline.json
--evidence native=... --evidence sql=... --output /evidence/candidate.json`. Omit
`--scope` to use all inferred scopes. The lower-level `release_bundle.py` remains
available for partial diagnostic checks; it does not authorize deployment.

Example for a purchase-only candidate, from a stable built staging directory:

```sh
python3 tools/ci/release_bundle.py --candidate /staging/rathena \
  --scope release --scope barter \
  --evidence native=/evidence/native/report.json \
  --evidence sql=/evidence/sql/report.json \
  --output /evidence/purchase-bundle.json
```

Reports produced before the binding fields were added are intentionally rejected.
Rerun their original harnesses; do not add a new binding to an old result. Native
and SQL harnesses record their binding before execution and verify it afterward.
The bundle requires the current full native check inventory, isolated startup,
the map binary, and source identity. SQL recovery also binds the character binary
and every object/header input recorded by the SQL harness.

Source identity includes server code, bundled dependencies, build entry points,
database definitions, NPCs, tools, client overlays and workflows. Generated object
files and Python caches are excluded; SQL object hashes are checked separately.
Every reported binary hash must match. These hashes establish what was checked;
clean compilation is still needed to establish correspondence between source and
binaries. Separate CI jobs with different build inputs are useful checks, but
their results are not interchangeable with tests of the final staging candidate.

Configuration hashes are recorded separately because SQL/startup fixtures use
isolated credentials. Do not treat them as proof that production configuration is
valid. The final deployment receipt must include `configuration_sha256` matching
the declared deployment candidate. The release controller must record/review the
fixture-to-deployment configuration changes and verify the actual target state.
Only hashes, never configuration contents, belong in these receipts.

Additional receipts (`rendered`, `responsiveness`, supplemental `sql`, and
`deployment`) have this structure:

```json
{
  "passed": true,
  "binding": {"source_sha256": "...", "binaries": {"map-server": "...", "char-server": "..."}},
  "cases": [
    {"name": "onboarding", "status": "passed", "artifacts": {"onboarding.png": "sha256..."}}
  ]
}
```

Capture binding before running the scenario; check that it is unchanged afterward.
Artifact paths are relative to the receipt directory and must remain within it.
Cases are distinct, named requirements, not a total test count. Missing, failed
and pending cases never satisfy a requirement. Rendered proof requires actual
screenshots and observations from the client. A textual packet log is not rendered
proof even though the bundle cannot semantically distinguish the two. The artifact
hash is an integrity check, not a substitute for review. The native and shop SQL
gates require original runner reports and cannot be replaced by generic cases.

For responsiveness, both `idle-baseline` and `load-baseline` need at least 1,800
seconds each, plus `process-stall` evidence. The receipt producer must verify the
actual measurements and load definition; writing a duration does not perform a
baseline experiment. Supplemental SQL cases can be added to the original SQL
report by a controller only after executing their corresponding harnesses against
the same stable candidate, preserving the original report as a hashed artifact.

`--stage deployed` additionally requires a deployment receipt with backup restore,
actual deployed hashes, health, data invariants and remote Git verification cases.
Without it, even a green candidate bundle makes no deployment claim. The controller additionally requires the deployment receipt to hash the exact
passed candidate bundle and provide a passed artifact-backed attestation for every
release scope. `guarded_source_deploy.py apply` now requires a controller bundle,
baseline and candidate directory, rechecks original receipts/artifacts and verifies
every incoming archive byte against that candidate before mutation. This source
installer still does not orchestrate SQL migrations, service restarts or binary
rollout; the final coordinated deployment procedure must call the controller before
mutation and again with `--stage deployed --candidate-bundle ...` afterward.

Run `python3 tools/ci/release_bundle_test.py` for rejection-path regression tests.
The test fixture screenshots and metrics are synthetic unit-test inputs and never
constitute acceptance evidence for the game.

## Runtime identity for Damage Lab comparisons

After the final binary, NPC, database and configuration inputs are stable, run
`release_controller.py runtime-identity --candidate CANDIDATE`. This writes
`conf/import/pn_runtime_identity`, containing only hashes and relative paths.
The map server verifies its actual running executable (`/proc/self/exe`), every
runtime file, and the complete runtime file inventory before readiness. The cached
SHA256 identity is then cheap to read during a lab run. Database loads, NPC changes,
battle configuration changes and administrative reloads invalidate it until a
verified restart. Missing/invalid attestation disables saved comparison runs;
empty identities cannot compare. Unsupported platforms also fail closed.

The controller independently rechecks current binary/content/configuration hashes
when certifying a lab scope. Disk changes without a reload do not alter already
loaded runtime state, but prevent certification until the attestation is regenerated
and services restart. Generate the manifest before the final native/rendered evidence
runs. Fixture configuration and production configuration produce different identities;
a fixture manifest must never be copied into production unchanged.

Pet-scoped SQL evidence must contain the original runner report and production
entitlement, asset commit, retirement, mail, restart and concurrency fields. Generic
scenario receipts cannot substitute for those executions. Package/mail/admin native
coverage is part of the mandatory full native inventory.
