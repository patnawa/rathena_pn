// Links actual character-server objects. Only the disposable bank_probe DB is accepted.
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>
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
static void sql(const char* text) { assert(Sql_QueryStr(sql_handle,text)==SQL_SUCCESS); }
static std::string result(const char* query) {
    sql(query); std::string value; int code;
    while((code=Sql_NextRow(sql_handle))==SQL_SUCCESS) {
        for(unsigned i=0;i<Sql_NumColumns(sql_handle);++i) {
            char* data=nullptr; size_t length=0; assert(Sql_GetData(sql_handle,i,&data,&length)==SQL_SUCCESS);
            value+=data?std::string(data,length):"NULL"; value+='|';
        }
        value+='\n';
    }
    assert(code==SQL_NO_DATA); Sql_FreeResult(sql_handle); return value;
}
static std::string snapshot() {
    return result("SELECT char_id,account_id,zeny FROM `char` ORDER BY char_id")+
        result("SELECT * FROM acc_reg_num ORDER BY account_id,`key`,`index`")+
        result("SELECT * FROM inventory ORDER BY id")+
        result("SELECT account_id,nonce_hi,nonce_lo,request_id,char_id,action,amount,bank_before,bank_after,wallet_before,wallet_after FROM pn_bank_commits ORDER BY request_id");
}
static void unchanged(const std::string& before) { assert(snapshot()==before); ++checks; }
static void connect_db() {
    sql_handle=Sql_Malloc();
    assert(Sql_Connect(sql_handle,"root","bank-validation-only","bank-20260913-db",3306,"bank_probe")==SQL_SUCCESS);
    assert(result("SELECT DATABASE()") == "bank_probe|\n");
    strcpy(schema_config.inventory_db,"inventory"); strcpy(schema_config.char_db,"char"); strcpy(schema_config.acc_reg_num_table,"acc_reg_num");
}
extern "C" int __wrap_main(int argc,char** argv) {
    malloc_init(); timer_init(); connect_db();
    bool crash=argc>1 && std::string(argv[1])=="crash";
    sql("DELETE FROM pn_bank_commits"); sql("DELETE FROM inventory");
    sql("DELETE FROM acc_reg_num"); sql("DELETE FROM `char`");
    sql("INSERT INTO `char` (char_id,account_id,zeny,name) VALUES (99001313,990013,500000000,'Bank fixture'),(99001314,990013,0,'Sibling fixture'),(99001315,990014,0,'Other account')");
    sql("INSERT INTO acc_reg_num(account_id,`key`,`index`,`value`) VALUES (990013,'#BANKVAULT',0,1500000000),(990014,'#BANKVAULT',0,77)");
    sql("INSERT INTO inventory(id,char_id,nameid,amount,identify,refine,bound,unique_id,card0) VALUES (1,99001313,501,2,1,0,0,123456789,0),(2,99001313,1201,1,1,10,2,987654321,4001)");
    s_storage inventory{}; assert(inventory_fromsql(99001313,&inventory));
    inventory.type=TABLE_INVENTORY;
    inventory.u.items_inventory[2].nameid=6024; inventory.u.items_inventory[2].amount=1; inventory.u.items_inventory[2].identify=1;
    inventory.amount=3;
    pn_bank_commit request;
    request.account_id=990013; request.char_id=99001313; request.nonce_hi=111; request.nonce_lo=222; request.request_id=1;
    request.action=pn_bank::BuyDiamond; request.amount=1;
    request.bank_before=1500000000; request.bank_after=999000000; request.wallet_before=request.wallet_after=500000000;
    auto cached=std::make_shared<mmo_charstatus>(); cached->zeny=500000000; char_get_chardb()[request.char_id]=cached;
    auto original=snapshot();
    if(crash) {
        sql("CREATE TRIGGER bank_crash BEFORE INSERT ON pn_bank_commits FOR EACH ROW DO SLEEP(20)");
        std::cout<<"CRASH_READY"<<std::endl;
        bool committed=bank_tosql(request,inventory);
        std::cout<<"CRASH_RETURN "<<committed<<std::endl;
        assert(!committed); return 0;
    }
    for(const char* failure : {
        "CREATE TRIGGER bank_fault BEFORE INSERT ON inventory FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='inventory fault'",
        "CREATE TRIGGER bank_fault BEFORE UPDATE ON `char` FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='wallet fault'",
        "CREATE TRIGGER bank_fault BEFORE INSERT ON acc_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='registry fault'",
        "CREATE TRIGGER bank_fault BEFORE INSERT ON pn_bank_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='journal fault'"}) {
        sql(failure); assert(!bank_tosql(request,inventory)); ++checks;
        unchanged(original); assert(cached->zeny==500000000); ++checks;
        sql("DROP TRIGGER bank_fault");
    }
    sql("ALTER TABLE `char` ENGINE=MyISAM"); assert(!bank_tosql(request,inventory)); ++checks; unchanged(original);
    sql("ALTER TABLE `char` ENGINE=InnoDB");
    sql("ALTER TABLE acc_reg_num ENGINE=MyISAM"); assert(!bank_tosql(request,inventory)); ++checks; unchanged(original);
    sql("ALTER TABLE acc_reg_num ENGINE=InnoDB");
    for(int bad=0;bad<7;++bad) {
        auto invalid=request;
        if(bad==0) invalid.account_id=990014;
        if(bad==1) invalid.char_id=123;
        if(bad==2) invalid.bank_after=-1;
        if(bad==3) invalid.wallet_after=int64_t(MAX_ZENY)+1;
        if(bad==4) invalid.wallet_after=-1;
        if(bad==5) invalid.request_id=0;
        if(bad==6) invalid.nonce_hi=invalid.nonce_lo=0;
        assert(!bank_tosql(invalid,inventory)); ++checks; unchanged(original);
    }
    // The SQL boundary must reject inconsistent NEW financial plans, even
    // when every individual balance is within its permitted numeric range.
    for(int bad=0;bad<10;++bad) {
        auto invalid=request;
        if(bad==0) invalid.bank_after++;
        if(bad==1) invalid.bank_after--;
        if(bad==2) invalid.wallet_after--;
        if(bad==3) invalid.bank_before=-1;
        if(bad==4) invalid.wallet_before=-1;
        if(bad==5) invalid.wallet_before=int64_t(MAX_ZENY)+1;
        if(bad==6) invalid.amount=4; // Claimed purchase exceeds pre-transaction bank funds.
        if(bad==7) { invalid.action=pn_bank::Deposit;invalid.amount=500000001; }
        if(bad==8) { invalid.action=pn_bank::Withdraw;invalid.amount=1500000001; }
        if(bad==9) { invalid.action=pn_bank::SellDiamond;invalid.bank_before=INT64_MAX; }
        assert(!bank_tosql(invalid,inventory)); ++checks; unchanged(original);
    }
    assert(bank_tosql(request,inventory)); ++checks;
    assert(result("SELECT value FROM acc_reg_num WHERE account_id=990013 AND `key`='#BANKVAULT'")=="999000000|\n"); ++checks;
    assert(result("SELECT value FROM acc_reg_num WHERE account_id=990014 AND `key`='#BANKVAULT'")=="77|\n"); ++checks;
    assert(result("SELECT amount FROM inventory WHERE nameid=6024")=="1|\n"); ++checks;
    assert(result("SELECT refine,bound,unique_id,card0 FROM inventory WHERE id=2")=="10|2|987654321|4001|\n"); ++checks;
    auto committed=snapshot();
    // Lost acknowledgement / reconnect: the old snapshot must not be reapplied.
    inventory.u.items_inventory[2].amount=9; request.bank_after=1; request.wallet_after=7; cached->zeny=333;
    assert(bank_tosql(request,inventory)); ++checks; unchanged(committed); assert(cached->zeny==333); ++checks;
    Sql_Free(sql_handle); connect_db();
    assert(bank_tosql(request,inventory)); ++checks; unchanged(committed);
    for(int bad=0;bad<2;++bad) {
        auto invalid=request;
        if(bad==0) invalid.bank_before++;
        else invalid.wallet_before++;
        assert(!bank_tosql(invalid,inventory)); ++checks; unchanged(committed);
    }
    request.amount=2; assert(!bank_tosql(request,inventory)); ++checks; unchanged(committed);
    // A new operation is committed and the char-cache baseline advances with it.
    request.request_id=2; request.action=pn_bank::Deposit; request.amount=100;
    request.bank_before=999000000; request.bank_after=999000100; request.wallet_before=500000000; request.wallet_after=499999900;
    inventory.u.items_inventory[2].amount=1;
    assert(bank_tosql(request,inventory) && cached->zeny==499999900); ++checks;
    assert(result("SELECT COUNT(*) FROM pn_bank_commits")=="2|\n"); ++checks;
    // SQL and the journal retain all 63 value bits across commit and reconnect.
    request.request_id=3;request.bank_before=INT64_MAX-1;request.bank_after=INT64_MAX;
    request.amount=1;request.wallet_before=499999900;request.wallet_after=499999899;
    sql("UPDATE acc_reg_num SET value=9223372036854775806 WHERE account_id=990013 AND `key`='#BANKVAULT'");
    assert(bank_tosql(request,inventory) && cached->zeny==499999899);++checks;
    assert(result("SELECT value FROM acc_reg_num WHERE account_id=990013 AND `key`='#BANKVAULT'")=="9223372036854775807|\n");++checks;
    assert(result("SELECT bank_before,bank_after FROM pn_bank_commits WHERE request_id=3")=="9223372036854775806|9223372036854775807|\n");++checks;
    committed=snapshot();Sql_Free(sql_handle);connect_db();
    request.bank_after=1;assert(bank_tosql(request,inventory));++checks;unchanged(committed);
    // Retry snapshots may include legitimate positive wallet rewards received
    // after the map's preflight. Validate the transfer without deleting those.
    request.request_id=4;request.action=pn_bank::Withdraw;request.amount=100;
    request.bank_before=INT64_MAX;request.bank_after=INT64_MAX-100;
    request.wallet_before=499999899;request.wallet_after=500000076; // +100 and +77 reward
    assert(bank_tosql(request,inventory) && cached->zeny==500000076);++checks;
    assert(result("SELECT zeny FROM `char` WHERE char_id=99001313")=="500000076|\n");++checks;
    committed=snapshot();request.wallet_after+=88;
    assert(bank_tosql(request,inventory));++checks;unchanged(committed);
    std::cout<<"BANK_SQL_PASS "<<checks<<" checks: actual InnoDB atomicity, failures at all four writes, journal retry, reconnect, ownership, caps, cache synchronization and unrelated item preservation\n";
    Sql_Free(sql_handle); sql_handle=nullptr; return 0;
}
