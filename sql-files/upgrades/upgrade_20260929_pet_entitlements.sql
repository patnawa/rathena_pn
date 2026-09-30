-- Apply only after a restore-verified backup and coordinated writer stop.
-- DDL is not transactional. Preserve all existing pet rows and all receipts.
ALTER TABLE `pet` ENGINE=InnoDB;
SET @pn_pet_index_sql = (
  SELECT IF(COUNT(*) = 0,
    'ALTER TABLE `inventory` ADD INDEX `pn_pet_identity_lookup` (`card0`,`card1`,`card2`)',
    'SELECT 1')
  FROM information_schema.STATISTICS
  WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='inventory' AND INDEX_NAME='pn_pet_identity_lookup'
);
PREPARE pn_pet_index_stmt FROM @pn_pet_index_sql;
EXECUTE pn_pet_index_stmt;
DEALLOCATE PREPARE pn_pet_index_stmt;
CREATE TABLE IF NOT EXISTS `pn_pet_entitlements` (
  `id` BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
  `account_id` INT UNSIGNED NOT NULL,
  `char_id` INT UNSIGNED NOT NULL,
  `nonce_hi` BIGINT UNSIGNED NOT NULL,
  `nonce_lo` BIGINT UNSIGNED NOT NULL,
  `sequence` BIGINT UNSIGNED NOT NULL,
  `ordinal` SMALLINT UNSIGNED NOT NULL,
  `pet_id` INT UNSIGNED NOT NULL,
  `payload` BLOB NOT NULL,
  `egg` BLOB NOT NULL,
  `claimed` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `claimed_at` TIMESTAMP NULL DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `request_output` (`account_id`,`nonce_hi`,`nonce_lo`,`sequence`,`ordinal`),
  UNIQUE KEY `pet_identity` (`pet_id`),
  KEY `owner_pending` (`account_id`,`char_id`,`claimed`,`id`)
) ENGINE=InnoDB;
-- A claimed record remains replay protection after the egg is traded, consumed,
-- or the pet is deleted. Do not cascade-delete or prune these receipts.

-- Raw mail egg extraction shares the entitlement transaction.
ALTER TABLE `mail` ENGINE=InnoDB;
ALTER TABLE `mail_attachments` ENGINE=InnoDB;
