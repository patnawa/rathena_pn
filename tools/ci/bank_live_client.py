"""Real bank packets against an explicitly isolated Docker game fixture only."""
from pathlib import Path
import json
import os
import re
import socket
import struct
import subprocess
import time

GAME='bank-20260913-game'
DB='bank-20260913-runtime-db'
AID,CID=99000011,99000012

def command(args):
    return subprocess.check_output(args,text=True).strip()

def sql(query):
    return subprocess.check_output(['docker','exec','-i',DB,'mariadb','-uroot','-pbank-runtime-only',
        '--batch','--raw','-N','bank_runtime'],input=query,text=True).strip()

def exact(connection,size):
    result=b''
    while len(result)<size:
        part=connection.recv(size-len(result))
        if not part: raise RuntimeError('Fixture connection ended early')
        result+=part
    return result

def drain(connection,seconds=.3):
    result=b'';deadline=time.monotonic()+seconds;connection.settimeout(.1)
    while time.monotonic()<deadline:
        try:
            part=connection.recv(65536)
            if not part: break
            result+=part
        except socket.timeout: pass
    connection.settimeout(8);return result

class Client:
    def __init__(self,slot=0,attach=True,*,account_id=AID,character_id=CID,username=b'bankfixture',password=b'bank-fixture-only',login_port=6900,char_port=6121,map_port=5121):
        self.aid=account_id;self.cid=character_id+slot;self.sequence=0
        self.map_port=map_port
        # A real map crash retains the old login session until the stock
        # disconnect watchdog clears it. Respect that recovery path.
        for attempt in range(12):
            with socket.create_connection(('127.0.0.1',login_port),8) as login:
                login.sendall(struct.pack('<HI24s24sB',0x64,20260219,username,password,0))
                head=exact(login,2);kind=struct.unpack('<H',head)[0]
                if kind==0x81:
                    assert exact(login,1)==b'\x08','Unexpected fixture login rejection'
                    time.sleep(5);continue
                assert kind==0xac4,hex(kind)
                head+=exact(login,2);length=struct.unpack_from('<H',head,2)[0]
                data=head+exact(login,length-4)
                self.key1,aid,self.key2=struct.unpack_from('<III',data,4);sex=data[46];assert aid==self.aid
                break
        else:raise RuntimeError('Stock disconnect watchdog did not release fixture login')
        self.char=socket.create_connection(('127.0.0.1',char_port),8)
        self.char.sendall(struct.pack('<HIIIHB',0x65,self.aid,self.key1,self.key2,0,sex));assert struct.unpack('<I',exact(self.char,4))[0]==self.aid
        drain(self.char);self.char.sendall(struct.pack('<H',0x9a1));drain(self.char)
        self.char.sendall(struct.pack('<HB',0x66,slot));data=drain(self.char)
        position=data.find(b'\xc5\x0a');assert position>=0,'Character selection did not return a map endpoint'
        assert struct.unpack_from('<I',data,position+2)[0]==self.cid
        self.world=socket.create_connection(('127.0.0.1',map_port),8)
        self.world.sendall(struct.pack('<HIIIIIB',0x436,self.aid,self.cid,self.key1,1000,0,sex));assert drain(self.world)
        self.world.sendall(struct.pack('<H',0x7d));drain(self.world,1.5)
        if attach:self.attach()
    def attach(self):
        self.companion=socket.create_connection(('127.0.0.1',self.map_port),5)
        self.nonce=(0,0);self.refresh()
    def bytes(self,action,amount,sequence=None):
        if sequence is None:
            self.sequence+=1;sequence=self.sequence
        return struct.pack('<IHHIIIIQQQqII',0x314b4250,2,64,self.aid,self.cid,self.key1,self.key2,
                           *self.nonce,sequence,amount,action,0)
    def send(self,data,fragment=False):
        if fragment:
            for start,end in ((0,2),(2,9),(9,31),(31,64)):
                self.companion.sendall(data[start:end]);time.sleep(.02)
        else:self.companion.sendall(data)
        state=self.receive()
        while state['flags']:state=self.receive()
        return state
    def receive(self):
        response=exact(self.companion,136)
        assert struct.unpack_from('<IHH',response)==(0x314b4250,2,136)
        assert struct.unpack_from('<qq',response,56)==(9223372036854775807,2147483647)
        assert struct.unpack_from('<I',response,128)[0]==self.cid or struct.unpack_from('<I',response,32)[0]==2, 'Bank reply must identify the authenticated character'
        return {'nonce':struct.unpack_from('<QQ',response,8),'sequence':struct.unpack_from('<Q',response,24)[0],
                'result':struct.unpack_from('<I',response,32)[0],
                'flags':struct.unpack_from('<I',response,36)[0],
                'bank':struct.unpack_from('<q',response,40)[0],'wallet':struct.unpack_from('<q',response,48)[0],
                'diamonds':struct.unpack_from('<I',response,72)[0],'notes':struct.unpack_from('<I',response,76)[0]}
    def refresh(self):
        self.state=self.send(self.bytes(0,0,0));self.nonce=self.state['nonce'];self.sequence=max(self.sequence,self.state['sequence']);return self.state
    def action(self,action,amount):
        time.sleep(.3);self.last=self.bytes(action,amount);state=self.send(self.last)
        assert state['result']==1,state
        return self.wait()
    def wait(self):
        for _ in range(80):
            state=self.refresh()
            if state['result']==0:return state
            assert state['result']==1,state
            drain(self.world,.03);time.sleep(.05)
        raise RuntimeError('Bank commit acknowledgement timed out')
    def close(self):
        self.companion.close()
        self.world.setsockopt(socket.SOL_SOCKET,socket.SO_LINGER,struct.pack('ii',1,0));self.world.close();self.char.close()

def compact(state):
    return {k:state[k] for k in ('bank','wallet','diamonds','notes')}

def start_map():
    log=Path(os.environ.get('BANK_FIXTURE_ROOT','/app/rathena-builds/bank-20260913'))/'runtime/runtime-map.log'
    offset=log.stat().st_size
    command(['docker','exec','-d',GAME,'sh','-c','exec ./map-server >> /evidence/runtime-map.log 2>&1'])
    for _ in range(240):
        if b'Map Server is now online' in log.read_bytes()[offset:]:
            time.sleep(.5);return
        time.sleep(.5)
    raise RuntimeError('Fixture map restart did not become ready')

def main():
    inspect=json.loads(command(['docker','inspect',GAME]))[0]
    assert inspect['Config']['Labels'].get('pn.bank.fixture')=='true'
    assert os.readlink('/proc/self/ns/net')==os.readlink('/proc/'+str(inspect['State']['Pid'])+'/ns/net'), 'Run inside the fixture network namespace only'
    assert sql('SELECT DATABASE()')=='bank_runtime'
    cases=[];c=Client(attach=False)
    c.world.sendall(struct.pack('<HI',0x9b6,AID));wire=drain(c.world)
    assert struct.pack('<HH',0x9b9,0) in wire
    assert struct.pack('<HH',0x9b7,0) not in wire and struct.pack('<H',0x9a6) not in wire
    c.world.sendall(struct.pack('<HI',0x9ab,AID));wire=drain(c.world)
    assert struct.pack('<HH',0x9b9,0) in wire
    assert struct.pack('<HH',0x9b7,0) not in wire and struct.pack('<H',0x9a6) not in wire
    # A close emitted for the suppressed stock panel must not erase the
    # pending custom open before the companion finishes attaching.
    c.world.sendall(struct.pack('<HI',0x9b8,AID));drain(c.world)
    c.attach();assert c.receive()['flags']==1
    assert struct.pack('<HH',0x9b9,0) in drain(c.world)
    cases.append('early bank open and balance check emit no stock open/balance; legacy close preserves the queued custom open')
    c.companion.close();drain(c.world,.3)
    c.world.sendall(struct.pack('<HI',0x9b6,AID));wire=drain(c.world)
    assert struct.pack('<HH',0x9b9,0) in wire
    assert struct.pack('<HH',0x9b7,0) not in wire and struct.pack('<H',0x9a6) not in wire
    c.attach();assert c.receive()['flags']==1
    drain(c.world)
    cases.append('companion reconnection keeps native banking suppressed and reopens only the authenticated custom panel')
    assert compact(c.state)==dict(bank=1000000000,wallet=1000000,diamonds=1,notes=10),c.state
    # Real game bank button and chat command take the server-driven custom path.
    c.world.sendall(struct.pack('<HI',0x9b6,AID))
    opened=c.receive();assert opened['flags']==1 and opened['result']==0 and compact(opened)==compact(c.state)
    wire=drain(c.world);assert struct.pack('<HH',0x9b7,0) not in wire and struct.pack('<H',0x9a6) not in wire
    c.world.sendall(struct.pack('<HI',0x9ab,AID));assert c.receive()['flags']==1
    wire=drain(c.world);assert struct.pack('<HH',0x9b7,0) not in wire and struct.pack('<H',0x9a6) not in wire
    time.sleep(.5);chat=b'BankFixture0 : @bank\0'
    c.world.sendall(struct.pack('<HH',0xf3,4+len(chat))+chat)
    assert c.receive()['flags']==1
    log=(Path(os.environ['BANK_FIXTURE_ROOT'])/'runtime/runtime-map.log').read_text(errors='replace')
    gid=int(re.search(r'BANK_FIXTURE_NPC_ID=(\d+)',log)[1])
    time.sleep(.5);c.world.sendall(struct.pack('<HIB',0x90,gid,0))
    assert c.receive()['flags']==1
    assert sql('SELECT COUNT(*) FROM pn_bank_commits')=='0'
    cases.append('native bank button, @bank and actual NPC openbank open custom panel without a financial mutation')
    first=c.bytes(1,50000);initial=c.send(first,True);assert initial['result']==1
    done=c.wait();assert done['bank']==1000050000 and done['wallet']==950000
    replay=c.send(first);assert replay['result']==0 and compact(replay)==compact(done)
    assert sql('SELECT COUNT(*) FROM pn_bank_commits')=='1'
    cases.append('fragmented authenticated request and duplicate delivery')
    for action,value,code in ((1,-1,3),(1,9223372036854775807,6),(3,5,6),(4,2,9)):
        time.sleep(.3);before=compact(c.refresh());after=c.send(c.bytes(action,value));assert after['result']==code,(action,after);assert compact(after)==before
    cases.append('negative, overflowing, unaffordable and missing-item rejection')
    c.action(3,1);c.action(4,1);c.action(5,5);done=c.action(6,5)
    assert compact(done)==dict(bank=998030000,wallet=950000,diamonds=1,notes=10),done
    assert sql('SELECT refine,bound,unique_id,card0 FROM inventory WHERE nameid=1201')=='10\t2\t987654321\t4001'
    cases.append('diamond and note round trips charge exact fees and preserve other item metadata')
    assert sql("SELECT COUNT(*),SUM(amount) FROM picklog WHERE type='K'")=='4\t0'
    cases.append('all four item exchanges retain bank-specific item logs')
    # Existing native banking uses the same durable path.
    time.sleep(.3);c.world.sendall(struct.pack('<HII',0x9a7,AID,123));wire=drain(c.world,.6)
    assert struct.pack('<HHqI',0x9a8,0,998030123,949877) in wire
    assert sql("SELECT value FROM acc_reg_num WHERE account_id=99000011 AND `key`='#BANKVAULT'")=='998030123'
    time.sleep(.3);c.world.sendall(struct.pack('<HII',0x9a9,AID,123));wire=drain(c.world,.6)
    assert struct.pack('<HHqI',0x9aa,0,998030000,950000) in wire
    cases.append('native deposit and withdrawal acknowledge only after commit')
    old=c.bytes(1,1);c.close();time.sleep(2);c=Client(1)
    assert c.state['bank']==998030000 and c.state['wallet']==1000000 and c.state['diamonds']==0
    assert c.send(old)['result']==2
    cases.append('same-login character sharing and expired-session rejection')
    c.close();time.sleep(2);c=Client()
    before=compact(c.refresh())
    sql("CREATE TRIGGER bank_runtime_fault BEFORE INSERT ON pn_bank_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='intentional bank fixture fault'")
    time.sleep(.3);pending=c.send(c.bytes(1,500));assert pending['result']==1
    time.sleep(1.3)
    assert sql("SELECT value FROM acc_reg_num WHERE account_id=99000011 AND `key`='#BANKVAULT'")==str(before['bank'])
    assert sql('SELECT zeny FROM `char` WHERE char_id=99000012')==str(before['wallet'])
    c.close();time.sleep(.5);sql('DROP TRIGGER bank_runtime_fault');time.sleep(2)
    c=Client();assert c.state['bank']==before['bank']+500 and c.state['wallet']==before['wallet']-500
    cases.append('SQL rollback, lost client socket, save retry and relog')
    after=c.action(1,700);expected=compact(after)
    command(['docker','exec',GAME,'sh','-c','kill -KILL $(pidof map-server)']);c.close();start_map()
    c=Client();assert compact(c.state)==expected,(expected,c.state)
    cases.append('acknowledged transaction survives actual map-server crash')
    before=compact(c.refresh())
    sql("CREATE TRIGGER bank_runtime_fault BEFORE INSERT ON pn_bank_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='intentional bank fixture fault'")
    time.sleep(.3);assert c.send(c.bytes(3,1))['result']==1;time.sleep(.5)
    command(['docker','exec',GAME,'sh','-c','kill -KILL $(pidof map-server)']);c.close()
    sql('DROP TRIGGER bank_runtime_fault');start_map();c=Client()
    assert compact(c.state)==before,(before,c.state)
    cases.append('uncommitted item purchase is fully rolled back after actual map-server crash')
    c.close();time.sleep(2)
    # Seed only this disposable account while offline, then exercise actual
    # registry loading, pc_setparam, interserver commit and character sharing.
    for balance in (2147483647,2**53,2**63-2):
        sql(f"UPDATE acc_reg_num SET value={balance} WHERE account_id={AID} AND `key`='#BANKVAULT'")
        c=Client();assert c.state['bank']==balance,('registry load',balance,c.state)
        done=c.action(1,1);assert done['bank']==balance+1
        if balance==2**63-2:
            time.sleep(.3);assert c.send(c.bytes(1,1))['result']==8
            time.sleep(.3);assert c.send(c.bytes(2,2147483648-done['wallet']))['result']==8
        c.close();time.sleep(2)
        stored=int(sql(f"SELECT value FROM acc_reg_num WHERE account_id={AID} AND `key`='#BANKVAULT'"))
        assert stored==balance+1,('ordinary registry flush',balance+1,stored)
        c=Client(1);assert c.state['bank']==balance+1,('sibling load',balance+1,c.state)
        assert c.action(2,1)['bank']==balance;c.close();time.sleep(2)
    cases.append('exact 64-bit deposit, withdrawal, relog and sibling balances at 2^31, 2^53 and INT64_MAX')
    sql(f"UPDATE acc_reg_num SET value={2**53} WHERE account_id={AID} AND `key`='#BANKVAULT'")
    c=Client();c.action(3,5);done=c.action(4,5);assert done['bank']==2**53-10000000
    assert sql("SELECT DATA_TYPE FROM information_schema.COLUMNS WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='zenylog' AND COLUMN_NAME='amount'")=='bigint'
    assert sql("SELECT amount FROM zenylog WHERE amount IN (-2505000000,2495000000) ORDER BY amount")=='-2505000000\n2495000000'
    cases.append('multi-billion item exchange costs and proceeds persist exactly in bank and zeny log')
    done=c.action(2,2147483647-done['wallet']);assert done['wallet']==2147483647
    time.sleep(.3);c.world.sendall(struct.pack('<HII',0x9a9,AID,1));drain(c.world,.6)
    assert compact(c.refresh())==compact(done)
    cases.append('full character wallet rejects native withdrawal while bank remains above old ceiling')
    c.close()
    print(json.dumps({'passed':True,'cases':cases,'final_state':compact(done),'synthetic_account':AID,
                      'journal_rows':int(sql('SELECT COUNT(*) FROM pn_bank_commits'))},indent=2))

if __name__=='__main__':main()
