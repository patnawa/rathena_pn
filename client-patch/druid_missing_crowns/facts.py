# ============================================================================
#  PN  /  CLIENT TOOLING
#  facts.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/client-patch/druid_missing_crowns/facts.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Reviewed clean-room Gravity facts; no third-party description text.

Every effect corresponds to the itemview page for the containing item ID.
See doc/druid_missing_crowns_audit.md for implementation assumptions and limits.
The typed facts also serve as an arithmetic oracle for actual Script VM tests.
"""
from copy import deepcopy


def fx(op, value, selector=None, step=0, **conditions):
    return dict(op=op, value=value, selector=selector, step=step, conditions=conditions)


def sk(name, value, **kw):
    return fx('bSkillAtk', value, name, **kw)


def magic(element, value, **kw):
    return fx('bMagicAtkEle', value, 'Ele_' + element, **kw)


def race(value, **kw):
    return [fx('bAddRace', value, 'RC_All', **kw),
            fx('bAddRace', -value, 'RC_Player_Human', **kw),
            fx('bAddRace', -value, 'RC_Player_Doram', **kw)]


def grades(magical=False):
    return [fx('bSMatk' if magical else 'bPAtk', 5, grade=1),
            fx('bMagicAddSize' if magical else 'bAddSize', 10, 'Size_All', grade=3),
            fx('bMagicAddEle' if magical else 'bAddEle', 10, 'Ele_All', grade=4)]


ITEMS = []
COMBOS = []


def crown(id, aegis, label, job, effects, resource='Midgard_Diadem_JP', view=2692,
          level=240, defense=50, weight=0):
    row = dict(Id=id, AegisName=aegis, Name=label, Type='Armor', Defense=defense,
               Weight=weight * 10, Slots=1, Jobs={job: True}, Classes={'Fourth': True},
               Locations={'Head_Top': True}, ArmorLevel=2, EquipLevelMin=level,
               Refineable=True, Gradable=True, View=view)
    ITEMS.append(dict(item=row, effects=effects, resource=resource,
                      source=f'https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID={id}&itemSeq=2'))


def sky_physical(kind, skill):
    effects = [fx('bBaseAtk', 3, step=2),
               fx('bCritical' if kind == 'critical' else 'bHit', 2 if kind == 'critical' else 5, step=2),
               fx('bAtkRate', 2, step=3), sk(skill, 5, step=4),
               fx('bPAtk', 5, refine=9)] + grades()
    if kind == 'critical':
        effects += [fx('bCritAtkRate', 3, step=3)]
    else:
        effects += [fx('bLongAtkRate', 1, step=3)]
    return effects


def sky_magic(step_element, skills, refine_elements, fixed=True):
    return ([fx('bMatk', 5, step=2), fx('bMatkRate', 2, step=3),
             magic(step_element, 2, step=3)] + [sk(s, 5, step=4) for s in skills]
            + [magic(e, 10, refine=7) for e in refine_elements]
            + [fx('bSMatk', 5, refine=9),
               fx('bFixedCast' if fixed else 'bVariableCastrate', -500 if fixed else -10, refine=11)]
            + grades(True))


crown(400999, 'Time_DM_R_Crown_AT', 'Time Dimensions Rune Crown (Alitea)', 'Alitea',
      [fx('bMaxHP', 120, step=2), fx('bMaxSP', 30, step=2), fx('bAtkRate', 2, step=3),
       sk('AT_FRENZY_FANG', 5, step=4), sk('AT_QUILL_SPEAR', 5, step=4),
       fx('bShortAtkRate', 10, refine=7), fx('bLongAtkRate', 10, refine=7),
       fx('bPAtk', 5, refine=9), *race(15, refine=10),
       fx('bCritical', 10, refine=11), fx('bFixedCast', -500, refine=11),
       fx('bPAtk', 3, grade=1), fx('bMaxHPrate', 3, grade=2), fx('bMaxSPrate', 3, grade=2),
       fx('bShortAtkRate', 10, grade=3), fx('bLongAtkRate', 10, grade=3),
       fx('bAddSize', 15, 'Size_All', grade=4)],
      resource='Time_Dimen_R_Crown', view=2441, level=250)
crown(401176, 'FuriousCirclet_AT', 'Furious Circlet (Alitea)', 'Alitea',
      [fx('bBaseAtk', 10, step=2), fx('bMatk', 10, step=2),
       sk('AT_ALPHA_CLAW', 4, step=3), sk('AT_TERRA_HARVEST', 4, step=3), sk('AT_GLACIER_SHARD', 4, step=3),
       fx('bVariableCastrate', -15, refine=7), magic('Water', 10, refine=9), magic('Earth', 10, refine=9),
       fx('bShortAtkRate', 10, refine=9), fx('bAtkRate', 5, refine=11), fx('bMatkRate', 5, refine=11),
       fx('bFixedCast', -300, refine=11),
       fx('bPow', 5, grade=1), fx('bSpl', 5, grade=1), fx('bCon', 5, grade=1),
       fx('bPAtk', 3, grade=1), fx('bSMatk', 3, grade=1),
       fx('bShortAtkRate', 10, grade=2), magic('Water', 10, grade=2), magic('Earth', 10, grade=2),
       fx('bAtkRate', 5, grade=3), fx('bMatkRate', 5, grade=3), fx('bFixedCast', -200, grade=3),
       fx('bPAtk', 5, grade=4), fx('bSMatk', 5, grade=4), fx('bShortAtkRate', 15, grade=4), magic('All', 15, grade=4)],
      resource='Flash_of_Lightning', view=2415, level=235, defense=30, weight=30)
crown(401171, 'Sky_Rune_Crown_SHC', 'Sky Rune Crown (Shadow Cross)', 'Assassin',
      sky_physical('critical', 'SHC_SAVAGE_IMPACT') +
      [fx('bShortAtkRate', 10, refine=7), fx('bDelayrate', -10, refine=11),
       fx('bCritical', 10, grade=2), fx('bCRate', 5, grade=2)])
crown(401172, 'Sky_Rune_Crown_AG', 'Sky Rune Crown (Arch Mage)', 'Wizard',
      sky_magic('Dark', ['AG_SOUL_VC_STRIKE', 'AG_MYSTERY_ILLUSION'], ['Ghost', 'Dark']) +
      [magic(e, 10, grade=2) for e in ('Ghost', 'Neutral', 'Dark')])
crown(401173, 'Sky_Rune_Crown_BO', 'Sky Rune Crown (Biolo)', 'Alchemist',
      sky_physical('critical', 'BO_MAYHEMIC_THORNS') +
      [fx('bLongAtkRate', 10, refine=7), fx('bFixedCast', -500, refine=11), fx('bLongAtkRate', 15, grade=2)])
crown(401174, 'Sky_Rune_Crown_TR', 'Sky Rune Crown (Troubadour/Trouvere)', 'BardDancer',
      sky_physical('hit', 'TR_ROSEBLOSSOM') +
      [fx('bNonCritAtkRate', 3, step=3), fx('bLongAtkRate', 10, refine=7),
       fx('bFixedCast', -500, refine=11), fx('bPerfectHitAddRate', 10, grade=2)])
crown(401175, 'Sky_Rune_Crown_AT', 'Sky Rune Crown (Alitea)', 'Alitea',
      sky_magic('Water', ['AT_GLACIER_SHARD', 'AT_GLACIER_NOVA'], ['Water']) +
      [sk('AT_GLACIER_NOVA', 10, grade=2)])
crown(401216, 'Sky_Rune_Crown_DK', 'Sky Rune Crown (Dragon Knight)', 'Knight',
      sky_physical('hit', 'DK_DRAGONIC_BREATH') +
      [fx('bMaxHPrate', 3, step=3), fx('bLongAtkRate', 10, refine=7),
       fx('bFixedCast', -500, refine=11), fx('bPerfectHitAddRate', 10, grade=2)])
crown(401217, 'Sky_Rune_Crown_EM', 'Sky Rune Crown (Elemental Master)', 'Sage',
      sky_magic('Earth', ['EM_DIAMOND_STORM', 'EM_TERRA_DRIVE'], ['Water', 'Earth']) +
      [sk('EM_TERRA_DRIVE', 10, grade=2)])
crown(401218, 'Sky_Rune_Crown_SS', 'Sky Rune Crown (Shinkiro/Shiranui)', 'KagerouOboro',
      sky_magic('Wind', ['SS_RAIDENPOU', 'SS_REIKETSUHOU'], ['Wind', 'Water'], fixed=False) +
      [sk('SS_RAIDENPOU', 10, grade=2)])
crown(401219, 'Sky_Rune_Crown_NW', 'Sky Rune Crown (Nightwatch)', 'Rebellion',
      sky_physical('hit', 'NW_THE_VIGILANTE_AT_NIGHT') +
      [fx('bNonCritAtkRate', 3, step=3), sk('NW_MIDNIGHT_FALLEN', 5, step=4),
       fx('bLongAtkRate', 10, refine=7), fx('bFixedCast', -500, refine=11), sk('NW_MIDNIGHT_FALLEN', 15, grade=2)])
crown(401220, 'Sky_Rune_Crown_SOA', 'Sky Rune Crown (Soul Ascetic)', 'SoulLinker',
      sky_magic('All', ['SOA_TALISMAN_OF_FOUR_BEARING_GOD'], ['All']) + [magic('All', 10, grade=2)])


def weapon(id, aegis, label, job, subtype, attack, weight, effects, resource, view,
           range=1, matk=0, both=False, gender=None):
    row = dict(Id=id, AegisName=aegis, Name=label, Type='Weapon', SubType=subtype,
               Weight=weight * 10, Attack=attack, Range=range, Slots=2,
               Jobs={job: True}, Classes={'Fourth': True}, Locations={'Both_Hand' if both else 'Right_Hand': True},
               WeaponLevel=5, EquipLevelMin=240, Refineable=True, Gradable=True)
    if matk:
        row['MagicAttack'] = matk
    if gender:
        row['Gender'] = gender
    ITEMS.append(dict(item=row, effects=effects, resource=resource, client_view=view,
                      source=f'https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID={id}&itemSeq=1'))


def crit_weapon(skill, atk_step, crit_step, grade_b):
    return [fx('bCritical', 5), sk(skill, 15), fx('bBaseAtk', atk_step, step=3),
            fx('bCritAtkRate', crit_step, step=3), sk(skill, 7, step=4),
            fx('bCritAtkRate', 25, refine=7), fx('bCritical', 10, refine=9), fx('bPAtk', 5, refine=9),
            fx('bCritical', 5, refine=11), sk(skill, 10, refine=11), fx('bAtkRate', 5, grade=1),
            sk(skill, 10, grade=2), *grade_b, fx('bPAtk', 10, grade=4)]


def magic_weapon(skill, element, matk_step=30, ele_step=2, grade_b=None, unbreakable=True):
    return ([fx('bUnbreakableWeapon', 1)] if unbreakable else []) + [
        fx('bVariableCastrate', -5), sk(skill, 15), fx('bMatk', matk_step, step=3),
        magic(element, ele_step, step=3), sk(skill, 7, step=4), magic(element, 25, refine=7),
        fx('bVariableCastrate', -10, refine=9), fx('bSMatk', 5, refine=9), sk(skill, 15, refine=11),
        fx('bMatkRate', 5, grade=1), sk(skill, 10, grade=2),
        *(grade_b if grade_b is not None else [sk(skill, 10, grade=3)]), fx('bSMatk', 10, grade=4)]


weapon(610093, 'Sky_Slasher_Katar', 'Sky Slasher Katar', 'Assassin', 'Katar', 310, 200,
       crit_weapon('SHC_SAVAGE_IMPACT', 35, 3, [fx('bCritical', 5, grade=3), fx('bCRate', 7, grade=3)]),
       'Midgard_Katar_JP', 16, both=True)
weapon(500136, 'Sky_Thorns_Sword', 'Sky Thorns Sword', 'Alchemist', '1hSword', 220, 180,
       crit_weapon('BO_MAYHEMIC_THORNS', 30, 2, [fx('bVariableCastrate', -10, grade=3)]), 'Midgard_T_Sword_JP', 2)
weapon(550194, 'Sky_Deadsoul_Staff', 'Sky Deadsoul Staff', 'Wizard', 'Staff', 110, 160,
       magic_weapon('AG_SOUL_VC_STRIKE', 'Ghost', grade_b=[fx('bDelayrate', -10, grade=3)]),
       'Midgard_Staff_JP', 10, matk=230)
for id, aegis, label, subtype, resource, view, reach, gender in [
    (570093, 'Sky_Music_Viollin', 'Sky Music Violin', 'Musical', 'Midgard_Viollin_JP', 13, 1, 'Male'),
    (580093, 'Sky_Music_Whip', 'Sky Music Whip', 'Whip', 'Midgard_Whip_JP', 14, 2, 'Female')]:
    weapon(id, aegis, label, 'BardDancer', subtype, 220, 180,
           [fx('bPerfectHitAddRate', 5), sk('TR_ROSEBLOSSOM', 15), fx('bBaseAtk', 30, step=3),
            fx('bNonCritAtkRate', 2, step=3), sk('TR_ROSEBLOSSOM', 7, step=4),
            fx('bNonCritAtkRate', 25, refine=7), fx('bVariableCastrate', -10, refine=9),
            fx('bPAtk', 5, refine=9), sk('TR_ROSEBLOSSOM', 15, refine=11),
            fx('bAtkRate', 5, grade=1), sk('TR_ROSEBLOSSOM', 10, grade=2),
            fx('bVariableCastrate', -10, grade=3), fx('bPAtk', 10, grade=4)], resource, view, range=reach, gender=gender)
weapon(590116, 'Sky_Glacier_Mace', 'Sky Glacier Mace', 'Alitea', 'Mace', 100, 180,
       magic_weapon('AT_GLACIER_SHARD', 'Water'), 'Midgard_Mace_JP', 8, matk=230)
weapon(630066, 'Sky_Dragon_Spear', 'Sky Dragon Spear', 'Knight', '2hSpear', 370, 400,
       [fx('bPerfectHitAddRate', 5), sk('DK_DRAGONIC_BREATH', 15), fx('bBaseAtk', 35, step=3),
        fx('bLongAtkRate', 3, step=3), sk('DK_DRAGONIC_BREATH', 7, step=4), fx('bMaxHPrate', 25, refine=7),
        fx('bVariableCastrate', -10, refine=9), fx('bPAtk', 5, refine=9), sk('DK_DRAGONIC_BREATH', 15, refine=11),
        fx('bAtkRate', 5, grade=1), sk('DK_DRAGONIC_BREATH', 10, grade=2),
        fx('bVariableCastrate', -10, grade=3), fx('bPAtk', 10, grade=4)], 'Midgard_Lance_JP', 5, range=3, both=True)
weapon(650062, 'Sky_Frost_Humma', 'Sky Frost Huuma', 'KagerouOboro', 'Huuma', 110, 200,
       magic_weapon('SS_REIKETSUHOU', 'Water', 35, 3), 'Sky_Frost_Humma', 22, matk=330, both=True)
weapon(830047, 'Sky_Nightshot_Gatling', 'Sky Nightshot Gatling', 'Rebellion', 'Gatling', 310, 180,
       [fx('bPerfectHitAddRate', 5), sk('NW_THE_VIGILANTE_AT_NIGHT', 15), fx('bBaseAtk', 35, step=3),
        fx('bLongAtkRate', 3, step=3), sk('NW_THE_VIGILANTE_AT_NIGHT', 7, step=4), fx('bLongAtkRate', 25, refine=7),
        fx('bDelayrate', -10, refine=9), fx('bPAtk', 5, refine=9),
        sk('NW_THE_VIGILANTE_AT_NIGHT', 10, refine=11), fx('bPAtk', 5, refine=11),
        fx('bAtkRate', 5, grade=1), sk('NW_THE_VIGILANTE_AT_NIGHT', 10, grade=2),
        fx('bPerfectHitAddRate', 10, grade=3), fx('bPAtk', 10, grade=4)], 'Midgard_Gatling_Gun_JP', 19, range=9, both=True)
weapon(540125, 'Sky_Elemental_Book', 'Sky Elemental Book', 'Sage', 'Book', 100, 180,
       magic_weapon('EM_DIAMOND_STORM', 'Water', unbreakable=False), 'Sky_Elemental_Book', 15, matk=230)
weapon(550197, 'Sky_Seonang_Wand', 'Sky Seonang Wand', 'SoulLinker', 'Staff', 100, 180,
       magic_weapon('SOA_TALISMAN_OF_FOUR_BEARING_GOD', 'All',
                    grade_b=[fx('bMatkRate', 3, grade=3), fx('bSpl', 5, grade=3)]),
       'Sky_Seonang_Wand', 10, matk=230)


def combo(crown_name, weapon_names, base, learned, level, gated):
    conditions = dict(sum=24, crown_grade=4, weapon_grade=4, learned=learned, learned_level=level)
    for name in weapon_names:
        COMBOS.append(dict(names=[crown_name, name], effects=base +
                           [dict(deepcopy(e), conditions=conditions) for e in gated]))


combo('Sky_Rune_Crown_SHC', ['Sky_Slasher_Katar'],
      [sk('SHC_SAVAGE_IMPACT', 20), fx('bCritAtkRate', 20)], 'SHC_SAVAGE_IMPACT', 10,
      [fx('bSkillCooldown', -200, 'SHC_SAVAGE_IMPACT'), fx('bShortAtkRate', 10)])
combo('Sky_Rune_Crown_AG', ['Sky_Deadsoul_Staff'],
      [sk('AG_MYSTERY_ILLUSION', 40), sk('AG_ASTRAL_STRIKE', 40)], 'AG_ASTRAL_STRIKE', 10,
      [fx('bSkillCooldown', -200, 'AG_SOUL_VC_STRIKE'), sk('AG_ASTRAL_STRIKE', 20), magic('Neutral', 15)])
combo('Sky_Rune_Crown_BO', ['Sky_Thorns_Sword'],
      [sk('BO_MAYHEMIC_THORNS', 20), fx('bCritAtkRate', 20)], 'BO_MAYHEMIC_THORNS', 5,
      [fx('bSkillCooldown', -200, 'BO_MAYHEMIC_THORNS'), sk('BO_MAYHEMIC_THORNS', 10)])
combo('Sky_Rune_Crown_TR', ['Sky_Music_Viollin', 'Sky_Music_Whip'],
      [sk('TR_ROSEBLOSSOM', 20), fx('bNonCritAtkRate', 20)], 'TR_ROSEBLOSSOM', 5,
      [fx('bSkillCooldown', -150, 'TR_ROSEBLOSSOM'), fx('bLongAtkRate', 10)])
combo('Sky_Rune_Crown_AT', ['Sky_Glacier_Mace'],
      [sk('AT_GLACIER_NOVA', 45), magic('Water', 15)], 'AT_GLACIER_STOMP', 5,
      [fx('bSkillCooldown', -1000, 'AT_GLACIER_MONOLITH'),
       fx('bAutoSpellOnSkill', 1000, 'AT_GLACIER_MONOLITH|AT_GLACIER_STOMP|5')])
combo('Sky_Rune_Crown_DK', ['Sky_Dragon_Spear'],
      [sk('DK_DRAGONIC_BREATH', 20), fx('bMaxHPrate', 20)], 'DK_DRAGONIC_AURA', 10,
      [fx('bSkillCooldown', -150, 'DK_DRAGONIC_BREATH'), fx('bLongAtkRate', 10)])
combo('Sky_Rune_Crown_EM', ['Sky_Elemental_Book'],
      [sk('EM_TERRA_DRIVE', 45), magic('Earth', 15)], 'EM_TERRA_DRIVE', 5,
      [fx('bSkillCooldown', -300, 'EM_DIAMOND_STORM'), fx('bSkillCooldown', -1000, 'EM_PSYCHIC_STREAM'),
       fx('bAutoSpellOnSkill', 1000, 'EM_PSYCHIC_STREAM|EM_TERRA_DRIVE|5')])
combo('Sky_Rune_Crown_SS', ['Sky_Frost_Humma'],
      [sk('SS_RAIDENPOU', 45), magic('Wind', 15)], 'SS_REIKETSUHOU', 10,
      [fx('bSkillCooldown', -200, 'SS_RAIDENPOU'), fx('bAutoSpellOnSkill', 1000, 'SS_RAIDENPOU|SS_REIKETSUHOU|10')])
combo('Sky_Rune_Crown_NW', ['Sky_Nightshot_Gatling'],
      [sk('NW_MIDNIGHT_FALLEN', 45), fx('bNonCritAtkRate', 15)], 'NW_MIDNIGHT_FALLEN', 5,
      [fx('bAutoSpellOnSkill', 1000, 'NW_THE_VIGILANTE_AT_NIGHT|NW_MIDNIGHT_FALLEN|5'), fx('bLongAtkRate', 10)])
combo('Sky_Rune_Crown_SOA', ['Sky_Seonang_Wand'],
      [sk('SOA_CIRCLE_OF_DIRECTIONS_AND_ELEMENTALS', 40), magic('All', 15)], 'SOA_TALISMAN_OF_FOUR_BEARING_GOD', 5,
      [fx('bSkillCooldown', -250, 'SOA_TALISMAN_OF_FOUR_BEARING_GOD')])

# These four cross-sets are registered ONLY in druid_missing_weapon_combos.yml.
# They are included here for complete crown descriptions and drift verification.
EXTERNAL_COMBOS = [
    dict(names=['FuriousCirclet_AT', 'Axe_Furious'], effects=race(10)),
    dict(names=['FuriousCirclet_AT', 'Hall_Furious'], effects=[sk('AT_GLACIER_SHARD', 10)]),
    dict(names=['Time_DM_R_Crown_AT', 'Dimen_AT_Axe'], effects=[sk('AT_SAVAGE_LUNGE', 60),
         fx('bSkillAtk', 1, 'AT_SAVAGE_LUNGE', step=-1, crown_grade=4, weapon_grade=4),
         fx('bSkillCooldown', -200, 'AT_FRENZY_FANG', crown_grade=4, weapon_grade=4)]),
    dict(names=['Time_DM_R_Crown_AT', 'Dimen_AT_Knife'], effects=[sk('AT_TEMPEST_FLAP', 45), fx('bLongAtkRate', 15),
         fx('bSkillAtk', 1, 'AT_QUILL_SPEAR', step=-1, crown_grade=4, weapon_grade=4),
         fx('bSkillCooldown', -200, 'AT_QUILL_SPEAR', crown_grade=4, weapon_grade=4)]),
]

UNRESOLVED = {401195: 'Frontier_R_Crown_AT'}
assert len(ITEMS) == 23 and len(COMBOS) == 11
