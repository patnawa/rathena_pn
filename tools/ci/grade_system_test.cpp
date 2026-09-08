// Explicit boundary fixture for unmodified production grading functions.
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>
using uint8=uint8_t;using uint16=uint16_t;using uint32=uint32_t;using uint64=uint64_t;using int16=int16_t;using int32=int32_t;using t_itemid=uint32;
#define PACKETVER_MAIN_NUM 20260219
#define PACKETVER_RE_NUM 0
#define MAX_INVENTORY 10
#define MAX_REFINE 20
#define MAX_ENCHANTGRADE 4
#define MAX_AMOUNT 30000
#define MAX_ZENY INT32_MAX
#define min(a,b) ((a)<(b)?(a):(b))
#define nullpo_retv(x) if(!(x))return
template<class T> T cap_value(T v,T low,T high){return std::clamp(v,low,high);}
enum {IT_WEAPON=4,IT_ARMOR=5,LOG_TYPE_ENCHANTGRADE};
enum e_enchantgrade {ENCHANTGRADE_NONE,ENCHANTGRADE_D,ENCHANTGRADE_C,ENCHANTGRADE_B,ENCHANTGRADE_A};
enum e_enchantgrade_result {ENCHANTGRADE_UPGRADE_SUCCESS,ENCHANTGRADE_UPGRADE_FAILED,ENCHANTGRADE_UPGRADE_DOWNGRADE,ENCHANTGRADE_UPGRADE_BREAK};
struct item {uint32 nameid=0;int amount=0;uint8 refine=0,enchantgrade=0,identify=1,attribute=0;uint32 equip=0,equipSwitch=0;uint64 unique_id=12345;std::array<int,4> card{{4001,4002,0,0}};};
struct item_data {uint32 nameid=0;int type=IT_WEAPON;uint16 weapon_level=5,armor_level=2;struct {bool gradable=true;}flag;};
struct map_session_data {
    struct {bool enchantgrade_open=true,trading=false,vending=false,buyingstore=false,storage_flag=false;}state;
    struct {int zeny=1000000;}status;
    struct {struct {item items_inventory[MAX_INVENTORY];}u;}inventory;
    item_data* inventory_data[MAX_INVENTORY]{};int fd=1;
};
struct s_enchantgradeoption {uint16 id=0;t_itemid item=100;uint16 amount=5;uint32 zeny=1000;uint16 breaking_rate=0,downgrade_amount=0;};
struct s_enchantgradelevel {
    e_enchantgrade grade=ENCHANTGRADE_NONE;uint16 chances[MAX_REFINE+1]{};bool announceSuccess=true,announceFail=true;
    struct {t_itemid item=101;uint16 amountPerStep=1,maximumSteps=10,chanceIncrease=100;}catalyst;
    std::map<uint16,std::shared_ptr<s_enchantgradeoption>> options;
};
struct s_enchantgrade {std::map<uint16,std::map<e_enchantgrade,std::shared_ptr<s_enchantgradelevel>>>levels;};
namespace util {template<class K,class V> V map_find(const std::map<K,V>& data,K key){auto it=data.find(key);return it==data.end()?V{}:it->second;}}
struct grade_db {std::map<int,std::shared_ptr<s_enchantgrade>>rows;std::shared_ptr<s_enchantgrade>find(int t){return util::map_find(rows,t);}}enchantgrade_db;
struct PACKET_CZ_GRADE_ENCHANT_REQUEST {uint16 index=2,material_index=0,blessing_amount=0;bool blessing_flag=false;}packet;
struct PACKET_CZ_GRADE_ENCHANT_SELECT_EQUIPMENT {uint16 index;};
#define RFIFOP(fd,offset) ((void)(fd),reinterpret_cast<void*>(&packet))
uint16 server_index(uint16 n){return n-2;}
std::vector<std::pair<int,int>>debits;std::vector<int>results,logs;std::vector<std::string>messages;std::vector<uint32>rolls;
int rng=0,payments=0,selections=0;
void clif_displaymessage(int,const char* s){messages.emplace_back(s);}
void clif_enchantgrade_add(map_session_data&,uint16=UINT16_MAX,std::shared_ptr<s_enchantgradelevel> =nullptr){++selections;}
int16 pc_search_inventory(map_session_data* sd,t_itemid id){for(int i=0;i<MAX_INVENTORY;++i)if(sd->inventory.u.items_inventory[i].nameid==id)return i;return -1;}
int pc_delitem(map_session_data* sd,int slot,int amount,int,int,int){
    auto& i=sd->inventory.u.items_inventory[slot];assert(sd->inventory_data[slot] && amount>0 && i.amount>=amount);
    debits.emplace_back(slot,amount);i.amount-=amount;if(!i.amount){i.nameid=0;sd->inventory_data[slot]=nullptr;}return 0;
}
int pc_payzeny(map_session_data* sd,int amount,int){assert(amount>=0 && sd->status.zeny>=amount);++payments;sd->status.zeny-=amount;return 0;}
uint32 rnd(){assert(rng<(int)rolls.size());return rolls[rng++];}
void log_pick_pc(map_session_data*,int,int amount,const item*){logs.push_back(amount);}
void clif_enchantgrade_result(map_session_data&,uint16,e_enchantgrade_result r){results.push_back(r);}
void clif_enchantgrade_announce(map_session_data&,item&,bool){}
// PRODUCTION_HANDLERS
map_session_data sd;item_data target_data,material_data,catalyst_data;
std::shared_ptr<s_enchantgradelevel> grade;std::shared_ptr<s_enchantgradeoption>option;
void reset(){
    sd=map_session_data{};packet={};debits.clear();results.clear();logs.clear();messages.clear();rolls={0};rng=payments=selections=0;
    target_data={};target_data.nameid=500001;material_data={};material_data.nameid=100;catalyst_data={};catalyst_data.nameid=101;
    for(int n=0;n<3;++n){auto& i=sd.inventory.u.items_inventory[n];i.nameid=n==0?500001:n==1?100:101;i.amount=n==0?1:100;sd.inventory_data[n]=n==0?&target_data:n==1?&material_data:&catalyst_data;}
    sd.inventory.u.items_inventory[0].refine=11;
    grade=std::make_shared<s_enchantgradelevel>();grade->chances[11]=7000;option=std::make_shared<s_enchantgradeoption>();grade->options[0]=option;
    auto type=std::make_shared<s_enchantgrade>();type->levels[5][ENCHANTGRADE_NONE]=grade;enchantgrade_db.rows.clear();enchantgrade_db.rows[IT_WEAPON]=type;
}
void request(){clif_parse_enchantgrade_start(1,&sd);}
void unchanged(){assert(debits.empty() && results.empty() && logs.empty() && payments==0 && rng==0 && sd.status.zeny==1000000);}
int main(){
    reset();clif_parse_enchantgrade_add(1,&sd);assert(selections==1);
    for(int failure=0;failure<14;++failure){
        reset();auto& item=sd.inventory.u.items_inventory[0];
        switch(failure){
            case 0:target_data.flag.gradable=false;break;case 1:item.refine=255;break;case 2:item.enchantgrade=4;break;
            case 3:item.equip=1;break;case 4:item.equipSwitch=1;break;case 5:item.nameid=0;break;case 6:item.amount=0;break;
            case 7:item.identify=0;break;case 8:item.attribute=1;break;case 9:sd.state.trading=true;break;case 10:sd.state.vending=true;break;
            case 11:sd.state.buyingstore=true;break;case 12:sd.state.storage_flag=true;break;case 13:sd.state.enchantgrade_open=false;break;
        }
        clif_parse_enchantgrade_add(1,&sd);assert(selections==0);request();unchanged();
    }
    reset();packet.index=0;request();unchanged();
    reset();packet.material_index=99;request();unchanged();
    reset();target_data.nameid=123;request();unchanged();
    reset();option->zeny=UINT32_MAX;request();unchanged();
    reset();sd.inventory_data[1]=nullptr;request();unchanged();
    reset();option->item=500001;option->amount=1;request();unchanged();
    reset();packet.blessing_flag=true;packet.blessing_amount=10;sd.inventory.u.items_inventory[2].amount=9;request();unchanged();
    reset();packet.blessing_flag=true;packet.blessing_amount=65535;grade->catalyst.maximumSteps=65535;grade->catalyst.amountPerStep=65535;request();unchanged();
    reset();packet.blessing_flag=true;packet.blessing_amount=10;grade->catalyst.item=100;grade->catalyst.amountPerStep=3000;request();unchanged();
    reset();packet.blessing_flag=true;packet.blessing_amount=10;grade->catalyst.amountPerStep=0;request();unchanged();
    reset();request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_SUCCESS} && sd.inventory.u.items_inventory[0].refine==0 && sd.inventory.u.items_inventory[0].enchantgrade==1);
    assert((debits==std::vector<std::pair<int,int>>({{1,5}}) && payments==1 && sd.status.zeny==999000 && logs==std::vector<int>({-1,1})));
    assert(sd.inventory.u.items_inventory[0].card[0]==4001 && sd.inventory.u.items_inventory[0].unique_id==12345);
    reset();rolls={7000};request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_FAILED} && sd.inventory.u.items_inventory[0].refine==11);
    reset();rolls={6999};request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_SUCCESS});
    reset();packet.blessing_flag=true;packet.blessing_amount=10;grade->catalyst.item=100;rolls={7999};request();assert((debits==std::vector<std::pair<int,int>>({{1,15}}) && results==std::vector<int>{ENCHANTGRADE_UPGRADE_SUCCESS}));
    reset();packet.blessing_flag=true;packet.blessing_amount=10;grade->catalyst.chanceIncrease=10000;rolls={9999};request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_SUCCESS});
    reset();rolls={9999,0};option->breaking_rate=10000;request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_BREAK} && !sd.inventory.u.items_inventory[0].nameid);
    reset();rolls={9999};option->downgrade_amount=2;request();assert(results==std::vector<int>{ENCHANTGRADE_UPGRADE_DOWNGRADE} && sd.inventory.u.items_inventory[0].refine==9);
    std::cout<<"GRADE_HANDLER_OK: actual selection/commit handlers, guards, costs, overflow, RNG boundaries, success/failure/break/downgrade\n";
}
