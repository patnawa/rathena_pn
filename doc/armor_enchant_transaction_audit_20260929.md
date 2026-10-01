# Apprentice Craftsman transaction audit — 29 September 2026

Fixed the active legacy armor enchant service in `npc/merchants/enchan_arm.txt`.
It previously deleted the input and recreated it with `getitem2`, so a successful
attempt removed binding, rental expiry, random options, grade and other metadata.
The pre-fix actual-NPC VM test reproduces the binding loss. Refine and card reset
are intentional and explicitly advertised by this NPC; they remain unchanged.

The service now snapshots the sole selected armor's inventory index and unique
ID, then revalidates identity, count, money and supported metadata after the
seven-second progress bar. Equipped, bound, rental, favorite, unidentified,
damaged, graded and random-option armor are refused without charging or removing
anything. Ordinary unequipped armor retains the old success/failure probabilities,
400,000 Zeny fee and advertised refine/card reset. This is safe refusal of
unsupported records, not added support for retaining their metadata.

Verification: `tools/ci/armor_enchant_transaction_test.py` parses the actual NPC
body and executes it in the existing native script VM with actual item DB
definitions and native inventory add/delete operations. Fifteen cases / 840
assertions pass with clean allocator shutdown: eight metadata/equipped refusal
cases, four changes at the real progressbar suspension (money removed, armor
removed, same-item UID replacement, binding changed), ordinary success and
failure, and all 200 inventory slots full with the selected armor in the last
slot. The unmodified source fails the first inventory-preservation assertion.
Normal successful reconstruction creates a new UID, as the original crafting
operation did. Equipment-switch registration is not exposed by `getinventorylist`
and was not independently asserted by this fixture.

The fixture wraps UI, player lookup, registry persistence, equip callbacks and
progressbar delivery; it does not launch a world or connect to an account DB.
Docker networking and native sockets are disabled. Existing compiled core objects
are used; this run is not a sanitizer certification of the entire core. Run from
a built Linux tree with `python3 tools/ci/armor_enchant_transaction_test.py`;
`--source /path/to/before.txt` reproduces the pre-fix failure. Workstation remote
wrapper and logs are in `Server-Development/client-consistency-20260929/`:
`run_armor.py`, `armor-before.log`, `armor-after.log`, `armor-portable.log`,
and final `armor-full-inventory.log`.

## Additional review boundaries

- Both custom Star-of-Spell routes already validate/mutate before their uninterrupted
  deductions. The broad debit/suspension text scan falsely crossed their `close`
  terminators; no fix was needed there.
- Legacy socket enchanters also reconstruct items. Their binding/metadata semantics
  remain a follow-up; this fix applies only to Apprentice Craftsman.
- Mayomayo's failure compensation in `npc/re/merchants/enchan_mal.txt` has a
  native repro: inject unequip refusal through the shared fixture boundary,
  execute the unchanged actual NPC, and the reset refunds one bound Silvervine
  as unbound; enchanting refunds 15 bound E-grade coins as unbound. The weapon
  stays unchanged. Two cases / 381 assertions pass with clean allocator shutdown,
  positively demonstrating the defect. `mayomayo_driver.cpp`, `run_mayomayo.py`
  and `mayomayo-repro.log` in the same work directory reproduce it. Bound-material
  acquisition and a player-triggerable ordinary unequip rejection were not
  established, so this is not a demonstrated player exploit. The subsequent
  [Mayomayo payment fix](mayomayo_payment_audit_20260929.md) now refuses unsupported
  currency before any debit. Moving mutation before payment alone would be
  unsafe because callbacks can alter the payment state.
- `VIP_Third_Class` has a debit-before-dialogue pattern but its callers are gated
  by `VIP_SCRIPT`, currently zero in `src/config/core.hpp`; it was excluded from
  active findings.

No production service or installed client was changed by this audit.
