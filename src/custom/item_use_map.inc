// Included by pc.cpp after PcItemDeliveryScope. The scope keeps ordinary
// in-memory consumption/free-slot behavior, but defers externally visible asset
// effects until it knows whether this item script produced durable pet outputs.
struct PnItemUseState {
    item original[MAX_INVENTORY]{};
    uint32 weight=0,counter=0;
    int64 wallet=0;
    int32 cash=0,kafra=0,deferred_save=0;
    bool collecting=true,failed=false,save_requested=false;
    bool awaiting_capture=false,force_commit=false;
    uint64 capture_generation=0;
    uint32 capture_weight=0;
    std::shared_ptr<pn_shop::Commit> capture_assets;
    std::function<void(bool)> capture_result;
    std::vector<pn_shop::Grant> pets;
    std::vector<std::function<void()>> effects;
    std::vector<std::function<void()>> success_effects;
    std::function<void(bool)> use_ack;
    std::unique_ptr<PcItemDeliveryScope> delivery;
    ~PnItemUseState(){if(delivery)delivery->cancel();}
};
static std::shared_ptr<PnItemUseState> pn_item_use_state(const map_session_data* sd) {
    return sd?sd->item_use:nullptr;
}
bool pn_item_use_active(const map_session_data* sd) {
    const auto state=pn_item_use_state(sd);return state && state->collecting;
}
bool pn_item_use_defer(const map_session_data* sd,std::function<void()> effect) {
    auto state=pn_item_use_state(sd);if(!state || !state->collecting)return false;
    state->effects.push_back(std::move(effect));return true;
}
bool pn_item_use_ack(const map_session_data* sd,std::function<void(bool)> effect) {
    auto state=pn_item_use_state(sd);if(!state || !state->collecting)return false;
    state->use_ack=effect;state->effects.push_back([effect](){effect(true);});return true;
}
bool pn_item_use_capture_waiting(const map_session_data* sd) {
    const auto state=pn_item_use_state(sd);return state && state->awaiting_capture && !state->collecting;
}
void pn_item_use_abort(map_session_data& sd) {auto state=pn_item_use_state(&sd);if(state && state->collecting)state->failed=true;}
bool pn_item_use_success_defer(const map_session_data* sd,std::function<void()> effect) {
    auto state=pn_item_use_state(sd);if(!state || !state->collecting)return false;
    state->success_effects.push_back(std::move(effect));return true;
}
bool pn_item_use_save_defer(map_session_data* sd,int32 flags) {
    auto state=pn_item_use_state(sd);if(!state || !state->collecting)return false;
    state->save_requested=true;state->deferred_save|=flags;return true;
}
bool pn_item_use_world_allowed(map_session_data* sd) {
    auto state=pn_item_use_state(sd);if(!state || !state->collecting)return true;
    state->failed=true;
    clif_displaymessage(sd->fd,"This item combines pet rewards with an unsupported world action. Its inventory cost will be restored.");
    return false;
}
bool pn_item_use_collect_pet(map_session_data& sd,const item& egg,uint32 amount) {
    auto state=pn_item_use_state(&sd);if(!state || !state->collecting)return false;
    size_t count=0;for(const auto& grant:state->pets)count+=grant.amount;
    pn_pet::Output output{};item prototype=egg;prototype.amount=1;
    if(state->failed || !amount || amount>MAX_INVENTORY-count || !pn_pet_describe_egg(prototype,output)) {
        state->failed=true;return false;
    }
    pn_shop::Grant grant{};grant.nameid=egg.nameid;grant.amount=amount;grant.prototype=egg;
    state->pets.push_back(grant);return true;
}
static void pn_item_use_restore(map_session_data& sd,const PnItemUseState& state) {
    memcpy(sd.inventory.u.items_inventory,state.original,sizeof(state.original));
    sd.weight=state.weight;sd.status.zeny=state.wallet;sd.status.uniqueitem_counter=state.counter;
    sd.cashPoints=state.cash;sd.kafraPoints=state.kafra;
    pc_setaccountreg(&sd,add_str(CASHPOINT_VAR),state.cash);
    pc_setaccountreg(&sd,add_str(KAFRAPOINT_VAR),state.kafra);
    pc_setinventorydata(sd);pc_setequipindex(&sd);
}
static bool pn_item_use_begin(map_session_data& sd) {
    if(pn_item_use_state(&sd) || pc_transaction_pending(&sd) || sd.mail.pending_slots || sd.mail.pending_weight || sd.mail.pending_zeny)return false;
    auto state=std::make_shared<PnItemUseState>();
    memcpy(state->original,sd.inventory.u.items_inventory,sizeof(state->original));
    state->weight=sd.weight;state->wallet=sd.status.zeny;state->cash=sd.cashPoints;state->kafra=sd.kafraPoints;state->counter=sd.status.uniqueitem_counter;
    state->delivery=std::make_unique<PcItemDeliveryScope>(sd);sd.item_use=state;return true;
}
void pn_item_use_settled(map_session_data& sd,bool committed) {
    auto state=pn_item_use_state(&sd);if(!state)return;
    sd.item_use.reset();
    if(state->use_ack)state->use_ack(committed);
    if(state->capture_result)state->capture_result(committed);
    if(committed)for(auto& effect:state->success_effects)effect();
    if(!committed) {
        clif_inventorylist(&sd);clif_updatestatus(sd,SP_WEIGHT);clif_updatestatus(sd,SP_ZENY);
    }
}
static bool pn_item_use_finish(map_session_data& sd) {
    auto state=pn_item_use_state(&sd);if(!state)return true;
    if(!state->failed && state->awaiting_capture) {
        state->capture_assets=pn_shop_request(sd,pn_shop::ItemUse);state->capture_weight=sd.weight;
        memcpy(state->capture_assets->items,sd.inventory.u.items_inventory,sizeof(state->capture_assets->items));
        pn_item_use_restore(sd,*state);state->delivery->cancel();state->delivery.reset();state->effects.clear();state->collecting=false;
        return true;
    }
    if(!state->failed && state->pets.empty() && !state->force_commit) {
        state->collecting=false;sd.item_use.reset();
        for(auto& effect:state->effects)effect();
        for(auto& effect:state->success_effects)effect();
        state->delivery.reset();
        if(state->save_requested)chrif_save(&sd,state->deferred_save);
        return true;
    }
    std::shared_ptr<pn_shop::Commit> request;
    std::vector<pn_shop::Event> events;uint32 final_weight=0;
    if(!state->failed) {
        request=pn_shop_request(sd,pn_shop::ItemUse);
        if(!pn_shop_plan_inventory(sd,*request,state->pets,nullptr,events,final_weight))state->failed=true;
        else {
            request->wallet_before=state->wallet;request->cash_before=state->cash;request->kafra_before=state->kafra;request->counter_before=state->counter;
            for(const auto& old:state->original)if(old.nameid && old.card[0]==CARD0_PET) {
                bool retained=false;
                for(const auto& now:request->items)if(now.nameid==old.nameid && now.card[0]==CARD0_PET && now.card[1]==old.card[1] && now.card[2]==old.card[2]){retained=true;break;}
                if(retained)continue;
                if(old.amount!=1 || old.card[1]>UINT16_MAX || old.card[2]>INT16_MAX || !(old.card[1]|old.card[2])){state->failed=true;break;}
                request->retired_pets[request->pet_retire_count++]={static_cast<uint32>(old.card[1]|(old.card[2]<<16)),old.nameid};
            }
            // Ordinary outputs were staged by the native item APIs. Derive only
            // final delivered quantities; transient script additions are not ACKed.
            for(int i=0;i<MAX_INVENTORY;++i) {
                const auto& now=request->items[i];const auto& old=state->original[i];
                if(!now.nameid)continue;
                item current_key=now,original_key=old;current_key.id=original_key.id=0;current_key.amount=original_key.amount=0;
                const int before=!memcmp(&current_key,&original_key,sizeof(item))?old.amount:0;
                if(now.amount<=before)continue;
                auto data=item_db.find(now.nameid);if(!data){state->failed=true;break;}
                events.push_back({static_cast<int16>(i),static_cast<uint32>(now.amount-before),now.nameid,now.unique_id,
                    data->flag.autoequip?data->equip:0,data->value_sell,0});
            }
        }
    }
    // Crucial ordering: all prior assets are restored before the existing shop
    // submitter flushes its baseline. The consumed input never reaches SQL alone.
    pn_item_use_restore(sd,*state);
    state->delivery->cancel();state->delivery.reset();state->effects.clear();state->collecting=false;
    if(state->failed || !pn_shop_submit(sd,request,std::move(events),final_weight)) {
        pn_item_use_settled(sd,false);
        clif_displaymessage(sd.fd,"The pet reward could not be committed. Your inventory and currency cost were restored.");
        return false;
    }
    return true;
}
static TIMER_FUNC(pn_item_use_capture_timeout) {
    auto* sd=map_id2sd(id);const auto state=pn_item_use_state(sd);
    if(sd && state && state->capture_generation==static_cast<uint64>(data) && pn_item_use_capture_waiting(sd))pet_catch_cancel(*sd);
    return 0;
}
bool pn_item_use_batch_begin(map_session_data& sd) {return pn_item_use_begin(sd);}
bool pn_item_use_batch_finish(map_session_data& sd) {return pn_item_use_finish(sd);}
bool pn_item_use_capture_start(map_session_data& sd) {
    const bool external=!pn_item_use_active(&sd);
    if(external && !pn_item_use_begin(sd))return false;
    auto state=pn_item_use_state(&sd);if(state->awaiting_capture)return false;
    state->awaiting_capture=true;static uint64 generation=0;if(++generation==0)++generation;
    state->capture_generation=generation;
    add_timer(gettick()+30000,pn_item_use_capture_timeout,sd.status.account_id,static_cast<intptr_t>(generation));
    return !external || pn_item_use_finish(sd);
}
bool pn_item_use_capture_finish(map_session_data& sd,const item* egg,std::function<void(bool)> result) {
    auto state=pn_item_use_state(&sd);
    if(!pn_item_use_capture_waiting(&sd) || !state->capture_assets)return false;
    state->awaiting_capture=false;state->collecting=true;state->force_commit=true;state->capture_result=std::move(result);
    const auto& staged=*state->capture_assets;
    memcpy(sd.inventory.u.items_inventory,staged.items,sizeof(staged.items));sd.weight=state->capture_weight;
    sd.status.zeny=staged.wallet_after;sd.status.uniqueitem_counter=staged.counter_after;
    sd.cashPoints=staged.cash_after;sd.kafraPoints=staged.kafra_after;
    pc_setinventorydata(sd);pc_setequipindex(&sd);state->delivery=std::make_unique<PcItemDeliveryScope>(sd);
    if(egg && !pn_item_use_collect_pet(sd,*egg,1))state->failed=true;
    return pn_item_use_finish(sd);
}
