// Included by npc.cpp. Planning never invokes inventory, script, log or packet APIs.
#include <custom/shop_state.hpp>
#include <custom/retired_tokens.hpp>
#include <common/runtime_metrics.hpp>
#include <limits>
#include <cerrno>

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

bool pn_shop_plan_inventory(const map_session_data& sd,pn_shop::Commit& request,
    const std::vector<pn_shop::Grant>& grants,const uint32_t* requiredItems,
    std::vector<pn_shop::Event>& events,uint32_t& final_weight) {
    auto* r=&request;
    if((grants.empty() && request.kind!=pn_shop::ItemUse && request.kind!=pn_shop::MailSend &&
        !(request.kind==pn_shop::Asset && request.mail_id && request.mail_zeny>0)) || grants.size()>MAX_INVENTORY ||
       sd.status.inventory_slots<1 || sd.status.inventory_slots>MAX_INVENTORY)return false;
    events.clear();
    r->pet_count=0;
    for(auto& change:r->pets)change={};
    r->pet_retire_count=0;
    for(auto& retired:r->retired_pets)retired={};
    r->counter_after=sd.status.uniqueitem_counter;
    memcpy(r->items,sd.inventory.u.items_inventory,sizeof(r->items));
    int64_t weight=sd.weight;
    if(requiredItems)for(int i=0;i<MAX_INVENTORY;++i)if(requiredItems[i]) {
        auto& it=r->items[i];auto data=item_db.find(it.nameid);
        if(!data || i>=sd.status.inventory_slots || it.amount<=0 || static_cast<uint32_t>(it.amount)<requiredItems[i] || it.equip || it.equipSwitch)return false;
        if(it.card[0]==CARD0_PET && r->kind!=pn_shop::MailSend) {
            if((data->type!=IT_PETEGG && !pet_db_search(it.nameid,PET_EGG)) || it.amount!=1 || requiredItems[i]!=1 ||
               it.card[1]>UINT16_MAX || it.card[2]>INT16_MAX || !(it.card[1]|it.card[2]) || r->kind==pn_shop::PetClaim)return false;
            auto& retired=r->retired_pets[r->pet_retire_count++];
            retired.pet_id=it.card[1]|(it.card[2]<<16);retired.egg_id=it.nameid;
        }
        weight-=static_cast<int64_t>(data->weight)*requiredItems[i];
        it.amount-=requiredItems[i];if(!it.amount)it={};
    }
    auto next_id=[&](uint64_t& id) {
        if(r->counter_after==UINT32_MAX)return false;
        id=(static_cast<uint64_t>(sd.status.char_id)<<32)|r->counter_after++;
        return true;
    };
    for(const auto& grant:grants) {
        auto data=item_db.find(grant.nameid);
        if(!data || !grant.amount || grant.amount>MAX_AMOUNT || pn_tokens::retired(grant.nameid))return false;
        item prototype=grant.prototype;
        if(!prototype.nameid){prototype.nameid=grant.nameid;prototype.identify=1;prototype.refine=grant.refine;}
        if(prototype.nameid!=grant.nameid || prototype.id || prototype.equip || prototype.equipSwitch)return false;
        prototype.amount=0;
        const bool pet=data->type==IT_PETEGG || pet_db_search(grant.nameid,PET_EGG);
        bool mailed_output=false;
        if(request.mail_id) {
            for(const auto& attachment:request.mail_items) {
                item expected=attachment;expected.amount=0;
                if(attachment.amount==grant.amount && !memcmp(&expected,&prototype,sizeof(item)))mailed_output=true;
            }
            if(!mailed_output)return false;
        }
        const bool mailed_identity=pet && mailed_output && prototype.card[0]==CARD0_PET;
        if(mailed_identity && (grant.amount!=1 || !(prototype.card[1]|prototype.card[2]) ||
           prototype.card[1]>UINT16_MAX || prototype.card[2]>INT16_MAX))return false;
        if(pet && !grant.pet_claim_id && !mailed_identity) {
            if(r->kind==pn_shop::PetClaim || grant.amount>MAX_INVENTORY-r->pet_count)return false;
            for(uint32_t n=0;n<grant.amount;++n) {
                auto& change=r->pets[r->pet_count];
                item egg=prototype;egg.amount=1;
                uint64_t generated=0;if(!next_id(generated))return false;egg.unique_id=generated;
                if(!pn_pet_describe_egg(egg,change.output))return false;
                ++r->pet_count;
                events.push_back({-1,1,grant.nameid,egg.unique_id,0,data->value_sell,grant.price});
            }
            continue;
        }
        if(grant.pet_claim_id && (!pet || grant.amount!=1 || r->kind!=pn_shop::PetClaim ||
            prototype.card[0]!=CARD0_PET || r->pet_count>=MAX_INVENTORY))return false;
        if(r->kind==pn_shop::PetClaim && !grant.pet_claim_id)return false;
        // A GUID identifies a stack too. Native pc_additem generates one ID per
        // stackable grant; it does not split that grant into single-item slots.
        bool separate=grant.pet_claim_id || prototype.expire_time || !itemdb_isstackable2(data.get());
        bool unique=separate || data->flag.guid;
        const uint32_t chunk=separate?1:grant.amount;
        if(data->stack.inventory && chunk>data->stack.amount)return false;
        for(uint32_t added=0;added<grant.amount;added+=chunk) {
            item output=prototype;
            if(unique && !grant.pet_claim_id && !mailed_identity && !(mailed_output && prototype.unique_id)) { uint64_t generated=0;if(!next_id(generated))return false;output.unique_id=generated; }
            int index=MAX_INVENTORY;
            if(!separate && itemdb_isstackable2(data.get()))for(int i=0;i<MAX_INVENTORY;++i) {
                const auto& current=r->items[i];
                if(current.nameid==output.nameid && current.bound==output.bound && current.expire_time==output.expire_time &&
                   current.unique_id==output.unique_id && current.identify==output.identify &&
                   current.refine==output.refine && current.attribute==output.attribute && current.enchantgrade==output.enchantgrade &&
                   !memcmp(current.option,output.option,sizeof(output.option)) && !memcmp(current.card,output.card,sizeof(output.card))) {
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
            if(grant.pet_claim_id) {
                auto& change=r->pets[r->pet_count++];change.claim_id=grant.pet_claim_id;
                change.inventory_index=index;change.output.egg=r->items[index];
            }
            weight+=static_cast<int64_t>(data->weight)*chunk;
            if(weight<0 || weight>sd.max_weight || weight>UINT32_MAX)return false;
            events.push_back({static_cast<int16_t>(index),chunk,grant.nameid,r->items[index].unique_id,
                data->flag.autoequip?data->equip:0,data->value_sell,grant.price});
        }
    }
    if(weight<0 || weight>sd.max_weight)return false;
    final_weight=static_cast<uint32_t>(weight);
    return true;
}

bool pn_shop_begin(map_session_data& sd,std::shared_ptr<pn_shop::Commit> r,
    const std::vector<pn_shop::Grant>& grants,const uint32_t* requiredItems) {
	if(pn_shop_queue_full())++pn_metrics::runtime.shop_busy_refusals;
    if(!r || pn_shop_queue_full() || pc_transaction_pending(&sd) ||
       sd.mail.pending_zeny || sd.mail.pending_slots || sd.mail.pending_weight ||
       r->account_id!=sd.status.account_id || r->char_id!=sd.status.char_id ||
       r->wallet_before!=sd.status.zeny || r->cash_before!=sd.cashPoints ||
       r->kafra_before!=sd.kafraPoints || r->counter_before!=sd.status.uniqueitem_counter ||
       r->wallet_after<0 || (!r->mail_id && r->wallet_after>r->wallet_before) ||
       r->cash_after<0 || r->cash_after>r->cash_before ||
       r->kafra_after<0 || r->kafra_after>r->kafra_before)return false;
    std::vector<pn_shop::Event> events;
    uint32_t weight=0;
    if(!pn_shop_plan_inventory(sd,*r,grants,requiredItems,events,weight))return false;
    return pn_shop_submit(sd,std::move(r),std::move(events),weight);
}

// Called once, before the first send of a queued immutable purchase. The SQL
// commit still checks stock, price and sale dates atomically. Subsequent retries
// must keep this admitted payload byte-for-byte, even after a lost reply.
bool pn_shop_stock_rebase(pn_shop::Commit& request) {
    for (uint32 i=0;i<request.stock_count;++i) {
        auto& stock=request.stocks[i];
        const int64 wanted=stock.before-stock.after;
        char escaped[sizeof(stock.name)*2+1];Sql_EscapeString(mmysql_handle,escaped,stock.name);
        int queried;
        if(request.kind==pn_shop::Sale)
            queried=Sql_Query(mmysql_handle,"SELECT amount FROM sales WHERE nameid=%u",stock.key);
        else if(request.kind==pn_shop::Market)
            queried=Sql_Query(mmysql_handle,"SELECT amount FROM market WHERE name='%s' AND nameid=%u",escaped,stock.key);
        else
            queried=Sql_Query(mmysql_handle,"SELECT amount FROM barter WHERE name='%s' AND `index`=%u",escaped,stock.key);
        if(queried!=SQL_SUCCESS)return false;
        const int row=Sql_NextRow(mmysql_handle);char* value=nullptr;
        int64 amount=0;
        if(row==SQL_SUCCESS) {
            if(Sql_GetData(mmysql_handle,0,&value,nullptr)!=SQL_SUCCESS || !value){Sql_FreeResult(mmysql_handle);return false;}
            char* end=nullptr;errno=0;amount=strtoll(value,&end,10);
            if(errno || !end || *end || amount<0 || amount>INT32_MAX){Sql_FreeResult(mmysql_handle);return false;}
        }
        Sql_FreeResult(mmysql_handle);
        if(row!=SQL_SUCCESS && row!=SQL_NO_DATA)return false;
        // Missing legacy rows retain the declared initial catalog stock. An
        // insufficient existing row keeps the old request so SQL records a
        // durable rejection without charging or delivering anything.
        if(row==SQL_SUCCESS && amount>=wanted) { stock.before=amount;stock.after=amount-wanted; }
    }
    return pn_shop::valid(request);
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

bool pn_pet_recover(map_session_data& sd) {
    if(!sd.vars_ok || !sd.state.pc_loaded || pc_transaction_pending(&sd) || pn_shop_queue_full())return false;
    if(Sql_Query(mmysql_handle,"SELECT id,egg FROM pn_pet_entitlements WHERE account_id=%u AND char_id=%u AND claimed=0 ORDER BY id LIMIT %u",
        sd.status.account_id,sd.status.char_id,static_cast<unsigned>(MAX_INVENTORY))!=SQL_SUCCESS)return false;
    std::vector<pn_shop::Grant> available;
    int row=SQL_NO_DATA;bool valid=true;
    while((row=Sql_NextRow(mmysql_handle))==SQL_SUCCESS) {
        char *identity=nullptr,*bytes=nullptr;size_t size=0;
        if(Sql_GetData(mmysql_handle,0,&identity,nullptr)!=SQL_SUCCESS || !identity || !*identity || *identity=='-' ||
           Sql_GetData(mmysql_handle,1,&bytes,&size)!=SQL_SUCCESS || !bytes || size!=sizeof(item)){valid=false;break;}
        char* end=nullptr;errno=0;uint64 id=strtoull(identity,&end,10);
        if(errno || !id || end==identity || *end){valid=false;break;}
        pn_shop::Grant grant{};memcpy(&grant.prototype,bytes,sizeof(item));
        grant.nameid=grant.prototype.nameid;grant.amount=1;grant.pet_claim_id=id;
        available.push_back(grant);
    }
    Sql_FreeResult(mmysql_handle);
    if(!valid || row!=SQL_NO_DATA || available.empty())return false;
    auto request=pn_shop_request(sd,pn_shop::PetClaim);
    std::vector<pn_shop::Grant> selected;
    std::vector<pn_shop::Event> events;uint32_t weight=0;
    // Claim only what fits now. The rest remains durable, without compensation
    // or pet deletion. The character transaction rechecks every claim identity.
    for(const auto& grant:available) {
        selected.push_back(grant);
        if(!pn_shop_plan_inventory(sd,*request,selected,nullptr,events,weight))selected.pop_back();
    }
    return !selected.empty() && pn_shop_begin(sd,request,selected);
}

static TIMER_FUNC(pn_pet_login_recovery) {
    auto* sd=map_id2sd(id);
    if(!sd || sd->status.char_id!=static_cast<uint32>(data))return 0;
    if(!sd->state.pc_loaded || pc_transaction_pending(sd) || pn_shop_queue_full()) {
        add_timer(tick+5000,pn_pet_login_recovery,id,data);return 0;
    }
    pn_pet_recover(*sd);
    return 0;
}
void pn_pet_schedule_recovery(map_session_data& sd) {
    add_timer(gettick()+1000,pn_pet_login_recovery,sd.status.account_id,sd.status.char_id);
}
