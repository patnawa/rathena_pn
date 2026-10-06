// Real pair SQL handler and native character inventory writer in disposable MariaDB.
#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <char/char.hpp>
#include <char/inter.hpp>
#include <common/malloc.hpp>
#include <common/socket.hpp>
#include <common/sql.hpp>
#include <common/timer.hpp>
#include <custom/pair_sql.inc>
static unsigned checks=0;
static void query(const std::string& sql){assert(Sql_QueryStr(sql_handle,sql.c_str())==SQL_SUCCESS);}
static std::string rows(const std::string& sql){
 query(sql);std::string out;int result;
 while((result=Sql_NextRow(sql_handle))==SQL_SUCCESS){
  for(unsigned i=0;i<Sql_NumColumns(sql_handle);++i){char* value=nullptr;size_t size=0;assert(Sql_GetData(sql_handle,i,&value,&size)==SQL_SUCCESS);out+=value?std::string(value,size):"NULL";out+='|';}out+='\n';
 }assert(result==SQL_NO_DATA);Sql_FreeResult(sql_handle);return out;
}
static std::string state(){return rows("SELECT char_id,account_id,zeny FROM `char` ORDER BY char_id")+rows("SELECT * FROM acc_reg_num ORDER BY account_id,`key`,`index`")+rows("SELECT * FROM inventory ORDER BY id")+rows("SELECT * FROM cart_inventory ORDER BY id")+rows("SELECT * FROM vending_items ORDER BY vending_id,`index`")+rows("SELECT account_id,nonce_hi,nonce_lo,sequence,HEX(payload) FROM pn_pair_commits ORDER BY account_id,sequence");}
static void connect(){sql_handle=Sql_Malloc();assert(Sql_Connect(sql_handle,"root","market-bank-fixture-only","market-bank-db",3306,"market_bank_probe")==SQL_SUCCESS);assert(rows("SELECT DATABASE() ")=="market_bank_probe|\n");
 strcpy(schema_config.char_db,"char");strcpy(schema_config.inventory_db,"inventory");strcpy(schema_config.cart_db,"cart_inventory");strcpy(schema_config.acc_reg_num_table,"acc_reg_num");}
static pn_pair::Commit seed(int64 price,int64 seller_bank){
 for(const auto* table:{"pn_pair_commits","vending_items","vendings","inventory","cart_inventory","acc_reg_num","char"})query(std::string("DELETE FROM `")+table+"`");
 query("INSERT INTO `char`(char_id,account_id,name,zeny) VALUES(99001313,990013,'Buyer',"+std::to_string(price)+"),(99001414,990014,'Seller',9223372036854775807)");
 query("INSERT INTO acc_reg_num(account_id,`key`,`index`,value) VALUES(990013,'#BANKVAULT',0,9223372036854775807),(990014,'#BANKVAULT',0,"+std::to_string(seller_bank)+")");
 query("INSERT INTO cart_inventory(id,char_id,nameid,amount,identify,unique_id) VALUES(77,99001414,501,2,1,0)");
 query("INSERT INTO vendings(id,account_id,char_id,sex,map,x,y,title,autotrade) VALUES(77,990014,99001414,'M','prontera',1,1,'Wide proof',0)");
 query("INSERT INTO vending_items(vending_id,`index`,cartinventory_id,amount,price) VALUES(77,0,77,"+std::to_string(price>INT64_MAX/2?1:2)+","+std::to_string(price)+")");
 pn_pair::Commit r;r.length=sizeof(r);r.kind=pn_pair::Vending;r.sequence=1;r.vending_id=77;r.listing_count=price>INT64_MAX/2?0:1;r.listings[0]={77,1,price};
 auto& buyer=r.side[0];auto& seller=r.side[1];buyer.account_id=990013;buyer.char_id=99001313;seller.account_id=990014;seller.char_id=99001414;
 for(auto& side:r.side){side.nonce_hi=11;side.nonce_lo=22;side.items[0].nameid=501;side.items[0].amount=1;side.items[0].identify=1;}
 seller.items[0].id=77;buyer.wallet_before=price;buyer.wallet_after=0;buyer.bank_before=buyer.bank_after=INT64_MAX;
 seller.wallet_before=seller.wallet_after=INT64_MAX;seller.bank_before=seller_bank;assert(price<=INT64_MAX-seller_bank);seller.bank_after=seller_bank+price;
 assert(pn_pair::conserved(r));return r;
}
extern "C" int __wrap_main(int,char**){malloc_init();timer_init();connect();
 for(const int64 price:{int64(3000000000LL),int64(9007199254740993LL),int64(INT64_MAX)}){
  auto r=seed(price,INT64_MAX-price);const auto before=state();
  for(const auto* fault:{"CREATE TRIGGER fixture_fault BEFORE INSERT ON acc_reg_num FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='seller bank fault'","CREATE TRIGGER fixture_fault BEFORE DELETE ON vending_items FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='listing fault'","CREATE TRIGGER fixture_fault BEFORE INSERT ON pn_pair_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='receipt fault'"}){
   query(fault);assert(!pn_pair_tosql(r));assert(state()==before);++checks;query("DROP TRIGGER fixture_fault");
  }
  auto invalid=r;invalid.side[1].bank_after=-1;assert(!pn_pair_tosql(invalid)&&state()==before);++checks;
  invalid=r;invalid.side[0].wallet_after=-1;assert(!pn_pair_tosql(invalid)&&state()==before);++checks;
  assert(pn_pair_tosql(r));++checks;
  assert(rows("SELECT zeny FROM `char` ORDER BY char_id")=="0|\n9223372036854775807|\n");++checks;
  assert(rows("SELECT value FROM acc_reg_num WHERE account_id=990014")=="9223372036854775807|\n");++checks;
  assert(rows("SELECT amount,price FROM vending_items")== (r.listing_count ? "1|"+std::to_string(price)+"|\n" : ""));++checks;
  assert(rows("SELECT amount FROM inventory WHERE char_id=99001313")=="1|\n"&&rows("SELECT amount FROM cart_inventory WHERE id=77")=="1|\n");++checks;
  assert(rows("SELECT COUNT(*) FROM pn_pair_commits")=="1|\n");++checks;
  query("UPDATE `char` SET zeny=123 WHERE char_id=99001313");query("UPDATE acc_reg_num SET value=456 WHERE account_id=990014");query("UPDATE inventory SET amount=9 WHERE char_id=99001313");
  const auto newer=state();assert(pn_pair_tosql(r)&&state()==newer);++checks;
  Sql_Free(sql_handle);connect();assert(pn_pair_tosql(r)&&state()==newer);++checks;
  invalid=r;invalid.side[0].items[0].amount++;assert(!pn_pair_tosql(invalid)&&state()==newer);++checks;
  std::cout<<"MARKET_BANK_SQL_CASE price="<<price<<" seller_bank_after=9223372036854775807 rollback/retry/reconnect=pass\n";
 }
 std::cout<<"MARKET_BANK_SQL_PASS "<<checks<<" checks\n";Sql_Free(sql_handle);sql_handle=nullptr;return 0;
}
