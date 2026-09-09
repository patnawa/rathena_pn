# MuhRO hotfix comparison: #25?49

Reviewed 26 first posts (number 28 occurs twice) from the provided category, captured in topics/*.json. Repository paths are relative to rathena_pn_push; live-sensitive NPCs were read from the fresh audit/live snapshot. Readable source summaries below are paraphrases. A mention of no confirmed defect is not a full gameplay certification.

## [Hotfix #25 ~ 10 January 2025](https://dis.muhro.eu/t/hotfix-25-10-january-2025/3027)

Reviewed live HallOfLife.txt. Script uses purple damage portals and green buff portals, percentage-based explosion damage (MaxHp / 5 * level), explicit difficulty bands for regeneration. No old +10,000 constant or off-by-one level index found. Muh note lacks final numeric specification; do not overwrite the independently implemented instance to guess parity.

## [Hotfix #26 ~ 10 January 2025](https://dis.muhro.eu/t/hotfix-26-10-january-2025/3028)

Garden vending machine has tools/storage/save, but no healing option. Healing is a Muh feature addition. Inspection independently confirmed savepoint case falls through to storage, charges 100 zeny and opens storage; fixed the savepoint case to terminate before storage/payment, in both repository and a separate exact-live candidate. The live candidate differs by only that one break-to-end edit; control-flow checks preserve all three storage paths and the tool path.

## [Hotfix #27 ~ 11 January 2025](https://dis.muhro.eu/t/hotfix-27-11-january-2025/3034)

NPC weapon sell-price rebalance explicitly follows Muh custom drop-rate increases. No exact replacement prices published; economy differences are not a demonstrated bug. Leave PN prices unchanged.

## [Hotfix #28 ~ 15 January 2025](https://dis.muhro.eu/t/hotfix-28-15-january-2025/3052)

Rental-box topic 3069 assigned to root DB reviewer for effective item-group duration checks. Separate topic 3052 concerns golden Archangel garment art on Priest: client assets, no server mechanics specified; no invented server fix.

## [Hotfix #28 ~ 15 January 2025](https://dis.muhro.eu/t/hotfix-28-15-january-2025/3069)

Rental-box topic 3069 assigned to root DB reviewer for effective item-group duration checks. Separate topic 3052 concerns golden Archangel garment art on Priest: client assets, no server mechanics specified; no invented server fix.

## [Hotfix #29 ~ 18 January 2025](https://dis.muhro.eu/t/hotfix-29-18-january-2025/3078)

Infinite Fly Wing duplicate rental rewards assigned to root DB reviewer for effective item-group validation.

## [Hotfix #30 ~ 25 January 2025](https://dis.muhro.eu/t/hotfix-30-25-january-2025/3111)

Unspecified client Unknown messages; no item IDs or reproduction. Not sufficient to identify a matching defect. Previous navigation/item-name repairs are not evidence that every unknown message is solved.

## [Hotfix #31 ~ 26 January 2025](https://dis.muhro.eu/t/hotfix-31-26-january-2025/3117)

Live HallOfLife.txt already detects Rigel leaving sanctuary bounds each second and calls OnBreakSanctuary/S_ClearSanctuary, then recomputes damage immunity from remaining sanctuary/member state. Automic Module boxes delegated to DB review.

## [Hotfix #32 ~ 02 February 2025](https://dis.muhro.eu/t/hotfix-32-02-february-2025/3149)

Gender Change Coupon note provides no ID or failed condition. Compare effective item script with supported native changesex instead of guessing a replacement; deferred DB disposition.

## [Hotfix #33 ~ 07 February](https://dis.muhro.eu/t/hotfix-33-07-february/3170)

Signet perfect enchants delegated to DB reviewer; previous enchant audit repaired native selection but current effective six-trait definitions must be verified.

## [Hotfix #34 ~ 02 March](https://dis.muhro.eu/t/hotfix-34-02-march/3258)

Rune tablet login behavior already present: npc/custom/rune_tablet/bonuses.txt OnPCLoginEvent calls PN_RT_Refresh and recalculatestat; no first-map-change dependency. No ACMD_FUNC(getall) implementation found (only unrelated iterator names); Muh custom command not imported.

## [Hotfix #35 ~ 02 March](https://dis.muhro.eu/t/hotfix-35-02-march/3276)

mob.cpp KS protection explicitly allows matching nonzero party IDs in party mode and skips allowks maps. Status restrictions need the named effect/reproduction; broad note does not prove PN Hiding/PlayDead broken.

## [Hotfix #36 ~ 08 March](https://dis.muhro.eu/t/hotfix-36-08-march/3279)

Default KS-protection/settings policy is Muh-specific. Existing mob.cpp honors owner state.noks and MF_ALLOWKS; no settings NPC named in note. Do not silently change default combat policy.

## [Hotfix #37 ~ 08 March](https://dis.muhro.eu/t/hotfix-37-08-march/3283)

Unspecified never-ending buffs: native status timers and save/reload were inspected. Confirmed separate 50-status restore truncation is #44; this note provides no affected buff IDs to validate perpetual-duration semantics.

## [Hotfix #38 ~ 21 March 2025](https://dis.muhro.eu/t/hotfix-38-21-march-2025/3313)

Six nonspecific runtime/client/homepage fixes; no skills/items/monsters or reproduction supplied. Cannot claim a matching fix. Homepage and costume display are external to this emulator tree.

## [Hotfix #39 ~ 21 March 2025](https://dis.muhro.eu/t/hotfix-39-21-march-2025/3320)

Muh-specific removal of monster AI optimizations; no patch or optimization identification. PN native source retains normal rAthena mob AI. No speculative rewrite.

## [Hotfix #40 ~ 22 March 2025](https://dis.muhro.eu/t/hotfix-40-22-march-2025/3324)

Drop/sell values are Muh economy choices. PN mobskill_use supports random skill-start via monster_ai 0x100, while local config deliberately has 0 (sequential official priority). Full-skill variety is configurable, not enough to prove a bug. No policy change.

## [Hotfix #41 ~ 21 June 2025](https://dis.muhro.eu/t/hotfix-41-21-june-2025/3508)

Live episode21 scripts contain explicit getexp rewards (Progression.txt, MysteriousGhostShip.txt etc). Card text, seasonal loading screens and website listings are separate data/products. Note lacks quest IDs/final zeny amounts; no unsupported blanket reward additions.

## [Hotfix #42 ~ 22 June 2025](https://dis.muhro.eu/t/hotfix-42-22-june-2025/3519)

Fishing cooldown and summer race scoring are Muh seasonal systems; no matching Summer Island race implementation identified. Not imported as server bugfixes.

## [Hotfix #43 ~ 09 July 2025](https://dis.muhro.eu/t/hotfix-43-09-july-2025/3564)

Confirmed missing common busy-state Rodex guard: mail_invalid_operation only checks MF_NORODEX; inbox/read/claim use it. Begin-write separately blocks storage/trade but not npc_id. Root notified to repair and test native cross-feature restrictions.

## [Hotfix #44 ~ 31 August 2025](https://dis.muhro.eu/t/hotfix-44-31-august-2025/3669)

CONFIRMED BUG: char_mapif.cpp chmapif_parse_askscdata hard-caps SQL restore at 50 entries despite map chrif_save_scdata saving all statuses. Matches buffs lost upon relog with many effects. Root owns fix/native tests. Muh @partyloot/reward-map feature is distinct and not automatically imported.

## [Hot Fix #45 ~ 05 September 2025](https://dis.muhro.eu/t/hot-fix-45-05-september-2025/3683)

Uniform ASPD-dependent walk delay and Mega Dodgeball hardmode are policy/event changes. No exact formula or defect trigger; leave native per-skill walk behavior unchanged.

## [Hotfix #46 ~ 28 September 2025](https://dis.muhro.eu/t/hotfix-46-28-september-2025/3736)

PN YuNa#pront (npc/cities/prontera.txt) is stock Odin dialogue. Muh note gives no condition/text; no actionable matching defect.

## [Hotfix #47 ~ 30 October 2025](https://dis.muhro.eu/t/hotfix-47-30-october-2025/3829)

Autumn/wedding achievements and EU proxy replacement are additions/infrastructure, not evidence of a PN gameplay bug.

## [Hotfix #48 ~ 22 November 2025](https://dis.muhro.eu/t/hotfix-48-22-november-2025/3903)

No Slotty Enchanty NPC in PN tree. Analogous native clif_parse_enchantwindow_reset mutates card enchant slots and preserves selected_item.refine; previous enchant fixes are retained. No matching reset-refine loss found.

## [Hotfix #49 ~ 12 December 2025](https://dis.muhro.eu/t/hotfix-49-12-december-2025/3976)

Muh Turbo Config compatibility is its client launcher feature. No matching framework in emulator source; no server fix.

## Additional assigned repairs (#52, #57)

CH1.c originally checked dimensional resistance only at entrances. Added CH1_DimensionalGuard for hem_dun02 (SC_CONTENTS_37) and ch1_gfn01/ch1_gfn03 (SC_CONTENTS_38): immediate load/relog check plus one-second player timer; mismatched/expired protection returns to the relevant hem_fild entrance, and leaving the maps stops the timer. Existing entry/quest/level requirements remain. ch1_gfn02 does not exist in live map cache and was excluded.

Added amicitia2 nomemo to loaded npc/re/mapflag/nomemo.txt to prevent /memo bypass of the level-gated entrance. Both edited source files matched the fresh live snapshot before changes, allowing only appended bytes without unrelated rewrites.

Validation: native startup performed by root; chapter1_protection_test.py/.cpp execute the exact extracted guard body in the actual native parser/VM and getstatus/strcharinfo builtins, with explicit world/timer/message doubles. Tests cover all four protection combinations across three restricted and two ordinary maps, expiration, duplicate-load cancellation, and exit coordinates. Native execution result is recorded by root; this document does not predeclare a passing result. No live player was used for the harness.

## Garden savepoint validation

Both repository and live-specific candidates were checked: tool path terminates; storage cases 2?4 still continue to the common payment/openstorage path; savepoint case terminates before both. The live candidate is byte-for-byte identical to the snapshot except the single break-to-end statement. Native full-file startup/parse is performed by root; no player interaction was simulated for this simple control-flow correction.

Final coordinator result: native Chapter 1 guard passed 20 cases / 137 assertions under UBSan; all eight fixes are deployed. See the main audit report.
