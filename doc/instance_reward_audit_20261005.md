# Alice and Bioresearch reward claim audit — 2026-10-05

Both clear-reward NPCs could consume a one-time claim without delivering its
items. Their `checkweight2` preflight matches existing stacks by ID, whereas
native `pc_additem` also checks binding, rental expiry, unique ID and cards.
After a false-positive preflight, `getitem` fails but its script continues.
Both NPCs set the claimed flag before delivery, then could award certainty
credit and, in Bioresearch, advance quests despite a missing reward.

The original Alice NPC reproduced this with a full bag containing a bound
Heavy Chain: inventory stayed at one chain, the claim flag became one, and
the credit callback ran. The original Sierra Story reward independently failed
the same regression with an incompatible Somatology Experimental Fragment.
These are actual NPC bodies executing in the native VM and inventory engine.

## Fix

- A shared, bounded capacity helper checks the complete remaining batch, using
  native stack metadata, first-compatible-stack ordering, quota and weight.
  It reserves separate rows for non-stackable Bioresearch weapons.
- Instance variables record delivered quantities per character and output.
  A native refusal retains the undelivered remainder. Retrying skips outputs
  already delivered, including items the player moved or used afterward.
- The claimed flag, certainty credit and Bioresearch quest completion follow
  successful delivery of the entire batch.
- An in-progress guard prevents a synchronous callback from opening another
  claim dialogue or duplicating the current delivery.
- Bioresearch keeps its original cached weapon roll across retries.

The helper supports only the five actual stackable reward outputs and the 39
weapons in `BIO_W_BOX` subgroup 6. Effective metadata and every weapon output
are exercised by the focused native test. Extending the catalog requires
auditing GUID, autoequip and custom stack-limit assumptions.

The helper inspects inventory in one ascending pass, including full 200-slot
bags. Native `copyarray` treats equal variable names/indices as self-copies
across scopes, so its local arrays have distinct names. Native `min` accepts
array references as ranges; delivery calculations explicitly convert each
remaining amount to a scalar.

## Validation and scope

`tools/ci/instance_reward_claim_test.py` freshly compiles script execution,
inventory mutation, quest mutation and allocator code under ASan/UBSan. It
loads effective item definitions and the actual weapon group, executes both
NPC bodies, and runs the real certainty/weekly helpers and instance registers.
Network creation is denied. World map names/time, transport, registry storage
and achievement/quest-info callbacks are declared doubles; quest persistence
is disabled in the private fixture.

The native suite passes 133 cases and 12,119 assertions: both modes, 100/200-slot
bags, seven incompatible stack variants, partial delivery failures at every
output position, consumed outputs before retry, exact weight and stack limits,
eligible-party gates, duplicate/reentrant claims, all 39 weapons and a real
random-group roll that survives a blocked claim. Eight deliberate native
refusals are expected; other script, sanitizer and memory-leak diagnostics fail
the test.

The existing Alice, Bioresearch and weekly encounter tests retain their explicit
inventory doubles; they are not evidence of native item delivery. The new
focused test supplies that coverage independently.

Partial-delivery ledgers belong to the current instance. Players must collect
remaining items before that instance expires. This change does not recover
historically lost grants or establish exactly-once SQL recovery after a process
crash. Instance-independent reward mail/receipts would require a separate
durable design. Manual client gameplay verification remains separate.

Next audit candidate: the Confused Boy paid material exchange, particularly
delivery failures after costs are deducted and item consumption during callbacks.
