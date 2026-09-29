"""Real shop SQL atomicity/retry/crash proof in a disposable internal Docker network.

Run on the Docker host, with a fully built candidate. Never connects to a configured
game database. Evidence belongs outside the candidate source tree.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--image', default='pn-bank-validation:20260929')
    parser.add_argument('--database-image', default='mariadb:noble')
    args = parser.parse_args()
    candidate, evidence = args.candidate.resolve(), args.evidence.resolve()
    if evidence == candidate or candidate in evidence.parents:
        raise ValueError('Evidence must be outside candidate')
    evidence.mkdir(parents=True, exist_ok=True)
    tag = 'pn-shop-proof-' + uuid.uuid4().hex[:12]
    db, net, probe = tag + '-db', tag + '-net', tag + '-probe'
    created_containers, network_created = [], False
    report = {'passed': False, 'production_database_accessed': False, 'database': 'shop_recovery_probe'}

    def run(command, **kwargs):
        return subprocess.run(command, check=True, **kwargs)

    def sql(query, check=True):
        return subprocess.run(['docker', 'exec', '-i', db, 'mariadb', '--protocol=TCP', '-h127.0.0.1',
                               '-uroot', '-pshop-fixture-only', '--batch', '--raw', '-N'],
                              input=query, text=True, capture_output=True, check=check)

    def ready():
        for _ in range(100):
            if sql('SELECT 1', False).returncode == 0:
                return
            time.sleep(.25)
        raise RuntimeError('Disposable database did not become ready')

    def runtime_command(mode, background=False):
        command = ['docker', 'run']
        command += ['-d', '--name', probe] if background else ['--rm']
        command += ['--network', net, '--memory', '1g', '--cpus', '1',
                    '--mount', 'type=bind,src=' + str(evidence) + ',dst=/evidence',
                    '--mount', 'type=bind,src=' + str(candidate) + ',dst=/rathena,readonly',
                    '-w', '/rathena', '--entrypoint', '/evidence/shop-sql-probe', args.image, mode]
        return command

    def runtime(mode, background=False):
        command = runtime_command(mode, background)
        if background:
            result = run(command, capture_output=True)
            created_containers.append(probe)
            return result
        result = subprocess.run(command, text=True, capture_output=True, timeout=90)
        (evidence / (mode + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(mode + ' failed: ' + (result.stdout + result.stderr)[-5000:])
        return result.stdout

    try:
        # Never remove or reuse a resource not created by this invocation.
        for name, kind in ((db, 'container'), (probe, 'container'), (net, 'network')):
            if subprocess.run(['docker', kind, 'inspect', name], capture_output=True).returncode == 0:
                raise RuntimeError('Fixture name already exists: ' + name)
        run(['docker', 'network', 'create', '--internal', net], capture_output=True)
        network_created = True
        run(['docker', 'run', '-d', '--name', db, '--network', net, '--network-alias', 'shop-recovery-db',
             '--memory', '768m', '--cpus', '1', '-e', 'MARIADB_ROOT_PASSWORD=shop-fixture-only',
             args.database_image], capture_output=True)
        created_containers.append(db)
        ready()
        schema = (candidate / 'sql-files/main.sql').read_text()
        migration = candidate / 'sql-files/upgrades/upgrade_20260929_shop_purchase.sql'
        sql('CREATE DATABASE shop_recovery_probe; USE shop_recovery_probe;\n' + schema + '\n' + migration.read_text())
        source_paths = ['tools/ci/shop_recovery_sql_runtime.cpp', 'src/custom/shop_commit.hpp',
                        'src/custom/shop_sql.inc', 'sql-files/main.sql', str(migration.relative_to(candidate))]
        report['input_sha256'] = {p: hashlib.sha256((candidate / p).read_bytes()).hexdigest() for p in source_paths}
        compiler = ['g++', '-std=c++17', '-g', '-O1', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    '-DPACKETVER=20260219']
        compiler += ['-I/rathena/' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                                                '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
        compiler += ['-I/usr/include/mysql', '/rathena/tools/ci/shop_recovery_sql_runtime.cpp']
        compiler += ['/rathena/' + str(p.relative_to(candidate)) for p in sorted((candidate / 'src/char/obj').glob('*.o'))]
        compiler += ['/rathena/' + p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                              '3rdparty/rapidyaml/obj/ryml.a')]
        compiler += ['-Wl,--wrap=main', '-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto',
                     '-lresolv', '-lm', '-o', '/evidence/shop-sql-probe']
        built = subprocess.run(['docker', 'run', '--rm', '--network', 'none', '--memory', '2g', '--cpus', '2',
                                '--mount', 'type=bind,src=' + str(candidate) + ',dst=/rathena,readonly',
                                '--mount', 'type=bind,src=' + str(evidence) + ',dst=/evidence',
                                '--entrypoint', compiler[0], args.image] + compiler[1:],
                               text=True, capture_output=True, timeout=180)
        (evidence / 'build.log').write_text(built.stdout + built.stderr)
        if built.returncode:
            raise RuntimeError('Compile failed: ' + built.stderr[-5000:])
        output = runtime('normal')
        assert 'SHOP_SQL_PASS' in output, output
        report['runtime'] = output.strip()
        report['character_wire_handler'] = runtime('wire').strip()
        runtime('seed-race')
        buyers = [(mode, subprocess.Popen(runtime_command(mode), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT))
                  for mode in ('race-a', 'race-b')]
        race_outcomes = []
        for mode, process in buyers:
            output, _ = process.communicate(timeout=60)
            (evidence / (mode + '.log')).write_text(output)
            assert process.returncode == 0, mode + ': ' + output
            race_outcomes.append(output.strip())
        report['concurrent_buyers'] = race_outcomes
        report['concurrent_final_unit'] = runtime('verify-race').strip()
        # A committed operation, a newer ordinary save, then an actual DB and
        # fixture-process restart. The original receipt must not replay old data.
        runtime('seed-restart')
        run(['docker', 'restart', db], capture_output=True, timeout=30)
        ready()
        report['committed_restart'] = runtime('verify-restart').strip()
        runtime('crash', background=True)
        for _ in range(80):
            state = sql("SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE DB='shop_recovery_probe' AND STATE='User sleep'", False)
            if state.returncode == 0 and state.stdout.strip() == '1':
                break
            time.sleep(.25)
        else:
            raise RuntimeError('Did not reach pre-receipt crash trigger')
        run(['docker', 'kill', '--signal', 'KILL', db], capture_output=True)
        code = run(['docker', 'wait', probe], text=True, capture_output=True, timeout=20).stdout.strip()
        logs = run(['docker', 'logs', probe], text=True, capture_output=True)
        (evidence / 'crash.log').write_text(logs.stdout + logs.stderr)
        assert code == '0' and 'CRASH_RETURN 0' in logs.stdout, logs.stdout + logs.stderr
        run(['docker', 'start', db], capture_output=True)
        ready()
        report['uncommitted_restart'] = runtime('verify-rollback').strip()
        report['passed'] = True
        print(json.dumps(report, indent=2))
    finally:
        for name in reversed(created_containers):
            subprocess.run(['docker', 'rm', '-f', '-v', name], capture_output=True)
        if network_created:
            subprocess.run(['docker', 'network', 'rm', net], capture_output=True)
        (evidence / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
