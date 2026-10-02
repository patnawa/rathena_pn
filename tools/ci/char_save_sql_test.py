"""Verify the native character saver against disposable MariaDB and a DB kill.

Run on a Docker host with freshly built character objects. No production
configuration, database credentials or published container ports are used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--image', default='sha256:9edfb206479e0766e833cfcdeb7ce06ad447d318fa77b7dfae084face2f59028')
    parser.add_argument('--database-image', default='mariadb@sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979')
    args = parser.parse_args(); root = args.candidate.resolve(); out = args.evidence.resolve()
    if root == out or root in out.parents:
        raise ValueError('Evidence must be outside source')
    out.mkdir(parents=True, exist_ok=False)
    tag = 'pn-character-proof-' + uuid.uuid4().hex[:12]
    net, db, probe = tag + '-net', tag + '-db', tag + '-probe'
    created = []; network = False
    report = {'passed': False, 'production_database_accessed': False, 'checks': []}

    def run(*command, **kwargs):
        return subprocess.run(command, check=kwargs.pop('check', True), capture_output=True,
                              text=True, timeout=kwargs.pop('timeout', 120), **kwargs)

    def sql(query, check=True):
        return run('docker', 'exec', '-i', '-e', 'MYSQL_PWD=character-fixture-only', db,
                   'mariadb', '-uroot', '--batch', '--raw', '-N', input=query, check=check)

    def ready():
        for _ in range(100):
            if sql('SELECT 1', False).returncode == 0:
                return
            time.sleep(.25)
        raise RuntimeError('Fixture SQL not ready')

    def container(entry, command, network_name='none', detach=False):
        flags = ['docker', 'run'] + (['-d', '--name', probe] if detach else ['--rm'])
        flags += ['--network', network_name, '--memory', '2g', '--cpus', '2',
                  '--mount', f'type=bind,src={root},dst=/rathena,readonly',
                  '--mount', f'type=bind,src={out},dst=/evidence', '-w', '/rathena',
                  '--entrypoint', entry, args.image]
        return run(*(flags + command), timeout=300)

    def runtime(mode):
        result = container('/evidence/char-save-sql', [mode], net)
        (out / (mode + '.log')).write_text(result.stdout + result.stderr)
        if 'CHARACTER_SAVE_SQL_OK' not in result.stdout:
            raise RuntimeError('Missing success marker: ' + mode)
        report['checks'].append(mode)

    try:
        paths = [root / name for name in ('src/char/char.cpp', 'src/char/char.hpp',
                 'src/char/char_mapif.cpp', 'sql-files/main.sql',
                 'sql-files/upgrades/upgrade_20261002_character_save_atomicity.sql',
                 'tools/ci/char_save_sql_runtime.cpp', 'tools/ci/char_save_sql_test.py')]
        objects = sorted((root / 'src/char/obj').glob('*.o'))
        if not objects:
            raise RuntimeError('Build current character objects first')
        libraries = [root / name for name in ('src/common/obj/common.a',
                     '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        paths += objects + libraries
        paths += sorted(p for p in (root / 'src').rglob('*') if p.is_file() and p.suffix in ('.hpp', '.h', '.inc'))
        def hashes():
            return {str(p.relative_to(root)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
        report['inputs_sha256'] = hashes()
        report['images'] = {name: run('docker', 'image', 'inspect', '--format', '{{.Id}}', value).stdout.strip()
                            for name, value in (('toolchain', args.image), ('database', args.database_image))}
        flags = ['-std=c++17', '-O1', '-g', '-fsanitize=undefined', '-fno-sanitize=alignment',
                 '-fno-sanitize-recover=all', '-DPACKETVER=20260219']
        flags += ['-I/rathena/' + name for name in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                  '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')] + ['-I/usr/include/mysql']
        result = container('g++', flags + ['-c', 'src/char/char.cpp', '-o', '/evidence/char.o'])
        (out / 'compile.log').write_text(result.stdout + result.stderr)
        inputs = ['/rathena/' + p.relative_to(root).as_posix() for p in objects if p.name != 'char.o']
        inputs += ['/rathena/' + p.relative_to(root).as_posix() for p in libraries]
        result = container('g++', flags + ['tools/ci/char_save_sql_runtime.cpp', '/evidence/char.o'] + inputs +
                           ['-Wl,--wrap=main', '-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1',
                            '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', '/evidence/char-save-sql'])
        (out / 'link.log').write_text(result.stdout + result.stderr)
        report['binary_sha256'] = hashlib.sha256((out / 'char-save-sql').read_bytes()).hexdigest()
        run('docker', 'network', 'create', '--internal', net); network = True
        run('docker', 'run', '-d', '--name', db, '--network', net, '--network-alias', 'character-save-db',
            '--memory', '512m', '--cpus', '1', '-e', 'MARIADB_ROOT_PASSWORD=character-fixture-only', args.database_image)
        created.append(db); ready()
        schema = (root / 'sql-files/main.sql').read_text()
        def table(name):
            start = schema.index(f'CREATE TABLE IF NOT EXISTS `{name}`')
            return schema[start:schema.index(';', start) + 1]
        sql('CREATE DATABASE character_save_probe; USE character_save_probe;' +
            ''.join(table(name) for name in ('char', 'memo', 'skill', 'friends', 'mercenary_owner', 'hotkey')))
        # Exercise the actual upgrade from legacy engines, not just fresh DDL.
        sql('USE character_save_probe;' + ''.join(f'ALTER TABLE `{name}` ENGINE=MyISAM;'
            for name in ('char', 'memo', 'skill', 'friends', 'mercenary_owner', 'hotkey')))
        sql('USE character_save_probe;' + (root / 'sql-files/upgrades/upgrade_20261002_character_save_atomicity.sql').read_text())
        report['checks'].append('legacy-engine-migration')
        runtime('normal'); runtime('crash-setup')
        container('/evidence/char-save-sql', ['crash'], net, True); created.append(probe)
        for _ in range(120):
            if int(sql("SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE STATE='User sleep'").stdout.strip()):
                break
            if run('docker', 'inspect', '--format', '{{.State.Running}}', probe).stdout.strip() != 'true':
                raise RuntimeError('Crash probe exited before reaching the delayed SQL write')
            time.sleep(.25)
        else:
            raise RuntimeError('No delayed write observed')
        run('docker', 'kill', db)
        code = run('docker', 'wait', probe).stdout.strip()
        result = run('docker', 'logs', probe)
        (out / 'crash.log').write_text(result.stdout + result.stderr)
        if code != '0' or 'CHARACTER_SAVE_SQL_OK' not in result.stdout:
            raise RuntimeError('Crash failure not handled')
        report['checks'].append('database-kill')
        run('docker', 'start', db); ready(); runtime('crash-verify')
        if hashes() != report['inputs_sha256']:
            raise RuntimeError('Source or linked objects changed during proof')
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        if isinstance(error, subprocess.CalledProcessError):
            (out / 'failed-command.log').write_text((error.stdout or '') + (error.stderr or ''))
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
        for name in reversed(created):
            run('docker', 'rm', '-f', '-v', name, check=False)
        if network:
            run('docker', 'network', 'rm', net, check=False)
    print(json.dumps({'passed': report['passed'], 'checks': report['checks']}))


if __name__ == '__main__':
    main()
