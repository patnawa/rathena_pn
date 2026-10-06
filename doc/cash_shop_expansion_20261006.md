# Cash shop expansion - 6 October 2026

The shop grows from 97 to 168 listings: 36 more supplies and 35 directly purchased
costumes. All existing products and prices are preserved. Costumes are in Permanent;
combat consumables are in Consumables; refinement materials and utilities are in Other.

These are PN prices, not a claim of current official regional mall prices. Costume
headwear costs 2,500 Cash Points; garment costumes cost 5,000. Supply singles retain
the existing single-box price where one exists, or use the smallest current package
price divided by its main item quantity, rounded up. For packages containing several
products this uses the full package price without allocating a discount. The four
additional support scroll/card prices are PN convenience prices.

No item effects, box reward tables, player balances, or Fashion Point exchanges change.
All added usable items have existing nonempty scripts; ETC supplies use existing
refinement, revival, inventory expansion, gym or Malangdo services. Costume scripts
are either empty or existing appearance effects; the candidate with an EXP bonus was
excluded. Purchases grant the selected costume directly.

## Client validation

The installed Lua 5.1 item-info loader resolves all 168 item identities. Item icons
and collection artwork, both genders' headwear sprite/action pairs, and garment
sprite/action resources pass asset validation (2,127 distinct files). Existing effect
costumes retain their scripts. Five candidates with missing icons and headwear assets
were excluded: 31899, 19855, 31375, 19293, and 19670. No client patch is required.

A rendered gameplay check has not been performed. Native protocol purchase/equipment
results and activation status are recorded in OPS/cashshop-20261006 at the workspace
root. Its isolated fixture uses the deployed binary, a read-only game source mount,
a candidate cash-catalogue override, and synthetic accounts/database only.

## Native purchase validation

The real deployed map binary, with only the candidate catalogue mounted into a
private fixture, emits every expected ID, price, equipment location and preview field
for all 168 listings. Purchasing all 71 additions charges exactly 133,838 Cash Points
and saves one of each purchased item after logout. Representative costumes equip in
upper, middle, lower and garment slots. A purchase submitted under the wrong category
is refused without another cash charge. Production player data was not used.

## Production activation

### Catalogue refresh regression

A real native session reproduces the server refresh failure: the first `0x08c9`
request returns 168 items, while a second request and a request after closing and
reopening return zero. The session flag `cashshop_sent` suppressed subsequent
catalogue responses. The handler now sends the current catalogue on every explicit
request, while retaining the once-per-session sale notification. The regression
script is `tools/ci/cashshop_refresh_live_client.py`; it fails against the original
binary and returns all 168 matching item, price and preview fields in all three
cases against the repaired binary.

The private baseline rebuild reproduces the running map binary byte for byte.
Only `clif.cpp` is recompiled for the fix, and unrelated compiled objects are
unchanged. The retained build baseline is the source/object cache for the running
binary. Ten pre-existing differences in other live source files are preserved;
this fix does not incorporate them into the map binary. Native refresh validation
is separate from rendered PN-Client validation.

Activated on 6 October 2026 at 15:22 UTC (22:22 Bangkok). The map server loaded
all four catalogue categories and reached its explicit online state without startup
errors. All seven production services are healthy. The running map binary matches
the binary used by the isolated purchase fixture; the live catalogue contains 168
listings. Existing players were offline during the graceful map restart, all other
services retained their start times, and normal login access was restored.

The final activation receipt is `OPS/cashshop-20261006/expansion-deployment.json`
at the workspace root. The remote rollback backup is
`/app/pn-cashshop-20261006/production-backup-retry2`. Reopen the cash shop after
relogging to refresh PN-Client's cached catalogue.

The refresh fix was activated at 22:42 Bangkok on 6 October 2026. All seven services
are healthy; the running map SHA-256 is
`aeb0610bd9b30f987e8cc31d05ac22202db2b28ba229c61685b9feb9d14baea6`, matching
the passing private refresh and purchase fixtures. The catalogue remains at 168
items, map restart policy is restored, and normal login access is open. The other
six services retained their start times. The fix backup is
`/app/pn-cashshop-20261006/refresh-production-cutover/backup`; the portable receipt
is `doc/evidence/cash_shop_deployment_20261006.json`.

## Added products

| Item ID | Client name | Cash Points | Tab |
|---:|---|---:|---|
| 12684 | ASPD Enhanced Potion | 500 | Consumables |
| 12796 | Red Booster | 250 | Consumables |
| 23203 | Small Mana Potion | 500 | Consumables |
| 23204 | Brilliant Protection Scroll | 84 | Consumables |
| 23475 | Infinity Drink | 250 | Consumables |
| 14766 | Limited Power Booster | 250 | Consumables |
| 12883 | Almighty | 250 | Consumables |
| 102803 | Force Booster | 250 | Consumables |
| 102985 | Speed Booster | 250 | Consumables |
| 12208 | Battle Manual | 800 | Consumables |
| 14606 | JOB Battle Manual | 500 | Consumables |
| 12209 | Life Insurance | 120 | Consumables |
| 7621 | Token of Siegfried | 360 | Other |
| 12210 | Bubble Gum | 500 | Consumables |
| 14587 | Repair Weapon Scroll | 100 | Other |
| 14591 | WoE Teleport Scroll | 87 | Other |
| 12415 | Siege Map Teleport Scroll II | 87 | Other |
| 12214 | Convex Mirror | 200 | Other |
| 12221 | Megaphone | 200 | Other |
| 12213 | Neuralizer | 14700 | Other |
| 12278 | Alice Summon Book | 430 | Other |
| 12276 | Mimic Summon Book | 320 | Other |
| 12277 | Disguise Summon Book | 320 | Other |
| 6225 | HD Carnium | 1060 | Other |
| 6226 | HD Bradium | 1060 | Other |
| 6241 | HD Elunium | 1060 | Other |
| 6240 | HD Oridecon | 1060 | Other |
| 7619 | Enriched Elunium | 300 | Other |
| 7620 | Enriched Oridecon | 300 | Other |
| 25793 | Inventory Expansion Voucher | 2500 | Other |
| 7776 | Gym Pass | 2300 | Other |
| 6909 | Nyangvine | 250 | Other |
| 12211 | Kafra Card | 100 | Other |
| 12218 | Level 5 Assumptio Scroll | 15 | Consumables |
| 12219 | Level 10 Wind Walk Scroll | 15 | Consumables |
| 12220 | Level 5 Adrenaline Rush Scroll | 10 | Consumables |
| 480177 | Costume Clutch Bouquet | 5000 | Permanent |
| 420271 | Costume Cosmic Connection | 2500 | Permanent |
| 480235 | Costume Diabolic Friend | 5000 | Permanent |
| 20454 | Costume Crown of Ancient Queen | 2500 | Permanent |
| 410286 | Costume Evil Eye of the False God | 2500 | Permanent |
| 20532 | Costume Flaming Burst Wave(Garment) | 5000 | Permanent |
| 480117 | Costume Phen's Electric Guitar | 5000 | Permanent |
| 400343 | Costume Fox Ear Hat | 2500 | Permanent |
| 31495 | Costume Lolita Two Side Up | 2500 | Permanent |
| 19740 | Costume Guild Member Wanted Hat | 2500 | Permanent |
| 420034 | Costume Long Wave(Blonde) | 2500 | Permanent |
| 31531 | Costume Lovely Heart Cap | 2500 | Permanent |
| 31040 | Costume Magical Feather | 2500 | Permanent |
| 400057 | Costume Shadow Perm Hair | 2500 | Permanent |
| 21206 | Costume Nut Cracker Mask | 2500 | Permanent |
| 410277 | Costume Ragdoll | 2500 | Permanent |
| 400073 | Costume Romance Rose | 2500 | Permanent |
| 31383 | Costume Volume Low Twin | 2500 | Permanent |
| 31315 | Costume Stall of Angel | 2500 | Permanent |
| 5909 | Costume Valkyrie Circlet | 2500 | Permanent |
| 420165 | Costume Tent | 2500 | Permanent |
| 400056 | Costume Blue Wave Long Hair | 2500 | Permanent |
| 400149 | Costume Black Thunder | 2500 | Permanent |
| 480362 | Costume White Nine-Tailed Fox Tail | 5000 | Permanent |
| 480207 | Costume Traveler's Universal Bag | 5000 | Permanent |
| 410081 | Costume Released Ground | 2500 | Permanent |
| 410132 | Costume Tiger(Black) | 2500 | Permanent |
| 420107 | Costume Twinkle Twin | 2500 | Permanent |
| 420047 | Costume Honorable Knight Cloak | 2500 | Permanent |
| 400214 | Costume Faith of Yggdrasil | 2500 | Permanent |
| 400272 | Costume Hakuba's Wish | 2500 | Permanent |
| 19605 | Costume Gang Scarf | 2500 | Permanent |
| 436008 | Costume Big Tiger Hat | 2500 | Permanent |
| 436006 | Costume Rgan Disguise Tool | 2500 | Permanent |
| 440010 | Costume Teddy Bear Mask | 2500 | Permanent |
