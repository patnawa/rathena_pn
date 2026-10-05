#ifndef PN_PAIR_STATE_HPP
#define PN_PAIR_STATE_HPP
#include "pair_commit.hpp"
#include <memory>
class map_session_data;
struct pn_pair_rollback;
struct pn_pair_state {
    bool pending=false,applying=false;
    std::shared_ptr<pn_pair::Commit> request;
    std::shared_ptr<pn_pair_rollback> rollback;
};
bool pn_pair_begin(map_session_data& a,map_session_data& b,pn_pair::Kind kind,int64_t fee=0);
void pn_pair_submit(map_session_data& a,map_session_data& b);
bool pn_pair_abort(map_session_data& a,map_session_data& b);
#endif
