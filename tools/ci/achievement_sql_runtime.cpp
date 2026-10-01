// Native character achievement writer against disposable MariaDB only.
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "char/char.hpp"
#include "char/inter.hpp"
#include "char/int_achievement.hpp"
#include "common/mmo.hpp"
#include "common/malloc.hpp"
#include "common/socket.hpp"
#include "common/sql.hpp"
#include "common/timer.hpp"
int32 mapif_parse_achievement_reward(int32 fd);
static unsigned checks=0;
static void require(bool ok,const char* label){++checks;if(!ok){std::cerr<<"FAIL: "<<label<<std::endl;std::exit(1);}}
static void sql(const std::string& query){require(Sql_QueryStr(sql_handle,query.c_str())==SQL_SUCCESS,query.c_str());}
static std::string result(const std::string& query){
 sql(query);std::string value;int row;
 while((row=Sql_NextRow(sql_handle))==SQL_SUCCESS){
  for(unsigned i=0;i<Sql_NumColumns(sql_handle);++i){char* text=nullptr;size_t size=0;
   require(Sql_GetData(sql_handle,i,&text,&size)==SQL_SUCCESS,"SQL cell");value+=text?std::string(text,size):"NULL";value+='|';}
  value+='\n';
 }
 require(row==SQL_NO_DATA,"SQL rows complete");Sql_FreeResult(sql_handle);return value;
}
static std::string snapshot(){return result("SELECT * FROM achievement ORDER BY char_id,id");}

static constexpr uint32 reward_account_id=99001;
static constexpr uint32 reward_char_id=9900101;
static constexpr int32 reward_achievement_id=7001;

struct RewardReply {
 uint16 packet=0;
 uint32 char_id=0;
 int32 achievement_id=0;
 uint32 rewarded=0;
 size_t bytes=0;
};

static void reward_tables(const char* achievement_table="achievement",const char* mail_table="mail",
 const char* attachment_table="mail_attachments"){
 std::strcpy(schema_config.achievement_table,achievement_table);
 std::strcpy(schema_config.mail_db,mail_table);
 std::strcpy(schema_config.mail_attachment_db,attachment_table);
}

static void reward_seed(int32 achievement_id=reward_achievement_id){
 reward_tables();
 sql("DROP TRIGGER IF EXISTS reward_mail_fault");
 sql("DROP TRIGGER IF EXISTS reward_attachment_fault");
 sql("DROP TRIGGER IF EXISTS reward_update_fault");
 sql("DELETE FROM mail_attachments");
 sql("DELETE FROM mail");
 sql("DELETE FROM achievement");
 sql("INSERT INTO achievement(char_id,id,count1,completed,rewarded) VALUES("+
  std::to_string(reward_char_id)+","+std::to_string(achievement_id)+",1,FROM_UNIXTIME(1700000000),NULL)");
}

static std::string reward_state(){
 return result("SELECT char_id,id,COALESCE(UNIX_TIMESTAMP(rewarded),0) FROM achievement ORDER BY char_id,id")+
  result("SELECT id,dest_id,time,status,type FROM mail ORDER BY id")+
  result("SELECT id,`index`,nameid,amount,identify FROM mail_attachments ORDER BY id,`index`");
}

static int reward_sql_cases(){
 constexpr int fd=42;
 require(session[fd]==nullptr,"unused reward fixture FIFO slot");
 // uint32 FIFO fields start at offsets 2, 6 and 10. Shifting the aligned
 // allocation by two keeps the production accessors aligned under UBSan.
 std::vector<uint8> input_storage(2+16+NAME_LENGTH+ACHIEVEMENT_NAME_LENGTH);
 std::vector<uint8> output_storage(2+FIFOSIZE_SERVERLINK);
 uint8* input=input_storage.data()+2;
 uint8* output=output_storage.data()+2;
 socket_data fifo{};fifo.flag.server=1;fifo.rdata=input;fifo.wdata=output;
 fifo.max_rdata=input_storage.size()-2;fifo.max_wdata=output_storage.size()-2;session[fd]=&fifo;

 auto owner=[&](int variant=0){
  char_get_onlinedb().clear();
  for(int i=0;i<MAX_MAP_SERVERS;++i)map_server[i].fd=-1;
  map_server[0].fd=fd;
  if(variant==1)return;
  auto online=std::make_shared<online_char_data>(reward_account_id);
  online->char_id=reward_char_id;online->server=0;
  if(variant==2)online->char_id++;
  if(variant==3)online->server=-1;
  if(variant==4)online->server=MAX_MAP_SERVERS;
  if(variant==5)map_server[0].fd=fd+1;
  char_get_onlinedb()[reward_account_id]=online;
 };
 auto dispatch=[&](uint32 nameid,uint16 amount,int32 achievement_id=reward_achievement_id){
  std::memset(input,0,input_storage.size()-2);fifo.rdata_size=16+NAME_LENGTH+ACHIEVEMENT_NAME_LENGTH;
  fifo.rdata_pos=0;fifo.wdata_size=0;
  const uint16 packet=0x3064;std::memcpy(input,&packet,sizeof(packet));
  std::memcpy(input+2,&reward_char_id,sizeof(reward_char_id));
  std::memcpy(input+6,&achievement_id,sizeof(achievement_id));
  std::memcpy(input+10,&nameid,sizeof(nameid));std::memcpy(input+14,&amount,sizeof(amount));
  std::strcpy(reinterpret_cast<char*>(input+16),"Reward fixture");
  std::strcpy(reinterpret_cast<char*>(input+16+NAME_LENGTH),"Atomic reward");
  mapif_parse_achievement_reward(fd);
  require(fifo.wdata_size>=14,"reward handler emits acknowledgement");
  const size_t offset=fifo.wdata_size-14;RewardReply reply{};reply.bytes=fifo.wdata_size;
  std::memcpy(&reply.packet,output+offset,sizeof(reply.packet));
  std::memcpy(&reply.char_id,output+offset+2,sizeof(reply.char_id));
  std::memcpy(&reply.achievement_id,output+offset+6,sizeof(reply.achievement_id));
  std::memcpy(&reply.rewarded,output+offset+10,sizeof(reply.rewarded));
  require(reply.packet==0x3864 && reply.char_id==reward_char_id && reply.achievement_id==achievement_id,
   "reward acknowledgement preserves request identity");
  return reply;
 };
 auto untouched=[&](const char* label){
  require(result("SELECT COALESCE(UNIX_TIMESTAMP(rewarded),0) FROM achievement WHERE char_id="+
   std::to_string(reward_char_id)+" AND id="+std::to_string(reward_achievement_id))=="0|\n",label);
  require(result("SELECT COUNT(*) FROM mail")=="0|\n","failed reward leaves no mail header");
  require(result("SELECT COUNT(*) FROM mail_attachments")=="0|\n","failed reward leaves no attachment");
 };

 // Header, attachment and rewarded timestamp are one durable operation.
 reward_seed();owner();auto reply=dispatch(501,3);
 require(reply.rewarded>0,"item reward succeeds");
 require(reply.bytes==75+14,"committed item reward notifies mail before its reward ACK");
 require(result("SELECT COUNT(*),MIN(dest_id),MIN(time) FROM mail")==
  "1|"+std::to_string(reward_char_id)+"|"+std::to_string(reply.rewarded)+"|\n","mail header committed with ACK timestamp");
 require(result("SELECT nameid,amount,identify FROM mail_attachments")=="501|3|1|\n","item attachment committed");
 require(result("SELECT COUNT(*) FROM mail m JOIN mail_attachments a ON a.id=m.id WHERE m.dest_id="+
  std::to_string(reward_char_id)+" AND a.nameid=501 AND a.amount=3")=="1|\n","mail header owns committed attachment");
 require(result("SELECT UNIX_TIMESTAMP(rewarded) FROM achievement WHERE char_id="+std::to_string(reward_char_id)+
  " AND id="+std::to_string(reward_achievement_id))==std::to_string(reply.rewarded)+"|\n","rewarded timestamp committed");

 const std::vector<std::pair<std::string,std::string>> faults={
  {"reward_mail_fault","BEFORE INSERT ON mail FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='mail header fault'"},
  {"reward_attachment_fault","BEFORE INSERT ON mail_attachments FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='attachment fault'"},
  {"reward_update_fault","BEFORE UPDATE ON achievement FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='reward update fault'"}};
 for(const auto& fault:faults){
  reward_seed();owner();sql("CREATE TRIGGER "+fault.first+" "+fault.second);
  reply=dispatch(501,3);require(reply.rewarded==0,"SQL fault returns failed reward ACK");
  require(reply.bytes==14,"rolled-back reward emits no new-mail notification");
  untouched("SQL fault leaves achievement unclaimed");sql("DROP TRIGGER "+fault.first);
 }

 // A title/script-only reward never consults either mail table's engine.
 reward_seed(7002);owner();reward_tables("achievement","missing_mail_engine","missing_attachment_engine");
 reply=dispatch(0,0,7002);require(reply.rewarded>0,"title-only reward succeeds without mail tables");
 require(result("SELECT UNIX_TIMESTAMP(rewarded) FROM achievement WHERE id=7002")==std::to_string(reply.rewarded)+"|\n",
  "title-only rewarded timestamp committed");
 require(result("SELECT COUNT(*) FROM mail")=="0|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="0|\n",
  "title-only reward creates no mail rows");reward_tables();

 // Every table participating in an item claim must support transactions.
 sql("DROP TABLE IF EXISTS reward_achievement_myisam");sql("DROP TABLE IF EXISTS reward_mail_myisam");
 sql("DROP TABLE IF EXISTS reward_attachment_myisam");
 sql("CREATE TABLE reward_achievement_myisam LIKE achievement");sql("ALTER TABLE reward_achievement_myisam ENGINE=MyISAM");
 sql("CREATE TABLE reward_mail_myisam LIKE mail");sql("ALTER TABLE reward_mail_myisam ENGINE=MyISAM");
 sql("CREATE TABLE reward_attachment_myisam LIKE mail_attachments");sql("ALTER TABLE reward_attachment_myisam ENGINE=MyISAM");
 for(int participant=0;participant<3;++participant){
  reward_seed();owner();
  if(participant==0){
   sql("DELETE FROM reward_achievement_myisam");sql("INSERT INTO reward_achievement_myisam SELECT * FROM achievement");
   reward_tables("reward_achievement_myisam","mail","mail_attachments");
  }else if(participant==1)reward_tables("achievement","reward_mail_myisam","mail_attachments");
  else reward_tables("achievement","mail","reward_attachment_myisam");
  reply=dispatch(501,3);require(reply.rewarded==0,"non-InnoDB participant refuses item reward");
  require(result("SELECT COUNT(*) FROM mail")=="0|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="0|\n",
   "engine refusal creates no default mail rows");
  require(result("SELECT COUNT(*) FROM reward_mail_myisam")=="0|\n" &&
   result("SELECT COUNT(*) FROM reward_attachment_myisam")=="0|\n","engine refusal creates no shadow mail rows");
  if(participant==0)require(result("SELECT COALESCE(UNIX_TIMESTAMP(rewarded),0) FROM reward_achievement_myisam")=="0|\n",
   "nontransactional achievement remains unclaimed");
  else require(result("SELECT COALESCE(UNIX_TIMESTAMP(rewarded),0) FROM achievement")=="0|\n",
   "mail engine refusal leaves achievement unclaimed");
 }
 reward_tables();

 // A lost reward ACK can be followed by an older dirty map autosave. The
 // monotonic full-snapshot writer must retain the char-server's newer reward
 // timestamp and counters before a user retries the claim.
 reward_seed();owner();auto first=dispatch(502,4);require(first.rewarded>0,"first lost-response claim commits");
 const auto durable=reward_state();
 achievement stale{};stale.achievement_id=reward_achievement_id;stale.completed=1700000000;
 require(mapif_achievement_save_rows(reward_char_id,&stale,1),"stale ordinary snapshot is merged monotonically");
 require(reward_state()==durable,"stale ordinary snapshot preserves reward timestamp, counters and mail");
 require(result("SELECT count1,UNIX_TIMESTAMP(completed),UNIX_TIMESTAMP(rewarded) FROM achievement WHERE char_id="+
  std::to_string(reward_char_id)+" AND id="+std::to_string(reward_achievement_id))==
  "1|1700000000|"+std::to_string(first.rewarded)+"|\n","durable progression dominates stale map fields");
 auto retry=dispatch(502,4);
 require(retry.rewarded==first.rewarded,"retry returns existing rewarded timestamp");
 require(retry.bytes==14,"idempotent retry sends no duplicate new-mail notification");
 require(reward_state()==durable,"retry does not duplicate mail or mutate achievement");
 require(result("SELECT COUNT(*) FROM mail")=="1|\n" && result("SELECT COUNT(*) FROM mail_attachments")=="1|\n",
  "lost-response retry retains exactly one mail and attachment");

 for(int variant=1;variant<=5;++variant){
  reward_seed();owner(variant);const auto before=reward_state();reply=dispatch(501,1);
  require(reply.rewarded==0,"nonowner reward request rejected");
  require(reward_state()==before,"nonowner reward request changes no durable state");
 }

 reward_tables();char_get_onlinedb().clear();map_server[0].fd=-1;session[fd]=nullptr;
 std::cout<<"ACHIEVEMENT_SQL_OK mode=reward checks="<<checks<<std::endl;return 0;
}

static std::vector<achievement> request(){
 std::vector<achievement> rows(3);
 for(int i=0;i<3;++i){rows[i].achievement_id=i==2?4:i+1;rows[i].count[0]=20+i;}
 rows[0].completed=1700000000;return rows;
}
static void seed(){
 sql("DROP TRIGGER IF EXISTS achievement_fault");sql("DELETE FROM achievement");
 sql("INSERT INTO achievement(char_id,id,count1) VALUES(99001,1,10),(99001,2,10),(99001,3,10),(99002,1,77)");
}
extern "C" int __wrap_main(int argc,char**argv){
 malloc_init();timer_init();sql_handle=Sql_Malloc();
 require(Sql_Connect(sql_handle,"root","achievement-fixture-only","achievement-db",3306,"achievement_probe")==SQL_SUCCESS,"connect disposable SQL");
 require(result("SELECT DATABASE()") == "achievement_probe|\n","fixture database guard");
 reward_tables();
 const std::string mode=argc>1?argv[1]:"normal";auto rows=request();
 if(mode=="reward")return reward_sql_cases();
 else if(mode=="concurrency-setup"){
  seed();sql("CREATE TRIGGER achievement_fault BEFORE UPDATE ON achievement FOR EACH ROW BEGIN IF NEW.id=1 THEN DO SLEEP(2); END IF; END");
 }else if(mode=="concurrent-a" || mode=="concurrent-b"){
  if(mode=="concurrent-b")for(auto& row:rows)row.count[0]+=10;
  require(mapif_achievement_save_rows(99001,rows.data(),rows.size()),"concurrent complete snapshot accepted");
 }else if(mode=="concurrency-verify"){
  sql("DROP TRIGGER achievement_fault");
  const auto value=result("SELECT id,count1 FROM achievement WHERE char_id=99001 ORDER BY id");
  require(value=="1|20|\n2|21|\n3|10|\n4|22|\n" || value=="1|30|\n2|31|\n3|10|\n4|32|\n",
   "concurrent saves cannot mix snapshots or delete omitted durable rows");
  require(result("SELECT count1 FROM achievement WHERE char_id=99002")=="77|\n","concurrent saves preserve other character");
 }else if(mode=="crash-setup"){
  seed();sql("CREATE TRIGGER achievement_fault BEFORE UPDATE ON achievement FOR EACH ROW BEGIN IF NEW.id=2 THEN DO SLEEP(60); END IF; END");
 }else if(mode=="crash"){
  require(!mapif_achievement_save_rows(99001,rows.data(),rows.size()),"connection loss reports failure");
 }else if(mode=="crash-verify"){
  sql("DROP TRIGGER IF EXISTS achievement_fault");
  require(result("SELECT id,count1 FROM achievement WHERE char_id=99001 ORDER BY id")=="1|10|\n2|10|\n3|10|\n","database kill rolled back partial snapshot");
  require(mapif_achievement_save_rows(99001,rows.data(),rows.size()),"retry after restart succeeds");
 }else{
  seed();const auto before=snapshot();
  const std::vector<std::string> faults={
   "BEFORE UPDATE ON achievement FOR EACH ROW BEGIN IF NEW.id=1 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='first update'; END IF; END",
   "BEFORE UPDATE ON achievement FOR EACH ROW BEGIN IF NEW.id=2 THEN SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='second update'; END IF; END",
   "BEFORE INSERT ON achievement FOR EACH ROW SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='insert'"};
  for(const auto& fault:faults){
   sql("CREATE TRIGGER achievement_fault "+fault);
   require(!mapif_achievement_save_rows(99001,rows.data(),rows.size()),"write fault rejected");
   require(snapshot()==before,"all prior rows preserved after SQL fault");
   sql("DROP TRIGGER achievement_fault");
  }
  sql("ALTER TABLE achievement CHANGE count1 damaged_count INT UNSIGNED NOT NULL DEFAULT 0");
  require(!mapif_achievement_save_rows(99001,rows.data(),rows.size()),"failed SELECT is not an empty log");
  require(snapshot()==before,"load failure preserves rows");
  sql("ALTER TABLE achievement CHANGE damaged_count count1 INT UNSIGNED NOT NULL DEFAULT 0");
  sql("ALTER TABLE achievement ENGINE=MyISAM");
  require(!mapif_achievement_save_rows(99001,rows.data(),rows.size()),"nontransactional table refused");
  require(snapshot()==before,"engine refusal preserves rows");sql("ALTER TABLE achievement ENGINE=InnoDB");
  require(mapif_achievement_save_rows(99001,rows.data(),rows.size()),"whole snapshot accepted");
  require(result("SELECT id,count1 FROM achievement WHERE char_id=99001 ORDER BY id")==
   "1|20|\n2|21|\n3|10|\n4|22|\n","updates/inserts preserve omitted durable achievement");
  const auto after=snapshot();require(mapif_achievement_save_rows(99001,rows.data(),rows.size()),"same snapshot retry accepted");
  require(snapshot()==after,"same snapshot retry is idempotent");
  require(result("SELECT count1 FROM achievement WHERE char_id=99002")=="77|\n","other character preserved");
  auto duplicate=rows;duplicate[1].achievement_id=1;
  require(!mapif_achievement_save_rows(99001,duplicate.data(),duplicate.size()),"duplicate IDs refused");
  require(snapshot()==after,"invalid snapshot preserves rows");
  require(mapif_achievement_save_rows(99001,nullptr,0),"explicit empty stale snapshot accepted");
  require(snapshot()==after,"empty stale snapshot cannot erase durable achievements");
 }
 Sql_Free(sql_handle);sql_handle=nullptr;timer_final();malloc_final();
 std::cout<<"ACHIEVEMENT_SQL_OK mode="<<mode<<" checks="<<checks<<std::endl;return 0;
}
