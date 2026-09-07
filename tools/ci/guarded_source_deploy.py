#!/usr/bin/env python3
"""Scoped source deployment; plan by default. Does not restart services or edit SQL.

Run only during a maintenance window with the map service stopped. Requires a
deploy_scope_manifest.py manifest and a successful isolated startup test log.
Backups must be outside the live root, in a NEW directory. Rollback restores
existing files; newly introduced files are moved into backup/quarantine.
"""
import argparse
import json
from pathlib import Path
import shutil
import subprocess
import tarfile

from deploy_scope_manifest import digest, safe_path


def target(root, name):
    path = root / safe_path(name)
    if path.is_symlink() or any(p.is_symlink() for p in path.parents):
        raise ValueError('Symlink target rejected: ' + name)
    path.resolve().relative_to(root)
    return path


def check_stopped(container):
    result = subprocess.run(['docker', 'inspect', '--format', '{{.State.Running}}', container],
                            check=True, capture_output=True, text=True)
    if result.stdout.strip() != 'false':
        raise ValueError('Stop the map container during maintenance before applying or rolling back')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('mode', choices=['plan', 'apply', 'rollback'])
    parser.add_argument('--root', type=Path, required=True)
    parser.add_argument('--backup', type=Path, required=True)
    parser.add_argument('--archive', type=Path)
    parser.add_argument('--manifest', type=Path)
    parser.add_argument('--startup-log', type=Path)
    parser.add_argument('--container', default='rathena-map')
    args = parser.parse_args()
    root, backup = args.root.resolve(), args.backup.resolve()
    if not root.is_dir() or root == Path(root.anchor) or len(root.parts) < 3:
        raise ValueError('An explicit project directory is required')
    if backup == root or root in backup.parents or backup in root.parents:
        raise ValueError('Backup must be outside, and not an ancestor of, live root')
    if args.mode == 'rollback':
        check_stopped(args.container)
        receipt = json.loads((backup / 'receipt.json').read_text())
        if receipt['root'] != str(root):
            raise ValueError('Receipt root mismatch')
        for entry in reversed(receipt['entries']):
            dest = target(root, entry['path'])
            if dest.exists() and digest(dest.read_bytes()) not in (entry['after'], entry['old_hash']):
                raise ValueError('Refusing to overwrite post-deployment drift: ' + entry['path'])
            if entry['existed']:
                saved = target(backup / 'files', entry['path'])
                if digest(saved.read_bytes()) != entry['old_hash']:
                    raise ValueError('Backup checksum mismatch')
        for entry in reversed(receipt['entries']):
            dest = target(root, entry['path'])
            if entry['existed']:
                saved = target(backup / 'files', entry['path'])
                if digest(saved.read_bytes()) != entry['old_hash']:
                    raise ValueError('Backup checksum mismatch')
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(saved, dest)
            elif dest.exists():
                saved = target(backup / 'quarantine', entry['path'])
                saved.parent.mkdir(parents=True, exist_ok=True)
                shutil.move(str(dest), str(saved))
        print('Rollback complete; new files retained in backup/quarantine. Services remain stopped.')
        return
    if not args.archive or not args.manifest:
        parser.error('--archive and --manifest required')
    manifest = json.loads(args.manifest.read_text())
    if digest(args.archive.read_bytes()) != manifest['archive_sha256']:
        raise ValueError('Archive checksum mismatch')
    entries = manifest['entries']
    if not entries or len({e['path'] for e in entries}) != len(entries):
        raise ValueError('Empty or duplicate manifest')
    contents = {}
    with tarfile.open(args.archive, 'r:gz') as archive:
        for member in archive:
            safe_path(member.name)
            if not member.isfile() or member.name in contents:
                raise ValueError('Only unique regular files are allowed')
            contents[member.name] = archive.extractfile(member).read()
    if set(contents) != {e['path'] for e in entries}:
        raise ValueError('Archive/manifest path mismatch')
    for entry in entries:
        dest = target(root, entry['path'])
        if digest(contents[entry['path']]) != entry['after']:
            raise ValueError('Incoming checksum mismatch')
        entry['existed'] = dest.exists()
        old = dest.read_bytes() if dest.exists() else None
        if old is None and entry['before'] is not None:
            raise ValueError('Missing baseline file: ' + entry['path'])
        entry['old_hash'] = digest(old) if old is not None else None
        if old is not None and digest(old) != entry['after'] and digest(old, True) != entry['before']:
            raise ValueError('Live source drift: ' + entry['path'])
    print(json.dumps({'mode': args.mode, 'root': str(root), 'paths': list(contents)}, indent=2))
    if args.mode == 'plan':
        return
    check_stopped(args.container)
    if not args.startup_log:
        parser.error('--startup-log required after isolated candidate validation')
    log = args.startup_log.read_text(errors='replace')
    ready = "Server is 'ready' and listening" in log or 'Map Server is now online' in log
    if not ready or '[Error]' in log or '[Fatal' in log:
        raise ValueError('Candidate startup log is not clean/ready')
    backup.mkdir(parents=True, exist_ok=False)
    for entry in entries:
        if entry['existed']:
            saved = target(backup / 'files', entry['path'])
            saved.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(target(root, entry['path']), saved)
    (backup / 'receipt.json').write_text(json.dumps({'root': str(root), 'entries': entries}, indent=2))
    shutil.copy2(args.startup_log, backup / 'candidate-startup.log')
    # Complete backup/receipt precedes the first mutation. On failure use rollback.
    for entry in entries:
        dest = target(root, entry['path'])
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(contents[entry['path']])
        if digest(dest.read_bytes()) != entry['after']:
            raise ValueError('Installed checksum mismatch; roll back using the receipt')
    print('Applied and verified. Start map, inspect fresh logs, then run in-game acceptance checks.')


if __name__ == '__main__':
    main()
