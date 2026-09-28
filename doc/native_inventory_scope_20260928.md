# Native inventory and bulk-sale scope, 28 September 2026

The economy companion does not change native inventory capacity, item stack
representation or NPC sell-list construction. These require separate acceptance.

## Observed baseline

The located client is the protected 20260219 Ragexe documented in
[the executable inventory](../client-patch/wide_zeny/baseline_20260928.json).
No running Ragexe process was present during this audit, so no native sale packet
or runtime serializer was captured. A static signature or a server synthetic
packet test does not establish that this executable constructs a correct packet.

Local WARP2026 `Scripts/Patches/CustomInventoryLimit.qjs` changes the string used
to display the maximum inventory count. Its `CustomInventoryExpandingLimit`
changes two expansion-limit comparisons. Neither implementation audits native
item allocations or rewrites sell-list serialization. No NPC sell-190 or Add All
implementation was found in the local patch scripts searched.

## Capacity evidence

The staged server's `src/map/packets.hpp` defines packet 0x00c9 as a 4-byte header
followed by 4-byte rows containing `uint16 index` and `uint16 amount`. A 191-row
sale is 768 bytes; a 400-row sale is 1,604 bytes. The wire length has no boundary
at 190 rows. Server acceptance and duplicate/shape validation are handled by the
gameplay patch; native construction above 190 remains a separate test.

The current `src/common/mmo.hpp` sets inventory base and expansion to 100 each,
and the inventory array uses their sum. Native expansion replies carry the
configured capacity, but raising this server allocation alone does not prove the
client's UI and internal arrays accept 400 slots.

A 900,000 stack cannot fit the current representations: `item.amount` in
`src/common/mmo.hpp` is signed 16-bit, `NORMALITEM_INFO.count` in
`src/map/packets_struct.hpp` is signed 16-bit, and NPC sell quantities are unsigned
16-bit. Changing only MAX_AMOUNT would truncate or overflow these paths. A full
implementation must widen persisted/inter-server item quantities and every
relevant packet or provide a negotiated replacement item protocol with complete
inventory, storage, cart, mail, trade, crafting and shop handling.

## Native acceptance still required

On the pinned executable, populate actual inventories with 190, 191, 200 and then
400 distinct eligible items. Review the native sell list, capture the outgoing
0x00c9 frame, compare each selected slot and quantity, and verify the server
receipt, inventory and Zeny after relog. Include a canceled review, duplicate or
changed slots, restricted/favorite/equipped items and insufficient wallet room.
This distinguishes a serializer defect from a server parser rejection.

An independent Add All control can be implemented through a companion inventory
snapshot and explicit sale confirmation, but must preserve the native NPC shop
identity, eligibility rules and item identities. Silently splitting one native
sale into multiple packets is unsuitable: the server clears `npc_shopid` after
each sale and partial batches would have different transaction behavior.

No native inventory or bulk-sale fix is claimed by the staged economy package.
