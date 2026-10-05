# Grade Workshop enchant audit — 2026-10-05

The Workshop's Star of Spell upgrade previously built one menu containing both
eligible enchant slots for every Star Signet in inventory. With 25 eligible
items, the actual script VM emits a 2,106-byte menu warning: the native client
limit truncates the menu and removes Cancel. With a full 200-slot expanded
inventory, enumerating the 400 choices also exhausts the normal VM execution
budget before the selection menu appears. Identical copies had identical labels.

Both the consolidated Equipment Enchanter and the retained Constellation template
now show ten choices per page, with Next, Previous and Cancel controls. Labels
show the inventory slot, refine and enchant slot. The inventory scan temporarily
disables the execution budget only around a loop bounded by native inventory
size and two enchant slots; it restores the guard before any dialogue yields.
The selected page offset is validated before resolving the private item snapshot.
The existing compare-and-swap mutation, fees, probabilities and deductions are
unchanged.

## Evidence and regression coverage

`tools/ci/workshop_enchant_audit_test.py` extracts the actual NPC bodies and uses
the native script VM, item metadata parser, inventory enchant mutation and
material deletion. It compiles script.cpp, malloc.cpp and its generated driver
fresh with AddressSanitizer and UndefinedBehaviorSanitizer. Network operations
are denied by the kernel. Player lookup, register persistence, logging and UI
delivery are explicit boundaries; no server or production database is started.
Other native map objects provide link dependencies.

The final test covers 261 scenarios and 12,208 assertions:

- All 198 menu routes across 110 referenced recipe groups, including Back,
  Cancel, Escape and the crown reputation threshold.
- All four Star of Spell slot/level prices through both upgrade entry points,
  preservation of every other equipment byte and upgrades with no free slot.
- Every required material independently missing, insufficient Zeny, confirmation
  cancellation, stale identities, all four stale card fields, changed equip
  state, unidentified targets, invalid quantities and invalid target IDs.
- Empty inventories, equipped/rental items and non-upgradeable enchant levels.
- All 400 item/slot choices reachable with distinct labels, Previous navigation,
  and a paid upgrade of the final eligible copy on the last page.
- Normal script execution limits restored before every dialogue suspension and
  leak-free teardown.

Read-only recipe audits compare the effective Renewal imports with the repository
client recipe sources in `client-patch/enchant_repair/source`. All 164 groups
match their initial enchant eligibility, costs, materials, resets and outcomes;
all 339 normal probability tables have valid totals. The 2,401 selectable initial
enchants, 1,269 client ordinary upgrade recipes and 444 guaranteed upgrade recipes
resolve without a mismatch or missing item name. Existing server-only custom
upgrade recipes are retained. No recipe database or client asset changes are
needed for this fix.

Additional checks exercise the native grading selection/commit handlers, enchant
probability and upgrade selectors, inventory metadata helper and Workshop map
reachability. The new VM regression is included in the full release checks.

On Linux with built map support objects and PyYAML:

```sh
python3 -B tools/ci/workshop_enchant_audit_test.py --build-dir /tmp/pn-workshop-audit
```

Rendered game-client interaction remains a separate acceptance check. The tests
verify generated menu limits and actual server execution, not client rendering.
