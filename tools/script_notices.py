#!/usr/bin/env python3
# ============================================================================
#  PN  /  SOURCE SIGNATURE
#  script_notices.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/script_notices.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Maintain PN source banners without changing script bodies or client payloads."""
import argparse
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'doc/script_notices.json'


def insertion(raw):
    offset = 3 if raw.startswith(b'\xef\xbb\xbf') else 0
    lines = raw[offset:].splitlines(keepends=True)
    if lines and lines[0].startswith(b'#!'):
        offset += len(lines.pop(0))
    if lines and re.search(br'coding[:=]\s*[-\w.]+', lines[0]):
        offset += len(lines[0])
    return offset


def banner(row, manifest, newline=b'\n'):
    prefix = row['comment']
    lines = [
        '=' * 76,
        ' PN  /  ' + row['category'].upper(),
        ' ' + Path(row['path']).name,
        '-' * 76,
        ' Project contributions: (C) 2026 ' + manifest['team'],
        ' License for project contributions: GPL-3.0-or-later; see LICENSE.',
        ' Source: ' + manifest['source'] + '/blob/main/' + row['path'],
        ' Existing upstream authors, notices and other rights are retained.',
        '=' * 76,
    ]
    return newline.join((prefix + ' ' + line).encode('ascii') for line in lines) + newline + newline


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--apply', action='store_true', help='Insert missing banners; default only checks')
    args = parser.parse_args()
    manifest = json.loads(MANIFEST.read_text(encoding='utf-8'))
    missing = []
    inline = companion = 0
    for row in manifest['scripts']:
        path = ROOT / row['path']
        if not path.is_file():
            raise SystemExit('Missing source: ' + row['path'])
        if row['notice'] == 'companion':
            companion += 1
            continue
        raw = path.read_bytes()
        offset = insertion(raw)
        newline = b'\r\n' if b'\r\n' in raw[:4096] else b'\n'
        expected = banner(row, manifest, newline)
        if not raw[offset:].startswith(expected):
            if b'PN  /  ' in raw[offset:offset + 180]:
                raise SystemExit('Existing PN banner needs review: ' + row['path'])
            if args.apply:
                path.write_bytes(raw[:offset] + expected + raw[offset:])
            else:
                missing.append(row['path'])
        inline += 1
    if missing:
        raise SystemExit('Missing PN source banners:\n' + '\n'.join(missing))
    print(f'PN notices: {inline} inline, {companion} companion; source files present.')


if __name__ == '__main__':
    main()
