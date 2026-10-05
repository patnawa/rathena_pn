"""Start all fresh binaries with disposable SQL and a private internal network."""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import os
import re
import shutil
import subprocess
import time

ROOT = Path(os.environ['GAMEPLAY_CANDIDATE']).resolve()
assert str(ROOT).startswith('/app/rathena-builds/')
OUT = Path('/app/pn-muhro-zeny-20260928/gameplay-startup-' + datetime.now(timezone.utc).strftime('%H%M%S'))
NET = 'pn-muhro-gameplay-20260928'
DB, GAME = NET + '-db', NET + '-game'
CORE = 'sha256:cae30d441d1c6e8f784be1d5525ea0573d9cd680dd21baf077e85908ccfe531b'
DBIMAGE = 'sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979'
os.umask(0o077)

def run(*args, **kwargs):
    return subprocess.run(args, capture_output=True, check=kwargs.pop('check', True),
                          timeout=kwargs.pop('timeout', 120), **kwargs)

def sql(data, check=True):
    return run('docker', 'exec', '-e', 'MYSQL_PWD=project-validation-only', '-i', DB,
               'mariadb', '-uroot', 'ragnarok_ci', input=data, check=check)

def digest(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()

for name in (DB, GAME):
    if run('docker', 'container', 'inspect', name, check=False).returncode == 0:
        raise RuntimeError('Disposable container already exists: ' + name)
if run('docker', 'network', 'inspect', NET, check=False).returncode == 0 or OUT.exists():
    raise RuntimeError('Disposable network/evidence already exists')
# Snapshot artifact digests; full compiler log is retained alongside this fixture.
assert (ROOT / 'src/map/skills/acolyte/warpportal.cpp').exists()
for role in ('login', 'char', 'map', 'web'):
    assert (ROOT / (role + '-server')).stat().st_size > 100000
OUT.mkdir(mode=0o700)
report = {'passed': False, 'started_utc': datetime.now(timezone.utc).isoformat(),
          'production_database_accessed': False, 'runtime_image': CORE, 'database_image': DBIMAGE,
          'binary_sha256': {role + '-server': digest(ROOT / (role + '-server'))
                            for role in ('login', 'char', 'map', 'web')},
          'npc_sha256': digest(ROOT / 'npc/custom/fashion_points/FashionPoints.txt')}
created = []
network = False
try:
    config = OUT / 'conf'
    shutil.copytree(Path('/app/rathena/conf'), config)
    imports = config / 'import'
    imports.mkdir(exist_ok=True)
    lines = []
    for prefix in ('login_server', 'ipban_db', 'char_server', 'map_server', 'web_server', 'log_db'):
        lines += [f'{prefix}_ip: {DB}', f'{prefix}_port: 3306', f'{prefix}_id: validation',
                  f'{prefix}_pw: project-validation-only', f'{prefix}_db: ragnarok_ci']
    (imports / 'inter_conf.txt').write_text('\n'.join(lines) + '\n')
    (imports / 'map_conf.txt').write_text('bind_ip: 127.0.0.1\nmap_ip: 127.0.0.1\nchar_ip: 127.0.0.1\nchar_port: 6121\nmap_port: 5121\nuserid: s1\npasswd: p1\n')
    (imports / 'char_conf.txt').write_text('bind_ip: 127.0.0.1\nchar_ip: 127.0.0.1\nlogin_ip: 127.0.0.1\nlogin_port: 6900\nchar_port: 6121\nuserid: s1\npasswd: p1\n')
    (imports / 'login_conf.txt').write_text('bind_ip: 127.0.0.1\nlogin_port: 6900\n')
    (imports / 'web_conf.txt').write_text('bind_ip: 127.0.0.1\nweb_port: 8888\n')
    run('docker', 'network', 'create', '--internal', NET)
    network = True
    run('docker', 'run', '-d', '--name', DB, '--network', NET, '--memory', '768m', '--cpus', '1',
        '-e', 'MARIADB_ROOT_PASSWORD=project-validation-only', '-e', 'MARIADB_DATABASE=ragnarok_ci', DBIMAGE)
    created.append(DB)
    for _ in range(90):
        if sql(b'SELECT 1;', check=False).returncode == 0:
            break
        time.sleep(1)
    else:
        raise RuntimeError('Disposable database did not become ready')
    names = re.findall(r'< (sql-files/[^ ]+\.sql)', (ROOT / 'tools/ci/sql.sh').read_text())
    if len(names) < 15:
        raise RuntimeError('Incomplete SQL schema list')
    for name in names:
        sql((ROOT / name).read_bytes())
    sql((ROOT / 'sql-files/upgrades/upgrade_20260919_reserve_purchase.sql').read_bytes())
    sql(b"CREATE USER 'validation'@'%' IDENTIFIED BY 'project-validation-only'; GRANT ALL ON ragnarok_ci.* TO 'validation'@'%';")
    sql(b"INSERT INTO login(account_id,userid,user_pass,sex,email,group_id) VALUES(99000031,'gameplayfixture','wide-fixture-only','M','fixture@localhost',99); INSERT INTO `char`(char_id,account_id,char_num,name,class,base_level,job_level,str,sex,last_map,last_x,last_y,save_map,save_x,save_y,hp,max_hp,sp,max_sp,zeny) VALUES(99000032,99000031,0,'gameplayfixture',4252,275,50,100,'M','prontera',150,180,'prontera',150,180,10000,10000,1000,1000,1000000); INSERT INTO skill(char_id,id,lv,flag) VALUES(99000032,1,9,3);")
    for town,x,y in [('prontera',150,180),('izlude',128,114),('payon',180,100),('geffen',119,59),('morocc',156,93),('alberta',116,57)]:
        sql(f"INSERT INTO memo(char_id,map,x,y) VALUES(99000032,'{town}',{x},{y});".encode())
    source = Path(os.environ['GAMEPLAY_FIXTURE_FILES'])
    for name in ('pn_gameplay_fixture.txt','muhro_gameplay_live_client.py','bank_live_client.py'):
        shutil.copyfile(source/name,OUT/name)
    (OUT/'scripts_custom.conf').write_bytes((ROOT/'npc/scripts_custom.conf').read_bytes() + b'\nnpc: /evidence/pn_gameplay_fixture.txt\n')
    with (imports/'battle_conf.txt').open('a') as f:f.write('\nskill_log: 1\n')
    report['schema_files'] = len(names)
    boot = '''#!/bin/sh
set -eu
./login-server > /evidence/login.log 2>&1 &
./char-server > /evidence/char.log 2>&1 &
./map-server > /evidence/map.log 2>&1 &
./web-server > /evidence/web.log 2>&1 &
wait
'''
    (OUT / 'boot.sh').write_text(boot)
    run('docker', 'run', '-d', '--name', GAME, '--label', 'pn.wallet64.fixture=true', '--network', NET, '--memory', '3g', '--cpus', '3',
        '--mount', f'type=bind,src={ROOT},dst=/rathena',
        '--mount', f'type=bind,src={config},dst=/rathena/conf,readonly',
        '--mount', f'type=bind,src={OUT},dst=/evidence',
        '--mount', f'type=bind,src={OUT}/scripts_custom.conf,dst=/rathena/npc/scripts_custom.conf,readonly',
        '-w', '/rathena', '--entrypoint', 'sh', CORE, '/evidence/boot.sh')
    created.append(GAME)
    markers = {'login': 'The login-server is ready', 'char': 'The char-server is ready',
               'map': 'Map Server is now online', 'web': 'The web-server is ready'}
    clean = {}
    for _ in range(180):
        for role in markers:
            path = OUT / (role + '.log')
            text = path.read_text(errors='replace') if path.exists() else ''
            clean[role] = re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]', '', text)
        if all(marker in clean[role] for role, marker in markers.items()):
            break
        if run('docker', 'inspect', '--format', '{{.State.Running}}', GAME, text=True).stdout.strip() != 'true':
            raise RuntimeError('Isolated game container exited before readiness')
        time.sleep(1)
    else:
        raise RuntimeError('Fresh login/char/map/web did not complete startup and handshake')
    errors = []
    for role, text in clean.items():
        for line in text.splitlines():
            severity = re.search(r'\[(Info|Status|Notice|Warning|Debug|SQL)\]:', line, re.I)
            if ((severity and severity.group(1).lower() == 'sql') or
                    re.search(r'\[Error\]|\[Fatal[^\]]*\]|\[SQL\]:\s*DB error|Segmentation fault|AddressSanitizer|runtime error:', line, re.I)):
                errors.append(role + ': ' + line)
    report['errors'] = errors
    if errors:
        raise RuntimeError('Startup diagnostics failed: ' + repr(errors[:4]))
    pid=run('docker','inspect','--format','{{.State.Pid}}',GAME,text=True).stdout.strip()
    completed=run('nsenter','-t',pid,'-n','python3',str(OUT/'muhro_gameplay_live_client.py'),'--game',GAME,'--database',DB,text=True,check=False,timeout=240)
    (OUT/'client.log').write_text(completed.stdout+completed.stderr)
    completed.check_returncode()
    report['gameplay_live']=json.loads(completed.stdout)
    report['passed'] = True
    report['handshake_complete'] = True
except BaseException as error:
    report['error'] = str(error)
    raise
finally:
    report['finished_utc'] = datetime.now(timezone.utc).isoformat()
    (OUT / 'report.json').write_text(json.dumps(report, indent=2) + '\n')
    for name in reversed(created):
        run('docker', 'rm', '-f', '-v', name, check=False)
    if network:
        run('docker', 'network', 'rm', NET, check=False)
print(json.dumps(report, indent=2))
