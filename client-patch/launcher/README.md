# PN LAN launcher

`PNLauncher.exe` installs, updates, verifies, repairs and starts the complete
Windows client from `http://192.168.10.18:8082/`. Save the bootstrap executable
in an empty writable directory for a fresh installation, or use the supplied
`Launch PN.cmd` inside PN-Client. No additional .NET download is needed on this
Windows installation; the build targets .NET Framework 4.x.

The release envelope uses RSA-3072/SHA-256 and the embedded public key. The
private publisher key stays in the developer's user-only `.ssh` directory.
Every downloaded object is verified by length and SHA-256 before installation.
The launcher rejects redirects, invalid paths, reparse points and older feed
sequences. Initial bootstrap trust comes from the local client or this LAN
server; the executable does not silently replace itself. A launcher binary
upgrade is downloaded explicitly from the status page while the launcher is
closed.

Updates hash the installed release and fetch only changed files. An interrupted
installation restores its journaled originals on the next run. A completed
version update keeps one previous version for Rollback; same-version Repair
does not erase that backup. First installation has no previous version. A
rollback retains the highest feed sequence, preventing replay of an old network
release. The next Update reinstalls the current published version.

The publisher lists release files from the clean manifest, never by enumerating
the user's installation. Savedata, screenshots, replays and updater state cannot
appear in the signed feed. Existing AI settings and BankUI.ini are preserved.
Do not share a personal folder's savedata/screenshots when a clean download is
preferred. PN-Client is approximately 4.99 GiB; server development files are
outside it. Large future GRF updates need free space for the downloaded archive
and the previous version retained for rollback.

Build: `powershell -File build.ps1 -Output ABSOLUTE_DIRECTORY`. This also runs
the development console self-tests for signature/path rejection, interrupted
updates, idempotent recovery and preservation/promotion of rollback files.
`PNLauncherCheck.exe` belongs in development output only. A real LAN repair was
also tested by replacing and restoring the small FONT-FIX.txt file through the
signed object feed; no GRF was downloaded for that repair.

The LAN service is `tools/admin/lan_client_service.py`, installed as
`pn-lan-client.service`. Publisher and deployment evidence live in the workspace
`Server-Development/improvements-20260919` directory. There are no external
alerts or public-network endpoints in this release.
