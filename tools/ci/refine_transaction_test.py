#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  refine_transaction_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/refine_transaction_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Compile the actual refine request handler with deterministic inventory/RNG doubles."""
from pathlib import Path
import subprocess
import tempfile
import yaml

ROOT = Path(__file__).resolve().parents[2]

# Follow Renewal imports and native nested override keys, including local overrides.
effective = {}
def read_database(path, active=()):
    assert path not in active, 'refine database import cycle'
    db = yaml.safe_load((ROOT/path).read_text())
    assert db['Header'] == {'Type': 'REFINE_DB', 'Version': 2}
    for group in db.get('Body') or []:
        for level in group.get('Levels') or []:
            for refine in level.get('RefineLevels') or []:
                key = (group['Group'], level['Level'], refine['Level'])
                target = effective.setdefault(key, {'Chances': {}})
                target.update({k: v for k, v in refine.items() if k != 'Chances'})
                for cost in refine.get('Chances') or []:
                    target['Chances'].setdefault(cost['Type'], {}).update(cost)
    for entry in (db.get('Footer') or {}).get('Imports') or []:
        if entry.get('Mode', 'Renewal') == 'Renewal':
            read_database(entry['Path'], (*active, path))
read_database('db/refine.yml')
categories = {(g, l) for g, l, r in effective}
assert categories == {('Armor', 1), ('Armor', 2), *(('Weapon', i) for i in range(1, 6)), ('Shadow_Armor', 1), ('Shadow_Weapon', 1)}
for group, level in categories:
    assert {r for g,l,r in effective if (g,l)==(group,level)} == set(range(1,11 if group.startswith('Shadow_') else 21))
for key, row in effective.items():
    assert 0 <= row.get('BlacksmithBlessingAmount', 0) <= 30000
    materials = [c['Material'] for c in row['Chances'].values()]
    assert len(materials) == len(set(materials)), ('ambiguous material selection', key)
    for cost in row['Chances'].values():
        assert 0 <= cost.get('Rate', 0) <= 10000
        assert 0 <= cost.get('BreakingRate', 0) <= 10000
        assert 0 <= cost.get('DowngradeAmount', 0) <= 20
        assert 0 <= cost.get('Price', 0) <= 1000000000
print(f'PASS: {len(effective)} effective refine levels in {len(categories)} categories', flush=True)

source = (ROOT / 'src/map/clif.cpp').read_text()
start = source.index('void clif_parse_refineui_refine(')
end = source.index('\nvoid clif_unequipall_reply(', start)
handler = source[start:end]
prefix = r'''
#include <cassert>
#include <algorithm>
#include <memory>
#include <unordered_map>
#include <iostream>
using int32=int; using int16=short; using uint16=unsigned short; using t_itemid=unsigned;
#define PACKETVER 20260219
constexpr int MAX_INVENTORY=10,MAX_REFINE=20,ITEMID_BLACKSMITH_BLESSING=6635,IT_WEAPON=4;
constexpr int LOG_TYPE_CONSUME=0,LOG_TYPE_OTHER=0,NOTIFYEFFECT_REFINE_SUCCESS=0,NOTIFYEFFECT_REFINE_FAILURE=1,AG_ENCHANT_SUCCESS=0,AG_ENCHANT_FAIL=1;
constexpr int ITEMREFINING_SUCCESS=0,ITEMREFINING_FAILURE=1,ITEMREFINING_DOWNGRADE=2,ITEMREFINING_FAILURE2=3;
enum class e_purchase_result { PURCHASE_FAIL_MONEY };
struct item { unsigned nameid=0; int amount=0,refine=0,identify=1,attribute=0,equip=0,equipSwitch=0; };
struct item_data { int type=IT_WEAPON,weapon_level=5; };
struct map_session_data {
 int fd=0,zeny=1000;
 struct { bool refineui_open=true,trading=false,vending=false,buyingstore=false,storage_flag=false; } state;
 struct { struct { item items_inventory[MAX_INVENTORY]; } u; } inventory;
 item_data* inventory_data[MAX_INVENTORY]{};
};
struct s_refine_cost { unsigned nameid=984; int chance=5000,zeny=100,breaking_rate=0,downgrade_amount=0; };
struct s_refine_level_info { unsigned blessing_amount=1; bool broadcast_success=false,broadcast_failure=false; std::unordered_map<int,std::shared_ptr<s_refine_cost>> costs; };
std::shared_ptr<s_refine_level_info> info;
struct { std::shared_ptr<s_refine_level_info> findLevelInfo(item_data&,item& i) { return i.refine>=MAX_REFINE?nullptr:info; } } refine_db;
struct PACKET_CZ_REQ_REFINING { uint16 index=2; unsigned itemId=984; int blacksmithBlessing=0; } packet;
#define RFIFOP(fd,offset) (&packet)
uint16 server_index(uint16 i) { return i-2; }
int roll=0,draws=0,payments=0;
int rnd() { ++draws; return roll; }
int pc_search_inventory(map_session_data* sd,unsigned id) { for(int i=0;i<MAX_INVENTORY;i++) if(sd->inventory.u.items_inventory[i].nameid==id && sd->inventory.u.items_inventory[i].amount>0) return i; return -1; }
int pc_payzeny(map_session_data* sd,int n,int) { if(sd->zeny<n)return 1;sd->zeny-=n;++payments;return 0; }
int pc_delitem(map_session_data* sd,int i,int n,int,int,int) { auto& it=sd->inventory.u.items_inventory[i]; if(it.amount<n)return 1;it.amount-=n;if(!it.amount){it.nameid=0;sd->inventory_data[i]=nullptr;}return 0; }
template<class T> T cap_value(T n,int lo,int hi) { return std::clamp(n,lo,hi); }
template<class... T> void clif_displaymessage(T...){}
template<class... T> void clif_npc_buy_result(T...){}
template<class... T> void log_pick_pc(T...){}
template<class... T> void clif_misceffect(T...){}
template<class... T> void clif_refine(T...){}
template<class... T> void clif_broadcast_refine_result(T...){}
template<class... T> void achievement_update_objective(T...){}
template<class... T> void clif_refineui_info(T...){}
'''
suffix = r'''
item_data data;
map_session_data fresh() {
 info=std::make_shared<s_refine_level_info>();info->costs[0]=std::make_shared<s_refine_cost>();
 packet={};payments=draws=0;roll=0;
 map_session_data sd;
 sd.inventory.u.items_inventory[0]={100,1,5};
 sd.inventory.u.items_inventory[1]={984,10};
 sd.inventory.u.items_inventory[2]={6635,10};
 for(int i=0;i<3;i++)sd.inventory_data[i]=&data;
 return sd;
}
void unchanged(map_session_data& sd) { assert(sd.zeny==1000 && payments==0 && draws==0);assert(sd.inventory.u.items_inventory[0].refine==5);assert(sd.inventory.u.items_inventory[1].amount==10);assert(sd.inventory.u.items_inventory[2].amount==10); }
int main(){
 // Exhaust every possible RNG result for endpoints and representative live rates.
 for(int chance: {0,1,100,500,1500,3500,5000,6000,9000,9999,10000}) {
  int successes=0;
  for(int r=0;r<10000;r++){auto sd=fresh();info->costs[0]->chance=chance;roll=r;clif_parse_refineui_refine(0,&sd);successes+=sd.inventory.u.items_inventory[0].refine==6;assert(sd.zeny==900);assert(sd.inventory.u.items_inventory[1].amount==9);}
  assert(successes==chance);
 }
 for(int guard=0;guard<13;guard++){
  auto sd=fresh();
  switch(guard){case 0:sd.state.refineui_open=false;break;case 1:packet.index=1;break;case 2:sd.inventory_data[0]=nullptr;break;case 3:sd.inventory.u.items_inventory[0].identify=0;break;case 4:sd.inventory.u.items_inventory[0].attribute=1;break;case 5:sd.inventory.u.items_inventory[0].equip=1;break;case 6:sd.inventory.u.items_inventory[0].equipSwitch=1;break;case 7:sd.state.trading=true;break;case 8:sd.state.vending=true;break;case 9:sd.state.buyingstore=true;break;case 10:sd.state.storage_flag=true;break;case 11:packet.itemId=999;break;case 12:packet.blacksmithBlessing=1;info->blessing_amount=11;break;}
  clif_parse_refineui_refine(0,&sd);unchanged(sd);
 }
 {auto sd=fresh();sd.zeny=99;clif_parse_refineui_refine(0,&sd);assert(sd.zeny==99 && payments==0 && draws==0 && sd.inventory.u.items_inventory[1].amount==10);}
 {auto sd=fresh();sd.inventory.u.items_inventory[0].refine=20;clif_parse_refineui_refine(0,&sd);assert(sd.zeny==1000&&draws==0);}
 {auto sd=fresh();info->costs[0]->chance=0;info->costs[0]->breaking_rate=10000;clif_parse_refineui_refine(0,&sd);assert(sd.inventory.u.items_inventory[0].amount==0);}
 {auto sd=fresh();info->costs[0]->chance=0;info->costs[0]->breaking_rate=10000;packet.blacksmithBlessing=1;clif_parse_refineui_refine(0,&sd);assert(sd.inventory.u.items_inventory[0].amount==1&&sd.inventory.u.items_inventory[0].refine==5&&sd.inventory.u.items_inventory[2].amount==9);}
 {auto sd=fresh();info->costs[0]->chance=0;info->costs[0]->downgrade_amount=3;clif_parse_refineui_refine(0,&sd);assert(sd.inventory.u.items_inventory[0].refine==2);}
 std::cout << "PASS: 110000 RNG outcomes, 13 pre-payment guards, funds/max-refine, break/protection/downgrade\n";
}
'''
with tempfile.TemporaryDirectory(prefix='pn-refine-test-') as temp:
    cpp=Path(temp)/'test.cpp'; binary=Path(temp)/'test'
    cpp.write_text(prefix+handler+suffix)
    subprocess.run(['g++','-std=c++17','-O1','-fsanitize=undefined','-o',str(binary),str(cpp)],check=True)
    subprocess.run([str(binary)],check=True)
