// Transport/timer/account boundaries for verbatim login roster implementation.
#include "login/login.hpp"
#include "common/utilities.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace rathena;

static void check(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
static std::vector<int32> cancelled;
static std::vector<uint32> enabled,disabled;
static bool enable_token(AccountDB*,uint32 id){enabled.push_back(id);return true;}
static bool disable_token(AccountDB*,uint32 id){disabled.push_back(id);return true;}
static AccountDB fake_accounts{};
AccountDB* accounts=&fake_accounts;
std::unordered_map<uint32,online_login_data> online_db;
std::unordered_map<uint32,auth_node> auth_db;
int32 delete_timer(int32 tid,TimerFunc){cancelled.push_back(tid);return 0;}

// The source sender and receiver use these bounded FIFO adapters. Scalar bytes
// retain their native little-endian packet layout; no sockets are opened.
static std::vector<uint8> outgoing,incoming;
static size_t consumed=0;
static bool disconnected=false;
static void set_eof(int32){disconnected=true;}
template<class T> struct Wire {
    size_t offset;
    Wire& operator=(T value){check(offset+sizeof(T)<=outgoing.size(),"sender exceeded reserved FIFO");std::memcpy(outgoing.data()+offset,&value,sizeof(value));return *this;}
    operator T()const{check(offset+sizeof(T)<=outgoing.size(),"sender read exceeded FIFO");T value;std::memcpy(&value,outgoing.data()+offset,sizeof(value));return value;}
};
template<class T> static T receive(size_t offset){check(consumed+offset+sizeof(T)<=incoming.size(),"receiver read beyond complete packet");T value;std::memcpy(&value,incoming.data()+consumed+offset,sizeof(value));return value;}
static void flush(size_t length){check(length<=outgoing.size(),"sender invalid packet length");incoming.assign(outgoing.begin(),outgoing.begin()+length);consumed=0;}
#define WFIFOHEAD(fd,length) outgoing.assign(length,0)
#define WFIFOW(fd,offset) Wire<uint16>{static_cast<size_t>(offset)}
#define WFIFOL(fd,offset) Wire<uint32>{static_cast<size_t>(offset)}
#define WFIFOSET(fd,length) flush(length)
#define RFIFOREST(fd) (incoming.size()-consumed)
#define RFIFOW(fd,offset) receive<uint16>(offset)
#define RFIFOL(fd,offset) receive<uint32>(offset)
#define RFIFOSKIP(fd,length) do{const size_t n=(length);check(n<=RFIFOREST(fd),"receiver invalid consume");consumed+=n;}while(0)
static bool chlogif_isconnected(){return true;}
static int32 login_fd=1;

// @ACTUAL_SOURCE@

static void seed(uint32 id,int32 server,int32 timer=INVALID_TIMER){
    auto& row=online_db[id];row.account_id=id;row.char_server=server;row.waiting_disconnect=timer;
}
static void roster(uint32 id,int16 server){
    auto row=std::make_shared<online_char_data>(id);row->server=server;char_get_onlinedb()[id]=row;
}
int main(int argc,char** argv){
    try{
        check(argc==2,"one named case required");const std::string test=argv[1];
        fake_accounts.enable_webtoken=enable_token;fake_accounts.disable_webtoken=disable_token;
        if(test=="packet-roundtrip"){
            roster(99060101,0);roster(99060102,-1);
            check(chlogif_send_acc_tologin(0,0,0,0)==1,"actual sender accepted roster");
            check(incoming.size()==12 && receive<uint16>(0)==0x272d && receive<uint32>(4)==1 && receive<uint32>(8)==99060101,"actual sender wire contract");
            check(logchrif_parse_updonlinedb(0,1)==1 && consumed==12,"actual receiver consumes packet");
            for(const auto& entry:online_db)std::cout<<"OBSERVED_ACCOUNT "<<entry.first<<" server="<<entry.second.char_server<<std::endl;
            check(online_db.size()==1 && online_db.count(99060101)==1,"sender account ID must survive receiver without a ghost account");
            check(enabled==std::vector<uint32>{99060101},"webtoken enabled only for transmitted account");
        }else if(test=="server-offline"){
            seed(99060101,1,71);seed(99060102,2,72);
            login_online_db_setoffline(1);
            std::cout<<"OBSERVED_SERVER "<<online_db.at(99060101).char_server<<std::endl;
            check(online_db.at(99060101).char_server==-2,"offline roster reset must persist in the real map");
            check(online_db.at(99060102).char_server==2,"another char server must remain online");
            check(cancelled.empty(),"server-specific reset retains its normal disconnect timer");
        }else if(test=="all-offline"){
            seed(99060101,1,71);seed(99060102,2);seed(99060103,-2,73);
            login_online_db_setoffline(-1);
            for(const auto& entry:online_db)check(entry.second.char_server==-1 && entry.second.waiting_disconnect==INVALID_TIMER,"global offline reset must persist state and timer cancellation");
            std::sort(cancelled.begin(),cancelled.end());check(cancelled==std::vector<int32>({71,73}),"each pending timer cancelled once");
            login_online_db_setoffline(-1);check(cancelled.size()==2,"repeated reset cannot cancel stale timer IDs");
        }else if(test=="cleanup"){
            for(uint32 i=1;i<=192;++i)seed(99060000+i,i<=128?-2:(i%2?1:-1),i<=128?1000+i:INVALID_TIMER);
            check(login_online_data_cleanup(0,0,0,0)==0,"cleanup callback returns normally");
            check(online_db.size()==64 && disabled.size()==128 && cancelled.size()==128,"cleanup removes every unknown record once");
            for(uint32 i=1;i<=128;++i)check(!online_db.count(99060000+i),"unknown account removed");
            for(uint32 i=129;i<=192;++i)check(online_db.count(99060000+i)==1,"known or offline account preserved");
            std::sort(disabled.begin(),disabled.end());check(std::adjacent_find(disabled.begin(),disabled.end())==disabled.end(),"no duplicate webtoken invalidation");
        }else if(test=="multiple-empty-isolation"){
            seed(99060101,1);seed(99060102,1,72);seed(99060103,2,73);
            roster(99060101,0);roster(99060104,1);roster(99060105,-1);
            chlogif_send_acc_tologin(0,0,0,0);
            check(incoming.size()==16 && receive<uint32>(4)==2,"actual sender includes two active accounts only");
            check(logchrif_parse_updonlinedb(0,1)==1 && consumed==16 && !disconnected,"multiple roster accepted");
            check(online_db.size()==4 && online_db.at(99060101).char_server==1 && online_db.at(99060104).char_server==1,"multiple account IDs preserved");
            check(online_db.at(99060102).char_server==-2 && online_db.at(99060103).char_server==2,"omitted account marked unknown and other server isolated");
            login_online_data_cleanup(0,0,0,0);
            check(!online_db.count(99060102) && cancelled==std::vector<int32>{72} && disabled==std::vector<uint32>{99060102},"omitted account cleaned with its timer and token once");
            char_get_onlinedb().clear();chlogif_send_acc_tologin(0,0,0,0);
            check(incoming.size()==8 && receive<uint32>(4)==0,"actual empty roster header");
            check(logchrif_parse_updonlinedb(0,1)==1 && consumed==8,"empty roster accepted");
            login_online_data_cleanup(0,0,0,0);
            check(online_db.size()==1 && online_db.at(99060103).char_server==2 && online_db.at(99060103).waiting_disconnect==73,"empty roster cleans its server only");
        }else if(test=="empty-roster"){
            seed(99060101,1,71);seed(99060102,1);seed(99060103,2,73);
            check(char_get_onlinedb().empty(),"empty sender roster");chlogif_send_acc_tologin(0,0,0,0);
            check(incoming.size()==8 && receive<uint32>(4)==0 && logchrif_parse_updonlinedb(0,1)==1 && consumed==8,"empty roster received in full");
            check(online_db.at(99060101).char_server==-2 && online_db.at(99060102).char_server==-2,"empty roster marks its previous accounts unknown");
            login_online_data_cleanup(0,0,0,0);login_online_data_cleanup(0,0,0,0);
            check(online_db.size()==1 && online_db.at(99060103).char_server==2 && online_db.at(99060103).waiting_disconnect==73,"empty roster cleanup preserves other server");
            std::sort(disabled.begin(),disabled.end());
            check(cancelled==std::vector<int32>{71} && disabled==std::vector<uint32>({99060101,99060102}),"empty roster cleanup cancels timer and disables each token only once");
        }else if(test=="partial-packet"){
            roster(99060101,0);roster(99060102,1);chlogif_send_acc_tologin(0,0,0,0);
            const auto complete=incoming;
            seed(99060103,1,73);
            for(size_t length=0;length<complete.size();++length){
                incoming.assign(complete.begin(),complete.begin()+length);consumed=0;disconnected=false;
                check(logchrif_parse_updonlinedb(0,1)==0 && consumed==0 && !disconnected,"partial valid packet waits without consuming or disconnecting");
                check(online_db.size()==1 && online_db.at(99060103).char_server==1 && online_db.at(99060103).waiting_disconnect==73 && enabled.empty() && disabled.empty() && cancelled.empty(),"partial packet cannot mutate roster or side effects");
            }
            incoming=complete;incoming.insert(incoming.end(),{0x19,0x27,0x19,0x27});consumed=0;
            check(logchrif_parse_updonlinedb(0,1)==1 && consumed==complete.size() && RFIFOREST(0)==4,"completed packet consumes only its own bytes");
            check(online_db.count(99060101)==1 && online_db.count(99060102)==1,"completed fragmented packet retains real account IDs");
        }else if(test=="malformed-packet"){
            struct Invalid {uint16 length;uint32 users;};
            for(const auto bad:std::vector<Invalid>{{0,0},{4,0},{6,0},{9,0},{8,1},{12,2},{12,0},{8,65536},{8,0xffffffff}}){
                online_db.clear();enabled.clear();disabled.clear();cancelled.clear();seed(99060103,1,73);
                outgoing.assign(std::max<size_t>(8,bad.length),0);WFIFOW(0,0)=0x272d;WFIFOW(0,2)=bad.length;WFIFOL(0,4)=bad.users;
                flush(outgoing.size());disconnected=false;
                check(logchrif_parse_updonlinedb(0,1)==0 && disconnected && consumed==0,"malformed length/count must disconnect before roster mutation");
                check(online_db.size()==1 && online_db.at(99060103).char_server==1 && online_db.at(99060103).waiting_disconnect==73 && enabled.empty() && disabled.empty() && cancelled.empty(),"invalid packet preserves existing roster and timer/token state");
            }
        }else throw std::runtime_error("unknown case");
        std::cout<<"LOGIN_ROSTER_PASS "<<test<<std::endl;return 0;
    }catch(const std::exception& error){std::cerr<<"LOGIN_ROSTER_FAIL "<<(argc>1?argv[1]:"arguments")<<": "<<error.what()<<std::endl;return 1;}
}
