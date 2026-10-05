#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  client_preflight.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/client_preflight.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Read-only active-client inventory. Not a packet or rendered-asset test."""
import argparse
import configparser
import hashlib
import json
import re
import struct
from pathlib import Path


def inspect(root, expected_packetver, hash_archives=False):
    root = Path(root).resolve()
    issues = []
    ini = configparser.ConfigParser(interpolation=None)
    try:
        ini.read(root / 'DATA.INI', encoding='utf-8-sig')
    except (configparser.Error, UnicodeError, OSError) as exc:
        issues.append('Invalid DATA.INI: ' + str(exc))
        ini = configparser.ConfigParser(interpolation=None)
    archives = []
    if not ini.has_section('Data'):
        issues.append('DATA.INI missing [Data] section')
    else:
        entries = []
        for key, name in ini.items('Data'):
            if not re.fullmatch(r'[0-9]+', key):
                issues.append('Invalid archive priority: ' + key)
                continue
            try:
                priority = int(key.lstrip('0') or '0')
            except ValueError:
                issues.append('Invalid archive priority: ' + key)
                continue
            entries.append((priority, name))
        entries.sort(key=lambda row: row[0])
        if not entries:
            issues.append('DATA.INI has no archives in its [Data] section')
        if len(entries) > 10 or any(key > 9 for key, _ in entries):
            issues.append('Client archive limit exceeded: DATA.INI supports only slots 0 through 9')
        if [k for k, _ in entries] != list(range(len(entries))):
            issues.append('GRF priorities must be contiguous from zero')
        seen = set()
        for priority, name in entries:
            if re.search(r'[/\\:]', name) or not name.lower().endswith('.grf'):
                issues.append('Use a GRF filename in DATA.INI: ' + name)
                continue
            path = (root / name).resolve()
            if not path.is_relative_to(root):
                issues.append('Archive escapes client directory: ' + name)
                continue
            record = {'priority': priority, 'file': name}
            if name.lower() in seen:
                issues.append('Duplicate archive: ' + name)
            seen.add(name.lower())
            if not path.is_file():
                issues.append('Missing archive: ' + name)
            else:
                record['bytes'] = path.stat().st_size
                with path.open('rb') as stream:
                    header = stream.read(46)
                    version = struct.unpack_from('<I', header, 42)[0] if len(header) == 46 else None
                    classic = header.startswith(b'Master of Magic\0') and version == 0x200
                    modern = header.startswith(b'Event Horizon\0') and version == 0x300
                    if not (classic or modern):
                        issues.append('Unreadable GRF header: ' + name)
                    record['format'] = 'GRF v3' if modern else 'GRF standard' if classic else 'invalid'
                    if hash_archives:
                        stream.seek(0)
                        digest = hashlib.sha256()
                        for block in iter(lambda: stream.read(1024 * 1024), b''):
                            digest.update(block)
                        record['sha256'] = digest.hexdigest()
            archives.append(record)
    executables = [p.name for p in root.glob('*.exe')]
    if not (root / 'Ragexe.exe').is_file():
        issues.append('Missing game executable: Ragexe.exe')
    return {'issues': issues, 'archives': archives, 'executables': executables,
            'server_packetver_to_verify': expected_packetver,
            'not_verified': ['EXE packet date (filename/PE date is not proof)',
                             'GRF asset coverage and override correctness',
                             'job sprites, effects, maps and Battle Mode in game']}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('client_root', type=Path)
    parser.add_argument('--packetver', default='20260219')
    parser.add_argument('--hash-archives', action='store_true')
    args = parser.parse_args()
    report = inspect(args.client_root, args.packetver, args.hash_archives)
    print(json.dumps(report, indent=2))
    raise SystemExit(bool(report['issues']))
