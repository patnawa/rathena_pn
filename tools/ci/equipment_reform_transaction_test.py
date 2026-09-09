#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  equipment_reform_transaction_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/equipment_reform_transaction_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile the production reform handler with deterministic inventory boundaries."""
from pathlib import Path
import argparse
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--source',type=Path,default=ROOT/'src/map/clif.cpp');args=p.parse_args()
s=args.source.read_text();start=s.index('void clif_parse_item_reform_start(');end=s.index('\nvoid clif_enchantwindow_open(',start);handler=s[s.index('void clif_item_reform_open('):s.index('\nvoid clif_parse_item_reform_close(')]+s[start:end]
prefix=r'''
#include <cassert>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <iostream>
using int16=int16_t;using uint16=uint16_t;using int32=int32_t;using uint32=uint32_t;using int64=int64_t;using t_itemid=uint32;
#define PACKETVER_MAIN_NUM 20260219
constexpr int MAX_INVENTORY=8,MAX_SLOTS=4,MAX_ITEM_RDM_OPT=5,MAX_REFINE=20,LOG_TYPE_REFORM=1,SP_WEIGHT=2;
struct random_option {int id=0,value=0;};
struct item {t_itemid nameid=0;int amount=0,identify=1,equip=0,equipSwitch=0,refine=10,enchantgrade=4;int card[4]={11,12,13,14};random_option option[5];uint64_t unique_id=777;};
struct item_data {int weight=0;};
struct random_group{void apply(item&) {}};
struct s_item_reform_base {int minimumRefine=10,maximumRefine=20,requiredRandomOptions=0,refineChange=0;bool cardsAllowed=true,clearSlots=false,removeEnchantgrade=false;t_itemid resultItemId=520055;std::unordered_map<t_itemid,uint16> materials;std::shared_ptr<random_group> randomOptionGroup;};
struct s_item_reform {std::unordered_map<t_itemid,std::shared_ptr<s_item_reform_base>> base_items;};
template<class T> struct database {std::unordered_map<t_itemid,std::shared_ptr<T>> rows;std::shared_ptr<T> find(t_itemid i){auto a=rows.find(i);return a==rows.end()?nullptr:a->second;}};
database<s_item_reform> item_reform_db;database<item_data> item_db;
namespace util {template<class K,class V> V umap_find(const std::unordered_map<K,V>&m,K k){auto a=m.find(k);return a==m.end()?V{}:a->second;}}
struct map_session_data {struct {t_itemid item_reform=106250;int16 item_reform_index=1;} state;struct {struct {item items_inventory[8];}u;}inventory;item_data* inventory_data[8] = {};t_itemid itemid=106250;int16 itemindex=1;uint32 weight=0,max_weight=100000;};
constexpr int HEADER_ZC_OPEN_REFORM_UI=1,SELF=0;
struct PACKET_ZC_OPEN_REFORM_UI{int PacketType; t_itemid ITID;};
template<class T> void clif_send(T*,size_t,map_session_data*,int){}
struct PACKET_CZ_ITEM_REFORM {t_itemid ITID=106250;uint16 index=0;}packet;
#define RFIFOP(fd,offset) (&packet)
uint16 server_index(uint16 i){return i;}
int16 pc_search_inventory(map_session_data*s,t_itemid id){for(int i=0;i<8;i++)if(s->inventory.u.items_inventory[i].nameid==id&&s->inventory.u.items_inventory[i].amount>0)return i;return -1;}
int deleted=0,results=0,weight_updates=0;
int pc_delitem(map_session_data*s,int i,int amount,int,int,int){if(i<0||i>=8||!s->inventory_data[i]||amount<=0||s->inventory.u.items_inventory[i].amount<amount)return 1;s->weight-=s->inventory_data[i]->weight*amount;s->inventory.u.items_inventory[i].amount-=amount;deleted+=amount;if(!s->inventory.u.items_inventory[i].amount){s->inventory.u.items_inventory[i].nameid=0;s->inventory_data[i]=nullptr;}return 0;}
void log_pick_pc(map_session_data*,int,int,item*){}
void clif_delitem(map_session_data&,int,int,int){}
void clif_additem(map_session_data*,int,int,int){}
void clif_updatestatus(map_session_data&,int){++weight_updates;}
void clif_item_reform_result(map_session_data&s,int,int){++results;s.state.item_reform=0;}
item_data* itemdb_search(t_itemid i){return item_db.find(i).get();}
template<class T> T cap_value(T x,T lo,T hi){return std::max(lo,std::min(x,hi));}
'''
# Adapt the old two-argument opening API only for regression comparison;
# the original implementation body remains unchanged.
if 'void clif_item_reform_open( map_session_data& sd, t_itemid item ){' in handler:
    handler += '\nvoid clif_item_reform_open(map_session_data& sd,t_itemid id,int16){clif_item_reform_open(sd,id); }\n'

suffix=r'''
std::shared_ptr<s_item_reform_base> base;
map_session_data fresh(){deleted=results=weight_updates=0;packet={};item_db.rows.clear();item_reform_db.rows.clear();map_session_data s;base=std::make_shared<s_item_reform_base>();base->materials={{1001996,2},{1002007,3}};auto r=std::make_shared<s_item_reform>();r->base_items[520054]=base;item_reform_db.rows[106250]=r;item_db.rows[520055]=std::make_shared<item_data>();item_db.rows[520055]->weight=3400;
 for(int i=0;i<8;i++){auto&v=s.inventory.u.items_inventory[i];v.nameid=i==0?520054:i==1?106250:i==2?1001996:i==3?1002007:500+i;v.amount=i<2?1:10;v.option[0]={44,19};auto d=std::make_shared<item_data>();d->weight=i==0?2900:i==1?50:10;item_db.rows[v.nameid]=d;s.inventory_data[i]=d.get();s.weight+=d->weight*v.amount;}
 return s;}
void unchanged(map_session_data&s,uint32 before){assert(results==0&&deleted==0&&s.weight==before&&s.inventory.u.items_inventory[0].nameid==520054);}
int main(){int cases=0;
 {auto s=fresh();s.state.item_reform_index=4;clif_item_reform_open(s,106250,s.itemindex);assert(s.state.item_reform==106250&&s.state.item_reform_index==1);++cases;}
 {auto s=fresh();s.itemid=106250;clif_item_reform_open(s,106250,-1);assert(s.state.item_reform_index==-1);++cases;}
 {auto s=fresh();auto old=s.inventory.u.items_inventory[0];auto expected=s.weight-50-20-30+500;clif_parse_item_reform_start(0,&s);assert(results==1&&deleted==6&&s.weight==expected&&weight_updates==1);auto&v=s.inventory.u.items_inventory[0];assert(v.nameid==520055&&v.amount==1&&v.refine==old.refine&&v.enchantgrade==old.enchantgrade&&v.unique_id==old.unique_id&&v.option[0].id==44&&v.option[0].value==19);assert(std::memcmp(v.card,old.card,sizeof(v.card))==0);assert(s.inventory_data[0]==item_db.find(520055).get());clif_parse_item_reform_start(0,&s);assert(results==1&&deleted==6);++cases;}
 for(int invalid=0;invalid<15;invalid++){auto s=fresh();auto&v=s.inventory.u.items_inventory[0];switch(invalid){case 0:v.equipSwitch=1;break;case 1:v.equip=1;break;case 2:v.amount=2;break;case 3:v.identify=0;break;case 4:v.refine=9;break;case 5:v.refine=21;break;case 6:s.inventory.u.items_inventory[1].nameid=501;break;case 7:s.inventory.u.items_inventory[1].amount=0;break;case 8:s.inventory_data[1]=nullptr;break;case 9:s.inventory.u.items_inventory[2].amount=1;break;case 10:s.inventory.u.items_inventory[3].amount=2;break;case 11:s.inventory_data[2]=nullptr;break;case 12:s.max_weight=1;break;case 13:base->materials[520054]=1;break;case 14:item_db.rows.erase(520055);break;}auto old=s.weight;clif_parse_item_reform_start(0,&s);unchanged(s,old);++cases;}
 {auto s=fresh();s.itemid=501;s.itemindex=4;clif_parse_item_reform_start(0,&s);assert(results==1&&s.inventory.u.items_inventory[1].amount==0&&s.inventory.u.items_inventory[4].amount==10);++cases;}
 {auto s=fresh();s.state.item_reform_index=-1;s.itemid=0;auto old=s.weight;clif_parse_item_reform_start(0,&s);assert(results==1&&deleted==5&&s.weight==old+500-50&&s.inventory.u.items_inventory[1].amount==1);++cases;}
 {auto s=fresh();base->materials[106250]=1;auto old=s.weight;clif_parse_item_reform_start(0,&s);unchanged(s,old);++cases;}
 {auto s=fresh();base->materials[106250]=1;s.inventory.u.items_inventory[1].amount=2;s.weight+=50;clif_parse_item_reform_start(0,&s);assert(results==1&&deleted==7&&s.inventory.u.items_inventory[1].amount==0);++cases;}
 {auto s=fresh();item_db.rows[520055]->weight=100;auto old=s.weight;clif_parse_item_reform_start(0,&s);assert(results==1&&s.weight==old-2800-100);++cases;}
 {auto s=fresh();s.max_weight=s.weight+400;clif_parse_item_reform_start(0,&s);assert(results==1&&s.weight==s.max_weight);++cases;}
 {auto s=fresh();s.state.item_reform=0;auto old=s.weight;clif_parse_item_reform_start(0,&s);unchanged(s,old);++cases;}
 std::cout<<"PASS: "<<cases<<" production reform transaction cases: full inventory, missing materials/tuning, stale trigger, capacity, metadata, replay, NPC path\n";
}
'''
with tempfile.TemporaryDirectory(prefix='pn-reform-test-') as temp:
 cpp=Path(temp)/'test.cpp';binary=Path(temp)/'test';cpp.write_text(prefix+handler+suffix)
 subprocess.run(['g++','-std=c++17','-O1','-fsanitize=address,undefined','-o',str(binary),str(cpp)],check=True)
 subprocess.run([str(binary)],check=True)
