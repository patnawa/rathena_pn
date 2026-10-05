CREATE TABLE IF NOT EXISTS pn_mail_commits (
  account_id INT UNSIGNED NOT NULL,
  nonce_hi BIGINT UNSIGNED NOT NULL,
  nonce_lo BIGINT UNSIGNED NOT NULL,
  request_id BIGINT UNSIGNED NOT NULL,
  char_id INT UNSIGNED NOT NULL,
  wallet_before BIGINT NOT NULL,
  payload VARBINARY(580) NOT NULL,
  result INT UNSIGNED NOT NULL,
  PRIMARY KEY(account_id,nonce_hi,nonce_lo,request_id)
) ENGINE=InnoDB;
