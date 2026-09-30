#include <cstdint>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using uint32=uint32_t; using int16=int16_t; using int32=int32_t; using t_itemid=uint32;
constexpr int PET_EGG=1, CARD0_PET=256, LOG_TYPE_PICKDROP_PLAYER=1;
struct item {t_itemid nameid=0;int identify=0;uint16_t card[4]{};};
struct map_session_data {struct {uint32 account_id=1,char_id=2;}status;std::vector<item> inventory;};
struct s_pet_db {int16 class_=1002; t_itemid EggID=9001; int intimate=250;};
struct s_mob_db {int16 lv=1;std::string jname="Poring";};
std::shared_ptr<s_pet_db> pet_record=std::make_shared<s_pet_db>();
std::shared_ptr<s_mob_db> mob_record=std::make_shared<s_mob_db>();
struct PetDb {std::shared_ptr<s_pet_db> find(int16 id){return pet_record && id==pet_record->class_?pet_record:nullptr;}}pet_db;
struct MobDb {std::shared_ptr<s_mob_db> find(int16){return mob_record;}}mob_db;
map_session_data player;
bool online=true, connected=true;
int capacity=2,requests=0,deleted=0,notifications=0;
std::shared_ptr<s_pet_db> pet_db_search(t_itemid id,int){return pet_record && id==pet_record->EggID?pet_record:nullptr;}
int pc_inventoryblank(map_session_data* sd){return capacity-static_cast<int>(sd->inventory.size());}
int intif_create_pet(uint32,uint32,int16,int16,t_itemid,t_itemid,int,int,int,int,const char*){if(!connected)return 0;++requests;return 1;}
map_session_data* map_id2sd(uint32 id){return online && id==player.status.account_id?&player:nullptr;}
void intif_delete_petdata(int32){++deleted;}
uint16_t GetWord(int32 v,int offset){return static_cast<uint16_t>(static_cast<uint32>(v)>>(offset*16));}
int pet_get_card3_intimacy(int){return 0;}
int pc_additem(map_session_data* sd,item* egg,int,int){if(!pc_inventoryblank(sd))return 1;sd->inventory.push_back(*egg);return 0;}
void clif_additem(map_session_data*,int,int,int){++notifications;}
// PRODUCTION FUNCTIONS
int main(int argc,char** argv){
    bool known=argc==2 && std::string(argv[1])=="--known-failures";
    int failures=0,unexpected=0;
    auto check=[&](bool value,const char* label,bool defect=false){std::cout<<(value?"PASS: ":"FAIL: ")<<label<<'\n';failures+=!value;unexpected+=(value==defect);};
    check(pet_create_egg(&player,9001) && requests==1,"valid pet request dispatches once");
    check(pet_get_egg(1,1002,42) && player.inventory.size()==1,"valid callback delivers egg");
    pet_get_egg(1,1002,42);
    check(player.inventory.size()==1 && deleted==0,"repeated callback does not duplicate or delete delivered pet",true);
    player.inventory.clear();connected=false;
    check(!pet_create_egg(&player,9001) && requests==1,"disconnected dispatch is not reported successful",true);
    connected=true;player.inventory.clear();capacity=1;
    check(pet_create_egg(&player,9001),"request accepted before capacity changes");
    player.inventory.push_back(item{});deleted=0;
    pet_get_egg(1,1002,43);
    check(deleted==0,"delivery failure retains earned pet instead of deleting its row",true);
    online=false;deleted=0;
    check(!pet_get_egg(1,1002,44) && deleted==0,"disconnected callback does not delete pet row (delivery unresolved)");
    online=true;player.inventory.clear();player.status.char_id=2;
    check(pet_create_egg(&player,9001),"request accepted for original character");
    player.status.char_id=3; // Model a new live character under the same account.
    pet_get_egg(1,1002,45);
    check(player.inventory.empty(),"late callback cannot deliver original character reward to replacement character",true);
    std::cout<<"Contract failures: "<<failures<<'\n';
    if(known){std::cout<<"KNOWN-FAILURE AUDIT ONLY: expected defects reproduced; not release acceptance\n";return unexpected?2:0;}
    return failures?1:0;
}
