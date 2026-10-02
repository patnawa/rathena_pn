-- Run in the character database AND the login database (once if shared).
-- For split databases, run only the ALTERs for the tables present there.
-- Substitute configured registry table names if they differ from defaults.
ALTER TABLE `char_reg_str` ENGINE=InnoDB;
ALTER TABLE `acc_reg_str` ENGINE=InnoDB;
ALTER TABLE `global_acc_reg_str` ENGINE=InnoDB;

CREATE TABLE IF NOT EXISTS `pn_registry_saves` (
  `account_id` int unsigned NOT NULL,
  `char_id` int unsigned NOT NULL,
  `nonce_hi` bigint unsigned NOT NULL,
  `nonce_lo` bigint unsigned NOT NULL,
  `scope` tinyint unsigned NOT NULL,
  `sequence` bigint unsigned NOT NULL,
  `payload` mediumblob NOT NULL,
  `created_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_id`,`char_id`,`nonce_hi`,`nonce_lo`,`scope`,`sequence`)
) ENGINE=InnoDB;
-- Keep receipts while a map process may retry. Do not age-delete them during
-- service: a missing receipt would make an old currency save writable again.
