"""Real offline-character collection with SQL fault/retry and wallet boundary checks."""
import json,os,subprocess,time
import wallet64_live_client as bank
def main():
    info=json.loads(subprocess.check_output(['docker','inspect',bank.GAME]))[0]
    assert info['Config']['Labels'].get('pn.wallet64.fixture')=='true'
    assert os.readlink('/proc/self/ns/net')==os.readlink('/proc/'+str(info['State']['Pid'])+'/ns/net')
    assert bank.sql('SELECT DATABASE()')==bank.DATABASE
    assert bank.sql('SELECT COUNT(*) FROM `char` WHERE online<>0')=='0'
    wide=(1<<53)+1
    for slot,cid,amount,online in ((1,99000013,wide,0),(2,99000014,(1<<63)-1,0),(3,99000015,123,1),(4,99000016,1,0)):
        bank.sql(f"INSERT INTO `char`(char_id,account_id,char_num,name,zeny,online) VALUES({cid},99000011,{slot},'SweepFixture{slot}',{amount},{online})")
    a=bank.Client(account_id=99000011,character_id=99000012,username=b'widefixturea',password=b'wide-fixture-only')
    try:
        before=a.refresh();other=bank.sql('SELECT zeny FROM `char` WHERE char_id=99000022')
        bank.sql("CREATE TRIGGER sweep_commit_fault BEFORE INSERT ON pn_bank_sweep_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='intentional isolated sweep fault'")
        request=a.bytes(11,0,trade_id=0,revision=0)
        pending=a.send(request);assert pending['result']==1,pending
        time.sleep(1.3)
        assert bank.sql('SELECT zeny FROM `char` WHERE char_id=99000013')==str(wide)
        assert bank.sql("SELECT value FROM acc_reg_num WHERE account_id=99000011 AND `key`='#BANKVAULT'")==str(before['bank'])
        bank.sql('DROP TRIGGER sweep_commit_fault')
        done=a.wait();assert done['bank']==before['bank']+wide+1 and done['wallet']==before['wallet'],done
        assert bank.sql('SELECT zeny FROM `char` WHERE char_id IN (99000013,99000016) ORDER BY char_id')=='0\n0'
        assert bank.sql('SELECT zeny FROM `char` WHERE char_id=99000014')==str((1<<63)-1)
        assert bank.sql('SELECT zeny FROM `char` WHERE char_id=99000015')=='123'
        assert bank.sql('SELECT zeny FROM `char` WHERE char_id=99000022')==other
        assert a.send(request)['bank']==done['bank']
        assert bank.sql('SELECT collected,skipped FROM pn_bank_sweep_commits')=='2\t3'
        time.sleep(.3);again=a.action(11,0);assert again['bank']==done['bank']
        a.close();a=None;time.sleep(1)
        assert bank.sql("SELECT value FROM acc_reg_num WHERE account_id=99000011 AND `key`='#BANKVAULT'")==str(done['bank'])
        print(json.dumps({'passed':True,'production_database_accessed':False,'cases':['exact offline >2^53 aggregation, current/online/overflow/other-account wallets untouched','receipt-insert SQL fault rolls back all wallets and bank; retry and duplicate are idempotent','empty second sweep and logout preserve all currency']},indent=2))
    finally:
        if a:a.close()
        bank.sql('UPDATE `char` SET online=0 WHERE char_id=99000015')
if __name__=='__main__':main()
