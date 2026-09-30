# Point-funded pet carts

Pet carts from both NPC cash-shop entry points use one immutable asset receipt. Character variables, account `#` variables and temporary `@` variables retain their native scopes; `#CASHPOINTS` and `#KAFRAPOINTS` use the canonical cash columns. Temporary points remain session-only and their key is fenced until the receipt resolves. Persistent point balances are compared and debited within the inventory/pet/receipt transaction using signed 64-bit values.

## Global account points (`##`)

This project uses a shared authoritative InnoDB database for login and character services. The adapter proves that authority at runtime by requiring the login SQL handle to see the exact immutable intent created by the character SQL handle. It does not assume that similarly named tables on separate databases are equivalent. A split-database login cannot approve an invisible intent; it requires a different distributed reservation adapter before global point pet shops can operate there.

1. The map fences the currency key and flushes existing registries before sending its asset request.
2. Character records a durable intent with one active intent per account. It sends the complete identity and payload to login on the same FIFO stream as earlier global registry saves.
3. Login processes those saves before the barrier and approves the shared intent using its configured authoritative global registry table. Only approval fences subsequent writes to that specific numeric key. Login authentication and character selection are fenced for every unresolved intent.
4. Character retries compare the actual authoritative balance and commit the global debit, inventory, pet entitlement, terminal intent and receipt together. A lost reply resolves from the same receipt.
5. Abandoned intents expire after 60 seconds, checked every five seconds. Expiry writes a durable rejection and releases admission in one transaction. It cannot refund a committed purchase, and a late barrier cannot revive a rejected intent. Failure to persist the outcome keeps admission fenced.

Deploy login, character and map binaries together. Apply `upgrade_20260929_point_assets.sql` before starting login or character. The adapter requires transactional barrier, global registry and receipt tables. Custom global registry table names must be ASCII identifiers up to 32 characters and use InnoDB. Default-table migration does not silently convert a differently configured table.

## Evidence

`point_shop_native_test.py` exercises real paid callers with all six currency forms (including cash aliases), large balances, refused dispatch, insufficient funds and legacy entry. `point_barrier_wire_test.py` executes the exact login FIFO handler with explicit ownership and SQL doubles; it is not a TCP test. The SQL acceptance runner links real character and login objects into separate probes. It exercises registry writes before and after login approval, actual split-database refusal, rollback at each payment/output/barrier/receipt boundary, stale balances, admission and immutable retry. MariaDB restart tests cover prepared, approved and committed intents. These are disposable-database tests, not production mutation or client-render acceptance.

The SQL evidence fields `point_asset_commit`, `point_global_barrier`, `point_global_restart` and `point_login_adapter` are mandatory for the pets release scope. Login and character binary hashes, linked objects, source and original log hashes are bound to that evidence. Test additions are not proof of passing execution: the candidate must complete the runner before release.

Registry ingress retains ownership protection after a terminal point receipt: persistent character/account point keys are recorded in `pn_point_registry_keys` within the debit transaction; global keys remain recorded in barrier history. A displaced map or character server cannot overwrite those numeric index-zero balances. Unrelated numeric keys, arrays and strings keep the existing offline final-save/reconnect behavior. Current-owner logout flushes are accepted because registry packets precede the final save/offline packet on the ordered FIFO. The SQL probes exercise both protected stale rejection and unrelated offline progression persistence.

Retain protected-key records and global barrier history while accepting legacy registry packets. Removing that history would remove the stale-writer fence; any future compaction must preserve the protected key set.
