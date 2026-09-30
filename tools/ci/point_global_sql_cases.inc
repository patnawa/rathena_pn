// Real shared-database barrier and original production payment transaction.
static pn_shop::Commit global_request(){
 auto r=request(pn_shop::Asset);r.stock_count=0;for(auto& s:r.stocks)s={};
 r.wallet_after=r.wallet_before;r.cash_after=r.cash_before;r.kafra_after=r.kafra_before;
 r.point.scope=pn_shop::GlobalPoint;std::strcpy(r.point.key,"##Points");r.point.before=5000000000LL;r.point.after=r.point.before-100;
 r.pet_count=1;r.counter_after=r.counter_before+1;r.pets[0].output=pet_output();return r;
}
static void global_seed(){
 pet_seed();sql("DELETE FROM pn_global_point_barriers");sql("DELETE FROM global_acc_reg_num");
 sql("INSERT INTO global_acc_reg_num(account_id,`key`,`index`,value) VALUES(990013,'##Points',0,5000000000)");
}
static std::string global_state(){return pet_state()+result("SELECT * FROM global_acc_reg_num ORDER BY account_id,`key`,`index`")+result("SELECT * FROM pn_global_point_barriers ORDER BY account_id,sequence");}
static void global_old(){sql("UPDATE pn_global_point_barriers SET created_at=DATE_SUB(NOW(),INTERVAL 120 SECOND)");}
extern int32 mapif_parse_Registry(int32 fd);
static void point_registry_ingress_cases(){
 constexpr int fd=42;socket_data fifo{};unsigned char bytes[128]{};fifo.rdata=bytes;fifo.max_rdata=sizeof(bytes);fifo.rdata_size=sizeof(bytes);session[fd]=&fifo;
 auto r=global_request();auto owner=std::make_shared<online_char_data>(r.account_id);owner->char_id=r.char_id;owner->server=0;char_get_onlinedb()[r.account_id]=owner;map_server[0].fd=fd;
 sql("INSERT IGNORE INTO pn_point_registry_keys(account_id,char_id,point_key) VALUES(990013,0,'#Ingress')");
 auto send=[&](int64_t value,const char* key="#Ingress"){
  fifo.rdata_pos=0;memset(bytes,0,sizeof(bytes));RFIFOL(fd,4)=r.account_id;RFIFOL(fd,8)=r.char_id;RFIFOW(fd,12)=1;
  size_t length=strlen(key)+1;RFIFOB(fd,14)=length;memcpy(RFIFOP(fd,15),key,length);size_t cursor=15+length;const uint32_t index=0;memcpy(RFIFOP(fd,cursor),&index,sizeof(index));cursor+=4;RFIFOB(fd,cursor++)=0;memcpy(RFIFOP(fd,cursor),&value,sizeof(value));
  mapif_parse_Registry(fd);
 };
 send(100);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="100|\n","current owner registry reaches actual SQL");
 owner->char_id++;send(1);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="100|\n","same account wrong character cannot overwrite");owner->char_id=r.char_id;
 map_server[0].fd=fd+1;send(2);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="100|\n","displaced map cannot overwrite after terminal barrier");map_server[0].fd=fd;
 // chrif_save enqueues registry before the final save packet marks this owner offline.
 send(90);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="90|\n","ordered logout flush still writes while owner is current");
 owner->server=-1;send(3);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="90|\n","post-logout registry cannot restore stale values");
 char_get_onlinedb().clear();send(4);require(result("SELECT value FROM acc_reg_num WHERE `key`='#Ingress'")=="90|\n","unknown session rejected");
 send(77,"#Progress");require(result("SELECT value FROM acc_reg_num WHERE `key`='#Progress'")=="77|\n","unrelated offline ST_LOGOUT reconnect progression preserved");
 session[fd]=nullptr;map_server[0].fd=-1;
}
static int global_point_cases(const std::string& mode){
 auto r=global_request();require(pn_shop::valid(r),"global point descriptor valid");
 if(mode=="global-seed-pending" || mode=="global-seed-approved" || mode=="global-seed-committed"){
  global_seed();require(pn_global_point::prepare(sql_handle,r),"persist restart intent");
  if(mode!="global-seed-pending")require(pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"persist restart barrier");
  if(mode=="global-seed-committed")require(pn_shop_tosql(r,true)==pn_shop::Committed,"persist restart receipt");
  global_old();std::cout<<"GLOBAL_POINT_RESTART_SEEDED "<<mode<<std::endl;return 0;
 }
 if(mode=="global-resolve-pending" || mode=="global-resolve-approved" || mode=="global-resolve-committed"){
  const bool committed=mode=="global-resolve-committed";
  require(pn_global_point::pending(sql_handle,r.account_id)==!committed,"restart retains admission state");
  require(pn_global_point::expire(sql_handle),"restart recovery closes orphan intent");
  require(!pn_global_point::pending(sql_handle,r.account_id),"restart clears admission after durable outcome");
  require(pn_shop_tosql(r,false)==(committed?pn_shop::Committed:pn_shop::Rejected),"restart resolves exact original outcome");
  require(result("SELECT value FROM global_acc_reg_num")== (committed?"4999999900|\n":"5000000000|\n"),"restart never refunds or double debits");
  std::cout<<"GLOBAL_POINT_RESTART_PASS "<<checks<<" "<<mode<<std::endl;return 0;
 }
 require(mode=="global-points","known global mode");global_seed();auto original=global_state();
 require(!pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"absent shared intent cannot approve (split database fail closed)");
 require(pn_shop_tosql(r,true)==pn_shop::Retry,"payment awaits explicit login barrier");require(global_state()==original,"pre-barrier retry preserves assets");
 require(pn_global_point::prepare(sql_handle,r),"intent preparation");require(pn_global_point::prepare(sql_handle,r),"immutable prepare retry");
 require(pn_global_point::pending(sql_handle,r.account_id),"pre-barrier intent fences admission");
 require(!pn_global_point::pending(sql_handle,r.account_id,r.point.key),"older ordered registry flush is accepted before approval");
 auto changed=r;changed.point.after--;require(!pn_global_point::prepare(sql_handle,changed),"mutated intent refused");require(!pn_global_point::approve(sql_handle,changed,"global_acc_reg_num"),"mutated approval refused");
 auto concurrent=r;concurrent.sequence++;require(!pn_global_point::prepare(sql_handle,concurrent),"second active request for account refused");
 sql("ALTER TABLE global_acc_reg_num ENGINE=MyISAM");require(!pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"nontransactional login authority refused");sql("ALTER TABLE global_acc_reg_num ENGINE=InnoDB");
 require(pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"ordered login barrier approved");require(pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"barrier retry idempotent");
 require(pn_global_point::pending(sql_handle,r.account_id,r.point.key),"approved key rejects late registry saves");require(!pn_global_point::pending(sql_handle,r.account_id,"##Other"),"unrelated registry remains writable");
 auto before=global_state();
 for(const std::string& fault:{"BEFORE INSERT ON pet","BEFORE INSERT ON pn_pet_entitlements","BEFORE INSERT ON inventory","BEFORE UPDATE ON `char`","BEFORE UPDATE ON global_acc_reg_num","BEFORE UPDATE ON pn_global_point_barriers","BEFORE INSERT ON pn_shop_commits"}){
  sql("CREATE TRIGGER global_point_fault "+fault+" FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='global point fault'");
  require(pn_shop_tosql(r,true)==pn_shop::Retry,"global write failure retries");require(global_state()==before,"pet, inventory, global debit, barrier and receipt roll back atomically");sql("DROP TRIGGER global_point_fault");
 }
 require(pn_shop_tosql(r,true)==pn_shop::Committed,"global purchase commits after barrier");require(!pn_global_point::pending(sql_handle,r.account_id),"committed receipt releases admission");
 require(result("SELECT value FROM global_acc_reg_num")=="4999999900|\n","global debit exactly once at 64-bit precision");
 newer();before=global_state();require(pn_shop_tosql(r,false)==pn_shop::Committed,"departed owner resolves immutable receipt");require(global_state()==before,"receipt retry preserves newer state");require(!pn_global_point::prepare(sql_handle,r),"finished receipt cannot recreate fence");
 for(bool approve:{false,true}){
  global_seed();require(pn_global_point::prepare(sql_handle,r),"expiry intent");if(approve)require(pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"expiry approved intent");
  global_old();sql("CREATE TRIGGER global_point_fault BEFORE INSERT ON pn_shop_commits FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='expiry receipt fault'");
  require(!pn_global_point::expire(sql_handle),"failed timeout receipt cannot release admission");require(pn_global_point::pending(sql_handle,r.account_id),"failed expiry stays fenced");sql("DROP TRIGGER global_point_fault");
  require(pn_global_point::expire(sql_handle),"orphan timeout resolves durably");require(!pn_global_point::pending(sql_handle,r.account_id),"timeout releases admission");
  require(!pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"late barrier cannot resurrect canceled request");require(pn_shop_tosql(r,true)==pn_shop::Rejected,"late map retry reads durable rejection");require(result("SELECT value FROM global_acc_reg_num")=="5000000000|\n","timeout never charges");
 }
 global_seed();require(pn_global_point::prepare(sql_handle,r),"older flush fixture");
 sql("UPDATE global_acc_reg_num SET value=4999999999");require(pn_global_point::approve(sql_handle,r,"global_acc_reg_num"),"older flush drained before barrier");
 require(pn_shop_tosql(r,true)==pn_shop::Rejected,"stale map view rejected after actual authority compare");require(!pn_global_point::pending(sql_handle,r.account_id),"stale balance rejection releases fence");
 require(result("SELECT value FROM global_acc_reg_num")=="4999999999|\n","stale purchase does not overwrite authority");
 point_registry_ingress_cases();
 std::cout<<"GLOBAL_POINT_SQL_PASS "<<checks<<"; shared InnoDB intent/barrier/CAS/receipt and admission lifecycle"<<std::endl;return 0;
}
