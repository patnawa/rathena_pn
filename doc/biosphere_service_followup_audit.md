# Omega and Ellie follow-up audit

Read-only source/effective-data audit at
`50717df9e48a6a762303a05e5d92dcc65092b8bc`, 2026-09-06. This document preserves
findings for a later NPC patch. It does not claim those NPC paths have been fixed,
or that ordinary player trade/admin mutation during NPC input was demonstrated.
The separately approved equipment-switch deletion engine correction and its
native proof are documented in `equipswitch_deletion_audit.md`.

Inspected NPCs:

- `npc/custom/varmundt_biosphere_quests.txt`: Omega#biosphere_materials (line 65),
  Ellie#biosphere_equipment (line 152). Raw SHA256
  `787f6ff54a4c43a60510e4a606dc5bd33a80c502853f113ad6756e257ac5aa5a`.
- `npc/custom/varmundt_biosphere_depth.txt`: Ellie#bio_d1_fusion (line 235).
  Raw SHA256 `40730ba4620a448e03ad2d71ff6d28f9398934fa5c1af1e22cb083e6ac392f28`.

Completed L_Convert and crown services are outside this follow-up scope.

## Confirmed source paths and priorities

1. Omega's fourteen elemental conversions and three waters, plus Depth-1
   Ellie's twenty-four fusions, calculate resource maxima before actual numeric
   input. They then delete materials before Zeny assignment, without a fresh
   complete payment check. A post-input Zeny shortage consumes materials before
   native negative-Zeny assignment aborts; there is no output. Omega water can
   consume earlier essences before a later essence shortage makes delitem abort.
   This is partial material loss, not a free-output claim.
2. All three quantity-input locations ignore input's native status. Below/above
   range values are clamped and can become an unintended purchase. Input sites
   are quests lines 102 and 135, and depth line 263.
3. Those material paths use ID-only checkweight/pc_checkadditem, while native
   pc_additem selects the first compatible binding/expiry/UID/cards stack. A
   small incompatible first-ID row can conceal a full compatible row, causing
   payment followed by failed output. Conversely a full incompatible row can
   reject a valid later compatible row. This conditional layout is native-valid;
   ordinary enabled acquisition of all such variants was not established.
4. Aggregate material requests narrow to signed int16 in delitem. Multipliers
   x10 cross that width at 3277 outputs; x5 at 6554. Do not replace this with
   arbitrary 3000/6000-output caps: preserve full resource/Zeny maxima, cap one
   output grant at native MAX_AMOUNT=30000, and chunk fully preflighted material
   totals into requests <=30000. Depth stage 3's Zeny ceiling remains lower than
   30000; do not discard that ceiling. These are conditional large-inventory
   arithmetic findings, not a demonstrated live exploit.
5. Access is checked only before later dialogue yields. Omega and equipment
   Ellie require only BaseLevel >=240. Depth fusion requires BaseLevel >=250
   and ep17_2_main >=33. None requires reputation. A future final access recheck
   must preserve these exact predicates, not borrow a Depth-2 gate.

Equipment Ellie already rechecks all resources after the final confirmation
(quests lines 196-208). Its output is nonstackable and its final checkweight
requires an actual allowed empty slot. No corresponding ordinary output-stack
capacity failure was established there; do not copy the material-stack finding
wholesale onto this branch.

## Equipment selection and metadata

ID-only delitem prefers unequipped, unrefined, cardless copies, then can consume
other matching copies. Binding, UID, expiry, favorite, options and equipment-switch
registration do not select an exact player-confirmed instance. With only an
important copy available, the exchange can consume a refined/carded/equipped
item. Fresh getitem does not transfer those attributes.

The current UI explicitly calls this a consuming exchange but does not identify
the exact inventory instance or disclose each discarded attribute. This is a
destructive-selection risk, not evidence authorizing automatic inheritance of
refinement/cards/binding. A faithful future change should explicitly identify
and confirm the intended item and discarded metadata, snapshot it across the
confirmation, and reject a changed target before payment. Any new restriction
or preservation policy needs separate approval rather than invented economy.

An ordinary reachable engine defect was found independently: a base item may be
registered in equipment-switch before opening Ellie. delitem ignores that field;
the old full-depletion pc_delitem clears the item but not equip_switch_index.
Fresh output may reuse the index while the stale cache still references it.
feature.equipswitch is enabled. getinventorylist does not expose equipSwitch;
the approved correction therefore belongs in native deletion, not a guessed
NPC array field. See the dedicated before/after regression.

## Exact unchanged recipe inventory

Omega element order is Glade, Fire, Ice, Death, Soul, Venom, Temple:

| Tier | Exact IDs in element order |
| --- | --- |
| Fragment | 1000636, 1000637, 1000638, 1000639, 1001181, 1001179, 1001177 |
| Rune | 1000640, 1000641, 1000642, 1000643, 1001182, 1001180, 1001178 |
| Essence | 1001138, 1001139, 1001140, 1001141, 1001185, 1001184, 1001183 |

Each element consumes ten fragments or five runes, plus 20,000z, for one next-tier
item. Each water consumes five of each common essence 1001138..1001141 plus five
of the matching special essence, plus 20,000z. Soul/Venom/Temple special inputs
are 1001185/1001184/1001183; outputs are 1001186/1001189/1001188 respectively.

Depth-1 element order is Fire, Earth, Ice, Storm, Soul, Purification, Corruption,
Poison. Energy IDs are 1001290..1001297; crystals 1001298..1001305; runes
1001306..1001313; essences 1001314..1001321. Stages cost x10/30,000z,
x5/50,000z and x10/100,000z respectively for one output.

Fragments, Depth energy and Depth crystals have native Weight 1; other material
tiers and waters Weight 10. All fifty-six material identities are Etc, with no
per-item inventory stack cap, script, generated-UID or autoequip flag. Outputs
have native sell zero. Do not copy the previous L_Convert uniform Weight 10
assumption. All prices above are current project recipes, not a new assertion
of an official acquisition economy.

Equipment order: Glade, Fire, Ice, Death; each has Armor, Garment, Boots:

| Element | Armor | Garment | Boots | Rune |
| --- | ---: | ---: | ---: | ---: |
| Glade | 450201 | 480145 | 470108 | 1000640 |
| Fire | 450200 | 480146 | 470109 | 1000641 |
| Ice | 450203 | 480148 | 470111 | 1000642 |
| Death | 450202 | 480147 | 470110 | 1000643 |

Bases are 450199/480144/470107; requirements 30/20/15 matching runes plus
300,000z. Soul/Venom/Temple rings use base 490297, thirty runes
1001182/1001180/1001178 plus 100,000z, producing 490299/490300/490301.
All fifteen results are nonstackable armor without autoequip; their regular
equipment Scripts do not run merely because getitem grants the unequipped item.

## Callback closure required for a later paid-path patch

The service map is ba_in01, not the earlier empty-QuestInfo ba_chess scope.
Enabled source inventory found thirty-three direct active questinfo registrations:
thirty in npc/re/quests/quests_17_2.txt and three in
npc/custom/episode19/quests_19.txt. Their condition calls are only isbegin_quest,
checkquest and countitem, plus scalar story/BaseLevel reads and comparisons.
Native quest_check is read-only, including PLAYTIME and HUNTING modes. Static
ba_in01 duplicates inherit dummy_npc/dummy_cloaked_npc, whose bodies only end.

Native deletion and addition call pc_show_questinfo; do not label this map's
registry empty. A new pinned condition inventory/native fixture and relocation
review should qualify the map-specific closure before a future patch is accepted.
Sell-zero outputs do not meet the current seven Get_Item thresholds. Weight
notifications can start/end the reviewed Weight50/Weight90 statuses.

Deleting an equipped base additionally enters pc_unequipitem, status calculation,
card/item UnEquipScript and combo/bonus callbacks. The four effective base item
definitions have no scripts, and no base-item combo or effective bAddMaxWeight
script was found, but base metadata alone is not a complete callback proof.
The earlier broad callback review must be explicitly qualified for the selected
equipment domain before claiming transaction safety.

Proposed regression: all forty-one quantity recipes and fifteen equipment
results; actual input/debit/add semantics; original partial-loss cases; each
stale resource/access predicate; exact weight and allowed-slot boundaries;
native separated-stack ordering; chunked aggregates across integer boundaries;
bound inputs; cancellations; exact selected-instance substitution; and full
equipment-switch/cache preservation. No runtime edits were made in this audit.
