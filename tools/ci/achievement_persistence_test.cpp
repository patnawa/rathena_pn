#include <common/mmo.hpp>
#include <custom/achievement_protocol.hpp>
#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <vector>
#include <unordered_set>
static std::map<int,achievement> persisted,original;
static std::vector<achievement> incoming;
static int failed_id=0,deleted=0,errors=0,warnings=0;
static bool reply=false,ack_success=false,present=true;
static unsigned char buffer[65536];
#define RFIFOW(fd,n) (*reinterpret_cast<uint16*>(buffer+(n)))
#define RFIFOL(fd,n) (*reinterpret_cast<uint32*>(buffer+(n)))
#define RFIFOQ(fd,n) (*reinterpret_cast<uint64*>(buffer+(n)))
#define RFIFOB(fd,n) (buffer[(n)])
#define RFIFOP(fd,n) (buffer+(n))
#define WFIFOW(fd,n) (*reinterpret_cast<uint16*>(buffer+(n)))
#define WFIFOL(fd,n) (*reinterpret_cast<uint32*>(buffer+(n)))
#define WFIFOQ(fd,n) (*reinterpret_cast<uint64*>(buffer+(n)))
#define WFIFOB(fd,n) (buffer[(n)])
#define WFIFOP(fd,n) (buffer+(n))
#define WFIFOHEAD(fd,n) ((void)0)
static int fifo_writes=0,last_fifo_size=0;
#define WFIFOSET(fd,n) (++fifo_writes,last_fifo_size=(n))
#define aFree(p) std::free(p)
#define CREATE(result,type,count) ((result)=static_cast<type*>(std::calloc((count),sizeof(type))))
enum class AchievementCompareReplaceResult : uint8 { Error, Conflict, Applied };
static bool load_ok=true,engine_ok=true,begin_ok=true,end_ok=true;
static std::map<int,achievement> transaction_before;
static constexpr int SQL_SUCCESS=0,SQL_ERROR=1;
static void* sql_handle=nullptr;
static struct {const char* achievement_table="achievement";} schema_config;
static int Sql_Query(void*,const char*,...){return SQL_SUCCESS;}
static int Sql_NextRow(void*){return SQL_SUCCESS;}
static int Sql_GetData(void*,int,char** out,void*){static char good[]="InnoDB",bad[]="MyISAM";*out=engine_ok?good:bad;return SQL_SUCCESS;}
static void Sql_FreeResult(void*){}
static int Sql_BeginTransaction(void*){transaction_before=persisted;return begin_ok?SQL_SUCCESS:SQL_ERROR;}
static int Sql_EndTransaction(void*,bool commit){if(!commit||!end_ok)persisted=transaction_before;return end_ok?SQL_SUCCESS:SQL_ERROR;}
static bool Sql_InTransaction(void*){return true;}
static achievement* mapif_achievements_fromsql(uint32,int32* count,bool* success=nullptr,bool=false){
 if(success)*success=load_ok;
 if(!load_ok){*count=0;return nullptr;}
 *count=persisted.size();auto* rows=static_cast<achievement*>(std::calloc(*count,sizeof(achievement)));
 int i=0;for(const auto& pair:persisted)rows[i++]=pair.second;return rows;
}
static bool mapif_achievement_update(uint32,const achievement* value){
 if(value->achievement_id==failed_id)return false;
 persisted[value->achievement_id]=*value;return true;
}
static bool mapif_achievement_add(uint32 id,const achievement* value){return mapif_achievement_update(id,value);}
static bool mapif_achievement_delete(uint32,int32 id){if(id==failed_id)return false;++deleted;persisted.erase(id);return true;}
static void mapif_achievement_save(int32,uint32,bool success){reply=true;ack_success=success;}
struct online_char_data {uint32 char_id=99001;int32 server=0;};
static std::map<uint32,std::shared_ptr<online_char_data>> online_characters{{77,std::make_shared<online_char_data>()}};
static auto& char_get_onlinedb(){return online_characters;}
static constexpr int32 MAX_MAP_SERVERS=1;
static struct {int32 fd=0;} map_server[MAX_MAP_SERVERS];
static int eof_calls=0,last_eof_fd=-1,objective_checks=0,level_checks=0,title_checks=0,client_updates=0,client_lists=0;
static void set_eof(int fd){++eof_calls;last_eof_fd=fd;}
static void ShowError(const char*,...){++errors;}
static void ShowWarning(const char*,...){++warnings;}
#include "char-handler.inc"
#include "char-load-handler.inc"
struct AchievementEvent {int32 group=0;std::vector<int32> arguments;};
struct map_session_data {
 struct {uint32 char_id=99001;char name[24]="Fixture";} status;
 struct {int32 total_score=0,level=0,reward_pending_id=0;bool save=false,loaded=false;uint16 count=0,incompleteCount=0,opaque_count=0;achievement* achievements=nullptr;} achievement_data;
 struct {std::vector<struct AchievementEvent> deferred_achievements;} shop_commit;
 int fd=1;
};
static map_session_data player;
static map_session_data* map_charid2sd(int32){return present?&player:nullptr;}
static struct {int32 feature_achievement=1;} battle_config;
#include "chrif-achievement-channel.inc"
static int32 invoke_achievement_channel(map_session_data* sd,int marker,...){
 va_list ap;va_start(ap,marker);const int32 result=chrif_recover_achievement_channel(sd,ap);va_end(ap);return result;
}
struct s_achievement_db {int32 score=0;};
static struct FakeAchievementDb {
 std::map<int32,std::shared_ptr<s_achievement_db>> rows;
 std::shared_ptr<s_achievement_db> find(int32 id){auto it=rows.find(id);return it==rows.end()?nullptr:it->second;}
} achievement_db;
enum e_achievement_group {AG_NONE=0,AG_ONE=1,AG_MAX=2};
static void achievement_update_objective(map_session_data*,e_achievement_group,uint8,...){++objective_checks;}
static std::vector<AchievementEvent> replayed;
static void achievement_update_objective_values(map_session_data*,e_achievement_group group,const std::vector<int32>& arguments){
 ++objective_checks;replayed.push_back({static_cast<int32>(group),arguments});
}
static int reward_callbacks=0,last_reward_id=0;
static time_t last_rewarded=0;
static void achievement_get_reward(map_session_data*,int32 id,time_t rewarded){
 ++reward_callbacks;last_reward_id=id;last_rewarded=rewarded;
}
static void achievement_level(map_session_data*,bool){++level_checks;}
static void achievement_get_titles(uint32){++title_checks;}
static void clif_achievement_update(map_session_data*,achievement*,int32){++client_updates;}
static void clif_achievement_list_all(map_session_data*){++client_lists;}
static bool logout_ack=false,logout_routed=false,logout_success=false;
static uint32 logout_account=0,logout_char=0;
static uint64 logout_generation=0;
static bool chrif_auth_achievement_saved(uint32 account_id,uint32 char_id,uint64 generation,bool success){
 logout_routed=true;logout_account=account_id;logout_char=char_id;logout_generation=generation;logout_success=success;return logout_ack;
}
#include "map-handler.inc"
#include "map-load-handler.inc"
static int inter_fd=0;
static int CheckForCharServer(){return 0;}
#include "map-request-handler.inc"
#include "map-save-handler.inc"
#include "map-reward-handler.inc"
static void require(bool condition,const char* label){if(!condition){std::cerr<<"FAIL: "<<label<<'\n';std::exit(1);}}
static void seed(){
 persisted.clear();incoming.clear();reply=ack_success=false;deleted=0;
 load_ok=engine_ok=begin_ok=end_ok=true;
 for(int id:{1,2,3}){achievement row{};row.achievement_id=id;row.count[0]=10;persisted[id]=row;incoming.push_back(row);}
 original=persisted;
 incoming[0].count[0]=20;
 incoming[1].count[0]=30;incoming[2].achievement_id=4;
 RFIFOW(0,2)=8+incoming.size()*sizeof(achievement);RFIFOL(0,4)=99001;
 std::memcpy(RFIFOP(0,8),incoming.data(),incoming.size()*sizeof(achievement));
}
int main(){
 const size_t max_load_rows=pn_achievement_protocol::max_logout_rows(sizeof(achievement));
 persisted.clear();
 for(size_t i=0;i<=max_load_rows;++i){achievement value{};value.achievement_id=static_cast<int32>(i+1);value.count[0]=1;persisted[value.achievement_id]=value;}
 std::memset(buffer,0,sizeof(buffer));fifo_writes=last_fifo_size=0;int prior_warnings=warnings;
 mapif_achievement_load(0,99001);
 require(fifo_writes==1&&last_fifo_size==static_cast<int>(pn_achievement_protocol::response_size)&&
  RFIFOW(0,0)==pn_achievement_protocol::load_response&&RFIFOW(0,2)==pn_achievement_protocol::response_size&&
  RFIFOL(0,4)==99001&&RFIFOB(0,8)==pn_achievement_protocol::version&&!RFIFOB(0,9)&&warnings==prior_warnings+1,
  "oversized achievement load emits a bounded identified failure frame");
 int prior_eof=eof_calls;RFIFOB(0,6)=pn_achievement_protocol::version+1;
 mapif_parse_achievement_load(0);
 require(eof_calls==prior_eof+1&&last_eof_fd==0,
  "unsupported map-to-char achievement load version closes the inter-server stream");

 persisted.clear();
 for(size_t i=0;i<max_load_rows;++i){achievement value{};value.achievement_id=static_cast<int32>(i+1);value.count[0]=1;persisted[value.achievement_id]=value;}
 std::vector<achievement> cap_submission;
 for(const auto& pair:persisted)cap_submission.push_back(pair.second);
 cap_submission.back().achievement_id=static_cast<int32>(max_load_rows+1);
 require(!mapif_achievement_save_rows(99001,cap_submission.data(),static_cast<int32>(cap_submission.size()))&&
  persisted.size()==max_load_rows&&persisted.count(static_cast<int>(max_load_rows))&&
  !persisted.count(static_cast<int>(max_load_rows+1)),
  "monotonic merge refuses a known replacement that would preserve an opaque row beyond the wire cap");

 seed();failed_id=0;mapif_parse_achievement_save(0);
 require(reply&&ack_success&&persisted[1].count[0]==20&&persisted.count(2)&&persisted.count(3)&&persisted.count(4),
  "ordinary save succeeds without deleting durable progress omitted by a stale snapshot");
 seed();failed_id=0;fifo_writes=0;const uint64 final_generation=0x1122334455667788ULL;
 RFIFOW(0,2)=pn_achievement_protocol::logout_request_size+incoming.size()*sizeof(achievement);
 RFIFOL(0,4)=77;RFIFOL(0,8)=99001;RFIFOB(0,12)=pn_achievement_protocol::version;RFIFOQ(0,16)=final_generation;
 std::memcpy(RFIFOP(0,pn_achievement_protocol::logout_request_size),incoming.data(),incoming.size()*sizeof(achievement));
 mapif_parse_achievement_logout_save(0);
 require(fifo_writes==1&&last_fifo_size==static_cast<int>(pn_achievement_protocol::logout_response_size)&&
  RFIFOW(0,0)==pn_achievement_protocol::logout_save_response&&RFIFOB(0,2)==pn_achievement_protocol::version&&
  RFIFOB(0,3)==1&&RFIFOL(0,4)==77&&RFIFOL(0,8)==99001&&RFIFOQ(0,16)==final_generation&&
  persisted[1].count[0]==20,"tokenized logout save commits and echoes its complete identity");
 seed();persisted[1].count[0]=50;persisted[1].completed=111;persisted[1].rewarded=222;
 incoming[0].count[0]=20;incoming[0].completed=0;incoming[0].rewarded=0;
 std::memcpy(RFIFOP(0,8),incoming.data(),incoming.size()*sizeof(achievement));
 mapif_parse_achievement_save(0);
 require(reply&&ack_success&&persisted[1].count[0]==50&&persisted[1].completed==111&&persisted[1].rewarded==222,
  "ordinary stale snapshot cannot roll back durable counters, completion, or reward identity");
 persisted.clear();achievement known{};known.achievement_id=1;known.count[0]=5;persisted[1]=known;
 achievement opaque{};opaque.achievement_id=88;opaque.count[0]=9;opaque.completed=333;opaque.rewarded=444;persisted[88]=opaque;
 achievement before=known,after=known;after.count[0]=6;
 require(mapif_achievement_compare_replace_locked(99001,&before,1,&after,1)==AchievementCompareReplaceResult::Applied&&
  persisted[1].count[0]==6&&persisted.count(88)&&std::memcmp(&persisted[88],&opaque,sizeof(opaque))==0,
  "shop compare-replace advances known progress while preserving an omitted opaque row exactly");
 for(int fault=1;fault<=7;++fault){
  seed();failed_id=fault<=4?fault:0;
  failed_id=fault==1?1:fault==2?2:fault==3?4:0;
  if(fault==4)load_ok=false;if(fault==5)engine_ok=false;if(fault==6)begin_ok=false;if(fault==7)end_ok=false;
  mapif_parse_achievement_save(0);
  require(reply&&!ack_success,"failed update must not become successful delete ACK");
  require(persisted.size()==original.size()&&persisted.count(3)&&persisted[1].count[0]==10&&persisted[2].count[0]==10,"failed save preserves all prior achievements");
 }
 seed();persisted[3].count[0]=0;original=persisted;failed_id=3;mapif_parse_achievement_save(0);
 require(reply&&!ack_success&&persisted.size()==original.size()&&persisted.count(3)&&!persisted[3].count[0],
  "failed cleanup of a legacy empty row rolls back every preceding write");
 seed();failed_id=0;RFIFOW(0,2)=7;mapif_parse_achievement_save(0);require(!ack_success,"short payload refused");
 seed();RFIFOW(0,2)-=1;mapif_parse_achievement_save(0);require(!ack_success,"partial record refused");
 seed();incoming[1].achievement_id=1;std::memcpy(RFIFOP(0,8),incoming.data(),incoming.size()*sizeof(achievement));mapif_parse_achievement_save(0);require(!ack_success,"duplicate identity refused");
 int prior_errors=errors;logout_ack=true;logout_routed=false;player.achievement_data.save=false;RFIFOL(0,2)=99001;RFIFOB(0,6)=1;intif_parse_achievementsave(0);
 require(!logout_routed&&!player.achievement_data.save&&errors==prior_errors,
  "ordinary autosave acknowledgement cannot route into retained logout state");
 logout_ack=false;player.achievement_data.save=false;RFIFOB(0,6)=0;intif_parse_achievementsave(0);
 require(player.achievement_data.save,"negative ACK retains dirty progression for retry");
 RFIFOB(0,6)=1;intif_parse_achievementsave(0);
 require(player.achievement_data.save,"older success ACK cannot clear newer dirty progress");
 logout_ack=true;logout_routed=false;RFIFOB(0,2)=pn_achievement_protocol::version;RFIFOB(0,3)=1;
 RFIFOL(0,4)=77;RFIFOL(0,8)=99001;RFIFOQ(0,16)=final_generation;intif_parse_achievementlogoutsave(0);
 require(logout_routed&&logout_account==77&&logout_char==99001&&logout_generation==final_generation&&logout_success,
  "dedicated logout acknowledgement routes the echoed account, character, token, and result");

 achievement_db.rows.clear();
 for(int id:{1,2}){auto value=std::make_shared<s_achievement_db>();value->score=id*100;achievement_db.rows[id]=value;}
 if(player.achievement_data.achievements)std::free(player.achievement_data.achievements);
 CREATE(player.achievement_data.achievements,achievement,4);player.achievement_data.count=4;player.achievement_data.incompleteCount=4;player.achievement_data.loaded=false;
 player.shop_commit.deferred_achievements.push_back({AG_ONE,{77}});
 std::vector<achievement> loaded(3);loaded[0].achievement_id=1;loaded[0].count[0]=5;loaded[1].achievement_id=999;
 loaded[2].achievement_id=2;loaded[2].completed=1234;
 RFIFOW(0,2)=pn_achievement_protocol::response_size+loaded.size()*sizeof(achievement);RFIFOL(0,4)=99001;
 RFIFOB(0,8)=pn_achievement_protocol::version;RFIFOB(0,9)=1;
 std::memcpy(RFIFOP(0,pn_achievement_protocol::response_size),loaded.data(),loaded.size()*sizeof(achievement));
 prior_errors=errors;intif_parse_achievements(0);
 require(player.achievement_data.loaded&&player.achievement_data.count==2&&player.achievement_data.incompleteCount==1&&
  player.achievement_data.opaque_count==1,
  "nonempty refresh resets stale counters and skips unknown IDs");
 require(player.achievement_data.achievements[0].achievement_id==1&&player.achievement_data.achievements[0].score==100&&
  player.achievement_data.achievements[1].achievement_id==2&&player.achievement_data.achievements[1].score==200&&errors==prior_errors+1,
  "refresh compacts accepted incomplete and completed rows in order");
 require(player.shop_commit.deferred_achievements.empty()&&replayed.size()==1&&replayed[0].group==AG_ONE&&
  replayed[0].arguments==std::vector<int32>{77},"initial-load events replay once after the authoritative snapshot");
 loaded.resize(1);loaded[0]={};loaded[0].achievement_id=2;loaded[0].completed=5678;
 RFIFOW(0,2)=pn_achievement_protocol::response_size+sizeof(achievement);
 std::memcpy(RFIFOP(0,pn_achievement_protocol::response_size),loaded.data(),sizeof(achievement));
 intif_parse_achievements(0);
 require(player.achievement_data.count==1&&player.achievement_data.incompleteCount==0&&player.achievement_data.opaque_count==0&&
  player.achievement_data.achievements[0].completed==5678,
  "second nonempty refresh cannot retain prior counts or rows");
 RFIFOW(0,2)=pn_achievement_protocol::response_size;RFIFOB(0,8)=pn_achievement_protocol::version;RFIFOB(0,9)=1;
 intif_parse_achievements(0);
 require(!player.achievement_data.achievements&&!player.achievement_data.count&&!player.achievement_data.incompleteCount,
  "authoritative empty refresh clears ownership and counters");
 achievement previous{};previous.achievement_id=1;CREATE(player.achievement_data.achievements,achievement,1);player.achievement_data.achievements[0]=previous;
 player.achievement_data.count=player.achievement_data.incompleteCount=1;player.achievement_data.loaded=true;player.achievement_data.save=true;
 player.shop_commit.deferred_achievements.push_back({AG_ONE,{88}});
 RFIFOW(0,2)=pn_achievement_protocol::response_size;RFIFOB(0,8)=pn_achievement_protocol::version;RFIFOB(0,9)=0;
 prior_eof=eof_calls;
 intif_parse_achievements(0);
 require(eof_calls==prior_eof+1&&last_eof_fd==player.fd&&!player.achievement_data.loaded&&!player.achievement_data.save&&
  player.shop_commit.deferred_achievements.empty()&&player.achievement_data.count==1&&player.achievement_data.achievements[0].achievement_id==1,
  "SQL load failure invalidates save authority and disconnects without replacing memory");
 player.achievement_data.loaded=true;player.achievement_data.save=true;
 player.shop_commit.deferred_achievements.push_back({AG_ONE,{99}});
 RFIFOW(0,2)=8;RFIFOL(0,4)=99001;prior_errors=errors;intif_parse_achievements(0);
 require(errors==prior_errors+1&&eof_calls==prior_eof+2&&last_eof_fd==player.fd&&!player.achievement_data.loaded&&
  !player.achievement_data.save&&player.shop_commit.deferred_achievements.empty()&&player.achievement_data.count==1,
  "malformed identified load frame invalidates save authority and disconnects before unsigned length math");
 player.achievement_data.save=true;fifo_writes=0;
 require(intif_achievement_save(&player)==0&&!player.achievement_data.save&&fifo_writes==0,
  "an invalidated bootstrap cache cannot be serialized even if marked dirty");
 player.achievement_data.loaded=true;player.achievement_data.save=true;
 player.achievement_data.count=static_cast<uint16>((std::numeric_limits<uint16>::max()-8)/sizeof(achievement)+1);
 prior_eof=eof_calls;fifo_writes=0;
 require(intif_achievement_save(&player)==0&&!player.achievement_data.loaded&&!player.achievement_data.save&&
  fifo_writes==0&&eof_calls==prior_eof+1&&last_eof_fd==player.fd,
  "oversized achievement save disconnects before truncating the 16-bit frame length");
 player.achievement_data.count=1;
	RFIFOW(0,2)=7;prior_errors=errors;prior_eof=eof_calls;intif_parse_achievements(0);
	require(errors==prior_errors+1&&eof_calls==prior_eof+1&&last_eof_fd==0,
  "an unidentifiable truncated load frame resets the inter-server stream");
 prior_eof=eof_calls;player.achievement_data.loaded=true;player.achievement_data.save=true;
 player.achievement_data.reward_pending_id=220023;player.shop_commit.deferred_achievements.push_back({AG_ONE,{101}});
 invoke_achievement_channel(&player,0);
 require(eof_calls==prior_eof+1&&!player.achievement_data.loaded&&!player.achievement_data.save&&
  !player.achievement_data.reward_pending_id&&player.shop_commit.deferred_achievements.empty(),
  "lost reward reply invalidates and disconnects the stale map cache without saving it");
 prior_eof=eof_calls;player.achievement_data.loaded=true;player.achievement_data.save=false;
 invoke_achievement_channel(&player,0);
 require(eof_calls==prior_eof&&player.achievement_data.loaded&&player.achievement_data.save,
  "lost ordinary save acknowledgement marks an authoritative cache for monotonic retry");
 battle_config.feature_achievement=0;player.achievement_data.loaded=false;player.achievement_data.reward_pending_id=7;
 invoke_achievement_channel(&player,0);
 require(eof_calls==prior_eof&&!player.achievement_data.reward_pending_id,
  "disabled achievement mode does not disconnect players during char-link recovery");
 replayed.clear();player.achievement_data.reward_pending_id=220023;
 player.shop_commit.deferred_achievements.push_back({AG_ONE,{121}});
 RFIFOL(0,2)=99001;RFIFOL(0,6)=220023;RFIFOL(0,10)=1700000200;
 intif_parse_achievementreward(0);
 require(!player.achievement_data.reward_pending_id&&replayed.size()==1&&
  replayed[0].arguments==std::vector<int32>{121}&&reward_callbacks==1&&
  last_reward_id==220023&&last_rewarded==1700000200,
  "reward ACK clears its fence, replays queued progression once, then applies the reward callback");
 std::memset(buffer,0,sizeof(buffer));fifo_writes=last_fifo_size=0;
 intif_request_achievements(99001);
 require(fifo_writes==1&&last_fifo_size==static_cast<int>(pn_achievement_protocol::request_size)&&
  RFIFOW(0,0)==pn_achievement_protocol::load_request&&RFIFOL(0,2)==99001&&
  RFIFOB(0,6)==pn_achievement_protocol::version,
  "achievement load request uses the dedicated versioned packet identity");
 std::vector<achievement> boundary(max_load_rows);
 fifo_writes=0;
 require(intif_achievement_logout_save(77,99001,99,boundary.data(),static_cast<uint16>(boundary.size()))==1&&
  fifo_writes==1&&last_fifo_size==static_cast<int>(pn_achievement_protocol::logout_request_size+
   boundary.size()*sizeof(achievement)),"largest loadable snapshot fits the tokenized logout frame");
 boundary.push_back({});fifo_writes=0;
 require(intif_achievement_logout_save(77,99001,100,boundary.data(),static_cast<uint16>(boundary.size()))==0&&
  fifo_writes==0,"snapshot above the shared load/logout limit is rejected before 16-bit truncation");
 std::free(player.achievement_data.achievements);player.achievement_data.achievements=nullptr;
 require(objective_checks>0&&level_checks==3&&title_checks==3&&client_updates==3&&client_lists==3,
  "each successful load runs the normal objective and client refresh path");
 std::cout<<"PASS achievement atomic save, retry, and repeat-load safety\n";
}
