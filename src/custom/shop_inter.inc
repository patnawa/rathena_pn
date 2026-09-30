// Included by map/intif.cpp. Retry exactly the submitted bytes until a durable
// result is known. No payment, item delivery, or client success precedes it.
#include <custom/shop_state.hpp>
#include <custom/item_use.hpp>
#include <custom/pet_floor.hpp>
#include <map/npc.hpp>
#include <map/script.hpp>
#include <common/runtime_metrics.hpp>

static bool pn_shop_inflight = false;
static uint64_t pn_shop_inflight_sequence=0;
bool pn_shop_stock_busy() { return pn_shop_inflight; }

static void pn_shop_send(const pn_shop::Commit& request) {
    if (CheckForCharServer() || !chrif_isconnected()) return;
    WFIFOHEAD(inter_fd, sizeof(request));
    memcpy(WFIFOP(inter_fd, 0), &request, sizeof(request));
    WFIFOSET(inter_fd, sizeof(request));
}
static TIMER_FUNC(pn_shop_retry) {
    auto* sd = map_id2sd(id);
    if (!sd || !sd->shop_commit.pending || !sd->shop_commit.request ||
        sd->shop_commit.request->sequence != static_cast<uint64>(data)) {
        auto floor=pn_pet_floor_pending(id,static_cast<uint64>(data));
        if(!floor)return 0;
        pn_shop_send(*floor);add_timer(tick+1000,pn_shop_retry,id,data);return 0;
    }
    ++pn_metrics::runtime.shop_retry_attempts;
    pn_shop_send(*sd->shop_commit.request);
    add_timer(tick + 1000, pn_shop_retry, id, data);
    return 0;
}
bool pn_shop_submit(map_session_data& sd, std::shared_ptr<pn_shop::Commit> request,
        std::vector<pn_shop::Event> events, uint32_t final_weight) {
    if (!request || pn_shop_inflight || pc_transaction_pending(&sd) || pn_item_use_active(&sd) ||
        CheckForCharServer() || !chrif_isconnected() || !sd.vars_ok) return false;
    static uint64 sequence = 0;
    if (!(sd.bank_ui.nonce_hi | sd.bank_ui.nonce_lo)) {
        sd.bank_ui.nonce_hi = (static_cast<uint64>(rnd()) << 32) | static_cast<uint32>(rnd());
        sd.bank_ui.nonce_lo = (static_cast<uint64>(rnd()) << 32) | static_cast<uint32>(rnd());
        if (!(sd.bank_ui.nonce_hi | sd.bank_ui.nonce_lo)) sd.bank_ui.nonce_lo = 1;
    }
    request->nonce_hi = sd.bank_ui.nonce_hi;
    request->nonce_lo = sd.bank_ui.nonce_lo;
    if (++sequence == 0) ++sequence;
    request->sequence = sequence;
    request->length = sizeof(*request);
    if (!pn_shop::valid(*request)) return false;
    if(request->point.scope && pc_readreg2(&sd,request->point.key)!=request->point.before)return false;
    // Ordered on the same map/char stream: persist current assets before the
    // conditional purchase. Ordinary saves are fenced from this point to ACK.
    if (chrif_save(&sd, CSAVE_NORMAL | CSAVE_INVENTORY) != 0) return false;
    // Copy once: callers cannot subsequently mutate the retry payload.
    sd.shop_commit.request = std::make_shared<const pn_shop::Commit>(*request);
    sd.shop_commit.events = std::move(events);
    sd.shop_commit.final_weight = final_weight;
    sd.shop_commit.pending = true;
    pn_shop_inflight = true;pn_shop_inflight_sequence=request->sequence;
    pn_metrics::runtime.shop_started(gettick());
    pn_shop_send(*sd.shop_commit.request);
    add_timer(gettick() + 1000, pn_shop_retry, sd.status.account_id,
        static_cast<intptr_t>(request->sequence));
    return true;
}

static void intif_parse_ShopCommitted(int32 fd) {
    pn_shop::Ack reply;
    memcpy(&reply, RFIFOP(fd, 0), sizeof(reply));
    const bool floor_resolved=pn_pet_floor_ack(reply);
    auto* sd = map_id2sd(reply.account_id);
    if (!sd || !sd->shop_commit.pending || !sd->shop_commit.request) {
        if(floor_resolved && pn_shop_inflight_sequence==reply.sequence){pn_shop_inflight=false;pn_shop_inflight_sequence=0;}
        return;
    }
    const auto request = sd->shop_commit.request;
    if (sd->status.char_id != reply.char_id || request->account_id != reply.account_id ||
        request->char_id != reply.char_id || request->nonce_hi != reply.nonce_hi ||
        request->nonce_lo != reply.nonce_lo || request->sequence != reply.sequence ||
        (reply.outcome != pn_shop::Committed && reply.outcome != pn_shop::Rejected)) {
        if(floor_resolved && pn_shop_inflight_sequence==reply.sequence){pn_shop_inflight=false;pn_shop_inflight_sequence=0;}
        return;
    }
    // A reload/admin operation may have changed the shop since submission.
    // Refresh from authoritative SQL, including on rejection. Retry ACK lookup
    // if refresh fails instead of exposing a stale stock cache to the next buy.
    pn_metrics::runtime.shop_acknowledged(gettick());
    if (!pn_shop_stock_refresh(*request)) {
        ++pn_metrics::runtime.shop_refresh_failures;
        return;
    }
    auto events = sd->shop_commit.events;
    if (reply.outcome == pn_shop::Committed) {
        auto logtype = request->kind == pn_shop::Sale ? LOG_TYPE_CASH :
            request->kind == pn_shop::Barter || request->response==pn_shop::BarterResponse ? LOG_TYPE_BARTER : LOG_TYPE_NPC;
        // Record the consumed part before replacing the immutable inventory.
        for (int i = 0; i < MAX_INVENTORY; ++i) {
            const auto& old = sd->inventory.u.items_inventory[i];
            const auto& next = request->items[i];
            if (!old.nameid) continue;
            int remaining = old.nameid == next.nameid && old.unique_id == next.unique_id ? next.amount : 0;
            for (const auto& e : events) if (e.index == i) remaining -= e.amount;
            const int consumed = old.amount - std::max(0, remaining);
            if (consumed > 0) log_pick_pc(sd, logtype, -consumed, &old);
        }
        sd->shop_commit.applying = true;
        memcpy(sd->inventory.u.items_inventory, request->items, sizeof(request->items));
        sd->status.zeny = request->wallet_after;
        sd->status.uniqueitem_counter = request->counter_after;
        if (request->kind == pn_shop::Sale || request->kind == pn_shop::Asset || request->kind == pn_shop::ItemUse) {
            sd->cashPoints = static_cast<int32>(request->cash_after);
            sd->kafraPoints = static_cast<int32>(request->kafra_after);
            pc_setaccountreg(sd, add_str(CASHPOINT_VAR), request->cash_after);
            pc_setaccountreg(sd, add_str(KAFRAPOINT_VAR), request->kafra_after);
            log_cash(sd, logtype, LOG_CASH_TYPE_CASH, request->cash_after - request->cash_before);
            log_cash(sd, logtype, LOG_CASH_TYPE_KAFRA, request->kafra_after - request->kafra_before);
        }
        if(request->point.scope)set_reg_num(nullptr,sd,add_str(request->point.key),request->point.key,request->point.after,nullptr);
        sd->weight = sd->shop_commit.final_weight;
        pc_setinventorydata(*sd);
        pc_setequipindex(sd);
        for (const auto& e : events)if(e.index>=0 && e.index<MAX_INVENTORY)
            log_pick_pc(sd, logtype, e.amount, &sd->inventory.u.items_inventory[e.index]);
        log_zeny(*sd, logtype, 0, request->wallet_after - request->wallet_before);
        clif_inventorylist(sd);
        clif_updatestatus(*sd, SP_ZENY);
        clif_updatestatus(*sd, SP_WEIGHT);
    }
    // Clear before callbacks; repeated ACKs cannot reinstall this snapshot.
    sd->shop_commit = {};
    pn_shop_inflight = false;pn_shop_inflight_sequence=0;
    pn_metrics::runtime.shop_finished(gettick(),reply.outcome == pn_shop::Committed);
    pn_item_use_settled(*sd,reply.outcome == pn_shop::Committed);
    pn_mail_asset_result(*sd,*request,reply.outcome == pn_shop::Committed);
    clif_shop_commit_result(*sd, *request, events, reply.outcome == pn_shop::Committed);
    if (reply.outcome == pn_shop::Committed) {
        PcItemDeliveryScope delivery(*sd);
        for (const auto& e : events) {
            if(e.index<0 || e.index>=MAX_INVENTORY)continue; // Entitlement, not yet an inventory delivery.
            auto data = item_db.find(e.nameid);
            if (data) delivery.added(e.index, *data);
        }
        delivery.refresh_questinfo();
        pn_shop_committed_effects(*sd, *request);
    }
    chrif_save(sd, CSAVE_NORMAL | CSAVE_INVENTORY);
    if(reply.outcome==pn_shop::Committed && request->pet_count && request->kind!=pn_shop::PetClaim && !pn_pet_recover(*sd))
        clif_displaymessage(sd->fd,"Your pet rewards are saved. Free inventory space/weight and use @petrewards to recover them.");
}
