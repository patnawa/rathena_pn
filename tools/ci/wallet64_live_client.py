"""Actual v3 wallet/trade requests. Must run in the labelled isolated game namespace."""
from pathlib import Path
import json
import os
import socket
import struct
import subprocess
import time
from bank_live_client import Client as LegacyHandshake, exact, drain

GAME = 'pn-muhro-last10-20260928-game'
DB = 'pn-muhro-last10-20260928-db'
DATABASE = 'ragnarok_ci'
LIMIT = (1 << 63) - 1

def sql(query):
    return subprocess.check_output(['docker', 'exec', '-i', '-e', 'MYSQL_PWD=project-validation-only', DB,
        'mariadb', '-uroot', '--batch', '--raw', '-N', DATABASE], input=query, text=True).strip()

class Client(LegacyHandshake):
    def bytes(self, action, amount, sequence=None, *, trade_id=None, revision=None):
        if sequence is None:
            self.sequence += 1
            sequence = self.sequence
        state = getattr(self, 'state', {})
        tid = state.get('trade_id', 0) if trade_id is None else trade_id
        rev = state.get('revision', 0) if revision is None else revision
        if action < 7:
            tid = rev = 0
        return struct.pack('<IHHIIIIQQQqIIQQ', 0x314b4250, 3, 80, self.aid, self.cid, self.key1, self.key2,
                           *self.nonce, sequence, amount, action, 0, tid, rev)
    def send(self, data, fragment=False):
        if fragment:
            for offset in range(0, len(data), 7):
                self.companion.sendall(data[offset:offset+7])
        else:
            self.companion.sendall(data)
        state = self.receive()
        while state['flags']:
            state = self.receive()
        self.state = state
        return state
    def receive(self):
        raw = exact(self.companion, 208)
        assert struct.unpack_from('<IHH', raw) == (0x314b4250, 3, 208)
        assert struct.unpack_from('<qq', raw, 56) == (LIMIT, LIMIT)
        result = struct.unpack_from('<I', raw, 32)[0]
        assert struct.unpack_from('<I', raw, 128)[0] == self.cid or result == 2
        return {'nonce': struct.unpack_from('<QQ', raw, 8), 'sequence': struct.unpack_from('<Q', raw, 24)[0],
                'result': result, 'flags': struct.unpack_from('<I', raw, 36)[0],
                'bank': struct.unpack_from('<q', raw, 40)[0], 'wallet': struct.unpack_from('<q', raw, 48)[0],
                'partner': struct.unpack_from('<I', raw, 140)[0],
                'name': raw[144:168].split(b'\0', 1)[0].decode(),
                'own_offer': struct.unpack_from('<q', raw, 168)[0],
                'partner_offer': struct.unpack_from('<q', raw, 176)[0],
                'own_state': struct.unpack_from('<I', raw, 184)[0],
                'partner_state': struct.unpack_from('<I', raw, 188)[0],
                'trade_id': struct.unpack_from('<Q', raw, 192)[0], 'revision': struct.unpack_from('<Q', raw, 200)[0]}
    def trade(self, action, amount=0, **kwargs):
        state = self.send(self.bytes(action, amount, **kwargs))
        if state['result']==1: state=self.wait()
        assert state['result'] == 0, state
        return state

def main():
    inspect = json.loads(subprocess.check_output(['docker','inspect',GAME]))[0]
    assert inspect['Config']['Labels'].get('pn.wallet64.fixture') == 'true'
    assert os.readlink('/proc/self/ns/net') == os.readlink('/proc/'+str(inspect['State']['Pid'])+'/ns/net')
    assert sql('SELECT DATABASE()') == DATABASE
    assert sql('SELECT COUNT(*) FROM `char` WHERE char_id NOT IN (99000012,99000022)') == '0'
    a = Client(account_id=99000011,character_id=99000012,username=b'widefixturea',password=b'wide-fixture-only')
    b = Client(account_id=99000021,character_id=99000022,username=b'widefixtureb',password=b'wide-fixture-only')
    cases=[]
    try:
        a0, b0 = a.refresh(), b.refresh()
        assert a0['wallet'] == 20000000000000000 and b0['wallet'] == 3000000000
        wide = (1 << 53) + 1
        before=a0['bank']
        deposited=a.action(1,wide)
        assert deposited['bank']==before+wide and deposited['wallet']==a0['wallet']-wide
        replay=a.send(a.last);assert replay['result']==0 and replay['wallet']==deposited['wallet']
        withdrawn=a.action(2,wide)
        assert withdrawn['wallet']==a0['wallet'] and withdrawn['bank']==before
        for action in (3,4,5,6):
            time.sleep(.3)
            rejected=a.send(a.bytes(action,1))
            assert rejected['result']==3,rejected
        cases.append('exact >2^53 deposit/withdraw, durable receipt and replay; legacy exchanges refused')
        a.world.sendall(struct.pack('<HI',0xe4,b.aid));request_wire=drain(a.world);peer_wire=drain(b.world)
        b.world.sendall(struct.pack('<HB',0xe6,3));accept_wire=drain(a.world);peer_accept=drain(b.world)
        sa,sb=a.refresh(),b.refresh()
        assert sa['trade_id'] and sa['trade_id']==sb['trade_id'] and sa['partner']==b.cid and sb['partner']==a.cid,(sa,sb,request_wire.hex(),peer_wire.hex(),accept_wire.hex(),peer_accept.hex())
        old_revision=sa['revision']
        a.trade(7,wide);b.refresh()
        assert b.state['partner_offer']==wide
        assert b.send(b.bytes(8,0,revision=old_revision))['result']==10
        b.refresh();b.trade(7,123456789);a.refresh()
        a.trade(8);b.refresh();b.trade(8);a.refresh()
        assert a.state['own_state']==a.state['partner_state']==1
        # Stock confirmation cannot approve the zero placeholder in its narrow UI.
        a.world.sendall(struct.pack('<H',0xef));drain(a.world);a.refresh()
        assert a.state['own_state']==1 and a.state['wallet']==a0['wallet']
        a.trade(9);b.refresh();assert b.state['partner_state']==2
        sql("CREATE TRIGGER pair_commit_fault BEFORE INSERT ON pn_pair_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='intentional isolated pair fault'")
        final_commit=b.bytes(9,0)
        pending=b.send(final_commit);assert pending['result']==1,pending
        time.sleep(1.3)
        assert sql('SELECT zeny FROM `char` WHERE char_id=99000012')==str(a0['wallet'])
        assert sql('SELECT zeny FROM `char` WHERE char_id=99000022')==str(b0['wallet'])
        sql('DROP TRIGGER pair_commit_fault');b.wait();a.refresh();b.refresh()
        assert b.send(final_commit)['result']==0 and sql('SELECT COUNT(*) FROM pn_pair_commits')=='1'
        assert not a.state['trade_id'] and not b.state['trade_id']
        assert a.state['wallet']==a0['wallet']-wide+123456789
        assert b.state['wallet']==b0['wallet']+wide-123456789
        assert a.state['wallet']+b.state['wallet']==a0['wallet']+b0['wallet']
        cases.append('native trade initiation, exact wide bilateral offers, stale revision refusal, stock confirmation blocked, explicit companion commitment conserves Zeny')
        # RODEX already transports a uint64 field; exercise its widened server path.
        before_a,before_b=a.state['wallet'],b.state['wallet']
        mail_amount=5000000001
        b.world.sendall(struct.pack('<H24s',0xa08,b'widefixturea'));drain(b.world)
        title=b'Wide Zeny fixture\0';body=b'Exact integer transfer\0'
        b.world.sendall(struct.pack('<HH24s24sQHHI',0xa6e,68+len(title)+len(body),b'widefixturea',b'widefixtureb',mail_amount,len(title),len(body),a.cid)+title+body)
        drain(b.world,.8);b.world.sendall(struct.pack('<H',0xa03));drain(b.world)
        rows=sql("SELECT id,zeny FROM mail WHERE dest_id=99000012 AND title='Wide Zeny fixture'")
        assert rows, 'RODEX send did not persist mail'
        mail_id,saved_amount=map(int,rows.split('\t'))
        assert saved_amount==mail_amount,rows
        assert b.refresh()['wallet']==before_b-mail_amount-(mail_amount*2//100),b.state
        a.close();time.sleep(2)
        a=Client(account_id=99000011,character_id=99000012,username=b'widefixturea',password=b'wide-fixture-only')
        a.world.sendall(struct.pack('<HBQ',0x9ea,0,mail_id));drain(a.world)
        a.world.sendall(struct.pack('<HQB',0x9f1,mail_id,0));drain(a.world,.8)
        assert a.refresh()['wallet']==before_a+mail_amount,a.state
        a.world.sendall(struct.pack('<HQB',0x9f1,mail_id,0));drain(a.world)
        assert a.refresh()['wallet']==before_a+mail_amount and sql(f'SELECT zeny FROM mail WHERE id={mail_id}')=='0'
        cases.append('RODEX >32-bit send, exact 2% fee, BIGINT persistence, recipient relog and duplicate claim protection')
        expected={a.cid:a.state['wallet'],b.cid:b.state['wallet']}
        a.close();b.close();a=b=None
        for _ in range(80):
            saved={int(cid):int(zeny) for cid,zeny in (row.split('\t') for row in sql('SELECT char_id,zeny FROM `char` ORDER BY char_id').splitlines())}
            if saved==expected:break
            time.sleep(.1)
        assert saved==expected,(saved,expected)
        cases.append('both full-width wallets persisted exactly on logout')
        print(json.dumps({'passed':True,'production_database_accessed':False,'cases':cases},indent=2))
    finally:
        for client in (a,b):
            if client:
                try:client.close()
                except OSError:pass

if __name__=='__main__':main()
