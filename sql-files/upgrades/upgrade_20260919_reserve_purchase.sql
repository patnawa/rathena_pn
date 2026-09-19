-- Stop map/char writers, back up SQL, and upgrade both server binaries together.
ALTER TABLE `char_reg_num` ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS `pn_reserve_commits` (
  `account_id` int unsigned NOT NULL,
  `nonce_hi` bigint unsigned NOT NULL,
  `nonce_lo` bigint unsigned NOT NULL,
  `request_id` bigint unsigned NOT NULL,
  `char_id` int unsigned NOT NULL,
  `cost` bigint NOT NULL,
  `points_before` bigint NOT NULL,
  `points_after` bigint NOT NULL,
  `item0` int unsigned NOT NULL,
  `quantity0` int unsigned NOT NULL,
  `item1` int unsigned NOT NULL DEFAULT 0,
  `quantity1` int unsigned NOT NULL DEFAULT 0,
  `committed_utc` timestamp NOT NULL DEFAULT current_timestamp,
  PRIMARY KEY (`account_id`,`nonce_hi`,`nonce_lo`,`request_id`),
  KEY `character_history` (`char_id`,`committed_utc`)
) ENGINE=InnoDB;
-- Keep the ledger and InnoDB conversion when rolling application code back.
