// Exact production handler; doubles represent only external world/transport APIs.
#include <custom/shop_state.hpp>
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
 int32 cashPoints=50,kafraPoints=20;
 uint32_t weight=0;
};
static map_session_data player;
static bool connected=true,refresh_ok=true;
static int saves=0,results=0,callbacks=0,refreshes=0,timers=0,save_result=0;
static bool last_success=false;
static std::vector<std::vector<unsigned char>> frames;
static std::vector<std::string> order;
static unsigned char outbound[65536],inbound[65536];
static int inter_fd=1;
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,n) (outbound+(n))
#define WFIFOSET(fd,n) (frames.emplace_back(outbound,outbound+(n)),order.emplace_back("send"))
#define RFIFOP(fd,n) (inbound+(n))

static map_session_data* map_id2sd(int id){return id==int(player.status.account_id)?&player:nullptr;}
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
static bool pc_setaccountreg(map_session_data*,int64_t,int64_t){return true;}
void log_cash(const map_session_data*,e_log_pick_type,e_log_cash_type,int32){}
static void pc_setinventorydata(map_session_data&){}
static void pc_setequipindex(map_session_data*){}
void log_zeny(const map_session_data&,e_log_pick_type,uint32,int64){}
static void clif_inventorylist(map_session_data*){}
static void clif_updatestatus(map_session_data&,_sp){}
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
static void reset(){player={};connected=refresh_ok=true;saves=results=callbacks=refreshes=timers=save_result=0;frames.clear();order.clear();pn_shop_inflight=false;player.inventory.u.items_inventory[0].nameid=501;player.inventory.u.items_inventory[0].amount=1;}
static std::shared_ptr<pn_shop::Commit> request(uint32_t kind=pn_shop::Market){
 auto r=std::make_shared<pn_shop::Commit>();r->kind=kind;r->account_id=11;r->char_id=22;r->wallet_before=100;r->wallet_after=kind==pn_shop::Sale?100:90;r->cash_before=50;r->cash_after=kind==pn_shop::Sale?40:50;r->kafra_before=r->kafra_after=20;r->stock_count=1;r->stocks[0].key=501;r->stocks[0].before=5;r->stocks[0].after=4;
 if(kind==pn_shop::Sale){r->stocks[0].sale_start=1;r->stocks[0].sale_end=2;}else std::strcpy(r->stocks[0].name,"shop");
 r->items[0]=player.inventory.u.items_inventory[0];r->items[0].amount=2;return r;
}
static pn_shop::Ack ack(uint32_t outcome){auto&r=*player.shop_commit.request;pn_shop::Ack a;a.account_id=r.account_id;a.char_id=r.char_id;a.nonce_hi=r.nonce_hi;a.nonce_lo=r.nonce_lo;a.sequence=r.sequence;a.outcome=outcome;return a;}
static void receive(const pn_shop::Ack& a){std::memcpy(inbound,&a,sizeof(a));intif_parse_ShopCommitted(1);}
static void unchanged(){assert(player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && callbacks==0 && results==0);}
int main(){
 for(auto kind:{pn_shop::Market,pn_shop::Barter,pn_shop::Sale}){
  reset();auto r=request(kind);assert(pn_shop_submit(player,r,{{0,1,501}},10));unchanged();assert(player.shop_commit.pending && pn_shop_stock_busy());assert(order==std::vector<std::string>({"save","send"}));auto frozen=frames[0];r->wallet_after=0;r->items[0].amount=99;
  pn_shop_retry(0,1000,11,player.shop_commit.request->sequence);assert(frames.size()==2 && frames[1]==frozen);unchanged();
  connected=false;pn_shop_retry(0,2000,11,player.shop_commit.request->sequence);assert(frames.size()==2);connected=true;
  auto a=ack(pn_shop::Committed);auto bad=a;bad.sequence++;receive(bad);bad=a;bad.char_id++;receive(bad);bad=a;bad.nonce_lo++;receive(bad);bad=a;bad.outcome=pn_shop::Retry;receive(bad);unchanged();assert(refreshes==0);
  refresh_ok=false;receive(a);unchanged();assert(player.shop_commit.pending && pn_shop_stock_busy());refresh_ok=true;receive(a);
  assert(!player.shop_commit.pending && !pn_shop_stock_busy() && results==1 && last_success && callbacks==3 && player.weight==10 && player.inventory.u.items_inventory[0].amount==2);assert(player.status.zeny==(kind==pn_shop::Sale?100:90));assert(player.cashPoints==(kind==pn_shop::Sale?40:50));
  player.status.zeny=77;receive(a);assert(player.status.zeny==77 && results==1 && callbacks==3);assert(saves==2);
  reset();assert(pn_shop_submit(player,request(kind),{},10));a=ack(pn_shop::Rejected);receive(a);assert(results==1 && !last_success && callbacks==0 && player.status.zeny==100 && player.cashPoints==50 && player.inventory.u.items_inventory[0].amount==1 && !pn_shop_stock_busy());receive(a);assert(results==1);
 }
 reset();save_result=1;assert(!pn_shop_submit(player,request(),{},0));assert(frames.empty() && !pn_shop_stock_busy());unchanged();
 reset();auto invalid=request();invalid->stock_count=0;assert(!pn_shop_submit(player,invalid,{},0));assert(saves==0 && frames.empty());
 reset();connected=false;assert(!pn_shop_submit(player,request(),{},0));assert(saves==0);
 reset();assert(pn_shop_submit(player,request(),{},0));auto second=request();assert(!pn_shop_submit(player,second,{},0));assert(frames.size()==1);
 std::cout<<"PASS exact shop inter: all 3 kinds, immutable retries, ACK fences, refresh failure, commit/reject, duplicate ACK, submit guards\n";
}
