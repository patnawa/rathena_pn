#!/usr/bin/env python3
"""Compile production weight recalculation and map-transition branch with doubles."""
from pathlib import Path
import re
import subprocess
import tempfile
import struct
import zlib
from episode_party_progression_test import scan_to

ROOT=Path(__file__).resolve().parents[2]
def function(source,signature):
    start=source.index(signature); brace=source.index('{',start)
    return source[start:scan_to(source,brace,'{','}')+1]
pc=(ROOT/'src/map/pc.cpp').read_text(encoding='utf-8')
status=(ROOT/'src/map/status.cpp').read_text(encoding='utf-8')
weight=function(status,'bool status_calc_weight(')
percent=function(pc,'uint16 pc_getpercentweight(')
transition=pc[pc.index('\tconst bool previous_unlimited_weight ='):pc.index('\n\tif( sd->status.guild_id',pc.index('\tconst bool previous_unlimited_weight ='))]
cpp=r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <climits>
using int32=int32_t; using uint16=uint16_t; using uint32=uint32_t; using uint64=uint64_t;
enum e_status_calc_weight_opt { CALCWT_ITEM=1,CALCWT_MAXBONUS=2 };
constexpr int MAX_INVENTORY=2, MF_UNLIMITEDWEIGHT=1, SP_WEIGHT=2,SP_MAXWEIGHT=3;
constexpr int MC_INCCARRY=1,KN_RIDING=2,RK_DRAGONTRAINING=3,SC_KNOWLEDGE=4,ALL_INCCARRY=5;
struct entry { int val1=0; };
struct status_change { entry* getSCE(int) { return nullptr; } };
struct item_data { int weight; };
struct map_session_data {
    status_change sc;
    struct { int str=10,sex=0; } status;
    struct { struct { struct { int nameid=0,amount=0; } items_inventory[2]; } u; } inventory;
    item_data* inventory_data[2]={};
    struct { bool active=true; } state;
    struct { int to_x=0,to_y=0; } ud;
    int m=0,mapindex=0,x=0,y=0,class_=0;
    uint32 weight=0,max_weight=0,add_max_weight=1000;
    int gym=2;
};
struct { int get_maxWeight(int) { return 20000; } } job_db;
bool market_enabled=true; int packets=0,recalculations=0; uint16 last_percent=0;
int map_getmapflag(int m,int) { return m==1 && market_enabled; }
int pc_mapid2jobid(int,int) { return 0; }
int pc_checkskill(map_session_data* sd,int id) { return id==ALL_INCCARRY?sd->gym:0; }
bool pc_isriding(map_session_data*) { return false; }
bool pc_isridingdragon(map_session_data*) { return false; }
bool pc_ismadogear(map_session_data*) { return false; }
void clif_updatestatus(map_session_data&,int) { ++packets; }
#define nullpo_retr(result,p) if(!(p)) return result
uint16 pc_getpercentweight(const map_session_data&,uint32=0);
void pc_updateweightstatus(map_session_data& sd) { ++recalculations;last_percent=pc_getpercentweight(sd); }
'''
cpp+=percent+'\n'+weight+'\n'
cpp+='void move(map_session_data* sd,int m) { int mapindex=m,x=20,y=20;\n'+transition+'\n}\n'
cpp+=r'''
int main() {
    int checks=0,failures=0;
    auto check=[&](bool result,const char* msg){++checks;if(!result){++failures;std::printf("FAIL %s\n",msg);}};
    map_session_data player;
    status_calc_weight(&player,CALCWT_MAXBONUS);
    check(player.max_weight==28000,"normal job STR gear and Gym bonus");
    move(&player,1); check(player.max_weight==INT32_MAX,"market entry capacity");
    player.weight=1000000000;
    check(pc_getpercentweight(player)==46,"large shopping weight uses wide arithmetic");
    status_calc_weight(&player,CALCWT_MAXBONUS);
    check(player.max_weight==INT32_MAX,"equipment refresh retains market capacity");
    move(&player,0); check(player.max_weight==28000,"exit restores normal and Gym capacity");
    check(last_percent==UINT16_MAX,"exit overweight saturates instead of wrapping");
    player.weight=14000; check(pc_getpercentweight(player)==50,"ordinary percent unchanged");
    move(&player,1); market_enabled=false;
    status_calc_weight(&player,CALCWT_MAXBONUS);
    check(player.max_weight==28000,"runtime mapflag removal restores normal capacity");
    player.max_weight=0;player.weight=UINT32_MAX;
    check(pc_getpercentweight(player)==UINT16_MAX,"zero capacity and maximum load handled");
    check(recalculations>=4 && packets>=4,"client and overweight refreshes delivered");
    std::printf("MARKET_WEIGHT checks=%d failures=%d\n",checks,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-market-weight-') as temp:
    path,exe=Path(temp)/'test.cpp',Path(temp)/'test'
    path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
script=(ROOT/'npc/custom/market_services.txt').read_text()
assert 'sp_rudus' not in script
assert 'itemmall\tmapflag\tunlimitedweight' in script
assert 'npc: npc/custom/market_services.txt' in (ROOT/'npc/scripts_custom.conf').read_text()
positions=re.findall(r'^itemmall,(\d+),(\d+),',script,re.M)
data=(ROOT/'db/map_cache.dat').read_bytes();pos=8;found=False
for _ in range(struct.unpack_from('<H',data,4)[0]):
    name,w,h,size=struct.unpack_from('<12shhi',data,pos);pos+=20
    if name.split(b'\0')[0]==b'itemmall':
        cells=zlib.decompress(data[pos:pos+size]);found=True
        for x,y in positions: assert cells[int(y)*w+int(x)] in (0,3,6)
    pos+=size
assert found and len(positions)==3
print('MARKET_SERVICES three placements walkable; only service mall gains capacity')
