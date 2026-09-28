"""Pinned offline server deployment; prepare is read-only, apply is explicit.

Input selection JSON: {"files": ["map-server", ...], "validation_reports": ["/absolute/report.json", ...]}.
Run --prepare SELECTION --plan NEW_PLAN, inspect the generated plan, then
--apply PLAN --expected-sha256 SHA. Client publication is a separate signed release.
"""
import argparse
import gzip
import hashlib
import importlib.util
import json
import os
from pathlib import Path, PurePosixPath
import re
import shutil
import subprocess
import time
from datetime import datetime, timezone

LIVE=Path('/app/rathena')
CANDIDATE=Path('/app/rathena-builds/muhro-mail-20260928')
EVIDENCE=Path('/app/pn-muhro-zeny-20260928')
DB='rathena-db'
WRITERS=['rathena-map','rathena-char','rathena-login','rathena-web','rathena-fluxcp']
BINARIES=[role+'-server' for role in ('login','char','map','web')]
MIGRATIONS=['upgrade_20260928_wide_zeny.sql','2026-09-28-paired-economy.sql','upgrade_20260928_bank_sweep.sql','upgrade_20260928_mail_companion.sql']
os.umask(0o077)

def run(args,**kwargs):
    return subprocess.run(args,check=True,capture_output=True,timeout=kwargs.pop('timeout',120),**kwargs)

def sha(path):
    with Path(path).open('rb') as f:return hashlib.file_digest(f,'sha256').hexdigest()

def write_new(path,value):
    with Path(path).open('x',encoding='utf-8') as f:
        json.dump(value,f,indent=2,sort_keys=True);f.write('\n');f.flush();os.fsync(f.fileno())

def safe(root,relative):
    name=PurePosixPath(relative)
    if name.is_absolute() or not name.parts or any(x in ('..','.') for x in name.parts):raise ValueError('Invalid relative deployment path')
    path=root.joinpath(*name.parts)
    if path.resolve()!=path or not path.is_relative_to(root):raise ValueError('Symlink or escaping deployment path')
    return path

def containers(names):return json.loads(run(['docker','inspect',*names]).stdout)

def offline():
    if any(c['State']['Running'] for c in containers(WRITERS)):raise RuntimeError('An application writer is running')

def stop_game_writer(name,binary,out):
    before=containers([name])[0]
    if not before['State']['Running']:
        if before['State'].get('OOMKilled') or before['State']['ExitCode'] not in (0,143):raise RuntimeError('Previous shutdown was not graceful: '+name)
        return {'already_stopped':True,'exit_code':before['State']['ExitCode']}
    pids=run(['docker','exec',name,'pidof',binary],text=True).stdout.split()
    if len(pids)!=1 or not pids[0].isdigit():raise RuntimeError('Expected exactly one '+binary+' process')
    since=datetime.now(timezone.utc).isoformat()
    policy=before['HostConfig']['RestartPolicy']
    write_new(out/(name+'-restart-policy.json'),policy)
    # Signal the server before touching PID 1. A wrapper shell may exit on TERM
    # without forwarding it; Docker would then tear down the saving child.
    # Disable restart first and retain the exact policy for startup coordination.
    run(['docker','update','--restart=no',name])
    run(['docker','exec',name,'kill','-TERM',pids[0]])
    for attempt in range(60):
        if not containers([name])[0]['State']['Running']:break
        time.sleep(1)
    else:raise RuntimeError('Graceful server exit timed out; no forced kill issued: '+name)
    state=containers([name])[0]['State']
    # Docker may emit stderr log records separately from stdout.
    logs=run(['docker','logs','--since',since,name])
    raw=(logs.stdout+logs.stderr).decode(errors='replace');(out/(name+'-shutdown.log')).write_text(raw)
    clean=re.sub(r'\x1b\[[0-?]*[ -/]*[@-~]','',raw)
    if state['Running'] or state.get('OOMKilled') or state['ExitCode'] not in (0,143):raise RuntimeError('Non-graceful shutdown: '+name)
    if 'Terminating...' not in clean:raise RuntimeError('No finalization evidence: '+name)
    if binary=='map-server' and 'Cleaned up' not in clean:raise RuntimeError('Map final-save/cleanup path did not finish')
    if binary=='char-server' and 'Finished.' not in clean:raise RuntimeError('Character finalization did not finish')
    if re.search(r'\[Error\]|\[Fatal[^\]]*\]|DB error|cannot be saved|can.t be saved',clean,re.I):raise RuntimeError('Shutdown diagnostics failed: '+name)
    return {'exit_code':state['ExitCode'],'oom_killed':state.get('OOMKilled',False),'log_sha256':sha(out/(name+'-shutdown.log')),'restart_policy_before':policy,'restart_policy_now':'no'}

def database_info():
    info=containers([DB])[0]
    values=dict(x.split('=',1) for x in info['Config']['Env'] if '=' in x)
    password=values.get('MARIADB_ROOT_PASSWORD',values.get('MYSQL_ROOT_PASSWORD'))
    if not password or not info['State']['Running']:raise RuntimeError('Database unavailable')
    return info,password

def sql(container,password,database,data):
    return run(['docker','exec','-e','MYSQL_PWD='+password,'-i',container,'mariadb','-uroot','--batch','--raw','-N',database],input=data)

def dump(container,password,database,target):
    args=['docker','exec','-e','MYSQL_PWD='+password,container,'mariadb-dump','-uroot','--single-transaction','--routines','--events','--triggers','--hex-blob','--skip-comments','--skip-dump-date','--skip-extended-insert','--order-by-primary','--databases',database]
    with Path(target).open('xb') as f:
        result=subprocess.run(args,stdout=f,stderr=subprocess.PIPE,timeout=600)
        f.flush();os.fsync(f.fileno())
    if result.returncode:raise RuntimeError('Database dump failed: '+result.stderr.decode(errors='replace'))
    if Path(target).stat().st_size<100:raise RuntimeError('Empty database backup')

def verify_backup(database,image,backup,out):
    name='pn-wide-restore-'+datetime.now(timezone.utc).strftime('%Y%m%d%H%M%S')
    password='isolated-backup-proof-only'
    created=False;network=False
    try:
        run(['docker','network','create','--internal',name]);network=True
        run(['docker','run','-d','--name',name,'--label','pn.wallet64.backup-proof=true','--network',name,'--memory','1g','--cpus','1','-e','MARIADB_ROOT_PASSWORD='+password,image]);created=True
        for attempt in range(90):
            try:sql(name,password,'mysql',b'SELECT 1;');break
            except subprocess.CalledProcessError:time.sleep(1)
        else:raise RuntimeError('Backup proof database did not start')
        with backup.open('rb') as f:
            result=subprocess.run(['docker','exec','-e','MYSQL_PWD='+password,'-i',name,'mariadb','-uroot'],stdin=f,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=600)
        if result.returncode:raise RuntimeError('Backup restore verification failed: '+result.stderr.decode(errors='replace'))
        restored=out/'restored-canonical.sql';dump(name,password,database,restored)
        if sha(restored)!=sha(backup):raise RuntimeError('Restored canonical database differs from backup')
        write_new(out/'backup-proof.json',{'passed':True,'database_image':image,'backup_sha256':sha(backup),'restored_sha256':sha(restored)})
    finally:
        if created:subprocess.run(['docker','rm','-f','-v',name],capture_output=True,timeout=120)
        if network:subprocess.run(['docker','network','rm',name],capture_output=True,timeout=120)

def validate(plan,baseline=True):
    if plan['live_root']!=str(LIVE) or plan['candidate_root']!=str(CANDIDATE):raise ValueError('Unexpected release roots')
    if not re.fullmatch(r'[A-Za-z0-9_]+',plan['database']):raise ValueError('Invalid database identifier')
    expected={name:None for name in BINARIES}
    required=set(BINARIES)|{'tools/admin/retire_zeny_tokens.py'}|{'sql-files/upgrades/'+x for x in MIGRATIONS}
    if not required.issubset(plan['files']):raise ValueError('Release omits binaries or economy migrations')
    for name,row in plan['files'].items():
        source=safe(CANDIDATE,name);destination=safe(LIVE,name)
        if not source.is_file() or sha(source)!=row['candidate_sha256']:raise ValueError('Candidate changed: '+name)
        if baseline and (sha(destination) if destination.exists() else None)!=row['live_sha256']:raise ValueError('Live baseline changed: '+name)
        if name in expected:expected[name]=row['candidate_sha256']
    proven=set()
    for entry in plan['reports']:
        path=Path(entry['path'])
        if sha(path)!=entry['sha256']:raise ValueError('Validation evidence changed')
        report=json.loads(path.read_text())
        if report.get('passed') is not True:raise ValueError('Validation did not pass')
        for name,value in report.get('binary_sha256',{}).items():
            if name in expected and value!=expected[name]:raise ValueError('Evidence tests a different binary')
            if name in expected:proven.add(name)
        if 'inputs_unchanged' in report:
            if not report['inputs_unchanged'] or report.get('exit_code')!=0:raise ValueError('Compiler receipt failed')
            for name,value in report['inputs'].items():
                if sha(safe(CANDIDATE,name))!=value:raise ValueError('Compiler source changed: '+name)
    if proven!=set(BINARIES):raise ValueError('Missing binary verification evidence')

def prepare(selection,plan_path,database):
    value=json.loads(Path(selection).read_text())
    names=sorted(set(value['files']))
    plan={'version':1,'database':database,'live_root':str(LIVE),'candidate_root':str(CANDIDATE),'files':{},'reports':[]}
    for name in names:
        source=safe(CANDIDATE,name);destination=safe(LIVE,name)
        plan['files'][name]={'candidate_sha256':sha(source),'live_sha256':sha(destination) if destination.exists() else None,'mode':destination.stat().st_mode&0o777 if destination.exists() else source.stat().st_mode&0o777}
    for path in value['validation_reports']:plan['reports'].append({'path':str(Path(path).resolve()),'sha256':sha(path)})
    validate(plan);write_new(plan_path,plan)
    print(json.dumps({'status':'prepared','plan':plan_path,'sha256':sha(plan_path)}))

def apply(plan_path,expected_hash):
    if sha(plan_path)!=expected_hash:raise ValueError('Reviewed deployment plan hash mismatch')
    plan=json.loads(Path(plan_path).read_text());validate(plan)
    out=EVIDENCE/('release-'+datetime.now(timezone.utc).strftime('%Y%m%dT%H%M%SZ'))
    out.mkdir(mode=0o700);shutil.copy2(plan_path,out/'plan.json')
    status={'status':'started','release':str(out),'plan_sha256':expected_hash}
    try:
        # Preserve exact prior files before stopping players or changing SQL.
        for name,row in plan['files'].items():
            if row['live_sha256'] is None:continue
            target=out/'files-before'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(safe(LIVE,name),target)
            if sha(target)!=row['live_sha256']:raise RuntimeError('Backup hash mismatch: '+name)
        info,password=database_info();database=plan['database']
        status['online_before_stop']=int(sql(DB,password,database,b'SELECT COUNT(*) FROM `char` WHERE online<>0;').stdout.strip())
        write_new(out/'pre-stop.json',{'online_characters':status['online_before_stop'],'containers':[{'name':x['Name'],'running':x['State']['Running']} for x in containers(WRITERS)]})
        # Block fresh logins/web mutations, then keep char alive for map saves.
        run(['docker','stop','--time','60','rathena-web','rathena-fluxcp','rathena-login'],timeout=240)
        status['map_shutdown']=stop_game_writer('rathena-map','map-server',out)
        for attempt in range(30):
            if int(sql(DB,password,database,b'SELECT COUNT(*) FROM `char` WHERE online<>0;').stdout.strip())==0:break
            time.sleep(1)
        else:raise RuntimeError('Character server did not confirm all map sessions offline')
        status['char_shutdown']=stop_game_writer('rathena-char','char-server',out)
        offline();validate(plan)
        backup=out/'database-before.sql';dump(DB,password,database,backup)
        verify_backup(database,info['Image'],backup,out)
        offline();validate(plan)
        for name in MIGRATIONS:sql(DB,password,database,safe(CANDIDATE,'sql-files/upgrades/'+name).read_bytes())
        script=safe(CANDIDATE,'tools/admin/retire_zeny_tokens.py')
        spec=importlib.util.spec_from_file_location('retire_zeny_tokens_release',script);retire=importlib.util.module_from_spec(spec);spec.loader.exec_module(retire)
        import pymysql
        addresses=[n['IPAddress'] for n in info['NetworkSettings']['Networks'].values() if n['IPAddress']]
        if len(addresses)!=1:raise ValueError('Ambiguous database network')
        db=pymysql.connect(host=addresses[0],user='root',password=password,database=database,autocommit=True)
        try:
            conversion=retire.migrate(db);write_new(out/'token-plan.json',conversion)
            receipt=retire.migrate(db,apply=True,expected_hash=conversion['sha256'],assert_offline=offline);write_new(out/'token-result.json',receipt)
            after=retire.migrate(db)
            if after['rows'] or after['listings']:raise RuntimeError('Legacy token rows remain')
            for account in conversion['accounts']:
                rows=retire.query(db,"SELECT value FROM acc_reg_num WHERE account_id=%s AND `key`='#BANKVAULT' AND `index`=0",(account['account'],))
                if rows!=((account['after'],),):raise RuntimeError('Converted account bank mismatch')
        finally:db.close()
        offline()
        for name,row in plan['files'].items():
            source=safe(CANDIDATE,name);target=safe(LIVE,name);target.parent.mkdir(parents=True,exist_ok=True)
            temporary=target.with_name(target.name+'.wide-stage')
            if temporary.exists():raise ValueError('Staging file already exists')
            shutil.copyfile(source,temporary);os.chmod(temporary,row['mode'])
            if sha(temporary)!=row['candidate_sha256']:raise RuntimeError('Install hash mismatch')
            os.replace(temporary,target)
        status['status']='installed_offline'
        # Return control to release coordination for client publication and
        # startup checks. No old binary or financial backup is auto-restored.
        status['writers_stopped']=True
    except BaseException as error:
        status.update(status='failed',error=type(error).__name__,message=str(error))
        raise
    finally:write_new(out/'release.json',status)
    print(json.dumps(status))

def main():
    p=argparse.ArgumentParser(description=__doc__);group=p.add_mutually_exclusive_group(required=True)
    group.add_argument('--prepare');group.add_argument('--apply');p.add_argument('--plan');p.add_argument('--database');p.add_argument('--expected-sha256')
    args=p.parse_args()
    if args.prepare:
        if not args.plan or not args.database:p.error('--prepare requires --plan and --database')
        prepare(args.prepare,args.plan,args.database)
    else:
        if not args.expected_sha256:p.error('--apply requires --expected-sha256')
        apply(args.apply,args.expected_sha256)
if __name__=='__main__':main()
