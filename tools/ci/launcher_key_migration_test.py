"""Exercise real signed key migration on an isolated Windows client fixture.

Publication contains release-v2.json, previous-release.json, replay-new-key.json,
wrong-key.json, plan.json and the changed objects. No private key is needed.
"""
from pathlib import Path
import argparse,base64,hashlib,http.server,json,os,subprocess,threading,time

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--publication',type=Path,required=True)
    parser.add_argument('--client',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    args=parser.parse_args();pub=args.publication.resolve();client=args.client.resolve();out=args.output.resolve()
    assert not out.exists(),'Preserve previous evidence';out.mkdir(parents=True)
    root=out/'client';root.mkdir();build=out/'build';build.mkdir()
    source=Path(__file__).resolve().parents[2]/'client-patch/launcher'
    old=(pub/'previous-release.json').read_bytes();new=(pub/'release-v2.json').read_bytes()
    plan=json.loads((pub/'plan.json').read_bytes())
    oldmanifest=json.loads(base64.b64decode(json.loads(old)['payload']));manifest=json.loads(base64.b64decode(json.loads(new)['payload']))
    assert (client/'.pn-updater/installed.json').read_bytes()==old,'Installed baseline differs'
    for row in oldmanifest['files']:
        target=root/row['path'];target.parent.mkdir(parents=True,exist_ok=True);os.link(client/row['path'],target)
    work=root/'.pn-updater';work.mkdir();(work/'installed.json').write_bytes(old);(work/'highest-sequence.txt').write_text(str(oldmanifest['sequence']))
    personal=['savedata/ChatWndInfo_U.lua','savedata/LikeListInfoFile.lua','savedata/shortcutlist/character.lua']
    for name in personal:
        target=root/name;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(b'personal fixture '+name.encode())
    requests=[];served=[new]
    class Handler(http.server.BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_GET(self):
            requests.append(self.path)
            if self.path=='/release-v2.json':data=served[0]
            elif self.path.startswith('/objects/'):
                digest=self.path[9:]
                assert len(digest)==64 and all(c in '0123456789abcdef' for c in digest)
                data=(pub/'objects'/digest).read_bytes();assert hashlib.sha256(data).hexdigest()==digest
            else:self.send_error(404);return
            self.send_response(200);self.send_header('Content-Length',str(len(data)));self.send_header('Connection','close');self.end_headers();self.wfile.write(data)
    server=http.server.ThreadingHTTPServer(('127.0.0.1',0),Handler);thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start()
    try:
        launcher=(source/'Launcher.cs').read_text(encoding='utf-8');original='public const string Feed="http://192.168.10.18:8082/";';assert launcher.count(original)==1
        (build/'Launcher.cs').write_text(launcher.replace(original,'public const string Feed="http://127.0.0.1:'+str(server.server_port)+'/";'),encoding='utf-8')
        compiler=Path(os.environ['WINDIR'])/'Microsoft.NET/Framework64/v4.0.30319/csc.exe';exe=build/'PNLauncherCheck.exe'
        command=[str(compiler),'/nologo','/target:exe','/r:System.Windows.Forms.dll','/r:System.Drawing.dll','/r:System.Web.Extensions.dll',
            '/resource:'+str(source/'trusted-public-key.xml')+',trusted-public-key.xml',
            '/resource:'+str(source/'legacy-public-key.xml')+',legacy-public-key.xml','/out:'+str(exe),str(build/'Launcher.cs')]
        command.extend(str(p) for p in source.glob('*.cs') if p.name!='Launcher.cs')
        result=subprocess.run(command,capture_output=True);(out/'compile.log').write_bytes(result.stdout+result.stderr);assert result.returncode==0
        def run(operation,label,passed=True,target=root):
            start=time.perf_counter();result=subprocess.run([str(exe),operation,str(target)],capture_output=True,timeout=120)
            (out/(label+'.log')).write_bytes(result.stdout+result.stderr)
            assert (result.returncode==0)==passed,label
            print(label+': '+str(round(time.perf_counter()-start,2))+'s',flush=True);return result
        run('--verify-installed','legacy-installed-verification')
        run('--verify-manifest','current-key-verification',target=pub/'release-v2.json')
        for label,data in [('wrong-key',(pub/'wrong-key.json').read_bytes()),('legacy-network',old)]:
            served[0]=data;result=run('--update',label,False)
            assert b'signature is invalid' in result.stderr
            assert (work/'installed.json').read_bytes()==old and not (work/'transaction.json').exists()
        served[0]=new;run('--update','signed-upgrade')
        for row in plan['client_changes']:assert hashlib.sha256((root/row['path']).read_bytes()).hexdigest()==row['sha256']
        assert len([p for p in requests if p.startswith('/objects/')])==len(plan['client_changes'])
        run('--verify-installed','current-installed-verification')
        # A valid signature by the current key must still obey the sequence floor.
        served[0]=(pub/'replay-new-key.json').read_bytes();result=run('--update','current-key-replay-refusal',False)
        assert b'older release was rejected' in result.stderr and (work/'installed.json').read_bytes()==new
        run('--rollback','signed-rollback')
        assert (work/'installed.json').read_bytes()==old
        for row in plan['client_changes']:
            target=root/row['path'];original=client/row['path']
            if row['path'] not in {r['path'] for r in oldmanifest['files']}:assert not target.exists()
            else:assert target.read_bytes()==original.read_bytes()
        assert int((work/'highest-sequence.txt').read_text())==manifest['sequence']
        run('--verify-installed','legacy-rollback-verification')
        result=run('--update','post-rollback-replay-refusal',False);assert b'older release was rejected' in result.stderr
        served[0]=new;run('--update','signed-reupgrade')
        envelope=json.loads(new);payload=base64.b64decode(envelope['payload'])+b' '
        envelope['payload']=base64.b64encode(payload).decode();served[0]=json.dumps(envelope).encode()
        result=run('--update','tampered-payload-refusal',False);assert b'signature is invalid' in result.stderr
        assert (work/'installed.json').read_bytes()==new and not (work/'transaction.json').exists()
        for name in personal:assert (root/name).read_bytes()==b'personal fixture '+name.encode()
        report=dict(passed=True,release=manifest['release'],sequence=manifest['sequence'],
            legacy_installed_verified=True,current_signature_verified=True,wrong_key_rejected=True,
            legacy_network_signature_rejected=True,native_signed_upgrade=True,native_local_verification=True,
            native_rollback_exact=True,legacy_rollback_verified=True,older_feed_refused=True,native_reupgrade=True,
            tampered_payload_rejected=True,settings_preserved=True,downloaded_only_launcher_objects=True,
            feed_sha256=hashlib.sha256(new).hexdigest(),source_revision=plan['source_revision'],
            test_source_difference='Only the feed base points to the loopback fixture server.')
        (out/'report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
    finally:server.shutdown();server.server_close();thread.join(5)

if __name__=='__main__':main()
