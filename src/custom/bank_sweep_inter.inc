#include <custom/bank_sweep.hpp>
static void intif_bank_sweep_request(const map_session_data& sd) {
    if (CheckForCharServer() || !chrif_isconnected()) return;
    if (!sd.registry_saves.empty()) { intif_registry_replay(&sd); return; }
    pn_bank_sweep request;request.account_id=sd.status.account_id;request.char_id=sd.status.char_id;
    request.nonce_hi=sd.bank_ui.nonce_hi;request.nonce_lo=sd.bank_ui.nonce_lo;request.request_id=sd.bank_ui.request_id;
    request.bank_before=sd.bank_ui.bank_before;
    WFIFOHEAD(inter_fd,sizeof(request));memcpy(WFIFOP(inter_fd,0),&request,sizeof(request));WFIFOSET(inter_fd,sizeof(request));
}
static TIMER_FUNC(intif_bank_sweep_retry) {
    auto* sd=map_id2sd(id);
    if (!sd || !sd->bank_ui.pending || sd->bank_ui.action!=pn_bank::CollectOffline || sd->bank_ui.request_id!=static_cast<uint64>(data)) return 0;
    intif_bank_sweep_request(*sd);add_timer(tick+1000,intif_bank_sweep_retry,id,data);return 0;
}
void intif_bank_sweep_save(map_session_data& sd) {
    intif_bank_sweep_request(sd);add_timer(gettick()+1000,intif_bank_sweep_retry,sd.status.account_id,static_cast<intptr_t>(sd.bank_ui.request_id));
}
static void intif_parse_BankSweepCommitted(int32 fd) {
    pn_bank_sweep_ack reply;memcpy(&reply,RFIFOP(fd,0),sizeof(reply));
    auto* sd=map_id2sd(reply.account_id);
    if (!sd || !sd->bank_ui.pending || sd->bank_ui.action!=pn_bank::CollectOffline ||
        sd->status.char_id!=reply.char_id || sd->bank_ui.nonce_hi!=reply.nonce_hi || sd->bank_ui.nonce_lo!=reply.nonce_lo ||
        sd->bank_ui.request_id!=reply.request_id || (reply.result!=pn_bank::Ok && reply.result!=pn_bank::SaveFailed)) return;
    if (reply.result==pn_bank::Ok) {
        if (reply.bank_after<sd->bank_ui.bank_before) return;
        sd->bank_ui.applying=true;pc_setparam(sd,SP_BANK_VAULT,reply.bank_after);sd->bank_ui.applying=false;
        sd->bank_ui.collected_characters=reply.collected;sd->bank_ui.skipped_characters=reply.skipped;
        char message[180];snprintf(message,sizeof(message),"Collected %" PRId64 " Zeny from %u offline characters; skipped %u characters.",reply.bank_after-sd->bank_ui.bank_before,reply.collected,reply.skipped);
        clif_displaymessage(sd->fd,message);
    }
    sd->bank_ui.pending=false;sd->bank_ui.result=reply.result;
    chrif_save(sd,CSAVE_NORMAL|CSAVE_INVENTORY);
}
