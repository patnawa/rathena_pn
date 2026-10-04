"""Actual sender/receiver + login ownership functions; no DB/network/server build.

Source definitions are copied verbatim into a tiny linked seam. The STL debug
iterator mode makes erase-current-then-increment defects deterministic.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time

ROOT=Path(__file__).resolve().parents[2]
CASES=('packet-roundtrip','server-offline','all-offline','cleanup',
       'multiple-empty-isolation','empty-roster','partial-packet','malformed-packet')


def definition(path, signature):
    source=path.read_text()
    assert source.count(signature)==1, 'source seam changed: '+signature
    start=source.index(signature); opening=source.index('{',start); depth=0
    for end in range(opening,len(source)):
        if source[end]=='{':depth+=1
        elif source[end]=='}':
            depth-=1
            if not depth:return source[start:end+1]
    raise AssertionError('incomplete definition: '+signature)


def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--evidence',type=Path);args=ap.parse_args()
    source=[]
    for signature in ('struct online_char_data {',):
        source.append(definition(ROOT/'src/char/char.hpp',signature)+';')
    source.append(definition(ROOT/'src/char/char.cpp','online_char_data::online_char_data( uint32 account_id )'))
    source.append('static std::unordered_map<uint32,std::shared_ptr<online_char_data>> char_online;\nstatic auto& char_get_onlinedb(){return char_online;}')
    for signature in ('struct online_login_data* login_get_online_user(', 'struct online_login_data* login_add_online_user(',
                      'void login_remove_online_user(', 'void login_remove_auth_node(',
                      'TIMER_FUNC(login_waiting_disconnect_timer)', 'void login_online_db_setoffline(',
                      'static TIMER_FUNC(login_online_data_cleanup)'):
        source.append(definition(ROOT/'src/login/login.cpp',signature))
    source.append(definition(ROOT/'src/char/char_logif.cpp','TIMER_FUNC(chlogif_send_acc_tologin)'))
    source.append(definition(ROOT/'src/login/loginchrif.cpp','int32 logchrif_parse_updonlinedb('))
    template=(ROOT/'tools/ci/login_online_roster_test.cpp').read_text();assert template.count('// @ACTUAL_SOURCE@')==1
    with tempfile.TemporaryDirectory(prefix='login-roster-') as temp:
        work=Path(temp);driver=work/'driver.cpp';binary=work/'test';driver.write_text(template.replace('// @ACTUAL_SOURCE@','\n\n'.join(source)))
        command=['g++','-std=c++17','-O1','-g','-D_GLIBCXX_DEBUG','-I'+str(ROOT/'src'),str(driver),'-o',str(binary)]
        subprocess.run(command,check=True)
        reports=[]
        if args.evidence:args.evidence.mkdir(parents=True,exist_ok=False);(args.evidence/'driver.cpp').write_bytes(driver.read_bytes())
        for case in CASES:
            start=time.monotonic();result=subprocess.run([str(binary),case],text=True,capture_output=True,timeout=5)
            output=result.stdout+result.stderr;print(output,end='')
            row={'name':case,'passed':result.returncode==0,'exit_code':result.returncode,'elapsed_seconds':time.monotonic()-start}
            reports.append(row)
            if args.evidence:(args.evidence/(case+'.log')).write_text(output)
        report={'passed':all(row['passed'] for row in reports),'cases':reports,'compile_command':command,
                'database_accessed':False,'network_accessed':False,'method':'verbatim production definitions; actual checked std::unordered_map; FIFO/timer/webtoken boundary adapters'}
        if args.evidence:
            sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
            report['source_hashes']={rel:sha(ROOT/rel) for rel in ['src/login/login.cpp','src/login/loginchrif.cpp','src/login/login.hpp','src/char/char_logif.cpp','src/char/char.cpp','src/char/char.hpp','tools/ci/login_online_roster_test.cpp','tools/ci/login_online_roster_test.py']}
            report['artifacts']={p.name:sha(p) for p in args.evidence.iterdir() if p.is_file()}
            (args.evidence/'report.json').write_text(json.dumps(report,indent=2)+'\n')
        print('LOGIN_ROSTER_RESULT',json.dumps(report['cases']))
        return 0 if report['passed'] else 1


if __name__=='__main__':raise SystemExit(main())
