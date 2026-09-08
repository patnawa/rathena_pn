#!/usr/bin/env python3
"""Compile the native persistent kill counter and verify its single death-hook placement."""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'src/map/mob.cpp').read_text()
start=source.index('static void mob_player_killcounter(')
end=source.index('/*==========================================\n * Signals death of mob.',start)
helper=source[start:end]
call='mob_player_killcounter( first_sd, *md, src, rebirth );'
assert source.count(call)==1
pos=source.index(call)
assert source.index('rebirth = ',source.index('int32 mob_dead(')) < pos < source.index('if( md->npc_event[0]',pos)
script=(ROOT/'npc/custom/player_commands.txt').read_text()
assert 'OnNPCKillEvent:' not in script, 'script handler would double-count ordinary mobs'
assert 'PNKCKills[.@i]+1' not in script
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <unordered_map>
#include <iostream>
using int32=int32_t;using uint32=uint32_t;using int64=int64_t;using uint64=uint64_t;
#define reference_uid(id,idx) ((int64)((uint64)(id)&0xFFFFFFFF)|((uint64)(idx)<<32))
struct map_session_data {std::unordered_map<int64,int64> persistent;int writes=0;};
struct mob_data {int mob_id=1002;struct{bool npc_killmonster=false;} state;const char* npc_event="";};
struct block_list {};
int add_str(const char* s){if(!strcmp(s,"PNKCMob"))return 1;assert(!strcmp(s,"PNKCKills"));return 2;}
int64 pc_readregistry(map_session_data* sd,int64 key){return sd->persistent[key];}
bool pc_setregistry(map_session_data* sd,int64 key,int64 value){sd->persistent[key]=value;++sd->writes;return true;}
'''
suffix=r'''
int main(){
 map_session_data sd,other;mob_data mob;block_list killer;
 for(int i=0;i<5;i++){sd.persistent[reference_uid(1,i)]=1002;other.persistent[reference_uid(1,i)]=1002;}
 mob_player_killcounter(&sd,mob,&killer,false);
 for(int i=0;i<5;i++)assert(sd.persistent[reference_uid(2,i)]==1);
 // A scripted instance monster has identical credit, independent of its label.
 mob.npc_event="InstanceBoss::OnDead";mob_player_killcounter(&sd,mob,&killer,false);
 for(int i=0;i<5;i++)assert(sd.persistent[reference_uid(2,i)]==2);
 assert(sd.writes==10&&other.writes==0); // No implicit party/other-player distribution.
 mob_player_killcounter(nullptr,mob,&killer,false);
 mob_player_killcounter(&sd,mob,nullptr,false);
 mob_player_killcounter(&sd,mob,&killer,true);
 mob.state.npc_killmonster=true;mob_player_killcounter(&sd,mob,&killer,false);
 assert(sd.writes==10);mob.state.npc_killmonster=false;
 sd.persistent[reference_uid(1,0)]=0;sd.persistent[reference_uid(1,1)]=1003;
 sd.persistent[reference_uid(2,2)]=INT32_MAX;
 sd.persistent[reference_uid(2,3)]=INT64_MAX;
 sd.persistent[reference_uid(2,4)]=-1;
 mob_player_killcounter(&sd,mob,&killer,false);
 assert(sd.writes==11&&sd.persistent[reference_uid(2,4)]==1);
 assert(sd.persistent[reference_uid(2,2)]==INT32_MAX&&sd.persistent[reference_uid(2,3)]==INT64_MAX);
 std::cout<<"PASS: five persistent slots, labeled monsters, death guards, attribution isolation, saturation and single hook\n";
}
'''
with tempfile.TemporaryDirectory(prefix='pn-killcounter-') as temp:
    cpp=Path(temp)/'test.cpp';binary=Path(temp)/'test'
    cpp.write_text(prefix+helper+suffix)
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=undefined','-o',str(binary),str(cpp)],check=True)
    subprocess.run([str(binary)],check=True)
