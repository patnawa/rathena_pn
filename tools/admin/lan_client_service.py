#!/usr/bin/env python3
"""Serve only public client artifacts and sanitized health on the development LAN."""
import argparse,ipaddress,json,re,socket
from datetime import datetime,timezone
from http.server import BaseHTTPRequestHandler,ThreadingHTTPServer
from pathlib import Path
from urllib.parse import urlsplit

PAGE='''<!doctype html><html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>PN Development Server</title><style>
*{box-sizing:border-box}body{background:#10151d;color:#eaf0f7;font:16px system-ui;margin:0;padding:7vh 6vw}main{max-width:1000px;margin:auto}small{color:#90a2b7}h1{font-size:40px;font-weight:600;margin:.4em 0}h2{font-size:18px}a{color:#8ac8ff}header{border-bottom:1px solid #303e50;padding-bottom:28px}#summary{font-size:24px;margin:30px 0 18px}.grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px}.card{background:#1b2431;border:1px solid #334156;border-radius:12px;padding:20px}.ok{color:#86e0b0}.bad{color:#ffbd86}footer{color:#9babbd;margin-top:30px;line-height:1.8}.button{display:inline-block;background:#b9dbff;color:#142a43;padding:10px 18px;border-radius:8px;text-decoration:none;margin-top:12px}</style>
<main><header><small>PN / PRIVATE LAN</small><h1>Development server</h1><div>192.168.10.18 &middot; No external notifications</div><a class="button" href="/PNLauncher.exe">Download launcher</a><div><small>Put PNLauncher.exe in your PN-Client folder, or an empty folder to install.</small></div></header>
<div id="summary">Checking services&hellip;</div><div id="cards" class="grid"></div><footer id="detail"></footer></main>
<script>async function update(){try{let r=await fetch('/status.json',{cache:'no-store'});if(!r.ok)throw Error();let s=await r.json();document.querySelector('#summary').textContent=s.game_online?'Game services are reachable':'Game services need attention';let cards=document.querySelector('#cards');cards.replaceChildren();for(let [name,value] of Object.entries(s.services)){let c=document.createElement('div');c.className='card';let h=document.createElement('h2');h.textContent=name;let p=document.createElement('div');p.className=value?'ok':'bad';p.textContent=value?'Reachable':'Unavailable';c.append(h,p);cards.append(c)}let footer=document.querySelector('#detail');footer.textContent='Last check: '+new Date(s.checked_utc).toLocaleString()+'. Health report: '+(s.health_fresh?(s.health_passed?'passed':'needs review'):'stale or unavailable')+'. Free disk: '+(s.free_gib===null?'unknown':s.free_gib+' GiB')+'. Restore-verified backup: '+(s.backup_passed?'available':'needs review')+'.';}catch(e){document.querySelector('#summary').textContent='Status service unavailable'}}update();setInterval(update,30000)</script></html>'''

def status(root):
    now=datetime.now(timezone.utc);services={}
    for name,port in [('Login',6900),('Character',6121),('Map',5121),('Game web',8888)]:
        try:
            with socket.create_connection(('192.168.10.18',port),timeout=.7):services[name]=True
        except OSError:services[name]=False
    health={};fresh=False
    try:
        health=json.loads((root/'health.json').read_text())
        stamp=datetime.fromisoformat(health['checked_utc'])
        fresh=0<=(now-stamp).total_seconds()<420
    except (OSError,ValueError,KeyError,TypeError):pass
    checks=health.get('checks',{})
    return {'checked_utc':now.isoformat(),'game_online':all(services[n] for n in ('Login','Character','Map')),
            'services':services,'health_fresh':fresh,'health_passed':fresh and health.get('passed') is True,
            'free_gib':checks.get('disk',{}).get('free_gib'),
            'backup_passed':fresh and checks.get('backup',{}).get('passed') is True}

def handler(root):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self,*args):pass
        def do_HEAD(self):self.serve(False)
        def do_GET(self):self.serve(True)
        def serve(self,body):
            address=ipaddress.ip_address(self.client_address[0])
            if not(address.is_loopback or address in ipaddress.ip_network('192.168.10.0/24')):
                self.send_error(403);return
            path=urlsplit(self.path).path;data=None;file=None;mime='application/octet-stream'
            if path=='/':data=PAGE.encode();mime='text/html; charset=utf-8'
            elif path=='/status.json':data=json.dumps(status(root)).encode();mime='application/json'
            elif path in ('/release.json','/PNLauncher.exe'):file=root/path[1:]
            elif re.fullmatch(r'/objects/[0-9a-f]{64}',path):file=root/path[1:]
            else:self.send_error(404);return
            if file:
                if file.is_symlink() or not file.is_file() or not file.resolve().is_relative_to(root.resolve()):self.send_error(404);return
                length=file.stat().st_size
            else:length=len(data)
            self.send_response(200);self.send_header('Content-Type',mime);self.send_header('Content-Length',str(length))
            self.send_header('X-Content-Type-Options','nosniff');self.send_header('Cache-Control','public,max-age=31536000,immutable' if path.startswith('/objects/') else 'no-store');self.end_headers()
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
