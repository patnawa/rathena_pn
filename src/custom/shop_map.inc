// Included by npc.cpp. Planning never invokes inventory, script, log or packet APIs.
#include <custom/shop_state.hpp>
#include <custom/retired_tokens.hpp>
#include <limits>

std::shared_ptr<pn_shop::Commit> pn_shop_request(const map_session_data& sd,uint32_t kind) {
    auto r=std::make_shared<pn_shop::Commit>();
    r->length=sizeof(*r);r->kind=kind;
    r->account_id=sd.status.account_id;r->char_id=sd.status.char_id;
    r->wallet_before=r->wallet_after=sd.status.zeny;
    r->cash_before=r->cash_after=sd.cashPoints;
    r->kafra_before=r->kafra_after=sd.kafraPoints;
    r->counter_before=r->counter_after=sd.status.uniqueitem_counter;
    return r;
}

bool pn_shop_begin(map_session_data& sd,std::shared_ptr<pn_shop::Commit> r,
    const std::vector<pn_shop::Grant>& grants,const uint32_t* requiredItems) {
    if(!r || grants.empty() || grants.size()>MAX_INVENTORY || pn_shop_stock_busy() || pc_transaction_pending(&sd) ||
       sd.mail.pending_zeny || sd.mail.pending_slots || sd.mail.pending_weight ||
       r->account_id!=sd.status.account_id || r->char_id!=sd.status.char_id ||
       r->wallet_before!=sd.status.zeny || r->cash_before!=sd.cashPoints ||
       r->kafra_before!=sd.kafraPoints || r->counter_before!=sd.status.uniqueitem_counter ||
       r->wallet_after<0 || r->wallet_after>r->wallet_before ||
       r->cash_after<0 || r->cash_after>r->cash_before ||
       r->kafra_after<0 || r->kafra_after>r->kafra_before)return false;
    memcpy(r->items,sd.inventory.u.items_inventory,sizeof(r->items));
    int64_t weight=sd.weight;
    if(requiredItems)for(int i=0;i<MAX_INVENTORY;++i)if(requiredItems[i]) {
        auto& it=r->items[i];auto data=item_db.find(it.nameid);
        if(!data || it.amount<=0 || static_cast<uint32_t>(it.amount)<requiredItems[i] || it.equip || it.equipSwitch ||
           data->type==IT_PETEGG || pet_db_search(it.nameid,PET_EGG) || it.card[0]==CARD0_PET)return false;
        weight-=static_cast<int64_t>(data->weight)*requiredItems[i];
        it.amount-=requiredItems[i];if(!it.amount)it={};
    }
    std::vector<pn_shop::Event> events;
    auto next_id=[&](uint64_t& id) {
        if(r->counter_after==UINT32_MAX)return false;
        id=(static_cast<uint64_t>(sd.status.char_id)<<32)|r->counter_after++;
        return true;
    };
    for(const auto& grant:grants) {
        auto data=item_db.find(grant.nameid);
        if(!data || !grant.amount || grant.amount>MAX_AMOUNT || pn_tokens::retired(grant.nameid) ||
           data->type==IT_PETEGG || pet_db_search(grant.nameid,PET_EGG))return false;
        bool separate=!itemdb_isstackable2(data.get()) || data->flag.guid;
        const uint32_t chunk=separate?1:grant.amount;
        if(data->stack.inventory && chunk>data->stack.amount)return false;
        for(uint32_t added=0;added<grant.amount;added+=chunk) {
            item output{};output.nameid=grant.nameid;output.identify=1;output.refine=grant.refine;
            if(separate) { uint64_t generated=0;if(!next_id(generated))return false;output.unique_id=generated; }
            int index=MAX_INVENTORY;
            if(itemdb_isstackable2(data.get()))for(int i=0;i<MAX_INVENTORY;++i) {
                const auto& current=r->items[i];
                if(current.nameid==output.nameid && !current.bound && !current.expire_time &&
                   current.unique_id==output.unique_id && !memcmp(current.card,output.card,sizeof(output.card))) {
                    if(i>=sd.status.inventory_slots || current.amount<0 || chunk>MAX_AMOUNT-current.amount ||
                       (data->stack.inventory && chunk>data->stack.amount-current.amount))return false;
                    index=i;break;
                }
            }
            if(index==MAX_INVENTORY) {
                for(int i=0;i<sd.status.inventory_slots && i<MAX_INVENTORY;++i)
                    if(!r->items[i].nameid){index=i;break;}
                if(index==MAX_INVENTORY)return false;
                r->items[index]=output;
            }
            r->items[index].amount+=chunk;
            weight+=static_cast<int64_t>(data->weight)*chunk;
            if(weight<0 || weight>sd.max_weight || weight>UINT32_MAX)return false;
            events.push_back({static_cast<int16_t>(index),chunk,grant.nameid,r->items[index].unique_id,
                data->flag.autoequip?data->equip:0,data->value_sell,grant.price});
        }
    }
    if(weight<0 || weight>sd.max_weight)return false;
    return pn_shop_submit(sd,std::move(r),std::move(events),static_cast<uint32_t>(weight));
}


bool pn_shop_stock_refresh(const pn_shop::Commit& r) {
    if(r.kind==pn_shop::Sale)return pn_shop_sale_refresh(r);
    for(uint32_t i=0;i<r.stock_count;++i) {
        const auto& stock=r.stocks[i];
        char escaped[sizeof(stock.name)*2+1];Sql_EscapeString(mmysql_handle,escaped,stock.name);
        const char* query=r.kind==pn_shop::Market?
            "SELECT amount FROM `market` WHERE name='%s' AND nameid=%u":
            "SELECT amount FROM `barter` WHERE name='%s' AND `index`=%u";
        if(Sql_Query(mmysql_handle,query,escaped,stock.key)!=SQL_SUCCESS)return false;
        const int row=Sql_NextRow(mmysql_handle);int64 amount=0;
        if(row==SQL_SUCCESS) {
            char* value=nullptr;
            if(Sql_GetData(mmysql_handle,0,&value,nullptr)!=SQL_SUCCESS || !value){Sql_FreeResult(mmysql_handle);return false;}
            amount=strtoll(value,nullptr,10);
        }else if(row!=SQL_NO_DATA){Sql_FreeResult(mmysql_handle);return false;}
        Sql_FreeResult(mmysql_handle);
        if(amount<0 || amount>INT32_MAX)return false;
        // Missing stock is fail-closed until the administrator reloads/recreates it.
        if(r.kind==pn_shop::Market) {
#if PACKETVER >= 20131223
            auto* nd=npc_name2id(stock.name);
            if(!nd || nd->subtype!=NPCTYPE_MARKETSHOP)continue;
            for(int j=0;j<nd->u.shop.count;++j) {
                auto& entry=nd->u.shop.shop_item[j];
                if(entry.nameid==stock.key)entry.qty=static_cast<int32>(amount);
            }
#endif
        }else if(r.kind==pn_shop::Barter) {
            auto barter=barter_db.find(stock.name);if(!barter)continue;
            auto found=barter->items.find(static_cast<uint16_t>(stock.key));
            if(found!=barter->items.end())found->second->stock=static_cast<uint32>(amount);
        }
    }
    return true;
}

// Called once after durable apply and unlocking, alongside delivery callbacks.
void pn_shop_committed_effects(map_session_data& sd,const pn_shop::Commit& request) {
    if(request.kind!=pn_shop::Market)return;
    int64 z=request.wallet_before-request.wallet_after;
    int32 skill=pc_checkskill(&sd,MC_DISCOUNT);
    if(battle_config.shop_exp<=0 || z<=0 || skill<=0)return;
    const uint16 sk_idx=skill_get_index(MC_DISCOUNT);
    if(sd.status.skill[sk_idx].flag>=SKILL_FLAG_REPLACED_LV_0)
        skill=sd.status.skill[sk_idx].flag-SKILL_FLAG_REPLACED_LV_0;
    if(skill<=0)return;
    z=static_cast<int64>(std::min<double>(MAX_ZENY,z*static_cast<double>(skill)*battle_config.shop_exp/10000.));
    if(z<1)z=1;
    pc_gainexp(&sd,nullptr,0,static_cast<int32>(z),0);
}
