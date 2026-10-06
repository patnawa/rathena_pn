"""Run real paired vending SQL against a disposable internal MariaDB instance."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
import uuid


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--image', default='9edfb206479e')
    parser.add_argument('--database-image', default='dd9b303aed4f')
    args = parser.parse_args()
    candidate, evidence = args.candidate.resolve(), args.evidence.resolve()
    assert evidence != candidate and candidate not in evidence.parents
    evidence.mkdir(parents=True, exist_ok=False)
    tag = 'pn-market-bank-' + uuid.uuid4().hex[:12]
    db, net = tag + '-db', tag + '-net'
    created = network = False
    report = {'passed': False, 'production_database_accessed': False,
              'limits': ['Includes the exact production pair SQL handler; links prebuilt character objects.',
                         'UBSan instruments the handler and fixture, not all prebuilt objects.',
                         'Does not run the client UI or map purchase preflight.']}

    def sql(query, check=True):
        return subprocess.run(['docker', 'exec', '-i', '-e', 'MYSQL_PWD=market-bank-fixture-only', db,
                               'mariadb', '-uroot', '-N', '--batch', '--raw'],
                              input=query, text=True, capture_output=True, check=check)

    def run(command, log):
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        (evidence / log).write_text(result.stdout + result.stderr)
        result.check_returncode()
        return result.stdout

    try:
        paths = list((candidate / 'src/char/obj').glob('*.o'))
        assert paths, 'Build candidate char objects first'
        paths += [candidate / p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                  '3rdparty/rapidyaml/obj/ryml.a', 'src/custom/pair_sql.inc', 'src/custom/pair_commit.hpp',
                  'tools/ci/market_bank_sql_runtime.cpp', 'tools/ci/market_bank_sql_test.py', 'sql-files/main.sql')]
        report['input_sha256'] = {str(p.relative_to(candidate)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
        subprocess.run(['docker', 'network', 'create', '--internal', net], capture_output=True, check=True)
        network = True
        subprocess.run(['docker', 'run', '-d', '--name', db, '--network', net, '--network-alias', 'market-bank-db',
                        '--memory', '768m', '--cpus', '1', '-e', 'MARIADB_ROOT_PASSWORD=market-bank-fixture-only',
                        args.database_image], capture_output=True, check=True)
        created = True
        for _ in range(100):
            if sql('SELECT 1', False).returncode == 0:
                break
            time.sleep(.25)
        else:
            raise RuntimeError('Disposable MariaDB did not become ready')
        sql('CREATE DATABASE market_bank_probe; USE market_bank_probe;\n' + (candidate / 'sql-files/main.sql').read_text())
        compiler = ['g++', '-std=c++17', '-g', '-O1', '-fsanitize=undefined', '-fno-sanitize-recover=all', '-DPACKETVER=20260219']
        compiler += ['-I/rathena/' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src', '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
        compiler += ['-I/usr/include/mysql', '/rathena/tools/ci/market_bank_sql_runtime.cpp']
        compiler += ['/rathena/' + str(p.relative_to(candidate)) for p in sorted((candidate / 'src/char/obj').glob('*.o'))]
        compiler += ['/rathena/' + p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a')]
        compiler += ['-Wl,--wrap=main', '-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm', '-o', '/evidence/market-bank-probe']
        mounts = ['--mount', 'type=bind,src=' + str(candidate) + ',dst=/rathena,readonly',
                  '--mount', 'type=bind,src=' + str(evidence) + ',dst=/evidence']
        run(['docker', 'run', '--rm', '--network', 'none', '--memory', '2g', '--cpus', '2'] + mounts +
            ['--entrypoint', compiler[0], args.image] + compiler[1:], 'build.log')
        output = run(['docker', 'run', '--rm', '--network', net, '--memory', '1g', '--cpus', '1'] + mounts +
                     ['--entrypoint', '/evidence/market-bank-probe', args.image], 'runtime.log')
        assert 'MARKET_BANK_SQL_PASS 42 checks' in output
        assert report['input_sha256'] == {str(p.relative_to(candidate)): hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
        report['output'] = output
        report['passed'] = True
        print(output)
    finally:
        if created:
            subprocess.run(['docker', 'rm', '-f', db], capture_output=True)
        if network:
            subprocess.run(['docker', 'network', 'rm', net], capture_output=True)
        (evidence / 'report.json').write_text(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
