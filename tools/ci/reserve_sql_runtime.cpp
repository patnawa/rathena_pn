// Real character-server SQL implementation against the disposable bank_probe DB.
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include "char/char.hpp"
#include "char/inter.hpp"
#include "char/int_storage.hpp"
#include "common/malloc.hpp"
#include "common/sql.hpp"
#include "common/timer.hpp"
#include "custom/bank_commit.hpp"
bool bank_tosql(const pn_bank_commit&,s_storage&);
bool inventory_fromsql(uint32,s_storage*);
static int checks=0;
static void sql(const char* q){assert(Sql_QueryStr(sql_handle,q)==SQL_SUCCESS);}
static std::string result(const char* q){sql(q);std::string out;while(Sql_NextRow(sql_handle)==SQL_SUCCESS){for(unsigned i=0;i<Sql_NumColumns(sql_handle);++i){char* p=nullptr;Sql_GetData(sql_handle,i,&p,nullptr);out+=p?p:"NULL";out+='|';}out+='\n';}Sql_FreeResult(sql_handle);return out;}
static void connect_db(){sql_handle=Sql_Malloc();assert(Sql_Connect(sql_handle,"root","bank-validation-only","bank-20260913-db",3306,"bank_probe")==SQL_SUCCESS);assert(result("SELECT DATABASE()")=="bank_probe|\n");strcpy(schema_config.char_db,"char");strcpy(schema_config.inventory_db,"inventory");strcpy(schema_config.char_reg_num_table,"char_reg_num");}
static std::string snapshot(){return result("SELECT * FROM inventory ORDER BY id")+result("SELECT * FROM char_reg_num ORDER BY char_id,`key`,`index`")+result("SELECT * FROM pn_reserve_commits ORDER BY request_id");}
extern "C" int __wrap_main(int argc,char** argv){
    malloc_init();timer_init();connect_db();
    sql("DELETE FROM inventory");sql("DELETE FROM pn_reserve_commits");sql("DELETE FROM char_reg_num");sql("DELETE FROM `char`");
    sql("INSERT INTO `char` (char_id,account_id,name,zeny) VALUES (99001313,990013,'Reserve fixture',100),(99001314,990014,'Unrelated fixture',200)");
    sql("INSERT INTO char_reg_num(char_id,`key`,`index`,`value`) VALUES (99001313,'RESRVPTS',0,5000),(99001314,'RESRVPTS',0,77)");
    sql("INSERT INTO inventory(id,char_id,nameid,amount,identify,refine,card0) VALUES (1,99001313,1201,1,1,10,4001)");
    s_storage inv{};assert(inventory_fromsql(99001313,&inv));inv.type=TABLE_INVENTORY;
    inv.u.items_inventory[1].nameid=516;inv.u.items_inventory[1].amount=7;inv.u.items_inventory[1].identify=1;
    inv.u.items_inventory[2].nameid=501;inv.u.items_inventory[2].amount=7;inv.u.items_inventory[2].identify=1;
    pn_bank_commit r;r.account_id=990013;r.char_id=99001313;r.nonce_hi=101;r.nonce_lo=202;r.request_id=1;r.action=7;r.amount=100;
    r.reserve_before=5000;r.reserve_after=4900;r.reserve_item[0]=516;r.reserve_quantity[0]=7;r.reserve_item[1]=501;r.reserve_quantity[1]=7;
    auto before=snapshot();
    if(argc>1&&std::string(argv[1])=="crash"){
        sql("CREATE TRIGGER reserve_crash BEFORE INSERT ON pn_reserve_commits FOR EACH ROW DO SLEEP(20)");std::cout<<"CRASH_READY"<<std::endl;
        bool ok=bank_tosql(r,inv);std::cout<<"CRASH_RETURN "<<ok<<std::endl;assert(!ok);return 0;
    }
    for(const char* fault:{
        "CREATE TRIGGER reserve_fault BEFORE INSERT ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='inventory fault'",
        "CREATE TRIGGER reserve_fault BEFORE INSERT ON char_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='points fault'",
        "CREATE TRIGGER reserve_fault BEFORE INSERT ON pn_reserve_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='receipt fault'"}){
        sql(fault);assert(!bank_tosql(r,inv));assert(snapshot()==before);checks+=2;sql("DROP TRIGGER reserve_fault");
    }
    sql("ALTER TABLE char_reg_num ENGINE=MyISAM");assert(!bank_tosql(r,inv));assert(snapshot()==before);checks+=2;sql("ALTER TABLE char_reg_num ENGINE=InnoDB");
    for(int i=0;i<6;++i){auto bad=r;if(i==0)bad.account_id=990014;if(i==1)bad.amount=0;if(i==2)bad.reserve_after=5000;if(i==3)bad.request_id=0;if(i==4)bad.reserve_quantity[0]=30001;if(i==5)bad.reserve_before=99;assert(!bank_tosql(bad,inv));assert(snapshot()==before);checks+=2;}
    assert(bank_tosql(r,inv));++checks;
    assert(result("SELECT value FROM char_reg_num WHERE char_id=99001313 AND `key`='RESRVPTS'")=="4900|\n");++checks;
    assert(result("SELECT value FROM char_reg_num WHERE char_id=99001314 AND `key`='RESRVPTS'")=="77|\n");++checks;
    assert(result("SELECT nameid,amount FROM inventory WHERE nameid IN (516,501) ORDER BY nameid")=="501|7|\n516|7|\n");++checks;
    assert(result("SELECT refine,card0 FROM inventory WHERE id=1")=="10|4001|\n");++checks;
    // Model a newer normal inventory save after a lost acknowledgement.
    sql("UPDATE inventory SET amount=3 WHERE nameid=516");auto committed=snapshot();
    assert(bank_tosql(r,inv));assert(snapshot()==committed);checks+=2;
    Sql_Free(sql_handle);connect_db();assert(bank_tosql(r,inv));assert(snapshot()==committed);checks+=2;
    r.reserve_quantity[0]=8;assert(!bank_tosql(r,inv));assert(snapshot()==committed);checks+=2;
    std::cout<<"RESERVE_SQL_PASS "<<checks<<" checks: atomic inventory/points/receipt, exact retry, reconnect, ownership, engine guard and write-fault rollback\n";
    Sql_Free(sql_handle);sql_handle=nullptr;return 0;
}
