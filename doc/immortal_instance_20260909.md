# The Immortal instance

Warper → Instances → The Immortal leads to the World Tree Branch at
`jor_twig,117,145`. Episode 20 completion is required. The existing internal
instance name, `The Undying`, and its 30-minute lifetime remain compatible with
the existing entry helper. Normal and Hard retain separate daily rewards,
resetting at 04:00 server time.

The repeat encounter uses mob 31900 with the existing Lasgand appearance.
Story mob 21981 and its drops are unchanged. The opening lasts three seconds:
movement, attacks and casting are disabled; loaded `SC_ARMOR` divides combat
damage by 1,000. The controller then removes that status and restores movement
with 10% damage taken in Normal or 1% in Hard. Normal retains 1 billion HP;
Hard retains 2 billion HP.

Four movable visions (31901) appear north, south, east and west. They use Rain
of Meteor, Ground Drive and Storm Gust, with shorter cast delays nearer the
boss. Exact vision HP and timing were not specified by the encounter reference:
this implementation uses 100 million HP and additional cast delays of 1, 3 or
5 seconds at distances up to 5, up to 10 or beyond 10 cells. Visions provide no
EXP or loot and disappear when the boss dies. A one-second controller grants
Lasgand damage immunity while an enrolled, online challenger is dead, and
removes it after revival. Offline characters are not queried.

Only same-party, story-complete characters present at seal break and eligible
for the selected mode enter the reward roster. Reconnects retain eligibility;
late arrivals cannot claim the chest. A character can claim each instance only
once, even if the instance crosses the daily reset. Inventory-full retries use
the original cached loot roll. Normal gives item 102567; Hard gives 102568.
Fourteen independent material/equipment rolls use the mode-specific storage
rates. The separate repeat boss card drop is 20/10,000 before server drop-rate
modifiers. NPC appearance 22004 is explicitly registered for the storage.

Deployment must preserve the live `db/import/mob_avail.yml` and add its import
of `db/episode20_mob_avail.yml`; updating the template alone does not update an
existing installation. No new client archive is required for the reused
appearances and maps.

Validation includes isolated startup with the production Alpine binary and a
separate database, plus actual script-VM regressions for dialogue races,
eligibility, reward retries, reset boundaries, timer state, immunity and vision
distance bands. World population, monster effects and inventory delivery are
controlled boundaries in the script-VM tests; rendered gameplay is not covered.
