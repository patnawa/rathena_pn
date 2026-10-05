# Native Zeny HUD and PN branding follow-up

The normal game HUD previously stopped at 2,147,483,647 even when Wallet & Bank showed the full balance. This follow-up renders authenticated signed-64-bit wallet values and separates Weight and Zeny into two right-aligned rows. The ordinary game packet remains clamped for compatibility.

## Implementation

The client verifies Ragexe SHA-256 73f72fea2458a4c2dfd1fcc5a2a8f7684485aecd52d612a3e68a27c828b4632d, decoded instruction guards, native callers and object layout before changing behavior. The executable file is unchanged. Unsupported executable versions retain their original behavior.

Expanded AP height changes from 149 to 164 pixels; non-AP height changes from 134 to 149. Compact heights are unchanged. Old saved heights are normalized through the native resize routine, which also positions child controls and the linked button strip. Existing windows migrate on the game thread with rollback on failed resize. A thread-local context distinguishes the new normal 149-pixel surface from an old AP surface of the same height.

Zeny uses its own full-width row at natural font size, sharing Weight's right edge five pixels inside the panel. The renderer extends the skin border and rejects incomplete or hole-containing destination clipping regions before modifying pixels. Weight retains the game's formatting and color logic.

The native text route supplies a scoped, verified owner/caller context. This is necessary because layout detours can remove the original wallet return frame from Windows stack unwinding. The earlier right-aligned candidate rejected that post-detour stack, leaving the extended row transparent and showing the legacy capped amount. A regression replays the observed missing-frame trace; unrelated or nested routes must not inherit wallet authority. The native GDI call prefix and imported call instruction remain verified.

A locked cache receives confirmed current-session wallet snapshots. Unauthorized or disconnected sessions invalidate it; pending saves retain the last confirmed value. Terminal read-only bank errors do not invalidate valid wallet snapshots. Render reads validate session generation after releasing the cache lock. Existing companion refreshes allow only one worker in flight, approximately every 500 ms while active and every three seconds in the background or while reconnecting.

## Server and project branding

The server snapshot considers all pc_transaction_pending categories. An authenticated, completed read-only refresh re-emits the ordinary clamped Zeny status so the native cached HUD redraws. This does not mutate balances. The deployed map binary is verified by hash; the other six containers are unchanged.

The welcome and version label identify rAthena PN. Player-facing project links, connection branding and Project Info labels use https://github.com/patnawa/rathena_pn, with its issues page for bug reports. Translation credits, licenses and reference links are preserved. The welcome includes Ctrl+B direct Zeny banking and @office. The server text update was activated through a verified map-only restart with unchanged wallet and bank rows.

## Verification

The user confirmed the full native balance 22,354,369,444 and live updates, then accepted separate AP rows, collapse/reopen behavior and bank closing on diagnostic DLL cc42f2edabb9326201ce66b20e03f543aec66e8d854fb4caeaefa7ab2a9a8110. The subsequent request changed Zeny from left to right alignment.

Bank tracing recorded four successful physical close commands with immediate hides and other working controls. Automated input did not reach the game client. No speculative bank input behavior change was added. Temporary input and decoded-code capture instrumentation was removed from product sources and archived in the workspace.

Regression checks cover server snapshot/authentication boundaries, stale sessions, logout, concurrent x86 cache reads, decoded guards, both native heights, saved heights, collapse/re-expand, resize rollback, the 149-pixel surface collision, clip holes, unrelated render paths, and natural-size right alignment from zero through 9,223,372,036,854,775,807. Wallet UI checks pass at 100%, 125% and 150%; bank, market and mail transport/UI suites pass.

Evidence is retained under Server-Development/native-zeny-20260928 and Server-Development/pn-welcome-20260928. The corrected final DLL b77fdbec764ea328e45b88c2badfd35a2c6611cb96b54c2973a58f65e32a65cb was visually verified in-game: native Zeny and Wallet & Bank both showed 10,000,000,000, with separate right-aligned rows, a solid white Zeny background and the button strip below the expanded panel. Its bounded trace confirms successful rendering; its final logged zero predates the current visual balance because the user transacted during the test. Client installation and signed-feed publication are tracked separately in the release receipts for client-20260928-native-zeny. Non-AP layout has decoded-code and automated coverage; live non-AP acceptance has not been recorded. This follow-up does not establish complete MuhRO parity or widen other legacy numeric widgets.

## Completed deployment

The matching server update is active on the PN Docker server. The installed and
published client release is `client-20260928-native-zeny`, sequence `2026092803`.
Installation passed quick and full manifest checks, and the launcher validated
the signed feed. Published changed objects were read back and checked by hash.
The live server binary, welcome configuration, published DLL and local client
were rechecked after deployment; all seven production containers were healthy.

Players receive the client fix by closing the game and running `Launch PN.cmd`.
The updater retains previous files for rollback. This was a LAN updater release,
not a new full GitHub client archive. Temporary fixture containers were removed
and diagnostic logging is disabled in the installed client.

The later [fashion shortcuts](main_office.md#gold-points-and-fashion-shortcuts)
are a separate server-script update and need no further client patch.
