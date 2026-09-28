#include <custom/mail_commit.hpp>
static void intif_mail_companion_request(const map_session_data& sd) {
    if (CheckForCharServer() || !chrif_isconnected()) return;
    pn_mail_commit request;request.request=sd.mail_companion.request;
    request.wallet_before=sd.mail_companion.wallet_before;request.fee_percent=sd.mail_companion.fee_percent;
    WFIFOHEAD(inter_fd,sizeof(request));memcpy(WFIFOP(inter_fd,0),&request,sizeof(request));WFIFOSET(inter_fd,sizeof(request));
}
static TIMER_FUNC(intif_mail_companion_retry) {
    auto* sd=map_id2sd(id);
    if (!sd || !sd->mail_companion.pending || sd->mail_companion.request.request_id!=static_cast<uint64>(data)) return 0;
    intif_mail_companion_request(*sd);add_timer(tick+1000,intif_mail_companion_retry,id,data);return 0;
}
void intif_mail_companion_save(map_session_data& sd) {
    intif_mail_companion_request(sd);
    add_timer(gettick()+1000,intif_mail_companion_retry,sd.status.account_id,static_cast<intptr_t>(sd.mail_companion.request.request_id));
}
static void intif_parse_MailCompanionCommitted(int32 fd) {
    pn_mail_ack reply;memcpy(&reply,RFIFOP(fd,0),sizeof(reply));
    auto* sd=map_id2sd(reply.account_id);
    if (!sd || !sd->mail_companion.pending || sd->status.char_id!=reply.char_id ||
        sd->bank_ui.nonce_hi!=reply.nonce_hi || sd->bank_ui.nonce_lo!=reply.nonce_lo ||
        sd->mail_companion.request.request_id!=reply.request_id ||
        (reply.result!=pn_mail::Ok && reply.result!=pn_mail::Failed)) return;
    auto& state=sd->mail_companion;
    sd->mail.pending_zeny-=state.total;state.applying=true;
    if (reply.result==pn_mail::Failed) pc_getzeny(sd,state.total,LOG_TYPE_MAIL);
    else {
        sd->cansendmail_tick=gettick()+battle_config.mail_delay;
        if (battle_config.mail_daily_count) {
            mail_refresh_remaining_amount(sd);
            auto* daily=sd->sc.getSCE(SC_DAILYSENDMAILCNT);
            if (daily) sc_start2(sd,sd,SC_DAILYSENDMAILCNT,100,date_get_dayofyear(),daily->val2+1,INFINITE_TICK);
        }
    }
    state.pending=state.applying=false;state.result=reply.result;
    chrif_save(sd,CSAVE_NORMAL|CSAVE_INVENTORY);
}
