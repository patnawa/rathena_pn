"""Real SQL and fresh-process market restore, with explicit NPC catalog doubles.

Links production common Sql/SqlStmt; extracts unchanged market handlers from npc.cpp.
Never starts map-server or accesses configured/production databases.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import time
import uuid
from release_bundle import binding


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--image', default='pn-improvement-validation:20260929')
    args = parser.parse_args()
    root, out = args.candidate.resolve(), args.evidence.resolve()
    if out == root or root in out.parents:
        raise ValueError('Evidence must be outside candidate')
    out.mkdir(parents=True, exist_ok=False)
    tag = 'pn-market-' + uuid.uuid4().hex[:12]
    db, net = tag + '-db', tag + '-net'
    dbimage = 'mariadb@sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979'
    report = {'passed': False, 'production_database_accessed': False, 'binding': binding(root), 'cases': [],
              'limits': ['NPC/item metadata and DBMap are explicit doubles.',
                         'Fresh probe processes exercise production SQL loading/restoration, not a full map restart.',
                         'Fence test uses real concurrent SQL transaction and production SQL writer guards; does not execute a player purchase.']}
    created_db = created_net = False
    logs = []
    def run(command, **kw):
        return subprocess.run(command, capture_output=True, check=kw.pop('check', True),
                              timeout=kw.pop('timeout', 90), **kw)
    def sql(query, check=True):
        return run(['docker', 'exec', '-i', '-e', 'MYSQL_PWD=market-fixture-only', db,
                    'mariadb', '-uroot', '--batch', '--raw'], input=query.encode(), check=check)
    def ready():
        for _ in range(90):
            if sql('SELECT 1', False).returncode == 0:
                return
            time.sleep(.25)
        raise RuntimeError('Fixture SQL startup timed out')
    def execute(*arguments):
        p = run(['docker', 'run', '--rm', '--network', net, '--memory', '256m', '--cpus', '.5',
                 '--mount', f'type=bind,src={out},dst=/evidence', '--entrypoint', '/evidence/market-probe',
                 args.image, *map(str, arguments)], timeout=30)
        text = (p.stdout + p.stderr).decode()
        logs.append(text)
        if 'MARKET_SQL_' not in text:
            raise RuntimeError('Missing probe success marker')
    try:
        source = (root / 'src/map/npc.cpp').read_text()
        def function(signature, content=source):
            start = content.index(signature)
            opening = content.index('{', start)
            depth = 1
            end = opening + 1
            while depth:
                depth += (content[end] == '{') - (content[end] == '}')
                end += 1
            return content[start:end]
        inter = (root / 'src/custom/shop_inter.inc').read_text()
        fence = inter[inter.index('static bool pn_shop_inflight'):inter.index('\n\nstatic void pn_shop_send')]
        body = fence + '\n' + '\n'.join(function(sig) for sig in (
            'void npc_market_tosql(', 'void npc_market_delfromsql_(',
            'static int32 npc_market_checkall_sub(', 'static void npc_market_fromsql(void) {'))
        body += '\n' + function('bool pn_shop_stock_refresh(', (root / 'src/custom/shop_map.inc').read_text())
        (out / 'market_sql_body.inc').write_text(body)
        files = ['src/map/npc.cpp', 'src/custom/shop_inter.inc', 'src/custom/shop_map.inc', 'src/common/obj/common.a',
                 'tools/ci/market_restore_sql_runtime.cpp', 'tools/ci/market_restore_sql_test.py', 'sql-files/main.sql']
        report['input_sha256'] = {name: hashlib.sha256((root / name).read_bytes()).hexdigest() for name in files}
        compile_cmd = ['g++', '-std=c++17', '-O1', '-g', '-DPACKETVER=20260219', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                       '-I/rathena/src', '-I/usr/include/mysql', '-I/evidence',
                       '/rathena/tools/ci/market_restore_sql_runtime.cpp', '/rathena/src/common/obj/common.a',
                       '/rathena/3rdparty/libconfig/obj/libconfig.a', '/rathena/3rdparty/rapidyaml/obj/ryml.a',
                       '-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto', '-lresolv', '-lm',
                       '-o', '/evidence/market-probe']
        compiled = run(['docker', 'run', '--rm', '--network', 'none', '--memory', '512m', '--cpus', '1',
                        '--mount', f'type=bind,src={root},dst=/rathena,readonly',
                        '--mount', f'type=bind,src={out},dst=/evidence', '--entrypoint', compile_cmd[0],
                        args.image, *compile_cmd[1:]], check=False)
        (out / 'build.log').write_bytes(compiled.stdout + compiled.stderr)
        if compiled.returncode:
            raise RuntimeError(compiled.stderr.decode()[-4000:])
        report['images'] = {image: run(['docker', 'image', 'inspect', '--format', '{{.Id}}', image]).stdout.decode().strip()
                            for image in (args.image, dbimage)}
        run(['docker', 'network', 'create', '--internal', net]); created_net = True
        run(['docker', 'run', '-d', '--name', db, '--network', net, '--network-alias', 'market-db',
             '--memory', '512m', '--cpus', '.5', '-e', 'MARIADB_ROOT_PASSWORD=market-fixture-only', dbimage]); created_db = True
        ready()
        schema = re.search(r'CREATE TABLE IF NOT EXISTS `market`[\s\S]*?;', (root / 'sql-files/main.sql').read_text()).group()
        sql('CREATE DATABASE market_probe; USE market_probe;\n' + schema + '\nALTER TABLE market ENGINE=InnoDB;')
        for stock in (0, 1, 37, 2147483647, -1):
            for poison in (0, 127, 128, 255):
                sql(f"USE market_probe;DELETE FROM market;INSERT INTO market(name,nameid,price,amount,flag) VALUES('market',501,100,9,0),('market',502,123,{stock},1);")
                execute('restore', stock, poison, 7)
                execute('restore', stock, poison, 7)  # Fresh process, retained SQL.
            run(['docker', 'restart', db]); ready()
            execute('restore', stock, 128, 7)  # Database process restart as well.
        for existing in (-1, 0, 8):
            sql("USE market_probe;DELETE FROM market;INSERT INTO market(name,nameid,price,amount,flag) VALUES('market',501,100,9,0),('market',502,123,37,1);")
            execute('restore', 37, 128, existing)
        sql("USE market_probe;DELETE FROM market;INSERT INTO market(name,nameid,price,amount,flag) VALUES('market',501,100,9,0),('market',502,123,7,1);")
        execute('fence')
        assert binding(root) == report['binding'], 'Candidate changed during proof'
        (out / 'runtime.log').write_text('\n'.join(logs))
        artifacts = {name: hashlib.sha256((out / name).read_bytes()).hexdigest()
                     for name in ('runtime.log', 'build.log', 'market_sql_body.inc')}
        report.update(passed=True, probe_processes=len(logs), database_restarts=5)
        report['cases'] = [{'name': 'market-restore-restart', 'status': 'passed', 'artifacts': artifacts}]
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        (out / 'report.json').write_text(json.dumps(report, indent=2))
        if created_db:
            run(['docker', 'rm', '-f', '-v', db], check=False)
        if created_net:
            run(['docker', 'network', 'rm', net], check=False)


if __name__ == '__main__':
    main()
