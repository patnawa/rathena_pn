-- Preserve full Attack2 values emitted by yaml2sql in strict SQL mode.
-- Apply to the configured monster tables if their names differ from defaults.
ALTER TABLE `mob_db` MODIFY `attack2` int(11) unsigned DEFAULT NULL;
ALTER TABLE `mob_db2` MODIFY `attack2` int(11) unsigned DEFAULT NULL;
ALTER TABLE `mob_db_re` MODIFY `attack2` int(11) unsigned DEFAULT NULL;
ALTER TABLE `mob_db2_re` MODIFY `attack2` int(11) unsigned DEFAULT NULL;
