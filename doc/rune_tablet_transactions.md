# Rune Tablet workshop: transactions and verification

The PN Rune Stone exposes the Rune Tablet collection through ordinary server NPC dialogs. Talk to the Rune Stone at `grademk,46,178` or its Main Office desk. The service supports catalog browsing and name search, rune registration, tablet activation and switching, enhancement, milestone rewards, equipment imprinting, Rune Shop exchanges, supported card/rune-stone decomposition, and sealed MVP card conversion.

The catalog follows the published [Rune Tablet System](https://wiki.muhro.eu/Rune_Tablet_System) and the matching client data. The native rune and tablet IDs identify persistent collection entries; they are not fabricated inventory items. This implementation does not claim to provide the client's native Rune Tablet window.

## Player progression

Registering a rune consumes its listed materials once and unlocks it for characters on the same RO login account. A character with at least one registered member can pay a tablet's activation cost. That payment is permanent for that character. Switching among paid tablets is free, and only one tablet is active at a time. Bonuses require the published piece-count thresholds, starting at two pieces.

Enhancement belongs to the character and tablet. The confirmation dialog shows the next level, complete material cost, current success probability, and failure increment. Each attempt consumes its materials. A failure retains the level and increases the next probability, capped at 100%; success increases the level and resets the accumulated bonus probability. Chaos follows its catalog maximum of zero rather than accepting upgrades.

Milestone rewards are claimed separately, once per RO login account and tablet milestone. Slot 1 represents activation; slots 2–6 represent the corresponding registered-piece counts; slot 7 represents the complete collection. Empty catalog reward slots do not create a claim option. Registration, activation, enhancement and claims use normal rAthena character/account registry persistence.

| Variable | Scope | Meaning |
| --- | --- | --- |
| `#PNRTPiece[piece_id - 1263000]` | RO login account | Permanent registered rune |
| `PNRTPaid[set_id - 1260000]` | Character | Activation paid |
| `PNRTLevel[set_id - 1260000]` | Character | Enhancement level |
| `PNRTPity[set_id - 1260000]` | Character | Additional probability, denominator 100000 |
| `#PNRTClaims[set_id - 1260000]` | RO login account | Seven milestone claim bits |
| `PNRTActive` | Character | Full active tablet ID, or zero |

No website master-account linking is inferred. Sharing means the same RO login, using the server's normal account variables.

## Exchange behavior

Equipment imprinting permanently consumes the selected equipment instance and 10 Imperfect Runes. Only identified, unequipped, unrefined, ungraded, uncarded equipment without random options, damage, binding or expiry is accepted. Selection captures the inventory index and unsigned unique-ID string. After confirmation, the service refreshes inventory and checks both identity and every eligibility field again. Native `delitemidx` must report success before the rune material is charged.

Card decomposition uses the explicit native decomposition categories and their one-item or 30-item recipes, including their actual probability and quantity tables. It reserves capacity for the complete maximum output before consuming input or rolling rewards. The sealed-card service uses an explicit effective-database card whitelist; it never infers eligible cards from a numeric ID range.

## Transaction guarantees and boundaries

- Costs and progression conditions are checked again after the final confirmation dialog. No dialog, timer or sleep is inserted between final validation and the transaction's debit/grant sequence.
- Repeat registration and reward claims are refused before spending or granting. A reward claim is marked before item delivery without yielding between those operations.
- Capacity is checked for the complete reward batch before spending. A second guard supplements native `checkweight2` with the exact plain-stack metadata, inventory-slot boundary and first-compatible-stack amount limit used by item delivery.
- Capacity checks deliberately do not credit slots or weight that the input debit might free. Players may need to make space before an exchange even when the eventual result would fit after consumption.
- Current catalog outputs are stackable, have no GUID requirement and have no item-specific inventory stack-limit override. Costs other than imprint inputs are non-equipment materials. Catalog changes must preserve these assumptions or extend the transaction guard and tests.
- Normal registry and inventory saving applies. This does not introduce a separate SQL transaction layer or claim crash-atomic persistence across the server's existing save boundaries.

## Automated native verification

On Linux, after building the local map server and installing the repository's Python test dependencies:

```sh
python3 tools/ci/rune_tablet_transaction_test.py \
  --build-dir /tmp/pn-rune-transaction-test
```

The harness parses the production catalog and service functions unchanged, executes real dialog continuations, uses loaded native numeric/string/account registry arrays, and performs real inventory operations. It compiles current `pc.cpp`, `script.cpp`, `itemdb.cpp` and `clif.cpp` with AddressSanitizer and UndefinedBehaviorSanitizer. Kernel filters deny networking. Player/world lookup, outbound transport, unrelated quest/achievement callbacks and bonus-stat refresh are explicit isolation boundaries.

Cases cover registration replay and cancellation, stale inventory, paid activation and free switching, enhancement success/failure/pity/max level, capacity-safe one-time claims, equipment with a maximum unsigned 64-bit unique ID, changed equipment metadata at confirmation, shop exchanges, sealed-card eligibility and deterministic decomposition. Differential capacity cases compare every approved capacity prediction with actual native `pc_additem` behavior for ordinary, bound, expiring, card-bearing and unique-ID-bearing stacks.

The harness does not replace live player acceptance testing of client dialogs, character switching, relog persistence, combat bonuses, reward-container contents or the deployed map placements. Those checks should accompany deployment.
