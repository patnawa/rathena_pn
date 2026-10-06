"""Execute production auction packet handlers with explicit transport/wallet spies."""
from pathlib import Path
import subprocess,tempfile
from achievement_persistence_test import function
ROOT=Path(__file__).resolve().parents[2]
prefix=r'''
#include <cstdint>
#include <cstdio>
using int32=int32_t;using uint32=uint32_t;
#define PACKETVER 20260219
#define LOG_TYPE_AUCTION 0
struct map_session_data {int fd=1;struct{uint32 char_id=1;char name[24]="Fixture";}status;bool pending=false;};
struct {bool feature_auction=false;}battle_config;
struct PACKET_CZ_AUCTION_BUY {uint32 auction_id=1;int32 money=100;};
PACKET_CZ_AUCTION_BUY packet;
struct {int pos[1]{};}packet_db[1];
#define RFIFOP(fd,pos) (&packet)
#define RFIFOW(fd,pos) 0
#define RFIFOL(fd,pos) 1
int sends=0,debits=0;bool authorized=true,offline=false;
bool pc_transaction_pending(const map_session_data* sd){return sd->pending;}
bool pc_can_give_items(map_session_data*){return authorized;}
void clif_displaymessage(int,const char*){}
const char* msg_txt(map_session_data*,int){return "";}
bool CheckForCharServer(){return offline;}
void clif_Auction_message(int,int){}
int pc_payzeny(map_session_data*,int32,int){++debits;return 0;}
void intif_Auction_bid(uint32,const char*,uint32,int32){++sends;}
bool pn_auction_bid(map_session_data*,uint32,int32){++sends;return true;}
void intif_Auction_cancel(uint32,uint32){++sends;}
void intif_Auction_close(uint32,uint32){++sends;}
'''
body='\n'.join(function(ROOT/'src/map/clif.cpp','void clif_parse_Auction_'+name+'(') for name in ('cancel','close'))
body+='\n'+function(ROOT/'src/map/clif.cpp','void clif_parse_Auction_bid(')
main=r'''
int main(){int failures=0,cases=0;map_session_data sd;
for(auto handler:{clif_parse_Auction_bid,clif_parse_Auction_cancel,clif_parse_Auction_close}){
  for(int state=0;state<3;++state){battle_config.feature_auction=state!=0;sd.pending=state==1;sends=debits=0;
    handler(1,&sd);bool ok=state==2?(sends==1&&debits==0):(sends==0&&debits==0);
    std::printf("AUCTION_GATE state=%d sends=%d debits=%d %s\n",state,sends,debits,ok?"PASS":"FAIL");failures+=!ok;++cases;
  }
}
std::printf("AUCTION_ADMISSION cases=%d failures=%d\n",cases,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-auction-admission-') as temp:
    root=Path(temp);source=root/'test.cpp';exe=root/'test'
    source.write_text('#include <initializer_list>\n'+prefix+body+main)
    subprocess.run(['g++','-std=c++17','-fsanitize=undefined','-fno-sanitize-recover=all',str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
