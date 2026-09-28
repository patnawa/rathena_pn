# Wide-Zeny client v3, staged

`PNWallet64.dll` implements a separate Windows wallet/bank panel and authenticated
companion transport using protocol v3. It displays the server's real character
wallet and bank up to INT64_MAX and sends exact deposits/withdrawals up to that
limit. It rejects protocol v2 and all four legacy ticket/diamond exchange actions.
Its presets include +100K through +10B, with checked saturation at INT64_MAX.
Collect offline Zeny moves the account's offline character wallets into its bank
through the matching server transaction; the active character keeps its wallet.

The panel also implements full-width player trade offers and confirmation.
Start a normal trade in the game and review its items in the native window;
the extension shows the partner, both exact Zeny offers and each side's lock/
confirmation state. Set offer, Lock offer, Confirm trade and Cancel are explicit
actions. Final confirmation shows both amounts and defaults to No. Every action
carries the trade identity and reviewed revision; a changed revision during the
confirmation dialog sends no commit. The server may accept cancellation despite
a stale revision, but still requires the same trade identity.

The same DLL includes [Market](../wide_market/README.md) and
[Mail](../wide_mail/README.md) companion windows. They supply exact vending and
buying-store prices, budgets, search filters, purchase/sale confirmations and
Zeny-only mail composition. Native inventory/cart selection and item-bearing or
received RODEX mail remain in the game. This is an independent economy extension;
it does not widen the protected executable's native numeric controls. Matching
server guards must reject legacy operations that cannot represent the real value.

The source derives from the existing GPL account-bank implementation, preserving
its authenticated session observer, asynchronous requests, saved receipts and
stale-session protections. It contains no player passwords and never changes
native game packet bytes. `bank_protocol.hpp` is an exact snapshot of the staged
server header, SHA-256 `22ea9af05225470b0fa4724311b5d0d1d90f209429c73136810732f067ffa34d`.
The staged v3 ABI is Request80/Reply208; the earlier unreleased Request64/Reply136
v3 build is superseded and must not be mixed with this build.
Keep the client/server header synchronized; don't downgrade to v2 automatically.

Build and test from the repository root:

```powershell
client-patch/wide_zeny/build.ps1 -Output C:/path/to/isolated-build
C:/path/to/isolated-build/wide_zeny_client_ui_test.exe 100
C:/path/to/isolated-build/wide_zeny_client_ui_test.exe 125
C:/path/to/isolated-build/wide_zeny_client_ui_test.exe 150
C:/path/to/isolated-build/wide_zeny_client_transport_test.exe
C:/path/to/isolated-build/wide_market_numbers_test.exe
C:/path/to/isolated-build/wide_market_ui_test.exe
C:/path/to/isolated-build/wide_market_transport_test.exe
C:/path/to/isolated-build/wide_mail_ui_test.exe
C:/path/to/isolated-build/wide_mail_transport_test.exe
```

The UI tests exercise actual Win32 controls and production event handlers; the
transport test uses production API hooks and a loopback fixture. They verify
exact values at 2^31, 2^32, 2^53+1 and INT64_MAX, overflow rejection, large presets,
confirmed transfers, bilateral trade values, no automatic confirmation, changed
revision during a real confirmation event, terminal errors with an older
server request ID, duplicate suppression, fragmented authentication/replies,
unchanged game bytes, idle notifications and logout. All passed on 28 September.
The maximum-balance render was visually inspected at 100%, and the trade render
at 150%; rectangle containment and overlap were checked at all three scales.
Real staging-server gameplay acceptance is tracked separately by the integration
fixtures; these client tests do not substitute for that acceptance.

Load this DLL **instead of** the old `BankUI.dll` in a staged package; do not load
both because they own the same socket hooks. The loader must explicitly choose
the v3 module, and `PNWallet64.ini` must accompany it. No installation or binary
replacement is performed by the build script.

`stage.py` prepares a fresh isolated release directory, verifies all three protocol
headers against the selected server source, records payload hashes, and copies the
existing font module/settings into a loader QA directory. It preserves the old
panel scale and server ports. The new FontScale forwarding loader selects only
PNWallet64 and preserves FontScaleOriginal; the paired PNTurbo recognizes its
session-gate exports. Run `wide_zeny_loader_test.exe` from the generated `loader-qa`
directory to verify the actual shipping DLL chain and hidden startup state.
Staging changes no active client executable, Lua file or archive.

## Native executable inventory

`probe.py` is a read-only executable inventory. It does not load or change the
executable, install hooks, send packets, or advertise a server capability.
Successful probing means the PE was parsed, not that native transactions support
64-bit amounts.

Run from the repository root:

```powershell
python client-patch/wide_zeny/probe.py C:/Users/Alpha/Downloads/Compressed/Data2026/PN-Client/Ragexe.exe --output client-patch/wide_zeny/baseline_20260928.json
python tools/ci/wide_zeny_client_probe_test.py
```

The baseline is protected PE32 with a `.themida` section, a `.boot` entry point,
no COFF symbols, and no usable debug-directory RVA. These observations do not
prove client modification is impossible. They mean source-level declarations
and static string searches do not provide the native hooks needed for this task.

Before enabling the matching server feature, map and verify the wallet model,
native display/input arithmetic, trade confirmation, mail composition/claim,
vending setup/purchase/sale, buying-store budgets/purchases, and shop-search
filter/results. Required acceptance values include 2,147,483,648; 4,294,967,296;
9,007,199,254,740,993; and 9,223,372,036,854,775,807. Negative, overflow, stale
confirmation, session transition, and insufficient-balance cases must reject
without a partial transfer. Prices multiplied by stack counts and fees need
checked arithmetic. An extension must preserve protocol framing and authentication
and reject unsupported peers before wide transactions are enabled.

See [the research report](../../doc/zeny_64bit_research_20260928.md) for evidence
and the implementation paths. The existing account-bank DLL is not proof that
these native client paths are covered.
