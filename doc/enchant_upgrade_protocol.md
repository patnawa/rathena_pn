# Enchantment upgrade protocol evidence

## Local client evidence

The bundled `Ragexe_Server_20250604.exe` has SHA-256
`33d4d9af476b8d24b5954d38d121b2bbe93044681b2945bb9b235fd9b25990cb`.
Read-only disassembly shows two separate upgrade senders:

- Ordinary: function VA `0x00644c70`, packet `0x0b9d`.
- New random: function VA `0x00644d60`, packet `0x0bf0`. Stores from
  `0x00644dbe` and registration at `0x00a8e1bb` confirm the same 14-byte
  group/index/slot layout as the ordinary request.
- Guaranteed/selectable: function VA `0x00644b70`, packet `0x0bf1`.
  Stores at `0x00644bce` through `0x00644bf4` construct the layout below.
  The packet-length registration at `0x00a8e1cd` through `0x00a8e1da`
  independently registers `0x0bf1` as 18 bytes.
- The guaranteed call at `0x009b7860` passes the selected result item after
  the slot, inventory index, and 64-bit group. Its neighboring random call
  at `0x009b77e7` has no selected-result argument.

| Offset | Width | Field |
| --- | --- | --- |
| 0 | 2 | Packet ID `0x0bf1` |
| 2 | 8 | Enchant group |
| 10 | 2 | Client inventory index |
| 12 | 2 | Zero-based enchant slot |
| 14 | 4 | Requested result item ID |

The server declaration has compile-time size/offset assertions. This is static
evidence from the bundled 2025 client, not a capture of the active 2026 client's
button click. The candidate uses the production `PACKETVER=20260219`; explicit
20250604 and 20260219 syntax builds also pass. The newer path is enabled for
main clients from 2023-09-20, matching the local client helper's compatibility tier.

The pre-deployment live-file comparison found additional packet aliases absent
from the Git baseline. The `0x0bf0` random and `0x0bf2` reset routes are preserved.
The old live `0x0bf1` alias incorrectly invoked the initial perfect-enchant handler,
which reads the result ID at offset 12 instead of the guaranteed-upgrade offset 14.
That alias is replaced by the dedicated source/slot/result-validated handler.
The trailing byte's meaning in `0x0bf2` still needs separate investigation; this
change preserves its existing reset behavior, not a claim of full reset parity.

## Recipe and runtime checks

The active `EnchantList.lub` from highest-priority `nebula_upgrade_v2.grf` has
SHA-256 `664a6083b051233496611664af4c84d4ccd1c9e6ceb685edac6cacf60feda80d`.
Its `AddPerfectUpgradeEnchant` recipes are now separate `PerfectUpgrades`, keyed
by both existing enchant and requested result. Ordinary `Upgrades` and guaranteed
`PerfectUpgrades` never fall back to one another. Overlapping crown recipes retain
their distinct prices, materials, and random/guaranteed outcomes.

`tools/ci/audit_enchant_upgrades.py` compares source/target, Zeny, every material
quantity, and weighted results against the client. All 364 ordinary and 412
guaranteed recipes in workshop groups 7–13, 117–124, 142, 163, and 164 match. The
guaranteed import contains 444 recipes across all loaded groups, including the
20 overlapping crown upgrades and 16 final Dimension footwear upgrades.

`--compare-non-upgrades-ref 89092979c` verifies that the migrated workshop files'
other parsed YAML fields are unchanged. Comments/formatting are not part of that
comparison. Normal enchant/entry recipes were not synchronized by this migration.

`tools/ci/enchant_upgrade_test.cpp` tests the exact selectors called by both
packet handlers: 17 checks, including all 100000 possible random rolls, independent
prices for overlapping sources and multiple guaranteed targets, no cross-mode
fallback, wrong source/result rejection, and invalid/uncovered random draws. It
passes with AddressSanitizer and UndefinedBehaviorSanitizer and in Docker.

The isolated production-image build and `map-server --run-once` pass with no
script/database errors or memory leaks; the existing root-user warning remains.
These tests do not establish complete client interaction, charging/persistence,
combat effects, or full episode playthrough correctness.
