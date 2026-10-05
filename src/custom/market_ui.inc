// Included by clif.cpp: independent authenticated PMK1 companion connection.
#include <custom/market_protocol.hpp>

static bool pn_market_shop(const map_session_data& owner, uint32 kind) {
    return kind == pn_market::Vending ? owner.state.vending : owner.state.buyingstore;
}
static uint32 pn_market_shop_id(const map_session_data& owner, uint32 kind) {
    return kind == pn_market::Vending ? owner.vender_id : owner.buyer_id;
}
static pn_market::Reply pn_market_snapshot(const map_session_data& viewer, const map_session_data* owner, const pn_market::Request& request) {
    pn_market::Reply reply;
    reply.nonce_hi=viewer.bank_ui.nonce_hi; reply.nonce_lo=viewer.bank_ui.nonce_lo;
    reply.request_id=request.request_id; reply.wallet=viewer.status.zeny; reply.kind=request.kind;
    reply.result=pn_market::Missing;
    // The map state is staged until both economic participants are durable.
    // Read-only polling must not report this snapshot as a completed purchase.
    if(viewer.pair_commit.pending) reply.result=pn_market::Saving;
    if (!owner || !pn_market_shop(*owner,request.kind)) return reply;
    reply.result=(viewer.pair_commit.pending || owner->pair_commit.pending) ? pn_market::Saving : pn_market::Ok;
    reply.owner_account=owner->status.account_id; reply.owner_char=owner->status.char_id;
    reply.shop_id=pn_market_shop_id(*owner,request.kind); reply.revision=owner->market.revision;
    reply.flags=owner->market.published ? pn_market::Published : pn_market::Draft;
    reply.budget=request.kind==pn_market::Buying ? owner->buyingstore.zenylimit : 0;
    safestrncpy(reply.title,owner->message,sizeof(reply.title));
    safestrncpy(reply.owner_name,owner->status.name,sizeof(reply.owner_name));
    const uint32 count=request.kind==pn_market::Vending ? owner->vend_num : owner->buyingstore.slots;
    for (uint32 slot=request.action==pn_market::Search ? 0 : request.cursor;slot<count;++slot) {
        pn_market::Entry entry;
        entry.index=slot;
        if (request.kind==pn_market::Vending) {
            const auto& listing=owner->vending[slot];
            if (listing.index<0 || listing.index>=MAX_CART) continue;
            const auto& item=owner->cart.u.items_cart[listing.index];
            entry.item_id=item.nameid; entry.quantity=listing.amount; entry.price=listing.value;
            entry.unique_id=item.unique_id; entry.refine=item.refine; entry.grade=item.enchantgrade;
            for (int i=0;i<4;++i) entry.cards[i]=item.card[i];
            for (int i=0;i<5;++i) entry.options[i]={item.option[i].id,item.option[i].value,item.option[i].param};
        } else {
            const auto& item=owner->buyingstore.items[slot];
            entry.item_id=item.nameid; entry.quantity=item.amount; entry.price=item.price;
        }
        if (!entry.item_id || !entry.quantity || entry.price<0 ||
            (request.action==pn_market::Search && request.item_id && entry.item_id!=request.item_id) ||
            entry.price<request.min_price || (request.max_price && entry.price>request.max_price)) continue;
        if (reply.count==pn_market::page_size) {reply.has_more=1; reply.next_cursor=slot; break;}
        auto data=item_db.find(entry.item_id);
        if (data) safestrncpy(entry.name,data->ename.c_str(),sizeof(entry.name));
        reply.entries[reply.count++]=entry;
    }
    return reply;
}
static bool pn_market_plain(const item& item, uint32 id, uint32 quantity, uint64 unique) {
    if (item.nameid!=id || item.amount<quantity || !item.identify || item.bound || item.expire_time ||
        item.equip || item.equipSwitch || item.favorite || item.refine || item.enchantgrade || item.attribute ||
        (unique && item.unique_id!=unique)) return false;
    for (auto card:item.card) if (card) return false;
    for (const auto& option:item.option) if (option.id || option.value || option.param) return false;
    return true;
}
static pn_market::Result pn_market_action(map_session_data& sd, map_session_data& owner, const pn_market::Request& request) {
    using namespace pn_market;
    if (!request.request_id || request.request_id<=sd.market.sequence ||
        request.shop_id!=pn_market_shop_id(owner,request.kind) || request.revision!=owner.market.revision) return Stale;
    // Consume once even on failure. An old quote must never repeat a purchase.
    sd.market.sequence=request.request_id;
    if (pc_isdead(&sd) || pc_transaction_pending(&sd) || pc_transaction_pending(&owner) || sd.mail.pending_zeny ||
        sd.state.trading || sd.state.storage_flag || sd.state.warping || sd.state.changemap || sd.state.mail_writing) return Busy;
    if (request.action==Close) {
        if (&sd!=&owner) return Unauthorized;
        if (request.kind==Vending) vending_closevending(&sd); else buyingstore_close(&sd);
        sd.market.published=false; sd.market.revision=pn_market_revision(); return Ok;
    }
    if (request.action==SetPrice || request.action==SetBudget || request.action==Publish) {
        if (&sd!=&owner) return Unauthorized;
        if (owner.market.published) return Busy; // Published quotes cannot change under a native client's consent.
        if (request.action==SetBudget) {
            if (request.kind!=Buying || request.budget<=0 || request.budget>sd.status.zeny) return Funds;
            if (Sql_Query(mmysql_handle,"UPDATE `%s` SET `limit`=%" PRId64 " WHERE id=%u",buyingstores_table,request.budget,request.shop_id)!=SQL_SUCCESS) return Unavailable;
            sd.buyingstore.zenylimit=request.budget;
        } else if (request.action==SetPrice) {
            if (request.budget<0 || (request.kind==Buying && request.budget==0)) return Invalid;
            if (request.kind==Vending) {
                if (request.index>=static_cast<uint32>(sd.vend_num)) return Invalid;
                auto& row=sd.vending[request.index];
                const auto& item=sd.cart.u.items_cart[row.index];
                if (item.nameid!=request.item_id || item.unique_id!=request.item_unique_id || row.value!=request.expected_price) return Stale;
                if (request.budget && row.amount>MAX_WALLET_ZENY/request.budget) return Capacity;
                if (Sql_Query(mmysql_handle,"UPDATE `%s` SET price=%" PRId64 " WHERE vending_id=%u AND cartinventory_id=%u",vending_items_table,request.budget,request.shop_id,item.id)!=SQL_SUCCESS) return Unavailable;
                row.value=request.budget;
            } else {
                if (request.index>=sd.buyingstore.slots) return Invalid;
                auto& row=sd.buyingstore.items[request.index];
                if (row.nameid!=request.item_id || row.price!=request.expected_price) return Stale;
                if (Sql_Query(mmysql_handle,"UPDATE `%s` SET price=%" PRId64 " WHERE buyingstore_id=%u AND `index`=%u",buyingstore_items_table,request.budget,request.shop_id,request.index)!=SQL_SUCCESS) return Unavailable;
                row.price=request.budget;
            }
        } else {
            if (request.kind==Vending) {
                int64 total=0;
                for (int i=0;i<sd.vend_num;++i) {
                    const auto& row=sd.vending[i];
                    if (row.value<0 || (row.value && row.amount>(MAX_WALLET_ZENY-total)/row.value)) return Capacity;
                    total+=row.value*row.amount;
                }
                if (vending_calc_tax(&sd,total)>MAX_BANK_ZENY-sd.bank_vault) return Capacity;
                sd.market.published=true; clif_showvendingboard(sd);
            } else {
                if (sd.buyingstore.zenylimit<=0 || sd.buyingstore.zenylimit>sd.status.zeny) return Funds;
                sd.market.published=true; clif_buyingstore_entry(sd);
            }
        }
        sd.market.revision=pn_market_revision(); return Ok;
    }
    if (&sd==&owner || !owner.market.published || pc_cant_act(&sd) || !pc_can_give_items(&sd) || !pc_can_give_items(&owner)) return Busy;
    if (sd.m!=owner.m || !check_distance_bl(&sd,&owner,AREA_SIZE)) return Unavailable;
    if (!request.quantity || request.quantity>INT16_MAX || request.expected_price<0 ||
        (request.expected_price && request.quantity>MAX_WALLET_ZENY/request.expected_price)) return Invalid;
    const int64 before=sd.status.zeny;
    if (request.action==Purchase && request.kind==Vending) {
        if (request.index>=static_cast<uint32>(owner.vend_num)) return Stale;
        const auto& row=owner.vending[request.index];
        const auto& item=owner.cart.u.items_cart[row.index];
        if (row.value!=request.expected_price || item.nameid!=request.item_id || item.unique_id!=request.item_unique_id || row.amount<request.quantity) return Stale;
        const uint16 packet[2]={static_cast<uint16>(request.quantity),static_cast<uint16>(row.index+2)};
        const uint32 owner_account=owner.status.account_id;
        vending_purchasereq(&sd,owner_account,owner.vender_id,reinterpret_cast<const uint8*>(packet),1,true);
        // A free item is proven by the shop revision, not by a zero wallet delta.
        auto* remaining=map_id2sd(owner_account);
        return sd.pair_commit.pending ? Saving : ((!remaining || remaining->market.revision!=request.revision) ? Ok : Capacity);
    }
    if (request.action==Sell && request.kind==Buying) {
        if (request.index>=owner.buyingstore.slots) return Stale;
        const auto& row=owner.buyingstore.items[request.index];
        if (row.price!=request.expected_price || row.nameid!=request.item_id || row.amount<request.quantity) return Stale;
        for (int i=0;i<MAX_INVENTORY;++i) {
            if (!sd.inventory_data[i] || !pn_market_plain(sd.inventory.u.items_inventory[i],request.item_id,request.quantity,request.item_unique_id)) continue;
            PACKET_CZ_REQ_TRADE_BUYING_STORE_sub item{};
            item.index=i+2; item.itemId=request.item_id; item.amount=request.quantity;
            buyingstore_trade(&sd,owner.status.account_id,owner.buyer_id,&item,1,true);
            return sd.pair_commit.pending ? Saving : (sd.status.zeny>before ? Ok : Capacity);
        }
        return Capacity;
    }
    return Invalid;
}
static int32 clif_parse_market_companion_session(int32 fd);
static bool clif_parse_market_companion(int32 fd) {
    if (RFIFOW(fd,0)!=(pn_market::magic&0xffff)) return false;
    session[fd]->func_parse=clif_parse_market_companion_session;
    if (RFIFOREST(fd)<sizeof(pn_market::Request)) return true;
    pn_market::Request request; memcpy(&request,RFIFOP(fd,0),sizeof(request)); RFIFOSKIP(fd,sizeof(request));
    if (request.magic_value!=pn_market::magic || request.protocol!=pn_market::version || request.length!=sizeof(request) ||
        request.action>pn_market::Close || request.kind>pn_market::Buying || request.min_price<0 || request.max_price<0 ||
        (request.max_price && request.max_price<request.min_price)) {set_eof(fd);return true;}
    pn_market::Reply reply;
    auto* sd=map_id2sd(request.account_id);
    if (sd && sd->state.active && sd->prev && sd->status.char_id==request.char_id &&
        sd->login_id1==request.login_id1 && sd->login_id2==request.login_id2 &&
        session_isValid(sd->fd) && !session[sd->fd]->flag.eof && session[fd]->client_addr==session[sd->fd]->client_addr &&
        (request.nonce_hi || request.nonce_lo) && sd->bank_ui.nonce_hi==request.nonce_hi && sd->bank_ui.nonce_lo==request.nonce_lo) {
        map_session_data* owner=nullptr;
        if (request.action==pn_market::InspectOwn) owner=sd;
        else if (request.action==pn_market::Search) {
            auto* iter=db_iterator(request.kind==pn_market::Vending?vending_getdb():buyingstore_getdb());
            for (auto* next=static_cast<map_session_data*>(dbi_first(iter));dbi_exists(iter);next=static_cast<map_session_data*>(dbi_next(iter))) {
                if (!next->market.published || next==sd || next->m!=sd->m || next->status.account_id<=request.cursor || (owner && next->status.account_id>=owner->status.account_id)) continue;
                auto candidate=pn_market_snapshot(*sd,next,request);
                if (candidate.count) owner=next;
            }
            dbi_destroy(iter);
        } else owner=map_id2sd(request.target_account?request.target_account:sd->market.last_target);
        if (owner && (owner->m!=sd->m || (!owner->market.published && owner!=sd))) owner=nullptr;
        reply=pn_market_snapshot(*sd,owner,request);
        if (request.action>=pn_market::SetPrice && owner && pn_market_shop(*owner,request.kind)) {
            const uint32 owner_account=owner->status.account_id;
            auto result=pn_market_action(*sd,*owner,request);
            owner=map_id2sd(owner_account); // Sold-out autotrade may free its session.
            reply=pn_market_snapshot(*sd,owner,request); reply.result=result; reply.wallet=sd->status.zeny;
        }
        if (request.action==pn_market::Search && owner) {reply.next_cursor=owner->status.account_id;reply.has_more=1;}
    }
    WFIFOHEAD(fd,sizeof(reply)); memcpy(WFIFOP(fd,0),&reply,sizeof(reply)); WFIFOSET(fd,sizeof(reply)); return true;
}
static int32 clif_parse_market_companion_session(int32 fd) {
    if (session[fd]->flag.eof) {do_close(fd);return 0;}
    if (RFIFOREST(fd)<2) return 0;
    if (!clif_parse_market_companion(fd)) set_eof(fd);
    return 0;
}
