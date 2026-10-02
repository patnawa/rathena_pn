// Exact production handler; doubles represent only external world/transport APIs.
#include <custom/shop_state.hpp>
#include <custom/registry_save.hpp>
#include <common/runtime_metrics.hpp>
#include <map/log.hpp>
#include <algorithm>
#include <deque>
#include <cassert>
#include <iostream>
#include <string>
using uint64=uint64_t; using int32=int32_t;
enum e_achievement_group {
 AG_NONE=0,AG_ADD_FRIEND,AG_ADVENTURE,AG_BABY,AG_BATTLE,AG_CHATTING,
 AG_CHATTING_COUNT,AG_CHATTING_CREATE,AG_CHATTING_DYING,AG_EAT,AG_GET_ITEM,
 AG_GET_ZENY,AG_GOAL_ACHIEVE,AG_GOAL_LEVEL,AG_GOAL_STATUS,AG_JOB_CHANGE,
 AG_MARRY,AG_PARTY,AG_ENCHANT_FAIL,AG_ENCHANT_SUCCESS,AG_SPEND_ZENY,AG_TAMING,AG_MAX
};
static struct {int32 shop_exp=0,feature_achievement=1;} battle_config;
class map_session_data {
public:
 struct {uint32_t account_id=11,char_id=22,uniqueitem_counter=0; int64_t zeny=100;int inventory_slots=MAX_INVENTORY;} status;
 struct {uint64_t nonce_hi=1,nonce_lo=2;} bank_ui;
 struct {struct {item items_inventory[MAX_INVENTORY]{};} u;} inventory;
 pn_shop_state shop_commit;
 pn_registry::Journal registry_saves;
 struct {bool save=true,loaded=true;int32 reward_pending_id=0;} achievement_data;
 bool vars_ok=true;
 int fd=1;
 int32 cashPoints=50,kafraPoints=20;
 uint32_t weight=0;
 uint32_t max_weight=10000;
};
static map_session_data player;
static std::vector<map_session_data*> other_players;
static void intif_registry_replay(const map_session_data*) {}
static bool connected=true,refresh_ok=true,present=true;
static std::shared_ptr<const pn_shop::Commit> floor_request;
std::shared_ptr<const pn_shop::Commit> pn_pet_floor_pending(uint32_t account,uint64_t sequence){return floor_request && floor_request->account_id==account && floor_request->sequence==sequence?floor_request:nullptr;}
bool pn_pet_floor_ack(const pn_shop::Ack& a){if(!floor_request)return false;const auto& r=*floor_request;
 if(a.account_id!=r.account_id || a.char_id!=r.char_id || a.nonce_hi!=r.nonce_hi || a.nonce_lo!=r.nonce_lo || a.sequence!=r.sequence || (a.outcome!=pn_shop::Committed && a.outcome!=pn_shop::Rejected))return false;floor_request.reset();return true;}
static int saves=0,results=0,callbacks=0,refreshes=0,timers=0,save_result=0;
static bool last_success=false;
static int pet_recoveries=0,pet_messages=0;
bool pn_pet_recover(map_session_data&){++pet_recoveries;return false;}
void clif_displaymessage(int,const char*){++pet_messages;}
static std::vector<std::vector<unsigned char>> frames;
static std::vector<std::string> order;
static bool item_use_collecting=false;
static int item_use_settlements=0;
static bool item_use_committed=false;
static bool progression_prepare_ok=true,progression_apply_ok=true;
static int progression_prepares=0,progression_applies=0,progression_suppressions=0,eof_calls=0;
static std::vector<achievement> capture_before,capture_after,applied_rows;
static std::vector<std::pair<int32,std::vector<int32>>> prepared_achievements;
static std::vector<std::pair<int32,std::vector<int32>>> replayed_achievements;
bool pn_item_use_active(const map_session_data*){return item_use_collecting;}
void pn_item_use_settled(map_session_data& sd,bool committed){assert(!sd.shop_commit.pending);++item_use_settlements;item_use_committed=committed;order.emplace_back("item-use-settled");}
bool achievement_prepare_shop_events(map_session_data& sd,const std::vector<pn_shop::AchievementEvent>& events,
 std::vector<achievement>& before,std::vector<achievement>& after){
 ++progression_prepares;prepared_achievements.clear();
 for(const auto& event:events)prepared_achievements.emplace_back(event.group,event.arguments);
 for(const auto& event:events)if(event.group<=AG_NONE || event.group>=AG_MAX || event.arguments.size()>MAX_ACHIEVEMENT_OBJECTIVES)return false;
 if(!progression_prepare_ok || !sd.achievement_data.loaded)return false;
 before=capture_before;after=capture_after;return true;
}
bool achievement_apply_shop(map_session_data& sd,const std::vector<achievement>& rows){
 assert(sd.shop_commit.pending && sd.shop_commit.applying);
 ++progression_applies;applied_rows=rows;order.emplace_back("achievement-apply");return progression_apply_ok;
}
void achievement_update_objective_values(map_session_data*,e_achievement_group group,const std::vector<int32>& values){
 replayed_achievements.emplace_back(static_cast<int32>(group),values);order.emplace_back("achievement-replay");
}
void set_eof(int fd){assert(fd==player.fd);++eof_calls;order.emplace_back("eof");}
static unsigned char outbound[65536],inbound[65536];
static int inter_fd=1;
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,n) (outbound+(n))
#define WFIFOSET(fd,n) (frames.emplace_back(outbound,outbound+(n)),order.emplace_back("send"))
#define RFIFOP(fd,n) (inbound+(n))

static map_session_data* map_id2sd(int id){
 if(present && id==int(player.status.account_id))return &player;
 for(auto* p:other_players)if(id==int(p->status.account_id))return p;
 return nullptr;
}
static bool CheckForCharServer(){return !connected;}
static bool chrif_isconnected(){return connected;}
static bool pc_transaction_pending(map_session_data* sd){return sd->shop_commit.pending || sd->achievement_data.reward_pending_id;}
static int rnd(){return 42;}
static constexpr int CSAVE_NORMAL=1,CSAVE_INVENTORY=2;
static int chrif_save(map_session_data* sd,int){assert(!sd->shop_commit.pending);++saves;order.emplace_back("save");return save_result;}
t_tick gettick(){return 100;}
int32 add_timer(t_tick,TimerFunc,int32,intptr_t){return ++timers;}
bool pn_shop_stock_refresh(const pn_shop::Commit&){++refreshes;return refresh_ok;}
static int64_t current_stock=5;
static bool rebase_ok=true;
bool pn_shop_stock_rebase(pn_shop::Commit& request){
 if(!rebase_ok)return false;
 for(uint32_t i=0;i<request.stock_count;++i){auto& stock=request.stocks[i];const auto wanted=stock.before-stock.after;
  if(current_stock>=wanted){stock.before=current_stock;stock.after=current_stock-wanted;}}
 return true;
}
enum _sp { SP_ZENY=6, SP_WEIGHT=7 };
static const char *CASHPOINT_VAR="#CASHPOINTS",*KAFRAPOINT_VAR="#KAFRAPOINTS";
void log_pick_pc(const map_session_data*,e_log_pick_type,int32,const item*){}
static int32 add_str(const char*){return 1;}
static int64_t point_value=100;
static int64_t pc_readreg2(const map_session_data*,const char*){return point_value;}
static bool set_reg_num(void*,map_session_data*,int64_t,const char*,int64_t value,void*){point_value=value;return true;}
static bool pc_setaccountreg(map_session_data*,int64_t,int64_t){return true;}
void log_cash(const map_session_data*,e_log_pick_type,e_log_cash_type,int32){}
static void pc_setinventorydata(map_session_data&){}
static void pc_setequipindex(map_session_data*){}
void log_zeny(const map_session_data&,e_log_pick_type,uint32,int64){}
static void clif_inventorylist(map_session_data*){}
static void clif_updatestatus(map_session_data&,_sp){}
static void pn_mail_asset_result(map_session_data&,const pn_shop::Commit&,bool){}
static void clif_shop_commit_result(map_session_data& sd,const pn_shop::Commit&,const std::vector<pn_shop::Event>&,bool success){assert(!sd.shop_commit.pending);++results;last_success=success;}
struct Data{};
static struct {std::shared_ptr<Data> find(uint32_t){return std::make_shared<Data>();}} item_db;
struct PcItemDeliveryScope {
 map_session_data& sd;
 int additions=0;
 bool refresh=false,suppressed=false;
 explicit PcItemDeliveryScope(map_session_data& p):sd(p){assert(!sd.shop_commit.pending);}
 ~PcItemDeliveryScope(){
  if(suppressed)assert(progression_applies==1);
  callbacks+=additions+(refresh?1:0);order.emplace_back("delivery-callbacks");
 }
 void added(int,const Data&){assert(sd.inventory.u.items_inventory[0].amount==2);++additions;}
 void suppress_achievements(){suppressed=true;++progression_suppressions;order.emplace_back("achievement-suppress");}
 void refresh_questinfo(){refresh=true;}
};
void pn_shop_committed_effects(map_session_data&,const pn_shop::Commit&){++callbacks;}
#include "shop_inter_body.inc"
static void reset(){pn_shop_queue.clear();other_players.clear();current_stock=5;rebase_ok=true;player={};present=true;floor_request.reset();pn_metrics::runtime={};connected=refresh_ok=true;saves=results=callbacks=refreshes=timers=save_result=0;item_use_settlements=0;item_use_committed=false;frames.clear();order.clear();pn_shop_inflight=false;player.inventory.u.items_inventory[0].nameid=501;player.inventory.u.items_inventory[0].amount=1;
 battle_config.shop_exp=0;battle_config.feature_achievement=1;progression_prepare_ok=progression_apply_ok=true;progression_prepares=progression_applies=progression_suppressions=eof_calls=0;capture_before.clear();capture_after.clear();applied_rows.clear();prepared_achievements.clear();replayed_achievements.clear();}
static std::shared_ptr<pn_shop::Commit> request(uint32_t kind=pn_shop::Market){
 auto r=std::make_shared<pn_shop::Commit>();r->kind=kind;r->account_id=11;r->char_id=22;r->wallet_before=100;r->wallet_after=kind==pn_shop::Sale?100:90;r->cash_before=50;r->cash_after=kind==pn_shop::Sale?40:50;r->kafra_before=r->kafra_after=20;r->stock_count=1;r->stocks[0].key=501;r->stocks[0].before=5;r->stocks[0].after=4;
 if(kind==pn_shop::Sale){r->stocks[0].sale_start=1;r->stocks[0].sale_end=2;}else std::strcpy(r->stocks[0].name,"shop");
 r->items[0]=player.inventory.u.items_inventory[0];r->items[0].amount=2;return r;
}
static pn_shop::Ack ack(uint32_t outcome){auto&r=*player.shop_commit.request;pn_shop::Ack a;a.account_id=r.account_id;a.char_id=r.char_id;a.nonce_hi=r.nonce_hi;a.nonce_lo=r.nonce_lo;a.sequence=r.sequence;a.outcome=outcome;return a;}
static void receive(const pn_shop::Ack& a){std::memcpy(inbound,&a,sizeof(a));intif_parse_ShopCommitted(1);}
static void unchanged(){assert(player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && callbacks==0 && results==0);}
static achievement achievement_row(int32 id,int32 count,time_t completed=0,time_t rewarded=0,int32 score=0){
 achievement row{};row.achievement_id=id;row.count[0]=count;row.completed=completed;row.rewarded=rewarded;row.score=score;return row;
}
static bool same_achievement(const achievement& lhs,const achievement& rhs){
 return lhs.achievement_id==rhs.achievement_id && !std::memcmp(lhs.count,rhs.count,sizeof(lhs.count)) &&
  lhs.completed==rhs.completed && lhs.rewarded==rhs.rewarded && lhs.score==rhs.score;
}
static pn_shop::Event item_event(uint32_t value_sell){
 pn_shop::Event event{};event.index=0;event.amount=1;event.nameid=501;event.value_sell=value_sell;return event;
}
#ifndef PN_TEST_SHOP_QUEUE
int main(){
 item_use_collecting=true;reset();assert(!pn_shop_submit(player,request(),{},0));assert(saves==0 && frames.empty());item_use_collecting=false;
 for(auto kind:{pn_shop::Market,pn_shop::Barter,pn_shop::Sale}){
  reset();auto r=request(kind);assert(pn_shop_submit(player,r,{{0,1,501}},10));unchanged();assert(player.shop_commit.pending && pn_shop_inflight);assert(pn_shop_stock_busy());assert(order==std::vector<std::string>({"save","send"}));auto frozen=frames[0];r->wallet_after=0;r->items[0].amount=99;
  pn_shop_retry(0,1000,11,player.shop_commit.request->sequence);assert(frames.size()==2 && frames[1]==frozen);unchanged();
  connected=false;pn_shop_retry(0,2000,11,player.shop_commit.request->sequence);assert(frames.size()==2);connected=true;
  auto a=ack(pn_shop::Committed);auto bad=a;bad.sequence++;receive(bad);bad=a;bad.char_id++;receive(bad);bad=a;bad.nonce_lo++;receive(bad);bad=a;bad.outcome=pn_shop::Retry;receive(bad);unchanged();assert(refreshes==0);
  refresh_ok=false;receive(a);unchanged();assert(player.shop_commit.pending && pn_shop_inflight);refresh_ok=true;receive(a);
  assert(!player.shop_commit.pending && !pn_shop_inflight && results==1 && last_success && callbacks==3 && player.weight==10 && player.inventory.u.items_inventory[0].amount==2);assert(player.status.zeny==(kind==pn_shop::Sale?100:90));assert(player.cashPoints==(kind==pn_shop::Sale?40:50));
  player.status.zeny=77;receive(a);assert(player.status.zeny==77 && results==1 && callbacks==3);assert(saves==2);
  assert(pn_metrics::runtime.shop_committed==1 && pn_metrics::runtime.shop_ack.count==1 &&
         pn_metrics::runtime.shop_refresh_failures==1 && pn_metrics::runtime.shop_retry_attempts==2 && !pn_metrics::runtime.pending);
  reset();assert(pn_shop_submit(player,request(kind),{},10));a=ack(pn_shop::Rejected);receive(a);assert(results==1 && !last_success && callbacks==0 && player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && !pn_shop_inflight);receive(a);assert(results==1);
 }
 reset();auto pet=request(pn_shop::Asset);pet->stock_count=0;pet->cash_after=40;
 pet->pet_count=1;pet->pets[0].output.egg.nameid=9001;pet->pets[0].output.egg.amount=1;
 pet->pets[0].output.pet_class=1002;pet->pets[0].output.level=1;std::strcpy(pet->pets[0].output.name,"Poring");
 assert(pn_shop_submit(player,pet,{{-1,1,9001}},0));auto pet_ack=ack(pn_shop::Committed);receive(pet_ack);
 assert(player.cashPoints==40 && player.status.zeny==90 && callbacks==2 && pet_recoveries==1 && pet_messages==1);
 receive(pet_ack);assert(pet_recoveries==1 && pet_messages==1);
 for(auto scope:{pn_shop::CharacterPoint,pn_shop::AccountPoint,pn_shop::SessionPoint,pn_shop::GlobalPoint}) {
  for(bool committed:{false,true}) {
   reset();point_value=100;auto r=request(pn_shop::Asset);r->stock_count=0;r->wallet_after=r->wallet_before;
   r->point.scope=scope;std::strcpy(r->point.key,scope==pn_shop::CharacterPoint?"Points":scope==pn_shop::AccountPoint?"#Points":scope==pn_shop::GlobalPoint?"##Points":"@Points");r->point.before=100;r->point.after=70;
   assert(pn_shop_submit(player,r,{},0));assert(point_value==100);auto a=ack(committed?pn_shop::Committed:pn_shop::Rejected);receive(a);
   assert(point_value==(committed?70:100));point_value=91;receive(a);assert(point_value==91);
  }
 }
 reset();save_result=1;assert(!pn_shop_submit(player,request(),{},0));assert(frames.empty() && !pn_shop_inflight);unchanged();
 for(bool committed:{false,true}){
  reset();item_use_settlements=0;auto r=request(pn_shop::ItemUse);r->stock_count=0;
  assert(pn_shop_submit(player,r,{},0));assert(item_use_settlements==0);
  auto a=ack(committed?pn_shop::Committed:pn_shop::Rejected);auto stale=a;stale.sequence++;
  receive(stale);assert(item_use_settlements==0);receive(a);
  assert(item_use_settlements==1 && item_use_committed==committed);
  assert(std::find(order.begin(),order.end(),"item-use-settled")!=order.end());
  receive(a);assert(item_use_settlements==1);
 }
 reset();auto invalid=request();invalid->stock_count=0;assert(!pn_shop_submit(player,invalid,{},0));assert(saves==0 && frames.empty());
 reset();connected=false;assert(!pn_shop_submit(player,request(),{},0));assert(saves==0);
 reset();assert(pn_shop_submit(player,request(),{},0));auto second=request();assert(!pn_shop_submit(player,second,{},0));assert(frames.size()==1);

 // A changed achievement snapshot is a companion frame bound to the immutable
 // commit identity. Both frames must be retried byte-for-byte, applied once,
 // and followed by every objective that arrived while the transaction waited.
 reset();capture_before={achievement_row(220023,90)};
 capture_after={achievement_row(220023,100,1700000000),achievement_row(220024,1,1700000001)};
 player.shop_commit.planned_achievements={{AG_TAMING,{1002}},{AG_GET_ZENY,{777}}};
 auto progressed=request();assert(pn_shop_submit(player,progressed,{item_event(100)},10));
 assert((progression_prepares==1 && player.shop_commit.progression_prepared &&
        prepared_achievements==std::vector<std::pair<int32,std::vector<int32>>>({
         {AG_TAMING,{1002}},{AG_GET_ZENY,{777}},{AG_GET_ITEM,{100}}})));
 assert(frames.size()==2 && order==std::vector<std::string>({"save","send","send"}));
 const auto frozen_progression=frames[0],frozen_commit=frames[1];
 pn_shop_progression::Bundle bundle;assert(pn_shop_progression::decode(frozen_progression.data(),frozen_progression.size(),bundle));
 assert(bundle.before.size()==1 && bundle.after.size()==2 && same_achievement(bundle.before[0],capture_before[0]) &&
        same_achievement(bundle.after[0],capture_after[0]) && same_achievement(bundle.after[1],capture_after[1]));
 pn_shop::Commit sent_commit{};assert(frozen_commit.size()==sizeof(sent_commit));std::memcpy(&sent_commit,frozen_commit.data(),sizeof(sent_commit));
 assert(sent_commit.progression_size==frozen_progression.size() && sent_commit.sequence==bundle.header.sequence &&
        sent_commit.account_id==bundle.header.account_id && sent_commit.char_id==bundle.header.char_id &&
        sent_commit.nonce_hi==bundle.header.nonce_hi && sent_commit.nonce_lo==bundle.header.nonce_lo);
 player.shop_commit.deferred_achievements.push_back({AG_SPEND_ZENY,{20}});
 player.shop_commit.deferred_achievements.push_back({AG_BATTLE,{1002,7}});
 progressed->progression_size=0;progressed->items[0].amount=99;
 pn_shop_retry(0,1000,11,sent_commit.sequence);
 assert(frames.size()==4 && frames[2]==frozen_progression && frames[3]==frozen_commit);
 auto progressed_ack=ack(pn_shop::Committed);receive(progressed_ack);
 assert(!player.shop_commit.pending && results==1 && last_success && progression_applies==1 && progression_suppressions==1 &&
        callbacks==2 && applied_rows.size()==2 && same_achievement(applied_rows[0],capture_after[0]) && same_achievement(applied_rows[1],capture_after[1]));
 assert((replayed_achievements==std::vector<std::pair<int32,std::vector<int32>>>({{AG_SPEND_ZENY,{20}},{AG_BATTLE,{1002,7}}}) &&
        eof_calls==0 && player.achievement_data.save && saves==2));
 assert(std::find(order.begin(),order.end(),"achievement-apply")<std::find(order.begin(),order.end(),"achievement-suppress") &&
        std::find(order.begin(),order.end(),"achievement-suppress")<std::find(order.begin(),order.end(),"delivery-callbacks") &&
        std::find(order.begin(),order.end(),"delivery-callbacks")<std::find(order.begin(),order.end(),"achievement-replay"));
 receive(progressed_ack);assert(progression_applies==1 && progression_suppressions==1 && replayed_achievements.size()==2 && saves==2);

 // A successfully evaluated no-op still owns its post-ACK callback. ItemUse
 // must be able to suppress that callback even though there is no companion
 // frame because the exact event changed no durable row.
 reset();capture_before=capture_after={achievement_row(220023,100,1700000000)};
 player.shop_commit.planned_achievements={{AG_GET_ZENY,{777}}};
 auto unchanged_progression=request(pn_shop::ItemUse);unchanged_progression->stock_count=0;
 assert(pn_shop_submit(player,unchanged_progression,{},10));
 assert((progression_prepares==1 && player.shop_commit.progression_prepared &&
        player.shop_commit.progression.empty() && player.shop_commit.request->progression_size==0 && frames.size()==1 &&
        prepared_achievements==std::vector<std::pair<int32,std::vector<int32>>>({{AG_GET_ZENY,{777}}})));
 auto unchanged_ack=ack(pn_shop::Committed);receive(unchanged_ack);
 assert(results==1 && last_success && progression_applies==0 && saves==2);

 // Rental eligibility is decided by the delivery callback at ACK time. A
 // delayed commit must not carry a precomputed progression image for a rental
 // that may have expired while SQL was retrying.
 reset();capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};
 auto rental=request();rental->items[0].expire_time=1700000000;
 assert(pn_shop_submit(player,rental,{item_event(100)},10));
 assert(progression_prepares==0 && player.shop_commit.progression.empty() && frames.size()==1 &&
        player.shop_commit.request->progression_size==0);
 auto rental_ack=ack(pn_shop::Committed);receive(rental_ack);
 assert(results==1 && last_success && progression_applies==0 && progression_suppressions==0 && callbacks==3 && saves==2);

 // If the durable snapshot cannot be installed, do not replay later objectives
 // or save the stale live log over SQL; force a reconnect to reload SQL instead.
 reset();capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};
 assert(pn_shop_submit(player,request(),{item_event(100)},10));
 player.shop_commit.deferred_achievements.push_back({AG_BATTLE,{1002,7}});progression_apply_ok=false;
 auto failed_apply_ack=ack(pn_shop::Committed);receive(failed_apply_ack);
 assert(progression_applies==1 && progression_suppressions==0 && callbacks==0 && replayed_achievements.empty() && eof_calls==1 &&
        !player.achievement_data.save && saves==1 && results==1 && last_success);
 receive(failed_apply_ack);assert(progression_applies==1 && eof_calls==1 && saves==1);

 // A malformed in-memory companion takes the same stale-state fence without
 // handing corrupt rows to the achievement layer.
 reset();capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};
 assert(pn_shop_submit(player,request(),{item_event(100)},10));assert(!player.shop_commit.progression.empty());
 player.shop_commit.deferred_achievements.push_back({AG_SPEND_ZENY,{20}});player.shop_commit.progression[0]^=0xff;
 auto malformed_ack=ack(pn_shop::Committed);receive(malformed_ack);
 assert(progression_applies==0 && progression_suppressions==0 && callbacks==0 && replayed_achievements.empty() &&
        eof_calls==1 && !player.achievement_data.save && saves==1);

 // The character server returns this terminal outcome when the submitted
 // baseline no longer matches SQL. Clear the purchase fence and force a reload
 // without exposing assets or replaying the queued live progression.
 reset();capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};
 assert(pn_shop_submit(player,request(),{item_event(100)},10));
 player.shop_commit.deferred_achievements.push_back({AG_BATTLE,{1002,7}});
 auto stale_ack=ack(pn_shop::ProgressionStale);receive(stale_ack);
 assert(!player.shop_commit.pending && !pn_shop_inflight && results==1 && !last_success &&
        player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && player.weight==0);
 assert(progression_applies==0 && progression_suppressions==0 && callbacks==0 && replayed_achievements.empty() &&
        eof_calls==1 && !player.achievement_data.save && !player.achievement_data.loaded && saves==1 &&
        item_use_settlements==1 && !item_use_committed);
 receive(stale_ack);assert(results==1 && progression_applies==0 && eof_calls==1 && saves==1);

 reset();capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};progression_prepare_ok=false;
 assert(!pn_shop_submit(player,request(),{item_event(100)},10));assert(progression_prepares==1 && saves==0 && frames.empty() && !pn_shop_inflight);
 reset();player.achievement_data.loaded=false;
 assert(!pn_shop_submit(player,request(),{item_event(100)},10));assert(progression_prepares==1 && saves==0 && frames.empty() && !pn_shop_inflight);
 reset();player.achievement_data.reward_pending_id=220023;
 assert(!pn_shop_submit(player,request(),{},10));assert(progression_prepares==0 && saves==0 && frames.empty() && !pn_shop_inflight);
 reset();player.shop_commit.planned_achievements={{AG_MAX,{1}}};
 assert(!pn_shop_submit(player,request(),{},10));assert(progression_prepares==1 && saves==0 && frames.empty() &&
        player.shop_commit.planned_achievements.empty() && !player.shop_commit.progression_prepared && !pn_shop_inflight);
 // Servers with the achievement feature disabled must keep the ordinary shop
 // path: no capture or companion frame, and the commit still settles normally.
 reset();battle_config.feature_achievement=0;capture_before={achievement_row(220023,90)};capture_after={achievement_row(220023,100,1700000000)};
 auto disabled_achievement=request();assert(pn_shop_submit(player,disabled_achievement,{item_event(100)},10));
 assert(progression_prepares==0 && player.shop_commit.progression.empty() && frames.size()==1 && frames[0].size()==sizeof(pn_shop::Commit));
 pn_shop::Commit disabled_wire{};std::memcpy(&disabled_wire,frames[0].data(),sizeof(disabled_wire));
 assert(disabled_wire.progression_size==0 && pn_shop::valid(disabled_wire));
 auto disabled_ack=ack(pn_shop::Committed);receive(disabled_ack);
 assert(results==1 && last_success && progression_applies==0 && progression_suppressions==0 && callbacks==3 && saves==2 && eof_calls==0);
 for(bool committed:{false,true}){
  reset();auto r=request(pn_shop::Asset);r->stock_count=0;assert(pn_shop_submit(player,r,{},0));
  floor_request=player.shop_commit.request;auto a=ack(committed?pn_shop::Committed:pn_shop::Rejected);auto initial=frames.back();
  present=false;player.shop_commit={};pn_shop_retry(0,1000,11,a.sequence);assert(frames.size()==2&&frames.back()==initial&&pn_shop_inflight);
  auto bad=a;bad.char_id++;receive(bad);assert(floor_request&&pn_shop_inflight);
  receive(a);assert(!floor_request&&!pn_shop_inflight);auto sent=frames.size();pn_shop_retry(0,2000,11,a.sequence);assert(frames.size()==sent);receive(a);assert(!pn_shop_inflight);
 }
 std::cout<<"PASS exact shop inter: all 3 kinds, immutable retries, ACK fences, progression apply/replay/stale/failure, refresh failure, commit/reject, duplicate ACK, submit guards\n";
}
#else
int main() {
 reset(); std::vector<map_session_data> buyers(pn_shop_queue_limit+1);
 auto make_ack=[](map_session_data& buyer) {
  const auto& r=*buyer.shop_commit.request;pn_shop::Ack a;a.account_id=r.account_id;a.char_id=r.char_id;
  a.nonce_hi=r.nonce_hi;a.nonce_lo=r.nonce_lo;a.sequence=r.sequence;a.outcome=pn_shop::Committed;return a;
 };
 for(size_t i=0;i<buyers.size();++i) {
  auto& buyer=buyers[i];buyer.status.account_id=100+i;buyer.status.char_id=200+i;
  auto purchase=request();purchase->account_id=buyer.status.account_id;purchase->char_id=buyer.status.char_id;
  other_players.push_back(&buyer);
  const bool accepted=pn_shop_submit(buyer,purchase,{},10);
  if(i<pn_shop_queue_limit && !accepted) {
   std::cerr<<"FAIL synchronized buyer "<<i+1<<" refused while another buyer waits for SQL\n";return 1;
  }
  if(i==pn_shop_queue_limit)assert(!accepted && !buyer.shop_commit.pending);
 }
 assert(pn_shop_stock_busy() && pn_shop_queue_full() && pn_shop_queue.size()==pn_shop_queue_limit && frames.size()==1);
 // An unsolicited ACK for a waiting player cannot deliver or skip the queue.
 receive(make_ack(buyers[1]));assert(frames.size()==1 && buyers[1].shop_commit.pending);
 for(size_t i=0;i<pn_shop_queue_limit;++i) {
  auto& buyer=buyers[i];assert(buyer.shop_commit.pending && frames.size()==i+1);
  pn_shop::Commit sent{};std::memcpy(&sent,frames.back().data(),sizeof(sent));
  assert(sent.account_id==buyer.status.account_id);
  const auto frozen=frames.back();current_stock=100+i;
  pn_shop_retry(0,1000,buyer.status.account_id,buyer.shop_commit.request->sequence);
  assert(frames.back()==frozen);frames.pop_back(); // Count one logical send per buyer.
  receive(make_ack(buyer));assert(!buyer.shop_commit.pending && buyer.status.zeny==90);
 }
 assert(!pn_shop_inflight && pn_shop_queue.empty() && !pn_shop_stock_busy() && !pn_shop_queue_full());
 assert(pn_metrics::runtime.shop_committed==pn_shop_queue_limit && !pn_metrics::runtime.pending);
 // Capacity can shrink while queued; an unsent rejection charges nothing and
 // immediately lets the next eligible buyer advance.
 reset();buyers=std::vector<map_session_data>(3);
 for(size_t i=0;i<3;++i){auto& buyer=buyers[i];buyer.status.account_id=100+i;buyer.status.char_id=200+i;
  other_players.push_back(&buyer);auto r=request();r->account_id=buyer.status.account_id;r->char_id=buyer.status.char_id;
  assert(pn_shop_submit(buyer,r,{},10));}
 buyers[1].max_weight=0;receive(make_ack(buyers[0]));
 assert(!buyers[1].shop_commit.pending && buyers[1].status.zeny==100 && frames.size()==2);
 receive(make_ack(buyers[2]));assert(pn_shop_queue.empty());
 std::cout<<"PASS bounded FIFO: 32 synchronized buyers, queue cap, ordered ACKs, immutable retries, stock rebase and capacity rejection\n";
}
#endif
