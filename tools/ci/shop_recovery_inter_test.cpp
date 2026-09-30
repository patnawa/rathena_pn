// Exact production handler; doubles represent only external world/transport APIs.
#include <custom/shop_state.hpp>
#include <common/runtime_metrics.hpp>
#include <map/log.hpp>
#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
using uint64=uint64_t; using int32=int32_t;
class map_session_data {
public:
 struct {uint32_t account_id=11,char_id=22,uniqueitem_counter=0; int64_t zeny=100;} status;
 struct {uint64_t nonce_hi=1,nonce_lo=2;} bank_ui;
 struct {struct {item items_inventory[MAX_INVENTORY]{};} u;} inventory;
 pn_shop_state shop_commit;
 bool vars_ok=true;
 int fd=1;
 int32 cashPoints=50,kafraPoints=20;
 uint32_t weight=0;
};
static map_session_data player;
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
bool pn_item_use_active(const map_session_data*){return item_use_collecting;}
void pn_item_use_settled(map_session_data& sd,bool committed){assert(!sd.shop_commit.pending);++item_use_settlements;item_use_committed=committed;order.emplace_back("item-use-settled");}
static unsigned char outbound[65536],inbound[65536];
static int inter_fd=1;
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,n) (outbound+(n))
#define WFIFOSET(fd,n) (frames.emplace_back(outbound,outbound+(n)),order.emplace_back("send"))
#define RFIFOP(fd,n) (inbound+(n))

static map_session_data* map_id2sd(int id){return present && id==int(player.status.account_id)?&player:nullptr;}
static bool CheckForCharServer(){return !connected;}
static bool chrif_isconnected(){return connected;}
static bool pc_transaction_pending(map_session_data* sd){return sd->shop_commit.pending;}
static int rnd(){return 42;}
static constexpr int CSAVE_NORMAL=1,CSAVE_INVENTORY=2;
static int chrif_save(map_session_data* sd,int){assert(!sd->shop_commit.pending);++saves;order.emplace_back("save");return save_result;}
t_tick gettick(){return 100;}
int32 add_timer(t_tick,TimerFunc,int32,intptr_t){return ++timers;}
bool pn_shop_stock_refresh(const pn_shop::Commit&){++refreshes;return refresh_ok;}
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
 explicit PcItemDeliveryScope(map_session_data& p):sd(p){assert(!sd.shop_commit.pending);}
 void added(int,const Data&){assert(sd.inventory.u.items_inventory[0].amount==2);++callbacks;}
 void refresh_questinfo(){++callbacks;}
};
void pn_shop_committed_effects(map_session_data&,const pn_shop::Commit&){++callbacks;}
#include "shop_inter_body.inc"
static void reset(){player={};present=true;floor_request.reset();pn_metrics::runtime={};connected=refresh_ok=true;saves=results=callbacks=refreshes=timers=save_result=0;frames.clear();order.clear();pn_shop_inflight=false;player.inventory.u.items_inventory[0].nameid=501;player.inventory.u.items_inventory[0].amount=1;}
static std::shared_ptr<pn_shop::Commit> request(uint32_t kind=pn_shop::Market){
 auto r=std::make_shared<pn_shop::Commit>();r->kind=kind;r->account_id=11;r->char_id=22;r->wallet_before=100;r->wallet_after=kind==pn_shop::Sale?100:90;r->cash_before=50;r->cash_after=kind==pn_shop::Sale?40:50;r->kafra_before=r->kafra_after=20;r->stock_count=1;r->stocks[0].key=501;r->stocks[0].before=5;r->stocks[0].after=4;
 if(kind==pn_shop::Sale){r->stocks[0].sale_start=1;r->stocks[0].sale_end=2;}else std::strcpy(r->stocks[0].name,"shop");
 r->items[0]=player.inventory.u.items_inventory[0];r->items[0].amount=2;return r;
}
static pn_shop::Ack ack(uint32_t outcome){auto&r=*player.shop_commit.request;pn_shop::Ack a;a.account_id=r.account_id;a.char_id=r.char_id;a.nonce_hi=r.nonce_hi;a.nonce_lo=r.nonce_lo;a.sequence=r.sequence;a.outcome=outcome;return a;}
static void receive(const pn_shop::Ack& a){std::memcpy(inbound,&a,sizeof(a));intif_parse_ShopCommitted(1);}
static void unchanged(){assert(player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && callbacks==0 && results==0);}
int main(){
 item_use_collecting=true;reset();assert(!pn_shop_submit(player,request(),{},0));assert(saves==0 && frames.empty());item_use_collecting=false;
 for(auto kind:{pn_shop::Market,pn_shop::Barter,pn_shop::Sale}){
  reset();auto r=request(kind);assert(pn_shop_submit(player,r,{{0,1,501}},10));unchanged();assert(player.shop_commit.pending && pn_shop_stock_busy());assert(order==std::vector<std::string>({"save","send"}));auto frozen=frames[0];r->wallet_after=0;r->items[0].amount=99;
  pn_shop_retry(0,1000,11,player.shop_commit.request->sequence);assert(frames.size()==2 && frames[1]==frozen);unchanged();
  connected=false;pn_shop_retry(0,2000,11,player.shop_commit.request->sequence);assert(frames.size()==2);connected=true;
  auto a=ack(pn_shop::Committed);auto bad=a;bad.sequence++;receive(bad);bad=a;bad.char_id++;receive(bad);bad=a;bad.nonce_lo++;receive(bad);bad=a;bad.outcome=pn_shop::Retry;receive(bad);unchanged();assert(refreshes==0);
  refresh_ok=false;receive(a);unchanged();assert(player.shop_commit.pending && pn_shop_stock_busy());refresh_ok=true;receive(a);
  assert(!player.shop_commit.pending && !pn_shop_stock_busy() && results==1 && last_success && callbacks==3 && player.weight==10 && player.inventory.u.items_inventory[0].amount==2);assert(player.status.zeny==(kind==pn_shop::Sale?100:90));assert(player.cashPoints==(kind==pn_shop::Sale?40:50));
  player.status.zeny=77;receive(a);assert(player.status.zeny==77 && results==1 && callbacks==3);assert(saves==2);
  assert(pn_metrics::runtime.shop_committed==1 && pn_metrics::runtime.shop_ack.count==1 &&
         pn_metrics::runtime.shop_refresh_failures==1 && pn_metrics::runtime.shop_retry_attempts==2 && !pn_metrics::runtime.pending);
  reset();assert(pn_shop_submit(player,request(kind),{},10));a=ack(pn_shop::Rejected);receive(a);assert(results==1 && !last_success && callbacks==0 && player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && !pn_shop_stock_busy());receive(a);assert(results==1);
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
 reset();save_result=1;assert(!pn_shop_submit(player,request(),{},0));assert(frames.empty() && !pn_shop_stock_busy());unchanged();
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
 for(bool committed:{false,true}){
  reset();auto r=request(pn_shop::Asset);r->stock_count=0;assert(pn_shop_submit(player,r,{},0));
  floor_request=player.shop_commit.request;auto a=ack(committed?pn_shop::Committed:pn_shop::Rejected);auto initial=frames.back();
  present=false;player.shop_commit={};pn_shop_retry(0,1000,11,a.sequence);assert(frames.size()==2&&frames.back()==initial&&pn_shop_stock_busy());
  auto bad=a;bad.char_id++;receive(bad);assert(floor_request&&pn_shop_stock_busy());
  receive(a);assert(!floor_request&&!pn_shop_stock_busy());auto sent=frames.size();pn_shop_retry(0,2000,11,a.sequence);assert(frames.size()==sent);receive(a);assert(!pn_shop_stock_busy());
 }
 std::cout<<"PASS exact shop inter: all 3 kinds, immutable retries, ACK fences, refresh failure, commit/reject, duplicate ACK, submit guards\n";
}
