# Shop map acknowledgement recovery fixture — 2026-09-29

`python tools/ci/shop_recovery_inter_test.py` passes under AddressSanitizer and UndefinedBehaviorSanitizer in the isolated Ubuntu validation image. Evidence: [log](evidence/shop_recovery_inter_20260929/pass.log), [input hashes](evidence/shop_recovery_inter_20260929/inputs.json).

The runner compiles every executable line of current `src/custom/shop_inter.inc`, removing only its include directives. The fixture supplies explicit doubles for session lookup, transport, timers, stock refresh, logging, client notifications, and post-commit callbacks. The actual shared protocol and state headers are included.

Covered independently for Market, Barter, and Sale:

- Submit saves before sending and leaves inventory, wallet and client results unchanged until a matching durable acknowledgement.
- Retries preserve identical bytes even if the caller mutates its original request; disconnected retries retain pending state.
- Wrong character, sequence, nonce, and unknown-outcome acknowledgements are ignored.
- Failed authoritative stock refresh leaves the player and global stock lock pending without applying assets or reporting success.
- Committed outcomes install the snapshot once, unlock before callbacks, and report success. Duplicate acknowledgement cannot overwrite subsequently changed state.
- Rejected outcomes leave inventory and currencies unchanged and execute no grant callbacks.
- Invalid request, disconnected transport, failed pre-save, and global pending purchase prevent submission.

This fixture proves the inter-handler control flow, not the production implementations of SQL stock refresh, save transport, callbacks, inventory index rebuilding, or combat/script mutation fences. Real database transaction and crash coverage lives in `shop_recovery_sql_test_20260929.md`. Independent review found an incoming equipment-break mutation could modify inventory while its snapshot was pending; root added a pending guard to `skill_break_equip`, and added equipment/identify guards. The fixture does not claim exhaustive coverage of every direct inventory mutation in the game.
