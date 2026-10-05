"""Run the shipping PowerShell preflight against disposable client fixtures."""
import argparse
from pathlib import Path
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--launcher', type=Path, default=ROOT / 'client-patch/client_usability/tools/client/start-client.ps1')
    args = parser.parse_args()
    with tempfile.TemporaryDirectory() as folder:
        root = Path(folder)
        (root / 'SystemEN').mkdir()
        (root / 'SystemEN/itemInfo.lua').write_text('ImportFiles = {}')
        (root / 'Ragexe.exe').write_bytes(b'Never execute this fixture')
        required_files = ('PNTurbo.dll', 'PNTurboConfig.exe', 'PNWallet64.ini', 'PNWallet64.dll',
                          'FontScale.ini', 'FontScale.dll', 'FontScaleOriginal.dll',
                          'SystemEN/AccountBankInfo.lua')
        for name in required_files:
            (root / name).write_text('Scale=1.25\n' if name == 'FontScale.ini' else 'Presence fixture only')
        header = b'Master of Magic\0' + bytes(26) + struct.pack('<I', 0x200)
        for i in range(11):
            (root / f'patch{i}.grf').write_bytes(header)
        cases = [
            ('ten slots', ''.join(f'{i}=patch{i}.grf\n' for i in range(10)), True, '10 archives'),
            ('eleven slots', ''.join(f'{i}=patch{i}.grf\n' for i in range(11)), False, 'archive limit exceeded'),
            ('slot ten', '10=patch0.grf\n', False, 'archive limit exceeded'),
            ('duplicate priority', '0=patch0.grf\n0=patch1.grf\n', False, 'Duplicate archive priority'),
            ('empty archive list', '', False, 'no archives'),
            ('escape path', '0=../patch0.grf\n', False, 'Use a GRF filename'),
        ]
        for name, entries, passed, message in cases:
            (root / 'DATA.INI').write_text('[Data]\n' + entries)
            result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                '-File', str(args.launcher.resolve()), '-CheckOnly', '-ClientRoot', str(root)],
                capture_output=True, text=True, timeout=30)
            assert (result.returncode == 0) == passed and message in result.stdout, (name, result.stdout, result.stderr)
            print('PASS:', name)
        (root / 'DATA.INI').write_text('[Data]\n0=patch0.grf\n')
        for loader in ('ImportFiles = {"missing.lua"}',
                       'ImportFiles = {"present.lua", "missing.lua"}',
                       'ImportFiles = {\n "present.lua",\n "missing.lua",\n}'):
            (root / 'SystemEN/present.lua').write_text('tbl = {}')
            (root / 'SystemEN/itemInfo.lua').write_text(loader)
            result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                '-File', str(args.launcher.resolve()), '-CheckOnly', '-ClientRoot', str(root)],
                capture_output=True, text=True, timeout=30)
            assert result.returncode != 0 and 'Missing file: SystemEN/missing.lua' in result.stdout, (loader, result.stdout, result.stderr)
            print('PASS: missing item-info import rejected:', repr(loader))
        for loader in ('ImportFiles = {"present.lua", -- "comment.lua"\n}',
                       'ImportFiles = {--[=[ "comment.lua" ]=]\n "present.lua"}'):
            (root / 'SystemEN/itemInfo.lua').write_text(loader)
            result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                '-File', str(args.launcher.resolve()), '-CheckOnly', '-ClientRoot', str(root)],
                capture_output=True, text=True, timeout=30)
            assert result.returncode == 0, (loader, result.stdout, result.stderr)
            print('PASS: commented item-info import ignored:', repr(loader))
        (root / 'SystemEN/itemInfo.lua').write_text('ImportFiles = {}')
        for removed in (('PNWallet64.ini', 'PNWallet64.dll'), *[(name,) for name in required_files]):
            saved = {name: (root / name).read_bytes() for name in removed}
            for name in removed:
                (root / name).unlink()
            result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                '-File', str(args.launcher.resolve()), '-CheckOnly', '-ClientRoot', str(root)],
                capture_output=True, text=True, timeout=30)
            assert result.returncode != 0, ('false pass', removed, result.stdout)
            for name in removed:
                assert 'Missing file: ' + name in result.stdout, result.stdout
                (root / name).write_bytes(saved[name])
            print('PASS: required component removal rejected:', ', '.join(removed))
        (root / 'FontScaleOriginal.dll').unlink()
        (root / 'PNWallet64.ini').write_text('[Bank]\nCharacterPort=6121\nMapPort=5121\n')
        for complete in (False, True):
            if complete:
                for name in ('PNWallet64.dll','FontScale.dll','FontScaleOriginal.dll','SystemEN/AccountBankInfo.lua'):
                    (root / name).write_bytes(b'Presence fixture only')
            result = subprocess.run(['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                '-File', str(args.launcher.resolve()), '-CheckOnly', '-ClientRoot', str(root)],
                capture_output=True, text=True, timeout=30)
            assert (result.returncode == 0) == complete, result.stdout
            if not complete: assert 'Missing file: FontScaleOriginal.dll' in result.stdout
            print('PASS: bank extension', 'complete' if complete else 'missing dependencies rejected')


if __name__ == '__main__':
    main()
