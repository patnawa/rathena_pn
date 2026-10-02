// Included by map/clif.cpp. The companion socket never owns a player session.
#include <custom/bank_protocol.hpp>

static pn_bank::Reply pn_bank_snapshot(map_session_data& sd) {
    if (!sd.bank_ui.nonce_hi && !sd.bank_ui.nonce_lo) {
        sd.bank_ui.nonce_hi = (static_cast<uint64>(rnd()) << 32) | static_cast<uint32>(rnd());
        sd.bank_ui.nonce_lo = (static_cast<uint64>(rnd()) << 32) | static_cast<uint32>(rnd());
        if (!sd.bank_ui.nonce_hi && !sd.bank_ui.nonce_lo) sd.bank_ui.nonce_lo = 1;
    }
    pn_bank::Reply result;
    result.char_id = sd.status.char_id;
    result.nonce_hi = sd.bank_ui.nonce_hi; result.nonce_lo = sd.bank_ui.nonce_lo;
    result.char_id = sd.status.char_id; result.request_id = sd.bank_ui.request_id;
    result.bank = sd.bank_vault; result.wallet = sd.status.zeny;
    // Read-only refreshes also feed the wallet HUD. Pending balances may be
    // staged by bank, paired trade/market, mail, or storage transactions; that
    // status must win over a map's banking restriction or a char disconnect.
    const bool saving = pc_transaction_pending(&sd);
    result.result = saving ? pn_bank::Saving : sd.bank_ui.result;
    if (!saving && (!battle_config.feature_banking || map_getmapflag(sd.m, MF_NOBANK) || !chrif_isconnected()))
        result.result = pn_bank::Unavailable;
    if (result.bank < 0 || result.wallet < 0 || result.wallet > pn_bank::wallet_limit) {
        result.result = pn_bank::Unavailable; return result;
    }
    result.max_deposit = std::min(result.wallet, pn_bank::bank_limit - result.bank);
    result.max_withdraw = std::min(result.bank, pn_bank::wallet_limit - result.wallet);
    auto* other = map_id2sd(sd.trade_partner.id);
    if (sd.state.trading && other && other->state.trading && other->trade_partner.id == sd.status.account_id &&
        sd.bank_ui.trade_id && sd.bank_ui.trade_id == other->bank_ui.trade_id) {
        result.partner_account_id = other->status.account_id;
        result.partner_char_id = other->status.char_id;
        safestrncpy(result.partner_name,other->status.name,sizeof(result.partner_name));
        result.own_offer = sd.deal.zeny;result.partner_offer = other->deal.zeny;
        result.own_trade_state = sd.state.deal_locked;result.partner_trade_state = other->state.deal_locked;
        result.trade_id = sd.bank_ui.trade_id;result.trade_revision = sd.bank_ui.trade_revision;
        result.max_deposit = result.max_withdraw = 0;
    }

    // Reuse legacy item counts for the last offline collection receipt.
    result.counts[0]=sd.bank_ui.collected_characters;
    result.counts[1]=sd.bank_ui.skipped_characters;
    return result;
}
// Reuse the native status handler to request a HUD redraw. This emits only
// the ordinary clamped wire value; full precision stays in the companion
// snapshot. Never nudge the native HUD with an in-flight financial snapshot.
static void pn_bank_refresh_native_wallet(map_session_data* sd, const pn_bank::Request& request,
    const pn_bank::Reply& reply, bool authenticated) {
    if (!authenticated || !sd || request.action != pn_bank::Refresh ||
        reply.result == pn_bank::Saving || reply.result == pn_bank::Unauthorized ||
        pc_transaction_pending(sd) || !pn_bank::valid_reply(reply) ||
        reply.char_id != sd->status.char_id || reply.wallet != sd->status.zeny ||
        reply.nonce_hi != sd->bank_ui.nonce_hi || reply.nonce_lo != sd->bank_ui.nonce_lo) return;
    clif_updatestatus(*sd, SP_ZENY);
}
static pn_bank::Result pn_bank_action(map_session_data& sd, const pn_bank::Request& request) {
    if (!request.request_id || request.nonce_hi != sd.bank_ui.nonce_hi || request.nonce_lo != sd.bank_ui.nonce_lo)
        return pn_bank::Stale;
    if (request.request_id <= sd.bank_ui.request_id) {
        if (request.request_id == sd.bank_ui.request_id && request.action == sd.bank_ui.action && request.amount == sd.bank_ui.amount && request.trade_id == sd.bank_ui.request_trade_id && request.trade_revision == sd.bank_ui.request_trade_revision)
            return (sd.bank_ui.pending || sd.pair_commit.pending) ? pn_bank::Saving : static_cast<pn_bank::Result>(sd.bank_ui.result);
        return pn_bank::Stale;
    }
    if (sd.bank_ui.pending || sd.pair_commit.pending) return pn_bank::Saving;
    if (request.action >= pn_bank::TradeSetOffer && request.action <= pn_bank::TradeCancel) {
        auto result = trade_wide_action(sd,request);
        if (result == pn_bank::Ok || result == pn_bank::Saving) {
            sd.bank_ui.request_id = request.request_id;sd.bank_ui.action = request.action;sd.bank_ui.amount = request.amount;
            sd.bank_ui.request_trade_id = request.trade_id;sd.bank_ui.request_trade_revision = request.trade_revision;
            sd.bank_ui.result = result;
        }
        return result;
    }
    if (request.trade_id || request.trade_revision) return pn_bank::Invalid;
    if (!battle_config.feature_banking || map_getmapflag(sd.m, MF_NOBANK) || !chrif_isconnected()) return pn_bank::Unavailable;
    if (pc_isdead(&sd) || pc_cant_act(&sd) || sd.state.mail_writing || sd.mail.pending_zeny || sd.state.autotrade || sd.state.warping ||
        sd.state.changemap || gettick() - sd.bank_ui.last_action_tick < 250) return pn_bank::Busy;
    if (intif_saveregistry(&sd) < 0) return pn_bank::Busy;
    auto snapshot = pn_bank_snapshot(sd);
    if (request.action == pn_bank::CollectOffline) {
        if (request.amount || snapshot.trade_id) return pn_bank::Invalid;
        auto& state=sd.bank_ui;
        state.request_id=request.request_id;state.action=request.action;state.amount=0;
        state.request_trade_id=state.request_trade_revision=0;
        state.bank_before=sd.bank_vault;state.wallet_before=sd.status.zeny;
        state.pending=true;state.result=pn_bank::Saving;state.last_action_tick=gettick();
        intif_bank_sweep_save(sd);return pn_bank::Saving;
    }
    auto plan = pn_bank::plan(snapshot, request.action, request.amount);
    if (plan.result != pn_bank::Ok) return plan.result;

    // The map loop is single-threaded. No player input or split financial save
    // may escape between mutation and the character server's atomic commit.
    auto& state = sd.bank_ui;
    state.pending = state.applying = true;
    state.bank_before = sd.bank_vault; state.wallet_before = sd.status.zeny;
    if (plan.wallet < sd.status.zeny) pc_payzeny(&sd, sd.status.zeny - plan.wallet, LOG_TYPE_BANK);
    else if (plan.wallet > sd.status.zeny) pc_getzeny(&sd, plan.wallet - sd.status.zeny, LOG_TYPE_BANK);
    pc_setparam(&sd, SP_BANK_VAULT, plan.bank);
    state.request_id = request.request_id; state.action = request.action; state.amount = request.amount;
    state.request_trade_id = state.request_trade_revision = 0;
    state.native_action = 0;
    state.result = pn_bank::Saving; state.applying = false; state.last_action_tick = gettick();
    intif_bank_save(sd);
    return pn_bank::Saving;
}

int32 clif_bank_native_transfer(map_session_data& sd, int32 amount, bool deposit) {
    auto snapshot = pn_bank_snapshot(sd);
    pn_bank::Request request;
    request.nonce_hi = snapshot.nonce_hi; request.nonce_lo = snapshot.nonce_lo;
    request.request_id = sd.bank_ui.request_id + 1;
    request.action = deposit ? pn_bank::Deposit : pn_bank::Withdraw;
    request.amount = amount;
    auto result = pn_bank_action(sd, request);
    if (result == pn_bank::Saving) sd.bank_ui.native_action = deposit ? 1 : 2;
    return result;
}

// Called before game packet obfuscation, only on an unauthenticated companion
// connection. Its credentials must match an already authenticated live player.
static int32 clif_parse_bank_companion_session(int32 fd);
// A companion owns only this small identity record, never a player pointer.
// delete_session frees it. Validate the nonce as well as fd to prevent reuse
// after logout or character selection from opening another player's panel.
struct pn_bank_companion {
    uint32_t account_id, char_id, login_id1, login_id2;
    uint64_t nonce_hi, nonce_lo;
};
static bool pn_bank_companion_matches(const pn_bank_companion& peer, const map_session_data& sd) {
    return peer.account_id == sd.status.account_id && peer.char_id == sd.status.char_id &&
        peer.login_id1 == sd.login_id1 && peer.login_id2 == sd.login_id2 &&
        peer.nonce_hi == sd.bank_ui.nonce_hi && peer.nonce_lo == sd.bank_ui.nonce_lo;
}
static bool clif_bank_open_custom(map_session_data& sd) {
    if (!sd.state.active || !sd.prev || !session_isValid(sd.fd) || session[sd.fd]->flag.eof) return false;
    sd.bank_ui.open_requested = true;
    // Some clients create their stock window before requesting the balance.
    // Dismiss it immediately, including during attachment/reconnection. Never
    // send stock open/balance replies while the custom open is waiting.
    clif_bank_close(sd);
    int fd = sd.bank_ui.companion_fd;
    if (!session_isValid(fd) || session[fd]->flag.eof ||
        session[fd]->func_parse != clif_parse_bank_companion_session ||
        !session[fd]->session_data ||
        session[fd]->client_addr != session[sd.fd]->client_addr ||
        !pn_bank_companion_matches(*static_cast<pn_bank_companion*>(session[fd]->session_data), sd)) return false;
    auto reply = pn_bank_snapshot(sd);
    reply.flags = pn_bank::open_panel;
    WFIFOHEAD(fd, sizeof(reply));
    memcpy(WFIFOP(fd, 0), &reply, sizeof(reply));
    WFIFOSET(fd, sizeof(reply));
    sd.bank_ui.open_requested = false;
    return true;
}
void clif_bank_trade_open(map_session_data& sd) { clif_bank_open_custom(sd); }
static bool clif_parse_bank_companion(int32 fd) {
    if (RFIFOW(fd, 0) != (pn_bank::magic & 0xffff)) return false;
    session[fd]->func_parse = clif_parse_bank_companion_session;
    if (RFIFOREST(fd) < 8) return true;
    if (RFIFOL(fd,0) != pn_bank::magic || RFIFOW(fd,4) != pn_bank::version || RFIFOW(fd,6) != sizeof(pn_bank::Request)) {
        set_eof(fd);return true;
    }
    if (RFIFOREST(fd) < sizeof(pn_bank::Request)) return true; // TCP may split the frame.
    pn_bank::Request request;
    memcpy(&request, RFIFOP(fd, 0), sizeof(request));
    RFIFOSKIP(fd, sizeof(request));
    pn_bank::Reply reply;
    if (request.magic_value != pn_bank::magic || request.protocol != pn_bank::version ||
        request.length != sizeof(request) || request.reserved || request.action > pn_bank::CollectOffline) {
        set_eof(fd); return true;
    }
    auto* sd = map_id2sd(request.account_id);
    bool authenticated = false;
    if (sd && sd->state.active && sd->prev && sd->status.char_id == request.char_id &&
        sd->login_id1 == request.login_id1 && sd->login_id2 == request.login_id2 &&
        session_isValid(sd->fd) && !session[sd->fd]->flag.eof &&
        session[sd->fd]->client_addr == session[fd]->client_addr) {
        reply = pn_bank_snapshot(*sd);
        auto* peer = static_cast<pn_bank_companion*>(session[fd]->session_data);
        if (!peer) {
            peer = static_cast<pn_bank_companion*>(aCalloc(1, sizeof(pn_bank_companion)));
            session[fd]->session_data = peer;
        }
        // Retire the previous attachment before replacing it. A new character
        // or login must authenticate independently, even on a recycled fd.
        if (auto* previous = map_id2sd(peer->account_id))
            if (previous->bank_ui.companion_fd == fd) previous->bank_ui.companion_fd = 0;
        *peer = {request.account_id, request.char_id, request.login_id1, request.login_id2,
            reply.nonce_hi, reply.nonce_lo};
        sd->bank_ui.companion_fd = fd;
        authenticated = true;
        if (request.action != pn_bank::Refresh) {
            auto result = pn_bank_action(*sd, request);
            reply = pn_bank_snapshot(*sd); reply.result = result;
        }
    }
    WFIFOHEAD(fd, sizeof(reply));
    memcpy(WFIFOP(fd, 0), &reply, sizeof(reply));
    WFIFOSET(fd, sizeof(reply));
    pn_bank_refresh_native_wallet(sd, request, reply, authenticated);
    // Keep the ordinary request/reply stream separate from open notifications.
    if (authenticated && sd->bank_ui.open_requested) clif_bank_open_custom(*sd);
    return true;
}
static int32 clif_parse_bank_companion_session(int32 fd) {
    if (session[fd]->flag.eof) {
        auto* peer = static_cast<pn_bank_companion*>(session[fd]->session_data);
        if (peer) {
            auto* sd = map_id2sd(peer->account_id);
            if (sd && sd->bank_ui.companion_fd == fd && pn_bank_companion_matches(*peer, *sd))
                sd->bank_ui.companion_fd = 0;
        }
        do_close(fd); return 0;
    }
    if (RFIFOREST(fd) < 2) return 0;
    if (!clif_parse_bank_companion(fd)) set_eof(fd);
    return 0;
}
