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
from pathlib import Path


def inspect(root, expected_packetver, hash_archives=False):
    root = Path(root).resolve()
    issues = []
    ini = configparser.ConfigParser()
    ini.read(root / 'DATA.INI')
    archives = []
    if not ini.has_section('Data'):
        issues.append('DATA.INI missing [Data] section')
    else:
        entries = sorted(ini.items('Data'), key=lambda row: int(row[0]))
        if len(entries) > 10 or any(int(key) > 9 for key, _ in entries):
            issues.append('Client archive limit exceeded: DATA.INI supports only slots 0 through 9')
        if [int(k) for k, _ in entries] != list(range(len(entries))):
            issues.append('GRF priorities must be contiguous from zero')
        seen = set()
        for priority, name in entries:
            path = (root / name).resolve()
            path.relative_to(root)  # Reject paths escaping the client directory.
            record = {'priority': int(priority), 'file': name}
            if name.lower() in seen:
                issues.append('Duplicate archive: ' + name)
            seen.add(name.lower())
            if not path.is_file():
                issues.append('Missing archive: ' + name)
            else:
                record['bytes'] = path.stat().st_size
                with path.open('rb') as stream:
                    signature = stream.read(16)
                    if not signature.startswith((b'Master of Magic', b'Event Horizon')):
                        issues.append('Invalid GRF signature: ' + name)
                    record['format'] = 'GRF v3' if signature.startswith(b'Event Horizon') else 'GRF standard'
                    if hash_archives:
                        stream.seek(0)
                        digest = hashlib.sha256()
                        for block in iter(lambda: stream.read(1024 * 1024), b''):
                            digest.update(block)
                        record['sha256'] = digest.hexdigest()
            archives.append(record)
    executables = [p.name for p in root.glob('*.exe')]
    if not executables:
        issues.append('No client executable found')
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
