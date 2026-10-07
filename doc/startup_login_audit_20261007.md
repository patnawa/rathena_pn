# Startup and login audit - 7 October 2026

The first automated startup checks passed against the installed PN-Client:
the shipping readiness script accepts its ten GRF archives and item-info imports.
The disposable PowerShell regression passes its 22 valid/missing-file,
archive-limit, path, and import cases.

## Confirmed startup cancellation defect

The launcher could start Ragnarok after Cancel when verification had already
returned successfully but the UI had not yet processed worker completion.
The completion handler checked only the worker exception and ignored the
cancellation token. Queued Play therefore survived this particular timing.

The native WinForms regression holds the worker completion callback, queues Play,
clicks the actual Cancel control, then delivers completion. The supplied launch
recorder observes the attempted launch without executing Ragnarok. It fails on
the shipping implementation and passes after the handler captures cancellation
before disposing the token source. Successful verification, failed verification,
and closing still retain their expected launch counts.

Cancellation feedback now says that Ragnarok was not started. It does not claim
that no update was installed, because a cancellation request arriving after
activation can prevent Play without undoing a completed update.

The complete native launcher build and existing self-tests pass. The fix is
prepared in source; this audit does not claim a signed production publication.
Local red and green evidence is retained under `OPS/startup-audit-20261007`
(the initial red log is `OPS/startup-red.log`).

## Pending acceptance

Rendered Play-to-login, character selection, entering a map, duplicate game
launches, and reconnect timing have not been certified by this audit. No new
game-loading timing claim is made. The Windows Computer Use plugin is enabled,
its local runtime exists, and its native named pipe is present, but this chat's
tool catalog initially exposed no `node_repl`/Computer Use action. Existing CLI
tests and offscreen WinForms tests do not replace rendered acceptance.

The app logs subsequently established the startup failure: both `node_repl` and
`cua_repl` failed with Windows error 267 because this chat's original
`Downloads/Compressed/Data2026/Client-Packages` working directory had been removed.
The same runtime failed to start from that missing directory. Restoring the empty
directory made the runtime start successfully. Both configured MCP servers then
passed standard initialization and tool discovery, exposing `js` and `js_reset`.
No desktop action, app permission change, or helper IPC bypass was used for this
diagnostic. The receipt is retained in
`OPS/startup-audit-20261007/computer-use-repair.json`. The chat still needs to load
the repaired servers before rendered acceptance can resume.
