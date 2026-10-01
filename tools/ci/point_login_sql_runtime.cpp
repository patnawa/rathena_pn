// Linked production login registry writer and barrier using independent SQL handles.
#include <login/account.hpp>
#include <login/login.hpp>
#include <unordered_map>
extern AccountDB* accounts;
extern std::unordered_map<uint32,online_login_data> online_db;
extern int32 logchrif_parse_upd_global_accreg(int32,int32,char*);
#include <common/malloc.hpp>
#include <common/socket.hpp>
#include <common/timer.hpp>
#include <custom/global_point.hpp>
#include <iostream>
#include <cstdlib>
static Sql* fixture;
static unsigned checks;
static void require(bool value,const char* message){++checks;if(!value){std::cerr<<"FAIL "<<message<<std::endl;abort();}}
static void query(const std::string& value){require(Sql_QueryStr(fixture,value.c_str())==SQL_SUCCESS,"fixture query");}
static std::string scalar(const std::string& value){query(value);char* data=nullptr;require(Sql_NextRow(fixture)==SQL_SUCCESS&&Sql_GetData(fixture,0,&data,nullptr)==SQL_SUCCESS&&data,"scalar");std::string result=data;Sql_FreeResult(fixture);return result;}
static AccountDB* connect(const char* database){
 auto* db=account_db_sql();
 for(auto pair:{std::make_pair("login_server_ip","shop-recovery-db"),std::make_pair("login_server_port","3306"),std::make_pair("login_server_id","root"),std::make_pair("login_server_pw","shop-fixture-only"),std::make_pair("login_server_db",database)})require(db->set_property(db,pair.first,pair.second),"set isolated login fixture property");
 require(db->init(db),"initialize actual account SQL adapter");return db;
}
extern "C" int __wrap_main(int,char**){
 malloc_init();timer_init();fixture=Sql_Malloc();require(Sql_Connect(fixture,"root","shop-fixture-only","shop-recovery-db",3306,"shop_recovery_probe")==SQL_SUCCESS,"disposable fixture connect");require(scalar("SELECT DATABASE()") =="shop_recovery_probe","database guard");
 query("SELECT payload FROM pn_global_point_barriers WHERE state=0");char* data=nullptr;size_t length=0;pn_shop::Commit r;
 require(Sql_NextRow(fixture)==SQL_SUCCESS&&Sql_GetData(fixture,0,&data,&length)==SQL_SUCCESS&&data&&length==sizeof(r),"read actual prepared intent");memcpy(&r,data,sizeof(r));Sql_FreeResult(fixture);
 auto* db=connect("shop_recovery_probe");accounts=db;online_db[r.account_id].char_server=0;require(mmo_point_pending(db,r.account_id),"actual login admission sees char intent through separate SQL handle");
 socket_data fifo{};unsigned char bytes[256]{};fifo.rdata=bytes;fifo.rdata_size=sizeof(bytes);constexpr int fd=13;session[fd]=&fifo;
 auto save=[&](const char* key,int64_t value,bool erase=false,bool ingress=false){
  fifo.rdata_pos=0;memset(bytes,0,sizeof(bytes));RFIFOL(fd,4)=r.account_id;RFIFOL(fd,8)=r.char_id;RFIFOW(fd,12)=1;const size_t len=strlen(key)+1;RFIFOB(fd,14)=len;memcpy(RFIFOP(fd,15),key,len);size_t cursor=15+len;const uint32_t index=0;memcpy(RFIFOP(fd,cursor),&index,sizeof(index));cursor+=4;RFIFOB(fd,cursor++)=erase?1:0;memcpy(RFIFOP(fd,cursor),&value,sizeof(value));
  RFIFOW(fd,2)=cursor+8;
  if(ingress){char ip[]="fixture";require(logchrif_parse_upd_global_accreg(fd,0,ip)==1,"actual login ingress consumes frame");}
  else mmo_save_global_accreg(db,fd,r.account_id,r.char_id,0);
 };
 save("##Points",5000000001LL);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="5000000001","older flush executes before approval");save("##Points",r.point.before);
 require(mmo_point_barrier(db,r),"actual login adapter approves shared char intent");
 save("##Points",19);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="5000000000","approved barrier blocks late overwrite");
 save("##Points",0,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="5000000000","approved barrier blocks late delete");
 save("##Other",123);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Other'")=="123","unrelated registry writes continue");
 // Separate database has valid transactional schema but cannot see the char intent.
 query("CREATE DATABASE point_split_probe");for(const char* table:{"login","global_acc_reg_num","pn_global_point_barriers"})query(std::string("CREATE TABLE point_split_probe.")+table+" LIKE shop_recovery_probe."+table);
 auto* split=connect("point_split_probe");require(!mmo_point_barrier(split,r),"actual separate login database refuses invisible character intent");split->destroy(split);query("DROP DATABASE point_split_probe");
 // Durable completion is tested with real char payment SQL in the companion probe.
 require(Sql_BeginTransaction(fixture)==SQL_SUCCESS,"finish fixture transaction");require(pn_global_point::finish_locked(fixture,r),"mark terminal barrier");require(Sql_EndTransaction(fixture,true)==SQL_SUCCESS,"finish fixture commit");
 require(!mmo_point_pending(db,r.account_id),"terminal receipt state releases actual login admission");save("##Points",99);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="99","subsequent registry writes resume");
 // Actual ingress additionally rejects displaced char-server writes after terminal state.
 query("REPLACE INTO login(account_id,userid,user_pass,sex) VALUES(990013,'point_fixture','unused','M')");accounts=db;online_db[r.account_id].char_server=0;
 save("##Points",88,false,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="88","current char server ordinary save accepted");
 online_db[r.account_id].char_server=1;save("##Points",87,false,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="88","displaced char server cannot overwrite terminal point balance");
 online_db[r.account_id].char_server=0;save("##Points",86,false,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="86","ordered logout flush accepted before offline transition");
 online_db.erase(r.account_id);save("##Points",85,false,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Points'")=="86","offline former char server save refused");save("##Progress",44,false,true);require(scalar("SELECT value FROM global_acc_reg_num WHERE `key`='##Progress'")=="44","unrelated offline reconnect progression is preserved");accounts=nullptr;
 session[fd]=nullptr;db->destroy(db);Sql_Free(fixture);timer_final();malloc_final();std::cout<<"POINT_LOGIN_SQL_PASS "<<checks<<"; linked login object, actual registry writes, shared authority and split database refusal (no TCP)\n";return 0;
}
