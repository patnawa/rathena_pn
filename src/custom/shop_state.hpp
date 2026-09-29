#ifndef PN_SHOP_STATE_HPP
#define PN_SHOP_STATE_HPP
#include "shop_commit.hpp"
#include <memory>
#include <vector>
class map_session_data;
namespace pn_shop {
struct Grant { uint32_t nameid=0,amount=0; uint8_t refine=0; uint32_t price=0; };
struct Event {
    int16_t index=0;
    uint32_t amount=0,nameid=0;
    uint64_t unique_id=0;
    uint32_t equip=0,value_sell=0,price=0;
};
}
struct pn_shop_state {
    bool pending=false,applying=false;
    std::shared_ptr<const pn_shop::Commit> request;
    std::vector<pn_shop::Event> events;
    uint32_t final_weight=0;
};
std::shared_ptr<pn_shop::Commit> pn_shop_request(const map_session_data& sd,uint32_t kind);
bool pn_shop_begin(map_session_data& sd,std::shared_ptr<pn_shop::Commit> request,
    const std::vector<pn_shop::Grant>& grants,const uint32_t* requiredItems=nullptr);
bool pn_shop_submit(map_session_data& sd,std::shared_ptr<pn_shop::Commit> request,
    std::vector<pn_shop::Event> events,uint32_t final_weight);
void pn_shop_committed_effects(map_session_data& sd,const pn_shop::Commit& request);
bool pn_shop_stock_busy();
bool pn_shop_stock_refresh(const pn_shop::Commit& request);
bool pn_shop_sale_refresh(const pn_shop::Commit& request);
#endif
