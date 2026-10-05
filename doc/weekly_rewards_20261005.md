# Weekly adventures and guaranteed boss materials

Weekly practice now accepts any three of seven objectives: the existing 50
field/dungeon kills, one laboratory measurement, and build review remain valid.
Alternatives are 25 kills in the rotating region, visiting three rotating cities,
one eligible supported boss clear, and completing a supported boss with at least
two encounter-eligible challengers. A solo player can finish without the social
objective. Tracking remains opt-in. Monday 00:00 UTC (07:00 Bangkok) resets progress;
existing lifetime stamps remain intact. Five and twenty stamps display cosmetic
recognition on the board. Stamps do not buy materials or currency.

The three-week rotation is Payon fields plus Payon/Alberta/Izlude, Geffen fields
plus Geffen/Aldebaran/Prontera, and Morocc fields plus Morocc/Comodo/Umbala. City
credit requires a map-load event; repeatedly entering one city does not complete
three visits. Hunt credit excludes training maps, summoned slaves, and absent
mob units. Instance bosses use their authentic encounter reward path, not a
global monster ID event. The existing weekly kill counter still accepts general
field/dungeon kills, preserving the original completion path.

Alice's actual Normal/Hard boss clear and EDDA Bioresearch's Battle final boss
each earn one character-owned clear credit after the eligible character collects
the original clear reward. Bioresearch Story lacks the final boss and does not
earn credits. Each track permits one credit per character per admission day,
reset at 04:00 UTC (11:00 Bangkok), and each live instance permits only one credit.
The per-instance marker prevents an old completed encounter paying on a later
day; the persistent day marker prevents switching modes or instances for more
credits that day. Clear balances and lifetime counts persist in character
registries; no account transfer or automatic material claim is available.

Players choose their ingredient at purchase time from Boss reward progress in
My Adventure. Costs are deliberately small supplements to the existing rewards:

| Track | Ingredient | Credits per item |
| --- | --- | ---: |
| Alice | Small Sewing Kit (1001076) | 1 |
| Alice | Maple Leaves (1001083) | 1 |
| Alice | Heavy Chain (1001074) | 5 |
| Alice | Monster's Stone (1001082) | 10 |
| Bioresearch Battle | Somatology Experimental Fragment (25787) | 2 |
| Bioresearch Battle | Somatology Research Document (25786) | 5 |

Alice materials are the real Confused Boy recipe inputs. Bioresearch materials
are the existing Battle reward bundle's document and fragment. This adds no
equipment, cards, cash points, or Zeny and does not remove or reroll original
drops. Because any stored credits can buy any listed ingredient, players can
change goals freely. Each quantity costs its listed credits; the shop displays
the inventory output and the existing purchase history records the receipt.

The two named permanent-character pointshops opt into `pn_shop`'s durable asset
transaction in `npc_cashshop_buylist`: capacity planning occurs before dispatch;
the character point balance is compared and debited with inventory and the
receipt in one SQL transaction. Item delivery and visible balance changes wait
for the acknowledgement. Failed capacity or SQL writes do not spend credits;
replaying an accepted receipt cannot grant twice. The script ends when opening
the shop so the Adventure dialog cannot resume over it. No new migration is
needed beyond the existing deployed PN point/receipt schemas.

Validation: `weekly_rewards_test.py` executes actual user-function source in the
native script VM, using explicit clock/map/UI doubles; the extended Alice and
Bioresearch suites retain full encounter regression coverage. The extended
`point_shop_native_test.cpp` tests nonpet certainty outputs, capacity/balance
refusal and pending-before-ACK semantics. `point_asset_sql_cases.inc` injects
point/inventory/receipt SQL failures for both actual keys and tests commit/replay.
Rendered client acceptance is separate from these native and SQL checks.

Integration exports: `PN_WeeklyBoard`, `PN_RewardProgress` (both dialogs), and
`PN_RewardClearCredit(track)` (trusted instance hooks only, 1 Alice / 2 Bio).
`scripts_custom.conf` must load `reward_progress.txt` with the existing weekly
board. Clear hooks have no interaction yields between eligibility and counter
writes. Active map sessions defer script mutations while a durable purchase is
pending through the existing native transaction fence.
