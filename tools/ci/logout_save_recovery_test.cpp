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
#include <custom/registry_save.hpp>
#include <arpa/inet.h>
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
    int login_id1=1, login_id2=2, fd=2, mapindex=0, id=7, x=20, y=30, group_id=0;
    pn_registry::Journal registry_saves;
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
    ChrifSaveBuffer* transfer_request=nullptr;
    bool transfer_saved=false;
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
auth_node* chrif_search(uint32 account){return retained && retained->account_id==account?retained:nullptr;}
auth_node* chrif_auth_check(uint32 a,uint32 c,sd_state s){return retained && retained->account_id==a && retained->char_id==c && retained->state==s?retained:nullptr;}
bool chrif_auth_delete(uint32,uint32,sd_state){return true;}
t_tick gettick(){return 100;}
struct socket_data { void* session_data=nullptr; uint32 client_addr=0; } sockets[3];
socket_data* session[]={&sockets[0],&sockets[1],&sockets[2]};
bool session_isValid(int fd) { return fd>0 && fd<3; }
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
#define WBUFW(p,pos) (*reinterpret_cast<uint16*>((p)+(pos)))
#define WBUFL(p,pos) (*reinterpret_cast<uint32*>((p)+(pos)))
#define WBUFB(p,pos) (*reinterpret_cast<uint8*>((p)+(pos)))
#define WBUFCP(p,pos) reinterpret_cast<char*>((p)+(pos))
void WFIFOSET(int,size_t n){wire.emplace_back(scratch.begin(),scratch.begin()+n);}
void emit(uint16 type,int value){ if(!chrif_save_available())return; std::vector<uint8> p(6); memcpy(p.data(),&type,2);memcpy(p.data()+2,&value,4);chrif_save_packet(p.data(),p.size()); }
void chrif_save_scdata(map_session_data* s){emit(10,s->statuses);}
void chrif_skillcooldown_save(const map_session_data& s){emit(11,s.cooldown);}
void chrif_bsdata_save(map_session_data* s,bool){emit(9,s->bonus);}
void intif_storage_save(map_session_data*,s_storage* s){emit(s->type,s->value);}
void storage_storagesave(map_session_data* s){intif_storage_save(s,&s->storage);}
void storage_premiumStorage_save(map_session_data* s){intif_storage_save(s,&s->premiumStorage);}
void storage_guild_storagesave(uint32,int,int){}
int32 intif_saveregistry(map_session_data* s){emit(8,s->registry);s->vars_dirty=false;return 0;}
void intif_registry_replay(const map_session_data* s) {
    if (s) for (const auto& packet : s->registry_saves.packets) chrif_save_packet(packet.data(), packet.size());
}
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
int32 chrif_changemapserver(map_session_data*,uint32,uint16);
int other_mapserver_count=1;
constexpr size_t MAP_NAME_LENGTH_EXT=24;
const char* mapindex_id2name(int) { return "prontera"; }
void safestrncpy(char* dest,const char* source,size_t length) { std::snprintf(dest,length,"%s",source); }
#define WFIFOCP(fd,pos) reinterpret_cast<char*>(WFIFOP(fd,pos))
void clif_authfail_fd(int,int){}
size_t readable=0; bool rejected=false;
#define RFIFOREST(fd) readable
void RFIFOSKIP(int,size_t n){readable-=n;}
void set_eof(int){rejected=true;}
struct Owner {uint32 char_id=17; int server=0;};
std::map<uint32,std::shared_ptr<Owner>> owners;
auto& char_get_onlinedb(){return owners;}
constexpr int MAX_MAP_SERVERS=1;
struct MapServer { int fd=1; } map_server[1];
bool status_sql_ok=true;
int char_mmo_char_tosql(uint32,mmo_charstatus*) { return status_sql_ok?0:1; }
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
#ifndef PN_TEST_TRANSFER
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
#else
int main() {
    map_session_data sd; achievement a{42}; sd.achievement_data.achievements=&a;
    pn_registry::Packet registry(pn_registry::header_size);
    assert(pn_registry::append_entry(registry,{"QuestStep","",0,17,0}));
    std::vector<pn_registry::Packet> pending{registry};
    assert(sd.registry_saves.retain(7,17,pending));
    const auto registry_ack=pn_registry::acknowledgement(sd.registry_saves.packets[0].data(),true);
    owners[7]=std::make_shared<Owner>();
    chrif_save(&sd,CSAVE_CHANGE_MAPSERV|CSAVE_INVENTORY|CSAVE_CART);
    chrif_changemapserver(&sd,0x7f000001,5122);
    for(const auto& packet:wire) if(kind(packet)==0x2b05) {
        std::cerr<<"FAIL map transfer admitted before achievement save acknowledgement\n"; return 1;
    }
    assert(retained->achievement_pending && retained->achievement_snapshot[0].value==42);
    // The live achievement memory and client socket disappear while waiting.
    sd.achievement_data.achievements=nullptr;sd.fd=99;
    deliver(wire.size());assert(wire.size()==1 && kind(wire[0])==logout_save::response);
    incoming=wire[0];wire.clear();chrif_save_barrier_ack(1);
    // The stream barrier cannot stand in for an asynchronous login/registry ACK.
    for(const auto& packet:wire) assert(kind(packet)==pn_registry::request_packet);
    wire.clear();assert(!retained->final_save_pending && retained->achievement_pending);
    assert(sd.registry_saves.acknowledge(registry_ack.data(),registry_ack.size()));
    chrif_registry_saved(&sd);
    assert(wire.size()==1 && kind(wire[0])==0x30a5);wire.clear();
    assert(chrif_auth_achievement_saved(7,17,retained->achievement_generation,false));
    assert(retained->achievement_pending && wire.empty());
    retry();assert(wire.size()==1 && kind(wire[0])==0x30a5);wire.clear();
    assert(!chrif_auth_achievement_saved(7,17,retained->achievement_generation+1,true));
    assert(chrif_auth_achievement_saved(7,17,retained->achievement_generation,true));
    assert(wire.size()==1 && kind(wire[0])==logout_save::transfer_status_request);
    const auto request=wire[0];wire.clear();incoming=request;readable=request.size();status_sql_ok=false;
    assert(chmapif_parse_transfer_status(1,0)==1 && wire.empty());
    retry();assert(wire.size()==1 && wire[0]==request);wire.clear();
    incoming=request;readable=request.size();status_sql_ok=true;
    assert(chmapif_parse_transfer_status(1,0)==1 && wire.size()==1);
    const auto ack=wire[0];wire.clear();
    for(size_t offset:{size_t(4),size_t(6),size_t(8),size_t(12),size_t(16)}) {
        incoming=ack;incoming[offset]^=1;chrif_transfer_save_ack(1);assert(wire.empty());
    }
    incoming=ack;chrif_transfer_save_ack(1);
    assert(wire.size()==1 && kind(wire[0])==0x2b05);
    const auto handoff=wire[0];wire.clear();
    retry();assert(wire.size()==1 && wire[0]==handoff);wire.clear();
    incoming=ack;chrif_transfer_save_ack(1);assert(wire.empty());
    std::cout<<"PASS transfer retains achievements, rejects failed/stale ACKs, waits for status SQL and retries owned handoff after socket teardown\n";
    delete retained->logout_saves;delete retained->transfer_request;free(retained->achievement_snapshot);free(retained->logout_status);delete retained;retained=nullptr;
}
#endif
