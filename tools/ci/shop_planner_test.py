#!/usr/bin/env python3
"""Compile exact pure shop planner bodies with explicit model/DB/submission doubles.
This verifies planning invariants, not SQL, network recovery or native callbacks.
"""
import pathlib, subprocess, tempfile
root=pathlib.Path(__file__).resolve().parents[2]
source=(root/'src/custom/shop_map.inc').read_text()
source=source[source.index('std::shared_ptr<pn_shop::Commit> pn_shop_request'):source.index('bool pn_shop_stock_refresh')]
prefix=r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
constexpr int MAX_INVENTORY=8,MAX_AMOUNT=30000,IT_PETEGG=7,PET_EGG=1,CARD0_PET=255;
struct __attribute__((packed)) item {uint32_t nameid=0;int32_t amount=0;uint64_t unique_id=0;uint32_t equip=0,equipSwitch=0,expire_time=0;uint8_t identify=0,refine=0,bound=0,favorite=0;uint32_t card[4]{};};
struct item_data {int type=0;bool stackable=true;uint32_t weight=1,equip=0,value_sell=10;struct{bool guid=false,autoequip=false;}flag;struct{bool inventory=false;int amount=30000;}stack;};
struct Database {std::map<uint32_t,std::shared_ptr<item_data>> rows;auto find(uint32_t n){auto it=rows.find(n);return it==rows.end()?std::shared_ptr<item_data>{}:it->second;}}item_db;
bool itemdb_isstackable2(const item_data* d){return d->stackable;}
bool pet_db_search(uint32_t n,int){return n==5;}
namespace pn_tokens {bool retired(uint32_t n){return n==6;}}
struct map_session_data {struct{uint32_t account_id=1,char_id=2,uniqueitem_counter=10,inventory_slots=8;int64_t zeny=1000;}status;int cashPoints=500,kafraPoints=400;uint32_t weight=0,max_weight=1000;struct{struct{item items_inventory[MAX_INVENTORY];}u;}inventory;struct{int pending_zeny=0,pending_slots=0,pending_weight=0;}mail;bool pending=false;};
namespace pn_shop {
struct Commit {uint16_t length=0;uint32_t kind=0,account_id=0,char_id=0,counter_before=0,counter_after=0;int64_t wallet_before=0,wallet_after=0,cash_before=0,cash_after=0,kafra_before=0,kafra_after=0;item items[MAX_INVENTORY]{};};
struct Grant{uint32_t nameid=0,amount=0;uint8_t refine=0;uint32_t price=0;};
struct Event{int16_t index=0;uint32_t amount=0,nameid=0;uint64_t unique_id=0;uint32_t equip=0,value_sell=0,price=0;};
}
bool busy=false;bool pn_shop_stock_busy(){return busy;}bool pc_transaction_pending(map_session_data* sd){return sd->pending;}
std::shared_ptr<pn_shop::Commit> submitted;std::vector<pn_shop::Event> observed;uint32_t planned_weight=0;
bool pn_shop_submit(map_session_data&,std::shared_ptr<pn_shop::Commit> r,std::vector<pn_shop::Event> e,uint32_t w){submitted=r;observed=e;planned_weight=w;return true;}
"""
suffix=r"""
int main(){
for(uint32_t i=1;i<=7;++i)item_db.rows[i]=std::make_shared<item_data>();
item_db.rows[2]->stackable=false;item_db.rows[3]->flag.guid=true;item_db.rows[4]->type=IT_PETEGG;
unsigned cases=0;
auto check=[&](map_session_data& sd,std::vector<pn_shop::Grant> grants,const uint32_t* costs,bool expected){
 const auto before=sd;submitted.reset();observed.clear();auto r=pn_shop_request(sd,1);r->wallet_after-=10;
 bool result=pn_shop_begin(sd,r,grants,costs);assert(result==expected);
 assert(!memcmp(&before,&sd,sizeof(sd)));assert(bool(submitted)==expected);++cases;
};
map_session_data sd;
check(sd,{{1,2,0,123}},nullptr,true);assert(submitted->items[0].amount==2&&planned_weight==2&&observed[0].price==123);
sd.inventory.u.items_inventory[0].nameid=1;sd.inventory.u.items_inventory[0].amount=3;sd.inventory.u.items_inventory[0].bound=1;sd.weight=3;
check(sd,{{1,2,0}},nullptr,true);assert(submitted->items[0].bound==1&&submitted->items[0].amount==3&&submitted->items[1].amount==2);
for(int mode=0;mode<4;++mode){map_session_data x; x.status.inventory_slots=1;auto& it=x.inventory.u.items_inventory[0];it.nameid=1;it.amount=3;if(mode==0)it.bound=1;if(mode==1)it.expire_time=12;if(mode==2)it.card[0]=99;if(mode==3)it.unique_id=99;x.weight=3;check(x,{{1,1,0}},nullptr,false);}
uint32_t costs[MAX_INVENTORY]{};costs[0]=3;sd.status.inventory_slots=1;
check(sd,{{1,2,0}},costs,true);assert(submitted->items[0].bound==0&&submitted->items[0].amount==2&&planned_weight==2);
sd={};sd.inventory.u.items_inventory[0].nameid=1;sd.inventory.u.items_inventory[0].amount=5;sd.inventory.u.items_inventory[0].favorite=1;sd.inventory.u.items_inventory[0].card[1]=7;sd.weight=5;costs[0]=2;
check(sd,{{2,1,4}},costs,true);assert(submitted->items[0].amount==3&&submitted->items[0].card[1]==7&&submitted->items[0].favorite==1&&submitted->items[1].refine==4);
for(int field=0;field<3;++field){auto x=sd;if(field==0)x.inventory.u.items_inventory[0].equip=1;if(field==1)x.inventory.u.items_inventory[0].equipSwitch=1;if(field==2)x.inventory.u.items_inventory[0].card[0]=CARD0_PET;check(x,{{1,1,0}},costs,false);}
sd={};check(sd,{{2,2,0},{3,2,0}},nullptr,true);assert(submitted->counter_after==14&&observed.size()==4);for(int i=0;i<4;++i)assert(submitted->items[i].unique_id==((uint64_t(2)<<32)|(10+i)));
sd.status.uniqueitem_counter=UINT32_MAX;check(sd,{{2,1,0}},nullptr,false);
sd={};for(uint32_t n:{4,5,6,99})check(sd,{{n,1,0}},nullptr,false);
for(uint32_t n:{4,5}){auto x=sd;x.inventory.u.items_inventory[0].nameid=n;x.inventory.u.items_inventory[0].amount=3;x.weight=3;check(x,{{1,1,0}},costs,false);}
check(sd,{{1,0,0}},nullptr,false);check(sd,{{1,30001,0}},nullptr,false);
sd.max_weight=1;check(sd,{{1,2,0}},nullptr,false);sd={};sd.pending=true;check(sd,{{1,1,0}},nullptr,false);sd={};busy=true;check(sd,{{1,1,0}},nullptr,false);busy=false;
item_db.rows[1]->stack.inventory=true;item_db.rows[1]->stack.amount=2;check(sd,{{1,3,0}},nullptr,false);check(sd,{{1,2,0},{1,1,0}},nullptr,false);
printf("SHOP_PLANNER cases=%u failures=0\n",cases);
}
"""
with tempfile.TemporaryDirectory(prefix='pn-shop-plan-') as td:
    cpp=pathlib.Path(td)/'test.cpp'; exe=pathlib.Path(td)/'test';cpp.write_text(prefix+source+suffix)
    subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
