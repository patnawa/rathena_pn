# Mail lifecycle audit — 2026-10-06

Real SQL failure injection reproduced loss of the original mail and attachments
when return insertion failed. A second probe deleted another character's mail
by ID. Return now locks the owned source header and inserts the returned mail,
copies attachments and deletes the original in one InnoDB transaction.
Notifications follow commit. Replaying the old ID cannot create another return.

Deletion verifies the destination character in SQL. Player deletion additionally
requires zero Zeny and no attachment rows, independently of the cached inbox.
Internal expiry still follows the configured return/delete periods, including
final deletion of unclaimed returned or server mail. Nonpositive expiry periods
do nothing. This change does not extend the configured retention policy.

Return rejects system mail, already returned mail, and missing senders without
destroying assets. Maximum-length titles are bounded to the destination field,
preserving the adjacent message body. Expiry return acknowledgements go to the
original recipient's map connection; new-mail notifications reach the sender.
Pet rows and encoded egg identities are preserved during return.

The disposable SQL suite covers insertion, attachment and deletion failures;
wrong owners; manual asset protection and ACKs; long titles; metadata; pet eggs;
missing senders; system mail; empty-mail policy; disabled and enabled expiry;
repeated and concurrent returns. It executes the linked character-server code,
not a replacement SQL model. The full existing recovery suite additionally
checks mail claims/sends, transaction receipts and restart recovery.

No schema or wire-format migration is required. Rendered client acceptance is
not claimed by these SQL tests.
