#include <algorithm>
#include <cassert>
#include <cstdarg>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <vector>
#include <limits>
#include <map/chrif_save.hpp>
#include <common/logout_save.hpp>
using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
using int32=int32_t; using t_tick=int64_t;
enum sd_state { ST_LOGIN, ST_LOGOUT, ST_MAPCHANGE };
enum { CSAVE_NORMAL=0, CSAVE_QUIT=1, CSAVE_CHANGE_MAPSERV=2, CSAVE_AUTOTRADE=4,
    CSAVE_INVENTORY=8, CSAVE_CART=16, CSAVE_QUITTING=7 };
struct achievement { int value; };
struct mmo_charstatus { uint32 account_id=7, char_id=17; int sex=0, pet_id=3, guild_id=0; };
struct s_storage { bool dirty=true; int type=0, value=0; };
struct pet_data { int pet=25; };
struct companion { int value=0; };
struct map_session_data {
    mmo_charstatus status;
    int login_id1=1, login_id2=2, fd=2, mapindex=0;
    struct { bool active=true; int storage_flag=0; } state;
    struct { bool loaded=true, save=true; uint16 count=1; achievement* achievements=nullptr; } achievement_data;
    s_storage storage{true,1,21}, inventory{true,2,22}, cart{true,3,23}, premiumStorage{true,4,24};
    bool vars_dirty=true, save_quest=true;
    int quest=29, registry=30, bonus=31, statuses=32, cooldown=33;
    pet_data* pd=nullptr; companion *hd=nullptr,*md=nullptr,*ed=nullptr;
};
using TBL_PC=map_session_data;
struct auth_node {
    uint32 account_id=0,char_id=0; int login_id1=0,login_id2=0,sex=0,fd=0;
    map_session_data* sd=nullptr; mmo_charstatus* char_dat=nullptr;
    achievement* achievement_snapshot=nullptr; mmo_charstatus* logout_status=nullptr;
    ChrifSaveBuffer* logout_saves=nullptr;
    uint16 achievement_count=0; uint64 achievement_generation=0;
    bool achievement_pending=false,final_save_pending=false;
    t_tick node_created=0; sd_state state=ST_LOGIN;
};
auth_node* retained=nullptr;
void* auth_db=nullptr; void* auth_db_ers=nullptr;
#define ers_alloc(pool,type) new type
#define CREATE(p,type,n) p=static_cast<type*>(calloc(n,sizeof(type)))
#define aFree(p) free(p)
void idb_put(void*,uint32,auth_node* p){retained=p;}
auth_node* chrif_search(uint32){return retained;}
auth_node* chrif_auth_check(uint32 a,uint32 c,sd_state s){return retained && retained->account_id==a && retained->char_id==c && retained->state==s?retained:nullptr;}
bool chrif_auth_delete(uint32,uint32,sd_state){return true;}
t_tick gettick(){return 100;}
struct socket_data { void* session_data=nullptr; } sockets[3];
socket_data* session[]={&sockets[0],&sockets[1],&sockets[2]};
bool connected=true; int char_fd=1;
int32 chrif_isconnected(){return connected;}
#define chrif_check(value) if(!chrif_isconnected()) return value
#define nullpo_retr(value,p) if(!(p)) return value
void ShowError(const char*,...){assert(false);}
bool pn_item_use_save_defer(map_session_data*,int){return false;}
bool pc_transaction_pending(const map_session_data*){return false;}
void pc_makesavestatus(map_session_data*){}
std::vector<uint8> scratch(65536);
std::vector<std::vector<uint8>> wire;
std::vector<uint8> incoming(65536);
#define RFIFOP(fd,pos) (incoming.data()+(pos))
#define RFIFOW(fd,pos) (*reinterpret_cast<uint16*>(RFIFOP(fd,pos)))
#define RFIFOL(fd,pos) (*reinterpret_cast<uint32*>(RFIFOP(fd,pos)))
#define WFIFOHEAD(fd,len) scratch.resize(std::max(scratch.size(),size_t(len)))
#define WFIFOP(fd,pos) (scratch.data()+(pos))
#define WFIFOW(fd,pos) (*reinterpret_cast<uint16*>(WFIFOP(fd,pos)))
#define WFIFOL(fd,pos) (*reinterpret_cast<uint32*>(WFIFOP(fd,pos)))
#define WFIFOB(fd,pos) (*reinterpret_cast<uint8*>(WFIFOP(fd,pos)))
void WFIFOSET(int,size_t n){wire.emplace_back(scratch.begin(),scratch.begin()+n);}
void emit(uint16 type,int value){ if(!chrif_save_available())return; std::vector<uint8> p(6); memcpy(p.data(),&type,2);memcpy(p.data()+2,&value,4);chrif_save_packet(p.data(),p.size()); }
void chrif_save_scdata(map_session_data* s){emit(10,s->statuses);}
void chrif_skillcooldown_save(const map_session_data& s){emit(11,s.cooldown);}
void chrif_bsdata_save(map_session_data* s,bool){emit(9,s->bonus);}
void intif_storage_save(map_session_data*,s_storage* s){emit(s->type,s->value);}
void storage_storagesave(map_session_data* s){intif_storage_save(s,&s->storage);}
void storage_premiumStorage_save(map_session_data* s){intif_storage_save(s,&s->premiumStorage);}
void storage_guild_storagesave(uint32,int,int){}
void intif_saveregistry(map_session_data* s){emit(8,s->registry);s->vars_dirty=false;}
void intif_save_petdata(uint32,int* p){emit(5,*p);}
bool hom_is_active(companion* p){return p;}
void hom_save(companion* p){emit(6,p->value);}
int mercenary_get_lifetime(companion*){return 1;}
void mercenary_save(companion* p){emit(12,p->value);}
int elemental_get_lifetime(companion*){return 1;}
void elemental_save(companion* p){emit(13,p->value);}
void intif_quest_save(map_session_data* s){emit(7,s->quest);}
bool intif_achievement_logout_save(uint32,uint32,uint64,const achievement*,uint16){emit(0x30a5,1);return connected;}
void intif_achievement_save(map_session_data*){}
using DBKey=int; struct DBData {void* ptr;};
void* db_data2ptr(DBData* d){return d->ptr;}
void pc_authfail(map_session_data*){} void chrif_char_offline(map_session_data*){}
int map_mapname2ipport(int,uint32*,uint16*){return 1;}
void chrif_changemapserver(map_session_data*,uint32,uint16){}
void clif_authfail_fd(int,int){}
size_t readable=0; bool rejected=false;
#define RFIFOREST(fd) readable
void RFIFOSKIP(int,size_t n){readable-=n;}
void set_eof(int){rejected=true;}
struct Owner {uint32 char_id=17; int server=0;};
std::map<uint32,std::shared_ptr<Owner>> owners;
auto& char_get_onlinedb(){return owners;}
#include "logout-production.inc"
std::map<int,int> saved;
const std::map<int,int> expected{{1,21},{2,22},{3,23},{4,24},{5,25},{6,26},{7,29},{8,30},{9,31},{10,32},{11,33},{12,27},{13,28}};
uint16 kind(const std::vector<uint8>& p){uint16 k;memcpy(&k,p.data(),2);return k;}
void deliver(size_t count){
    const auto batch=std::move(wire);wire.clear();
    for(size_t i=0;i<std::min(count,batch.size());++i){
        const auto& p=batch[i];const auto k=kind(p);
        if(k<100){int v;memcpy(&v,p.data()+2,4);saved[k]=v;}
        else if(k==logout_save::request){
            incoming=p;readable=p.size();assert(chmapif_parse_logout_barrier(1,0)==1);assert(!readable);
        }
    }
}
void retry(){DBData data{retained};va_list unused{};chrif_reconnect(0,&data,unused);}
void no_dependencies(){for(const auto& p:wire)assert(kind(p)>=100);}
int main(){
    int cases=0;
    for(bool offline:{false,true}) for(bool achievements:{false,true}) for(size_t cut=0;cut<=14;++cut) {
        map_session_data sd; achievement a{42}; pet_data pet; companion hom{26},merc{27},ele{28};
        sd.achievement_data.loaded=achievements;
        sd.achievement_data.achievements=&a;sd.pd=&pet;sd.hd=&hom;sd.md=&merc;sd.ed=&ele;
        owners[7]=std::make_shared<Owner>();
        connected=!offline;saved.clear();wire.clear();
        chrif_save(&sd,CSAVE_QUIT|CSAVE_INVENTORY|CSAVE_CART);
        // Lose the stream after any prefix, including a delivered barrier with a lost ACK.
        deliver(cut);wire.clear();connected=false;
        sd.pd=nullptr;sd.hd=sd.md=sd.ed=nullptr;
        sd.quest=sd.registry=sd.bonus=sd.statuses=sd.cooldown=-999;
        sd.inventory.value=sd.cart.value=sd.storage.value=sd.premiumStorage.value=-999;
        assert(!chrif_auth_achievement_saved(7,17,retained->achievement_generation,true));
        connected=true;retry();
        assert(kind(wire.back())==logout_save::request);
        const auto request=wire.back();
        deliver(wire.size());
        assert(saved==expected);
        assert(wire.size()==1 && kind(wire[0])==logout_save::response);
        const auto ack=wire[0];wire.clear();
        // Wrong identities, generation, version and reserved bits cannot free saves.
        for(size_t offset:{size_t(2),size_t(4),size_t(8),size_t(12),size_t(16)}) {
            incoming=ack;incoming[offset]^=1;chrif_save_barrier_ack(1);
            assert(retained->logout_saves && wire.empty());
        }
        // Character-side validation rejects malformed/stale owners; a partial frame waits.
        incoming=request;readable=request.size()-1;assert(chmapif_parse_logout_barrier(1,0)==0 && wire.empty());
        for(size_t offset:{size_t(2),size_t(4),size_t(8),size_t(12)}) {
            incoming=request;incoming[offset]^=1;readable=incoming.size();rejected=false;
            assert(chmapif_parse_logout_barrier(1,0)==0 && rejected && wire.empty());
        }
        incoming=request;readable=request.size();rejected=false;
        assert(chmapif_parse_logout_barrier(1,1)==0 && rejected && wire.empty());
        incoming=ack;chrif_save_barrier_ack(1);
        assert(!retained->logout_saves);
        assert(wire.size()==1 && kind(wire[0])==(achievements?0x30a5:0x2b01));
        wire.clear();chrif_save_barrier_ack(1);assert(wire.empty()); // Duplicate reply.
        for(auto& row:saved)row.second+=1000;
        retry();no_dependencies();wire.clear(); // Never replay assets after barrier ACK.
        if(achievements){
            assert(!chrif_auth_achievement_saved(7,18,retained->achievement_generation,true));
            assert(!chrif_auth_achievement_saved(7,17,retained->achievement_generation+1,true));
            assert(chrif_auth_achievement_saved(7,17,retained->achievement_generation,false));
            assert(wire.empty() && retained->achievement_pending);
            assert(chrif_auth_achievement_saved(7,17,retained->achievement_generation,true));
            assert(wire.size()==1 && kind(wire[0])==0x2b01);wire.clear();
        }
        retry();no_dependencies();assert(wire.size()==1 && kind(wire[0])==0x2b01);wire.clear();
        assert(retained->final_save_pending);
        delete retained->logout_saves;free(retained->achievement_snapshot);free(retained->logout_status);delete retained;retained=nullptr;
        ++cases;
    }
    // Incomplete or oversized journals fail closed and retain ownership.
    ChrifSaveBuffer buffer;uint8 bytes[4]{};
    assert(!buffer.append(bytes,65536) && buffer.failed);
    buffer={};buffer.bytes=ChrifSaveBuffer::max_bytes-2;
    assert(!buffer.append(bytes,4) && buffer.failed);
    auth_node blocked;blocked.state=ST_LOGOUT;blocked.logout_saves=&buffer;
    assert(!chrif_send_retained_logout(&blocked) && wire.empty());
    std::cout<<"PASS logout recovery cases="<<cases<<"; every packet-loss boundary, offline capture, ACK identity, disabled achievements and no stale replay\n";
}
