"""Native VM/packet acceptance, restricted to disposable labelled Docker fixtures.
Seed 99000031/99000032 gameplayfixture; memo rows contain six distinct towns.
Load npc/test/pn_gameplay_fixture.txt only in the isolated fixture.
"""
import argparse,json,os,socket,struct,subprocess,time,sys
from bank_live_client import Client,drain


def main():
 p=argparse.ArgumentParser();p.add_argument('--game',required=True);p.add_argument('--database',required=True);args=p.parse_args()
 info=json.loads(subprocess.check_output(['docker','inspect',args.game]))[0]
 assert info['Config']['Labels'].get('pn.wallet64.fixture')=='true'
 assert os.readlink('/proc/self/ns/net')==os.readlink('/proc/'+str(info['State']['Pid'])+'/ns/net')
 def sql(query):
  return subprocess.check_output(['docker','exec','-i','-e','MYSQL_PWD=project-validation-only',args.database,'mariadb','-uroot','-N','--batch','ragnarok_ci'],input=query,text=True).strip()
 assert sql("SELECT account_id,name FROM `char` WHERE char_id=99000032")=='99000031\tgameplayfixture'
 assert sql("SELECT COUNT(*) FROM `char` WHERE char_id NOT IN (99000012,99000022,99000032)")=='0'
 c=Client(account_id=99000031,character_id=99000032,username=b'gameplayfixture',password=b'wide-fixture-only',attach=False)
 evidence=[]
 def send(payload,delay=.6):
  c.world.sendall(payload);raw=drain(c.world,delay)
  assert b'PN_GAMEPLAY_NATIVE_FAIL' not in raw,raw
  return raw
 def command(action,delay=.8):
  time.sleep(.3);msg=('gameplayfixture : @pngameplayfixture '+action+'\0').encode()
  packet=struct.pack('<HH',0xf3,len(msg)+4)+msg
  raw=send(packet,delay)
  if len(raw)==6 and raw[:2]==struct.pack('<H',0xb6):
   send(struct.pack('<HI',0x146,struct.unpack_from('<I',raw,2)[0]));time.sleep(.3);raw=send(packet,delay)
  close=raw.find(struct.pack('<H',0xb6))
  if close>=0 and len(raw)>=close+6:
   send(struct.pack('<HI',0x146,struct.unpack_from('<I',raw,close+2)[0]))
  return raw
 def warps(raw):
  pos=raw.find(struct.pack('<H',0xabe));assert pos>=0,raw.hex()
  size,skill=struct.unpack_from('<HH',raw,pos+2);assert skill==27 and size>=6 and (size-6)%16==0
  return [raw[i:i+16].split(b'\0',1)[0] for i in range(pos+6,pos+size,16)]
 def menu(raw):
  pos=raw.find(struct.pack('<H',0xb7));assert pos>=0,raw.hex()
  return struct.unpack_from('<I',raw,pos+4)[0]
 try:
  destinations=warps(command('memo-unlock',1.2));assert len(destinations)==7,destinations
  selected=send(struct.pack('<HH16s',0x11b,27,destinations[-1]),1.2);print('WARP_SELECTION',destinations,selected.hex(),file=sys.stderr)
  entered=command('memo-enter',2);assert b'portal walk submitted' in entered,entered.hex()
  # A portal arrival changes map and requires the ordinary map-load acknowledgement.
  assert b'\x91\x00' in entered or b'\x92\x00' in entered,entered.hex()
  send(struct.pack('<H',0x7d))
  evidence.append('native Warp Portal lists save point + all six memo slots and selected sixth portal transports character')
  assert b'mission rescue setup' in command('mission-setup')
  for rescue in (2,0,0,1):
   response=command('mission-rescue '+str(rescue))
   pos=response.find(struct.pack('<H',0xb6));assert pos>=0,response.hex()
   rescue_gid=struct.unpack_from('<I',response,pos+2)[0]
   send(struct.pack('<HI',0x146,rescue_gid))
  assert b'PN_GAMEPLAY_NATIVE_PASS mission rescue bits and quest records' in command('mission-report')
  evidence.append('real missionary rescue function completes all three quest records out of order and tolerates duplicate rescue')
  reset=warps(command('memo-reset',1.2));assert len(reset)==4,reset
  send(struct.pack('<HH16s',0x11b,27,b'cancel'))
  evidence.append('native skill reset clears extra reward/mission state and restricts selection to three memo slots')
  rental=command('rental',6)
  assert b'PN_GAMEPLAY_NATIVE_PASS paid recovery and expired-rental no-refund' in rental,rental.hex()
  evidence.append('native paid trap consumes/refunds once; rental expires and its trap recovery refunds no material')
  first=command('refiner');gid=menu(first)
  selection=send(struct.pack('<HIB',0xb8,gid,1));assert struct.pack('<H',0xb5) in selection,selection.hex()
  confirm=send(struct.pack('<HI',0xb9,gid));assert menu(confirm)==gid
  send(struct.pack('<HIB',0xb8,gid,1));send(struct.pack('<HI',0x146,gid))
  result=command('refiner-report');assert b'PN_GAMEPLAY_NATIVE_PASS refiner +15 single certificate' in result,result.hex()
  evidence.append('real Master Refiner function dialog consumes exactly one certificate and refines equipped club to +15')
 finally:
  c.world.setsockopt(socket.SOL_SOCKET,socket.SO_LINGER,struct.pack('ii',1,0));c.world.close();c.char.close()
 for attempt in range(30):
  persisted=sql('SELECT nameid,amount,refine FROM inventory WHERE char_id=99000032 ORDER BY nameid')
  if sql('SELECT refine FROM inventory WHERE char_id=99000032 AND nameid=1501')=='15':break
  time.sleep(.5)
 else:raise AssertionError('Saved inventory after logout: '+repr(persisted))
 assert sql('SELECT COUNT(*) FROM inventory WHERE char_id=99000032 AND nameid IN (6872,50152,1065)')=='0'
 assert sql('SELECT COUNT(*) FROM memo WHERE char_id=99000032')=='3'
 evidence.append('logout persists +15 result, consumed certificate, expired rental, and three memo records')
 print(json.dumps({'passed':True,'cases':evidence},indent=2))

if __name__=='__main__':main()
