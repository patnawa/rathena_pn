#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using int32 = int32_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
static constexpr size_t NAME_LENGTH = 24;
static constexpr size_t ACHIEVEMENT_NAME_LENGTH = 50;
enum e_achievement_group : int32 { AG_NONE=0, AG_ONE=1 };
struct AchievementEvent { int32 group=0; std::vector<int32> arguments; };

struct map_session_data {
 struct { uint32 char_id=9900101; char name[NAME_LENGTH]="Reward fixture"; } status;
 struct { bool loaded=true; int32 reward_pending_id=0; uint16 count=0; struct achievement* achievements=nullptr; } achievement_data;
 struct { std::vector<AchievementEvent> deferred_achievements; } shop_commit;
 int32 fd=9;
};
struct achievement { int32 achievement_id=0; int32 completed=0; int32 rewarded=0; };
struct s_achievement_db {
 uint32 achievement_id=0;
 std::string name;
 struct { uint32 nameid=0; uint16 amount=0; } rewards;
};

static bool disconnected=false,online=true;
static map_session_data player;
static unsigned char inbound[256]{},outbound[256]{};
static std::vector<std::vector<unsigned char>> frames;
static int32 applied_id=0;
static uint32 applied_rewarded=0;
static unsigned applies=0;
static int32 inter_fd=7;
static bool other_transaction_pending=false;
static unsigned client_acks=0;
static uint32 last_client_result=0;
static int32 last_client_id=0;
static std::vector<int32> callback_order;

struct AchievementDbFixture {
 std::shared_ptr<s_achievement_db> value;
 std::shared_ptr<s_achievement_db> find(int32 id){return value&&value->achievement_id==static_cast<uint32>(id)?value:nullptr;}
} achievement_db;

#define ARR_FIND(start,end,iterator,condition) for((iterator)=(start);(iterator)<(end)&&!(condition);++(iterator))
#define nullpo_retv(value) do { if(!(value)) return; } while(false)
#define RFIFOP(fd,pos) ((void)(fd),inbound+(pos))
#define RFIFOL(fd,pos) (*reinterpret_cast<uint32*>(RFIFOP(fd,pos)))
#define WFIFOP(fd,pos) ((void)(fd),outbound+(pos))
#define WFIFOCP(fd,pos) reinterpret_cast<char*>(WFIFOP(fd,pos))
#define WFIFOW(fd,pos) (*reinterpret_cast<uint16*>(WFIFOP(fd,pos)))
#define WFIFOL(fd,pos) (*reinterpret_cast<uint32*>(WFIFOP(fd,pos)))
#define WFIFOHEAD(fd,len) ((void)(fd),(void)(len))
#define WFIFOSET(fd,len) ((void)(fd),frames.emplace_back(outbound,outbound+(len)))

static bool CheckForCharServer(){return disconnected;}
static bool pc_transaction_pending(const map_session_data* sd){
 return other_transaction_pending||sd->achievement_data.reward_pending_id!=0;
}
static char* safestrncpy(char* destination,const char* source,size_t size){
 if(!size)return destination;
 std::strncpy(destination,source,size-1);destination[size-1]='\0';return destination;
}
static map_session_data* map_charid2sd(uint32 char_id){return online&&char_id==player.status.char_id?&player:nullptr;}
static void achievement_get_reward(map_session_data* sd,int32 achievement_id,uint32 rewarded){
 assert(sd==&player);callback_order.push_back(2);++applies;applied_id=achievement_id;applied_rewarded=rewarded;
}
static void achievement_update_objective_values(map_session_data* sd,e_achievement_group group,const std::vector<int32>& arguments){
 assert(sd==&player&&group==AG_ONE&&arguments==std::vector<int32>({77}));callback_order.push_back(1);
}
static void clif_achievement_reward_ack(int32,uint32 result,int32 achievement_id){
 ++client_acks;last_client_result=result;last_client_id=achievement_id;
}
static void ShowError(const char*,...){}

#include "achievement-reward-send.inc"
#include "achievement-reward-check.inc"
#include "achievement-reward-ack.inc"

static uint32 read32(const std::vector<unsigned char>& bytes,size_t offset){
 uint32 value=0;std::memcpy(&value,bytes.data()+offset,sizeof(value));return value;
}
static uint16 read16(const std::vector<unsigned char>& bytes,size_t offset){
 uint16 value=0;std::memcpy(&value,bytes.data()+offset,sizeof(value));return value;
}
static void ack(uint32 char_id,int32 achievement_id,uint32 rewarded){
 std::memset(inbound,0,sizeof(inbound));std::memcpy(inbound+2,&char_id,sizeof(char_id));
 std::memcpy(inbound+6,&achievement_id,sizeof(achievement_id));
 std::memcpy(inbound+10,&rewarded,sizeof(rewarded));intif_parse_achievementreward(0);
}

int main(){
 s_achievement_db first{};first.achievement_id=7001;first.name="First reward";first.rewards.nameid=501;first.rewards.amount=3;
 s_achievement_db second{};second.achievement_id=7002;second.name="Second reward";second.rewards.nameid=502;second.rewards.amount=4;
 achievement_db.value=std::make_shared<s_achievement_db>(first);
 achievement row{};row.achievement_id=7001;row.completed=1700000000;
 player.achievement_data.achievements=&row;player.achievement_data.count=1;

 player.achievement_data.loaded=false;
 achievement_check_reward(&player,7001);
 assert(frames.empty()&&player.achievement_data.reward_pending_id==0&&client_acks==1&&last_client_result==0&&last_client_id==7001);
 player.achievement_data.loaded=true;other_transaction_pending=true;
 achievement_check_reward(&player,7001);
 assert(frames.empty()&&player.achievement_data.reward_pending_id==0&&client_acks==2&&last_client_result==0);
 other_transaction_pending=false;
 achievement_check_reward(&player,7001);
 assert(frames.size()==1&&player.achievement_data.reward_pending_id==7001&&client_acks==2);
 achievement_check_reward(&player,7001);
 assert(frames.size()==1&&player.achievement_data.reward_pending_id==7001&&client_acks==3&&last_client_result==0);
 frames.clear();player.achievement_data.reward_pending_id=0;client_acks=0;

 disconnected=true;
 assert(!intif_achievement_reward(&player,&first));
 assert(frames.empty()&&player.achievement_data.reward_pending_id==0);

 disconnected=false;
 assert(intif_achievement_reward(&player,&first)==1);
 assert(frames.size()==1&&player.achievement_data.reward_pending_id==7001);
 const auto& request=frames.back();
 assert(request.size()==16+NAME_LENGTH+ACHIEVEMENT_NAME_LENGTH);
 assert(read16(request,0)==0x3064&&read32(request,2)==player.status.char_id&&read32(request,6)==7001);
 assert(read32(request,10)==501&&read16(request,14)==3);
 assert(std::string(reinterpret_cast<const char*>(request.data()+16))=="Reward fixture");
 assert(std::string(reinterpret_cast<const char*>(request.data()+16+NAME_LENGTH))=="First reward");

 // A second claim cannot overwrite the identity used to match the first ACK.
 assert(!intif_achievement_reward(&player,&second));
 assert(frames.size()==1&&player.achievement_data.reward_pending_id==7001);

 ack(player.status.char_id+1,7001,1700000001);
 assert(player.achievement_data.reward_pending_id==7001&&applies==0);
 ack(player.status.char_id,7002,1700000002);
 assert(player.achievement_data.reward_pending_id==7001&&applies==0);
	player.shop_commit.deferred_achievements.push_back({AG_ONE,{77}});callback_order.clear();
 ack(player.status.char_id,7001,1700000003);
	assert(player.achievement_data.reward_pending_id==0&&player.shop_commit.deferred_achievements.empty()&&
	 callback_order==std::vector<int32>({1,2})&&applies==1&&applied_id==7001&&applied_rewarded==1700000003);
 ack(player.status.char_id,7001,1700000003);
 assert(player.achievement_data.reward_pending_id==0&&applies==1);

 assert(intif_achievement_reward(&player,&second)==1);
 assert(frames.size()==2&&player.achievement_data.reward_pending_id==7002);
 online=false;ack(player.status.char_id,7002,1700000004);
 assert(player.achievement_data.reward_pending_id==7002&&applies==1);
 online=true;ack(player.status.char_id,7002,1700000004);
 assert(player.achievement_data.reward_pending_id==0&&applies==2&&applied_id==7002&&applied_rewarded==1700000004);

 std::cout<<"PASS achievement reward pending fence, request wire identity, ACK identity and duplicate rejection\n";
}
