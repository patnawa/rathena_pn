// Included by map/intif.cpp: immutable snapshots, retry until durable receipt.
#include <custom/pair_state.hpp>
#include <common/random.hpp>
#include <map/trade.hpp>
void vending_pair_completed(map_session_data& buyer,map_session_data& seller);
void buyingstore_pair_completed(map_session_data& seller,map_session_data& buyer);
struct pn_pair_rollback {
    struct Side {s_storage inventory{},cart{};uint32 weight=0;int32 cart_weight=0,cart_num=0,vend_num=0;s_vending vending[MAX_VENDING]{};s_buyingstore buyingstore{};} side[2];
};

static void pn_pair_request(const pn_pair::Commit& request) {
    if(CheckForCharServer() || !chrif_isconnected())return;
    for (const auto& side : request.side) {
        auto* sd = map_id2sd(side.account_id);
        if (!sd || sd->status.char_id != side.char_id) return;
        if (!sd->registry_saves.empty()) { intif_registry_replay(sd); return; }
    }
    WFIFOHEAD(inter_fd,sizeof(request));memcpy(WFIFOP(inter_fd,0),&request,sizeof(request));WFIFOSET(inter_fd,sizeof(request));
}
static TIMER_FUNC(pn_pair_retry) {
    auto* a=map_id2sd(id);
    if(!a || !a->pair_commit.pending || !a->pair_commit.request ||
       a->pair_commit.request->sequence!=static_cast<uint64>(data))return 0;
    pn_pair_request(*a->pair_commit.request);
    add_timer(tick+1000,pn_pair_retry,id,data);return 0;
}
bool pn_pair_begin(map_session_data& a,map_session_data& b,pn_pair::Kind kind,int64_t fee) {
    if(&a==&b || a.status.account_id==b.status.account_id || pc_transaction_pending(&a) || pc_transaction_pending(&b) ||
       a.mail.pending_zeny || b.mail.pending_zeny || a.mail.pending_slots || b.mail.pending_slots ||
       !chrif_isconnected() || fee<0 || (kind!=pn_pair::Trade && kind!=pn_pair::Vending && kind!=pn_pair::Buying))return false;
    if(kind==pn_pair::Vending && (strcmp(vendings_table,"vendings") || strcmp(vending_items_table,"vending_items")))return false;
    if(kind==pn_pair::Buying && (strcmp(buyingstores_table,"buyingstores") || strcmp(buyingstore_items_table,"buyingstore_items")))return false;
    if (intif_saveregistry(&a) < 0 || intif_saveregistry(&b) < 0) return false;
    static uint64 sequence=0;
    auto request=std::make_shared<pn_pair::Commit>();request->length=sizeof(*request);
    auto rollback=std::make_shared<pn_pair_rollback>();
    request->kind=kind;request->fee=fee;if(++sequence==0)++sequence;request->sequence=sequence;
    map_session_data* players[]={&a,&b};
    for(size_t i=0;i<2;++i) {
        auto& sd=*players[i];auto& s=request->side[i];
        if(!(sd.bank_ui.nonce_hi|sd.bank_ui.nonce_lo)) {
            sd.bank_ui.nonce_hi=(static_cast<uint64>(rnd())<<32)|static_cast<uint32>(rnd());
            sd.bank_ui.nonce_lo=(static_cast<uint64>(rnd())<<32)|static_cast<uint32>(rnd());
            if(!(sd.bank_ui.nonce_hi|sd.bank_ui.nonce_lo))sd.bank_ui.nonce_lo=1;
        }
        s.account_id=sd.status.account_id;s.char_id=sd.status.char_id;
        s.nonce_hi=sd.bank_ui.nonce_hi;s.nonce_lo=sd.bank_ui.nonce_lo;
        s.wallet_before=sd.status.zeny;s.bank_before=sd.bank_vault;
        auto& old=rollback->side[i];old.inventory=sd.inventory;old.cart=sd.cart;old.weight=sd.weight;
        old.cart_weight=sd.cart_weight;old.cart_num=sd.cart_num;old.vend_num=sd.vend_num;memcpy(old.vending,sd.vending,sizeof(old.vending));
        old.buyingstore=sd.buyingstore;
        sd.pair_commit.pending=sd.pair_commit.applying=true;sd.pair_commit.request=request;sd.pair_commit.rollback=rollback;
    }
    return true;
}
bool pn_pair_abort(map_session_data& a,map_session_data& b) {
    // Legal only before submit: once SQL can see the snapshot, retries must
    // resolve its receipt and a local rollback could duplicate assets.
    auto request=a.pair_commit.request;auto rollback=a.pair_commit.rollback;
    if(!request || !rollback || request!=b.pair_commit.request || !a.pair_commit.applying || !b.pair_commit.applying)return false;
    map_session_data* players[]={&a,&b};
    for(size_t i=0;i<2;++i) {
        auto& sd=*players[i];const auto& old=rollback->side[i];
        sd.inventory=old.inventory;sd.cart=old.cart;sd.weight=old.weight;sd.cart_weight=old.cart_weight;sd.cart_num=old.cart_num;
        sd.vend_num=old.vend_num;memcpy(sd.vending,old.vending,sizeof(old.vending));
        sd.buyingstore=old.buyingstore;
        sd.status.zeny=request->side[i].wallet_before;
        if(sd.bank_vault!=request->side[i].bank_before)pc_setparam(&sd,SP_BANK_VAULT,request->side[i].bank_before);
        sd.market.sold_count=sd.market.bought_count=0;pc_setinventorydata(sd);pc_setequipindex(&sd);
        sd.pair_commit={};clif_inventorylist(&sd);if(pc_iscarton(&sd))clif_cartlist(&sd);
        clif_updatestatus(sd,SP_ZENY);clif_updatestatus(sd,SP_WEIGHT);clif_updatestatus(sd,SP_CARTINFO);
        status_calc_pc(&sd,SCO_NONE);
    }
    return true;
}
void pn_pair_submit(map_session_data& a,map_session_data& b) {
    static_assert(MAX_VENDING<=pn_pair::listing_capacity,"Widen pair listing capacity before increasing native vending slots");
    static_assert(MAX_BUYINGSTORE_SLOTS<=pn_pair::listing_capacity,"Widen pair listing capacity before increasing buying slots");
    auto request=a.pair_commit.request;
    if(!request || request!=b.pair_commit.request)return;
    map_session_data* players[]={&a,&b};
    for(size_t i=0;i<2;++i) {
        auto& sd=*players[i];auto& s=request->side[i];
        s.wallet_after=sd.status.zeny;s.bank_after=sd.bank_vault;
        if(i==1 && request->kind==pn_pair::Vending)memcpy(s.items,sd.cart.u.items_cart,sizeof(item)*MAX_CART);
        else memcpy(s.items,sd.inventory.u.items_inventory,sizeof(item)*MAX_INVENTORY);
        sd.pair_commit.applying=false;
        sd.pair_commit.rollback.reset();
    }
    if(request->kind==pn_pair::Vending) {
        request->vending_id=b.vender_id;request->listing_count=b.vend_num;
        for(uint32 i=0;i<request->listing_count && i<pn_pair::listing_capacity;++i) {
            auto& row=request->listings[i];const auto& source=b.vending[i];
            row.cart_id=b.cart.u.items_cart[source.index].id;row.amount=source.amount;row.price=source.value;
        }
    }
    if(request->kind==pn_pair::Buying) {
        request->vending_id=b.buyer_id;request->buying_budget=b.buyingstore.zenylimit;request->listing_count=b.buyingstore.slots;
        for(uint32 i=0;i<request->listing_count && i<pn_pair::listing_capacity;++i) {
            auto& row=request->listings[i];const auto& source=b.buyingstore.items[i];
            row.cart_id=source.nameid;row.amount=source.amount;row.price=source.price;
        }
    }
    pn_pair_request(*request);
    add_timer(gettick()+1000,pn_pair_retry,a.status.account_id,static_cast<intptr_t>(request->sequence));
}
static void intif_parse_PairCommitted(int32 fd) {
    pn_pair::Ack reply;memcpy(&reply,RFIFOP(fd,0),sizeof(reply));
    auto* a=map_id2sd(reply.account_id);
    if(!a || !a->pair_commit.pending || !a->pair_commit.request || reply.committed!=1)return;
    auto request=a->pair_commit.request;const auto& first=request->side[0];
    if(a->status.char_id!=reply.char_id || first.account_id!=reply.account_id ||
       first.char_id!=reply.char_id || first.nonce_hi!=reply.nonce_hi || first.nonce_lo!=reply.nonce_lo || request->sequence!=reply.sequence)return;
    auto* b=map_id2sd(request->side[1].account_id);
    if(!b || !b->pair_commit.pending || b->pair_commit.request!=request ||
       b->status.char_id!=request->side[1].char_id || b->bank_ui.nonce_hi!=request->side[1].nonce_hi ||
       b->bank_ui.nonce_lo!=request->side[1].nonce_lo)return;
    a->pair_commit={};b->pair_commit={};
    if(request->kind==pn_pair::Trade) {
        trade_pair_completed(*a,*b);
        chrif_save(a,CSAVE_NORMAL|CSAVE_INVENTORY);chrif_save(b,CSAVE_NORMAL|CSAVE_INVENTORY);
    } else if(request->kind==pn_pair::Vending) {
        chrif_save(a,CSAVE_NORMAL|CSAVE_INVENTORY);chrif_save(b,CSAVE_NORMAL|CSAVE_CART);
        vending_pair_completed(*a,*b); // May retire a sold-out autotrader.
    } else {
        chrif_save(a,CSAVE_NORMAL|CSAVE_INVENTORY);chrif_save(b,CSAVE_NORMAL|CSAVE_INVENTORY);
        buyingstore_pair_completed(*a,*b); // May retire a fulfilled autotrader.
    }
}
