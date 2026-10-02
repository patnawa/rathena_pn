// Included by map/intif.cpp. Retry current state; acknowledge only the tagged operation.
// PN contributions: GPL-3.0-or-later.
static pn_storage_request storage_request_identity(const map_session_data& sd) {
    pn_storage_request request;
    request.account_id = sd.status.account_id;request.char_id = sd.status.char_id;
    request.nonce_hi = sd.multi_storage.nonce_hi;request.nonce_lo = sd.multi_storage.nonce_lo;
    request.sequence = sd.multi_storage.sequence;request.page = sd.multi_storage.page;
    return request;
}
static bool storage_reply_matches(const map_session_data& sd, const pn_storage_reply& reply) {
    return reply.char_id == sd.status.char_id && reply.nonce_hi == sd.multi_storage.nonce_hi &&
        reply.nonce_lo == sd.multi_storage.nonce_lo && reply.sequence == sd.multi_storage.sequence &&
        reply.page == sd.multi_storage.page;
}
static bool intif_storage_page_request(const map_session_data& sd) {
    if (CheckForCharServer() || !chrif_isconnected()) return false;
    auto request = storage_request_identity(sd);request.action = sd.multi_storage.mode;
    WFIFOHEAD(inter_fd, sizeof(request));memcpy(WFIFOP(inter_fd,0), &request, sizeof(request));
    WFIFOSET(inter_fd, sizeof(request));return true;
}
static TIMER_FUNC(intif_storage_page_retry) {
    auto* sd = map_id2sd(id);
    if (!sd || !sd->multi_storage.loading || sd->multi_storage.sequence != static_cast<uint64>(data)) return 0;
    intif_storage_page_request(*sd);
    add_timer(tick + 1000, intif_storage_page_retry, id, data);return 0;
}
bool intif_storage_page_load(map_session_data& sd) {
    if (!intif_storage_page_request(sd)) return false;
    add_timer(gettick()+1000, intif_storage_page_retry, sd.status.account_id, sd.multi_storage.sequence);
    return true;
}
static void intif_parse_StoragePage(int32 fd) {
    if (RFIFOW(fd,2) != sizeof(pn_storage_reply) + sizeof(s_storage)) return;
    pn_storage_reply reply;memcpy(&reply, RFIFOP(fd,0), sizeof(reply));
    auto* sd = map_id2sd(reply.account_id);
    if (!sd || !sd->multi_storage.loading || !storage_reply_matches(*sd,reply)) return;
    sd->multi_storage.loading = false;
    if (reply.success != 1) { clif_displaymessage(sd->fd,"Storage could not be loaded. Please try again.");return; }
    s_storage loaded;memcpy(&loaded, RFIFOP(fd,sizeof(reply)), sizeof(loaded));
    if (loaded.stor_id != reply.page || loaded.type != TABLE_STORAGE ||
        loaded.max_amount > MAX_STORAGE || loaded.amount > MAX_STORAGE ||
        loaded.id != (reply.page == pn_storage::character ? sd->status.char_id : sd->status.account_id) ||
        sd->premiumStorage.dirty || sd->premiumStorage.status || sd->state.storage_flag ||
        !storage_page_available(*sd,reply.page)) return;
    sd->premiumStorage = loaded;
    storage_premiumStorage_open(sd);
}
static void intif_storage_commit_request(const map_session_data& sd) {
    if (CheckForCharServer() || !chrif_isconnected()) return;
    if (!sd.registry_saves.empty()) { intif_registry_replay(&sd); return; }
    auto request = storage_request_identity(sd);request.packet = 0x3094;
    request.action = sd.multi_storage.action;request.wallet_before = sd.multi_storage.wallet_before;
    request.wallet_after = sd.status.zeny;
    const bool cart = request.action == pn_storage::Cart, unlock = request.action == pn_storage::Unlock;
    const item* source = cart ? sd.cart.u.items_cart : sd.inventory.u.items_inventory;
    const size_t source_bytes = (cart ? MAX_CART : MAX_INVENTORY) * sizeof(item);
    const s_storage& page = request.page == 0 ? sd.storage : sd.premiumStorage;
    if (!unlock && page.stor_id != request.page) return;
    request.length = sizeof(request) + source_bytes + (unlock ? 0 : MAX_STORAGE * sizeof(item));
    WFIFOHEAD(inter_fd, request.length);
    memcpy(WFIFOP(inter_fd,0), &request, sizeof(request));
    memcpy(WFIFOP(inter_fd,sizeof(request)), source, source_bytes);
    if (!unlock) memcpy(WFIFOP(inter_fd,sizeof(request)+source_bytes), page.u.items_storage, MAX_STORAGE*sizeof(item));
    WFIFOSET(inter_fd, request.length);
}
static TIMER_FUNC(intif_storage_commit_retry) {
    auto* sd = map_id2sd(id);
    if (!sd || !sd->multi_storage.pending || sd->multi_storage.sequence != static_cast<uint64>(data)) return 0;
    intif_storage_commit_request(*sd);
    add_timer(tick+1000, intif_storage_commit_retry, id, data);return 0;
}
void intif_storage_commit(map_session_data& sd) {
    intif_storage_commit_request(sd);
    add_timer(gettick()+1000,intif_storage_commit_retry,sd.status.account_id,sd.multi_storage.sequence);
}
static void intif_parse_StorageCommitted(int32 fd) {
    pn_storage_reply reply;memcpy(&reply,RFIFOP(fd,0),sizeof(reply));
    auto* sd = map_id2sd(reply.account_id);
    if (!sd || !sd->multi_storage.pending || !storage_reply_matches(*sd,reply) || reply.success != 1) return;
    const bool unlock = sd->multi_storage.action == pn_storage::Unlock;
    if (!unlock) {
        auto& page = reply.page == 0 ? sd->storage : sd->premiumStorage;
        if (page.stor_id != reply.page) return;
        page.dirty = false;
    }
    sd->multi_storage.pending = false;
    if (unlock) pc_setregistry(sd,reference_uid(add_str(pn_storage::paid_key),reply.page),1);
    // Preserve any ordinary loot/zeny earned while the tagged commit was pending.
    chrif_save(sd,CSAVE_NORMAL|CSAVE_INVENTORY|CSAVE_CART);
}
