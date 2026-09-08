#!/usr/bin/env python3
"""Exercise actual native self-service command bodies with bounded engine doubles."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT/'src/map/atcommand.cpp').read_text()
start=source.index('static int32 pn_command_toggle(')
end=source.index('/*==========================================\n * Duel organizing',start)
code=source[start:end]
start=source.index('ACMD_FUNC(noask)')
end=source.index('/*=====================================\n * Send a @request',start)
code+=source[start:end]
prefix=r'''
#include <cassert>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include <strings.h>
#include <iostream>
using int32=int;using int16=short;using uint16=unsigned short;
#define PACKETVER 20260219
#define ACMD_FUNC(x) int atcommand_##x(int fd,map_session_data* sd,const char* command,const char* message)
#define cap_value(a,lo,hi) (((a)>=(hi))?(hi):((a)<=(lo))?(lo):(a))
constexpr int MAX_INVENTORY=6,NAV_KAFRA_AND_AIRSHIP=101,CELL_CHKNOPASS=0,SELF=0,HEADER_ZC_INVENTORY_TAB=0x908,AUTOSPELL_FORCE_RANDOM_LEVEL=2;
struct s_autospell {uint16 id=0,lv=0,trigger_skill=0;int16 rate=0,battle_flag=0;unsigned card_id=0,flag=0;};
struct item {int nameid=0,equip=0,favorite=0;};
struct map_session_data {bool cant=false,dead=false;struct {bool showexp=false,showzeny=false,showdelay=false,noask=false;} state;struct {struct {item items_inventory[MAX_INVENTORY];} u;} inventory;std::vector<s_autospell> autospell,autospell2,autospell3;};
struct map_data {int xs=100,ys=100;const char* name="prontera";} map;
struct PACKET_ZC_INVENTORY_TAB {uint16 packetType=0,index=0;bool favorite=false;};
std::vector<std::string> messages;
std::vector<PACKET_ZC_INVENTORY_TAB> packets;
int navigations=0;bool hidden=false;
void clif_displaymessage(int,const char* t){messages.emplace_back(t);}
const char* msg_txt(map_session_data*,int){return "state updated";}
int map_mapname2mapid(const char* s){return std::string(s)=="prontera"?0:-1;}
map_data* map_getmapdata(int){return &map;}
bool map_getcell(int,int x,int y,int){return x==10&&y==10;}
void clif_navigateTo(map_session_data*,const char*,int x,int y,int flags,bool hide,int mob){assert(x==25&&y==30&&flags==101&&mob==0);++navigations;hidden=hide;}
bool pc_cant_act(map_session_data* sd){return sd->cant;}
bool pc_isdead(map_session_data* sd){return sd->dead;}
bool pc_unequipitem(map_session_data* sd,int i,int flags){assert(flags==1);if(i==1)return false;sd->inventory.u.items_inventory[i].equip=0;return true;}
void clif_send(const PACKET_ZC_INVENTORY_TAB* p,size_t,map_session_data*,int){packets.push_back(*p);}
const char* skill_get_desc(int){return "Test Skill";}
'''
suffix=r'''
int main(){
 map_session_data sd;
 for(auto f:{atcommand_showexp,atcommand_showzeny,atcommand_showdelay,atcommand_noask}){
  bool* state=f==atcommand_showexp?&sd.state.showexp:f==atcommand_showzeny?&sd.state.showzeny:f==atcommand_showdelay?&sd.state.showdelay:&sd.state.noask;
  *state=false;assert(f(0,&sd,"@test","on")==0&&*state);assert(f(0,&sd,"@test","on")==0&&*state);
  assert(f(0,&sd,"@test","off")==0&&!*state);assert(f(0,&sd,"@test","OFF")==0&&!*state);
  assert(f(0,&sd,"@test","")==0&&*state);assert(f(0,&sd,"@test",nullptr)==0&&!*state);
  for(auto bad:{"yes","on extra","1","off on"})assert(f(0,&sd,"@test",bad)==-1&&!*state);
 }
 assert(atcommand_navi(0,&sd,"@navi","prontera 25 30")==0&&hidden);
 assert(atcommand_navi(0,&sd,"@navi2","prontera 25 30")==0&&!hidden);
 for(auto bad:{"","prontera","prontera -1 5","prontera 100 5","prontera 10 10","unknown 25 30","prontera 25 30 x","prontera 1.5 30","prontera 99999999999999999 0"})assert(atcommand_navi(0,&sd,"@navi",bad)==-1);
 assert(navigations==2);
 sd.inventory.u.items_inventory[0]={100,1,1};sd.inventory.u.items_inventory[1]={101,1,1};sd.inventory.u.items_inventory[2]={102,0,0};
 sd.cant=true;assert(atcommand_unequipall(0,&sd,"@unequipall","")==-1);assert(atcommand_clearfav(0,&sd,"@clearfav","")==-1);assert(packets.empty());
 sd.cant=false;sd.dead=true;assert(atcommand_unequipall(0,&sd,"@unequipall","")==-1);sd.dead=false;
 assert(atcommand_unequipall(0,&sd,"@unequipall","")==0);assert(sd.inventory.u.items_inventory[0].equip==0&&sd.inventory.u.items_inventory[1].equip==1);
 assert(atcommand_clearfav(0,&sd,"@clearfav","")==0);assert(packets.size()==2);
 for(int i=0;i<2;i++){assert(sd.inventory.u.items_inventory[i].favorite==0);assert(packets[i].packetType==0x908&&packets[i].index==i+2&&packets[i].favorite);assert(sd.inventory.u.items_inventory[i].nameid==100+i);}
 assert(atcommand_clearfav(0,&sd,"@clearfav","")==0&&packets.size()==2);
 for(auto f:{atcommand_clearfav,atcommand_unequipall,atcommand_autospells})assert(f(0,&sd,"@test","other player")==-1);
 sd.autospell.push_back({19,5,0,75,1,4000,2});messages.clear();assert(atcommand_autospells(0,&sd,"@autospells","")==0);
 bool shown=false;for(auto& text:messages)if(text.find("7.5%")!=std::string::npos&&text.find("random 1..level")!=std::string::npos)shown=true;assert(shown);
 std::cout<<"PASS: explicit/toggle settings, navigation boundaries, restricted unequip, favorite packets and autospell rates\n";
}
'''
with tempfile.TemporaryDirectory(prefix='pn-player-commands-') as temp:
    cpp=Path(temp)/'test.cpp'; binary=Path(temp)/'test'
    cpp.write_text(prefix+code+suffix)
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=undefined','-o',str(binary),str(cpp)],check=True)
    subprocess.run([str(binary)],check=True)
