-- Character point balances join asset commits. Account balances already use InnoDB.
ALTER TABLE `char_reg_num` ENGINE=InnoDB;
-- The global-point adapter requires login and character services to share this
-- authoritative database. Approval is an ordered login barrier, never a payment.
ALTER TABLE `global_acc_reg_num` ENGINE=InnoDB;
CREATE TABLE IF NOT EXISTS `pn_global_point_barriers` (
 `account_id` INT UNSIGNED NOT NULL,
 `nonce_hi` BIGINT UNSIGNED NOT NULL,
 `nonce_lo` BIGINT UNSIGNED NOT NULL,
 `sequence` BIGINT UNSIGNED NOT NULL,
 `char_id` INT UNSIGNED NOT NULL,
 `point_key` VARCHAR(32) BINARY NOT NULL,
 `state` TINYINT UNSIGNED NOT NULL DEFAULT 0,
 `registry_table` VARCHAR(32) BINARY NOT NULL DEFAULT '',
 `payload` MEDIUMBLOB NOT NULL,
 `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
 `active_account` INT UNSIGNED GENERATED ALWAYS AS (CASE WHEN `state`<2 THEN `account_id` ELSE NULL END) STORED,
 PRIMARY KEY (`account_id`,`nonce_hi`,`nonce_lo`,`sequence`),
 UNIQUE KEY `one_pending_account` (`active_account`),
 KEY `pending_age` (`state`,`created_at`),
 KEY `protected_key` (`account_id`,`point_key`)
) ENGINE=InnoDB;

-- Persistent keys that have participated in atomic point payment retain an
-- ownership fence; unrelated offline reconnect registries keep legacy behavior.
CREATE TABLE IF NOT EXISTS `pn_point_registry_keys` (
 `account_id` INT UNSIGNED NOT NULL,
 `char_id` INT UNSIGNED NOT NULL,
 `point_key` VARCHAR(32) BINARY NOT NULL,
 PRIMARY KEY (`account_id`,`char_id`,`point_key`)
) ENGINE=InnoDB;
