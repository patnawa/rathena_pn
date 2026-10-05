#!/usr/bin/env python3
"""Serve only public client artifacts and sanitized health on the development LAN."""
import argparse,base64,ipaddress,json,math,re,socket
from datetime import datetime,timezone
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

ASSETS=Path(__file__).with_name('lan_client_status')
PUBLIC_ASSETS={
    '/':('index.html','text/html; charset=utf-8'),
    '/assets/status.css':('status.css','text/css; charset=utf-8'),
    '/assets/status.js':('status.js','text/javascript; charset=utf-8'),
    '/assets/midgard-hero.jpg':('midgard-hero.jpg','image/jpeg'),
    '/assets/pn-launcher.png':('pn-launcher.png','image/png'),
}

def read_object(path):
    if path.stat().st_size>8000000:raise ValueError('JSON too large')
    value=json.loads(path.read_text(encoding='utf-8-sig'))
    if not isinstance(value,dict):raise ValueError('Expected object')
    return value

def client_info(root):
    """Display only published release metadata; the launcher verifies signatures."""
    try:
        envelope=read_object(root/'release.json')
        manifest=json.loads(base64.b64decode(envelope['payload'],validate=True))
        name=manifest['release'];files=manifest['files']
        if not isinstance(name,str) or not 0<len(name)<=180 or not isinstance(files,list):raise ValueError('Invalid metadata')
        if not 0<len(files)<=20000:raise ValueError('Invalid file list')
        sizes=[f['bytes'] for f in files]
        if any(type(n) is not int or not 0<=n<=8*1024**3 for n in sizes):raise ValueError('Invalid size')
        return {'release':name,'bytes':sum(sizes),'file_count':len(files)}
    except (OSError,ValueError,KeyError,TypeError):return None

def status(root):
    now=datetime.now(timezone.utc);services={}
    for name,port in [('Login',6900),('Character',6121),('Map',5121),('Game web',8888)]:
        try:
            with socket.create_connection(('192.168.10.18',port),timeout=.7):services[name]=True
        except OSError:services[name]=False
    health={};fresh=False;health_stamp=None
    try:
        health=read_object(root/'health.json')
        stamp=datetime.fromisoformat(health['checked_utc'])
        fresh=0<=(now-stamp).total_seconds()<420
        health_stamp=stamp.isoformat()
    except (OSError,ValueError,KeyError,TypeError):pass
    checks=health.get('checks',{})
    if not isinstance(checks,dict):checks={}
    disk=checks.get('disk',{});backup=checks.get('backup',{})
    if not isinstance(disk,dict):disk={}
    if not isinstance(backup,dict):backup={}
    free=disk.get('free_gib')
    try:valid_free=type(free) in (int,float) and math.isfinite(free) and free>=0
    except OverflowError:valid_free=False
    if not fresh or not valid_free:free=None
    return {'checked_utc':now.isoformat(),'game_online':all(services[n] for n in ('Login','Character','Map')),
            'services':services,'health_fresh':fresh,'health_passed':fresh and health.get('passed') is True,
            'health_checked_utc':health_stamp,'free_gib':free,
            'backup_passed':fresh and backup.get('passed') is True}

def handler(root,assets=ASSETS):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_HEAD(self):self.serve(False)
        def do_GET(self):self.serve(True)
        def serve(self,body):
            address=ipaddress.ip_address(self.client_address[0])
            if not(address.is_loopback or address in ipaddress.ip_network('192.168.10.0/24')):
                self.send_error(403);return
            path=urlsplit(self.path).path;data=None;file=None;mime='application/octet-stream';file_root=root
            if path in PUBLIC_ASSETS:
                name,mime=PUBLIC_ASSETS[path];file=assets/name;file_root=assets
            elif path=='/status.json':data=json.dumps(status(root)).encode();mime='application/json'
            elif path=='/client-info.json':
                info=client_info(root)
                if info is None:self.send_error(503);return
                data=json.dumps(info).encode();mime='application/json'
            elif path in ('/release.json','/PNLauncher.exe'):
                file=root/path[1:]
                if path=='/release.json':mime='application/json'
            elif re.fullmatch(r'/objects/[0-9a-f]{64}',path):file=root/path[1:]
            else:self.send_error(404);return
            if file:
                if file.is_symlink() or not file.is_file() or not file.resolve().is_relative_to(file_root.resolve()):self.send_error(404);return
                length=file.stat().st_size
            else:length=len(data)
            self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(length))
            self.send_header('X-Content-Type-Options','nosniff');self.send_header('Cache-Control','public,max-age=31536000,immutable' if path.startswith('/objects/') else 'no-store')
            self.send_header('Content-Security-Policy',"default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self'; connect-src 'self'; object-src 'none'; base-uri 'none'; frame-ancestors 'none'")
            self.send_header('Referrer-Policy','no-referrer');self.end_headers()
            if body:
                try:
                    if file:
                        with file.open('rb') as stream:
                            while block:=stream.read(1024*1024):self.wfile.write(block)
                    else:self.wfile.write(data)
                except (BrokenPipeError,ConnectionResetError):pass
    return Handler

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--root',type=Path,default=Path('/srv/pn-client'));p.add_argument('--bind',default='192.168.10.18');p.add_argument('--port',type=int,default=8082);a=p.parse_args()
    ThreadingHTTPServer((a.bind,a.port),handler(a.root)).serve_forever()
