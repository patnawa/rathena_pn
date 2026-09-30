#include <common/timer.hpp>
#include <char/char_logif.hpp>
static void pn_global_request_barrier(const pn_shop::Commit& request) {
 if(!chlogif_isconnected() || !pn_global_point::prepare(sql_handle,request))return;
 constexpr size_t size=4+sizeof(request);static_assert(size<65536,"point barrier frame");
 WFIFOHEAD(login_fd,size);WFIFOW(login_fd,0)=pn_global_point::request_packet;WFIFOW(login_fd,2)=size;
 memcpy(WFIFOP(login_fd,4),&request,sizeof(request));WFIFOSET(login_fd,size);
}
static TIMER_FUNC(pn_global_expire_barriers) {
 if(!pn_global_point::expire(sql_handle))ShowWarning("Pending global point transactions could not be checked; admission remains fenced.\n");
 return 0;
}
static void pn_global_points_init() {
 if(!pn_global_point::transactional(sql_handle,"pn_global_point_barriers") || !pn_global_point::transactional(sql_handle,"pn_point_registry_keys")) {
  ShowFatalError("Global point adapter requires upgrade_20260929_point_assets.sql.\n");exit(EXIT_FAILURE);
 }
 pn_global_point::expire(sql_handle);
 add_timer_func_list(pn_global_expire_barriers,"pn_global_expire_barriers");
 add_timer_interval(gettick()+5000,pn_global_expire_barriers,0,0,5000);
}
