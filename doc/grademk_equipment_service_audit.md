# Custom Grademk equipment-enchant entry points

Verified locally on 2026-09-06. The new
`npc/custom/grademk_equipment_enchants.txt` provides one explicitly custom menu
for five existing native enchant groups. This bounded task adds no equipment,
materials, acquisition route, price, recipe, quest/reputation gate, or client data.
It does not change any other service or group.

## Entry-route audit

Repository-wide NPC and item-script searches found no existing call that reaches
groups 63, 64, 147, 148, or 165. Nonliteral callers were inspected, not simply
discarded by a numeric search:

| Existing dynamic caller | Reachable groups |
| --- | --- |
| Biosphere initial equipment array | 16–19, 57–59 |
| Biosphere reformed equipment array | 52–55, 60–62 |
| Biosphere Depth arithmetic menu | 99–102 |
| Episode 20 Glacier arithmetic menu | 89–97 |
| Grademk Varmundt tier/element menu | 16–19, 52–55 |
| Grademk Constellation equipment menu | 7–13 |
| Grademk seasonal cloak/circle menu | 117–124, with signets separately at 142 |
| Grademk Frontier/Time ternary menu | 163, 164 |

Other `item_enchant` calls are literal groups and do not include the five missing
entry points. The only engine caller of `clif_enchantwindow_open` is the script
builtin. This is a source audit of the configured project, not a claim about
untracked live scripts or external plugins.

Group 24 already has usable item **101099 / ClockTower_Regulator**, whose script
calls `item_enchant(24)`. That item and group are left entirely alone.

## Menu and positioning

| Menu choice | Existing group | Effective target identities verified |
| --- | ---: | ---: |
| Flush Einbech | 63 | 19 |
| Muqaddas | 64 | 23 |
| Furious weapons | 147 | 40 |
| Furious crowns | 148 | 19 |
| Sky Rune crowns | 165 | 22 |
| Cancel / Escape | None | No window request |

All target names resolve through the effective Renewal item imports. The fixture
verifies each group has targets and slot definitions; it does not invent recipes
or material costs. The native enchant handler remains responsible for equipment
eligibility, costs, confirmation, and outcomes.

The proposed cell `34,184` was rejected because the enabled standard grade NPC
`Sratos#sratos` already occupies it (`npc/re/merchants/enchantgrade.txt`). The new
counter position is **Grademk 28,184**, immediately west of Tina at 30,184.
The test resolves the complete Renewal NPC configuration tree and checks all
enabled Grademk declarations for an exact coordinate collision.

The NPC cell 28,184, standing cell 28,181, existing aisle 38,177, and actual workshop
entrance 13,172 all decode as GAT 0 in the effective 200×200 Grademk map. A four-way
walkability search connects the entrance to the standing cell and aisle while
excluding occupied NPC cells. Cache priority is import → Renewal → base; Grademk
comes from `db/map_cache.dat`, SHA-256
`3d523caa567fdb6330c062824085bb19e1268940bec82fe360c1f14060e5b508`.
The standing cell is three tiles from the NPC; native `npc_checknear` allows
`AREA_SIZE+1`, with the project's configured area size 14. No live player was moved.

Routing uses only local `.@choice` and `.@group[]` values. Cancel closes the
dialogue, and native select's Escape terminates it. Every Open path yields at
`close2` before calling the real `item_enchant` builtin. The NPC has no grant,
deletion, payment, movement, or persistence command. Normal native `select` still
sets the attached player's transient `@menu` variable, as expected.

## Actual-VM regression

`tools/ci/grademk_equipment_service_test.py` extracts the complete NPC body without
rewriting its statements and freshly compiles `script.cpp`, `malloc.cpp`, and
`grademk_equipment_service_test.cpp` with ASan+UBSan and no recovery/suppression.

Result: **7 paths / 174 assertions / zero failures / zero native errors**, with
explicit `Memory manager: No memory leaks found.`. All five menu choices, Cancel,
and Escape execute through actual `parse_script`, `run_script`, `select`,
`close`/`close2`, and `item_enchant` behavior. Tests verify no early UI request,
exact group routing after close acknowledgement, only the attached player as
recipient, byte-identical inventory, unchanged zeny, no payment/deletion calls,
and correct detachment. The synthetic inventory identities are derived from
effective targets (540058 and 401059), not guessed IDs.

```sh
python3 tools/ci/grademk_equipment_service_test.py --build-dir ../grademk-equipment-service-20260906
```

The script fails if geometry, coordinate occupancy, or backend target identities
are unresolved. It also requires every path completion marker, the final native
result, leak-free shutdown, and no native/allocator/sanitizer diagnostic.

### Boundaries

Kernel seccomp denies socket/connect/bind/listen. Player/NPC lookup, transient
`@menu` persistence, outbound messages, UI transport, and payment/deletion services
are explicit isolated doubles. The real `item_enchant` builtin checks minimal
native group-existence entries and reaches the outbound window-request boundary.
It does **not** execute the client packet handler, weight checks, inventory
selection, material charging, actual enchanting, or native-client rendering.
The test deliberately leaves `item_enchant_index` inactive instead of simulating
successful packet delivery. Existing support objects are linked but are not all
freshly sanitizer-instrumented.

The complete body including `OnInit` parses; the title initializer itself is not
executed by the dialogue test. Geometry and proximity are read-only source/cache
checks, not an end-to-end native NPC click. No full server startup, SQL, SSH,
deployment, or active-client modification is performed by this task.

The new file was intentionally **not wired into a shared loader by this task**.
The reviewing parent can enable it with exactly one line in the custom NPC loader:

```text
npc: npc/custom/grademk_equipment_enchants.txt
```

Reviewed NPC SHA-256:
`62a4c017d9637653a14d2032aa9591aebde2f32e72c05ef66bd786679c88c1a7`.
Fresh script source SHA-256:
`6e01f947d419ae89527dc40ad37d0f184af1af37f86742a6ad315eefcd47fb8d`.
Fresh allocator SHA-256:
`064496e9722eeb1486178a6663aaa375ed7f312717986b9f41df3b69fd3a4a70`.
