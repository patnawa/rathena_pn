// Included by map/intif.cpp. Retry exactly the submitted bytes until a durable
// result is known. No payment, item delivery, or client success precedes it.
#include <algorithm>
#include <deque>
#include <custom/shop_state.hpp>
#include <custom/item_use.hpp>
#include <custom/pet_floor.hpp>
#include <map/npc.hpp>
#include <map/script.hpp>
#include <common/runtime_metrics.hpp>

static bool pn_shop_inflight = false;
static uint64_t pn_shop_inflight_sequence=0;
static constexpr size_t pn_shop_queue_limit=32;
struct PnShopQueued { uint32 account,character;uint64 sequence;t_tick since; };
static std::deque<PnShopQueued> pn_shop_queue;
// Catalog writes must wait for every unresolved purchase, even when another
// buyer can join the queue. Admission capacity is a separate predicate.
bool pn_shop_stock_busy() { return !pn_shop_queue.empty(); }
bool pn_shop_queue_full() { return pn_shop_queue.size()>=pn_shop_queue_limit; }
static void pn_shop_resolve(const pn_shop::Ack& reply,bool unsent=false);
static void pn_shop_pump();
static bool pn_shop_active(const pn_shop::Commit& request) {
    return !pn_shop_queue.empty() && pn_shop_queue.front().account==request.account_id &&
        pn_shop_queue.front().character==request.char_id && pn_shop_queue.front().sequence==request.sequence;
}
static void pn_shop_release(const pn_shop::Ack& reply) {
    if(pn_shop_queue.empty() || pn_shop_queue.front().account!=reply.account_id ||
       pn_shop_queue.front().character!=reply.char_id || pn_shop_queue.front().sequence!=reply.sequence)return;
    pn_shop_queue.pop_front();pn_shop_inflight=!pn_shop_queue.empty();
    pn_shop_inflight_sequence=pn_shop_inflight?pn_shop_queue.front().sequence:0;
    pn_metrics::runtime.shop_queue_depth=pn_shop_queue.size();
}

static void pn_shop_send(const pn_shop::Commit& request,const std::vector<uint8_t>* progression=nullptr) {
    if (CheckForCharServer() || !chrif_isconnected() || !pn_shop_active(request)) return;
    std::shared_ptr<const pn_shop::Commit> owned;
    const pn_shop::Commit* outgoing=&request;
    if (auto* sd = map_id2sd(request.account_id)) {
        if (sd->status.char_id!=request.char_id || !sd->shop_commit.pending ||
            !sd->shop_commit.request || sd->shop_commit.request->sequence!=request.sequence)return;
        if (!sd->registry_saves.empty()) { intif_registry_replay(sd); return; }
        // Waiting players keep the existing transaction fence. Recheck capacity
        // in case equipment/status effects changed the limit while queued.
        if(!sd->shop_commit.admitted) {
            bool capacity=sd->shop_commit.final_weight<=sd->max_weight &&
                sd->status.inventory_slots>=1 && sd->status.inventory_slots<=MAX_INVENTORY;
            for(int i=sd->status.inventory_slots;i<MAX_INVENTORY && capacity;++i)
                if(request.items[i].nameid)capacity=false;
            if(!capacity) {
                pn_shop::Ack reply;reply.account_id=request.account_id;reply.char_id=request.char_id;
                reply.nonce_hi=request.nonce_hi;reply.nonce_lo=request.nonce_lo;reply.sequence=request.sequence;
                reply.outcome=pn_shop::Rejected;pn_shop_resolve(reply,true);return;
            }
            auto admitted=std::make_shared<pn_shop::Commit>(request);
            if(!pn_shop_stock_rebase(*admitted))return;
            sd->shop_commit.request=admitted;sd->shop_commit.admitted=true;
        }
        owned=sd->shop_commit.request;outgoing=owned.get();
    }
    if(progression && !progression->empty()) {
        WFIFOHEAD(inter_fd,progression->size());
        memcpy(WFIFOP(inter_fd,0),progression->data(),progression->size());
        WFIFOSET(inter_fd,progression->size());
    }
    WFIFOHEAD(inter_fd, sizeof(*outgoing));
    memcpy(WFIFOP(inter_fd, 0), outgoing, sizeof(*outgoing));
    WFIFOSET(inter_fd, sizeof(*outgoing));
}
static void pn_shop_pump() {
    static bool pumping=false;if(pumping)return;pumping=true;
    while(!pn_shop_queue.empty()) {
        const auto next=pn_shop_queue.front();
        if(!pn_metrics::runtime.pending)pn_metrics::runtime.shop_started(next.since);
        auto* sd=map_id2sd(next.account);
        if(sd && sd->status.char_id==next.character && sd->shop_commit.request && sd->shop_commit.request->sequence==next.sequence)
            pn_shop_send(*sd->shop_commit.request,&sd->shop_commit.progression);
        else if(auto floor=pn_pet_floor_pending(next.account,next.sequence))pn_shop_send(*floor);
        if(!pn_shop_queue.empty() && pn_shop_queue.front().sequence==next.sequence)break;
    }
    pumping=false;
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
    pn_shop_send(*sd->shop_commit.request,&sd->shop_commit.progression);
    add_timer(tick + 1000, pn_shop_retry, id, data);
    return 0;
}
bool pn_shop_submit(map_session_data& sd, std::shared_ptr<pn_shop::Commit> request,
        std::vector<pn_shop::Event> events, uint32_t final_weight) {
    auto planned_achievements=std::move(sd.shop_commit.planned_achievements);
    sd.shop_commit.planned_achievements.clear();
    sd.shop_commit.progression_prepared=false;
    if (!request || pn_shop_queue_full() || pc_transaction_pending(&sd) || pn_item_use_active(&sd) ||
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
    request->progression_size=0;
    std::vector<uint8_t> progression;
    std::vector<achievement> progression_before,progression_after;
    // Current production configuration has no market job EXP. Resolve every
    // known success-only objective before commit so a map crash cannot lose a
    // callback after SQL commit. Configured shop EXP still follows its existing
    // live callback until its larger job progression snapshot is supported.
    const bool has_rental=std::any_of(events.begin(),events.end(),[&](const pn_shop::Event& event) {
        return event.index>=0 && event.index<MAX_INVENTORY && request->items[event.index].expire_time;
    });
    // Rental expiry is rechecked by the real delivery callback. Keep the whole
    // transaction on that live path when any rental is present so a delayed
    // commit cannot persist progress for an item the callback then discards.
    bool progression_prepared=false;
    if(battle_config.shop_exp<=0 && battle_config.feature_achievement && !has_rental) {
        for(const auto& event:events)if(event.index>=0 && event.index<MAX_INVENTORY) {
            planned_achievements.push_back({AG_GET_ITEM,{static_cast<int32>(event.value_sell)}});
        }
        if(!planned_achievements.empty()) {
            std::vector<achievement> before,after;
            if(!achievement_prepare_shop_events(sd,planned_achievements,before,after))return false;
            progression_prepared=true;
            auto equal=[](const std::vector<achievement>& a,const std::vector<achievement>& b) {
                if(a.size()!=b.size())return false;
                for(size_t i=0;i<a.size();++i)if(a[i].achievement_id!=b[i].achievement_id ||
                    a[i].completed!=b[i].completed || a[i].rewarded!=b[i].rewarded ||
                    memcmp(a[i].count,b[i].count,sizeof(a[i].count)))return false;
                return true;
            };
            if(!equal(before,after)) {
                progression=pn_shop_progression::encode(request->account_id,request->char_id,
                    request->nonce_hi,request->nonce_lo,request->sequence,before,after);
                if(progression.empty())return false;
                request->progression_size=static_cast<uint32>(progression.size());
                progression_before=std::move(before);
                progression_after=std::move(after);
            }
        }
    }
    if (!pn_shop::valid(*request)) return false;
    if(request->point.scope && pc_readreg2(&sd,request->point.key)!=request->point.before)return false;
    // Ordered on the same map/char stream: persist current assets before the
    // conditional purchase. Ordinary saves are fenced from this point to ACK.
    if (chrif_save(&sd, CSAVE_NORMAL | CSAVE_INVENTORY) != 0) return false;
    // Copy once: callers cannot subsequently mutate the retry payload.
    sd.shop_commit.request = std::make_shared<const pn_shop::Commit>(*request);
    sd.shop_commit.events = std::move(events);
    sd.shop_commit.progression_prepared=progression_prepared;
    sd.shop_commit.progression = std::move(progression);
    sd.shop_commit.progression_before = std::move(progression_before);
    sd.shop_commit.progression_after = std::move(progression_after);
    sd.shop_commit.final_weight = final_weight;
    sd.shop_commit.pending = true;
    pn_shop_queue.push_back({request->account_id,request->char_id,request->sequence,gettick()});
    pn_shop_inflight=true;pn_shop_inflight_sequence=pn_shop_queue.front().sequence;
    pn_metrics::runtime.shop_queue_depth=pn_shop_queue.size();
    if(request->kind<=pn_shop::Sale && pn_shop_queue.size()>1)
        clif_displaymessage(sd.fd,"Your purchase is queued. It will finish in turn.");
    if(pn_shop_queue.size()==1)pn_shop_pump();
    add_timer(gettick() + 1000, pn_shop_retry, sd.status.account_id,
        static_cast<intptr_t>(request->sequence));
    return true;
}

static void pn_shop_resolve(const pn_shop::Ack& reply,bool unsent) {
    const bool floor_resolved=pn_pet_floor_ack(reply);
    auto* sd = map_id2sd(reply.account_id);
    if (!sd || !sd->shop_commit.pending || !sd->shop_commit.request) {
        if(floor_resolved && pn_shop_inflight_sequence==reply.sequence){pn_shop_release(reply);pn_metrics::runtime.shop_finished(gettick(),reply.outcome==pn_shop::Committed);pn_shop_pump();}
        return;
    }
    const auto request = sd->shop_commit.request;
    if(!pn_shop_active(*request))return;
    if (sd->status.char_id != reply.char_id || request->account_id != reply.account_id ||
        request->char_id != reply.char_id || request->nonce_hi != reply.nonce_hi ||
        request->nonce_lo != reply.nonce_lo || request->sequence != reply.sequence ||
        (reply.outcome != pn_shop::Committed && reply.outcome != pn_shop::Rejected &&
         reply.outcome != pn_shop::ProgressionStale)) {
        if(floor_resolved && pn_shop_inflight_sequence==reply.sequence){pn_shop_release(reply);pn_metrics::runtime.shop_finished(gettick(),reply.outcome==pn_shop::Committed);pn_shop_pump();}
        return;
    }
    // A reload/admin operation may have changed the shop since submission.
    // Refresh from authoritative SQL, including on rejection. Retry ACK lookup
    // if refresh fails instead of exposing a stale stock cache to the next buy.
    pn_metrics::runtime.shop_acknowledged(gettick());
    if (!unsent && !pn_shop_stock_refresh(*request)) {
        ++pn_metrics::runtime.shop_refresh_failures;
        return;
    }
    auto events = sd->shop_commit.events;
    auto progression_bytes=sd->shop_commit.progression;
    auto progression_before=sd->shop_commit.progression_before;
    auto progression_after=sd->shop_commit.progression_after;
    auto deferred=std::move(sd->shop_commit.deferred_achievements);
    bool progression_applied=true;
    if (reply.outcome == pn_shop::Committed) {
        auto logtype = pn_shop::auction_kind(request->kind) ? LOG_TYPE_AUCTION : request->kind == pn_shop::MailSend || request->mail_id ? LOG_TYPE_MAIL : request->kind == pn_shop::Sale ? LOG_TYPE_CASH :
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
            if (request->cash_after != request->cash_before)
                log_cash(sd, logtype, LOG_CASH_TYPE_CASH, request->cash_after - request->cash_before);
            if (request->kafra_after != request->kafra_before)
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
        // Install the SQL-owned achievement image before delivery callbacks.
        // Callback-created progression then builds on it instead of being
        // overwritten by a later snapshot replacement.
        if(request->progression_size) {
            pn_shop_progression::Bundle progression;
            progression_applied=pn_shop_progression::decode(progression_bytes.data(),progression_bytes.size(),progression) &&
                progression.header.account_id==request->account_id && progression.header.char_id==request->char_id &&
                progression.header.nonce_hi==request->nonce_hi && progression.header.nonce_lo==request->nonce_lo &&
                progression.header.sequence==request->sequence &&
                pn_shop_progression::encode(request->account_id,request->char_id,request->nonce_hi,request->nonce_lo,
                    request->sequence,progression_before,progression_after)==progression_bytes &&
                achievement_apply_shop(*sd,progression_after);
        }
    }
    // Clear before callbacks; repeated ACKs cannot reinstall this snapshot.
    sd->shop_commit = {};
    pn_shop_release(reply);
    pn_metrics::runtime.shop_finished(gettick(),reply.outcome == pn_shop::Committed);
    pn_item_use_settled(*sd,reply.outcome == pn_shop::Committed);
    pn_mail_asset_result(*sd,*request,reply.outcome == pn_shop::Committed);
    clif_shop_commit_result(*sd, *request, events, reply.outcome == pn_shop::Committed);
    if (reply.outcome == pn_shop::ProgressionStale || !progression_applied) {
        // A stale baseline means SQL contains progress this map must not
        // overwrite. A local decode/apply failure has the same authority rule.
        sd->achievement_data.loaded=false;
        sd->achievement_data.save=false;
        set_eof(sd->fd);
        pn_shop_pump();return;
    }
    if (reply.outcome == pn_shop::Committed) {
        {
            PcItemDeliveryScope delivery(*sd);
            for (const auto& e : events) {
                if(e.index<0 || e.index>=MAX_INVENTORY)continue; // Entitlement, not yet an inventory delivery.
                auto data = item_db.find(e.nameid);
                if (data) delivery.added(e.index, *data);
            }
            if(request->progression_size)delivery.suppress_achievements();
            delivery.refresh_questinfo();
            if(!request->progression_size)pn_shop_committed_effects(*sd, *request);
        }
    }
    for(const auto& event:deferred)
        achievement_update_objective_values(sd,static_cast<e_achievement_group>(event.group),event.arguments);
    chrif_save(sd, CSAVE_NORMAL | CSAVE_INVENTORY);
    if(reply.outcome==pn_shop::Committed && request->pet_count && request->kind!=pn_shop::PetClaim && !pn_pet_recover(*sd))
        clif_displaymessage(sd->fd,"Your pet rewards are saved. Free inventory space/weight and use @petrewards to recover them.");
    pn_shop_pump();
}
static void intif_parse_ShopCommitted(int32 fd) {
    pn_shop::Ack reply;memcpy(&reply,RFIFOP(fd,0),sizeof(reply));pn_shop_resolve(reply);
}
