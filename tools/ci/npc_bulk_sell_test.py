#!/usr/bin/env python3
"""Run actual NPC sale parser and transaction against explicit inventory doubles."""
from pathlib import Path
import subprocess
import tempfile
from episode_party_progression_test import scan_to

ROOT=Path(__file__).resolve().parents[2]
def function(path, signature):
    source=(ROOT/path).read_text(encoding='utf-8')
    start=source.index(signature);brace=source.index('{',start)
    return source[start:scan_to(source,brace,'{','}')+1]
sale=function('src/map/npc.cpp','uint8 npc_selllist(')
parser=function('src/map/clif.cpp','void clif_parse_NpcSellListSend(')
cpp=r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>
using int16=int16_t;using int32=int32_t;using int64=int64_t;using uint8=uint8_t;using uint16=uint16_t;using t_itemid=uint32_t;
constexpr int MAX_INVENTORY=200,NPCTYPE_SHOP=1,IT_PETEGG=2,CARD0_PET=256,PET_EGG=1,LOG_TYPE_NPC=1,MC_OVERCHARGE=1,SKILL_FLAG_REPLACED_LV_0=10,MAX_ZENY=2147483647;
constexpr int64 MAX_WALLET_ZENY=INT64_MAX;
struct PACKET_CZ_PC_SELL_ITEMLIST_sub { uint16 index,amount; } __attribute__((packed));
struct PACKET_CZ_PC_SELL_ITEMLIST { int16 packetType,packetLength;PACKET_CZ_PC_SELL_ITEMLIST_sub sellList[]; } __attribute__((packed));
struct item { int nameid=501,amount=1,expire_time=0,card[4]={};bool sellable=true; };
struct item_data { int type=0,value_sell=10; };
struct map_session_data {
    int npc_shopid=1;struct { bool trading=false; } state;
    struct { struct { item items_inventory[MAX_INVENTORY]; } u; } inventory;
    item_data* inventory_data[MAX_INVENTORY]={};
    struct { int64 zeny=0;struct { int flag=0; } skill[2]; } status;
    struct { int64 pending_zeny=0; } mail;
};
struct npc_data { int subtype=NPCTYPE_SHOP;npc_data* master_nd=nullptr; } shop;
struct { int rental_item_novalue=1,shop_exp=0; } battle_config;
int deleted=0,pet_deleted=0,last_result=-1,script_calls=0;
alignas(8) uint8 packet[4+(MAX_INVENTORY+1)*4];
void* RFIFOP(int,int) { return packet; }
void clif_npc_sell_result(map_session_data*,int result) { last_result=result; }
void* map_id2bl(int) { return &shop; }
npc_data* npc_checknear(map_session_data*,void*) { return &shop; }
bool pc_can_sell_item(map_session_data*,item* value,int) { return value->sellable; }
int pc_modifysellvalue(map_session_data*,int value) { return value; }
int npc_selllist_sub(map_session_data*,int,const PACKET_CZ_PC_SELL_ITEMLIST_sub*,npc_data*) { ++script_calls;return 0; }
bool pet_db_search(int,int) { return true; }
uint32_t MakeDWord(int lo,int hi) { return uint32_t(lo)|(uint32_t(hi)<<16); }
void intif_delete_petdata(uint32_t) { ++pet_deleted; }
int pc_delitem(map_session_data* sd,int idx,int amount,int,int,int) { sd->inventory.u.items_inventory[idx].amount-=amount;++deleted;return 0; }
int pc_getzeny(map_session_data* sd,int amount,int) { sd->status.zeny+=amount;return 0; }
int pc_getzeny64(map_session_data* sd,int64 amount,int) { sd->status.zeny+=amount;return 0; }
bool pc_transaction_locked(map_session_data*) { return false; }
int pc_checkskill(map_session_data*,int) { return 0; }
int skill_get_index(int) { return 0; }
void pc_gainexp(map_session_data*,void*,int,int,int) {}
#define nullpo_retr(result,p) if(!(p)) return result
'''
cpp+=sale+'\n'+parser+'\n'
cpp+=r'''
int main() {
    int failures=0,checks=0;
    auto check=[&](bool ok) { ++checks;if(!ok){++failures;std::printf("FAIL %d\n",checks);} };
    item_data data[MAX_INVENTORY];
    for(int scenario=0;scenario<12;++scenario) {
        map_session_data sd{};deleted=pet_deleted=script_calls=0;last_result=-1;shop.master_nd=nullptr;
        auto* p=reinterpret_cast<PACKET_CZ_PC_SELL_ITEMLIST*>(packet);
        int count=scenario==0?191:MAX_INVENTORY;
        p->packetType=0xc9;p->packetLength=4+count*4;
        for(int i=0;i<MAX_INVENTORY;++i) { sd.inventory_data[i]=&data[i];p->sellList[i]={(uint16)(i+2),1}; }
        if(scenario==2) p->sellList[count-1].index=2; // repeated slot at end
        if(scenario==3) p->sellList[count-1].amount=0;
        if(scenario==4) p->sellList[count-1].amount=2; // insufficient stack
        if(scenario==5) p->sellList[count-1].index=MAX_INVENTORY+2;
        if(scenario==6) p->packetLength=4+count*4-1; // trailing partial entry
        if(scenario==7) p->packetLength=4+(MAX_INVENTORY+1)*4;
        if(scenario==8) p->packetLength=3;
        if(scenario==9) sd.inventory.u.items_inventory[count-1].sellable=false;
        if(scenario==10) { shop.master_nd=&shop;p->sellList[count-1].index=2; }
        if(scenario==11) { sd.state.trading=true; }
        clif_parse_NpcSellListSend(0,&sd);
        check(scenario<2 ? last_result==0 && deleted==count && sd.status.zeny==count*10 : last_result==1 && deleted==0 && sd.status.zeny==0 && script_calls==0);
        check(sd.npc_shopid==0 && pet_deleted==0);
        if(scenario>=2) for(int i=0;i<MAX_INVENTORY;++i) check(sd.inventory.u.items_inventory[i].amount==1);
    }
    std::printf("NPC_BULK_SELL checks=%d failures=%d;191/200 accepted;invalid batches unchanged\n",checks,failures);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='pn-bulk-sell-') as temp:
    path,exe=Path(temp)/'test.cpp',Path(temp)/'test';path.write_text(cpp)
    subprocess.run(['g++','-std=c++17','-fsanitize=address,undefined','-fno-sanitize-recover=all',str(path),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
