# Taiwan costume stone primary audit — 2026-09-28

The current Taiwan client contains seven physical costume-enchant materials absent from the 490-entry integration catalogue at the start of this check. This is a bounded Taiwan client audit, not a claim that every regional Ragnarok catalogue is complete.

## Primary source and finite scan

- [Official Taiwan patch manifest](http://twcdn.gnjoy.com.tw/ragnarok/patchinfo/patch3.txt), fetched September 28, ends at patch 9348. The latest item metadata is patch **9347**, September 22.
- [Official patch 9347](http://twcdn.gnjoy.com.tw/ragnarok/patchfile/2026-09-22_live_client_4412_4412_1789972792.rgz): 3,336,927 compressed bytes; extracted `System/iteminfo_new.lub` is 9,194,303 bytes. All **21,138** native item records were evaluated with Lua 5.1; names and descriptions are UTF-8, resources were converted to CP949 only for the English client's asset paths.
- `costume_taiwan_official_20260928.json` records the source hash, scan predicate, 360 physical material candidates, and exact native descriptions for the seven new pairs. A second broader pass over position-named stones and Malangdo effect-enchanter NAVI entries found no further physical materials. Excluded entries are boxes, actual costumes and enchant cards. This lexical scan has an explicit boundary and cannot prove that a differently named item does not exist.
- [Official exchange catalogue](https://ro.gnjoy.com.tw/notice/Guide_View?id=217680) states its last added entries are from August 18, 2026. It corroborates the general catalogue but does not include every event-exclusive regional stone.

## Additional native pairs

| Material | Enchant | English description | Costume position |
| --- | --- | --- | --- |
| 1001736 | 300548 | Melody Note Effect | Upper, fourth slot |
| 1001752 | 313331 | Released Power Effect | Lower, fourth slot; corrected below |
| 1001868 | 313524 | 22nd Anniversary | Garment, second slot |
| 1002293 | 314198 | Golden Aura Effect | Lower, fourth slot |
| 1002420 | 300713 | 23rd Anniversary | Garment, first slot |
| 1002464 | 300714 | Dark Lord Circle Effect | Lower, fourth slot |
| 1002585 | 314802 | Digital Space Effect | Lower, fourth slot |

Native pair identification uses corresponding material/enchant names and descriptions in the same official itemInfo, with official event pages corroborating the positions. It is not copied from a secondary database.

The [2024 spring spending event](https://ro.gnjoy.com.tw/notice/notice_view.aspx?id=216669) explicitly identifies Melody Note as upper/fourth slot. The [official EP19 reward page](https://event.gnjoy.com.tw/Ro/ACT_20240508_ep19/rare) explicitly identifies Released Power as lower/fourth slot, agreeing with its native item name; the current client description's upper position is stale. The [23rd anniversary event](https://event.gnjoy.com.tw/Ro/ACT_20251013_23rd/Event06) and [official effects notice](https://ro.gnjoy.com.tw/notice/notice_view.aspx?id=217913) verify Dark Lord lower/fourth slot.

## Functional effects and corrections

23rd Anniversary grants fixed cast -0.2 seconds, after-cast delay -3%, CRI +5, melee/ranged physical damage +3%, all-element magic damage +3%, and variable cast -5%. The material description includes the last bonus, while the enchant description omits it. The [November 4, 2025 official correction](https://ro.gnjoy.com.tw/notice/notice_view.aspx?id=217963) explicitly fixes the missing variable-cast -5% implementation, resolving the discrepancy. The [October 21 maintenance notice](https://ro.gnjoy.com.tw/notice/notice_view.aspx?id=217925) separately fixes its variable-cast-stone set combo. All eight described set bonuses are translated into the proposal and English metadata; the three-position sets require all three named stones.

22nd Anniversary's current description grants its halo and +11% melee/ranged physical and all-element magic damage. Do not restore its obsolete temporary +22%/shadow-equipment event bonuses: the [January 7, 2025 maintenance notice](https://ro.gnjoy.com.tw/notice/notice_view.aspx?id=217197) removes the event abilities, and the current native metadata contains only the permanent effects. Both anniversary materials remain official rewards in the [2026 New Year event](https://event.gnjoy.com.tw/Ro/ACT_20260202_horseYear/eventMary).

## Implementation handoff and limits

`costume_taiwan_client_metadata_20260928.json` has 14 translated material/enchant entries with resource bytes. The audit-work `taiwan_proposal.json` has 14 proposed records, seven pairs and eight 23rd-anniversary combos. Internal Aegis names are locally proposed; official itemInfo supplies IDs, display names, resources and descriptions, not server Aegis identifiers.

At this handoff, the proposal deliberately leaves visual scripts pending: official item descriptions identify the effect, but do not themselves encode the server `hateffect` numeric ID. Native effect-table/name and asset verification is tracked by the cosmetics researcher. In particular, Dark Lord has two existing distinct client effects (Cloak 185 and Manteau 187); their names alone do not justify choosing one. 22nd Anniversary's generic halo is likewise not uniquely resolved by its description. Do not enable scriptless visual cards as completed implementations.

The 23rd ranged set currently references the catalogue's existing middle material 25061 → enchant 29048. Native enchant 310330 has another Range Middle label; both are not interchangeable solely because of their names. The final integration must avoid either missing the locally obtainable version or granting two set bonuses if aliases are supported.


## Final evidence review

The cosmetics review resolved five native visual mappings, with inference explicitly distinguished from direct metadata: Melody Note → `HAT_EF_C_MELODY_WING` 173 (official screenshot's exact music-note shapes/colors), Released Power → `HAT_EF_C_RELEASED_GROUND` 161 (unique same-name costume effect), Golden Aura → `HAT_EF_GOLDEN_AURA_TW` 278 (unique regional name and recovered official assets), Digital Space → `HAT_EF_DIGITAL_SPACE` 87 (unique native name, built-in effect 1240), and Dark Lord Circle → `HAT_EF_C_DARK_LORD_CLOAK` 185 (official flame-ring/triangle-star visual match, distinct from Manteau 187). See the cosmetic report and the explicit `effect_evidence` fields in the Taiwan JSON. None is represented as an official server item-to-effect script dump.

**22nd Anniversary halo remains unresolved.** The [official anniversary landing page](https://event.gnjoy.com.tw/Ro/ACT_20241008_22th/) links directly to the [22nd Anniversary shop](https://ro.gnjoy.com.tw/notice/Scroll/index.html#22Anniversary_pack). Its official JSON API `/Notice/forAjax_ScrollSubInfo`, request `{"SN":"22Anniversary_pack"}`, was recovered and retained in audit-work `tw-22anniv-promo.json`. All its promotional images were inspected; the stone appears in images 9341/9342 only as text tooltips. They show no equipped halo and no effect identifier. Current native effect tables likewise provide no unique 22nd-anniversary association. This bounded focused check therefore supports retaining the verified permanent numeric stone and clearly disclosing the unsupported halo. No speculative effect was selected.

The Range Middle implementation was reviewed against the [April 7, 2022 official notice](https://ro.gnjoy.com.tw/notice/Guide_View?id=216589). The notice expressly changes the material's enchant outcome from Expert Archer to Range Middle, preserves its base +3% ranged damage, and enables the upper/middle/lower set's extra +6%. Accordingly, catalogue `25061 → 310330` is correct. Legacy enchant 29048 is retained for recovery and compatibility. The three added legacy combos (+6% ranged set, +2% ranged dual, P.ATK +1 / CON +1 ranged-physical garment) and the 23rd-anniversary legacy +5% set all require 29048 specifically in the costume-middle stat slot (card index 1). This excludes ordinary equipment's Expert Archer enchant. In NPC-produced loadouts, the old and new middle cards occupy the same single slot, so these guarded aliases do not duplicate the native new-card combos. No discrepancy was found in this semantic review; it is not an in-game runtime test.
