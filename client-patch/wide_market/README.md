# Exact Zeny market companion

Built into PNWallet64.dll and opened by its Market button. Native vending and
buying-store setup selects inventory/cart items; the matching server creates an
unpublished draft. Inspect own shop, set exact prices and buying budget, review
all rows, then explicitly publish. Published prices cannot be edited in place.
The native shop selection supplies the current target for Open selected shop.

Search accepts exact minimum/maximum prices and an optional item ID. Next shop
uses the server cursor. Each quote includes owner, shop identity and revision.
Purchases and plain-item sales show quantity, unit price and checked total in a
default-No confirmation. Any changed quote during confirmation cancels submission.
Plain-item sales deliberately skip refined, graded, carded, random-option, bound,
expired, equipped or favorite inventory variants; the server remains authoritative.

PMK1 v1 is Request128/Reply2760 with up to 20 Entry128 rows. The protocol header
is an exact copy of the staged server contract. Transport reuses one persistent
socket per game session, shares PNWallet64's existing authenticated-session
observer, verifies nonces and request identity, and closes on malformed replies or transport failure.
It neither installs duplicate socket hooks nor alters native game packets.
Saving responses disable further financial actions while read-only status queries
wait for the server's durable paired receipt. Purchases and sales are never
automatically resubmitted after a delay or connection failure.

Production Win32-control tests cover INT64_MAX rendering, exact filters/prices,
explicit confirmation, changed quotes, draft publishing, duplicate suppression,
and read-only polling of a pending durable transaction.
Loopback tests exercise fragmented replies and authenticated session binding.
The portable number tests cover parsing and quantity-times-price overflow.
These tests pass independently of the real server gameplay fixtures.
