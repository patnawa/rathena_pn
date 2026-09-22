# Time Dimensions crown and weapon audit — 2026-09-22

The audit found two additional server data defects beyond Elemental Master's
autocast bug: Meister's Dimensions Axe disabled its autocast when the equipment
was calculated while mounted, and the magical Troubadour/Trouvere set granted
half the documented refine-based Reverberation bonus. The client also contained
incorrect autocast descriptions on 27 original Dimensions weapons. These are
separate server and client findings; updating the tooltip alone does not repair
combat behavior.

## Scope and evidence

Inspected 19 Time Dimensions crown variants, 40 Dimensions weapons (38 original
weapons and two Alitea weapons), and the 21 Time Gap weapons associated with the
same crowns. The original Dimensions sets occupy 36 shared combo scripts; the
Time Gap sets occupy 18; the Alitea overlay adds two. Two Time Gap dagger sets
require both daggers. Troubadour/Trouvere share scripts across their instruments
and whips.

All 27 original weapon records that advertise an autocast were checked against
Gravity's current Korean item descriptions. The two additional Time Gap autocast
weapons, 530060 and 550159, were also checked. The other weapon records and the
Alitea overlay were checked for combo membership, stated versus scripted skill
effects, and presence of autocasts; this is not a complete numerical balance
audit against Gravity for every non-autocast bonus.

Local sources:

- `db/re/item_combos.yml`, Dimensions sets beginning near line 46083 and Time Gap sets near lines 51106 and 53400.
- `db/re/item_db_equip.yml`, original crown records 400529–400546.
- `db/import/druid_missing_crowns.yml` and `db/import/druid_missing_weapon_combos.yml`, Alitea crown 400999 and weapons 510193/620059.
- `PN-Client/SystemEN/LuaFiles514/itemInfo.lua`, loaded through the installed full item loader.
- `src/map/skill.cpp`, `src/map/pc.cpp`, `src/map/status.cpp`, and individual skill implementations for dispatch, self/ground targets, skill learning, and changing summon/mount state.

## Confirmed server findings

1. **Meister, weapon 620037:** the combo conditioned registration on
   `!checkmadogear()`. Both `NC_AXETORNADO` and `MT_MIGHTY_SMASH` explicitly have
   `AllowOnMado: true` in `db/re/skill_db.yml`. The current
   [Gravity description](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=620037&itemSeq=1)
   requires learning Axe Tornado and having both items at Grade A; it has no
   mount exclusion. Removing the equipment-time mount guard restores the proc
   while mounted and avoids stale registration after a mount transition.
   `pc_setoption` only starts/ends Madogear, whose calculation flag is Speed;
   empty bonus-script cleanup does not trigger a full equipment calculation.

2. **Troubadour/Trouvere, weapons 570062 and 580061:** the Grade A set's
   `bSkillAtk,"WM_REVERBERATION",.@sum` must use `2*.@sum`. Both
   [Gravity's violin](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=570062&itemSeq=1)
   and [ribbon](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=580061&itemSeq=1)
   specify twice the combined refine levels. The installed client already stated
   the correct multiplier.

3. **Elemental Master, weapons 540079 and 540080:** equipment-time elemental
   checks must not decide whether the proc remains registered after a summon
   changes. The actual cast must validate the current high elemental and route
   Elemental Buster correctly. The main fix and its native regression tests own
   this diagnosis. [Gravity's magic book](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=540079&itemSeq=1)
   and [spellbook](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=540080&itemSeq=1)
   both require a summoned high elemental and learned Elemental Buster.

The target-skill `getskilllv(...) > 0` guards are intentional. Gravity's
descriptions consistently require the autocast skill to have been learned at
least at Level 1, then use the highest learned level. The old English fallback
wording is a tooltip defect; granting unlearned skills would change the rules.
The fixed Level 5 Hyper Novice proc and the two explicit 50% proc rates are
exceptions that remain intact.

## Coverage by crown

All listed procs require Grade A on both the weapon and matching crown. Learned
target level is required except for Hyper Novice's fixed Level 5 Soul Vulcan
Strike. “No proc” refers to the Dimensions variant in this row; it does not mean
that the job has no other autocast equipment.

| Crown / job | Dimensions weapons | Autocast routing reviewed |
| --- | --- | --- |
| 400529 Dragon Knight | 600054, 630041 | Hack and Slasher → Ignition Break; lance has no proc |
| 400530 Imperial Guard | 500092, 530054 | Overslash → Overbrand; sword has no proc |
| 400531 Meister | 590079, 620037 | Mighty Smash → Axe Tornado; mace has no proc; erroneous Mado guard identified |
| 400532 Biolo | 500093, 590080 | Explosive Powder → Cart Tornado; sword has no proc |
| 400533 Shadow Cross | 610064, 610065 | Phantom Menace → Rolling Cutter → Impact Crater; katar has no proc |
| 400534 Abyss Chaser | 510139, 700092 | Deft Stab → Abyss Dagger; bow grants Rose Blossom instead |
| 400535 Arch Mage | 640049, 640050 | Crimson Arrow → Storm Cannon; Rock Down → Frozen Slash |
| 400536 Elemental Master | 540079, 540080 | Diamond Storm / Terra Drive → Elemental Buster, with current high elemental |
| 400537 Cardinal | 540081, 550130 | Arbitrium → Adoramus; bible grants Unlucky Rush instead |
| 400538 Inquisitor | 560060, 560061 | First Brand → Second Flame → Third Flame Bomb; claw has no proc |
| 400539 Windhawk | 700093, 700094 | Crescive Bolt → Sharp Shooting; Hawk Rush → Hawk Boomerang (50%) |
| 400540 Troubadour / Trouvere | 570062, 570063, 580061, 580062 | Rhythm Shooting → Sound Blend; Metallic Fury → Reverberation; multiplier defect identified |
| 400541 Shinkiro / Shiranui | 650046, 650047 | Cross Slash → Shadow Slash; Red Flame Cannon → Thundering Cannon |
| 400542 Night Watch | 810040, 840032 | Only One Bullet → Spiral Shooting; Spiral Shooting → Wild Fire |
| 400543 Sky Emperor | 540082, 540083 | Dawn Break → Midnight Kick; Sunset Blast → Noon Blast |
| 400544 Soul Ascetic | 550131, 550132 | Blue Dragon → Red Phoenix; White Tiger → Black Tortoise |
| 400545 Hyper Novice | 500094, 550133 | Napalm Vulcan Strike → Level 5 Soul Vulcan Strike (50%); sword grants Cross Impact instead |
| 400546 Spirit Handler | 550134, 550135 | Neither Dimensions weapon advertises a proc |
| 400999 Alitea | 510193, 620059 | Neither Dimensions weapon advertises a proc; damage/cooldown sets match the reviewed local overlay |

The additional Time Gap procs are Radiant Spear → Cannon Spear on 530060 and
Chulho Sonic Claw → Chulho Battering on 550159. Their client wording already
matches the learned-skill requirement and the server routes. Sources:
[Time Gap Guardian Spear](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=530060&itemSeq=1),
[Time Gap Spirit Foxtail](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=550159&itemSeq=1).

## Client corrections and verification

`client-patch/time_dimensions_tooltips/corrections.json` records exact before and
after strings and a Gravity source URL for every changed weapon. The Lua module
corrects 58 lines across 27 records (57 distinct replacements because one
condition line occurs twice).

- Replace the misleading unlearned-Level-1 fallback and vague chance wording with highest learned level and explicit learning requirements.
- Retain Hyper Novice's fixed Level 5 proc and Windhawk's 50% proc.
- State Elemental Master's high-elemental requirement.
- Correct Soul Rod 550132 from Blue Dragon to White Tiger as the trigger. [Gravity source](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=550132&itemSeq=1).
- Correct Night Launcher 840032 to Spiral Shooting → Wild Fire and the weapon type to Grenade Launcher. [Gravity source](https://ro.gnjoy.com/guide/runemidgarts/itemview.asp?ID=840032&itemSeq=1).
- Correct Meister Axe 620037's unconditional set label from Axe Tornado +45% to Axe Stomp +45%.

The real Lua 5.1 verification executes the entire installed loader as the
baseline and the full candidate loader, checks every expected old line, compares
the entire resulting item table against only the reviewed replacements, and
checks that applying the module twice has no extra effect. Result:

```text
PASS: 27 weapon tooltips, 58 corrected lines; full loader and unchanged item metadata verified
```

The full signed client candidate also passed the ordinary quick and full client
checks and `PNLauncher.exe --verify-manifest`. The four-file ZIP contains only
the item loader, new tooltip module, release metadata, and client manifest.
The signed feed preserves every unrelated entry and its preservation policy.

Artifacts are outside the repository at
`Server-Development/time-dimensions-20260922`: `candidate.json`, `release.json`,
`release-candidate`, and `PN-Client-Update-20260922-Time-Dimensions.zip`.
The candidate sequence is 2026092201, based on signed sequence 2026092105.
The ZIP SHA-256 is
`ab67b382287c14464104200b33587003b62286ceccf16ce9308b36b64db60540`.
Preparation does not publish or install it; publication and installation have
separate receipts.

This audit establishes source/data and real Lua-loader coverage. It does not
claim a manual gameplay cast of every job. Server native dispatch/equipment
regressions and production health checks are recorded by the main repair task.
