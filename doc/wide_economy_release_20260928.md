# Wide Zeny and MuhRO-inspired gameplay release — 28 September 2026

Deployed to the Docker server at `192.168.10.18`; all seven production services
passed health checks and all four game servers completed startup without errors.
The signed client feed is `client-20260928-wide-zeny`, sequence `2026092802`.
The local PN-Client installation was updated and its 5,639 manifest files verified.

## Player behavior

Character wallets and the account bank each support **9,223,372,036,854,775,807
Zeny**. Open Wallet & Bank with Ctrl+B, Alt+B, the bank button or `@bank`.
Deposits, withdrawals, offline-character collection, bilateral trades, vending,
buying stores and Zeny-only mail use exact integer companion controls. Pending
saves wait for a durable server receipt. Vending proceeds enter the account bank.

The protected Ragexe executable is unchanged. Its native wallet display remains
capped at 2,147,483,647; Wallet & Bank displays the actual balance. Native item
selection and received/item-bearing RODEX mail remain in the game. Unsafe narrow
transaction paths are rejected. This independent PN implementation does not
claim full MuhRO native-client parity.

The existing one diamond (6024) and 127 tickets (12781) were removed atomically
and credited at 499,000,000 and 998,000 each. The account bank increased from
3,573,752,000 to **4,199,498,000**. Combined character wallets stayed at
2,325,994,998. Total value, including the retired items at those redemption rates,
remained **6,525,492,998**. All fourteen checked storage/ownership tables contain
zero retired currency rows. Exchange controls and new token issuance are disabled.

Start through `Launch PN.cmd` and restart the game after updating. It selects
`PNLauncher-20260928.exe`; the original launcher remains available. Large-file
verification is cached only while open handles prevent changes, and Repair
rehashes everything. Rollback refuses to modify its own running executable before
touching files; close the versioned launcher and use the original launcher for
that rollback. PNWallet64.ini is a preserved setting; existing BankUI settings
remain on disk, while other installations can adjust the new panel's defaults.

## Verification and recovery evidence

The immutable compiled candidate is `build-combined3`. Actual isolated server
sessions proved values above 2^53, both trade participants, vending/buying stock,
exact taxes and mail fees, native mail claim, offline wallet collection, duplicate
requests, disconnect recovery and injected SQL failures with no partial transfers.
The paired transaction handler also passed Linux undefined-behavior sanitizer
tests. Windows UI/transport/loader tests and launcher rollback/cache tests passed.

Native gameplay sessions proved six memo destinations and actual portal travel,
missionary rescue progression, reset behavior, rental expiry without trap material
duplication, and +15 certificate refinement with persisted inventory. Full item
and quest metadata loaded in Lua 5.1. The applicable balance and instance changes
are detailed in [the gameplay audit](muhro_last10_gameplay_20260928.md).

Remote evidence and complete original SQL/file backups are under
`/app/pn-muhro-zeny-20260928/release-20260928T060316Z`:

- `backup-proof.json`: the full database was restored into an isolated database;
  its canonical dump matched SHA-256
  `05b44e2184b28d0c0cd50978513187cc770bab2e27c31fcacfd61a621a8e1335`.
- `token-result.json`: committed plan
  `51924a6cc597609a3bb09a5e4809d3f9384a40675485f475c6dd13c2bf35455a`.
- `continued-release.json`, `startup-verification.json`,
  `financial-verification.json`: final installation, health and conservation.

The first cutover attempt found a runner return-value error before stopping any
service. The second stopped and saved all servers cleanly, verified the backup,
and refused conversion because legacy guild storage was MyISAM. No currency had
changed. Guild storage was converted to InnoDB, the exact plan was reviewed and
committed, and installation continued from the same original backup. Both map and
char restart policies were restored to `unless-stopped`. The original failure
receipt is retained; the continuation and final verification receipts supersede
its deployment status. Local fresh schemas and upgrades now include all required
transactional storage tables; these SQL-only followups were independently tested.

Do not restore the narrow old binaries/database after new financial activity.
Retain BIGINT columns and use a compatible forward repair. Original backups are
for coordinated recovery, not automatic rollback of players' later transactions.

## Remaining latest-ten scope

This release is **not complete parity with all ten MuhRO patches**. Remaining
work includes BG/event systems, rankings/betting and their equipment/reward
catalogs; the two-floor market; native 900,000-item stacks and larger inventory
windows; gray-map/rendering features; and verification of the protected client's
reported bulk-sale disconnect. Public notes do not supply every asset or balance
catalog. See the release-by-release audit for delivered, already-existing,
unreproduced and still-unimplemented behavior.
