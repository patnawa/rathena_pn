"""Build clean GitHub assets from the existing signed PN feed; never publish it.

Requires a built Windows launcher and 7-Zip. Output must not already exist.
Only signed files are copied from the installed client. Mismatches come from
the hash-addressed LAN objects. OpenSetup is installed from its author, never
mirrored in these archives. Extracted output is checked against every hash.
"""
import argparse
import base64
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import urllib.request
import zipfile

ROOT = Path(__file__).resolve().parents[3]
FEED = 'http://192.168.10.18:8082/'
TAG = 'client-2026-10-05-midgard'
BASELINE = 'ae72d5a98c921159b1302fac8077cdb16da67dd9d09f74c1f8e21b8c8f4e3fd0'
INSTALL = """PN RAGNAROK - MIDGARD CLIENT - 5 OCTOBER 2026

Full installation:
1. Download all PN-Client-20261005-Midgard.7z.001/.002/.003 volumes
   into the same folder. Download 7-Zip from https://www.7-zip.org/.
2. In 7-Zip, extract .001 once into a fresh writable folder. The other
   volumes are read automatically. Allow at least 12 GiB free for downloads
   and extraction; later updates may also retain a rollback copy.
3. Open PN-Client and run Install PN Launcher.cmd. This creates shortcuts
   for this folder and installs OpenSetup 3.5.0.692 from its author (internet
   access required). Existing game settings and original Setup are preserved.
4. Run PN Launcher.lnk or Launch PN Dashboard.cmd. Use Settings to select
   resolution, graphics and sound; close the game before applying settings.
5. Check for updates, then Play. The game and signed feed require the PN LAN
   at 192.168.10.18. Status: http://192.168.10.18:8082/.

Existing client:
Extract PN-Launcher-20261005.zip into PN-Client. Close the game and launcher,
then run PN-Launcher-20261005\\Install Launcher.cmd. This installs only the
dashboard, icons and official settings tool. Use Check for updates to obtain
the signed game files separately. No GRF is included in the small update.

Offline OpenSetup installation:
Visit https://nn.ai4rei.net/dev/opensetup/#download and download the stable
normal RagnarokOnline Lua 3.5.0.692 no-telemetry ZIP. The launcher installer
accepts -OpenSetupArchive ABSOLUTE_ZIP_PATH. Use -SkipOpenSetup for dashboard
installation only; original Setup.exe remains available.

Archive baseline: client-20261003-chapter1-quests-clock, signed sequence
2026092906. Midgard dashboard additions are separate from that signed feed.
Launch PN.cmd still follows the older signed entry point; use PN Launcher.lnk
or Launch PN Dashboard.cmd for the refined dashboard. The signed game feed
and server addresses have not been changed by this packaging operation.

No personal savedata, screenshots, replays, logs, backups or developer state
is included. The manifest describes distributed files, not personal settings.
Verify asset hashes against SHA256SUMS.txt before extraction.

Validation: launcher self-tests, status service regression checks, signed feed
verification, every packaged file hash and extracted archive hashes. OpenSetup
native saving retained all 92 existing setting keys in an isolated fixture.
A full gameplay session was not retested for this UI release. The status page
separately reports game availability and maintenance/backup health.

PN companion source: https://github.com/patnawa/rathena_pn/tree/client-2026-10-05-midgard
PN launcher code: GPL-3.0-or-later (LICENSE included).
OpenSetup: Ai4rei/AN, CC BY-NC 4.0; fetched directly from the official site,
with its documentation and license retained by the installer.
Original Ragnarok materials retain their respective owners' rights.
"""


def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def no_links(path):
    for part in (path, *path.parents):
        if part.is_symlink() or part.is_junction():
            raise ValueError('Linked path refused: ' + str(part))


def download(name, target):
    url = FEED + name
    with urllib.request.urlopen(url, timeout=60) as response:
        if response.geturl() != url:
            raise ValueError('Redirect refused')
        with target.open('xb') as stream:
            shutil.copyfileobj(response, stream, 1024 * 1024)


def copy(source, destination):
    no_links(source)
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, destination)


def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding='utf-8', newline='\r\n')


def run(args, log, cwd=None):
    with log.open('w', encoding='utf-8') as stream:
        subprocess.run([str(x) for x in args], cwd=cwd, stdout=stream,
                       stderr=subprocess.STDOUT, check=True)


def file_rows(root):
    return [{'path': p.relative_to(root).as_posix(), 'bytes': p.stat().st_size,
             'sha256': digest(p)} for p in sorted(root.rglob('*')) if p.is_file()]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--sevenzip', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    client, build, out = (p.absolute() for p in (args.client, args.build, args.output))
    for path in (client, build, out):
        no_links(path)
    out.mkdir(parents=True, exist_ok=False)
    assets = out / 'assets'
    assets.mkdir()
    signed = assets / 'signed-client-feed.json'
    download('release.json', signed)
    if digest(signed) != BASELINE:
        raise ValueError('Signed feed changed; review the new baseline first')
    run([build / 'PNLauncherCheck.exe', '--verify-manifest', signed], out / 'signature.log')
    manifest = json.loads(base64.b64decode(json.loads(signed.read_bytes())['payload']))
    full = out / 'staging' / 'PN-Client'
    full.mkdir(parents=True)
    fetched = []
    print('Preparing', len(manifest['files']), 'signed files', flush=True)
    for i, row in enumerate(manifest['files']):
        name = row['path']
        source, target = client / name, full / name
        no_links(source)
        target.parent.mkdir(parents=True, exist_ok=True)
        if source.is_file() and source.stat().st_size == row['bytes'] and digest(source) == row['sha256']:
            shutil.copyfile(source, target)
        else:
            download('objects/' + row['sha256'], target)
            fetched.append(name)
        if target.stat().st_size != row['bytes'] or digest(target) != row['sha256']:
            raise ValueError('Packaged object mismatch: ' + name)
        if i % 1000 == 0:
            print('Verified', i + 1, 'signed files', flush=True)

    launcher = ROOT / 'client-patch' / 'launcher'
    update = out / 'update' / 'PN-Launcher-20261005'
    update.mkdir(parents=True)
    copy(build / 'PNLauncher.exe', update / 'PNLauncher.exe')
    for name in ('install-dashboard.ps1', 'install-opensetup.ps1'):
        copy(launcher / name, update / name)
    for name in ('pn-launcher.ico', 'ragexe.ico'):
        copy(launcher / 'assets' / 'midgard' / name, update / 'assets' / 'midgard' / name)
    copy(ROOT / 'LICENSE', update / 'LICENSE')
    write(update / 'INSTALL.txt', INSTALL)
    wrapper = ('@echo off\ncd /d "%~dp0"\n'
               'powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0install-dashboard.ps1" '
               '-Build "%~dp0." -ClientRoot "%~dp0.."\npause\n')
    write(update / 'Install Launcher.cmd', wrapper)
    with zipfile.ZipFile(assets / 'PN-Launcher-20261005.zip', 'x', zipfile.ZIP_DEFLATED) as archive:
        for path in sorted(update.rglob('*')):
            if path.is_file():
                archive.write(path, path.relative_to(update.parent).as_posix())
    copy(build / 'PNLauncher.exe', assets / 'PNLauncher.exe')
    copy(build / 'PNLauncher.exe', full / 'PNLauncher.exe')
    copy(build / 'PNLauncher.exe', full / 'PNLauncher-20261005.exe')
    for path in sorted(update.rglob('*')):
        if path.is_file() and path.name not in ('PNLauncher.exe', 'Install Launcher.cmd', 'INSTALL.txt'):
            copy(path, full / 'tools' / 'pn-launcher' / path.relative_to(update))
    for name in ('pn-launcher.ico', 'ragexe.ico'):
        copy(launcher / 'assets' / 'midgard' / name, full / 'PN-Branding' / 'Midgard' / name)
    write(full / 'Install PN Launcher.cmd',
          '@echo off\ncd /d "%~dp0"\npowershell.exe -NoProfile -ExecutionPolicy Bypass '
          '-File "%~dp0tools\\pn-launcher\\install-dashboard.ps1" -Build "%~dp0." -ClientRoot "%~dp0."\npause\n')
    write(full / 'Launch PN Dashboard.cmd',
          '@echo off\ncd /d "%~dp0"\nstart "" "%~dp0PNLauncher-20261005.exe"\n')
    write(full / 'MIDGARD-INSTALL.txt', INSTALL)
    copy(signed, full / 'pn-signed-release.json')
    rows = file_rows(full)
    result = {'schema': 1, 'release': TAG, 'signed_game_release': manifest['release'],
              'signed_sequence': manifest['sequence'], 'signed_feed_sha256': BASELINE,
              'files': rows}
    metadata = assets / 'pn-download-manifest.json'
    write(metadata, json.dumps(result, indent=2))
    copy(metadata, full / metadata.name)
    write(assets / 'INSTALL.txt', INSTALL)
    print('Creating three full-client volumes', flush=True)
    run([args.sevenzip, 'a', '-t7z', '-mx=0', '-v1800m', '-bd', '-bso0',
         assets / 'PN-Client-20261005-Midgard.7z', 'PN-Client'], out / 'archive.log', full.parent)
    parts = sorted(assets.glob('*.7z.*'))
    if len(parts) != 3 or any(p.stat().st_size >= 2 * 1024 ** 3 for p in parts):
        raise ValueError('Unexpected release volume sizes')
    print('Testing and extracting full-client volumes', flush=True)
    run([args.sevenzip, 't', parts[0], '-bd', '-bso0'], out / 'archive-test.log')
    extracted = out / 'extracted'
    run([args.sevenzip, 'x', parts[0], '-o' + str(extracted), '-bd', '-bso0'], out / 'archive-extract.log')
    expected = {r['path']: r for r in rows}
    expected[metadata.name] = {'path': metadata.name, 'bytes': metadata.stat().st_size, 'sha256': digest(metadata)}
    actual = {r['path']: r for r in file_rows(extracted / 'PN-Client')}
    if actual != expected:
        raise ValueError('Extracted full-client manifest mismatch')
    run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File',
         extracted / 'PN-Client' / 'tools' / 'client' / 'start-client.ps1', '-CheckOnly',
         '-ClientRoot', extracted / 'PN-Client'], out / 'client-preflight.log')
    with zipfile.ZipFile(assets / 'PN-Launcher-20261005.zip') as archive:
        if archive.testzip() is not None:
            raise ValueError('Launcher update ZIP failed CRC validation')
        for path in update.rglob('*'):
            if path.is_file() and archive.read(path.relative_to(update.parent).as_posix()) != path.read_bytes():
                raise ValueError('Launcher update ZIP content mismatch')
    if any(name.split('/')[0].lower() in ('.pn-updater', 'savedata', 'screenshot', 'replay', 'memo') for name in actual):
        raise ValueError('Personal directory included')
    checksums = ''.join(digest(p) + '  ' + p.name + '\n' for p in sorted(assets.iterdir()))
    write(assets / 'SHA256SUMS.txt', checksums)
    report = {'passed': True, 'release': TAG, 'signed_files': len(manifest['files']),
              'distributed_files': len(actual), 'distributed_bytes': sum(r['bytes'] for r in actual.values()),
              'downloaded_mismatches': fetched, 'extracted_hashes_verified': True,
              'launcher_zip_verified': True, 'client_preflight_passed': True,
              'personal_directories_excluded': True, 'opensetup_mirrored': False,
              'gameplay_retested': False, 'assets': file_rows(assets)}
    write(out / 'verification.json', json.dumps(report, indent=2))
    print(json.dumps({k: v for k, v in report.items() if k not in ('downloaded_mismatches', 'assets')}), flush=True)


if __name__ == '__main__':
    main()
