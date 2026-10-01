#!/usr/bin/env python3
"""Run a disposable real-map heartbeat baseline on a Linux Docker host.

This measures idle and bounded NPC/SQL dispatch, not player combat capacity.
No published ports, production mounts, or production database access are used.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import importlib.util
import json
from pathlib import Path
import re
import shutil
import subprocess
import time


def run(*args, **kwargs):
    return subprocess.run(args, capture_output=True, check=kwargs.pop('check', True),
                          timeout=kwargs.pop('timeout', 120), **kwargs)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def module(path, name):
    spec = importlib.util.spec_from_file_location(name, path)
    value = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(value)
    return value


def samples(log):
    clean = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', log)
    return [json.loads(line.split('PN_METRICS ', 1)[1]) for line in clean.splitlines()
            if 'PN_METRICS {' in line]


def summarize(rows):
    # A maximum of per-window percentiles is not an aggregate percentile.
    return {'sample_count': len(rows), 'max_window_timer_p95_ms': max(r['timer_late_p95_ms'] for r in rows),
            'max_window_timer_p99_ms': max(r['timer_late_p99_ms'] for r in rows),
            'max_window_dispatch_p95_ms': max(r['dispatch_p95_ms'] for r in rows),
            'max_window_dispatch_p99_ms': max(r['dispatch_p99_ms'] for r in rows),
            'max_shop_pending': max(r['shop_pending'] for r in rows),
            'max_shop_oldest_ms': max(r['shop_oldest_ms'] for r in rows),
            'shop_started': rows[-1]['shop_started']}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--evidence', type=Path, required=True)
    parser.add_argument('--seconds', type=int, default=1800)
    parser.add_argument('--image', default='pn-improvement-validation:20260929')
    args = parser.parse_args()
    if args.seconds < 1800:
        parser.error('Acceptance baselines require at least 1800 seconds per phase')
    out = args.evidence.resolve()
    out.mkdir(parents=True, exist_ok=False)
    root = out / 'candidate'
    shutil.copytree(args.candidate, root, symlinks=True,
                    ignore=shutil.ignore_patterns('obj', '*.o', '*.a', '__pycache__', '.git'))
    release = module(root / 'tools/ci/release_bundle.py', 'baseline_release')
    health = module(root / 'tools/admin/health_check.py', 'baseline_health')
    initial = release.binding(root)
    net = 'pn-metrics-' + str(time.time_ns())
    db, game = net + '-db', net + '-map'
    dbimage = 'mariadb@sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979'
    report = {'passed': False, 'binding': initial, 'cases': [],
              'scope': 'isolated idle and bounded NPC/SQL dispatch; no player/combat or purchase load',
              'production_accessed': False, 'network': net}
    created = []
    network_created = False
    def save():
        (out / 'report.json').write_text(json.dumps(report, indent=2))
    def sql(data, check=True):
        return run('docker', 'exec', '-e', 'MYSQL_PWD=fixture-only', '-i', db,
                   'mariadb', '-uroot', 'ragnarok_ci', input=data, check=check)
    def logs():
        result = run('docker', 'logs', game)
        return (result.stdout + result.stderr).decode(errors='replace')
    try:
        run('docker', 'network', 'create', '--internal', net)
        network_created = True
        run('docker', 'run', '-d', '--name', db, '--network', net, '--memory', '768m', '--cpus', '1',
            '-e', 'MARIADB_ROOT_PASSWORD=fixture-only', '-e', 'MARIADB_DATABASE=ragnarok_ci', dbimage)
        created.append(db)
        for _ in range(60):
            if sql(b'SELECT 1;', False).returncode == 0:
                break
            time.sleep(1)
        else:
            raise RuntimeError('Disposable database unavailable')
        for name in re.findall(r'< (sql-files/[^ ]+\.sql)', (root / 'tools/ci/sql.sh').read_text()):
            sql((root / name).read_bytes())
        for name in ('upgrade_20260919_reserve_purchase.sql', 'upgrade_20260929_shop_purchase.sql',
                     'upgrade_20260929_pet_entitlements.sql'):
            sql((root / 'sql-files/upgrades' / name).read_bytes())
        sql(b"CREATE USER 'validation'@'%' IDENTIFIED BY 'fixture-only'; GRANT ALL ON ragnarok_ci.* TO 'validation'@'%';")
        sql(b'CREATE TABLE pn_metrics_fixture (phase INT NOT NULL); INSERT INTO pn_metrics_fixture VALUES(0);')
        conf = out / 'conf'
        shutil.copytree(root / 'conf', conf)
        lines = []
        for prefix in ('login_server', 'ipban_db', 'char_server', 'map_server', 'web_server', 'log_db'):
            lines += [f'{prefix}_ip: {db}', f'{prefix}_port: 3306', f'{prefix}_id: validation',
                      f'{prefix}_pw: fixture-only', f'{prefix}_db: ragnarok_ci']
        (conf / 'import/inter_conf.txt').write_text('\n'.join(lines) + '\n')
        (conf / 'import/map_conf.txt').write_text('bind_ip: 127.0.0.1\nmap_ip: 127.0.0.1\nchar_ip: 127.0.0.1\nmap_port: 5121\n')
        fixture = out / 'workload.txt'
        fixture.write_text('''-\tscript\tPNMetricsFixture\t-1,{
OnInit:
 initnpctimer;
 end;
OnTimer1000:
 query_sql("SELECT phase FROM pn_metrics_fixture", .@phase);
 if (.@phase == 1) {
  for (.@i = 0; .@i < 500; ++.@i) .@value = (.@i * 17) % 131;
  query_sql("SELECT SLEEP(0.020)", .@result);
 }
 if (.@phase == 2) query_sql("SELECT SLEEP(5)", .@result);
 setnpctimer 0;
 end;
}
''')
        with (conf / 'import/map_conf.txt').open('a') as stream:
            stream.write('npc: /fixture/workload.txt\n')
        report['fixture_sha256'] = digest(fixture)
        report['configuration_sha256'] = {str(p.relative_to(conf)): digest(p)
                                           for p in sorted(conf.rglob('*')) if p.is_file()}
        report['images'] = {name: run('docker', 'image', 'inspect', '--format', '{{.Id}}', name).stdout.decode().strip()
                            for name in (args.image, dbimage)}
        run('docker', 'run', '-d', '--name', game, '--network', net, '--memory', '2g', '--cpus', '1.5',
            '--mount', f'type=bind,src={root},dst=/rathena',
            '--mount', f'type=bind,src={conf},dst=/rathena/conf,readonly',
            '--mount', f'type=bind,src={out},dst=/fixture,readonly',
            '-w', '/rathena', '--entrypoint', './map-server', args.image)
        created.append(game)
        save()
        for _ in range(180):
            if "Server is 'ready' and listening" in logs():
                break
            time.sleep(1)
        else:
            raise RuntimeError('Map did not become ready')
        clean_startup = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', logs())
        unexpected = [line for line in clean_startup.splitlines()
                      if re.match(r'^\s*\[(?:Error|Fatal[^\]]*|SQL)\]', line)
                      and 'make_connection: connect failed' not in line]
        if unexpected:
            raise RuntimeError('Unexpected startup errors: ' + '\n'.join(unexpected))
        for phase, name in ((0, 'idle-baseline'), (1, 'load-baseline')):
            sql(f'UPDATE pn_metrics_fixture SET phase={phase};'.encode())
            started_utc = int(time.time())
            start = time.monotonic()
            # Include one extra reporting interval to retain 30 complete windows.
            while time.monotonic() - start < args.seconds + 65:
                state = run('docker', 'inspect', '--format', '{{.State.Running}}', game).stdout.strip()
                if state != b'true':
                    raise RuntimeError('Map exited during baseline')
                time.sleep(10)
            elapsed = time.monotonic() - start
            text = logs()
            path = out / (name + '.log')
            path.write_text(text)
            rows = [row for row in samples(text) if row['utc'] >= started_utc + 60]
            assert len(rows) >= 29, 'Insufficient complete metrics windows'
            assert all(row['shop_pending'] == 0 for row in rows)
            report['cases'].append({'name': name, 'status': 'passed', 'duration_seconds': elapsed,
                                    'summary': summarize(rows), 'artifacts': {path.name: digest(path)}})
            save()
        sql(b'UPDATE pn_metrics_fixture SET phase=0;')
        # Actual stopped process, measured by the unmodified production health parser.
        run('docker', 'kill', '--signal', 'STOP', game)
        time.sleep(165)
        stalled = health.metrics_status(logs(), datetime.now(timezone.utc), 4000)
        assert not stalled['passed'] and any('stale' in reason for reason in stalled.get('reasons', []))
        run('docker', 'kill', '--signal', 'CONT', game)
        time.sleep(70)
        recovered = health.metrics_status(logs(), datetime.now(timezone.utc), 4000)
        assert recovered['passed']
        stallfile = out / 'process-stall.json'
        stallfile.write_text(json.dumps({'stopped_seconds': 165, 'stalled': stalled, 'recovered': recovered}, indent=2))
        report['cases'].append({'name': 'process-stall', 'status': 'passed',
                                'artifacts': {stallfile.name: digest(stallfile)}})
        sql(b'UPDATE pn_metrics_fixture SET phase=2;')
        delayed_start = int(time.time())
        time.sleep(130)
        delayed = [row for row in samples(logs()) if row['utc'] > delayed_start + 60]
        assert delayed and max(row['dispatch_max_ms'] for row in delayed) >= 4900
        report['sql_delay'] = {'query_sleep_seconds': 5, 'summary': summarize(delayed)}
        assert release.binding(root) == initial, 'Candidate mutated during baseline'
        report['passed'] = True
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        if game in created:
            (out / 'complete-map.log').write_text(logs())
        save()
        for name in reversed(created):
            run('docker', 'rm', '-f', '-v', name, check=False)
        if network_created:
            run('docker', 'network', 'rm', net, check=False)


if __name__ == '__main__':
    main()
