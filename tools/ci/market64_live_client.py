"""Actual PMK1/native-draft test; only the labelled disposable fixture is accepted."""
import json, os, socket, struct, subprocess, time
from wallet64_live_client import Client, GAME, DB, DATABASE, sql, exact, drain

class Market:
    def __init__(self, client):
        self.client=client; self.sequence=0
        self.socket=socket.create_connection(('127.0.0.1',5121),8)
    def call(self,action,kind=0,*,quote=None,row=None,budget=0,quantity=0,cursor=0,min_price=0,max_price=0,item_id=0,target=0,replay=None):
        c=self.client; self.sequence+=1
        quote=quote or {}; row=row or {}
        packet=replay or struct.pack('<IHHIIIIQQQIIIIIIIIQqqqqQ',0x314b4d50,1,128,c.aid,c.cid,c.key1,c.key2,*c.nonce,self.sequence,
            action,kind,row.get('index',0),quantity,target or quote.get('owner',0),quote.get('shop',0),cursor,item_id or row.get('item',0),
            quote.get('revision',0),row.get('price',0),budget,min_price,max_price,row.get('unique',0))
        self.last=packet
        # Exercise TCP fragmentation on the real map-server companion parser.
        self.socket.sendall(packet[:11]);self.socket.sendall(packet[11:])
        raw=exact(self.socket,2760)
        assert struct.unpack_from('<IHH',raw)==(0x314b4d50,1,2760)
        assert struct.unpack_from('<QQ',raw,8)==c.nonce
        fields=struct.unpack_from('<9I',raw,32)
        result=dict(zip(('result','flags','count','cursor','more','kind','owner','char','shop'),fields))
        result.update(revision=struct.unpack_from('<Q',raw,68)[0],wallet=struct.unpack_from('<q',raw,76)[0],budget=struct.unpack_from('<q',raw,84)[0])
        result['rows']=[]
        for i in range(result['count']):
            offset=200+i*128
            index,item,amount=struct.unpack_from('<III',raw,offset)
            price,unique=struct.unpack_from('<qQ',raw,offset+16)
            result['rows'].append(dict(index=index,item=item,amount=amount,price=price,unique=unique))
        return result

def chat(client, username, command):
    text=username+b' : '+command+b'\0'
    client.world.sendall(struct.pack('<HH',0xf3,len(text)+4)+text)
    return drain(client.world,.5)

def main():
    info=json.loads(subprocess.check_output(['docker','inspect',GAME]))[0]
    assert info['Config']['Labels'].get('pn.wallet64.fixture')=='true'
    assert os.readlink('/proc/self/ns/net')==os.readlink('/proc/'+str(info['State']['Pid'])+'/ns/net')
    assert sql('SELECT DATABASE()')==DATABASE
    a=Client(account_id=99000011,character_id=99000012,username=b'widefixturea',password=b'wide-fixture-only')
    b=Client(account_id=99000021,character_id=99000022,username=b'widefixtureb',password=b'wide-fixture-only')
    ma,mb=Market(a),Market(b);cases=[]
    try:
        before_a=a.refresh()['wallet'];before_b=b.refresh()['wallet'];bank_before=a.state['bank']
        chat(a,b'widefixturea',b'@cart 1')
        a.world.sendall(struct.pack('<HHHI',0x438,10,41,a.aid));drain(a.world,.5)
        a.world.sendall(struct.pack('<HH80sBHHI',0x1b2,93,b'Wide vendor fixture',1,2,2,1));wire=drain(a.world,.5)
        draft=ma.call(0);assert draft['result']==0 and draft['flags']==1 and draft['rows'],(draft,wire.hex())
        assert mb.call(2)['result']==4,'Unpublished drafts must not appear in search'
        wide=(1<<53)+1
        priced=ma.call(3,quote=draft,row=draft['rows'][0],budget=wide)
        assert priced['result']==0 and priced['rows'][0]['price']==wide,priced
        published=ma.call(7,quote=priced);assert published['result']==0 and published['flags']==2,published
        immutable=ma.call(3,quote=published,row=published['rows'][0],budget=wide+1)
        assert immutable['result']==3,immutable
        quote=mb.call(2,min_price=wide,max_price=wide,item_id=501)
        assert quote['owner']==a.aid and quote['rows'][0]['price']==wide,quote
        assert mb.call(2,min_price=wide+1)['result']==4
        b.world.sendall(struct.pack('<HHIIHH',0x801,16,a.aid,quote['shop'],1,2));drain(b.world)
        assert b.refresh()['wallet']==before_b,'Narrow native purchase must not accept a wide price'
        # Force a mid-transaction bank write failure. The buyer wallet/items,
        # seller cart/bank and shop listing must all remain at their old SQL state.
        db_before=sql("SELECT CONCAT(c.zeny,':',COALESCE((SELECT SUM(amount) FROM inventory WHERE char_id=c.char_id AND nameid=501),0)) FROM `char` c WHERE char_id=99000022")
        cart_before=sql("SELECT amount FROM cart_inventory WHERE char_id=99000012 AND nameid=501")
        listing_before=sql("SELECT amount FROM vending_items WHERE vending_id="+str(quote['shop']))
        receipts_before=int(sql('SELECT COUNT(*) FROM pn_pair_commits'))
        sql("CREATE TRIGGER fixture_pair_fault BEFORE INSERT ON acc_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='fixture pair bank fault'")
        purchased=mb.call(5,quote=quote,row=quote['rows'][0],quantity=1)
        last=mb.last
        assert purchased['result']==9 and purchased['wallet']==before_b-wide,purchased
        time.sleep(.3)
        assert sql("SELECT CONCAT(c.zeny,':',COALESCE((SELECT SUM(amount) FROM inventory WHERE char_id=c.char_id AND nameid=501),0)) FROM `char` c WHERE char_id=99000022")==db_before
        assert sql("SELECT amount FROM cart_inventory WHERE char_id=99000012 AND nameid=501")==cart_before
        assert sql("SELECT amount FROM vending_items WHERE vending_id="+str(quote['shop']))==listing_before
        assert int(sql('SELECT COUNT(*) FROM pn_pair_commits'))==receipts_before
        sql('DROP TRIGGER fixture_pair_fault')
        for _ in range(100):
            if int(sql('SELECT COUNT(*) FROM pn_pair_commits'))==receipts_before+1: break
            time.sleep(.1)
        else: raise AssertionError('Vending receipt did not commit after retry')
        time.sleep(.15)
        net=wide*9750//10000 # Fixture config: 2.5% vending tax, exact integer cents.
        assert a.refresh()['wallet']==before_a and a.state['bank']==bank_before+net,a.state
        assert int(sql("SELECT value FROM acc_reg_num WHERE account_id=99000011 AND `key`='#BANKVAULT' AND `index`=0"))==bank_before+net
        assert int(sql("SELECT zeny FROM `char` WHERE char_id=99000022"))==before_b-wide
        assert mb.call(5,replay=last)['result']==5
        assert b.refresh()['wallet']==before_b-wide
        stale=mb.call(5,quote=quote,row=quote['rows'][0],quantity=1)
        assert stale['result']==5,stale
        cases.append('draft hidden, >2^53 price exact, published quote immutable, full-width filters, native underquoted purchase rejected, atomic buyer debit/items + seller cart/bank + listings, fault rollback/retry, exact tax, replay/stale quote refused')
        own=ma.call(0);assert ma.call(8,quote=own)['result']==0
        b.world.sendall(struct.pack('<HHHI',0x438,1,2535,b.aid));drain(b.world,.5)
        b.world.sendall(struct.pack('<HHIB80sIHI',0x811,99,1000,1,b'Wide buyer fixture',909,2,1));wire=drain(b.world,.5)
        draft=mb.call(0,1);assert draft['flags']==1 and draft['rows'],(draft,wire.hex())
        price=wide+6
        priced=mb.call(3,1,quote=draft,row=draft['rows'][0],budget=price);assert priced['result']==0,priced
        funded=mb.call(4,1,quote=priced,budget=price);assert funded['result']==0 and funded['budget']==price,funded
        published=mb.call(7,1,quote=funded);assert published['flags']==2,published
        quote=ma.call(2,1,min_price=price,max_price=price,item_id=909)
        assert quote['owner']==b.aid and quote['rows'][0]['price']==price,quote
        seller_before=a.refresh()['wallet'];buyer_before=b.refresh()['wallet']
        def buying_snapshot():
            return [sql("SELECT CONCAT(char_id,':',zeny) FROM `char` WHERE char_id IN (99000012,99000022) ORDER BY char_id"),
                    sql("SELECT CONCAT(char_id,':',nameid,':',amount) FROM inventory WHERE char_id IN (99000012,99000022) ORDER BY char_id,id"),
                    sql("SELECT CONCAT(id,':',`limit`) FROM buyingstores ORDER BY id"),
                    sql("SELECT CONCAT(buyingstore_id,':',`index`,':',item_id,':',amount,':',price) FROM buyingstore_items ORDER BY buyingstore_id,`index`")]
        before_sql=buying_snapshot()
        receipts_before=int(sql('SELECT COUNT(*) FROM pn_pair_commits'))
        sql("DELIMITER //\nCREATE TRIGGER fixture_buy_pair_fault BEFORE UPDATE ON `char` FOR EACH ROW BEGIN IF NEW.char_id=99000022 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='fixture buying pair fault'; END IF; END//\nDELIMITER ;")
        sold=ma.call(6,1,quote=quote,row=quote['rows'][0],quantity=1)
        last=ma.last
        assert sold['result']==9 and sold['wallet']==seller_before+price,sold
        time.sleep(.3)
        assert buying_snapshot()==before_sql
        assert int(sql('SELECT COUNT(*) FROM pn_pair_commits'))==receipts_before
        assert ma.call(1,1,quote=quote)['result']==9, 'Pending purchase query must remain Saving'
        sql('DROP TRIGGER fixture_buy_pair_fault')
        for _ in range(100):
            if int(sql('SELECT COUNT(*) FROM pn_pair_commits'))==receipts_before+1: break
            time.sleep(.1)
        else: raise AssertionError('Buying receipt did not commit after retry')
        time.sleep(.15)
        assert b.refresh()['wallet']==buyer_before-price
        assert int(sql("SELECT zeny FROM `char` WHERE char_id=99000012"))==seller_before+price
        assert int(sql("SELECT zeny FROM `char` WHERE char_id=99000022"))==buyer_before-price
        assert not sql("SELECT id FROM buyingstores WHERE id="+str(quote['shop']))
        assert not sql("SELECT buyingstore_id FROM buyingstore_items WHERE buyingstore_id="+str(quote['shop']))
        assert ma.call(6,1,replay=last)['result'] in (4,5)
        assert ma.call(6,1,quote=quote,row=quote['rows'][0],quantity=1)['result'] in (4,5)
        assert a.refresh()['wallet']==seller_before+price
        cases.append('wide buying price and budget, atomic inventories/wallets/order budget, injected second-player SQL failure rolls entire purchase back then retry commits, pending queries remain Saving, exact balances and sold-out cleanup, replay refused')
        print(json.dumps({'passed':True,'production_database_accessed':False,'cases':cases},indent=2))
    finally:
        ma.socket.close();mb.socket.close();a.close();b.close()

if __name__=='__main__':main()
