# Patcher 2.2 publication - 7 October 2026

The signed `client-20261007-patcher` release (sequence `2026092909`) is published
on the PN LAN updater and installed in PN-Client. Open `PN Launcher.lnk` or
`Launch PN Dashboard.cmd`; both select `PNLauncher-20261007.exe`.

The native updater installed four changed paths, downloading 1.2 MB. Only the
launcher, its two entry commands, and release metadata changed. The signed update
preserves game assets and personal preferences. Full native local verification
passed after installation and shortcut refresh. The installed signed envelope
matches the public feed exactly.

Before publication, a disposable client passed native signed upgrade, exact
rollback, rejection of the older feed after rollback, and reupgrade. The Windows
build passed 15 groups of signature, recovery, file cache, cancellation, retry,
settings, and startup tests. All seven production services remained healthy with
the same container identities and start times; no game restart was required.

The deployed launcher source is commit
`6198b1b46d2403504fb8faf12f6f8f8c0091a3ea`. Production preimages are retained at
`/app/pn-patcher-20261007/production-backup`. Publication placed immutable objects
first and atomically replaced the signed feed last.

See [implementation and measurements](patcher_improvements_20261007.md) and the
[deployment receipt](evidence/patcher_deployment_20261007.json). Full test logs and
native previews remain in the workspace at `OPS/patcher-20261007`.
