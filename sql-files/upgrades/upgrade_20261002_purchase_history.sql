-- Apply before starting the new char/map binaries. No old receipt is replayed.
-- Detailed player history begins with purchases processed by these binaries.
CREATE TABLE IF NOT EXISTS `pn_purchase_history` (
  `account_id` int unsigned NOT NULL,
  `char_id` int unsigned NOT NULL,
  `nonce_hi` bigint unsigned NOT NULL,
  `nonce_lo` bigint unsigned NOT NULL,
  `sequence` bigint unsigned NOT NULL,
  `kind` int unsigned NOT NULL,
  `outcome` int unsigned NOT NULL,
  `details` mediumtext NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_id`,`nonce_hi`,`nonce_lo`,`sequence`),
  KEY `owner_recent` (`account_id`,`char_id`,`created_at`)
) ENGINE=InnoDB;
