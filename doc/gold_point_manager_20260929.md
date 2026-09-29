# Private Gold Point Manager — 29 September 2026

Clicking the native top-right Gold icon sends `CZ_DYNAMICNPC_CREATE_REQUEST`
(`0x0a16`, name `GOLDPCCAFE`). The handler accepts only that exact service name
and summons an owner-scoped copy of `Gold Point Manager#FP` within two cells.
The player then clicks the NPC to open its menu. Other characters cannot see
or interact with it. Existing dynamic-NPC duplicate prevention, map restrictions,
and 60-second idle cleanup apply. Dead or busy characters cannot summon it.

The opening dialog keeps the balances visible while showing these four choices:

- Convert Gold Points to Fashion Points.
- Exchange style consumables (1 point each).
- View balances.
- Cancel.

Style consumables are the eight existing dyestuffs (975, 976, 978–983), costing
one Gold Point per item. Delivery is checked before charging; confirmation,
insufficient balance, inventory capacity, and cancellation are handled.
Conversions retain their existing amount confirmation, balance recheck, and
64-bit Fashion Point overflow protection.

`#FP_Gold` and `#FP_Fashion` remain login-account balances. The existing script
clock still awards one Gold Point per three minutes online and caps it at 50.
No new wallet, database migration, or parallel reward timer was introduced.
`@FP_GoldNextAt` records the deadline for the native icon's countdown.
`goldpointinfo` refreshes its owner-only `0x0a15` balance/countdown packet after
login, accrual, and spending; map-load handling restores the icon after warps.
The native client uses a 3,600-second elapsed field, so the remaining seconds
are encoded relative to that value.

The client overlay changes the Gold tooltip to “You can accumulate Gold Points
up to 50 points maximum.” and its heading to “Gold Points”. Both loose message
tables and their copies in `client_repairs.grf` agree. This is a text-resource
update, not an executable patch. The server enforces the cap. `/goldpc` toggles
the native icon if it was hidden in personal settings.

## Verification and delivery

The map-server builds successfully. The actual script-VM suite passes 105 cases
and 5,324 assertions under UndefinedBehaviorSanitizer, with clean allocator
teardown. Five deliberately broken variants are rejected. Tests include the
actual native owner-visibility predicate, four menu routes, conversion limits,
dyestuff delivery, insufficient funds/weight, cancellation, and an acknowledged
delivery with no inventory delta. UI and persistence remain fixture boundaries;
these tests do not claim a captured client click or wire-level integration test.

The deployment uses the current September 28 gameplay binary as its baseline,
verifies all unrelated live C/C++ source against the candidate, and retains
matched binary/script rollback files. The idle map-only restart completed with
all seven containers healthy, other containers unchanged, and financial and
Gold/Fashion balances unchanged.

The client GRF was read back and checked: exactly two resource entries changed
among 317, with all remaining payloads identical. The signed updater release is
`client-20260929-gold-manager`. Restart through `Launch PN.cmd` to install it.

Evidence, build logs, client preimages and publication tooling are in
`Server-Development/gold-manager-20260929`; server rollback is in
`/app/pn-gold-manager-20260929/deployment/before`. Final in-game visual acceptance
is pending: the Windows computer-use helper was unavailable in this session.

Protocol reference: [rAthena GoldPC proposal #7410](https://github.com/rathena/rathena/pull/7410).
The implementation reuses PN's existing account balance and clock rather than
adding the proposal's independent point system.
