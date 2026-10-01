// Included after bank_ui.inc. PZL1 owns no player pointer or game socket.
#include <custom/mail_protocol.hpp>
#include <custom/zeny_arithmetic.hpp>
static int32 clif_parse_mail_companion_session(int32 fd);
static pn_mail::Reply pn_mail_snapshot(map_session_data& sd,const pn_mail::Request& request) {
    pn_mail::Reply reply;
    reply.nonce_hi=sd.bank_ui.nonce_hi;reply.nonce_lo=sd.bank_ui.nonce_lo;
    reply.request_id=request.request_id;reply.char_id=sd.status.char_id;reply.wallet=sd.status.zeny;
    const auto& state=sd.mail_companion;
    reply.result=state.result;reply.pending=state.pending;reply.send_sequence=state.request.request_id;
    reply.amount=state.request.amount;reply.total=state.total;reply.fee=state.total-state.request.amount;
    reply.fee_percent=state.fee_percent;
    return reply;
}
static pn_mail::Reply pn_mail_action(map_session_data& sd,const pn_mail::Request& request) {
    using namespace pn_mail;
    auto reply=pn_mail_snapshot(sd,request);
    if (request.action==Status) return reply;
    auto& state=sd.mail_companion;
    if (request.action==Send && request.request_id<=state.request.request_id) {
        if (!request.request_id || request.request_id!=state.request.request_id ||
            memcmp(&request,&state.request,sizeof(request))) reply.result=Stale;
        return reply;
    }
    if (pc_transaction_pending(&sd) || pc_cant_act(&sd) || pc_isdead(&sd) || sd.state.mail_writing ||
        sd.mail.pending_zeny || sd.state.autotrade || sd.state.warping || sd.state.changemap ||
        !chrif_isconnected() || mail_invalid_operation(&sd)) {reply.result=Busy;return reply;}
    if (!pc_can_give_items(&sd)) {reply.result=Denied;return reply;}
    if (request.amount<=0 || !request.recipient[0] || !request.title[0] ||
        request.recipient[23] || request.title[39] || request.body[499]) {reply.result=Invalid;return reply;}
    std::int64_t total;
    if (!pn_zeny::fee_total(request.amount,battle_config.mail_zeny_fee,0,total)) {reply.result=Invalid;return reply;}
    reply.amount=request.amount;reply.total=total;reply.fee=total-request.amount;
    reply.fee_percent=battle_config.mail_zeny_fee;reply.result=Ok;
    if (total>sd.status.zeny) {reply.result=Funds;return reply;}
    if (request.action==Quote) return reply;
    if (!request.request_id || request.expected_total!=total) {reply.result=Stale;return reply;}
    if (DIFF_TICK(sd.cansendmail_tick,gettick())>0) {reply.result=Busy;return reply;}
    if (battle_config.mail_daily_count) {
        mail_refresh_remaining_amount(&sd);
        auto* daily=sd.sc.getSCE(SC_DAILYSENDMAILCNT);
        if (!daily || daily->val2>=battle_config.mail_daily_count) {reply.result=Denied;return reply;}
    }
    state.request=request;state.total=total;state.wallet_before=sd.status.zeny;
    state.fee_percent=battle_config.mail_zeny_fee;state.pending=state.applying=true;state.result=Pending;
    // Debit while locked. The immutable SQL request persists this wallet and mail
    // together; positive rewards meanwhile are saved after acknowledgement.
    if (pc_payzeny(&sd,total,LOG_TYPE_MAIL)) {
        state.pending=state.applying=false;state.result=Failed;
    } else {
        sd.mail.pending_zeny+=total; // Keep enough room for a rejected-send refund.
        state.applying=false;intif_mail_companion_save(sd);
    }
    return pn_mail_snapshot(sd,request);
}
static bool clif_parse_mail_companion(int32 fd) {
    if (RFIFOW(fd,0)!=(pn_mail::magic&0xffff)) return false;
    session[fd]->func_parse=clif_parse_mail_companion_session;
    if (RFIFOREST(fd)<8) return true;
    if (RFIFOL(fd,0)!=pn_mail::magic || RFIFOW(fd,4)!=pn_mail::version || RFIFOW(fd,6)!=sizeof(pn_mail::Request)) {set_eof(fd);return true;}
    if (RFIFOREST(fd)<sizeof(pn_mail::Request)) return true;
    pn_mail::Request request;memcpy(&request,RFIFOP(fd,0),sizeof(request));RFIFOSKIP(fd,sizeof(request));
    if (request.reserved || request.action>pn_mail::Send) {set_eof(fd);return true;}
    pn_mail::Reply reply;reply.request_id=request.request_id;
    auto* sd=map_id2sd(request.account_id);
    if (sd && sd->state.active && sd->prev && sd->status.char_id==request.char_id &&
        sd->login_id1==request.login_id1 && sd->login_id2==request.login_id2 &&
        session_isValid(sd->fd) && !session[sd->fd]->flag.eof &&
        session[fd]->client_addr==session[sd->fd]->client_addr && (request.nonce_hi || request.nonce_lo) &&
        request.nonce_hi==sd->bank_ui.nonce_hi && request.nonce_lo==sd->bank_ui.nonce_lo)
        reply=pn_mail_action(*sd,request);
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));return true;
}
static int32 clif_parse_mail_companion_session(int32 fd) {
    if (session[fd]->flag.eof) {do_close(fd);return 0;}
    if (RFIFOREST(fd)<2) return 0;
    if (!clif_parse_mail_companion(fd)) set_eof(fd);
    return 0;
}
