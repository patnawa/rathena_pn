#!/usr/bin/env python3
"""Emit/verify a scoped textual-source deployment manifest, without writing files.

Creation: deploy_scope_manifest.py create archive.tar.gz BASE_GIT_REF
Verify old live files: deploy_scope_manifest.py before manifest.json /live/root
Verify installed files: deploy_scope_manifest.py after manifest.json /live/root
Missing old files are safe additions. Existing unrelated modifications fail.
Only CRLF/LF normalization is accepted for an existing baseline comparison.
The after check requires exact bytes. Binaries must be checked separately.
"""
import hashlib
import json
from pathlib import Path, PurePosixPath
import subprocess
import sys
import tarfile


def digest(data, normalized=False):
    return hashlib.sha256(data.replace(b'\r\n', b'\n') if normalized else data).hexdigest()


def safe_path(name):
    path = PurePosixPath(name)
    if path.is_absolute() or '..' in path.parts or not path.parts:
        raise ValueError(f'Unsafe archive path: {name}')
    return path


def main():
    mode, source, target = sys.argv[1:]
    if mode == 'create':
        entries = []
        with tarfile.open(source, 'r:gz') as archive:
            for member in archive:
                if not member.isfile():
                    raise ValueError(f'Nonregular archive entry: {member.name}')
                safe_path(member.name)
                old = subprocess.run(['git', 'show', f'{target}:{member.name}'], capture_output=True)
                incoming = archive.extractfile(member).read()
                entries.append({'path': member.name, 'before': digest(old.stdout, True) if old.returncode == 0 else None,
                                'after': digest(incoming)})
        if len({e['path'] for e in entries}) != len(entries):
            raise ValueError('Duplicate archive paths')
        print(json.dumps({'base': target, 'archive_sha256': digest(Path(source).read_bytes()), 'entries': entries}, indent=2))
        return
    if mode not in ('before', 'after'):
        raise ValueError('Expected create, before or after')
    root = Path(target).resolve()
    manifest = json.loads(Path(source).read_text())
    failures = []
    for entry in manifest['entries']:
        path = (root / safe_path(entry['path'])).resolve()
        path.relative_to(root)
        if not path.exists():
            if mode == 'after':
                failures.append(f'Missing: {entry["path"]}')
            continue
        data = path.read_bytes()
        if digest(data) == entry['after']:
            continue
        if mode == 'before' and digest(data, True) == entry['before']:
            continue
        failures.append(f'Drift: {entry["path"]}')
    print(json.dumps({'mode': mode, 'checked': len(manifest['entries']), 'failures': failures}, indent=2))
    if failures:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
