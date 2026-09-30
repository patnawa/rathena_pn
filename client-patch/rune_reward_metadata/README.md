# Rune reward metadata

Adds the 32 absent client definitions 105616, 105617 and 105619–105648.
Names and weights come from the effective Renewal item definitions. Descriptions
state the verified container behavior without inventing contents or probabilities.
The two albums reuse the resource bytes of Old Card Album (616); the remaining
reward containers reuse Old Purple Box (617). These are deliberate generic artwork
aliases, not claims of official episode-specific artwork. No new bitmap is needed.

`python build.py --write` regenerates the overlay; `python build.py` rejects drift.
Both need PyYAML, as do the existing database audit tools.

`python verify.py --client <installed-client> --lua <lua5.1.exe> --stage <new-dir>`
creates a separate candidate SystemEN tree. It preserves the installed loader,
appends PNWeights and RuneRewards after existing merges, and applies the idempotent
Sealed Drake correction to the staged tree. It executes the real Lua loader and
client callbacks, checks all 32 names/weights/resource aliases, proves exactly 184
weight records changed, and checks every generated icon reference against DATA.INI
archive indexes. Existing corrected Sealed Drake data must remain exact. The
installed client is never written. The stage must not already exist.

The stage contains `report.json`, `effective-export.tsv`, `metadata.tsv`, and
`triage.json`. Run both `tools/dynamic_reward_catalog.py` and
`tools/audit_item_acquisition.py` with those metadata and triage arguments before
release. Their reports are separate from native gameplay and rendered acceptance.

Validation on 2026-09-29: `improvement-delivery-20260929/rune-client-candidate-v4`
passed the actual Lua 5.1 callback test and archive index audit. Catalog verification
passed 2,681 recipes / 342 output IDs. The original loader SHA256 remains
`a4cdf2fa12c723febe35a1ad66a6621af6382bea47507fe09a0f7900088136ca`; staged loader:
`264960c40410e9731c68c6cbe13ea0cc73afd2c2d8fee61f4bacf508e7700d72`.
Earlier v1/v2 staging attempts failed test expectations and are not release inputs.
Acquisition traversal covers 6,976 IDs with zero missing metadata and zero missing
archive icon references among those traced IDs. Its 193 unresolved dynamic grant
lines remain unresolved; the patch does not certify those routes.

Deployment must use the combined staged loader, not replace it with the older
PNWeights-only candidate. Copy the new overlay and PNWeights alongside it after
the root release controller's preimage/backup checks. The installed Sealed Drake
4496 definition was already corrected at validation time. Do not copy an entire
staged SystemEN tree blindly. Regenerate/verify if the loader or server definitions
change. No installation, rendered verification, or live deployment occurred here.

Existing unfinished NPC fixes remain in the combined release gate:
`card_removal_transaction_test.py`, `armor_enchant_transaction_test.py`, and
`mayomayo_payment_test.py`. Earlier evidence logs show armor 14 cases / 607 assertions
and Mayomayo 35 cases / 1,511 assertions with no memory leaks. Those older logs do
not bind the final candidate; root must rerun the full gate and complete the
outstanding rendered journeys. This patch does not modify those NPCs.
