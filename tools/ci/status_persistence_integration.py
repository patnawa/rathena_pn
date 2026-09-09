#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  status_persistence_integration.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/status_persistence_integration.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Exercise saved-status persistence through a real isolated character server.

This protocol test is restricted to the named internal test service and synthetic
account/character IDs. It requires the isolated test login and SQL services.
"""
import argparse,json,socket,struct
from pathlib import Path
HOST='progression-test-char'
AID,CID=99000001,99000002
RECORD=struct.Struct('<H6xqqqqq')
EXPECTED=[(i,10*i,-i,3*i,4*i,3600000+i) for i in range(1,101)]
def exact(s,n):
 data=b''
 while len(data)<n:
  more=s.recv(n-len(data))
  if not more:raise RuntimeError('Character server disconnected')
  data+=more
 return data
def packet(s):
 head=exact(s,2);kind=struct.unpack('<H',head)[0]
 if kind==0x2af9:return kind,head+exact(s,1)
 if kind==0x2b24:return kind,head
 length=exact(s,2);n=struct.unpack('<H',length)[0]
 if n<4 or n>65535:raise ValueError(('Invalid packet length',kind,n))
 return kind,head+length+exact(s,n-4)
def connect():
 s=socket.create_connection((HOST,6121),timeout=10);s.settimeout(10)
 request=bytearray(60);struct.pack_into('<H',request,0,0x2af8)
 request[2:26]=b'validation'.ljust(24,b'\0');request[26:50]=b'validation-only'.ljust(24,b'\0')
 request[54:58]=socket.inet_aton('127.0.0.1');struct.pack_into('!H',request,58,5999)
 s.sendall(request);kind,reply=packet(s);assert kind==0x2af9 and reply[2]==0,'Isolated map-server authentication failed'
 return s
def load(s):
 s.sendall(struct.pack('<HII',0x2afc,AID,CID))
 while True:
  kind,data=packet(s)
  if kind!=0x2b1d:continue
  _,length,aid,cid,count=struct.unpack_from('<HHIIH',data)
  assert (aid,cid)==(AID,CID) and count==100 and length==14+count*RECORD.size
  values=[RECORD.unpack_from(data,14+i*RECORD.size) for i in range(count)]
  assert sorted(values)==EXPECTED,'Saved status fields did not round-trip'
  return count

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--phase',choices=['seed','verify'],required=True);p.add_argument('--report',type=Path,required=True);a=p.parse_args()
 with connect() as s:
  if a.phase=='seed':
   payload=b''.join(RECORD.pack(*row) for row in EXPECTED)
   s.sendall(struct.pack('<HHIIH',0x2b1c,14+len(payload),AID,CID,len(EXPECTED))+payload)
  count=load(s)
 # A fresh connection must see the same committed rows.
 with connect() as s:assert load(s)==count
 report={'phase':a.phase,'passed':True,'statuses':count,'connections':2,'record_bytes':RECORD.size,'boundary':'Real character-server protocol and SQL; no rendered game client'}
 a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n');print('PASS:',a.phase,'100 statuses preserve every value over two real server connections')
if __name__=='__main__':main()
