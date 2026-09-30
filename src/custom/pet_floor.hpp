#ifndef PN_PET_FLOOR_HPP
#define PN_PET_FLOOR_HPP
#include <custom/shop_commit.hpp>
#include <memory>
class map_session_data;
struct flooritem_data;
bool pn_pet_floor_raw(const item& egg);
bool pn_pet_floor_take(map_session_data& picker,flooritem_data& floor,bool mvp=false);
void pn_pet_floor_removed(const flooritem_data& floor);
std::shared_ptr<const pn_shop::Commit> pn_pet_floor_pending(uint32_t account,uint64_t sequence);
bool pn_pet_floor_ack(const pn_shop::Ack& reply);
#endif
