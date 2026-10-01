-- Required before installing the atomic achievement snapshot writer.
-- If achievement_table is customized in inter-server configuration, migrate
-- that configured table too. The writer refuses non-InnoDB tables.
ALTER TABLE `achievement` ENGINE=InnoDB;

-- Legacy reward writes marked `achievement.rewarded` before inserting mail, so
-- a crash could leave a claimed row with no reward mail. There is no durable
-- achievement-to-mail key in the old schema, making automatic resend unsafe:
-- it would duplicate valid rewards. Preserve every pre-upgrade claimed row as
-- an explicit operator audit set before enabling the atomic writer.
CREATE TABLE IF NOT EXISTS `pn_achievement_legacy_reward_audit` (
  `char_id` INT UNSIGNED NOT NULL,
  `achievement_id` BIGINT UNSIGNED NOT NULL,
  `completed` DATETIME NULL,
  `rewarded` DATETIME NOT NULL,
  `captured_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
  PRIMARY KEY (`char_id`,`achievement_id`)
) ENGINE=InnoDB;
INSERT IGNORE INTO `pn_achievement_legacy_reward_audit`
  (`char_id`,`achievement_id`,`completed`,`rewarded`)
SELECT `char_id`,`id`,`completed`,`rewarded`
FROM `achievement`
WHERE `rewarded` IS NOT NULL;

-- This release changes the packed durable shop Commit from v2 (59954 bytes)
-- to v3 (59958 bytes). Services are upgraded together, so an unfinished v2
-- global-point admission cannot have a valid retry after restart. Release its
-- login fence instead of leaving the account permanently pending.
UPDATE `pn_global_point_barriers`
SET `state` = 2
WHERE `state` < 2 AND OCTET_LENGTH(`payload`) <> 59958;
