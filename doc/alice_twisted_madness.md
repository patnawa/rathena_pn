# Alice Twisted Madness

This implementation adds a level 175+ party instance with Normal and Hard modes. A solo character can create a party for Normal. Alice at `dali,66,98` handles admission; the Warper routes to `dali,66,100`. Both modes share quest 62090 and a daily 04:00 admission reset. Reservations last 30 minutes, with a five-minute empty-instance timeout. Cooldown is consumed only after successful entry. Admitted members can reconnect to saved checkpoints while the same party instance exists.

## Maze and party rules

The first briefing freezes the admitted starting roster. Each character receives a fixed random solution among five doors. Corridors can return a wrong choice to the beginning or lead through a small detour; the correct route reaches the clock room. Each character must activate both clock sides before continuing through the second maze and chess room.

Hard adds a second large maze with one active exit among 14 candidates. More than half of the original starting roster must reach the final staging area: a seven-person party needs four. Dead travelers count; disconnected or departed members do not. Once that threshold is met, the leader can recall the connected starting members who are still inside either instance map. Normal requires connected travelers inside the instance to finish their individual maze first.

The current leader can transfer leadership through Alice at the first-map briefing or at `2@alice_mad,150,302`, before the boss encounter begins. The target must be a connected member of the original roster and still belong to the same party. Leadership and target eligibility are rechecked after the selection dialogue.

Starting the boss freezes a separate encounter eligibility list. Disconnected members and characters outside the instance are excluded and cannot join or claim later. An eligible character who disconnects after the lock can return; in Hard, disconnection shields the boss until that character reconnects alive. The controller checks eligible members every second, so a death or disconnection can take up to one polling interval to activate the shield. Delayed combat hits are checked again against an already active shield at delivery.

## Combat and original balance

Normal has one Nightmare Keyholder fight. Hard has three consecutive phases: melee physical damage, magic damage, then ranged physical damage. Other channels, including miscellaneous damage, are blocked. Jabberwocky is reused for the second and third phases as an explicit original adaptation. Hard Spades block physical and miscellaneous damage; Absolem blocks magic and miscellaneous damage. Cheshire reinforcements recur during Hard fights, with a maximum of 12 alive, and provide neither drops nor EXP.

| Monster | HP | Database ATK | Database MATK |
| --- | ---: | ---: | ---: |
| Nightmare Keyholder |30,000,000 |3,500 |2,200 |
| Duckworth |500,000 |1,500 |1,200 |
| Spades |4,000,000 |3,000 |2,400 |
| Absolem |4,000,000 |3,000 |2,400 |
| Heart |500,000 |1,700 |900 |
| Cheshire |2,000,000 |2,600 |1,800 |
| Jabberwocky |120,000,000 |5,000 |3,500 |

These are original database inputs, not final per-hit damage or a recovered official table. Attributes, defenses, attack elements and skills also affect actual combat. All seven actors have zero EXP; completion materials are the encounter reward.

Both maps enable a 50% **per-category** defensive equipment/card resistance cap. Independent categories continue to multiply: 50% race and 50% size resistance combine to 75% reduction. This is not a 50% limit on total damage reduction. See [combat-rule details](alice_encounter_data.md) for the exact scope and unmodified defenses. The opt-in `strictdamage` flag makes monster ignored damage channels truly zero; maps without these flags retain existing behavior.

## Clear rewards and exchanges

After the final phase, each eligible character can claim once:

| Mode | Guaranteed clear currency |
| --- | --- |
| Normal | 1 Heavy Chain |
| Hard | 1 Heavy Chain and 1 Monster's Stone |
| Hard completed within 20 minutes of reservation | 1 additional Monster's Stone |

The timed bonus is fixed at completion, not claim time. A full inventory leaves the complete reward unclaimed for a later retry. Shared daily reset does not reopen an already claimed instance reward. Boss monster records do not supply chain or stone drops, preventing phase kills from duplicating clear currency; each boss has an originally tuned 1% Sealed Card Album drop chance.

Normal spawns 50 Duckworth and 50 Heart with one sewing kit or maple leaf per base 100% drop. Both maps disable Renewal level-gap drop penalties. These are ordinary shared field drops, not a separate 50-item grant to every party member. Materials left on the ground or lost when the instance ends are not replaced by the clear reward.

Confused Boy at `dali,70,100` offers nine material exchanges. The Mad Bunny recipe is **1 Heavy Chain, 50 Small Sewing Kits, 50 Maple Leaves and 5,000,000 zeny**. Thus a solo full Normal clear supplies its materials if all field drops are collected. The other exchanges offer Gambler's Seal, four card-in-mouth variants and three costumes. Exchanges recheck costs after confirmation and check capacity before consuming anything. Further equipment reform/enchantment systems are outside this instance's exchange script.

## Assets and validation limits

The maps are locally generated original terrain, with deterministic collision, rendered walls and matching minimaps. They do not reproduce unavailable custom source geometry or artwork. The two map assets must be present in the client's repair overlay, and the server cache must come from their matching GAT files. Native monster sprite aliases are deliberate appearance substitutions. Numerical monster balance, skill timing and undocumented reward amounts are original tuning.

The source release gate runs `alice_geometry_test.py`, `alice_database_test.py` and `instance_combat_rules_test.py`; the full gate additionally runs `alice_test.py` against the native script VM. Geometry checks compare generated resources and collision; database checks cover names, aliases, phase-safe reward sources and imports; combat checks exercise extracted rules and delivery guards. NPC checks execute actual script bodies, including detached event callbacks, with explicit world movement/spawn, inventory, wallet and party-backend doubles.

These tests and isolated startup do not establish a complete human in-game playthrough or final encounter balance. Use a coordinated party containing all three damage types to assess Hard-mode difficulty. The original map builder and asset contract are documented under [client-patch/alice_maze](../client-patch/alice_maze/README.md).
