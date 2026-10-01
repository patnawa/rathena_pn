#ifndef PN_ITEM_USE_HPP
#define PN_ITEM_USE_HPP
#include <common/mmo.hpp>
#include <functional>
#include <vector>
class map_session_data;
struct script_code;
enum PnItemScriptEffect : uint32 { PN_ITEM_PET=1, PN_ITEM_WORLD=2, PN_ITEM_DYNAMIC=4 };
uint32 script_item_use_effects(const script_code* code);
bool pn_item_use_active(const map_session_data* sd);
bool pn_item_use_defer(const map_session_data* sd,std::function<void()> effect);
bool pn_item_use_success_defer(const map_session_data* sd,std::function<void()> effect);
bool pn_item_use_achievement_defer(const map_session_data* sd,int32 group,std::vector<int32> arguments);
bool pn_item_use_ack(const map_session_data* sd,std::function<void(bool)> effect);
bool pn_item_use_save_defer(map_session_data* sd,int32 flags);
bool pn_item_use_world_allowed(map_session_data* sd);
bool pn_item_use_collect_pet(map_session_data& sd,const item& egg,uint32 amount);
void pn_item_use_settled(map_session_data& sd,bool committed);
bool pn_item_use_capture_waiting(const map_session_data* sd);
bool pn_item_use_capture_start(map_session_data& sd);
bool pn_item_use_capture_finish(map_session_data& sd,const item* egg,std::function<void(bool)> result,
    int32 achievement_group=0,std::vector<int32> achievement_arguments={});
bool pn_item_use_batch_begin(map_session_data& sd);
bool pn_item_use_batch_finish(map_session_data& sd);
void pn_item_use_abort(map_session_data& sd);
#endif
