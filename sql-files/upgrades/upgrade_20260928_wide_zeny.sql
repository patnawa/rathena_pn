-- Staged only: stop all writers; back up first; deploy every rebuilt server
-- together because mmo_charstatus/mail inter-server ABIs also change.
-- Requires PNWallet64 protocol v3 client for truthful full-wallet display.
ALTER TABLE `char` MODIFY `zeny` BIGINT NOT NULL DEFAULT 0;
ALTER TABLE `mail` MODIFY `zeny` BIGINT NOT NULL DEFAULT 0;
-- Do not roll back these columns to INT while any value exceeds 2147483647.

-- Market64 signed unit prices and buying budget.
ALTER TABLE `vending_items` MODIFY `price` BIGINT NOT NULL DEFAULT 0;
ALTER TABLE `buyingstore_items` MODIFY `price` BIGINT NOT NULL DEFAULT 0;
ALTER TABLE `buyingstores` MODIFY `limit` BIGINT NOT NULL DEFAULT 0;

-- Required for atomic token retirement and market transactions.
ALTER TABLE `vending_items` ENGINE=InnoDB;
ALTER TABLE `vendings` ENGINE=InnoDB;
ALTER TABLE `buyingstore_items` ENGINE=InnoDB;
ALTER TABLE `buyingstores` ENGINE=InnoDB;

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
