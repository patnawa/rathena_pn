"""Health freshness, release privacy and public route regression checks."""
import base64,json,socket,tempfile,threading,unittest,urllib.error,urllib.request
from datetime import datetime,timedelta,timezone
from http.server import ThreadingHTTPServer
from pathlib import Path
from unittest.mock import patch
import lan_client_service as service

class Connection:
    def __enter__(self):return self
    def __exit__(self,*args):pass

class StatusTests(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory();self.root=Path(self.temp.name)
    def tearDown(self):self.temp.cleanup()
    def health(self,age=0,**extra):
        value={'checked_utc':(datetime.now(timezone.utc)-timedelta(seconds=age)).isoformat(),'passed':True,'checks':{'disk':{'free_gib':228.03},'backup':{'passed':True}}};value.update(extra)
        (self.root/'health.json').write_text(json.dumps(value))
    def status(self,failed=()):
        def connect(address,**kw):
            if address[1] in failed:raise OSError('offline')
            return Connection()
        with patch.object(socket,'create_connection',side_effect=connect):return service.status(self.root)
    def test_game_online_independent_of_backup(self):
        self.health(passed=False,checks={'disk':{'free_gib':228.03},'backup':{'passed':False}})
        value=self.status();self.assertTrue(value['game_online']);self.assertFalse(value['health_passed']);self.assertFalse(value['backup_passed'])
    def test_only_required_services_determine_play(self):
        self.assertTrue(self.status((8888,))['game_online']);self.assertFalse(self.status((5121,))['game_online'])
    def test_stale_health_hides_old_values(self):
        self.health(age=421);value=self.status();self.assertFalse(value['health_fresh']);self.assertFalse(value['backup_passed']);self.assertIsNone(value['free_gib'])
        self.health(age=-30);self.assertFalse(self.status()['health_fresh'])
    def test_malformed_reports_and_disk(self):
        for value in ([],None,{'checked_utc':'bad','checks':[]},{'checked_utc':datetime.now(timezone.utc).isoformat(),'checks':{'disk':{'free_gib':float('nan')},'backup':[]}}):
            (self.root/'health.json').write_text(json.dumps(value));self.assertIsNone(self.status()['free_gib'])
    def test_unrepresentable_disk_size_is_hidden(self):
        self.health(checks={'disk':{'free_gib':10**400},'backup':{'passed':True}})
        value=self.status()
        self.assertIsNone(value['free_gib'])
        self.assertTrue(value['game_online'])
        self.assertTrue(value['backup_passed'])
    def test_release_metadata_is_allowlisted(self):
        payload={'release':'client-20261005-test','files':[{'path':'private/path','bytes':123,'sha256':'secret'}],'private':'secret'}
        (self.root/'release.json').write_text(json.dumps({'payload':base64.b64encode(json.dumps(payload).encode()).decode(),'signature':'not displayed'}))
        self.assertEqual(service.client_info(self.root),{'release':'client-20261005-test','bytes':123,'file_count':1})
        payload['files'][0]['bytes']=-1;(self.root/'release.json').write_text(json.dumps({'payload':base64.b64encode(json.dumps(payload).encode()).decode()}));self.assertIsNone(service.client_info(self.root))
    def test_public_routes_head_and_private_denials(self):
        (self.root/'PNLauncher.exe').write_bytes(b'launcher');(self.root/'health.json').write_text('private');(self.root/'objects').mkdir();digest='a'*64;(self.root/'objects'/digest).write_bytes(b'object')
        server=ThreadingHTTPServer(('127.0.0.1',0),service.handler(self.root));thread=threading.Thread(target=server.serve_forever,daemon=True);thread.start();base=f'http://127.0.0.1:{server.server_port}'
        try:
            for path,mime in [('/','text/html'),('/assets/status.js','text/javascript'),('/assets/status.css','text/css'),('/assets/midgard-hero.jpg','image/jpeg'),('/PNLauncher.exe','application/octet-stream')]:
                with urllib.request.urlopen(base+path) as response:self.assertIn(mime,response.headers['Content-Type']);self.assertIn("script-src 'self'",response.headers['Content-Security-Policy']);self.assertGreater(len(response.read()),0)
            with urllib.request.urlopen(urllib.request.Request(base+'/PNLauncher.exe',method='HEAD')) as response:self.assertEqual(response.read(),b'');self.assertEqual(response.headers['Content-Length'],'8')
            with urllib.request.urlopen(base+'/objects/'+digest) as response:self.assertIn('immutable',response.headers['Cache-Control']);self.assertEqual(response.read(),b'object')
            for path in ['/health.json','/assets/../health.json','/assets/index.html','/objects/not-a-hash','/unknown']:
                with self.assertRaises(urllib.error.HTTPError) as error:urllib.request.urlopen(base+path)
                self.assertEqual(error.exception.code,404)
        finally:server.shutdown();server.server_close();thread.join(3)

if __name__=='__main__':unittest.main()
