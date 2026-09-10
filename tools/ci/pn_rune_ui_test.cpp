// Project contributions: (C) 2026 PN Development Team.
// GPL-3.0-or-later; see LICENSE. Existing upstream rights retained.
// Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/pn_rune_ui_test.cpp

#include <cstdint>
#include <cstring>
#include <cassert>
#include <map>
#include <string>
#include <vector>
#include <iostream>
using int32=int32_t; using int64=int64_t; using uint8=uint8_t; using uint16=uint16_t; using uint32=uint32_t;
struct map_session_data { int fd=1; int npc_id=0; struct {bool trading=false; bool storage_flag=false;} state; std::map<int64,int64> regs; };
std::map<std::string,int64> names;
int64 add_str(const char* name) { auto found=names.find(name); if(found!=names.end())return found->second; return names[name]=names.size()+1; }
int64 reference_uid(int64 name,uint32 index){return name|(int64(index)<<32);}
int64 pc_readregistry(map_session_data* sd,int64 key){return sd->regs[key];}
int64 pc_readreg2(map_session_data* sd,const char* name){return sd->regs[add_str(name)];}
void pc_setreg(map_session_data* sd,int64 key,int64 value){sd->regs[key]=value;}
std::vector<uint8> buffer(10000),request(20);
std::vector<std::vector<uint8>> sent;
int events=0;
void npc_event(map_session_data*,const char* event,int){assert(std::string(event)=="PN Rune Stone::OnNativeAction");++events;}
#define WFIFOHEAD(fd,n) ((void)(fd),buffer.assign((n),0))
#define WFIFOW(fd,n) (*reinterpret_cast<uint16*>(buffer.data()+(n)+0*(fd)))
#define WFIFOB(fd,n) (buffer[(n)+0*(fd)])
#define WFIFOL(fd,n) (*reinterpret_cast<uint32*>(buffer.data()+(n)+0*(fd)))
#define WFIFOSET(fd,n) ((void)(fd),sent.emplace_back(buffer.begin(),buffer.begin()+(n)))
#define RFIFOW(fd,n) (*reinterpret_cast<uint16*>(request.data()+(n)+0*(fd)))
#define RFIFOB(fd,n) (request[(n)+0*(fd)])
#define RFIFOL(fd,n) (*reinterpret_cast<uint32*>(request.data()+(n)+0*(fd)))
#include "../../src/map/pn_rune_ui.hpp"
uint16 word(const std::vector<uint8>& p,size_t i){uint16 v;std::memcpy(&v,p.data()+i,2);return v;}
uint32 dword(const std::vector<uint8>& p,size_t i){uint32 v;std::memcpy(&v,p.data()+i,4);return v;}
void req(uint16 op,uint16 tag=0,uint32 id=0){request.assign(20,0);std::memcpy(request.data(),&op,2);std::memcpy(request.data()+2,&tag,2);std::memcpy(request.data()+4,&id,4);}
int main(){
 map_session_data sd;
 clif_rune_ui_open(sd);assert(word(sent.back(),0)==0xbdf && sent.back()[2]==1);
 req(0xbe0,1);clif_parse_rune_ui(1,&sd);assert(sent.size()==16 && pc_readreg2(&sd,"@PNRTUIOpen")==1);
 for(size_t i=2;i<sent.size();++i){assert(sent[i].size()==9 && word(sent[i],2)==9 && word(sent[i],7)==0);}
 sd.regs[reference_uid(add_str("#PNRTPiece"),0)]=1;
 sd.regs[reference_uid(add_str("PNRTPaid"),0)]=1;
 sd.regs[reference_uid(add_str("PNRTLevel"),0)]=2;
 sd.regs[reference_uid(add_str("PNRTPity"),0)]=8000;
 req(0xbcb,17);clif_parse_rune_list(1,&sd);
 auto pieces=sent[sent.size()-2],sets=sent.back();
 assert(word(pieces,0)==0xbcc && pieces.size()==13 && word(pieces,5)==17 && word(pieces,7)==1 && dword(pieces,9)==1263000);
 assert(word(sets,0)==0xbcd && sets.size()==17 && dword(sets,9)==1260000 && word(sets,13)==2 && word(sets,15)==2);
 size_t count=sent.size();req(0xbcb,65535);clif_parse_rune_list(1,&sd);assert(sent.size()==count);
 auto saved=sd.regs;
 req(0xbce,17,1260000);clif_parse_rune_action(1,&sd);assert(events==0);
 req(0xbd2,17,1263000);clif_parse_rune_action(1,&sd);assert(events==0);
 req(0xbce,17,1263000);clif_parse_rune_action(1,&sd);assert(events==1 && !pc_readreg2(&sd,"@PNRTUIOpen"));
 assert(pc_readreg2(&sd,"@PNRTNativeCommand")==0xbce && pc_readreg2(&sd,"@PNRTNativeID")==1263000);
 assert(word(sent.back(),0)==0xbdf && sent.back()[2]==0);
 clif_parse_rune_action(1,&sd);assert(events==1);
 for(auto& v:saved)if(v.first!=add_str("@PNRTUIOpen"))assert(sd.regs[v.first]==v.second);
 sd.npc_id=100;req(0xbe0,1);clif_parse_rune_ui(1,&sd);assert(sent.back()[2]==0);
 sd.npc_id=0;sd.state.trading=true;clif_parse_rune_ui(1,&sd);assert(!pc_readreg2(&sd,"@PNRTUIOpen"));
 sd.state.trading=false;req(0xbe0,7);count=sent.size();clif_parse_rune_ui(1,&sd);assert(count==sent.size());
 std::cout<<"PASS native open/close, empty and owned lists, levels/pity, invalid tags/IDs, busy sessions, duplicate actions, no progression mutation in transport\n";
}
