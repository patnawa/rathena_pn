#include <custom/shop_state.hpp>
#include <custom/pet_floor.hpp>
#include <cassert>
#include <iostream>
#include <functional>
#include <unordered_map>
using int32=int32_t;using uint16=uint16_t;
constexpr int CARD0_PET=0xff00,PET_EGG=0,ADDITEM_INVALID=1,BL_ITEM=2,ITEMOBTAIN_TYPE_MONSTER_ITEM=1;
class map_session_data {public:struct {uint32_t account_id=7,char_id=8,party_id=0;}status;int m=1;bool refused=false,dead=false,idle=false;pn_shop_state shop_commit;};
using TBL_PC=map_session_data;
struct flooritem_data {int32 id=10,type=BL_ITEM,cleartimer=23;uint64_t pet_claim_token=0;uint32_t first_get_charid=8;struct item item{};uint16 mob_id=1002;};
struct party_data {struct {int item=2;}party;struct {map_session_data* sd=nullptr;}data[MAX_PARTY];int itemc=0;};
struct Data {int type=IT_PETEGG;struct {bool broadcast=true;}flag;};
static struct {std::shared_ptr<Data> find(uint32_t id){return id==9001?std::make_shared<Data>():nullptr;}}item_db;
static struct {int party_show_share_picker=1,show_picker_item_type=1<<IT_PETEGG,party_share_type=2,idle_no_share=1;}battle_config;
static party_data* group=nullptr;
static std::unordered_map<uint32_t,map_session_data*> players;
static std::unordered_map<int,flooritem_data*> floors;
static int animations=0,clears=0,notices=0,mvp_notices=0,broadcasts=0,dispatches=0;
static bool accept=true;
static uint64_t next_sequence=0;
static pn_shop::Grant captured;
static int itemdb_type(uint32_t){return IT_PETEGG;}
static bool pet_db_search(uint32_t id,int){return id==9001;}
static party_data* party_search(uint32_t id){return id?group:nullptr;}
static bool pc_isdead(map_session_data* sd){return sd->dead;}
static bool pc_isidle_party(map_session_data* sd){return sd->idle;}
static int rnd_value(int a,int){return a;}
static void clif_party_show_picker(map_session_data*,item*){++notices;}
static map_session_data* map_id2sd(uint32_t id){auto it=players.find(id);return it==players.end()?nullptr:it->second;}
static flooritem_data* map_id2bl(int id){auto it=floors.find(id);return it==floors.end()?nullptr:it->second;}
#define BL_CAST(type,pointer) (pointer)
static void unit_stop_attack(map_session_data*){}
static void clif_takeitem(map_session_data&,flooritem_data&){++animations;}
static void clif_mvp_item(map_session_data*,uint32_t){++mvp_notices;}
static void intif_broadcast_obtain_special_item(map_session_data*,uint32_t,uint16,int){++broadcasts;}
static void map_clearflooritem(flooritem_data* floor){pn_pet_floor_removed(*floor);floors.erase(floor->id);++clears;}
std::shared_ptr<pn_shop::Commit> pn_shop_request(const map_session_data& sd,uint32_t kind){auto r=std::make_shared<pn_shop::Commit>();r->account_id=sd.status.account_id;r->char_id=sd.status.char_id;r->kind=kind;return r;}
bool pn_shop_begin(map_session_data& sd,std::shared_ptr<pn_shop::Commit> r,const std::vector<pn_shop::Grant>& grants,const uint32_t*){
 ++dispatches;if(!accept || sd.refused)return false;r->sequence=++next_sequence;r->nonce_hi=17;r->nonce_lo=18;sd.shop_commit.request=r;sd.shop_commit.pending=true;captured=grants.at(0);return true;
}
#include "party_loot_body.inc"
#include "pet_floor_body.inc"
#undef idb_get
TIMER_FUNC(map_clearflooritem_timer);
static int id_db=0,expired_pets=0;
static flooritem_data* idb_get(int,int id){return map_id2bl(id);}
static void ShowError(const char*){assert(false);}
int32 add_timer(t_tick,TimerFunc,int32,intptr_t){return 99;}
static void intif_delete_petdata(uint32_t){++expired_pets;}
static uint32_t MakeDWord(uint32_t lo,uint32_t hi){return lo|(hi<<16);}
static void clif_clearflooritem(flooritem_data&){++clears;}
static void map_deliddb(flooritem_data* floor){floors.erase(floor->id);}
static void map_delblock(flooritem_data*){}
static void map_freeblock(flooritem_data*){}
#include "floor_timer_body.inc"

static pn_shop::Ack reply(map_session_data& sd,uint32_t outcome){const auto& r=*sd.shop_commit.request;pn_shop::Ack a;a.account_id=r.account_id;a.char_id=r.char_id;a.nonce_hi=r.nonce_hi;a.nonce_lo=r.nonce_lo;a.sequence=r.sequence;a.outcome=outcome;return a;}
int main(){
 unsigned cases=0;map_session_data sd;flooritem_data floor;
 auto reset=[&](){pn_pet_floor_claims.clear();players.clear();floors.clear();sd={};floor={};floor.item.nameid=9001;floor.item.amount=2;floor.item.refine=7;floor.item.bound=2;floor.item.option[0].id=1;players[7]=&sd;floors[10]=&floor;group=nullptr;animations=clears=notices=mvp_notices=broadcasts=dispatches=0;accept=true;};
 reset();++cases;assert(pn_pet_floor_take(sd,floor));assert(floor.pet_claim_token&&floors.count(10)&&!animations&&!clears&&!broadcasts);assert(captured.amount==2&&captured.prototype.refine==7&&captured.prototype.bound==2&&captured.prototype.option[0].id==1);assert(!pn_pet_floor_take(sd,floor)&&dispatches==1);
 auto a=reply(sd,pn_shop::Committed);auto bad=a;bad.nonce_hi++;assert(!pn_pet_floor_ack(bad));bad=a;bad.char_id++;assert(!pn_pet_floor_ack(bad));bad=a;bad.outcome=pn_shop::Retry;assert(!pn_pet_floor_ack(bad));assert(floors.count(10));assert(pn_pet_floor_ack(a));assert(clears==1&&animations==1&&broadcasts==1&&!floors.count(10));assert(!pn_pet_floor_ack(a)&&clears==1);
 reset();++cases;accept=false;assert(!pn_pet_floor_take(sd,floor));assert(!floor.pet_claim_token&&floors.count(10)&&!clears&&!animations);
 reset();++cases;assert(pn_pet_floor_take(sd,floor));a=reply(sd,pn_shop::Rejected);assert(pn_pet_floor_ack(a)&&floors.count(10)&&!floor.pet_claim_token&&!animations&&!clears);assert(pn_pet_floor_take(sd,floor));assert(!pn_pet_floor_ack(a));
 reset();++cases;assert(pn_pet_floor_take(sd,floor));a=reply(sd,pn_shop::Committed);auto saved=sd.shop_commit.request;players.clear();sd.shop_commit={};assert(pn_pet_floor_pending(7,a.sequence)==saved);assert(!pn_pet_floor_pending(99,a.sequence));assert(pn_pet_floor_ack(a)&&clears==1&&!animations&&!broadcasts);assert(!pn_pet_floor_pending(7,a.sequence));
 reset();++cases;assert(pn_pet_floor_take(sd,floor));a=reply(sd,pn_shop::Committed);map_clearflooritem(&floor);flooritem_data replacement;replacement.id=10;floors[10]=&replacement;assert(pn_pet_floor_ack(a));assert(floors[10]==&replacement&&clears==1);
 reset();++cases;assert(pn_pet_floor_take(sd,floor,true));a=reply(sd,pn_shop::Committed);sd.status.char_id++;assert(pn_pet_floor_ack(a)&&!animations&&!mvp_notices&&!broadcasts&&clears==1);
 reset();++cases;floor.item.card[0]=CARD0_PET;assert(!pn_pet_floor_raw(floor.item)&&!pn_pet_floor_take(sd,floor)&&!dispatches);floor.item.card[0]=0;floor.item.nameid=501;assert(!pn_pet_floor_raw(floor.item)&&!dispatches);
 for(bool random:{false,true}){reset();++cases;party_data party;group=&party;sd.status.party_id=1;map_session_data busy,recipient;busy.refused=true;busy.status.account_id=9;recipient.status.account_id=10;recipient.status.char_id=11;recipient.status.party_id=1;players[10]=&recipient;party.data[1].sd=&busy;party.data[2].sd=&recipient;battle_config.party_share_type=random?0:2;assert(pn_pet_floor_take(sd,floor));assert(!notices&&!mvp_notices);a=reply(recipient,pn_shop::Committed);assert(pn_pet_floor_ack(a));assert(notices==1&&mvp_notices==0&&clears==1);}
 reset();++cases;assert(pn_pet_floor_take(sd,floor,true));assert(!mvp_notices);assert(pn_pet_floor_ack(reply(sd,pn_shop::Committed))&&mvp_notices==1);
 reset();++cases;assert(pn_pet_floor_take(sd,floor));assert(map_clearflooritem_timer(23,1000,10,0)==0&&floor.cleartimer==99&&floors.count(10)&&!clears);assert(pn_pet_floor_ack(reply(sd,pn_shop::Rejected)));assert(map_clearflooritem_timer(99,2000,10,0)==0&&!floors.count(10)&&clears==1&&!expired_pets);
 pn_pet_floor_claims.clear();std::cout<<"PET_FLOOR_NATIVE_PASS cases="<<cases<<"; exact floor+party bodies, explicit admission/SQL/world doubles\n";
}
