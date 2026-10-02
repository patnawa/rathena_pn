-- Apply before the atomic character-status writer is installed.
-- Custom schema_config table names require the same engine conversion.
-- No player values are rewritten by this migration.
ALTER TABLE `char` ENGINE=InnoDB;
ALTER TABLE `memo` ENGINE=InnoDB;
ALTER TABLE `skill` ENGINE=InnoDB;
ALTER TABLE `friends` ENGINE=InnoDB;
ALTER TABLE `hotkey` ENGINE=InnoDB;
ALTER TABLE `mercenary_owner` ENGINE=InnoDB;
