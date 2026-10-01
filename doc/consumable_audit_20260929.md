# Consumable audit — 2026-09-29

Scope: the active cash-shop catalogue, its box rewards, four previously unimplemented
booster variants, and matching client tooltips. kRO's public item mall is the baseline;
Thai-specific promotions are reviewed separately. This is not a certification of every
consumable or every current regional mechanic in Ragnarok Online.

## Confirmed repairs

- Force Booster (102803) and event variant (103273): implement all six trait stats +5,
  P.ATK +10 and S.MATK +10 for 30 minutes, ending on death.
- Speed Booster (102985) and event variant (103272): implement FLEE +50, ASPD +1,
  30-minute movement haste, ending on death. Haste uses the existing non-stacking
  25% walk-delay reduction shared with Increase Agility/Moonlight Flower-style effects.
  The public official description does not disclose a numeric movement rate; 25% is
  an implementation assumption, not an independently verified current kRO measurement.
- Power Booster: fix the status script's SP-cost bonus from +5% to -5%. Other bonuses
  and existing status flags remain unchanged; the fix also applies to variants using
  the same status.
- Timed bonus refresh: synchronize the expiry stored in the bonus entry with the timer.
  Previously repeat use rescheduled the timer but left the comparison deadline stale.
  The save path already reads the active timer, so this was not a relog persistence bug.
- Restore Alice, Mimic, and Disguise mercenary boxes, mistakenly removed as cosmetics.
  Their 30-minute summon scripts, mercenary IDs, levels, and skill lists match the
  official descriptions. Hair and dye products remain absent.
- Enable four completed Force/Speed packages at the official kRO prices: 10 boosters
  and one Kachua key for 2500 points; 100 boosters and 11 keys for 25000 points.
- Correct client descriptions for HE Battle Manual and its box (+200% experience,
  rather than total 200%), and Almighty (persists after death).

## Evidence

Official pages checked during this audit:

- [Force Booster](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=49870&productKey=825bb1e7c92491ab8782ecf264c6b0ce)
- [Speed Booster](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=49949&productKey=19c04f7f20617a3d4a8925e2931ad91b)
- [Power Booster](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=31947&productKey=6fe7bebc2a50767bf9b449039031a736)
- [Alice mercenary](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=10313&productKey=4736fed1704ca049adaeb35c7efbf242)
- [Mimic mercenary](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=10315&productKey=8df861827eeac4dc132fb7c3a321bbee)
- [Disguise mercenary](https://mall.gnjoy.com/joyshop_new/item.grv?productNum=10317&productKey=41b0e49a1eabb1a2721a4f4586f50784)

The earlier 89-product official snapshot is retained in cashshop-20260929/official-details.json.
Box reward comparisons preserve the previously corrected Shadow Armor Scroll quantity
(30) and Siege Teleport II box reward (30 of item 12415). Kachua's seasonal prize pool
is not updated by this audit. Shop prices of the 90 existing listings are unchanged.

## Verification and limits

- 97 unique shop listings; 144 reachable item identities; no missing client entries
  or empty usable reward scripts. Inventory and effective database snapshots retained.
- Native offline VM executes the actual booster item scripts and Power Booster status
  script against real bonus arithmetic, lifecycle, and production movement formula.
  Five cases / 116 assertions, UBSan, no network, no live accounts, clean allocator exit.
- Regression mutations reject missing Force/Speed effects, reversed SP-cost sign,
  and additive movement stacking. Captured pre-fix native execution fails on stale internal expiry metadata.
- All 27133 client identities/artwork references preserved; only six description lines
  across three items (identified/unidentified) change.
- A separate isolated SQL/map startup validates the final candidate binary and database.
  Full in-client purchase/use, packet status icons, and official-server empirical timing
  have not been tested. Boosters use timed bonus scripts without dedicated official icons.

Deployment completed: all seven services healthy, other services unchanged, account
balances unchanged. Client release `client-20260929-consumables` (sequence 2026092903)
is signed, published, and installed locally. Server rollback files are retained under
`/app/pn-consumable-audit-20260929/deployment/before`.

Still unavailable, and not advertised for sale: Thai Full Chemical Protection Scroll
(no corresponding server/client item identity), regional Golden Angeling Coin packages,
Zonda and bell services, subscription service, name/stat reset coupon redemption, and
event passes. Those require complete item/service implementations, not just shop entries.
The current Thai FCP description is documented at
https://ro.gnjoy.in.th/cash-shop-update-full-chemical-protection-scroll-10-box/.
