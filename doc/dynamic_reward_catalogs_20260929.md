# Dynamic inventory reward catalog

`tools/dynamic_reward_catalog.py --write` derives the checked-in typed catalog
from Rune Tablet's authoritative JSON and the actual Omega/Ellie/Abyss recipe
definitions. Ordinary invocation rejects changed output declarations, stale
source bindings, generated Rune script drift and missing effective Renewal items.
Unsupported recipe forms fail rather than guessing an output. The bounded parser
does not interpret arbitrary scripts.

The catalog covers 2,681 recipes and 342 distinct inventory output IDs: Rune
milestones, imprints, purchases, decomposition and seals; 41 Biosphere material
recipes; and five Abyss conversions. Records distinguish quantity ranges,
independent chance, batch limits, repeatability, material cost and eligibility.
Rune collection IDs are not represented as inventory grants. This is not an
economy-cycle analysis or proof of crash-atomic payment.

`audit_item_acquisition.py` incorporates validated catalogs for active NPC files.
It separately reports cataloged dynamic lines and retains all unresolved lines.
No catalog means the earlier literal-only audit behavior remains available.

Client verification requires both `--metadata <effective metadata.tsv>` and
`--triage <resource audit.json>`. It rejects missing output metadata, missing
resources and unknown triage schemas. `--report` records failures as failures.
This checks the supplied effective archive inventory, not rendered appearance;
regenerate those inputs after overlays change and bind them to the release.

The existing actual native Biosphere material and conversion fixtures execute
every recipe. The Rune native runner now generates exhaustive production service
calls from the authoritative catalog, checking every milestone, imprint, purchase
and decomposition recipe against actual inventory outputs. Random decomposition
checks allowed identity/ranges and guaranteed outputs; it does not statistically
certify probabilities. Existing cancellation, capacity, metadata and repeated-claim
cases remain. Rebuild/run these fixtures on the candidate before claiming native
acceptance. Portable rejection tests are `tools/ci/dynamic_reward_catalog_test.py`.

Fresh read-only Lua 5.1 export of installed `PN-Client/SystemEN/itemInfo.lua`
confirms 30 missing cataloged Rune reward IDs 105619–105648. Acquisition traversal
also exposes nested containers 105616 and 105617, for 32 missing definitions.
The expanded graph traces 6,976 IDs, catalogs six dynamic grant lines, and leaves
193 dynamic lines unresolved. The supplied archive inventory contains no missing
artwork references for these traced items; absent metadata still fails acceptance.
Repair and regenerate effective client evidence before release. Five rendered
journeys and previous visual gaps remain separate acceptance requirements.
