"""Authenticated atomic mail, SQL fault/retry, replay and exact RODEX claim."""
import json,os,socket,struct,subprocess,time
import wallet64_live_client as bank

MAGIC=0x314c5a50
def packet(c,action,sequence=0,amount=0,total=0,recipient=b'widefixtureb',title=b'Atomic wide mail',body=b'Isolated database fixture'):
    return struct.pack('<IHHIIIIQQQIIqq24s40s500s',MAGIC,1,636,c.aid,c.cid,c.key1,c.key2,*c.nonce,sequence,action,0,amount,total,recipient,title,body)
def read(s):
    raw=bank.exact(s,96)
    assert struct.unpack_from('<IHH',raw)==(MAGIC,1,96)
    values=struct.unpack('<IHHQQQIIqqqqqIIQ',raw)
    return dict(zip(('magic','version','length','noncehi','noncelo','request','result','cid','wallet','amount','fee','total','limit','percent','pending','sequence'),values))
def exchange(s,data):s.sendall(data);return read(s)
def wait(s,c,sequence,result):
    for _ in range(80):
        r=exchange(s,packet(c,0))
        if r['result']==result and not r['pending']:
            assert r['sequence']==sequence,r
            return r
        assert r['result']==1 and r['sequence']==sequence,r
        time.sleep(.1)
    raise AssertionError('Atomic mail acknowledgement timed out')
def main():
    info=json.loads(subprocess.check_output(['docker','inspect',bank.GAME]))[0]
    assert info['Config']['Labels'].get('pn.wallet64.fixture')=='true'
    assert os.readlink('/proc/self/ns/net')==os.readlink('/proc/'+str(info['State']['Pid'])+'/ns/net')
    assert bank.sql('SELECT DATABASE()')==bank.DATABASE
    assert bank.sql('SELECT COUNT(*) FROM `char` WHERE online<>0')=='0'
    bank.sql('UPDATE `char` SET zeny=20000000000000000 WHERE char_id=99000012')
    a=bank.Client(account_id=99000011,character_id=99000012,username=b'widefixturea',password=b'wide-fixture-only')
    b=None;s=socket.create_connection(('127.0.0.1',5121),8);cases=[]
    try:
        amount=(1<<53)+1;before=a.refresh()['wallet']
        quote=exchange(s,packet(a,1,amount=amount))
        total=amount+amount*2//100
        assert quote['result']==0 and quote['total']==total and quote['fee']==amount*2//100,quote
        assert exchange(s,packet(a,1,amount=(1<<63)-1))['result']==3
        assert exchange(s,packet(a,2,1,amount,total-1))['result']==6
        forged=bytearray(packet(a,1,amount=amount));forged[24]^=1
        assert exchange(s,forged)['result']==2
        bank.sql("CREATE TRIGGER mail_commit_fault BEFORE INSERT ON pn_mail_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='intentional isolated mail fault'")
        request=packet(a,2,1,amount,total)
        # Fragment the entire authenticated frame.
        for pos in range(0,len(request),13):s.sendall(request[pos:pos+13])
        pending=read(s);assert pending['result']==1 and pending['pending'],pending
        time.sleep(1.3)
        assert bank.sql("SELECT COUNT(*) FROM mail WHERE title='Atomic wide mail'")=='0'
        assert int(bank.sql('SELECT zeny FROM `char` WHERE char_id=99000012'))==before
        assert a.send(a.bytes(1,1))['result']==4 # economic actions stay locked
        s.close();s=socket.create_connection(('127.0.0.1',5121),8)
        bank.sql('DROP TRIGGER mail_commit_fault')
        done=wait(s,a,1,0)
        assert done['wallet']==before-total and done['amount']==amount and done['total']==total,done
        assert exchange(s,request)['result']==0
        assert bank.sql("SELECT COUNT(*) FROM mail WHERE title='Atomic wide mail'")=='1'
        assert bank.sql('SELECT COUNT(*) FROM pn_mail_commits')=='1'
        assert exchange(s,packet(a,2,1,amount,total,body=b'Changed replay'))['result']==6
        cases.append('exact >2^53 quote and fee, malformed auth/overflow/stale-total refusal, fragmented send, SQL rollback, disconnected companion retry and identical durable receipt')
        b=bank.Client(account_id=99000021,character_id=99000022,username=b'widefixtureb',password=b'wide-fixture-only')
        recipient_before=b.refresh()['wallet']
        mail_id=int(bank.sql("SELECT id FROM mail WHERE title='Atomic wide mail'"))
        b.world.sendall(struct.pack('<HBQ',0x9ea,0,mail_id));bank.drain(b.world)
        for _ in range(2):
            b.world.sendall(struct.pack('<HQB',0x9f1,mail_id,0));bank.drain(b.world,.5)
        assert b.refresh()['wallet']==recipient_before+amount,b.state
        assert bank.sql(f'SELECT zeny FROM mail WHERE id={mail_id}')=='0'
        cases.append('atomic persisted message is claimable through native RODEX exactly once')
        time.sleep(1.1)
        failed=exchange(s,packet(a,2,2,100,102,recipient=b'MissingWideFixture'))
        assert failed['result']==1,failed
        rejected=wait(s,a,2,7)
        assert rejected['wallet']==before-total,rejected
        assert exchange(s,packet(a,2,2,100,102,recipient=b'MissingWideFixture'))['result']==7
        cases.append('nonexistent recipient produces durable failure and exact reserved refund')
        a.close();a=None;b.close();b=None;time.sleep(1)
        assert int(bank.sql('SELECT zeny FROM `char` WHERE char_id=99000012'))==before-total
        assert int(bank.sql('SELECT zeny FROM `char` WHERE char_id=99000022'))==recipient_before+amount
        print(json.dumps({'passed':True,'cases':cases,'production_database_accessed':False},indent=2))
    finally:
        s.close()
        for c in (a,b):
            if c:c.close()
if __name__=='__main__':main()
