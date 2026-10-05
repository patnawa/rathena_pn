# PN Client — Midgard launcher and settings

**5 October 2026 · Windows client · Packet baseline `20260219`**

[Download the release](https://github.com/patnawa/rathena_pn/releases/tag/client-2026-10-05-midgard).

![Midgard launcher overview](../images/midgard-launcher-20261005.png)

*Native dashboard render; the online state is an illustrative fixture.*

The navy-and-gold launcher adds Midgard artwork, matching PN and Ragnarok
icons, live connection checks, readable update progress and quick access to
game settings. The Settings page offers display and sound setup, Turbo,
snapshots of personal settings, automatic status refresh and minimizing after
the game starts. Activity and copied diagnostics help investigate failed updates.

![Launcher Settings](../images/midgard-settings-20261005.png)

## Download and install

| Asset | Use |
| --- | --- |
| `PN-Client-20261005-Midgard.7z.001` | Full client; start extraction here |
| `PN-Client-20261005-Midgard.7z.002` | Required continuation volume |
| `PN-Client-20261005-Midgard.7z.003` | Required continuation volume |
| `PN-Launcher-20261005.zip` | Dashboard installer for an existing client |
| `PNLauncher.exe` | Standalone LAN bootstrap; downloads signed game files |
| `INSTALL.txt` | Installation and settings instructions |
| `SHA256SUMS.txt` | SHA-256 hashes for every other download asset |
| `pn-download-manifest.json` | Exact paths, sizes and hashes of packaged files, excluding itself |
| `signed-client-feed.json` | Original signed game manifest used as the package baseline |

Download all three full-client volumes into one directory, then extract `.001`
once with [7-Zip](https://www.7-zip.org/). Do not extract each part separately.
The package contains **5,666 files, 5,369,994,499 bytes** after extraction. Allow
at least **12 GiB** free for downloads and extraction; future updates may also
need room for staging and rollback.

In the extracted `PN-Client` folder, run **Install PN Launcher.cmd**, then
**PN Launcher.lnk** or **Launch PN Dashboard.cmd**. The installer creates
shortcuts pointing to the extracted folder and fetches the settings tool.
Choose Settings, check for updates, then Play. Keep the supplied `DATA.INI`
order and companion DLLs. The game and update feed require access to the PN
LAN at `192.168.10.18`.

For an existing installation, extract `PN-Launcher-20261005.zip` into
`PN-Client`. Close the game and launcher, then run
`PN-Launcher-20261005/Install Launcher.cmd`. The ZIP contains no game archives;
use the dashboard to receive signed game updates.

The standalone bootstrap downloads the existing signed game baseline. To add
OpenSetup and local shortcuts afterward, use the small launcher package.
The original signed `Launch PN.cmd` still selects the previous launcher. Use
the new shortcut or `Launch PN Dashboard.cmd` for the refined interface.

## OpenSetup

The installer obtains the official **OpenSetup 3.5.0.692 normal RagnarokOnline
Lua edition without telemetry**, verifies the pinned ZIP hash and keeps the
author's documentation and license. Its binary is downloaded directly from
[Ai4rei/AN](https://nn.ai4rei.net/dev/opensetup/#download); it is not mirrored
inside these release assets.

Internet access is required for the normal installer. For offline installation,
download the matching ZIP from the author's page and pass
`-OpenSetupArchive ABSOLUTE_ZIP_PATH` to `install-dashboard.ps1`. Use
`-SkipOpenSetup` to install just the launcher. Original `Setup.exe` remains
available, and the Settings page falls back to it when OpenSetup is absent.

PN uses `LuaSaveData=1`, English and DirectX 9. Installing does not save or
reset personal Lua settings. Close Ragnarok before changing settings, then
use OK/Apply inside OpenSetup. The Settings backup button copies display,
hotkeys, chat, UI and Turbo files into `.pn-updater/settings-backups/`; it is
a manual snapshot. Restore desired files only while the game and settings
tools are closed. Launcher preferences are saved separately.

## Status and validation

![PN LAN status](../images/midgard-status-20261005.png)

The [LAN status page](http://192.168.10.18:8082/) reports login, character,
world and game web availability separately from maintenance/backup health.
Stale or failed responses clear previous green indicators. The screenshot is
dated evidence; use the page for current availability.

The package is assembled only from the existing signed
`client-20261003-chapter1-quests-clock` file list, sequence `2026092906`, plus
explicit dashboard and installer additions. Every game file is checked by
size and SHA-256. Local files that differ are fetched from the signed LAN
objects. The feed signature is checked by the shipping launcher. Both the
full archive and the small ZIP are extracted or read back and validated.
The extracted full client passes the shipping client preflight.

Personal savedata, screenshots, replays, logs, updater backups and developer
files are excluded. Launcher self-tests and all six status-service regression
checks pass. OpenSetup native saving preserved all 92 existing setting keys
in an isolated fixture using the client's Lua resources. No complete gameplay
session was rerun for this interface release. Packaging does not modify the
signed game feed or production game binaries.

Build and package sources: [launcher](../../client-patch/launcher/README.md),
[clean release builder](../../tools/release/midgard_20261005/build_client_release.py).
PN companion code is GPL-3.0-or-later; OpenSetup is by Ai4rei/AN under CC BY-NC
4.0. Original game assets retain their respective owners' rights.
