# LAN development release, 19 September 2026

The client lives outside the server tree, at roughly 4.99 GiB. Its launcher
uses the private 192.168.10.18:8082 feed and embedded RSA verification key;
`client-patch/launcher` contains the build and recovery logic. Font creation
uses measured Arial sizes: body 12, panel labels 11 and resource values 10.
The reference raster comparisons and the user's visual checks passed. See
`client-patch/native_font` for the compatibility loader and test harness.

Kafra reserve purchases now use a durable receipt and atomic inventory/points
write. Apply `upgrade_20260919_reserve_purchase.sql` before starting the new
map/char pair. The internal commit packet grew from 80 to 112 bytes; these two
binaries must be installed together. External Bank UI protocol remains v2.
An exact retry acknowledges its existing receipt without replacing newer
inventory. Keep the additive ledger and InnoDB conversion on code rollback.

The Progression Guide offers milestones/build priorities and optional weekly
practice. Three objectives award a cosmetic stamp: 50 eligible kills, a completed
private damage-lab measurement and a build-priority review. No item, currency
or experience rewards are added. Reset is Monday 00:00 UTC.

Validation used real production inventory/VM and SQL implementations. Transport
and world boundaries in the VM fixture are explicit doubles. SQL proof includes
49 bank and 31 reserve assertions, actual database kill-9/restart recovery,
ownership checks and retry behavior. The native run included 108 Kafra scenarios,
weekly reset/claim tests and the inherited VM suite (676 cases, 28,906 assertions,
clean allocator teardown). Both production binaries passed isolated startup,
complete schema loading and live startup checks after guarded deployment.
Interactive gameplay acceptance of the new services remains a manual check.

`tools/admin/health_check.py` checks all seven containers and only considers
error logs from the current process lifetime within its lookback. This prevents
a planned stop's disconnect message from failing a healthy replacement process.
`build_retention.py` defaults to a preview and only prunes old compiler/cache
files in inactive build trees; images, volumes, backups, executables and source
are preserved. Docker logs are capped at 20 MB x 5 per service.

Runtime image IDs are pinned in live Compose configuration. A protected off-host
recovery set includes Docker-save images, exact source/config/binaries and a SQL
dump whose isolated restore passed 138 tables. The Windows daily/logon task
copies subsequent restore-verified SQL backups. No external alert delivery or
public-server access is configured.

Deployment scripts, exact hash manifests, rollback receipts and the full runbook
are retained outside the client in the workspace's
`Server-Development/improvements-20260919` directory. The live rollback is
`/app/pn-improvements-20260919/gameplay-before`. Preserve unrelated source work
and player data when using it; an ordinary code rollback must not restore an
old SQL dump over newer progress.
