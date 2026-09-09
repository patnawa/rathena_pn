# Missing native-enchant crowns and Sky partners

Date: 2026-09-06. This batch is **partial: 12 of 13 requested crowns**, plus 11 fully defined Sky weapon dependencies. `401195 Frontier_R_Crown_AT` is deliberately absent from this package. These standalone files do not install themselves; parent integration now owns and has wired the shared import roots. No drop, shop, acquisition recipe, existing item, or existing combo is changed by this package.

## Evidence and scope

Every supported crown was absent from the effective Renewal item load chain at the start of the audit. The original active compiled `ItemDBNameTbl` already contains all 23 exact name/ID pairs. The supplied reference server loose files were used for resource identities, not as authoritative effect text. No full third-party description was copied.

Gravity's item pages are the primary property/effect sources. The reviewed clean-room typed facts, individual item source URLs, every refine divisor, grade threshold, learned-skill gate, and set condition are recorded in `client-patch/druid_missing_crowns/facts.py` and its deterministic manifest. Each source below uses `itemSeq=2` for crowns and `itemSeq=1` for weapons.

| Crown ID / exact Aegis name | Primary source | Partner supplied by this package |
|---|---|---|
| 400999 `Time_DM_R_Crown_AT` | [Gravity Time crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=400999&itemSeq=2) | Crosssets owned by the weapon batch |
| 401176 `FuriousCirclet_AT` | [Gravity Furious crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401176&itemSeq=2) | Crosssets owned by the weapon batch |
| 401171 `Sky_Rune_Crown_SHC` | [Gravity Shadow Cross crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401171&itemSeq=2) | [610093 Sky_Slasher_Katar](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=610093&itemSeq=1) |
| 401172 `Sky_Rune_Crown_AG` | [Gravity Arch Mage crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401172&itemSeq=2) | [550194 Sky_Deadsoul_Staff](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=550194&itemSeq=1) |
| 401173 `Sky_Rune_Crown_BO` | [Gravity Biolo crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401173&itemSeq=2) | [500136 Sky_Thorns_Sword](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=500136&itemSeq=1) |
| 401174 `Sky_Rune_Crown_TR` | [Gravity Troubadour/Trouvere crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401174&itemSeq=2) | [570093 Sky_Music_Viollin](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=570093&itemSeq=1), [580093 Sky_Music_Whip](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=580093&itemSeq=1) |
| 401175 `Sky_Rune_Crown_AT` | [Gravity Alitea crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401175&itemSeq=2) | [590116 Sky_Glacier_Mace](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=590116&itemSeq=1) |
| 401216 `Sky_Rune_Crown_DK` | [Gravity Dragon Knight crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401216&itemSeq=2) | [630066 Sky_Dragon_Spear](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=630066&itemSeq=1) |
| 401217 `Sky_Rune_Crown_EM` | [Gravity Elemental Master crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401217&itemSeq=2) | [540125 Sky_Elemental_Book](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=540125&itemSeq=1) |
| 401218 `Sky_Rune_Crown_SS` | [Gravity Shinkiro/Shiranui crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401218&itemSeq=2) | [650062 Sky_Frost_Humma](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=650062&itemSeq=1) |
| 401219 `Sky_Rune_Crown_NW` | [Gravity Nightwatch crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401219&itemSeq=2) | [830047 Sky_Nightshot_Gatling](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=830047&itemSeq=1) |
| 401220 `Sky_Rune_Crown_SOA` | [Gravity Soul Ascetic crown](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=401220&itemSeq=2) | [550197 Sky_Seonang_Wand](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=550197&itemSeq=1) |

The unusual aliases `Sky_Music_Viollin` and `Sky_Frost_Humma` are preserved exactly; their display labels use ordinary English spelling. New crown definitions are in `db/import/druid_missing_crowns.yml`, partners in `db/import/sky_crown_partner_weapons.yml`, and precisely 11 new Sky memberships in `db/import/druid_missing_crown_combos.yml`.

## Corrections to supplied reference metadata

| Property | Supplied reference server | Gravity value implemented |
|---|---:|---:|
| Furious Alitea crown minimum level | 205 | 235 |
| Sky Thorns Sword ATK | 230 | 220 |
| Sky Music Violin / Whip ATK | 230 | 220 |
| Sky Dragon Spear ATK / weight | 330 / 260 | 370 / 400 |
| Sky Nightshot Gatling ATK / weight | 340 / 260 | 310 / 180 |
| Sky Seonang Wand ATK / weight | 120 / 160 | 100 / 180 |

Weights above are client units; server YAML uses tenths. The reference's Alitea crown View=0 entries are placeholders. Actual Lua 5.1 execution of the active `data.grf` datainfo accessory tables proves `Flash_of_Lightning=2415`, `Time_Dimen_R_Crown=2441`, and `Midgard_Diadem_JP=2692`. These existing view mappings are used for Furious, Time and Sky respectively. All associated inventory/collection images, dropped sprite/action pairs and both male/female worn crown sprite/action pairs exist in the active load chain.

## Explicit implementation assumptions and limits

- Official item pages omit normal-attack reach. Current official rAthena master `e985006171d2eb320ee512a653f4c83aea3d81b6` was checked directly: none of the 11 exact Sky weapon IDs exists. With parent approval, this package explicitly uses existing server category conventions: whip 2, two-handed spear 3, gatling 9, all other supplied categories 1. These are **project defaults, not verified official item-specific reach**. No skill's own range is modified. Official reach verification remains open.
- New job masks use the existing class branches with `Classes: Fourth`, whose current `pc_is_trait_job` gate covers expanded trait classes too. Existing item masks are untouched. Instruments remain male-only and whips female-only through their actual native category/sex rules. Armor, weapon levels, slots, required levels, attack/MATK/DEF and weight come from the linked primary pages. No buy/sell value is guessed.
- The 11 Sky sets require combined crown/right-hand weapon refine at least 24, both Grade A, and the specifically stated learned skill. All gated effects share the complete condition. The Biolo threshold is learned Mayhemic Thorns 5 even though the current skill maximum is 10; this follows its item page rather than inferring a maximum-level requirement.
- The four Time/Furious crosssets are registered **only** in the coordinated `druid_missing_weapon_combos.yml`, never here. Complete crown descriptions include those effects: Furious Axe/non-player race +10%; Furious Hall/Glacial Shard +10%; Dimensions Axe/Savage Lunge +60%, with both Grade A adding combined-refine Savage Lunge damage and Frenzy Fang cooldown -0.2 sec; Dimensions Knife/Tempest Flap +45% and ranged +15%, with both Grade A adding combined-refine Quill Spear damage and Quill Spear cooldown -0.2 sec. There is no minimum combined-refine threshold for those four sets.
- Skill bonuses use canonical parent IDs. Existing actual `skill_dummy2skill_id` aliases handle Astral Strike, Rose Blossom and enhanced Druid damage without redundant/double dummy-key bonuses. Nightwatch's Vigilante 5496/5497 are visual packet IDs in its current handler; actual damage uses base skill 5405.
- Engine semantics remain visible in descriptions: Renewal halves critical-damage gear bonuses for critical skills; non-critical bonuses honor skill flags; autocasts remain subject to skill/target/equipment restrictions. Native registration tests do not prove successful casts against every in-game target or a graphical login/combat session. No official acquisition route or new NPC recipe is inferred from item presence.
- Frontier `401195` is still unsupported. [Gravity's 2026-02-04 notice](https://ro.gnjoy.com/news/notice/View.asp?BBSMode=10001&curpage=4&seq=8193) confirms release/existence, not complete properties. Its item-detail page was unavailable and direct access returned HTTP 403; no bypass was attempted. The supplied reference alone does not meet this batch's agreed primary-evidence threshold. No Frontier identity, effectless metadata, guessed set or acquisition record is emitted.

## Verification

`python3 -B tools/ci/druid_missing_crowns_test.py` passes the static source/render checks and deep prior-data comparison. Before parent integration, all 29,548 effective items and 8,655 effective combos are preserved; the exact 11 new set memberships are disjoint even from old unresolved rows. Candidate overlays are applied in memory only. `--require-import` additionally requires each of the three new files to occur exactly once in the Renewal load chain, rejecting partial, duplicate and wrong-mode imports. Suppressing only these overlays provides the true baseline after installation too.

The subsequent integrated `--require-import` run also passes: all 23 new records and 11 own sets are active exactly once. With the coordinated weapon batch now present and only this package's three overlays suppressed, the preserved baseline is 29,560 items and 8,661 effective combos. The four reviewed external crosssets additionally become resolvable when these crowns are present; none replaces an existing effective set.

`python3 -B client-patch/druid_missing_crowns/verify_client.py` passes with the actual active loader: all 26,865 previous records and nested fields preserved, exactly 23 additions, original target metadata gaps 27 to 15, and 72 required asset roles verified. This is an ephemeral candidate merge through the actual Lua merger, not an active installation. Original bytecode lookup and datainfo accessory functions/tables are executed by the real Win32 Lua 5.1 runtime. The verifier supports explicit `--before-loader`, `--loader` and `--installed` checkpoints and reports whether its checked loader is actually active.

That candidate run finished before parent installation and did not race the loader change. Parent subsequently installed this 23-record fragment together with the coordinated 12-weapon fragment, reporting a passing actual-active 35-record deep comparison: 26,865 to 26,900 records, 602,841 recursive comparisons, every old field unchanged. Active loader SHA256 is now `f2628f0aa0b39eae1a984ea5ef56f37a03c1912ed8551d499cec7e1e30e51c38`. The package's exact 23-record candidate proof was independently rerun successfully against the immutable pre-install loader, explicitly labeled `loader_is_active=false`:

```sh
python3 -B client-patch/druid_missing_crowns/verify_client.py --loader ../client-before-druid-missing-equipment-20260906/SystemEN/itemInfo.lua
```

That checkpoint's SHA256 is `5a3f33728795cc6278183688746b2e6efdd96ca1ced28364a7c55699611f1797`. Do not present the checkpoint's 27-to-15 candidate gap count as the current installed client's gap count. Parent's combined current-client full-reference check reports only three unresolved target metadata identities (Frontier and the two Booster weapons), with all 7,193 original callbacks preserved. The installed combined proof belongs to `tools/ci/druid_missing_client_install_test.py`; this package's strict dedicated-installed mode intentionally still expects only its own two-line loader addition.

Native proof **PASS**: 89,733 VM executions and 8,169,504 assertions, retained at `../missing-crowns-native-proof-20260906/receipt.json`. The fixture compiled fresh production `pc.cpp`, `skill.cpp`, `script.cpp`, `itemdb.cpp`, `clif.cpp` and `malloc.cpp` with ASan/UBSan, while using existing unrelated link objects. The actual Item/Combo parsers and Script VM were compared with the separately evaluated typed arithmetic facts: 23 item scripts across refine 0..20 and grades None/D/C/B/A, and 11 combo scripts across both refinements, grade threshold boundaries and immediately-below/at learned-skill thresholds. It also checks actual job/sex/minimum-level eligibility, cooldown consumers, parent/dummy skill queries, unrelated zero bonus fields and exact autocast registration. Kernel syscall filtering denies sockets/connections/binds/listens. The run ended with explicit clean allocator teardown and no warning/error/sanitizer diagnostics.

The facts are a reviewed oracle, not an independent second primary source: this proves implementation behavior against the reviewed transcription, not that Gravity's public pages are internally complete or current. A full fresh server build and actual graphical combat remain separate integration checks. The four external crosssets are registered and native-tested by the coordinated weapon package; this package pins their actual Script hashes in its manifest so later source drift cannot silently leave stale crown descriptions.

Client fragment SHA256: `3cfd17437eb6de75b93d8cbb546628aa426daca6e326c2a0923548250891983f`.
