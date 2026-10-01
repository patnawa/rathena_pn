"""Fresh native achievement writer, SQL faults and database-crash rollback.

Docker-host runner; creates its own internal network and disposable database.
Never reads server connection settings or connects to production SQL.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import time
import uuid


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--candidate',type=Path,default=Path(__file__).resolve().parents[2])
    parser.add_argument('--evidence',type=Path,required=True)
    parser.add_argument('--image',default='sha256:9edfb206479e0766e833cfcdeb7ce06ad447d318fa77b7dfae084face2f59028')
    parser.add_argument('--database-image',default='mariadb@sha256:dd9b303aed4f4890ed09f766d8ca9ddfd176c0c6f6267feff53b3192ec65a979')
    args=parser.parse_args();root=args.candidate.resolve();out=args.evidence.resolve()
    if out==root or root in out.parents:raise ValueError('Evidence must be outside candidate')
    out.mkdir(parents=True,exist_ok=False)
    tag='pn-achievement-proof-'+uuid.uuid4().hex[:12];net=tag+'-net';db=tag+'-db';probe=tag+'-probe'
    created=[];network=False;report={'passed':False,'production_database_accessed':False,'checks':[]}
    def run(*command,**kwargs):return subprocess.run(command,check=kwargs.pop('check',True),capture_output=True,text=True,timeout=kwargs.pop('timeout',120),**kwargs)
    def sql(query,check=True):return run('docker','exec','-i','-e','MYSQL_PWD=achievement-fixture-only',db,'mariadb','-uroot','--batch','--raw','-N',input=query,check=check)
    def ready():
        for _ in range(100):
            if sql('SELECT 1',False).returncode==0:return
            time.sleep(.25)
        raise RuntimeError('Fixture database not ready')
    def container(entry,commands,network_name='none',detach=False,name=None):
        flags=['docker','run']
        flags+=['-d','--name',name or probe] if detach else ['--rm']
        flags+=['--network',network_name,'--memory','2g','--cpus','2',
            '--mount',f'type=bind,src={root},dst=/rathena,readonly',
            '--mount',f'type=bind,src={out},dst=/evidence','-w','/rathena','--entrypoint',entry,args.image]
        return run(*(flags+commands),timeout=300)
    def runtime(mode):
        result=container('/evidence/achievement-sql',[mode],net)
        (out/(mode+'.log')).write_text(result.stdout+result.stderr)
        if 'ACHIEVEMENT_SQL_OK' not in result.stdout:raise RuntimeError('Missing native success marker')
        report['checks'].append(mode)
    try:
        paths=[root/'src/char/int_achievement.cpp',root/'src/char/int_achievement.hpp',root/'src/char/int_mail.cpp',root/'src/char/int_mail.hpp',Path(__file__).resolve(),root/'tools/ci/achievement_sql_runtime.cpp',root/'sql-files/main.sql',root/'sql-files/upgrades/upgrade_20261001_achievement_atomicity.sql']
        paths+=sorted((root/'src/char/obj').glob('*.o'))
        paths+=sorted(p for p in (root/'src').rglob('*') if p.is_file() and p.suffix in ('.h','.hpp','.inc'))
        paths+=[root/p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        def hashes():return {str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths}
        report['inputs_sha256']=hashes()
        report['images']={kind:run('docker','image','inspect','--format','{{.Id}}',name).stdout.strip() for kind,name in [('toolchain',args.image),('database',args.database_image)]}
        # rAthena's legacy packet macros intentionally access packed fields;
        # exercise all other UBSan checks while excluding alignment noise.
        flags=['-std=c++17','-O1','-g','-fsanitize=undefined','-fno-sanitize=alignment','-fno-sanitize-recover=all','-DPACKETVER=20260219']
        flags+=['-I/rathena/'+p for p in ('src','3rdparty/libconfig','3rdparty/rapidyaml/src','3rdparty/rapidyaml/ext/c4core/src','3rdparty/json/include')]+['-I/usr/include/mysql']
        compile_logs=[]
        for source,name in [('src/char/int_achievement.cpp','int_achievement'),('src/char/int_mail.cpp','int_mail')]:
            result=container('g++',flags+['-c',source,'-o',f'/evidence/{name}.o'])
            compile_logs.append(result.stdout+result.stderr)
        (out/'compile.log').write_text(''.join(compile_logs))
        objects=['/rathena/'+p.relative_to(root).as_posix() for p in sorted((root/'src/char/obj').glob('*.o')) if p.name not in ('int_achievement.o','int_mail.o')]
        libraries=['/rathena/'+p for p in ('src/common/obj/common.a','3rdparty/libconfig/obj/libconfig.a','3rdparty/rapidyaml/obj/ryml.a')]
        result=container('g++',flags+['tools/ci/achievement_sql_runtime.cpp','/evidence/int_achievement.o','/evidence/int_mail.o']+objects+libraries+['-Wl,--wrap=main','-lz','-ldl','-lmysqlclient','-l:libzstd.so.1','-lssl','-lcrypto','-lresolv','-lm','-o','/evidence/achievement-sql'])
        (out/'link.log').write_text(result.stdout+result.stderr)
        run('docker','network','create','--internal',net);network=True
        run('docker','run','-d','--name',db,'--network',net,'--network-alias','achievement-db','--memory','512m','--cpus','1','-e','MARIADB_ROOT_PASSWORD=achievement-fixture-only',args.database_image);created.append(db);ready()
        schema=(root/'sql-files/main.sql').read_text()
        def table(name):
            start=schema.index(f'CREATE TABLE IF NOT EXISTS `{name}`')
            return schema[start:schema.index(';',start)+1]
        sql('CREATE DATABASE achievement_probe; USE achievement_probe;'+''.join(table(name) for name in ('achievement','mail','mail_attachments')))
        # This focused fixture does not install the unrelated global-point
        # schema referenced by the coordinated release migration.
        sql('USE achievement_probe; ALTER TABLE achievement ENGINE=InnoDB;')
        report['binary_sha256']=hashlib.sha256((out/'achievement-sql').read_bytes()).hexdigest()
        runtime('normal');runtime('reward');runtime('concurrency-setup')
        for suffix in ('a','b'):
            name=probe+'-'+suffix
            container('/evidence/achievement-sql',['concurrent-'+suffix],net,True,name);created.append(name)
        for suffix in ('a','b'):
            name=probe+'-'+suffix;code=run('docker','wait',name).stdout.strip()
            logs=run('docker','logs',name);(out/('concurrent-'+suffix+'.log')).write_text(logs.stdout+logs.stderr)
            if code!='0':raise RuntimeError('Concurrent snapshot failed: '+suffix)
        runtime('concurrency-verify');runtime('crash-setup')
        container('/evidence/achievement-sql',['crash'],net,True);created.append(probe)
        for _ in range(120):
            if int(sql("SELECT COUNT(*) FROM information_schema.PROCESSLIST WHERE STATE='User sleep'").stdout.strip())>0:break
            state=run('docker','inspect','--format','{{.State.Running}}',probe).stdout.strip()
            if state!='true':raise RuntimeError('Crash probe exited before SQL delay')
            time.sleep(.25)
        else:raise RuntimeError('No active trigger delay observed')
        run('docker','kill',db)
        code=run('docker','wait',probe).stdout.strip()
        logs=run('docker','logs',probe);(out/'crash.log').write_text(logs.stdout+logs.stderr)
        if code!='0':raise RuntimeError('Crash probe did not return failure safely: '+code)
        run('docker','start',db);ready();runtime('crash-verify')
        if hashes()!=report['inputs_sha256']:raise RuntimeError('Inputs changed during SQL proof')
        report['passed']=True
    except Exception as error:
        report['error']=str(error)
        if isinstance(error,subprocess.CalledProcessError):(out/'failed-command.log').write_text((error.stdout or '')+(error.stderr or ''))
        raise
    finally:
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        for name in reversed(created):run('docker','rm','-f','-v',name,check=False)
        if network:run('docker','network','rm',net,check=False)
    print(json.dumps({'passed':report['passed'],'checks':report['checks']}))


if __name__=='__main__':main()
