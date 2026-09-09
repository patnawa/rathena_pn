# Alice encounter data and combat rules

This is an original implementation of a custom maze encounter. Public walkthroughs identify actor names, race/element/size, damage-channel mechanics and material identities, but do not publish a reproducible numerical monster database. No external NPC script, artwork or proprietary monster table was copied.

## Original balance

All numerical HP, attacks, defenses, attributes, motion, skill rates and drop rates in `db/alice_mob_db.yml` and the marked Renewal skill block are original tuning. They are not represented as official or exact reference-server stats. Normal trash has 500,000 HP; the normal/first Hard boss has 30,000,000. Hard trash has 2–4 million, and the second/third Hard boss has 120 million. The two last phases intentionally reuse Jabberwocky, since only two boss identities were documented for a three-phase fight. Duckworth uses level 10 Water Ball with a visible 1.8-second interruptible cast and 12-second cooldown. Other attacks form a small explicitly tuned set.

Field monsters provide one sewing kit, maple leaf or tentacle per ordinary successful drop roll at base 100%: 50 Duckworth and 50 Heart provide the non-chain materials for one Mad Bunny. Configured server drop-rate modifiers/caps still apply. Boss records have no chain or stone drops: atomic per-character final-clear rewards deliver Normal chain 1; Hard chain 1+stone 1, plus one further stone when timely. A Sealed Card Album has a base 1% boss drop rate. Cheshire has no drops. All actors have zero EXP to prevent repeat-spawn experience farming; completion materials are the encounter reward.

Native sprite aliases are deliberate visual adaptations: Bloody Knight, Phen, Drosera, Caterpillar, Mandragora, Leaf Cat and Detale respectively for 30239,30245–30250. Their identity mappings and SPR/ACT assets were verified. Phen is an aquatic substitute, not a claimed matching duck sprite. The five materials and all nine exchange outputs already exist; no duplicate item definitions are added.

The only monster appearance loader is `db/import/mob_avail.yml`. Fresh checkouts receive the added `db/alice_mob_avail.yml` footer import from `db/import-tmpl/mob_avail.yml`. Existing installations must append that same import without replacing their other overrides.

## Opt-in native map flags

`resistancecap 50` (`MF_RESISTANCECAP`) caps each defensive card/gear category at 50% in weapon, magic and miscellaneous cardfix calculations. Race, race2, size, damage-type size, class, element, source element, range, type and individual-monster reductions retain their existing independent multiplicative categories. For example, 50% race and 50% size still combine to 75% reduction. This is **not a cap on total combined reduction**. Negative vulnerability values are preserved. DEF/MDEF, RES/MRES, elemental armor tables, separate status reductions and attack bonuses are unchanged. Any contribution already combined into a cardfix category follows that category cap, including the legacy pre-Renewal element adjustment. The cap is calculated at damage time, so no equipment bonuses need changing or restoring after leaving. Values 1–100 are valid; remove the flag to disable it.

`strictdamage` (`MF_STRICTDAMAGE`) changes monster `MD_IGNOREMELEE`, `MD_IGNORERANGED`, `MD_IGNOREMAGIC` and `MD_IGNOREMISC` from plant-style 1 damage to zero on that map only. It checks both calculated damage and final delivery, so delayed hits are checked against the current boss phase and reflected miscellaneous damage cannot bypass a blocked miscellaneous channel. Unflagged maps retain existing behavior. Scripted direct HP manipulation is outside the combat-channel contract. Complete death-lock immunity uses existing `UMOB_DMGIMMUNE`; strict delivery also rechecks that flag to reject attacks queued before the shield activated.

Both flags default off and inherit through ordinary instance map flag copying. There is no global balance change and no actor-ID hardcoding in the engine.

## Validation boundaries

`tools/ci/instance_combat_rules_test.py` compiles the actual pure rule helpers and source-extracted calculation/delivery guards. Its matrix checks every channel/mode combination, normal/skill flags, map opt-in, player exclusion, delayed phase changes and cap/vulnerability boundaries. A negative control disables the new rules and must fail. This focused fixture doubles world lookup and HP delivery; it does not replace a configured full-server build/startup or the NPC state-machine tests. The same guard fixture is compiled with and without the Renewal definition; actual server non-Renewal compilation additionally requires `PRERE`.
