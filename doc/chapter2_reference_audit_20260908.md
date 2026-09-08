# Chapter 2 reference coverage audit

Audited 2026-09-08 against the [Midgard Tales Chapter 2 index](https://wiki.midgardtales.com/Chapter_2) and every linked quest guide: **10 main chapters, one side quest and seven daily guide pages**. The Tamarin daily page contains three tasks. This is a static source/database audit, not a live playthrough. Machine-readable findings and local source hashes are in [chapter2_reference_coverage_20260908.json](chapter2_reference_coverage_20260908.json).

## Finding

PN currently implements a **condensed compatibility campaign**, not the complete referenced quest chain. Its playable content includes three party instances, the major outdoor areas, a 12-stage controller, ten repeatable board tasks and equipment services. The named story interactions, native quest journal transitions and several collection mechanics differ substantially. The source statement that it preserves the published quest order and requirements is too broad.

The reference is a private server guide. Its Explorer Bookmark/VIP payouts, prices and warper behavior are reference-server policy, not proof of official kRO behavior. Differences below are classified separately from implementation defects. Nothing in this audit changes character progress, rewards or prices.

## Main story comparison

| Reference chapter | Reference quest IDs | Local coverage and material difference |
| --- | --- | --- |
| [Guardian's Call](https://wiki.midgardtales.com/Guardian%27s_Call) | 26035–26038 | Partial: Crossroads exists, with four mana waves and a core. Local start uses Guardian El at `ygg_edge,156,179`; reference names Laphine Shasha there, then Vedr/Varg. The reference index instead lists Shasha at `ygg_fruit,79,121`, which the detailed guide uses later. Local start additionally requires five Apples before travel. |
| [To Sanctuary Veledor](https://wiki.midgardtales.com/To_Sanctuary_Veledor) | 18397–18399 | Condensed: local Guardian advances directly to Kindlebrook after Crossroads. El/Newt and the `mu_fild01` safe-spot handoff are absent. |
| [Between Cold and Heat](https://wiki.midgardtales.com/Between_Cold_and_Heat) | 24160–24174 | Mostly absent: interviews, White Ash, boat trip, Chez/Dew and Strange Hal are replaced by a Shadow Jailer hunt and direct warp. Kindlebrook/Dead Root locations exist, but this does not reproduce their story. |
| [Encounter at the Dead Root](https://wiki.midgardtales.com/Encounter_at_the_Dead_Root) | 23388–23420 | Mostly absent: survey points, Gemis inspection, cryo devices and the five-Fluffy-Ember handoff are not local objectives. Orion instead advances the character to Primordial Flame. |
| [Fragments of the End](https://wiki.midgardtales.com/Fragments_of_the_End) | 18400–18401, 8932–8934 | Partial objective: one 20-Shadow-Jailer hunt exists, but its NPC, location routing and place in the story differ. The local Karilon hunt precedes Dead Root. |
| [Residents of Ragsruth](https://wiki.midgardtales.com/Residents_of_Ragsruth) | 27141–27158 | Condensed: the local Keeper takes five Apples. Reference has Painu take five and Sirinna one more, plus the residents' dialogue chain. |
| [Into Nyrholt](https://wiki.midgardtales.com/Into_Nyrholt) | 12668–12669 | Partial: a party-owned Nyrholt instance with waves/gates and Snapdragon exists. Local Keeper replaces Artea's story handoff; room geometry and placements are compatibility substitutions. |
| [True Face of Ragsruth](https://wiki.midgardtales.com/True_Face_of_Ragsruth) | 27159–27162, 8935–8937, 8985, 18403 | Mostly absent/reordered: the second 20-Shadow-Jailer hunt happens locally before Nyrholt, whereas this chapter follows it. Roarin/Jasminna, branch travel and One/Zero/Gregor transitions are absent. |
| [News from Kindlebrook](https://wiki.midgardtales.com/News_from_Kindlebrook) | 26039–26060 | Story absent: the local daily resembles one hunting/material subtask, but does not implement the machine, frozen material nodes, Gemis investigation, delivery items or nursery story. |
| [Distorted Ragsruth](https://wiki.midgardtales.com/Distorted_Ragsruth) | 27163–27171, 12671, 18404 | Partial: local distortion hunt, Phantom and final El report exist. Reference story describes vortex clearing then tree exit; local story requires Phantom Snapdragon combat. Final reference sets 100 Flame Branch reputation; local ensures a minimum of 100. |

Local state is `CH2_Step` plus quest **27101**, objective quests **27102–27104**, and clear flags. The effective quest DB lacks the reference completion IDs **27171/18404** and sampled reference starting IDs. The login compatibility handler only recognizes local 27101; its comment about preserving characters who completed the published quest does not establish migration from native completion IDs. A future native expansion needs an explicit migration plan rather than replacing these IDs in place.

## Side quest and daily comparison

All local daily tasks unlock on local Chapter 2 completion and use independent four-hour cooldowns starting at reward claim. The referenced daily guides do not establish that exact timer, so this audit does not label it wrong.

| Reference page | Local quest(s) | Coverage |
| --- | --- | --- |
| [When It's Time to Do Hal Work](https://wiki.midgardtales.com/When_It%27s_Time_to_Do_Hal_Work.) | None | Missing side chain 24175–24178. Tamarin's three tasks therefore skip this reference prerequisite. |
| [Even a Handful of Mana](https://wiki.midgardtales.com/Even_a_Handful_of_Mana) | 27117 | Ten-item turn-in exists. Reference uses grass-pile collection at 50%; local Ghost Flower/`Ch2_Ghost_Grass` comes from Ice Needle's unconditional 8% base drop. No grass-pile interaction appears in the Chapter 2 controller. |
| [Okay, Calm Down](https://wiki.midgardtales.com/Okay%2C_Calm_Down) | 27118 | 100-monster objective exists, limited locally to three Folnir types in `rgs_dun1`. The reference says 100 monsters without enumerating eligible species. Board replaces Gregor. |
| [Dream of Perpetual Motion](https://wiki.midgardtales.com/Dream_of_Perpetual_Motion) | 27113 | Matches 15 each of the three named enemies through translated native IDs 22691/22692/22693. Board replaces Nedim; reference first-completion extra payout is not implemented. |
| [Helping Hands at the Nursery](https://wiki.midgardtales.com/Helping_Hands_at_the_Nursery) | 27114 | Ten Hal Puffs required. Reference Bartum/Lutem introduction and quest-conditional drops from three enemy species are absent; local base drop is only Tuktakson, 12.5%, without active-quest gating. |
| [Tamarin's Hal Work](https://wiki.midgardtales.com/Tamarin%27s_Hal_Work) | 27110–27112 | All three hunt/material pairs match counts: ten enemies and five associated materials each. Named NPC and side prerequisite are absent. |
| [Research Assistance](https://wiki.midgardtales.com/Research_Assistance) | 27116 | Matches ten Fire Cotton and three Fluffy Ember/Spark items. Board replaces Samra. |
| [Shelter Visit](https://wiki.midgardtales.com/Shelter_Visit) | 27115 | Matches ten Alpuring, ten Icy Blast and five each of two materials. Missing random frozen-material task, Orion handoff and five hydraulic tubes/five screws delivery to Karillon. |

Daily payouts are locally 3 or 6 Flame Coins, +2 reputation and 12m/8.4m EXP. Reference payouts use Explorer Bookmarks, equal base/job EXP and VIP additions. Treat these as explicit economy differences; copying the other server's payout system is not required to fix missing interactions.

## Places, equipment and services

The six featured field/dungeon populations are represented: `deadroot`, `mu_dun01`, `mu_dun02`, `mu_fild02`, `mu_fild03`, `rgs_dun1`. Translation variants such as Dudulson/Doodlehand do not by themselves indicate a missing monster. Native map geometry and coordinates are deliberately replaced by synchronized compatibility aliases, as documented in `npc/custom/chapter2/SOURCE_INFO.txt`; this audit does not certify client walkability.

The Flame Coin Exchanger has the reference's Leaf/Mana Ring/coin exchange costs and eleven Azure equipment purchases: six at 125 coins +4m Zeny and five at 200 coins +6m. Its location is `kin_in01,119,215`, versus reference `27,197`.

The Kindle Hal merchant charges 200k/400k/700k Zeny outright. The index lists outright prices of 2m/4m/7m, and discounted prices of 200k/400k/700k **plus Hal materials**. Local discounted-material variants are absent. This is a clear price-policy divergence, not sufficient evidence to raise live prices without an explicit decision. The local merchant comment claiming published economy needs qualification.

Cardron, native enchant groups 167–171, refinement and Azure-to-Blaze reform exist. This audit confirms their source presence, not every stat formula or live client UI. The separate [native enchant coverage report](chapter2_native_client_coverage.md) and [gear deployment receipt](chapter2_gear_deployment_20260906.md) cover that prior work. The Midgard Tales index alone is insufficient to validate all local refinement/enchant formulas.

## Recommended implementation order

1. Describe the existing campaign as a compatibility adaptation in player/help documentation; remove full-parity claims.
2. Add named daily NPC aliases, accurate objective navigation and transparent item-source help without changing existing character state.
3. Add missing collection interactions and the Hal Work side story with migration-safe unlocks for existing players; decide whether to preserve old acquisition sources.
4. Expand the narrative in reference order behind a versioned state scheme. Keep a tested path for characters in every current stage and completed characters.
5. Decide reward/merchant policies separately, then test a new and an existing character through entry, objectives, full inventory, death, disconnect, re-entry and final reward.

No claim of complete Chapter 2 parity or runtime acceptance is supported by this static audit.
