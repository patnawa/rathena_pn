# Personal storage and bulk deposit audit — 2026-10-06

The native deposit path previously invoked quest scripts after removing each
inventory stack, before the caller finished its batch. A condition could inspect
or alter a later item after the preparation tool's exact-item preflight. The
native withdrawal path invoked item-grant callbacks before removing the stored
source, so a callback could observe the item in both locations.

The minimal two-stack deposit reproduction uses the actual preparation builtin,
native inventory/storage operations and a real quest-condition script. Its first
callback previously saw the second stack still in inventory. The withdrawal
reproduction observes the native quest-condition boundary and forwards to the
actual VM: the stored source was still present and the commit fence was absent.
These are callback-ordering defects; these fixtures do not establish a SQL crash
loss or receipt-replay defect.

Native personal-storage add/get now hold `PcItemDeliveryScope` until source
removal and commit submission. Deletion explicitly postpones quest refresh.
The preparation tool and `@storeall` hold an outer scope across their whole
batch, so inner transfers cannot run callbacks between preflighted items. The
existing pending-transaction fence and durable storage protocol remain in use.
Player-facing protection rules, item costs and storage capacities are unchanged.

`tools/ci/preparation_storage_native_test.py` compiles real `pc.cpp`, `script.cpp`,
`itemdb.cpp`, `clif.cpp`, `storage.cpp`, `quest.cpp` and allocator code with
ASan/UBSan, plus the exact extracted `@storeall` command body. Its 91 cases cover:

- Deposit and withdrawal callback ordering, including before acknowledgment.
- All 29 protected metadata variants and stale confirmations/context changes.
- Whole-batch capacity refusal, native stack limits, occupied-count consistency
  and merging multiple inventory stacks into one storage slot.
- Invalid indices/amounts, access, ownership, connection, missing metadata,
  withdrawal weight/capacity and pending-save replay fences.
- Exact completed page/inventory submission and bound equipment roundtrips with
  GUID, cards, options, refine, grade and favorite metadata.
- `@storeall` callback ordering and one commit for its complete batch.

World availability, actor/NPC lookup, packets, localized messages, item logging,
achievement-objective notifications and interserver submission/ACK are explicit
test boundaries. Quest conditions and inventory/storage mutations execute native
code. Kernel network denial prevents contacting a game or database server.
The suite is registered in the full release checks. Existing preparation tests
cover presets/restock/junk separately; source checks include storage SQL-failure
and cleanup regressions. Private whole-NPC startup checks the rebuilt map ELF.

The scope is personal storage and the two deposit callers. Guild storage, real
SQL crash/reconnect recovery and rendered client acceptance require their own
checks and are not claimed by this native fixture.
