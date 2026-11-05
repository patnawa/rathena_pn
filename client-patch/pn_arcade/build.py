"""Build the Minigame Arcade achievement overlay from an installed client.

The installed English text table is the base. The arcade rows are inserted before
its loader so every existing row, and the loader itself, stays byte-identical.
Plain-text .lub files are already loaded by this client (see the quest list).
"""
from pathlib import Path
import argparse
import hashlib
import json
import struct
import sys
import zlib

ROOT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT.parent / 'client_compat'))
from merge_grfs import read, merge

MARKER = b'function main()'
GRF_NAME = b'data\\System\\achievement_list.lub'


def merged_table(base):
    assert base.count(MARKER) == 1, 'Expected one achievement loader'
    assert b'pn_arcade' not in base, 'The arcade rows are already present'
    rows = (ROOT / 'arcade_achievements.lua').read_bytes().replace(b'\r\n', b'\n')
    newline = b'\r\n' if b'\r\n' in base else b'\n'
    return base.replace(MARKER, rows.replace(b'\n', newline) + MARKER)


def overlay(assets):
    body, table = bytearray(), bytearray()
    for name, raw in sorted(assets.items()):
        packed = zlib.compress(raw, 9)
        table += name + b'\0' + struct.pack('<IIIBI', len(packed), len(packed), len(raw), 1, len(body))
        body += packed
    packed = zlib.compress(table, 9)
    archive = (b'Master of Magic\0' + bytes(14) + struct.pack('<IIII', len(body), 0, len(assets) + 7, 0x200)
               + body + struct.pack('<II', len(packed), len(table)) + packed)
    assert read(archive) == assets
    return archive


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--client', type=Path, required=True, help='Installed client to read')
    parser.add_argument('--output', type=Path, required=True, help='Empty directory for the changed files')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    assert not any(args.output.iterdir()), 'Output directory must be empty'
    base = (args.client / 'SystemEN/achievements.lub').read_bytes()
    table = merged_table(base)
    old_repairs = (args.client / 'client_repairs.grf').read_bytes()
    existing = read(old_repairs)
    assert GRF_NAME in existing, 'Expected the existing repair entry'
    repairs = merge([overlay({GRF_NAME: table}), old_repairs])
    actual = read(repairs)
    assert actual[GRF_NAME] == table
    assert all(actual[name] == raw for name, raw in existing.items() if name != GRF_NAME), 'Unrelated repair entry changed'
    assert set(actual) == set(existing), 'Repair entries changed'
    files = {'SystemEN/achievements.lub': table, 'System/achievement_list.lub': table, 'client_repairs.grf': repairs}
    report = {}
    for name, raw in files.items():
        target = args.output / name
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(raw)
        report[name] = dict(bytes=len(raw), sha256=hashlib.sha256(raw).hexdigest())
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
