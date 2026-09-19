// Included by script.cpp. Map-side preflight and the existing durable save lock.
static int32 pn_reserve_purchase(map_session_data& sd, int64 cost, uint32 id0, int32 qty0, uint32 id1, int32 qty1) {
    if (pc_transaction_pending(&sd) || !sd.vars_ok || !chrif_isconnected() || pc_isdead(&sd) ||
        sd.state.trading || sd.state.storage_flag || sd.state.mail_writing || sd.state.autotrade ||
        sd.state.warping || sd.state.changemap || sd.multi_storage.loading) return 0;
    int64 points=pc_readregistry(&sd,add_str("RESRVPTS"));
    if(cost<1 || cost>30000 || points<cost || qty0<1 || qty0>30000 || qty1<0 || qty1>30000 ||
        (!id1 != !qty1) || id0==id1 || sd.status.inventory_slots<1 || sd.status.inventory_slots>MAX_INVENTORY)return 0;
    uint32 ids[]={id0,id1};int32 amounts[]={qty0,qty1};
    s_storage planned=sd.inventory;int64 weight=sd.weight;
    for(int n=0;n<2;++n){
        if(!ids[n])continue;
        switch(ids[n]){case 501:case 504:case 505:case 516:case 526:case 602:case 607:case 608:case 645:case 656:case 657:case 2201:case 2226:case 2328:case 2403:break;default:return 0;}
        auto data=item_db.find(ids[n]);if(!data || data->flag.autoequip)return 0;
        weight+=static_cast<int64>(data->weight)*amounts[n];if(weight>sd.max_weight)return 0;
        bool stackable=itemdb_isstackable2(data.get());if(!stackable && amounts[n]!=1)return 0;
        int slot=MAX_INVENTORY;
        if(stackable){
            for(int i=0;i<MAX_INVENTORY;++i){auto& x=planned.u.items_inventory[i];
                if(x.nameid==ids[n] && !x.bound && !x.expire_time && !x.unique_id && !x.card[0] && !x.card[1] && !x.card[2] && !x.card[3]){slot=i;break;}
            }
        }
        if(slot==MAX_INVENTORY){for(int i=0;i<sd.status.inventory_slots;++i)if(!planned.u.items_inventory[i].nameid){slot=i;break;}}
        if(slot>=sd.status.inventory_slots)return 0;
        auto& target=planned.u.items_inventory[slot];
        if(amounts[n]>MAX_AMOUNT-target.amount || (data->stack.inventory && amounts[n]>data->stack.amount-target.amount))return 0;
        target.nameid=ids[n];target.amount+=amounts[n];
    }
    // No event-loop yield occurs from lock acquisition through request send.
    auto before=sd.inventory;auto old_weight=sd.weight;auto& state=sd.bank_ui;
    state.pending=state.applying=true;
    for(int n=0;n<2;++n)if(ids[n]){
        item entry={};entry.nameid=ids[n];entry.identify=1;
        if(pc_additem(&sd,&entry,amounts[n],LOG_TYPE_SCRIPT)!=ADDITEM_SUCCESS){
            sd.inventory=before;sd.weight=old_weight;state.pending=state.applying=false;
            pc_setinventorydata(sd);clif_inventorylist(&sd);clif_updatestatus(sd,SP_WEIGHT);return 0;
        }
    }
    if(!state.nonce_hi && !state.nonce_lo){state.nonce_hi=(static_cast<uint64>(rnd())<<32)|static_cast<uint32>(rnd());state.nonce_lo=(static_cast<uint64>(rnd())<<32)|static_cast<uint32>(rnd());if(!state.nonce_hi&&!state.nonce_lo)state.nonce_lo=1;}
    ++state.request_id;if(!state.request_id)++state.request_id;
    state.action=7;state.amount=cost;state.native_action=0;state.reserve_before=points;state.reserve_after=points-cost;
    for(int n=0;n<2;++n){state.reserve_item[n]=ids[n];state.reserve_quantity[n]=amounts[n];}
    state.bank_before=sd.bank_vault;state.wallet_before=sd.status.zeny;
    pc_setregistry(&sd,add_str("RESRVPTS"),points-cost);
    state.result=pn_bank::Saving;state.applying=false;state.last_action_tick=gettick();
    intif_bank_save(sd);return 1;
}
