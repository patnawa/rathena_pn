# Cash shop display repair — 29 September 2026

The installed 2026-02-19 client has WARP PreviewInShop enabled. Its ZC_ACK_SCHEDULER_CASHITEM decoder advances 14 bytes per product: uint32 item ID, uint32 price, uint16 view sprite, uint32 equipment location. The server was built with ENABLE_CASHSHOP_PREVIEW_PATCH disabled, producing 8-byte rows. After the first product in each tab, the client consumed the wrong bytes as item IDs and prices, causing Unknown Item entries and unreliable shop display.

Enabled the matching server option in src/config/core.hpp. No catalogue items or prices changed. The catalogue remains 97 consumable/support products across New, Consumables and Other, with no costume listings.

## Verification

- Captured only executable sections from an isolated diagnostic client, then stopped and removed that copy.
- Located the installed client's actual item/price and preview decoding instructions. Both use a 14-byte row stride.
- Compiled the production clif_cashshop_list function and its production packet structures into an isolated wire fixture. Replayed those emitted packets through the actual client decoding instructions with Unicorn.
- Previous format: 94 of 97 products decoded incorrectly across the three actual tabs. Fixed format: all 97 IDs and prices match; all consumable preview fields decode as zero.
- Full native map-server rebuild and isolated database/startup validation pass. The existing item metadata, icons and illustrations cover all 144 reachable products and box contents.
- Client English banner published and installed as client-20260929-cashshop-english, sequence 2026092904. Four payload hashes verified. The banner callback passes Lua 5.1 validation.

Evidence and runnable reproduction scripts are in Server-Development/cashshop-display-20260929 at the workspace root: native-wire-validation.json, wire_native_remote.py, verify_native_wire.py, build.json, validation.json and deployment.json. The native wire fixture stubs item look/location for these consumables; it executes the real serializer and real client decoder, not the entire game UI. An in-game visual check has not been performed.

Do not disable ENABLE_CASHSHOP_PREVIEW_PATCH without also removing the matching client PreviewInShop patch. This setting changes the network row layout.
