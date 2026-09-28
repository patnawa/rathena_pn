# Costume gameplay and refinement audit — 2026-09-28

## Confirmed fixes

1. **Recovery duplication with a full inventory.** At 200 occupied slots, the
   recovery script could grant a returned stone and then exceed the VM jump
   limit before removing the enchant or charging payment. Repeated inventory
   passes caused the abort. The repaired script indexes equipped records once
   and captures exact-index stack amounts during an existing pass. It retains
   the configured VM limits and exact-index payment/refund checks.
2. **HD NPCs used the wrong success recipe.** Mighty Hammer and Basta charged
   HD ore but queried Enriched success rates. The native VM reproduced 85
   mismatches across 91 equipment/level cases. Basta returned zero chance above
   +10 because those levels have no Enriched recipe. `REFINE_SUCCESS_RATE` now
   exposes the selected recipe's chance out of 10,000; both NPCs use it with
   `rand(10000)`. Missing HD recipes are refused before payment. Database rates
   and prices are unchanged; displayed fees now use the actual charged value.
3. **Shadow refinement ended at a referral.** With the native refine UI enabled,
   Shadow Blacksmith referred players to a legacy service that did not offer
   shadow slots. It now opens the native refinement window directly. The
   fallback's fee text also matches its configured charge.

## Validation

- Fashion transactions: **93 native cases, 5,070 assertions**, including full
  200-slot inventories, multi-position costumes, all four enchant slots,
  returned stacks at the final index, split bound-coin payments, rollback,
  and equipment callback changes. Five broken variants are rejected.
- Combat: **517 native cases, 11,939 assertions**. Forty-five independent
  expected-result cases exercise actual status/combo calculation, skill-level
  thresholds, Renewal cast/delay and magic cardfix. All 472 unique outputs
  representing the 497 materials pass recalculation and removal smoke checks.
  Four broken variants are rejected. This is not every skill's complete damage
  simulation. See `costume_combat_audit_20260928.md`.
- Refinement: **92 native recipe/routing cases**, two rejected old-rate variants,
  110,000 deterministic native handler roll outcomes, 13 payment guards, and
  eight certificate guard cases. UBSan checks pass. The container's ASan library
  cannot link, so no ASan result is claimed for this run.
- A complete candidate map-server build succeeds while preserving the previous
  build. A fault-injection check verifies restoration after a partial install.

## Live validation and visual scope

Four real packet/SQL cases pass: actual Enchanter application, logout and
reconnect, actual Gregio recovery with all 200 inventory slots occupied, and a
native refine-window upgrade followed by a repeated request. Recovery grants
exactly one stone and charges 30 points; refinement consumes one Phracon and
50 Zeny once. Costume unique ID, all unrelated cards/options, refine +7,
grade 3, binding/favorite fields and calculated maximum HP survive both relogs.
The earlier six integration/persistence smoke scenarios also pass.

Actual Ragexe screenshots confirm visible rendering for **37 of 52 applied
native effect IDs**; 15 remain visually unconfirmed. This is not an all-effects
visual pass. See [the visual audit](costume_visual_audit_20260928.md).
Separate structural checks cover 59 STR animations and 714 textures.
An earlier unexplained client exit and incomplete moving-footprint coverage
remain limitations; no production asset defect was established. The visual
test realm, tunnels, adapter and current client were stopped. Three earlier
elevated private client processes could not be closed with current Windows
permissions. See `visual-runtime-report.md`, `visual-runtime-report.json`,
`visual-contact-sheet.jpg` and `visual-cleanup.json` in the audit workspace.

A SQL inventory
row's auto-increment `id` can change when its card array changes: the character
server deliberately deletes and reinserts unmatched rows. Persistent
`unique_id`, item fields and calculated bonuses are the relevant invariants.

Red Flame and the 22nd Anniversary halo still lack a verified effect mapping.
Fresh primary-source checks found no mapping suitable for implementation;
existing verified 22nd Anniversary numeric bonuses remain available. See
`costume_remaining_visuals_followup_20260928.md`.

## Fashion Point visual boxes

The deployed Fashion Box Shop at `mal_in01,27,120` already offers all 56
supported visual materials through four boxes costing 50 Fashion Points each:
Top Visual Effect Box (8 outcomes), Middle Visual Effect Box (14), Lower
Visual Effect Box (10), and Garment Footprint Box (24). Use `@fashion`, then
click **Fashion Boxes** at `pn_style,160,130` (the same shop as the original
NPC at `mal_in01,27,120`). The visual boxes are menu entries 22 through 25.
Buying delivers a box; double-clicking it awards
one uniformly selected stone from that box's pool. The separate Festa Upper
Slot 2 Box (entry 26, 14 outcomes) also costs 50 points. No additional NPC or
database change is needed.

Five focused native purchase-then-open cases read the actual box item scripts,
select these five shop entries, and verify exactly 50 points charged once,
one physical stone from the correct pool and one consumed box. The expanded
suite passes **98 cases / 5,221 assertions**, with clean UBSan/allocator checks
and all five negative mutations rejected. Evidence: `visual-box-native.log`
and `visual-box-verification.json` in the audit work directory. This test-only
follow-up does not rewrite the original deployed validation proof.

## Evidence and rollout

Evidence and reversible deployment backups are under
`Server-Development/gameplay-refine-audit-20260928` and the corresponding
isolated server audit directory. `validation.json` binds the tested files to
the seven-file update; `deployment/report.json` records actual rollout status.
No production account or valuable equipment is used for audit experiments.

The seven-file update is **deployed**. Production startup is clean, all
services are healthy, and wallet/bank balances plus Fashion/Gold Points are
unchanged. The running map artifact is
`34e0bc12a3e7fdd70b401ce8cb579e22df352958c268cbdc3f487269c6027c61`.
The tested build source is `/app/pn-gameplay-refine-20260928/candidate`; the
previous build and exact pre-deployment files remain available for rollback.

A read-only release check before Git publication on September 28 confirms all
14 combined costume/refinement payload files match their deployment manifests,
the running map-server matches the tested binary, and all seven production
services are healthy. No repeat restart or data migration was needed.

Pre-publication catalogue generation, all five fashion data tests and the
497-pair type-guard test pass. The broader Druid mentor suite passes 17 of 18
tests: its Alitea level-cap assertion expects 275 while the effective database
contains 285. Running that assertion from the prior Git HEAD reproduces the
same failure; this unrelated existing mismatch is not changed by this release.
