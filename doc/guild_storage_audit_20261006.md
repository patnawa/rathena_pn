# Guild storage audit — 2026-10-06

Six targeted native reproductions identified these defects:

- A member whose storage permission was revoked could still withdraw through
  the actual client packet handler while their earlier window remained open.
- Item-grant quest callbacks ran before withdrawal removed the guild source,
  exposing the item in both inventory and guild storage during the callback.
- Cart transfers ignored the guild-storage lock checked by inventory transfers.
- Cart stacking compared fewer identity fields than storage. A guild withdrawal
  merged an item with random options into a plain cart stack, losing its options.
- The log reader bound a four-byte SQL value into the packed one-byte bound
  field, overlapping the following unique ID bytes.
- The log writer embedded character names in SQL text instead of binding them,
  so names containing apostrophes could break the query.

Guild opening/transfers now validate current membership and role bounds. Every
inventory/cart transfer checks the actor's open guild-storage state, guild/page
identity, lock, capacity bounds, connection and pending transaction fences.
Deposits reject absent or noncanonical item metadata; withdrawals reject unknown
IDs. The client dispatcher already fences pending transactions; the native
guard also protects internal callers before destination mutation. This is not a
claim that ordinary client packets previously bypassed the dispatcher fence.

Native delivery scopes defer quest/grant callbacks until the guild source has
been removed. Cart stacking uses the existing complete `compare_item` identity,
preserving identification, refine, attribute, expiry, bound status, GUID, cards,
grade and options. Equal identities still stack normally. Cart's existing
favorite/equipment-switch handling and guild-bound trade restrictions remain.

Guild logging binds the character name as a string parameter and reads the
bound field with its actual byte width. Log entries are initialized before
fetching. No SQL migration or guild capacity/economy change is required.

`python3 tools/ci/guild_storage_native_test.py` runs 173 native cases with
ASan/UBSan. It compiles actual inventory, storage, guild membership, packet
handler, quest/script VM and allocator sources. Coverage includes the six
reproductions, a permitted packet control, all four transfer paths across 24
access/context refusals, invalid indices/amounts/metadata, stack/weight/slot
limits, 26 cart identity variants, normal stacking and guild-bound equipment
metadata roundtrips. Quest conditions run in the real VM. World lookup,
availability, client packets, logging/objective notifications and SQL statement
preparation/execution/fetch are explicit boundaries. The ordinary suite denies
network access at the kernel.

An optional `--case sql_logs` omits the SQL statement doubles and connects only
to the fixed `release-db` fixture alias with fixture credentials. It requires
`PN_GUILD_SQL_FIXTURE=owned-disposable-only`; run it only on an owned internal
Docker network. The disposable schema probe performs a native transfer and
actual prepared SQL write/read, verifying `O'Brien`, `UINT64_MAX`, bound status,
cards, options and grade. It does not access the production database. Existing
personal-storage/preparation regressions, source release checks and private
whole-NPC startup cover the release separately.

These fixes address runtime access, transfer ordering, metadata and logging.
Guild storage retains its existing character/guild save protocol. Atomic crash
recovery across inventory and shared guild storage, multi-map concurrency and
rendered client acceptance are not proven by these tests.
