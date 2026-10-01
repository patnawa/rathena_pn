-- Run during a coordinated map/character maintenance stop, after backing up.
-- Existing quantities are preserved. DDL itself is not transactional.
ALTER TABLE `market` ENGINE=InnoDB;
ALTER TABLE `barter` ENGINE=InnoDB;
ALTER TABLE `sales` ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS `pn_shop_commits` (
  `account_id` INT UNSIGNED NOT NULL,
  `nonce_hi` BIGINT UNSIGNED NOT NULL,
  `nonce_lo` BIGINT UNSIGNED NOT NULL,
  `sequence` BIGINT UNSIGNED NOT NULL,
  `outcome` TINYINT UNSIGNED NOT NULL,
  `payload` MEDIUMBLOB NOT NULL,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`account_id`,`nonce_hi`,`nonce_lo`,`sequence`)
) ENGINE=InnoDB;
-- char/inventory/acc_reg_num must already use InnoDB; the handler refuses
-- commits if any participating table is nontransactional or absent.
-- Receipts are replay protection: never prune them while old sessions or
-- queued requests could still submit their identifiers.
