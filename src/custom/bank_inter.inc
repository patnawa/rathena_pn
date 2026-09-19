// Included by map/intif.cpp. Current state is retried; snapshots are not cached.
#include <custom/bank_commit.hpp>

static void intif_bank_save_request(const map_session_data& sd) {
    if (CheckForCharServer() || !chrif_isconnected()) return;
    pn_bank_commit request;
    request.length = sizeof(request) + sizeof(s_storage);
    request.account_id = sd.status.account_id; request.char_id = sd.status.char_id;
    request.nonce_hi = sd.bank_ui.nonce_hi; request.nonce_lo = sd.bank_ui.nonce_lo;
    request.request_id = sd.bank_ui.request_id; request.action = sd.bank_ui.action;
    request.amount = sd.bank_ui.amount;
    request.bank_before = sd.bank_ui.bank_before; request.wallet_before = sd.bank_ui.wallet_before;
    request.bank_after = sd.bank_vault; request.wallet_after = sd.status.zeny;
    if (request.action == 7) {
        request.reserve_before = sd.bank_ui.reserve_before; request.reserve_after = sd.bank_ui.reserve_after;
        for (int i = 0; i < 2; ++i) {
            request.reserve_item[i] = sd.bank_ui.reserve_item[i];
            request.reserve_quantity[i] = sd.bank_ui.reserve_quantity[i];
        }
    }
    WFIFOHEAD(inter_fd, request.length);
    memcpy(WFIFOP(inter_fd, 0), &request, sizeof(request));
    memcpy(WFIFOP(inter_fd, sizeof(request)), &sd.inventory, sizeof(s_storage));
    WFIFOSET(inter_fd, request.length);
}
static TIMER_FUNC(intif_bank_save_retry) {
    map_session_data* sd = map_id2sd(id);
    if (!sd || !sd->bank_ui.pending || sd->bank_ui.request_id != static_cast<uint64>(data)) return 0;
    intif_bank_save_request(*sd);
    add_timer(tick + 1000, intif_bank_save_retry, id, data);
    return 0;
}
void intif_bank_save(map_session_data& sd) {
    intif_bank_save_request(sd);
    add_timer(gettick() + 1000, intif_bank_save_retry, sd.status.account_id,
        static_cast<intptr_t>(sd.bank_ui.request_id));
}
static void intif_parse_BankCommitted(int32 fd) {
    pn_bank_ack reply;
    memcpy(&reply, RFIFOP(fd, 0), sizeof(reply));
    map_session_data* sd = map_id2sd(reply.account_id);
    if (!sd || !sd->bank_ui.pending || sd->status.char_id != reply.char_id ||
        sd->bank_ui.nonce_hi != reply.nonce_hi || sd->bank_ui.nonce_lo != reply.nonce_lo ||
        sd->bank_ui.request_id != reply.request_id || reply.committed != 1) return;
    sd->bank_ui.pending = false;
    sd->bank_ui.result = pn_bank::Ok;
    auto native_action = sd->bank_ui.native_action;
    sd->bank_ui.native_action = 0;
    // Flush deferred ordinary state, including any loot/zeny added while saving.
    chrif_save(sd, CSAVE_NORMAL | CSAVE_INVENTORY);
    if (native_action == 1) clif_bank_deposit(*sd, BDA_SUCCESS);
    if (native_action == 2) clif_bank_withdraw(*sd, BWA_SUCCESS);
}
