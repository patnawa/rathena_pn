"""Real MariaDB transaction tests, restricted to a disposable fixture database."""
import importlib.util
import os
from pathlib import Path
import pymysql

module_path = Path(__file__).resolve().parents[1] / 'admin/retire_zeny_tokens.py'
spec = importlib.util.spec_from_file_location('retire', module_path)
retire = importlib.util.module_from_spec(spec)
spec.loader.exec_module(retire)
db = pymysql.connect(host=os.environ.get('PN_TOKEN_TEST_HOST', 'pn-zeny-migration-db'), user='root',
                     password='disposable-fixture-only', database='pn_zeny_migration_test', autocommit=True)
q = lambda sql, args=(): retire.query(db, sql, args)
x = lambda sql, args=(): retire.execute(db, sql, args)
assert q('SELECT DATABASE()') == (('pn_zeny_migration_test',),)
for (table,) in q('SHOW TABLES'):
    assert table.replace('_', '').isalnum()
    x(f'DROP TABLE `{table}`')
x('CREATE TABLE vendings(id INT PRIMARY KEY) ENGINE=InnoDB')
x('CREATE TABLE buyingstores(id INT PRIMARY KEY) ENGINE=InnoDB')
x('CREATE TABLE vending_items(vending_id INT,`index` INT,cartinventory_id BIGINT,PRIMARY KEY(vending_id,`index`)) ENGINE=InnoDB')
x('CREATE TABLE buyingstore_items(buyingstore_id INT,`index` INT,item_id INT,PRIMARY KEY(buyingstore_id,`index`)) ENGINE=InnoDB')
x('INSERT INTO vendings VALUES (1),(2),(3)')
x('INSERT INTO buyingstores VALUES (1),(2),(3)')
x('INSERT INTO vending_items VALUES (1,0,1),(2,0,1),(2,1,2),(3,0,2)')
x('INSERT INTO buyingstore_items VALUES (1,0,6024),(2,0,12781),(2,1,501),(3,0,501)')
x('CREATE TABLE login(account_id INT PRIMARY KEY) ENGINE=InnoDB')
x('CREATE TABLE `char`(char_id INT PRIMARY KEY,account_id INT NOT NULL) ENGINE=InnoDB')
x('CREATE TABLE guild(guild_id INT PRIMARY KEY,char_id INT NOT NULL) ENGINE=InnoDB')
x('CREATE TABLE mail(id BIGINT PRIMARY KEY,dest_id INT NOT NULL) ENGINE=InnoDB')
x('CREATE TABLE auction(auction_id INT PRIMARY KEY,nameid INT NOT NULL) ENGINE=InnoDB')
x('CREATE TABLE acc_reg_num(account_id INT NOT NULL,`key` VARCHAR(32),`index` INT,value BIGINT,PRIMARY KEY(account_id,`key`,`index`)) ENGINE=InnoDB')
for table in retire.SOURCES:
    owner = 'char_id' if table in ('inventory','cart_inventory') else 'guild_id' if table == 'guild_storage' else 'account_id'
    suffix = ',`index` INT NOT NULL,PRIMARY KEY(id,`index`)' if table == 'mail_attachments' else ',PRIMARY KEY(id)'
    x(f'CREATE TABLE `{table}`(id BIGINT NOT NULL,{owner} INT NOT NULL,nameid INT NOT NULL,amount INT NOT NULL{suffix}) ENGINE=InnoDB')
x('INSERT INTO login VALUES (100),(200)')
x('INSERT INTO `char` VALUES (10,100),(20,200)')
x('INSERT INTO guild VALUES (7,20)')
x('INSERT INTO mail VALUES (50,20)')
x("INSERT INTO acc_reg_num VALUES (100,'#BANKVAULT',0,9007199254740993)")
for table in retire.SOURCES:
    if table == 'mail_attachments':
        x('INSERT INTO mail_attachments VALUES (50,0,12781,3,0)')
    else:
        owner = 10 if table in ('inventory','cart_inventory') else 7 if table == 'guild_storage' else 100
        x(f'INSERT INTO `{table}` VALUES (1,%s,6024,2),(2,%s,501,17)', (owner,owner))

def snapshot():
    return {table: q(f'SELECT * FROM `{table}` ORDER BY 1') for table in list(retire.SOURCES)+['acc_reg_num','vendings','vending_items','buyingstores','buyingstore_items']}

def fails(call):
    before = snapshot()
    try:
        call()
    except (ValueError,pymysql.MySQLError):
        assert snapshot() == before
    else:
        raise AssertionError('Expected fail-closed transaction')

before = snapshot()
plan = retire.migrate(db)
assert snapshot() == before and len(plan['rows']) == len(retire.SOURCES)
expected = 9007199254740993 + 998000000 * 9 # inventory, cart and seven storages
assert next(a for a in plan['accounts'] if a['account']==100)['after'] == expected
assert next(a for a in plan['accounts'] if a['account']==200)['credit'] == 998000000 + 2994000
fails(lambda: retire.migrate(db, apply=True, expected_hash=plan['sha256']))
offline_checks = []
offline = lambda: offline_checks.append(True)
fails(lambda: retire.migrate(db, apply=True, expected_hash='0'*64, assert_offline=offline))
x("CREATE TRIGGER fail_retirement BEFORE DELETE ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='fixture write failure'")
fails(lambda: retire.migrate(db, apply=True, expected_hash=plan['sha256'], assert_offline=offline))
x('DROP TRIGGER fail_retirement')
x("CREATE TRIGGER fail_listing BEFORE DELETE ON vending_items FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='fixture listing failure'")
fails(lambda: retire.migrate(db, apply=True, expected_hash=plan['sha256'], assert_offline=offline))
x('DROP TRIGGER fail_listing')
x('INSERT INTO auction VALUES (1,6024)')
fails(lambda: retire.migrate(db))
x('DELETE FROM auction')
x('UPDATE mail SET dest_id=999')
fails(lambda: retire.migrate(db))
x('UPDATE mail SET dest_id=20')
x("UPDATE acc_reg_num SET value=9223372036854775807")
fails(lambda: retire.migrate(db))
x("UPDATE acc_reg_num SET value=9007199254740993")
x('CREATE TABLE surprise_items(id INT,nameid INT) ENGINE=InnoDB')
x('INSERT INTO surprise_items VALUES (1,6024)')
fails(lambda: retire.migrate(db))
x('DROP TABLE surprise_items')
result = retire.migrate(db, apply=True, expected_hash=plan['sha256'], assert_offline=offline)
assert result['status'] == 'committed'
assert q('SELECT * FROM vendings ORDER BY id') == ((2,),(3,))
assert q('SELECT * FROM buyingstores ORDER BY id') == ((2,),(3,))
assert q('SELECT * FROM vending_items ORDER BY vending_id,`index`') == ((2,1,2),(3,0,2))
assert q('SELECT * FROM buyingstore_items ORDER BY buyingstore_id,`index`') == ((2,1,501),(3,0,501))
assert q("SELECT value FROM acc_reg_num WHERE account_id=100")[0][0] == expected
for table in retire.SOURCES:
    assert not q(f'SELECT 1 FROM `{table}` WHERE nameid IN (6024,12781)')
    if table != 'mail_attachments':
        assert q(f'SELECT amount FROM `{table}` WHERE nameid=501') == ((17,),)
committed = snapshot()
assert retire.migrate(db, apply=True, expected_hash=plan['sha256'], assert_offline=offline)['status']=='already_committed'
assert snapshot()==committed and len(offline_checks)>=5
print('PASS: real InnoDB token conversion, all owned stores, exact >2^53 balances, guild/mail ownership, dry-run, overflow/orphan/auction/unmapped rejection, fault rollback and idempotent retry')
