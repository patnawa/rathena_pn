#!/usr/bin/env python3
"""Compile exact pure shop planner bodies with explicit model/DB/submission doubles.
This verifies planning invariants, not SQL, network recovery or native callbacks.
"""
import pathlib, subprocess, tempfile
from achievement_persistence_test import function
root=pathlib.Path(__file__).resolve().parents[2]
source=(root/'src/custom/shop_map.inc').read_text()
recovery=source[source.index('bool pn_pet_recover('):source.index('static TIMER_FUNC(pn_pet_login_recovery)')]
source='\n'.join(function(root/'src/custom/shop_map.inc',signature) for signature in (
    'std::shared_ptr<pn_shop::Commit> pn_shop_request(',
    'bool pn_shop_plan_inventory(',
    'bool pn_shop_begin('))
prefix=r"""
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cerrno>
#include <string>
#include <cstring>
#include <map>
#include <memory>
#include <vector>
#include <common/runtime_metrics.hpp>
constexpr int MAIL_MAX_ITEM=5,MAX_INVENTORY=8,MAX_AMOUNT=30000,IT_PETEGG=7,PET_EGG=1,CARD0_PET=255;
struct __attribute__((packed)) item {uint32_t id=0,nameid=0;int32_t amount=0;uint64_t unique_id=0;uint32_t equip=0,equipSwitch=0,expire_time=0;uint8_t identify=0,refine=0,bound=0,favorite=0,attribute=0,enchantgrade=0;uint32_t card[4]{},option[5]{};};
struct item_data {int type=0;bool stackable=true;uint32_t weight=1,equip=0,value_sell=10;struct{bool guid=false,autoequip=false;}flag;struct{bool inventory=false;int amount=30000;}stack;};
struct Database {std::map<uint32_t,std::shared_ptr<item_data>> rows;auto find(uint32_t n){auto it=rows.find(n);return it==rows.end()?std::shared_ptr<item_data>{}:it->second;}}item_db;
bool itemdb_isstackable2(const item_data* d){return d->stackable;}
bool pet_db_search(uint32_t n,int){return n==5;}
namespace pn_tokens {bool retired(uint32_t n){return n==6;}}
struct map_session_data {struct{uint32_t account_id=1,char_id=2,uniqueitem_counter=10,inventory_slots=8;int64_t zeny=1000;}status;int cashPoints=500,kafraPoints=400;uint32_t weight=0,max_weight=1000;struct{struct{item items_inventory[MAX_INVENTORY];}u;}inventory;struct{int pending_zeny=0,pending_slots=0,pending_weight=0;}mail;bool pending=false,vars_ok=true;struct{bool pc_loaded=true;}state;};
namespace pn_pet {struct Output{item egg{};};}
bool pn_pet_describe_egg(const item& egg,pn_pet::Output& output){if(egg.nameid!=5 || egg.card[0])return false;output.egg=egg;return true;}
namespace pn_shop {
constexpr uint32_t Asset=4,PetClaim=5,ItemUse=6,MailSend=7,AuctionRegister=8,AuctionBid=9;
bool auction_kind(uint32_t kind){return kind==AuctionRegister || kind==AuctionBid;}
struct PetRetirement{uint32_t pet_id=0,egg_id=0;};
struct PetChange{uint64_t claim_id=0;int16_t inventory_index=-1;pn_pet::Output output{};};
struct Commit {uint16_t length=0;uint32_t kind=0,account_id=0,char_id=0,counter_before=0,counter_after=0;int64_t wallet_before=0,wallet_after=0,cash_before=0,cash_after=0,kafra_before=0,kafra_after=0;item items[MAX_INVENTORY]{};uint16_t pet_count=0,pet_retire_count=0;PetChange pets[MAX_INVENTORY]{};PetRetirement retired_pets[MAX_INVENTORY]{};uint32_t mail_id=0;int64_t mail_zeny=0;item mail_items[MAIL_MAX_ITEM]{};};
struct Grant{uint32_t nameid=0,amount=0;uint8_t refine=0;uint32_t price=0;item prototype{};uint64_t pet_claim_id=0;};
struct Event{int16_t index=0;uint32_t amount=0,nameid=0;uint64_t unique_id=0;uint32_t equip=0,value_sell=0,price=0;};
}
using uint64=uint64_t;
bool pn_shop_begin(map_session_data&,std::shared_ptr<pn_shop::Commit>,const std::vector<pn_shop::Grant>&,const uint32_t* = nullptr);
constexpr int SQL_SUCCESS=0,SQL_NO_DATA=100,SQL_ERROR=-1;
int mmysql_handle=0,cursor=-1,queries=0;uint32_t queried_account=0,queried_character=0;
std::vector<std::pair<std::string,item>> sql_rows;bool truncated_blob=false;
int Sql_Query(int,const char* query,...){assert(std::string(query).find("WHERE account_id=%u AND char_id=%u AND claimed=0")!=std::string::npos);va_list args;va_start(args,query);queried_account=va_arg(args,unsigned);queried_character=va_arg(args,unsigned);va_end(args);cursor=-1;++queries;return SQL_SUCCESS;}
int Sql_NextRow(int){return ++cursor<int(sql_rows.size())?SQL_SUCCESS:SQL_NO_DATA;}
int Sql_GetData(int,int column,char** data,size_t* size){auto& row=sql_rows.at(cursor);if(column==0)*data=row.first.data();else{*data=reinterpret_cast<char*>(&row.second);if(size)*size=sizeof(item)-(truncated_blob?1:0);}return SQL_SUCCESS;}
void Sql_FreeResult(int){}
bool busy=false;bool pn_shop_queue_full(){return busy;}bool pc_transaction_pending(map_session_data* sd){return sd->pending;}
std::shared_ptr<pn_shop::Commit> submitted;std::vector<pn_shop::Event> observed;uint32_t planned_weight=0;
bool pn_shop_submit(map_session_data&,std::shared_ptr<pn_shop::Commit> r,std::vector<pn_shop::Event> e,uint32_t w){submitted=r;observed=e;planned_weight=w;return true;}
"""
suffix=r"""
int main(){
for(uint32_t i=1;i<=7;++i)item_db.rows[i]=std::make_shared<item_data>();
item_db.rows[2]->stackable=false;item_db.rows[3]->flag.guid=true;item_db.rows[4]->type=IT_PETEGG;
unsigned cases=0;
for(uint32_t kind=1;kind<=pn_shop::ItemUse;++kind){
 map_session_data x;auto& kept=x.inventory.u.items_inventory[0];kept.nameid=1;kept.amount=3;kept.bound=1;kept.card[1]=7;x.weight=3;
 const auto before=x;uint32_t cost[MAX_INVENTORY]{};cost[0]=1;
 auto request=pn_shop_request(x,kind);std::vector<pn_shop::Event> events;uint32_t weight=0;
 const bool planned=pn_shop_plan_inventory(x,*request,{},cost,events,weight);
 assert(planned==(kind==pn_shop::ItemUse));assert(!memcmp(&before,&x,sizeof(x)));
 if(planned){assert(request->items[0].amount==2&&request->items[0].bound==1&&request->items[0].card[1]==7&&weight==2&&events.empty());}
 ++cases;
}
auto check=[&](map_session_data& sd,std::vector<pn_shop::Grant> grants,const uint32_t* costs,bool expected){
 const auto before=sd;submitted.reset();observed.clear();auto r=pn_shop_request(sd,1);r->wallet_after-=10;
 bool result=pn_shop_begin(sd,r,grants,costs);assert(result==expected);
 assert(!memcmp(&before,&sd,sizeof(sd)));assert(bool(submitted)==expected);++cases;
};
map_session_data sd;
check(sd,{{1,2,0,123}},nullptr,true);assert(submitted->items[0].amount==2&&planned_weight==2&&observed[0].price==123);
sd.inventory.u.items_inventory[0].nameid=1;sd.inventory.u.items_inventory[0].amount=3;sd.inventory.u.items_inventory[0].bound=1;sd.weight=3;
check(sd,{{1,2,0}},nullptr,true);assert(submitted->items[0].bound==1&&submitted->items[0].amount==3&&submitted->items[1].amount==2);
for(int mode=0;mode<8;++mode){map_session_data x; x.status.inventory_slots=1;auto& it=x.inventory.u.items_inventory[0];it.nameid=1;it.amount=3;it.identify=1;if(mode==0)it.bound=1;if(mode==1)it.expire_time=12;if(mode==2)it.card[0]=99;if(mode==3)it.unique_id=99;if(mode==4)it.refine=1;if(mode==5)it.attribute=1;if(mode==6)it.enchantgrade=1;if(mode==7)it.option[0]=1;x.weight=3;check(x,{{1,1,0}},nullptr,false);}
uint32_t costs[MAX_INVENTORY]{};costs[0]=3;sd.status.inventory_slots=1;
check(sd,{{1,2,0}},costs,true);assert(submitted->items[0].bound==0&&submitted->items[0].amount==2&&planned_weight==2);
sd={};sd.inventory.u.items_inventory[0].nameid=1;sd.inventory.u.items_inventory[0].amount=5;sd.inventory.u.items_inventory[0].favorite=1;sd.inventory.u.items_inventory[0].card[1]=7;sd.weight=5;costs[0]=2;
check(sd,{{2,1,4}},costs,true);assert(submitted->items[0].amount==3&&submitted->items[0].card[1]==7&&submitted->items[0].favorite==1&&submitted->items[1].refine==4);
for(int field=0;field<3;++field){auto x=sd;if(field==0)x.inventory.u.items_inventory[0].equip=1;if(field==1)x.inventory.u.items_inventory[0].equipSwitch=1;if(field==2)x.inventory.u.items_inventory[0].card[0]=CARD0_PET;check(x,{{1,1,0}},costs,false);}
sd={};check(sd,{{2,2,0},{3,2,0}},nullptr,true);assert(submitted->counter_after==13&&observed.size()==3&&submitted->items[2].amount==2);for(int i=0;i<3;++i)assert(submitted->items[i].unique_id==((uint64_t(2)<<32)|(10+i)));
{map_session_data rental;pn_shop::Grant grant{1,2};grant.prototype.nameid=1;grant.prototype.identify=1;grant.prototype.expire_time=123;
 check(rental,{grant},nullptr,true);assert(submitted->items[0].amount==1&&submitted->items[1].amount==1&&submitted->items[0].unique_id!=submitted->items[1].unique_id);}
sd.status.uniqueitem_counter=UINT32_MAX;check(sd,{{2,1,0}},nullptr,false);
sd={};for(uint32_t n:{4,6,99})check(sd,{{n,1,0}},nullptr,false);
check(sd,{{5,2,0}},nullptr,true);assert(submitted->pet_count==2&&observed.size()==2&&observed[0].index==-1&&planned_weight==0&&submitted->counter_after==12);
{auto x=sd;x.status.inventory_slots=1;x.inventory.u.items_inventory[0].nameid=1;x.inventory.u.items_inventory[0].amount=1;x.weight=1;
 check(x,{{5,1,0}},nullptr,true);assert(submitted->pet_count==1&&submitted->items[0].nameid==1&&planned_weight==1);}
{pn_shop::Grant grant{5,1};grant.prototype.nameid=5;grant.prototype.amount=1;grant.prototype.bound=2;grant.prototype.card[0]=CARD0_PET;grant.prototype.card[1]=42;grant.prototype.unique_id=123;grant.pet_claim_id=7;
 auto r=pn_shop_request(sd,pn_shop::PetClaim);std::vector<pn_shop::Event> e;uint32_t w=0;
 assert(pn_shop_plan_inventory(sd,*r,{grant},nullptr,e,w));assert(r->counter_after==10&&r->items[0].bound==2&&r->items[0].unique_id==123&&r->pet_count==1&&r->pets[0].claim_id==7&&w==1);++cases;}
for(uint32_t n:{4,5}){auto x=sd;x.inventory.u.items_inventory[0].nameid=n;x.inventory.u.items_inventory[0].amount=3;x.weight=3;check(x,{{1,1,0}},costs,true);}
{auto x=sd;x.status.inventory_slots=1;auto& it=x.inventory.u.items_inventory[0];it.nameid=5;it.amount=1;it.card[0]=CARD0_PET;it.card[1]=42;x.weight=1;uint32_t petcost[MAX_INVENTORY]{};petcost[0]=1;
 check(x,{{1,1,0}},petcost,true);assert(submitted->pet_retire_count==1&&submitted->retired_pets[0].pet_id==42&&submitted->retired_pets[0].egg_id==5&&submitted->items[0].nameid==1);}
check(sd,{{1,0,0}},nullptr,false);check(sd,{{1,30001,0}},nullptr,false);
sd.max_weight=1;check(sd,{{1,2,0}},nullptr,false);sd={};sd.pending=true;check(sd,{{1,1,0}},nullptr,false);sd={};busy=true;check(sd,{{1,1,0}},nullptr,false);busy=false;
item_db.rows[1]->stack.inventory=true;item_db.rows[1]->stack.amount=2;check(sd,{{1,3,0}},nullptr,false);check(sd,{{1,2,0},{1,1,0}},nullptr,false);
sd={};sd.status.inventory_slots=1;sd.max_weight=1;submitted.reset();
item egg{};egg.nameid=5;egg.amount=1;egg.bound=2;egg.card[0]=CARD0_PET;egg.card[1]=42;egg.unique_id=123;
sql_rows={{"7",egg},{"8",egg}};sql_rows[1].second.card[1]=43;sql_rows[1].second.unique_id=124;
assert(pn_pet_recover(sd));assert(queried_account==1&&queried_character==2&&submitted->kind==pn_shop::PetClaim&&submitted->pet_count==1&&submitted->pets[0].claim_id==7&&submitted->items[0].unique_id==123);++cases;
submitted.reset();sd.inventory.u.items_inventory[0].nameid=1;sd.inventory.u.items_inventory[0].amount=1;sd.weight=1;
assert(!pn_pet_recover(sd)&&!submitted&&sql_rows.size()==2);++cases;
sd={};int before_queries=queries;sd.vars_ok=false;assert(!pn_pet_recover(sd)&&queries==before_queries);sd.vars_ok=true;sd.state.pc_loaded=false;assert(!pn_pet_recover(sd)&&queries==before_queries);sd.state.pc_loaded=true;busy=true;assert(!pn_pet_recover(sd)&&queries==before_queries);busy=false;++cases;
truncated_blob=true;assert(!pn_pet_recover(sd));truncated_blob=false;sql_rows[0].first="invalid";assert(!pn_pet_recover(sd));++cases;
{map_session_data x;pn_shop::Grant grant{5,1};grant.prototype=egg;
 auto r=pn_shop_request(x,4);r->mail_id=701;r->mail_items[0]=egg;std::vector<pn_shop::Event> e;uint32_t w=0;
 assert(pn_shop_plan_inventory(x,*r,{grant},nullptr,e,w));assert(!r->pet_count&&r->items[0].unique_id==egg.unique_id&&r->counter_after==x.status.uniqueitem_counter);++cases;
 grant.prototype.card[1]++;assert(!pn_shop_plan_inventory(x,*r,{grant},nullptr,e,w));++cases;
}
printf("SHOP_PLANNER cases=%u failures=0\n",cases);
}
"""
with tempfile.TemporaryDirectory(prefix='pn-shop-plan-') as td:
    cpp=pathlib.Path(td)/'test.cpp'; exe=pathlib.Path(td)/'test';cpp.write_text(prefix+source+recovery+suffix)
    subprocess.run(['g++','-std=c++17','-O1','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I'+str(root/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
