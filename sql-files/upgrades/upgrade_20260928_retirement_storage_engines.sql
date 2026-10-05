-- Required before atomic retirement; all application writers must be stopped.
-- Preserve columns and data while enabling rollback across every owned store.
ALTER TABLE `inventory` ENGINE=InnoDB;
ALTER TABLE `cart_inventory` ENGINE=InnoDB;
ALTER TABLE `storage` ENGINE=InnoDB;
ALTER TABLE `pn_storage_02` ENGINE=InnoDB;
ALTER TABLE `pn_storage_03` ENGINE=InnoDB;
ALTER TABLE `pn_storage_04` ENGINE=InnoDB;
ALTER TABLE `pn_storage_05` ENGINE=InnoDB;
ALTER TABLE `pn_storage_06` ENGINE=InnoDB;
ALTER TABLE `pn_storage_07` ENGINE=InnoDB;
ALTER TABLE `guild_storage` ENGINE=InnoDB;
ALTER TABLE `mail_attachments` ENGINE=InnoDB;
ALTER TABLE `acc_reg_num` ENGINE=InnoDB;
