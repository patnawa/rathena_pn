# Chapter 1 reward claim audit — 2026-10-05

Chapter 1 could silently lose an earned Root Gold Coin, Purification Amulet or
Collected Sample. `checkweight` finds an existing stack by item ID; `pc_additem`
also requires matching binding, rental expiry, unique ID and cards. With a full
bag and an incompatible stack, preflight passed but `getitem` failed. The VM
continued, allowing the grant helper to report success without queuing the
reward, or the claim helper to clear an existing pending balance.

The minimized native reproduction returned success with one bound coin still
in inventory, no pending balance, and a failed native grant. The regression was
run against the original helpers before the fix and failed on the same loss.

## Changes

- Check actual plain-stack metadata, current inventory quota, weight and the
  terminal first-compatible-stack limit for the three supported materials.
- Record newly earned rewards in permanent character variables before delivery.
- Reduce pending balances only by the observed inventory count increase.
- Bound each delivery to the native 30,000-unit maximum. Larger historical
  balances retain a claimable remainder.
- Reject unsupported item IDs and invalid amounts before any mutation, including
  when the inventory has free space.
- Keep login recovery and `@ch1rewards` using the same delivery path.
- Inspect inventory once for a multi-material claim. A regression on three
  separate full-bag scans hit the normal VM loop limit during development;
  the shared snapshot passes both ordinary and rental-filled 200-slot bags.

The test pins the effective definitions of all three materials and all 40 quest
grant call sites. This capacity helper is deliberately scoped to these plain
stackable items; extending its outputs requires auditing their metadata.

## Validation

`tools/ci/chapter1_reward_claim_test.py` freshly compiles the actual script VM,
inventory implementation and allocator with AddressSanitizer/UndefinedBehaviorSanitizer.
It runs the actual helper bodies and native item grants with network creation
denied. Transport, registry storage and world callbacks are explicit doubles.

The native suite passed 102 cases and 9,182 assertions: 1/100/200-slot bags,
binding/rental/UID/all four card fields, compatible full-bag grants, capped
stacks, out-of-quota stacks, exact weight thresholds, oversized pending balances,
partial multi-material claims, and duplicate retries. Three deliberate native
refusals prove that earned balances survive a failure after successful preflight;
all ordinary capacity denials complete without native errors. No sanitizer or
memory leak diagnostics occurred.

The story's sample handover intentionally clears `CH1_Pending_Sample` along with
the physical evidence while advancing quest 17916 to 17917. That consumption is
separate from reward delivery and remains unchanged.

This validates synchronous delivery and retry behavior. It does not establish an
atomic SQL transaction between inventory saves and character variable saves, or
exactly-once recovery from an abrupt process crash. Existing engine persistence
coverage remains separate. Visual gameplay acceptance is also separate.

## Further audit candidates

Alice and Bioresearch instance rewards also use ID-only `checkweight2` before
marking a clear as claimed. Their batch delivery, randomized weapon selection,
instance lifetime and real synchronous callback behavior need their own native
reproduction and fix; this Chapter 1 helper is not a general batch transaction.
