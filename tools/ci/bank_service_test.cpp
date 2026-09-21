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
    struct {uint32 account_id=2000001,char_id=150001; int32 zeny=1000000000;uint16 inventory_slots=MAX_INVENTORY;} status;
    struct {bool active=true,autotrade=false,warping=false,changemap=false,mail_writing=false;} state;
    pn_bank_state bank_ui;
    int64 bank_vault=1000000000;
    int32 weight=0,max_weight=1000000,fd=2,m=0;
    uint32 login_id1=11,login_id2=22;
    void* prev=reinterpret_cast<void*>(1);
    s_storage inventory{};
    ItemData* inventory_data[MAX_INVENTORY]{};
} player;
struct Session {struct {bool eof=false;}flag;uint32 client_addr=123;int32(*func_parse)(int32)=nullptr;void* session_data=nullptr;};
Session objects[4];Session* session[4]={&objects[0],&objects[1],&objects[2],&objects[3]};
alignas(8) unsigned char incoming[4][65536]{},outgoing[4][65536]{};
size_t in_size[4]{},in_pos[4]{},out_size[4]{};
#define RFIFOREST(fd) (in_size[fd]-in_pos[fd])
#define RFIFOP(fd,off) (incoming[fd]+in_pos[fd]+off)
#define RFIFOW(fd,off) (*reinterpret_cast<uint16*>(RFIFOP(fd,off)))
#define RFIFOSKIP(fd,n) (in_pos[fd]+=(n))
#define WFIFOHEAD(fd,n) ((void)0)
#define WFIFOP(fd,off) (outgoing[fd]+off)
#define WFIFOSET(fd,n) (out_size[fd]=(n))
int inter_fd=1; int64 clock_tick=10000;
bool connected=true,disabled=false,world_busy=false,present=true,add_fails=false;
int delete_failure=-1,save_count=0,timers=0,refreshes=0;
int native_replies=0, native_closes=0;
struct {bool feature_banking=true;} battle_config;
enum {MF_NOBANK,SP_BANK_VAULT,SP_WEIGHT,LOG_TYPE_BANK,ADDITEM_SUCCESS,CSAVE_NORMAL=1,CSAVE_INVENTORY=2};
enum {BDA_SUCCESS=0,BWA_SUCCESS=0};
bool itemdb_isstackable2(ItemData*){return true;}
int pc_inventoryblank(const map_session_data* sd){int count=0;for(auto& i:sd->inventory.u.items_inventory)if(!i.nameid)++count;return count;}
bool pc_isdead(map_session_data*){return false;}
bool pc_cant_act(map_session_data*){return world_busy;}
int64 gettick(){return clock_tick;}
uint32 rnd(){static uint32 n=7;return ++n;}
bool map_getmapflag(int,int){return disabled;}
bool chrif_isconnected(){return connected;}
int CheckForCharServer(){return !connected;}
map_session_data* map_id2sd(int id){return present && id==int(player.status.account_id)?&player:nullptr;}
bool session_isValid(int fd){return fd>0 && fd<4;}
void set_eof(int fd){session[fd]->flag.eof=true;}
void do_close(int fd){session[fd]->flag.eof=true;free(session[fd]->session_data);session[fd]->session_data=nullptr;}
void ShowError(const char*){}
void clif_updatestatus(map_session_data&,int){}
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
int pc_payzeny(map_session_data* sd,int count,int){assert(sd->bank_ui.applying && sd->status.zeny>=count);sd->status.zeny-=count;return 0;}
int pc_getzeny(map_session_data* sd,int count,int){assert(sd->bank_ui.applying);sd->status.zeny+=count;return 0;}
bool pc_setparam(map_session_data* sd,int,int64 value){assert(sd->bank_ui.applying);sd->bank_vault=value;return true;}
int32 add_timer(t_tick,TimerFunc,int32,intptr_t){return ++timers;}
void chrif_save(map_session_data* sd,int){assert(!sd->bank_ui.pending);++save_count;}
void clif_bank_deposit(map_session_data& sd,int){assert(!sd.bank_ui.pending);++native_replies;}
void clif_bank_withdraw(map_session_data& sd,int){assert(!sd.bank_ui.pending);++native_replies;}
void clif_bank_close(map_session_data&){++native_closes;}
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
    clock_tick+=1000;request.request_id=2;request.action=BuyDiamond;request.amount=1;add_fails=true;
    assert(rpc(request).result==Capacity && !player.bank_ui.pending && player.bank_vault==1000000100);add_fails=false;
    assert(rpc(request).result==Saving && pn_bank_count(player,6024)==1 && player.bank_vault==499000100);ack(true,2);
    // Older completed actions cannot be replayed after a newer action.
    invalid=request;invalid.request_id=1;invalid.action=Deposit;invalid.amount=100;assert(rpc(invalid).result==Stale);
    auto& gem=player.inventory.u.items_inventory[0];assert(gem.nameid==6024);
    gem.favorite=1;assert(pn_bank_count(player,6024)==0);gem.favorite=0;
    gem.bound=1;assert(pn_bank_count(player,6024)==0);gem.bound=0;
    gem.expire_time=99;assert(pn_bank_count(player,6024)==0);gem.expire_time=0;
    gem.card[0]=1;assert(pn_bank_count(player,6024)==0);gem.card[0]=0;
    gem.option[0].id=1;assert(pn_bank_count(player,6024)==0);gem.option[0].id=0;
    for(int bad=0;bad<9;++bad) {
        auto modified=gem;
        if(bad==0) modified.identify=0;
        if(bad==1) modified.equip=1;
        if(bad==2) modified.equipSwitch=1;
        if(bad==3) modified.refine=1;
        if(bad==4) modified.attribute=1;
        if(bad==5) modified.enchantgrade=1;
        if(bad==6) modified.option[4].value=1;
        if(bad==7) modified.option[4].param=1;
        if(bad==8) modified.amount=0;
        assert(!pn_bank_plain_item(modified,6024));
    }
    // Multi-stack deletion failure restores the entire batch and local locks.
    player.inventory.u.items_inventory[1]=gem;player.inventory_data[1]=item_db.data.get();player.weight+=10;
    auto before=player.inventory;auto weight=player.weight;delete_failure=1;
    clock_tick+=1000;request.request_id=3;request.action=SellDiamond;request.amount=2;
    assert(rpc(request).result==SaveFailed && !player.bank_ui.pending && !player.bank_ui.applying);
    assert(memcmp(&before,&player.inventory,sizeof(before))==0 && player.weight==weight && refreshes==1);
    delete_failure=-1;assert(rpc(request).result==Saving && pn_bank_count(player,6024)==0);ack(true,3);
    assert(player.bank_vault==1497000100);
    clock_tick+=1000;assert(clif_bank_native_transfer(player,100,true)==Saving && native_replies==0);
    ack(false,4);assert(native_replies==0);ack(true,4);assert(native_replies==1 && player.bank_vault==1497000200);
    clock_tick+=1000;assert(clif_bank_native_transfer(player,100,false)==Saving && native_replies==1);
    ack(true,5);assert(native_replies==2 && player.bank_vault==1497000100);
    player.bank_vault=INT64_MAX-1;player.status.zeny=1;
    request.request_id=6;request.action=Deposit;request.amount=1;clock_tick+=1000;
    assert(rpc(request).result==Saving && player.bank_vault==INT64_MAX && player.status.zeny==0);
    memcpy(&sent,outgoing[1],sizeof(sent));assert(sent.bank_after==INT64_MAX);ack(true,6);
    request.request_id=7;request.action=Withdraw;request.amount=MAX_ZENY;clock_tick+=1000;
    assert(rpc(request).result==Saving && player.status.zeny==MAX_ZENY && player.bank_vault==INT64_MAX-MAX_ZENY);ack(true,7);
    request.request_id=8;request.amount=1;clock_tick+=1000;
    assert(rpc(request).result==Limit && player.status.zeny==MAX_ZENY);
    session[2]->flag.eof=true;assert(rpc(Request{}).result==Unauthorized);
    assert(!clif_bank_open_custom(player));
    session[3]->flag.eof=true;clif_parse_bank_companion_session(3);
    assert(player.bank_ui.companion_fd==0 && !session[3]->session_data);
    std::cout<<"PASS: production bank service authentication, packet fragmentation, funds/capacity, locked saves, duplicate and stale requests/replies, current-state retries, item eligibility, rollback and cache release\n";
}
