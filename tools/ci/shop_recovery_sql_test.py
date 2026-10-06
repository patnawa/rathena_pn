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
from release_bundle import binding
from achievement_persistence_test import function


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--image', default='pn-bank-validation:20260929')
    parser.add_argument('--database-image', default='mariadb:noble')
    parser.add_argument('--mode',choices=['auction-handoff','auction-settlement','lifecycle','lifecycle-return','lifecycle-owner','lifecycle-disabled'])
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
        name = probe + '-pet' if mode.startswith('pet-') else probe
        command += ['-d', '--name', name] if background else ['--rm']
        command += ['--network', net, '--memory', '1g', '--cpus', '1',
                    '--mount', 'type=bind,src=' + str(evidence) + ',dst=/evidence',
                    '--mount', 'type=bind,src=' + str(candidate) + ',dst=/rathena,readonly',
                    '-w', '/rathena', '--entrypoint', ('/evidence/point-login-sql-probe' if mode == 'global-login' else '/evidence/shop-sql-probe'), args.image, mode]
        return command

    def runtime(mode, background=False):
        command = runtime_command(mode, background)
        if background:
            result = run(command, capture_output=True)
            created_containers.append(probe + '-pet' if mode.startswith('pet-') else probe)
            return result
        result = subprocess.run(command, text=True, capture_output=True, timeout=90)
        (evidence / (mode + '.log')).write_text(result.stdout + result.stderr)
        if result.returncode:
            raise RuntimeError(mode + ' failed: ' + (result.stdout + result.stderr)[-5000:])
        return result.stdout

    def input_hashes():
        # Bind evidence to the linked native objects and headers, not only the
        # extracted SQL body. A clean candidate build establishes their freshness.
        paths = {candidate / 'tools/ci/shop_recovery_sql_test.py',
                 candidate / 'tools/ci/pet_entitlement_sql_cases.inc',
                 candidate / 'tools/ci/pet_mail_sql_cases.inc',
                 candidate / 'tools/ci/mail_lifecycle_sql_cases.inc',
                 candidate / 'tools/ci/auction_settlement_sql_cases.inc',
                 candidate / 'tools/ci/auction_handoff_sql_cases.inc',
                 candidate / 'tools/ci/pet_floor_sql_cases.inc',
                 candidate / 'tools/ci/point_asset_sql_cases.inc',
                 candidate / 'tools/ci/point_global_sql_cases.inc',
                 candidate / 'tools/ci/shop_progression_sql_cases.inc',
                 candidate / 'tools/ci/point_login_sql_runtime.cpp',
                 candidate / 'sql-files/upgrades/upgrade_20260929_point_assets.sql',
                 candidate / 'sql-files/upgrades/upgrade_20260929_pet_entitlements.sql',
                 candidate / 'sql-files/upgrades/upgrade_20261001_achievement_atomicity.sql',
                 candidate / 'tools/ci/shop_recovery_sql_runtime.cpp',
                 candidate / 'tools/ci/player_tools_test.py',
                 candidate / 'tools/ci/player_tools_test.cpp',
                 candidate / 'sql-files/upgrades/upgrade_20261002_purchase_history.sql',
                 candidate / 'sql-files/main.sql',
                 candidate / 'sql-files/upgrades/upgrade_20260929_shop_purchase.sql'}
        paths.update(p for p in (candidate / 'src').rglob('*') if p.is_file()
                     and (p.suffix in ('.cpp', '.hpp', '.h', '.inc')
                          or (p.suffix == '.o' and p.parent in (candidate / 'src/char/obj', candidate / 'src/login/obj'))))
        paths.update(candidate / p for p in ('src/common/obj/common.a',
                     '3rdparty/libconfig/obj/libconfig.a', '3rdparty/rapidyaml/obj/ryml.a'))
        if not list((candidate / 'src/char/obj').glob('*.o')) or not list((candidate / 'src/login/obj').glob('*.o')):
            raise RuntimeError('Build current character-server and login-server objects before SQL proof')
        return {p.relative_to(candidate).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                for p in sorted(paths)}

    try:
        report['input_sha256'] = input_hashes()
        report['binding'] = binding(candidate)
        report['images'] = {}
        for kind, name in (('toolchain', args.image), ('database', args.database_image)):
            result = run(['docker', 'image', 'inspect', '--format', '{{.Id}}', name],
                         capture_output=True, text=True)
            report['images'][kind] = {'reference': name, 'id': result.stdout.strip()}
        # Never remove or reuse a resource not created by this invocation.
        for name, kind in ((db, 'container'), (probe, 'container'), (probe + '-pet', 'container'), (net, 'network')):
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
        sql('USE shop_recovery_probe;\n' + (candidate / 'sql-files/upgrades/upgrade_20260929_pet_entitlements.sql').read_text())
        sql('USE shop_recovery_probe;\n' + (candidate / 'sql-files/upgrades/upgrade_20260929_point_assets.sql').read_text())
        sql('USE shop_recovery_probe;\n' + (candidate / 'sql-files/upgrades/upgrade_20261001_achievement_atomicity.sql').read_text())
        sql('USE shop_recovery_probe; DROP TABLE pn_purchase_history;\n' + (candidate / 'sql-files/upgrades/upgrade_20261002_purchase_history.sql').read_text())
        sql('USE shop_recovery_probe;\n' + (candidate / 'sql-files/upgrades/upgrade_20261002_purchase_history.sql').read_text())
        compiler = ['g++', '-std=c++17', '-g', '-O1', '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    '-DPACKETVER=20260219']
        compiler += ['-I/rathena/' + p for p in ('src', '3rdparty/libconfig', '3rdparty/rapidyaml/src',
                                                '3rdparty/rapidyaml/ext/c4core/src', '3rdparty/json/include')]
        (evidence / 'shop-admission.inc').write_text(function(candidate / 'src/custom/shop_map.inc', 'bool pn_shop_stock_rebase('))
        compiler += ['-I/usr/include/mysql', '-I/evidence', '/rathena/tools/ci/shop_recovery_sql_runtime.cpp']
        compiler += ['/rathena/' + str(p.relative_to(candidate)) for p in sorted((candidate / 'src/char/obj').glob('*.o'))]
        compiler += ['/rathena/' + p for p in ('src/common/obj/common.a', '3rdparty/libconfig/obj/libconfig.a',
                                              '3rdparty/rapidyaml/obj/ryml.a')]
        compiler += ['-Wl,--wrap=main', '-lz', '-ldl', '-lmysqlclient', '-l:libzstd.so.1', '-lssl', '-lcrypto',
                     '-lresolv', '-lm', '-o', '/evidence/shop-sql-probe']
        def build_probe(compiler, name):
            built = subprocess.run(['docker', 'run', '--rm', '--network', 'none', '--memory', '2g', '--cpus', '2',
                                    '--mount', 'type=bind,src=' + str(candidate) + ',dst=/rathena,readonly',
                                    '--mount', 'type=bind,src=' + str(evidence) + ',dst=/evidence',
                                    '--entrypoint', compiler[0], args.image] + compiler[1:],
                                   text=True, capture_output=True, timeout=180)
            (evidence / (name + '-build.log')).write_text(built.stdout + built.stderr)
            if built.returncode:
                raise RuntimeError('Compile failed: ' + built.stderr[-5000:])
        build_probe(compiler, 'char')
        if args.mode:
            report['mail_lifecycle']=runtime(args.mode).strip()
            report['passed']=True
            print(report['mail_lifecycle'])
            return
        login_compiler = [arg for arg in compiler if '/src/char/obj/' not in arg]
        login_compiler = [arg.replace('shop_recovery_sql_runtime.cpp', 'point_login_sql_runtime.cpp').replace('/evidence/shop-sql-probe', '/evidence/point-login-sql-probe') for arg in login_compiler]
        archive = login_compiler.index('/rathena/src/common/obj/common.a')
        login_compiler[archive:archive] = ['/rathena/' + str(p.relative_to(candidate)) for p in sorted((candidate / 'src/login/obj').glob('*.o'))]
        build_probe(login_compiler, 'login')
        output = runtime('normal')
        assert 'SHOP_SQL_PASS' in output, output
        report['runtime'] = output.strip()
        report['queued_admission'] = runtime('queued').strip()
        player = subprocess.run(['docker','run','--rm','--network',net,'--memory','2g','--cpus','2',
            '--mount','type=bind,src='+str(candidate)+',dst=/rathena,readonly',
            '--mount','type=bind,src='+str(evidence)+',dst=/evidence','-w','/rathena',
            '--entrypoint','python3',args.image,'tools/ci/player_tools_test.py','--sql','--evidence','/evidence'],
            text=True,capture_output=True,timeout=180)
        (evidence/'player-tools-run.log').write_text(player.stdout+player.stderr)
        if player.returncode:raise RuntimeError('Player tools SQL failed: '+(player.stdout+player.stderr)[-5000:])
        report['player_tools_sql']=player.stdout.strip()
        report['character_wire_handler'] = runtime('wire').strip()
        progression_output = runtime('progression')
        assert 'SHOP_PROGRESSION_SQL_PASS' in progression_output, progression_output
        report['progression_atomicity'] = progression_output.strip()
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
        report['pet_entitlement_core'] = runtime('pet-core').strip()
        report['pet_asset_commit'] = runtime('pet-assets').strip()
        report['point_asset_commit'] = runtime('point-assets').strip()
        report['point_global_barrier'] = runtime('global-points').strip()
        runtime('global-seed-pending')
        report['point_login_adapter'] = runtime('global-login').strip()
        global_restarts = []
        for phase in ('pending', 'approved', 'committed'):
            runtime('global-seed-' + phase)
            run(['docker', 'restart', db], capture_output=True, timeout=30)
            ready()
            global_restarts.append(runtime('global-resolve-' + phase).strip())
        report['point_global_restart'] = '\n'.join(global_restarts)
        report['pet_retirement'] = runtime('pet-retirement').strip()
        report['pet_mail_asset'] = runtime('pet-mail').strip()
        report['mail_lifecycle'] = runtime('lifecycle').strip()
        report['auction_settlement'] = runtime('auction-settlement').strip()
        report['auction_handoff'] = runtime('auction-handoff').strip()
        run(['docker','restart',db],capture_output=True,timeout=30)
        ready()
        report['auction_handoff_restart'] = runtime('auction-handoff-replay').strip()
        runtime('lifecycle-race-seed')
        returning=[subprocess.Popen(runtime_command('lifecycle-race-return'),text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT) for _ in range(2)]
        for index,process in enumerate(returning):
            output,_=process.communicate(timeout=60)
            (evidence/('mail-race-'+str(index)+'.log')).write_text(output)
            assert process.returncode==0,output
        report['mail_return_race']=runtime('lifecycle-race-verify').strip()
        report['pet_floor_asset'] = runtime('floor-pets').strip()
        runtime('floor-pet-seed')
        run(['docker', 'restart', db], capture_output=True, timeout=30)
        ready()
        report['pet_floor_restart'] = runtime('floor-pet-restart').strip()
        runtime('pet-seed-restart')
        run(['docker', 'restart', db], capture_output=True, timeout=30)
        ready()
        report['pet_entitlement_restart'] = runtime('pet-verify-restart').strip()
        runtime('pet-seed-race')
        creators = [(mode, subprocess.Popen(runtime_command(mode), text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT))
                    for mode in ('pet-race-a', 'pet-race-b')]
        report['pet_concurrent_creators'] = []
        for mode, process in creators:
            output, _ = process.communicate(timeout=60)
            (evidence / (mode + '.log')).write_text(output)
            assert process.returncode == 0 and 'PET_RACE_PASS' in output, mode + ': ' + output
            report['pet_concurrent_creators'].append(output.strip())
        report['pet_concurrent_result'] = runtime('pet-verify-race').strip()
        runtime('pet-crash', background=True)
        for _ in range(80):
            state = sql("SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE DB='shop_recovery_probe' AND STATE='User sleep'", False)
            if state.returncode == 0 and state.stdout.strip() == '1':
                break
            time.sleep(.25)
        else:
            raise RuntimeError('Did not reach pet/payment pre-commit crash trigger')
        run(['docker', 'kill', '--signal', 'KILL', db], capture_output=True)
        code = run(['docker', 'wait', probe + '-pet'], text=True, capture_output=True, timeout=20).stdout.strip()
        logs = run(['docker', 'logs', probe + '-pet'], text=True, capture_output=True)
        (evidence / 'pet-crash.log').write_text(logs.stdout + logs.stderr)
        assert code == '0' and 'PET_CRASH_RETURN 0' in logs.stdout, logs.stdout + logs.stderr
        run(['docker', 'start', db], capture_output=True)
        ready()
        report['pet_uncommitted_restart'] = runtime('pet-verify-rollback').strip()
        if input_hashes() != report['input_sha256']:
            raise RuntimeError('Candidate source or native objects changed during SQL proof')
        if binding(candidate) != report['binding']:
            raise RuntimeError('Release inputs changed during SQL proof')
        report['artifacts'] = {p.relative_to(evidence).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in sorted(evidence.glob('*.log'))}
        report['passed'] = True
        print(json.dumps({k: v for k, v in report.items() if k != 'input_sha256'}, indent=2))
    finally:
        for name in reversed(created_containers):
            subprocess.run(['docker', 'rm', '-f', '-v', name], capture_output=True)
        if network_created:
            subprocess.run(['docker', 'network', 'rm', net], capture_output=True)
        (evidence / 'report.json').write_text(json.dumps(report, indent=2) + '\n')


if __name__ == '__main__':
    main()
