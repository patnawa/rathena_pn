# Soul Ascetic loadout and combo/card audit — 2026-09-07

## Live character

Created MSCSoul (character 150001), slot 2 on the existing GM account. The existing
MSCESXi character was not modified. Soul Ascetic class 4303, base/job 275/60,
82 learned class/inherited skills. STR/AGI/VIT/INT/DEX/LUK: 30/70/90/130/120/100;
4 unspent status points. SPL/CON/STA: 110/110/65; other traits zero. These use the
normal 4099 status-point and 285 trait-point budgets, not arbitrary maximum stats.

Inventory: 20 equipped items, 2 unequipped alternatives, 10,000 Soa Charms.
Refinable gear is +20; gradable gear is Grade A. Non-refinable accessories and
costumes are not given invalid refinements. IDs and attached cards/enchant names
were checked against local effective item imports and client metadata. This does
not prove rendered sprites or combat behavior; login and combat remain to test.

Main set: Dimensions Soul Stick + Time Dimensions Rune Crown (Soul Ascetic),
Nebula Robe of Spell, Circulation of Life: Autumn, Moan of Corruption, Mad Bunny-LT,
Spell Signet of Star and Signet of Circulation: Autumn. Six magic-focused shadow
pieces and Soul Reaper upper/middle/lower + Soul Ascetic garment costume stones.
Frontier Soul Staff and Frontier Rune Crown are paired swap alternatives. The
staff is two-handed: it cannot be equipped together with the regular shield.

This is a high-investment magic build, not a mathematically proven global damage
maximum against every monster. Copo cards favor small/medium targets, the armor
card favors bosses, and the garment card favors Neutral/Holy magic and increases
incoming elemental damage. Monster defenses and chosen attack element matter.

## Verified and repaired

- Audited 5719 effective card/enchant records and 5168 combo records (10504
  nonempty script fields). Checked referenced item names, bonus constants
  (case-insensitive like the script parser), and quoted skill-name references.
  This checks references, not all conditional expressions or effect semantics.
- Dimensions Soul Stick/crown Grade A combo: Blue Dragon triggers learned
  Red Phoenix; bonuses depend on equipped grades/refinement and learned skills.
- Frontier Grade A staff/crown requires summed refinement >=24 and learned
  White Tiger/Black Tortoise level 5. Casting Circle of Directions and Elementals
  activates Blue Dragon -> White Tiger and Red Phoenix -> Black Tortoise for
  60000 ms. Recasting the activator refreshes the bonus timer.
- Fixed player and pet `autobonus3` probability rolls in `skill_onskillusage`:
  inclusive 0..1000 was 1001 outcomes, so a nominal 100% trigger could fail.
  The corrected range is 0..999. A native C++ test enumerates production
  predicates at rates 0, 1, 500, 999, 1000. Ordinary attack autobonus paths were
  not changed by this scoped fix.
- Fixed combo references `WZ_METEORSTORM` -> `WZ_METEOR` and
  `MG_FIREBOLTBOLT` -> `MG_FIREBOLT`, including embedded autobonus scripts.
- Updated the service integrity test to the corrected Shadow Enchanter position
  (grademk 40,180), rather than its old wall coordinate.

Run: `python3 tools/ci/soul_combo_card_audit.py` (Linux/WSL, PyYAML, g++).

## Known exceptions — not certified bug-free

Sealed Clown Card 27213 references `BA_POEMBRAGI2`; Sealed Gypsy Card 27219
references `DC_FORTUNEKISS2`. Enum entries exist, but active Renewal skill database
records do not. Their conditional skill grants cannot be certified functional.
The audit explicitly reports these exceptions. Replacing them with ordinary
party songs without verifying intended card behavior would change functionality.
Neither card is in MSCSoul's loadout.

Card autospells also retain their configured attack triggers: an on-normal-attack
effect is not promised to trigger from magic skills. Runtime parsing at startup
does not execute every equipment script branch.

## Required in-game acceptance checks

1. Relog MSCSoul, confirm equipped gear, cards, Grade A and learned skills persist.
2. With the Dimensions pair, cast Blue Dragon against a valid target and observe
   the Red Phoenix proc. Test a surviving target as well as a killing blow.
3. Equip both Frontier pieces (remove the regular shield). Establish talisman
   states with Blue Dragon, White Tiger, Red Phoenix, Black Tortoise; cast Circle
   of Directions and Elementals. This skill requires Fourth/Fifth God state.
4. Within 60 seconds, verify both Frontier procs, including ground placement of
   Black Tortoise. Check expiration, recast refresh, gear removal and relog.
5. Compare controlled damage with/without each card using identical target,
   element, buffs and stats. Check normal-attack-only effects separately.

No claim of exhaustive combat validation is made. No GRF/EXE change was required
for this character creation/audit. Administrative inventory grants remain outside
the repository.

The initial direct character creation left `body=0`, a valid Novice appearance
on this server. The scoped offline correction set `body=class` (4303), with a
backup beforehand; class, skills, equipment and levels were unchanged. Future
administrative creation must initialize both fields. The subsequent client
screenshot shows Soul Ascetic and the corrected appearance.
