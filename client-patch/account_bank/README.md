# Account bank v2.5

The bank panel now provides direct Zeny deposits and withdrawals only. Diamond and 1M Zeny Ticket exchange controls are removed. Existing inventories and balances are preserved; the server protocol is unchanged for compatibility with older clients.

The account bank supports **9,223,372,036,854,775,807 Zeny**. The native character wallet supports **2,147,483,647 Zeny**. Both limits were already at their supported maximum on the Docker server when checked on September 28, 2026. No server binary or database migration is required for this panel change.

Open with the bank button, an NPC using `openbank`, `@bank`, Alt+B or Ctrl+B. Presets add 1M, 10M, 100M or 1B; Max fills the available deposit or withdrawal amount. Ctrl+A selects the amount. Comma-grouped whole numbers are accepted; negative values, decimals, malformed grouping and overflow are rejected.

Saved receipts require a matching server confirmation. Refreshes preserve usable controls; a click during a refresh queues one exact amount and revalidates it when balances arrive. Pending saves and session changes block duplicate or stale transfers. Existing transactional persistence and authentication are unchanged.

The compact panel is 414 × 244 pixels including its border at 100%. `UiScale=125` or `UiScale=150` under `[Bank]` in BankUI.ini enlarges it. Both maximum balances are displayed without floating-point conversion.

Build using `build.ps1 -Output ABSOLUTE_DIRECTORY`. Run bank_ui_test at 100, 125 and 150; bank_reentry_test; bank_refresh_test; bank_transport_test; and bank_loader_test with the installed loader/font dependencies. BankPreview renders synthetic states. Manual gameplay acceptance is separate from these automated checks.

Historical exchange behavior and the existing 64-bit server implementation are documented in [the 64-bit bank report](../../doc/account_bank_64bit_20260913.md). Deployment and client verification for this version are recorded in [the Zeny-only report](../../doc/zeny_only_20260928.md).
