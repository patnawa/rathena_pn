-- Required before enabling paired trade/vending/buying persistence.
-- Run during the stopped-server migration: all participants must roll back together.
ALTER TABLE `char` ENGINE=InnoDB;
ALTER TABLE `inventory` ENGINE=InnoDB;
ALTER TABLE `acc_reg_num` ENGINE=InnoDB;
ALTER TABLE `cart_inventory` ENGINE=InnoDB;
ALTER TABLE `vendings` ENGINE=InnoDB;
ALTER TABLE `vending_items` ENGINE=InnoDB;
ALTER TABLE `buyingstores` ENGINE=InnoDB;
ALTER TABLE `buyingstore_items` ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS `pn_pair_commits` (
  `account_id` INT UNSIGNED NOT NULL,
  `nonce_hi` BIGINT UNSIGNED NOT NULL,
  `nonce_lo` BIGINT UNSIGNED NOT NULL,
  `sequence` BIGINT UNSIGNED NOT NULL,
  `payload` MEDIUMBLOB NOT NULL,
  PRIMARY KEY (`account_id`,`nonce_hi`,`nonce_lo`,`sequence`)
) ENGINE=InnoDB;
