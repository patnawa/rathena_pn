# Database backup and recovery

`tools/admin/database_backup.py --check-restore` runs on the Docker host. It locks all tables while dumping because the live database contains both MyISAM and InnoDB tables. This briefly pauses database writes. A single-transaction dump alone would not protect the MyISAM data.

Backups are compressed and SHA-256 verified before publication in `/app/rathena-database-backups`, with owner-only permissions. Credentials are read inside the existing database container and are never written into the backup tool or its report. The SQL archive itself contains account data and must remain private.

The restore check starts a temporary database from the exact production image, with no network or published ports. It restores the archive, checks every restored table, then compares a deterministic dump of the restored database with the original SQL hash. It removes the test container afterward. The tool has no production restore command. A failed dump is not published; a failed restore check marks its report unsuccessful and exits nonzero.

The supplied systemd service and timer run this check daily around 04:00 server local time and catch up after downtime. Check status with `systemctl status rathena-database-backup.timer` and check failures with `journalctl -u rathena-database-backup.service`. Backups are retained; this tool does not prune them. Review disk usage and copy validated archives to an independently secured backup location for protection against host loss.

For actual recovery, first stop game services and control-panel writes, preserve the current database and configuration, verify the chosen archive and its successful restore report, and restore in an isolated database again. Compare the recovered character/inventory/quest state before arranging a controlled production replacement. Never test restoration against the live database. The daily backup does not provide point-in-time recovery between snapshots.
