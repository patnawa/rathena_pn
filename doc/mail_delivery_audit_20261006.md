# RODEX delivery audit — 2026-10-06

Zeny-only attachment claims previously used a legacy destructive request without
the durable inventory receipt used for item claims. An unreserved legacy reply
credited the current character and made pending Zeny negative. Outgoing sends
removed items and fees before persistence; a failed delivery could lose an item
when its old inventory slot had filled before the refund.

All native player sends and attachment claims now use the existing immutable
inventory/wallet transaction and replay receipt. SQL commits mail, attachments,
sender assets and pet ownership together. Missing recipients reject without a
debit, SQL failures retry, and a repeated receipt cannot send or charge twice.
Zeny-only claims preserve item attachments; item-only claims preserve Zeny.
Legacy replies cannot mutate assets belonging to a newer session.

MailSend uses a tagged envelope in the unused stock region of a stock-free
request. The version-3 frame remains 59,958 bytes, preserving existing receipt
payloads. Upgrade map and character services together; no SQL migration is needed.
System/script mail continues using its existing insertion API.

Validation includes native ASan/UBSan mail admission and inventory planning,
wallet limits, overlapping operations, malformed selections, fee accounting,
metadata and full inventory. Persistence submission is an explicit native-test
boundary. Separate disposable MariaDB tests execute production SQL with injected
mail, attachment, inventory, wallet, pet and receipt failures, replay after owner
departure, pet identity transfer and messages without attachments. No production
database is used by these tests. Client rendering remains a manual acceptance step.

The broader validation also detected a missing generated planner route for the
already-enabled Alice equipment exchange. Its generated entry is restored.
