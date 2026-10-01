#!/usr/bin/env python3
"""Compile the actual modern and legacy Warp Portal list serializers."""
from pathlib import Path
import subprocess
import tempfile
from episode_party_progression_test import scan_to

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/map/clif.cpp').read_text(encoding='utf-8')
start=source.index('void clif_skill_warppoint(');brace=source.index('{',start)
function=source[start:scan_to(source,brace,'{','}')+1]
cpp=r'''
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <cstdio>
using uint16=uint16_t;
constexpr int HEADER_ZC_WARPLIST=0xabe,AL_WARP=27,SELF=0,WIP_DISABLE_ALL=1;
struct PACKET_ZC_WARPLIST_sub {char map[16];};
#ifdef LEGACY
#define PACKETVER_MAIN_NUM 20100101
struct PACKET_ZC_WARPLIST {uint16 packetType,skillId;PACKET_ZC_WARPLIST_sub maps[4];};
#else
#define PACKETVER_MAIN_NUM 20250401
struct PACKET_ZC_WARPLIST {uint16 packetType,packetLength,skillId;PACKET_ZC_WARPLIST_sub maps[];};
#endif
struct map_session_data {int menuskill_id=0,menuskill_val=0;struct {int skillx=10,skilly=20;}ud;struct{int workinprogress=0;}state;};
alignas(8) char packet_buffer[4096];std::vector<std::string> sent;
void mapindex_getmapname_ext(const char* name,char* output){std::snprintf(output,16,"%s.gat",name);}
void clif_send(const PACKET_ZC_WARPLIST* packet,size_t size,map_session_data*,int) {
#ifdef LEGACY
 size_t count=4;
#else
 size_t count=(packet->packetLength-sizeof(*packet))/16;
#endif
 for(size_t i=0;i<count;++i)sent.emplace_back(packet->maps[i].map);
}
'''
cpp+=function+r'''
int main() {
 map_session_data sd;std::vector<std::string> maps={"","save","map0","map1","map2","map3","map4","map5"};
 clif_skill_warppoint(sd,AL_WARP,4,maps);
#ifdef LEGACY
 bool good=sent.size()==4 && sent.back()=="map2.gat";
#else
 bool good=sent.size()==7 && sent.back()=="map5.gat";
#endif
 good=good && sent.front()=="save.gat" && sd.menuskill_id==AL_WARP && sd.menuskill_val==((10<<16)|20) && sd.state.workinprogress==WIP_DISABLE_ALL;
 std::printf("MEMO_PACKET destinations=%zu correct=%d\n",sent.size(),good);return good?0:1;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-memo-packet-') as temp:
    path=Path(temp)/'test.cpp';path.write_text(cpp)
    for name,defines in [('modern',[]),('legacy',['-DLEGACY'])]:
        exe=Path(temp)/name
        subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',*defines,str(path),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
