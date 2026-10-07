# Patcher 2.2 - 7 October 2026

The launcher prepares an installed client in the background on opening. A Play click
during this check queues one launch after successful verification. It never launches
after a failed check or a close request. Settings can disable automatic checking.
Play installed client validates the saved signed manifest and local files without
contacting the patch server. The game still uses its existing PN server connection.

## Speed measurements

Measured on the installed 5,650 managed files (5,365,150,782 bytes), with no checksum
mismatches. An earlier baseline run took 13.637 seconds before the operating system
file cache was warm. The comparison below uses a warm filesystem cache and a new
launcher engine for each initial verification; results vary with the disk and PC.

| Check | Original | Repaired |
|---|---:|---:|
| Initial verification in a fresh engine | 6.953 s | 3.234 s |
| Repeat check in the same engine | 4.002 s | 1.428 s |

Native offscreen first-frame previews, including process startup and PNG output,
take 146-151 ms in three runs. This retains the existing quick window startup;
network and verification work begin on a worker after the window is shown.

The launcher retains read-only Windows handles for managed files so their checksums
can be reused safely in one session. Every lookup checks the current file identity.
INI files remain editable and are rehashed, and preserved preferences retain their
existing behavior. Closing releases all handles. No checksum cache is trusted across
process starts. Sequential reads and reduced filesystem probes improve the initial
check; hashing remains SHA-256 and signatures remain RSA/SHA-256.

## Reliability checks

The Windows build runs real loopback HTTP tests. Cancel stops a stalled response in
about 160 ms, rather than waiting for the HTTP timeout. Temporary 503 responses and
truncated bodies retry within a maximum of three attempts. Invalid checksums are
refused without retries. Failed or cancelled transfers remove their owned partial
files and never create an installation transaction. Checksum cancellation is checked
between 1 MiB blocks. The existing journaled recovery and rollback checks still pass.

Native WinForms tests drive the actual Play and completion callbacks with a supplied
launch recorder, so no game process is started. They cover queued Play success,
failure, and closing during preparation. Signed local verification passes against
the full installed client. Fixture tests reject forged saved metadata, same-length
corruption and missing managed files, while preserving personal preferences.

Build and test with `client-patch/launcher/build.ps1 -Output ABSOLUTE_BUILD_PATH`.
The console build accepts `--verify-installed CLIENT_ROOT`, `--update CLIENT_ROOT`
and `--rollback CLIENT_ROOT`; the graphical launch remains unchanged. Native preview
images and complete build logs are retained under `OPS/patcher-20261007` in the
workspace, outside this source repository. This validation does not start Ragnarok
or measure login/game loading time.
