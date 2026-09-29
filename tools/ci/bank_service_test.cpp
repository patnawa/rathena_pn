// Production bank RPC/action/commit handlers; explicit in-memory world and SQL boundaries.
#include <common/mmo.hpp>
#include <custom/bank_state.hpp>
#include <custom/bank_commit.hpp>
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iostream>
#include <memory>
#include <vector>
#include <cstdlib>
#define aCalloc(n,s) calloc(n,s)
struct ItemData { struct {bool inventory=false;int amount=30000;} stack; struct {bool guid=false,autoequip=false;} flag; int weight=10; };
struct ItemDb { std::shared_ptr<ItemData> data=std::make_shared<ItemData>(); std::shared_ptr<ItemData> find(uint32){return data;} } item_db;
struct map_session_data {
    struct {uint32 account_id=2000001,char_id=150001; int64 zeny=1000000000;uint16 inventory_slots=MAX_INVENTORY;char name[24]="Bank fixture";} status;
    struct {bool active=true,autotrade=false,warping=false,changemap=false,mail_writing=false,trading=false;uint8 deal_locked=0;} state;
    struct {int64 pending_zeny=0;} mail;
    struct {bool pending=false;} multi_storage,mail_companion,pair_commit;
    struct {uint32 id=0;} trade_partner;
    struct {int64 zeny=0;} deal;
    pn_bank_state bank_ui;
    int64 bank_vault=1000000000;
    int32 weight=0,max_weight=1000000,fd=2,m=0;
    uint32 login_id1=11,login_id2=22;
    void* prev=reinterpret_cast<void*>(1);
    s_storage inventory{};
    ItemData* inventory_data[MAX_INVENTORY]{};
} player,partner;
// Explicit player-state double of pc.hpp's pending predicate, not a lock or
// persistence implementation. The included production UI owns its consumers.
bool pc_transaction_pending(const map_session_data* sd){return sd->bank_ui.pending || sd->multi_storage.pending || sd->mail_companion.pending || sd->pair_commit.pending;}
struct Session {struct {bool eof=false;}flag;uint32 client_addr=123;int32(*func_parse)(int32)=nullptr;void* session_data=nullptr;};
Session objects[4];Session* session[4]={&objects[0],&objects[1],&objects[2],&objects[3]};
alignas(8) unsigned char incoming[4][65536]{},outgoing[4][65536]{};
size_t in_size[4]{},in_pos[4]{},out_size[4]{};
#define RFIFOREST(fd) (in_size[fd]-in_pos[fd])
#define RFIFOP(fd,off) (incoming[fd]+in_pos[fd]+off)
#define RFIFOW(fd,off) (*reinterpret_cast<uint16*>(RFIFOP(fd,off)))
#define RFIFOL(fd,off) (*reinterpret_cast<uint32*>(RFIFOP(fd,off)))
#define RFIFOSKIP(fd,n) (in_pos[fd]+=(n))
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,off) (outgoing[fd]+off)
#define WFIFOSET(fd,n) (out_size[fd]=(n))
int inter_fd=1; int64 clock_tick=10000;
bool connected=true,disabled=false,world_busy=false,present=true,add_fails=false,partner_present=false;
int delete_failure=-1,save_count=0,timers=0,refreshes=0;
int native_replies=0, native_closes=0, wallet_refreshes=0,trade_calls=0,sweep_calls=0;
pn_bank::Result trade_result=pn_bank::Ok;
pn_bank::Request last_trade;
struct {bool feature_banking=true;} battle_config;
enum {MF_NOBANK,SP_BANK_VAULT,SP_WEIGHT,SP_ZENY,LOG_TYPE_BANK,ADDITEM_SUCCESS,CSAVE_NORMAL=1,CSAVE_INVENTORY=2};
enum {BDA_SUCCESS=0,BWA_SUCCESS=0};
bool itemdb_isstackable2(ItemData*){return true;}
int pc_inventoryblank(const map_session_data* sd){int count=0;for(auto& i:sd->inventory.u.items_inventory)if(!i.nameid)++count;return count;}
bool pc_isdead(map_session_data*){return false;}
bool pc_cant_act(map_session_data* sd){return world_busy || pc_transaction_pending(sd);}
int64 gettick(){return clock_tick;}
uint32 rnd(){static uint32 n=7;return ++n;}
bool map_getmapflag(int,int){return disabled;}
bool chrif_isconnected(){return connected;}
int CheckForCharServer(){return !connected;}
map_session_data* map_id2sd(int id){if(present && id==int(player.status.account_id))return &player;return partner_present && id==int(partner.status.account_id)?&partner:nullptr;}
bool session_isValid(int fd){return fd>0 && fd<4;}
void set_eof(int fd){session[fd]->flag.eof=true;}
void do_close(int fd){session[fd]->flag.eof=true;free(session[fd]->session_data);session[fd]->session_data=nullptr;}
void ShowError(const char*){}
void clif_updatestatus(map_session_data&,int type){if(type==SP_ZENY)++wallet_refreshes;}
void safestrncpy(char* dest,const char* src,size_t size){if(size){std::strncpy(dest,src,size-1);dest[size-1]=0;}}
void clif_inventorylist(map_session_data*){++refreshes;}
void pc_setinventorydata(map_session_data& sd){for(int i=0;i<MAX_INVENTORY;++i)sd.inventory_data[i]=sd.inventory.u.items_inventory[i].nameid?item_db.data.get():nullptr;}
int pc_additem(map_session_data* sd,item* value,int count,int){
    if(add_fails)return 99;
    for(int i=0;i<MAX_INVENTORY;++i)if(!sd->inventory.u.items_inventory[i].nameid){sd->inventory.u.items_inventory[i]=*value;sd->inventory.u.items_inventory[i].amount=count;sd->inventory_data[i]=item_db.data.get();sd->weight+=count*10;return ADDITEM_SUCCESS;}
    return 99;
}
int pc_delitem(map_session_data* sd,int i,int count,int,int,int){
    if(i==delete_failure)return 1;
    assert(sd->bank_ui.applying && sd->inventory.u.items_inventory[i].amount>=count);
    sd->inventory.u.items_inventory[i].amount-=count;sd->weight-=count*10;
    if(!sd->inventory.u.items_inventory[i].amount){sd->inventory.u.items_inventory[i]={};sd->inventory_data[i]=nullptr;}
    return 0;
}
int pc_payzeny(map_session_data* sd,int64 count,int){assert(sd->bank_ui.applying && sd->status.zeny>=count);sd->status.zeny-=count;return 0;}
int pc_getzeny(map_session_data* sd,int64 count,int){assert(sd->bank_ui.applying);sd->status.zeny+=count;return 0;}
bool pc_setparam(map_session_data* sd,int,int64 value){assert(sd->bank_ui.applying);sd->bank_vault=value;return true;}
int32 add_timer(t_tick,TimerFunc,int32,intptr_t){return ++timers;}
void chrif_save(map_session_data* sd,int){assert(!sd->bank_ui.pending);++save_count;}
void clif_bank_deposit(map_session_data& sd,int){assert(!sd.bank_ui.pending);++native_replies;}
void clif_bank_withdraw(map_session_data& sd,int){assert(!sd.bank_ui.pending);++native_replies;}
void clif_bank_close(map_session_data&){++native_closes;}
// Dispatch doubles: these record the UI boundary, without simulating actual
// paired trade mutation, offline character collection, SQL, or acknowledgments.
pn_bank::Result trade_wide_action(map_session_data&,const pn_bank::Request& request){++trade_calls;last_trade=request;return trade_result;}
void intif_bank_sweep_save(map_session_data& sd){assert(sd.bank_ui.pending && sd.bank_ui.action==pn_bank::CollectOffline);++sweep_calls;}
#include <custom/bank_inter.inc>
#include <custom/bank_ui.inc>
static pn_bank::Reply rpc(pn_bank::Request request,int length=sizeof(pn_bank::Request)) {
    memcpy(incoming[3],&request,sizeof(request));in_pos[3]=0;in_size[3]=length;out_size[3]=0;
    assert(clif_parse_bank_companion(3));pn_bank::Reply reply;
    if(out_size[3])memcpy(&reply,outgoing[3],sizeof(reply));return reply;
}
static void ack(bool success,uint64 sequence,uint64 nonce_delta=0) {
    pn_bank_ack reply; reply.account_id=player.status.account_id;reply.char_id=player.status.char_id;
    reply.nonce_hi=player.bank_ui.nonce_hi+nonce_delta;reply.nonce_lo=player.bank_ui.nonce_lo;reply.request_id=sequence;reply.committed=success;
    memcpy(incoming[1],&reply,sizeof(reply));in_pos[1]=0;in_size[1]=sizeof(reply);intif_parse_BankCommitted(1);
}
static void current_ui_boundaries(){
    using namespace pn_bank;
    player=map_session_data{};partner=map_session_data{};partner_present=false;
    connected=true;disabled=false;battle_config.feature_banking=true;
    auto snapshot=pn_bank_snapshot(player);
    Request refresh;refresh.action=Refresh;
    int prior=wallet_refreshes;
    pn_bank_refresh_native_wallet(&player,refresh,snapshot,true);
    assert(wallet_refreshes==prior+1);
    prior=wallet_refreshes;
    pn_bank_refresh_native_wallet(&player,refresh,snapshot,false);
    pn_bank_refresh_native_wallet(nullptr,refresh,snapshot,true);
    auto invalid=snapshot;invalid.nonce_hi++;
    pn_bank_refresh_native_wallet(&player,refresh,invalid,true);
    invalid=snapshot;invalid.char_id++;
    pn_bank_refresh_native_wallet(&player,refresh,invalid,true);
    invalid=snapshot;invalid.wallet--;
    pn_bank_refresh_native_wallet(&player,refresh,invalid,true);
    invalid=snapshot;invalid.result=Unauthorized;
    pn_bank_refresh_native_wallet(&player,refresh,invalid,true);
    auto action=refresh;action.action=Deposit;
    pn_bank_refresh_native_wallet(&player,action,snapshot,true);
    assert(wallet_refreshes==prior);
    // Every pending subsystem wins over map restrictions and disconnection,
    // and also prevents a HUD redraw using an otherwise valid old snapshot.
    for(bool* pending:{&player.bank_ui.pending,&player.multi_storage.pending,&player.mail_companion.pending,&player.pair_commit.pending}){
        *pending=true;disabled=true;connected=false;battle_config.feature_banking=false;
        auto saving=pn_bank_snapshot(player);assert(saving.result==Saving);
        pn_bank_refresh_native_wallet(&player,refresh,saving,true);
        pn_bank_refresh_native_wallet(&player,refresh,snapshot,true);
        assert(wallet_refreshes==prior);*pending=false;
    }
    assert(pn_bank_snapshot(player).result==Unavailable);
    connected=true;disabled=false;battle_config.feature_banking=true;

    partner.status.account_id=2000002;partner.status.char_id=150002;
    safestrncpy(partner.status.name,"Trade partner",sizeof(partner.status.name));
    partner_present=true;player.state.trading=partner.state.trading=true;
    player.trade_partner.id=partner.status.account_id;partner.trade_partner.id=player.status.account_id;
    player.bank_ui.trade_id=partner.bank_ui.trade_id=91;player.bank_ui.trade_revision=4;
    player.deal.zeny=123;partner.deal.zeny=456;player.state.deal_locked=1;partner.state.deal_locked=2;
    auto trading=pn_bank_snapshot(player);
    assert(valid_reply(trading) && trading.trade_id==91 && trading.trade_revision==4);
    assert(trading.partner_account_id==partner.status.account_id && trading.partner_char_id==partner.status.char_id);
    assert(std::strcmp(trading.partner_name,"Trade partner")==0 && trading.own_offer==123 && trading.partner_offer==456);
    assert(trading.own_trade_state==1 && trading.partner_trade_state==2 && !trading.max_deposit && !trading.max_withdraw);
    partner.bank_ui.trade_id++;assert(!pn_bank_snapshot(player).trade_id);partner.bank_ui.trade_id--;
    partner.trade_partner.id=0;assert(!pn_bank_snapshot(player).trade_id);partner.trade_partner.id=player.status.account_id;
    player.bank_ui.collected_characters=3;player.bank_ui.skipped_characters=2;
    auto receipt=pn_bank_snapshot(player);assert(receipt.counts[0]==3 && receipt.counts[1]==2);

    Request request;request.nonce_hi=snapshot.nonce_hi;request.nonce_lo=snapshot.nonce_lo;
    request.trade_id=91;request.trade_revision=4;request.amount=123;
    for(uint32 kind=TradeSetOffer;kind<=TradeCancel;++kind){
        request.action=kind;request.request_id=100+kind;trade_result=Ok;
        auto calls=trade_calls;auto bank=player.bank_vault;auto wallet=player.status.zeny;
        assert(pn_bank_action(player,request)==Ok && trade_calls==calls+1);
        assert(last_trade.action==kind && last_trade.trade_id==91 && last_trade.trade_revision==4 && last_trade.amount==123);
        assert(player.bank_vault==bank && player.status.zeny==wallet);
        assert(pn_bank_action(player,request)==Ok && trade_calls==calls+1);
        auto changed=request;changed.trade_revision++;
        assert(pn_bank_action(player,changed)==Stale && trade_calls==calls+1);
    }
    request.request_id=200;trade_result=Invalid;auto calls=trade_calls;
    auto recorded=player.bank_ui.request_id;
    assert(pn_bank_action(player,request)==Invalid && trade_calls==calls+1 && player.bank_ui.request_id==recorded);
    trade_result=Saving;assert(pn_bank_action(player,request)==Saving);
    player.pair_commit.pending=true;calls=trade_calls;
    assert(pn_bank_action(player,request)==Saving && trade_calls==calls);
    request.request_id++;assert(pn_bank_action(player,request)==Saving && trade_calls==calls);
    player.pair_commit.pending=false;

    // Offline sweep tests stop at dispatch. No fake SQL acknowledgment is
    // used to claim collection or persistence has succeeded.
    player.state.trading=partner.state.trading=false;partner_present=false;
    request.action=CollectOffline;request.request_id=300;request.trade_id=request.trade_revision=0;
    request.amount=1;clock_tick+=1000;auto sweeps=sweep_calls;
    assert(pn_bank_action(player,request)==Invalid && sweep_calls==sweeps);
    request.amount=0;request.trade_id=91;
    assert(pn_bank_action(player,request)==Invalid && sweep_calls==sweeps);
    request.trade_id=0;auto bank=player.bank_vault;auto wallet=player.status.zeny;
    assert(pn_bank_action(player,request)==Saving && sweep_calls==sweeps+1);
    assert(player.bank_ui.pending && player.bank_ui.bank_before==bank && player.bank_ui.wallet_before==wallet);
    assert(player.bank_vault==bank && player.status.zeny==wallet);
    assert(pn_bank_action(player,request)==Saving && sweep_calls==sweeps+1);
    request.request_id++;assert(pn_bank_action(player,request)==Saving && sweep_calls==sweeps+1);
    player.bank_ui.pending=false;
}
int main(){
    using namespace pn_bank;
    player.inventory.type=TABLE_INVENTORY;
    Request request; request.account_id=player.status.account_id;request.char_id=player.status.char_id;request.login_id1=11;request.login_id2=22;
    auto snapshot=rpc(request);assert(snapshot.result==Ok && snapshot.bank==1000000000 && snapshot.nonce_hi);
    assert(snapshot.char_id==player.status.char_id && valid_reply(snapshot));
    assert(player.bank_ui.companion_fd==3 && clif_bank_open_custom(player) && native_closes==1);
    Reply notification;memcpy(&notification,outgoing[3],sizeof(notification));
    assert(notification.flags==open_panel && notification.char_id==request.char_id && valid_reply(notification));
    auto* peer=static_cast<pn_bank_companion*>(session[3]->session_data);
    peer->nonce_hi++;assert(!clif_bank_open_custom(player));peer->nonce_hi--;
    session[3]->client_addr++;assert(!clif_bank_open_custom(player));session[3]->client_addr--;
    assert(clif_bank_open_custom(player));
    rpc(request,9);assert(out_size[3]==0 && !session[3]->flag.eof);
    auto invalid=request;invalid.login_id1++;assert(rpc(invalid).result==Unauthorized);
    invalid=request;invalid.login_id2++;assert(rpc(invalid).result==Unauthorized);
    invalid=request;invalid.char_id++;assert(rpc(invalid).result==Unauthorized);
    session[3]->client_addr++;assert(rpc(request).result==Unauthorized);session[3]->client_addr--;
    // Malformed companion frames are closed before mutation or SQL dispatch.
    for(int bad=0;bad<8;++bad) {
        invalid=request;
        if(bad==0) invalid.magic_value^=0x10000;
        if(bad==1) invalid.protocol=1;
        if(bad==2) invalid.length=0;
        if(bad==3) invalid.length=63;
        if(bad==4) invalid.length=65;
        if(bad==5) invalid.length=UINT16_MAX;
        if(bad==6) invalid.reserved=1;
        if(bad==7) invalid.action=UINT32_MAX;
        rpc(invalid);assert(session[3]->flag.eof && !out_size[3] && !player.bank_ui.pending && !timers);
        session[3]->flag.eof=false;
    }
    request.nonce_hi=snapshot.nonce_hi;request.nonce_lo=snapshot.nonce_lo;request.request_id=1;request.action=Deposit;request.amount=100;
    invalid=request;invalid.nonce_hi++;assert(rpc(invalid).result==Stale && !player.bank_ui.pending);
    world_busy=true;assert(rpc(request).result==Busy);world_busy=false;
    player.state.mail_writing=true;
    assert(rpc(request).result==Busy && !player.bank_ui.pending && player.bank_vault==1000000000 && player.status.zeny==1000000000);
    player.state.mail_writing=false;
    disabled=true;assert(rpc(request).result==Unavailable);disabled=false;
    connected=false;assert(rpc(request).result==Unavailable);connected=true;
    assert(rpc(request).result==Saving && player.bank_ui.pending && player.bank_vault==1000000100 && player.status.zeny==999999900);
    assert(out_size[1]==sizeof(pn_bank_commit)+sizeof(s_storage));
    pn_bank_commit sent;memcpy(&sent,outgoing[1],sizeof(sent));assert(sent.amount==100 && sent.bank_after==1000000100 && sent.wallet_after==999999900);
    int prior=timers;assert(rpc(request).result==Saving && timers==prior && player.bank_vault==1000000100);
    ack(false,1);ack(true,2);ack(true,1,1);assert(player.bank_ui.pending && !save_count);
    // Loot added while waiting is retained in current-state retries.
    player.status.zeny+=77;player.inventory.u.items_inventory[4].nameid=501;player.inventory.u.items_inventory[4].amount=1;
    intif_bank_save_retry(0,11000,player.status.account_id,1);memcpy(&sent,outgoing[1],sizeof(sent));
    assert(sent.wallet_after==999999977);
    s_storage retried;memcpy(&retried,outgoing[1]+sizeof(sent),sizeof(retried));assert(retried.u.items_inventory[4].nameid==501);
    ack(true,1);assert(!player.bank_ui.pending && save_count==1);ack(true,1);assert(save_count==1);
    assert(rpc(request).result==Ok && player.bank_vault==1000000100);
    invalid=request;invalid.amount=200;assert(rpc(invalid).result==Stale);
    // An authenticated old client cannot exchange items after UI controls disappear.
    auto inventory_before=player.inventory;
    auto bank_before=player.bank_vault; auto wallet_before=player.status.zeny;
    auto timers_before=timers;
    clock_tick+=1000;request.request_id=2;request.amount=1;
    for(uint32_t action=BuyDiamond;action<=SellNote;++action) {
        request.action=action;
        assert(rpc(request).result==Invalid && !player.bank_ui.pending);
        assert(player.bank_vault==bank_before && player.status.zeny==wallet_before && timers==timers_before);
        assert(memcmp(&inventory_before,&player.inventory,sizeof(inventory_before))==0);
    }
    auto direct_snapshot=pn_bank_snapshot(player);
    for(int i=0;i<2;++i) assert(direct_snapshot.counts[i]==0 && direct_snapshot.max_buy[i]==0 && direct_snapshot.max_sell[i]==0);
    clock_tick+=1000;assert(clif_bank_native_transfer(player,100,true)==Saving && native_replies==0);
    ack(false,2);assert(native_replies==0);ack(true,2);assert(native_replies==1 && player.bank_vault==1000000200);
    clock_tick+=1000;assert(clif_bank_native_transfer(player,100,false)==Saving && native_replies==1);
    ack(true,3);assert(native_replies==2 && player.bank_vault==1000000100);
    player.bank_vault=INT64_MAX-1;player.status.zeny=1;
    request.request_id=6;request.action=Deposit;request.amount=1;clock_tick+=1000;
    assert(rpc(request).result==Saving && player.bank_vault==INT64_MAX && player.status.zeny==0);
    memcpy(&sent,outgoing[1],sizeof(sent));assert(sent.bank_after==INT64_MAX);ack(true,6);
    request.request_id=7;request.action=Withdraw;request.amount=MAX_WALLET_ZENY;clock_tick+=1000;
    assert(rpc(request).result==Saving && player.status.zeny==MAX_WALLET_ZENY && player.bank_vault==INT64_MAX-MAX_WALLET_ZENY);ack(true,7);
    player.bank_vault=1;request.request_id=8;request.amount=1;clock_tick+=1000;
    assert(rpc(request).result==Limit && player.status.zeny==MAX_WALLET_ZENY);
    session[2]->flag.eof=true;assert(rpc(Request{}).result==Unauthorized);
    assert(!clif_bank_open_custom(player));
    session[3]->flag.eof=true;clif_parse_bank_companion_session(3);
    assert(player.bank_ui.companion_fd==0 && !session[3]->session_data);
    current_ui_boundaries();
    std::cout<<"PASS: production bank service authentication, packet fragmentation, funds/capacity, locked saves, duplicate and stale requests/replies, current-state retries, legacy exchange rejection and cache release\n";
    std::cout<<"PASS: pending snapshot/HUD guards, reciprocal trade snapshot, trade routing/idempotency and offline-sweep dispatch boundaries (trade/sweep persistence explicitly stubbed)\n";
}
