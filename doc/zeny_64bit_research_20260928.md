# Wide Zeny: client and server evidence, 28 September 2026

## Findings

The local account bank already supports signed 64-bit balances, but the native
character wallet and several economic packet fields are still 32-bit. Increasing
the wallet constant alone is not a complete implementation. Independent client
and server work is possible; the missing implementation is a development task,
not evidence that MuhRO's source must be obtained.

MuhRO's 18 September patch announces wider wallets, trade, mail, vending,
buying stores and shop search, including large-price confirmation and summaries.
On 25 September it restores character-specific wallets, moves shared funds to
the bank, converts tickets/diamonds, and adds larger bank presets. The later
notes supersede the earlier shared-wallet design. Neither note specifies packet
layouts, source code, hook addresses, or an exact maximum. Therefore the target
9,223,372,036,854,775,807 is our signed-64-bit design choice, not a proven MuhRO
limit. Sources: [18 September](https://dis.muhro.eu/t/patch-notes-244-18-september-2026/5331),
[25 September](https://dis.muhro.eu/t/patch-notes-246-25-september-2026/5384).

## Local constraints that require implementation

Inspected base revision `1f4fd4cab8ad4f94f7729aa9e4590f898920aacc` plus current
working files; paths and line numbers below describe that inspection and can
move as the implementation proceeds.

| Path | Existing representation | Required work |
| --- | --- | --- |
| `src/common/mmo.hpp:82,571,1181` | MAX_ZENY INT_MAX; character Zeny int32; compile-time INT_MAX guard | Wide state plus full arithmetic audit |
| `src/map/pc.hpp:725,1463,1465` | Trade amount and pay/get APIs int32 | Wide accounting boundaries and callers |
| `src/char/char.cpp:308,972,1092,1813` | `%d`, SQLDT_INT32 and native character-list money | Wide persistence and separate legacy wire boundary |
| `src/common/packets.hpp:38` | Character-list money int32 | Client contract and character selection display |
| `src/map/clif.cpp:3734` | Zeny uses long parameter update; EXP uses a separate long-long update | Client Zeny handler must accept/store/display wide values |
| `src/map/vending.hpp:18` | Price uint32 | Listing, total, fee and purchase calculation changes |
| `src/map/buyingstore.hpp:26,52,59` | Budget int32/uint32 | Wide remaining budget and price accounting |
| `src/map/searchstore.hpp:66,76,95` | Filters/results uint32 | Wide sorting/filtering and wire fields |
| `src/map/packets_struct.hpp:2994,3105,3111,3430,3471` | Vending/buying/search wire prices and budgets uint32 | Versioned client/server replacements |
| `src/common/mmo.hpp:669` | Mail state Zeny uint32 | Wide persisted mail and mail arithmetic |
| `src/map/packets_struct.hpp:1815,1946,1977` | Some RodEx Zeny wire fields already int64 | Existing wide wire fields alone do not prove client input or server state support |
| `src/map/packets.hpp:338-357,541-550` | Native bank balance int64, wallet and requested transfer int32 | Companion bank protocol is distinct from native wallet capability |
| `sql-files/main.sql:219,815` | Character/mail Zeny unsigned INT | Migration and bind/format changes |
| `src/map/log.cpp:277`, `sql-files/logs.sql:239` | Existing custom log path int64/BIGINT | Preserve existing wide logging |

These are first-party source files in the repository. Current upstream still
defines an int32 character wallet and rejects MAX_ZENY above INT_MAX, as checked
on 28 September. This is consistent with the project's own limit documentation.
Sources: [upstream mmo.hpp](https://raw.githubusercontent.com/rathena/rathena/master/src/common/mmo.hpp),
[rAthena Zeny documentation](https://github.com/rathena/rathena/wiki/Zeny).

## Native client inventory

The checked executable `PN-Client/Ragexe.exe` has SHA-256
`73f72fea2458a4c2dfd1fcc5a2a8f7684485aecd52d612a3e68a27c828b4632d`,
size 10,629,120 bytes, PE32 machine 0x14c, no COFF symbols, no debug-directory
RVA, a `.themida` section, and an entry point at RVA 0x0412a058 in `.boot`.
Static ASCII scanning found no economic names or PDB path. The complete
reproducible inventory is [baseline_20260928.json](../client-patch/wide_zeny/baseline_20260928.json),
produced by [probe.py](../client-patch/wide_zeny/probe.py). Protection markers do
not prove patching impossible; they explain why static source/symbol assumptions
cannot establish usable native hook addresses for this executable.

A hidden/no-ignore filename search of the whole Data2026 workspace found only
the active executable, staged copies, font candidates, and a synthetic test
fixture. `WARP2026-Project/LastSession.yml` references original
`Data2026/Data/2026-02-19_Ragexe_1770960005.exe`; this path no longer exists.
Earlier compatibility reports also mention an old 20250604 reference executable,
but the workspace filename search did not locate that file. No native PDB, IDB,
I64, or DMP was found by that search. Search results are bounded to this workspace;
other disks, archives, or running-process memory were not exhaustively examined.

Existing [bank_transport.cpp](../client-patch/account_bank/bank_transport.cpp)
hooks `ws2_32!send` and `closesocket` to observe authentication and makes a
separate companion connection. It leaves game packet bytes unchanged. Existing
bank UI source is useful for window creation, exact int64 formatting, session
tracking and authenticated companion transport, but does not identify the native
wallet/trade/vending/buying/search model or handlers.

## Public patch availability

WARP describes itself as a script-driven patching toolkit for 32-bit Windows
applications. It can support an independently developed native patch, but is not
itself a wide-Zeny protocol. Its public `CustomVendingLimit.qjs` takes `D_Int32`
input and patches comparison immediates with `SetInt32`. The local WARP2026 copy
at revision `28533d6a34649d236a1dcefe349761909d410e1d` has the same relevant
limitation. The only other local Zeny-named patch found customizes barter number
separators. No complete wide-economic protocol patch was identified in the
reviewed public materials or local Scripts inventory. This is a scoped negative
search result, not a claim that nobody has written one.
Sources: [WARP repository](https://github.com/Neo-Mind/WARP),
[original CustomVendingLimit](https://raw.githubusercontent.com/Neo-Mind/WARP/master/Scripts/Patches/CustomVendingLimit.qjs),
[WARP2026 patch](https://raw.githubusercontent.com/zVictorHG/WARP2026-Project/main/Scripts/Patches/CustomVendingLimit.qjs).

## Feasible independent implementation paths

1. Implement exact signed-64-bit server accounting and persistence in an isolated
   branch. Audit intermediate multiplication, percentages, fees, scripts, save
   paths, inter-server struct sizes, mail, offline shops and transaction logs.
   Reject amounts before overflow; never use floating point for balances.
2. Define an authenticated, versioned extension contract and explicit capability
   bits for each economic surface. An unmodified peer must remain at the old
   limit or be rejected before entering a wide-enabled economy. Merely receiving
   a capability bit is insufficient evidence that a released client works.
3. Implement an independent client economy UI/model and companion protocol first,
   with real wallet state and exact input. This can prove end-to-end accounting
   without requiring native UI offsets. It is only equivalent feature coverage
   once trade, mail, shops, budgets, search and confirmation paths are supplied.
4. For native UI integration, map runtime wallet storage, inputs, formatters,
   packet readers/writers, list row structures and final confirmation arithmetic
   for the pinned executable. Preserve existing behavior below the old limit,
   and fail closed if byte signatures or protocol versions differ. Wide packets
   cannot be passed into legacy fixed-width structures unchanged.
5. Test two real clients plus relog/restart and mixed-version peers in staging.
   Required boundary values: 2^31, 2^32, 2^53+1, and INT64_MAX. Include fees,
   quantity-times-price overflow, cancellation, stale offers, retries, expired
   sessions, full recipient wallets, offline vending and mail return/claim.

The read-only probe and its four parser tests are implemented and pass. They
deliberately advertise no wide capability. An independent staged client now
implements the phase-1 wallet/bank companion described above. See the
[client README](../client-patch/wide_zeny/README.md): actual Windows control tests
pass at 100%, 125% and 150%, and production socket-hook loopback tests pass an
exact 2^53+1 request and INT64_MAX wallet reply. This is a working client protocol
and panel, not a native-economic hook map. The extension now also implements
exact bilateral trade offers, lock state and explicit revision-bound confirmation
using the authoritative Request80/Reply208 protocol. Native item selection is
retained; wide Zeny confirmation occurs in the new panel. The same DLL now
implements PMK1 market drafts, exact prices/budgets, purchases, plain-item sales
and search, plus PZL1 Zeny-only mail composition with exact fee quotes and durable
status handling. Actual Windows controls and fragmented socket fixtures pass for
each extension. The new forwarding loader selects only PNWallet64, preserves the
existing font module/settings, and passes a real DLL-chain startup test.
This remains independent companion coverage, not a native executable hook map.
Real staging-server gameplay acceptance and deployment remain integration work.
