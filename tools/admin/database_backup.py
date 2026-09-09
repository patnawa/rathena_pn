#!/usr/bin/env python3
"""Create a locked, verified MariaDB backup without copying credentials into files.

Run on the Docker host. Restores are restricted to a new networkless container;
this tool has no production restore operation.
"""
import argparse
import datetime
import gzip
import hashlib
import json
import os
from pathlib import Path
import re
import subprocess
import tempfile
import time


def run(args, **kwargs):
    return subprocess.check_output(args, **kwargs)


def db_command(container, program, args):
    return ['docker', 'exec', container, 'sh', '-c',
            'export MYSQL_PWD="${MYSQL_ROOT_PASSWORD:-${MARIADB_ROOT_PASSWORD:?missing root password}}"; exec "$@"',
            'db-backup', program, '-uroot', *args]


def dump(container, database, target):
    # A single transaction does not cover MyISAM. Lock all tables for the dump.
    command = db_command(container, 'mariadb-dump', [
        '--lock-all-tables', '--hex-blob', '--routines', '--events', '--triggers',
        '--skip-comments', '--skip-dump-date', '--order-by-primary',
        '--databases', database])
    digest = hashlib.sha256()
    with tempfile.TemporaryFile() as errors, target.open('xb') as raw:
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=errors)
        try:
            with gzip.GzipFile(filename='', mode='wb', fileobj=raw, mtime=0) as packed:
                while block := process.stdout.read(1024 * 1024):
                    digest.update(block)
                    packed.write(block)
            if process.wait() != 0:
                raise RuntimeError('Database dump failed; incomplete backup is not published')
            raw.flush()
            os.fsync(raw.fileno())
        except BaseException:
            if process.poll() is None:
                process.kill()
                process.wait()
            raise
        finally:
            process.stdout.close()
    with gzip.open(target, 'rb') as stream:
        verified = hashlib.file_digest(stream, 'sha256').hexdigest()
    if verified != digest.hexdigest():
        raise RuntimeError('Compressed backup verification failed')
    return verified


def restore_check(source, database, image, expected):
    name = 'rathena-restore-check-' + str(os.getpid())
    created = False
    try:
        run(['docker', 'run', '-d', '--name', name, '--network', 'none',
             '--tmpfs', '/var/lib/mysql:rw', '-e',
             'MARIADB_ROOT_PASSWORD=isolated-restore-only', image])
        created = True
        for attempt in range(60):
            result = subprocess.run(db_command(name, 'mariadb', ['-N', '-e', 'SELECT 1']),
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if result.returncode == 0:
                break
            time.sleep(1)
        else:
            raise RuntimeError('Isolated restore database did not become ready')
        command = db_command(name, 'mariadb', [])
        command.insert(2, '-i')
        with gzip.open(source, 'rb') as stream, tempfile.TemporaryFile() as errors:
            process = subprocess.Popen(command, stdin=subprocess.PIPE, stderr=errors)
            try:
                while block := stream.read(1024 * 1024):
                    process.stdin.write(block)
                process.stdin.close()
                if process.wait() != 0:
                    raise RuntimeError('Isolated SQL restore failed')
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
        # CHECK TABLE runs only against the restored copy, never live tables.
        checked = run(db_command(name, 'mariadb-check', ['--check', database]), text=True)
        if any(not line.rstrip().endswith('OK') for line in checked.splitlines() if line.strip()):
            raise RuntimeError('Restored table integrity check failed')
        with tempfile.TemporaryDirectory(prefix='rathena-restore-') as directory:
            actual = dump(name, database, Path(directory) / 'restored.sql.gz')
        if actual != expected:
            raise RuntimeError('Restored SQL does not match the original dump')
        return {'passed': True, 'tables_checked': len(checked.splitlines()),
                'restored_sql_sha256': actual}
    finally:
        if created:
            subprocess.run(['docker', 'rm', '-f', name], check=True, stdout=subprocess.DEVNULL)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--container', default='rathena-db')
    parser.add_argument('--database', default='ragnarok')
    parser.add_argument('--output', type=Path, default=Path('/app/rathena-database-backups'))
    parser.add_argument('--check-restore', action='store_true')
    args = parser.parse_args()
    if not re.fullmatch(r'[A-Za-z0-9_]+', args.database):
        parser.error('database must be a simple SQL identifier')
    os.umask(0o077)
    args.output.mkdir(mode=0o700, parents=True, exist_ok=True)
    import fcntl
    with (args.output / '.backup.lock').open('a') as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        stamp = datetime.datetime.now(datetime.timezone.utc).strftime('%Y%m%dT%H%M%S%fZ')
        target = args.output / (args.database + '-' + stamp + '.sql.gz')
        partial = target.with_suffix(target.suffix + '.partial')
        report = {'created_utc': stamp, 'database': args.database,
                  'consistency': 'lock-all-tables', 'restore': {'performed': False}}
        try:
            report['sql_sha256'] = dump(args.container, args.database, partial)
            partial.rename(target)
            report['archive_sha256'] = hashlib.sha256(target.read_bytes()).hexdigest()
            report['archive_bytes'] = target.stat().st_size
            if args.check_restore:
                image = run(['docker', 'inspect', '--format', '{{.Image}}', args.container], text=True).strip()
                report['restore'] = restore_check(target, args.database, image, report['sql_sha256'])
            report['passed'] = True
        except BaseException:
            partial.unlink(missing_ok=True)
            report['passed'] = False
            raise
        finally:
            target.with_suffix('.json').write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps({'backup': str(target), **report}, indent=2))


if __name__ == '__main__':
    main()
