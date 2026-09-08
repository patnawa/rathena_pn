# Shadow Gear reference and server coverage audit

Audit date: 2026-09-08. Scope: item identity, acquisition declarations, item-use recipes, refinement targeting, random options and native enchant entry points. This is a static audit of the shared working tree; it does not certify every combat effect or live client interaction.

Reference pages inspected: [Shadow Gear](https://wiki.muhro.eu/Shadow_Gear), revision 62112; [Shadow Enchanting](https://wiki.muhro.eu/Shadow_Enchanting); [Refinement](https://wiki.muhro.eu/Refinement); [Eden Market](https://wiki.muhro.eu/Eden_Market). These describe MuhRO policy, not necessarily Gravity or PN balance. The machine report stores the downloaded main-page hash, all item IDs, native recipe presence and local source hashes: [shadow_gear_reference_coverage_20260908.json](shadow_gear_reference_coverage_20260908.json).

## Result

The local server has substantial Shadow item data, but **item availability, usable spellbooks and service coverage are incomplete**. Native groups 70–88, 128 and 166 now have guarded service paths; their passing tests do not cover the other spellbooks or establish an acquisition economy.

| Check | Finding |
| --- | --- |
| Explicit reference item IDs | 918 unique IDs inspected |
| Present Shadow equipment | 840, all with nonempty item scripts |
| Missing IDs | 35: eight equipment pieces and 27 other reference items |
| Referenced items invoking a Laphine API | 30 |
| Matching effective Laphine recipes | Nine present; 21 missing |
| Refine hammers 23436 and 23926 | 700 native targets each; each covers 651 of the 840 present reference equipment pieces |
| Standard random-option item 23720 | 610 native targets; 561 of the 840 present reference pieces |
| Native Shadow enchant groups | 21 groups, including 70–88, 128 and 166 |

Counts use recursive Renewal imports, normalized `ShadowGear`/`Shadowgear` type spelling, and the enabled NPC import graph rooted at `npc/re/scripts_main.conf`. A nonempty effect script is not proof that every effect matches its description.

## High-priority defects

### 1. Twenty-one declared item-use paths lack a server recipe

These are actual internal inconsistencies: the effective item script calls an API that looks up a nonexistent recipe. They can be reproduced independently of MuhRO costs by giving a test character the item and using it. `src/map/script.cpp` rejects the missing recipe before opening the corresponding window.

| IDs | Current script API | Missing effective recipe families |
| --- | --- | --- |
| 100205–100208 | `laphine_upgrade()` | Class/Skill physical and magical spellbooks |
| 101177–101180 | `laphine_upgrade()` | Full Penetration, Reload, Spell Caster, Trait Status |
| 101260–101262, 101264 | `laphine_upgrade()` | Bearer, Mega Blitz, Absorb, Mammoth |
| 101308–101311 | `laphine_synthesis()` | Gemstone, Experience, Magical/Clever, Infinity |
| 101359–101362, 101564 | `laphine_upgrade()` | Major Auto Spell, Hasty, Physical/Durable, Perfect Size, Almighty |

Several definitions remain commented out in `db/re/laphine_upgrade.yml`, beginning around line 5116. The four synthesis calls deserve explicit client/source confirmation before selecting a replacement API; changing the call alone cannot supply the missing targets and option distributions. The six older Class Shadow Boxes 23236–23241 do have synthesis recipes, so they must not be conflated with these missing spellbooks.

### 2. Eight Druid skill equipment records are absent

| IDs | Missing family |
| --- | --- |
| 1270175–1270176 | Alpha Fang armor/shoes |
| 1270177–1270178 | Pinion Spear earring/pendant |
| 1270179–1270180 | Shard Nova armor/shoes |
| 1270181–1270182 | Terra Piercer earring/pendant |

The adjacent M. Alitea records 1270183–1270186 are present from the earlier scoped implementation. Its source report explicitly excluded acquisition policy. Add the eight missing records only after verifying their effects, equip restrictions and combinations against primary data/current client facts; do not clone another skill's bonuses.

### 3. Master enchant entry point: fixed after the audit

The baseline scan found no player route to native groups **70–88**, covering **74 target items**: Master weapon/shield and 18 four-piece class families. The existing Shadow Gear Enchanter now has a fourth Master menu entry with nineteen named families and Cancel. Original selections 1–3 remain unchanged, the dispatch accepts only groups 70–88, and `close2` completes before the native window opens. Its office duplicate inherits the same service.

The integration audit confirms all 21 exposed groups (70–88, 128 and 166) agree with installed client recipe semantics. All have reset disabled. Four structural tests pass. The actual production NPC dialogue passes 25 native VM paths and 1,178 assertions under ASan/UBSan: the original four paths, nineteen Master routes, Master Cancel and Escape. These checks preserve the complete synthetic inventory and Zeny and detect script-side payments or item deletion. The UI request is an explicit test double; this does not certify live transport or every recipe execution.

## Refinement and random effects

Both hammers omit **189 present reference pieces**, including Master weapon/shield and newer traits/master families. The missing-target lists are in the JSON. Adding them requires source/client eligibility review; the generic native refiner is a separate path and may still support them.

Local hammer 23436 accepts refine 0–9 and sets a random refine from 1–10. Hammer 23926 accepts 0–8 and produces +9. These limits are explicit in the native DB. The reference calls the hammers generally applicable, but does not publish a complete authoritative target whitelist. Its wording alone is insufficient to remove PN eligibility limits.

Standard item 23720 requires +7–+10 and applies one option from `SHADOW_RANDOM_MIX`. There are twenty equally weighted option entries in the local group. Nineteen ranges match the linked standard-option list; **local critical rate is 1–15, while the reference lists 1–5**. The option script applies the value directly through `bCritical`, so this is not merely a displayed unit conversion. Treat this as a balance discrepancy requiring a policy/source decision, not an automatic nerf. The other 279 present reference pieces are not in that item's target list; some are intentionally covered by class/advanced systems instead.

Group 128's 14 targets, selectable initial costs of five/seven Essence and 24 ordinary upgrade distributions agree with its pinned original-client data. Retention and downgrade outcomes are intentional. Current validation reported by the integration agent passed nine group tests, including native distribution checks over 2.4 million draws, plus four service checks. The expanded Shadow NPC test passes 25 paths/1,178 assertions under ASan/UBSan. Those results must not be described as proof that the 21 missing spellbooks work.

Groups 128/166 are card-slot enchants; item 23720 and class/advanced spellbooks concern random options. These are distinct systems with different eligibility and preservation rules.

## Acquisition, crafting and recycling

No enabled PN implementation of the reference's Clark/Dala/Eno/Gina/Milo/Rayja vendor network was found. MuhRO's Shadow currency/crate and star-stone IDs are missing locally, so its vendor, recycling and Master crafting recipes cannot be copied as functioning paths without a deliberate economy implementation. The reference's class purchase cost is 80 Shadow, five Fire Dragon Scales and 15 Shadowdecon; that is a private-server recipe, not a missing native rAthena requirement.

The native item group DB includes **800 of the 840 equipment pieces** as container rewards. This demonstrates reward definitions, not access to their parent containers. No direct mob drop was found for those 840 items. The machine report records container IDs and NPC token references separately to avoid mistaking a mention, cost, comment or random numeric match for an actual grant.

Forty present pieces have no item-group, direct-mob-drop or enabled-NPC token evidence in this scan. They include the first eight trait pieces 24872–24879, eight Troubadour skill pieces and 24 Special Master pieces. Supplemental searches found no recipes for the sampled families in synthesis/reform/production/barter data. This is a high-priority acquisition investigation, not a proof against every possible dynamic event or GM distribution.

Important examples:

- Shadow Essence 1001253 has container reward definitions but no direct mob/NPC source in this scan. Do not claim it is entirely absent or freely obtainable.
- +9 Hammer 23926 and exchange ticket 1000854 have no direct mob, NPC-token or item-group source evidence.
- Full Power armor 24872 exists and is a group-128 target, but has no source evidence in the audited paths.
- The reference's deterministic equipment cycling, recycling and star-stone downgrade services have no counterpart located in enabled PN scripts. Ticket item presence does not implement a conversion service.

The linked pages do not provide complete numeric recipe tables for every conversion box. Do not infer unidentified boxes or equal output chances from names or “random” wording. Existing native synthesis/group definitions must be checked per box before exposing a craft menu.

## Actionable remediation sequence

1. **Repair broken item use:** map the 21 items to validated current-client APIs, targets and option groups; add regression cases for missing/invalid targets and preservation. Do not sell them until that path passes.
2. **Complete missing equipment:** implement the eight Druid records and verified combos; test actual affected skill bonuses.
3. **Master service route completed:** guarded groups 70–88 are exposed and validated as described above; native charging is preserved.
4. **Audit acquisition closure:** trace the parent containers and the forty unlocated gear sources. Choose a PN source and document it rather than claiming that DB presence is gameplay coverage.
5. **Decide the economy:** approve a consistent currency, crafting, recycling and exchange design separately from bug fixes. Reference prices, VIP systems and outcomes must not silently replace existing PN policies.
6. **Accept in game:** verify representative old/class/trait/master/Druid pieces through acquisition, refine, option reroll, native enchant, cancellation, full inventory, relog and combat. Include modified/equipped/bound items when testing destructive exchange services.

The audit and subsequent Master menu fix made no economy, item or quest mutations. Reproduce its declaration inventory with `python3 tools/ci/shadow_gear_reference_audit.py <saved-Shadow_Gear.html>`; retain the source hash because the live guide changes.
