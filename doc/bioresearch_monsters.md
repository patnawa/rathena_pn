# Bioresearch Laboratory monster data

Seven new records (20536–20542) complete the existing actor family. Record 20543 remains entirely in the upstream database, unchanged. Skills load directly from the standard Renewal skill database; there is no extra CSV import mechanism.

The official [Gravity monster database](https://ro.gnjoylatam.com/en/intro/database/monster/detail/MD_ED_B_YGNIZEM) supplies levels, HP, attributes, defenses and experience; replacing the final Aegis name in that URL selects the other seven actors. The boss has 75,500,000 HP. Those records take precedence over rounded or inconsistent walkthrough summaries.

[Collected monster observations](https://www.divine-pride.net/database/monster/20536/) supply attack ranges, movement, drops and skill timing; IDs 20537–20543 select the remaining actors. The new Attack and Attack2 values are uniquely recovered from both displayed endpoints using the existing Renewal monster formulas: level + STR + floor(Attack × 0.8/1.2), and level + INT + floor(Attack2 × 0.7/1.3). The same reconstruction exactly reproduces existing scientist values 2408/1103.

## Explicit implementation limits

Movement delays and attack animation durations use the closest millisecond values consistent with the published rounded speed displays. New attack delays use twice the attack animation duration; new damage motion uses 480 ms, matching the existing scientist. These timing choices are estimates, not a recovered official packet timing database. Skill/chase ranges use the existing family's 10/12 defaults. New actors use standard aggressive AI 04; the boss additionally has Boss class and MVP mode. Existing scientist motion/AI values are preserved.

109 skill rows retain published skill IDs, levels, probability, cast time, cooldown, cancellation and state. Target selection follows the local skill database: self skills and defensive Pneuma/Safety Wall target self; ordinary offensive skills target the current enemy; self-HP healing conditions target self. The observed unreachable-target reaction uses the engine's rudeattacked condition with threshold one. Target choices are an adaptation because the published table does not expose target columns.

Seven uncertain observations are deliberately omitted: five acolyte ally-heal rows with no published ally HP threshold, merchant Emotion without its required parameter, and a swordsman Bleeding2 row rounded to 0.0% probability. No hidden numeric conditions are guessed.

The loot data uses the observed default monster drops, including the boss card, aura and boxes; no unverified additional rare drop rates are introduced. The clear reward is handled separately by the instance. Existing BIO_W_BOX contains 39 equally weighted weapons; the two materials and item 102571 already exist, so no item definitions are duplicated.

No third-party NPC implementation was copied. The current upstream tree contains no full instance. A public script collection lists it in its README, but its current tree has no corresponding file and its redistribution restrictions do not provide a suitable license for copying into this repository.
