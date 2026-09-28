CREATE TABLE IF NOT EXISTS pn_bank_sweep_commits (
  account_id INT UNSIGNED NOT NULL,
  nonce_hi BIGINT UNSIGNED NOT NULL,
  nonce_lo BIGINT UNSIGNED NOT NULL,
  request_id BIGINT UNSIGNED NOT NULL,
  char_id INT UNSIGNED NOT NULL,
  bank_before BIGINT NOT NULL,
  bank_after BIGINT NOT NULL,
  collected INT UNSIGNED NOT NULL,
  skipped INT UNSIGNED NOT NULL,
  PRIMARY KEY(account_id,nonce_hi,nonce_lo,request_id)
) ENGINE=InnoDB;
