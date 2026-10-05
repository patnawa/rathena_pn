# Adventure preparation and inventory tools

The office exposes `PN_Preparation` and `PN_InventoryTools` as NPC functions.
Both work in unrestricted towns while alive, connected and outside another
inventory, wallet, mail, storage, vending or shop transaction.

Each character owns three named presets: Farming, Boss and Support. Saving
records currently equipped exact item fingerprints and every native autoloot
rate, type filter and item filter. Applying resolves the fingerprints against
the current inventory, rejects missing, altered or ambiguous equipment, checks
normal job and slot restrictions, switches with native equipment APIs, and
restores the previous equipment if a native equip call fails. Weapons precede
ammunition. Rental items and equipment-switch entries cannot enter presets.
Equipment is never reconstructed or granted. Changing stats or skills remains
the responsibility of existing reset services.

Each preset also has eight independently editable supply target rows. Red,
orange, yellow, white, blue and green potions and fly/butterfly wings are sold
at their current item database retail prices. The preview shows missing amounts
and the exact total. Confirmation recomputes inventory holdings and prices;
changes require a fresh preview. The existing immutable inventory and wallet
transaction owns payment and delivery. Supply quantity is bounded to 1,000 per
row. Duplicate IDs and arbitrary non-retail item IDs are refused.

The deposit tool previews ordinary non-equipment inventory items and deposits
them into main account storage. Favorites, unidentified items, equipped and
equipment-switch items, refined, carded, graded, random-option, bound, rental,
broken and uniquely identified items are protected. Pet eggs and all weapon,
armor, shadow and ammunition categories are excluded. Before the first transfer,
the entire batch must pass exact inventory snapshots, native storage trade
restrictions, canonical item metadata, storage ownership and all capacity/stack
checks. It uses normal native storage transfers under one storage batch commit.
No script deletes an item and creates a substitute.

Junk selling requires the player's explicit character allowlist (20 rows);
miscellaneous items are never junk by default. The same protections apply.
Preview prices honor the normal native selling price calculation. Confirmation
revalidates every exact item, the allowlist and the price. One existing durable
`ItemUse` inventory/wallet transaction removes the reviewed items and credits
the reviewed Zeny; no inventory or wallet mutation precedes its acknowledgment.

Storage search accepts a name, Aegis name or exact ID and reads every accessible
personal storage page through the established owned-holdings planner. Loaded
live pages replace SQL snapshots, so unsaved changes are not double counted.
Queries are bounded to 80 item IDs per result page and never accept account IDs,
SQL fragments or table names from the player. Existing storage services own
withdrawals and individual storage page access.

Run `python3 tools/ci/preparation_native_test.py --build-dir /tmp/pn-preparation-native-test`
on a Linux checkout with built map/common/library objects. This executes the
production NPC parser, script VM, persistent and session registries, equipment
eligibility and preparation implementation with ASan/UBSan. World flags, network,
equipment packet delivery, storage transfers and durable submission are explicit
test boundaries. It checks protected items, changed previews, missing funds,
disconnects, ambiguous identities, failed equipment restoration, paid restock,
explicit junk sales and whole-batch capacity refusal. Existing shop and storage
SQL/recovery suites exercise durable acknowledgments; this suite does not claim
a SQL roundtrip or a rendered gameplay receipt.
