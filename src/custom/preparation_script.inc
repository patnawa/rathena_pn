// Included by script.cpp after the owned-storage planner. GPL-3.0-or-later.
#include <custom/preparation_policy.hpp>
#include <custom/shop_state.hpp>
#include <limits>
namespace pn_preparation {
static int64 number(map_session_data& sd,const char* key,int i=0){const auto uid=reference_uid(add_str(key),i);return key[0]=='@'?pc_readreg(&sd,uid):pc_readregistry(&sd,uid);}
static bool set(map_session_data& sd,const char* key,int i,int64 n){const auto uid=reference_uid(add_str(key),i);if(key[0]=='@'){pc_setreg(&sd,uid,n);return true;}return pc_setregistry(&sd,uid,n);}
static std::string text(map_session_data& sd,const char* key,int i){const auto uid=reference_uid(add_str(key),i);const char* s=key[0]=='@'?pc_readregstr(&sd,uid):pc_readregistry_str(&sd,uid);return s?s:"";}
static bool put(map_session_data& sd,const char* key,int i,const std::string& s){const auto uid=reference_uid(add_str(key),i);if(key[0]=='@'){pc_setregstr(&sd,uid,s.c_str());return true;}return pc_setregistry_str(&sd,uid,s.c_str());}
static bool ready(map_session_data& sd){
    return sd.m>=0 && map_getmapflag(sd.m,MF_TOWN) && !map_getmapflag(sd.m,MF_PVP) &&
        !map_getmapflag(sd.m,MF_GVG) && !map_getmapflag(sd.m,MF_GVG_TE) && !map_getmapflag(sd.m,MF_BATTLEGROUND) &&
        !map[sd.m].instance_id && !pc_isdead(&sd) && !pc_cant_act2(&sd) && !sd.state.warping &&
        !sd.state.changemap && !sd.state.autotrade && !sd.chatID && chrif_isconnected() && sd.vars_ok;
}
static bool eligible(map_session_data& sd,int i,bool sale){
    auto& it=sd.inventory.u.items_inventory[i];auto* data=sd.inventory_data[i];
    return data && data==item_db.find(it.nameid).get() && !protected_item(it) && data->type!=IT_PETEGG &&
        (sale ? (data->type==IT_ETC && data->value_sell>0 && itemdb_cansell(&it,pc_get_group_level(&sd))) :
        (data->type!=IT_WEAPON && data->type!=IT_ARMOR && data->type!=IT_SHADOWGEAR && data->type!=IT_AMMO && itemdb_canstore(&it,pc_get_group_level(&sd))));
}
static bool junk(map_session_data& sd,t_itemid id){for(int i=0;i<20;++i)if(number(sd,"PNJunk",i)==id)return true;return false;}
static int64 price(map_session_data& sd,int i){return static_cast<int64>(pc_modifysellvalue(&sd,sd.inventory_data[i]->value_sell))*sd.inventory.u.items_inventory[i].amount;}
// Preview state belongs to the attached NPC interaction. The serialized exact
// item snapshot is checked after confirmation; caller-controlled IDs cannot sell
// an item that was not shown in the preview.
static void clear_preview(map_session_data& sd){set(sd,"@PNPrepCount",0,0);set(sd,"@PNPrepTotal",0,0);set(sd,"@PNPrepAction",0,0);}
static std::string exact(const item& it){static const char hex[]="0123456789abcdef";std::string out;const auto* b=reinterpret_cast<const unsigned char*>(&it);for(size_t i=0;i<sizeof(it);++i){out+=hex[b[i]>>4];out+=hex[b[i]&15];}return out;}
}

// save=0 applies, save=1 records the current equipped items and all loot filters.
BUILDIN_FUNC(pnpreppreset){
    map_session_data* sd=nullptr;int result=0;const int slot=script_getnum(st,2),save=script_getnum(st,3);
    if(!script_rid2sd(sd) || slot<0 || slot>=3 || (save!=0 && save!=1) || !pn_preparation::ready(*sd))goto done;
    if(save){
        std::vector<std::pair<std::string,uint32>> gear;
        for(int i=0;i<MAX_INVENTORY;++i){const auto& it=sd->inventory.u.items_inventory[i];if(it.nameid && it.equip){
            if(it.expire_time || it.equipSwitch || gear.size()>=EQI_MAX)goto done;
            const auto fingerprint=pn_preparation::identity(it);int matches=0;
            for(const auto& candidate:sd->inventory.u.items_inventory)if(candidate.nameid && pn_preparation::identity(candidate)==fingerprint)++matches;
            if(matches!=1)goto done;gear.emplace_back(fingerprint,it.equip);
        }}
        if(gear.empty())goto done;
        pn_preparation::set(*sd,"PNPrepSaved",slot,0);
        for(int i=0;i<EQI_MAX;++i){const int key=slot*EQI_MAX+i;if(!pn_preparation::put(*sd,"PNPrepGear$",key,i<gear.size()?gear[i].first:"") || !pn_preparation::set(*sd,"PNPrepMask",key,i<gear.size()?gear[i].second:0))goto done;}
        pn_preparation::set(*sd,"PNPrepRate",slot,sd->state.autoloot);
        pn_preparation::set(*sd,"PNPrepType",slot,sd->state.autoloottype);
        for(int i=0;i<AUTOLOOTITEM_SIZE;++i)pn_preparation::set(*sd,"PNPrepLoot",slot*AUTOLOOTITEM_SIZE+i,sd->state.autolootid[i]);
        result=pn_preparation::set(*sd,"PNPrepSaved",slot,1)?1:0;
    }else{
        if(pn_preparation::number(*sd,"PNPrepSaved",slot)!=1 || sd->sc.cant.equip || sd->sc.cant.unequip || sd->sc.getSCE(SC_PYROCLASTIC) || DIFF_TICK(sd->canequip_tick,gettick())>0)goto done;
        std::vector<std::pair<int,uint32>> target,original;
        uint32 occupied=0;
        for(int j=0;j<EQI_MAX;++j){const auto fingerprint=pn_preparation::text(*sd,"PNPrepGear$",slot*EQI_MAX+j);if(fingerprint.empty())continue;
            const auto mask=static_cast<uint32>(pn_preparation::number(*sd,"PNPrepMask",slot*EQI_MAX+j));int found=-1;
            for(int i=0;i<MAX_INVENTORY;++i)if(sd->inventory.u.items_inventory[i].nameid && pn_preparation::identity(sd->inventory.u.items_inventory[i])==fingerprint){if(found>=0)goto done;found=i;}
            if(found<0 || !mask || (occupied&mask) || !sd->inventory_data[found] || sd->inventory.u.items_inventory[found].expire_time || sd->inventory.u.items_inventory[found].equipSwitch || (sd->inventory_data[found]->type!=IT_AMMO && pc_isequip(sd,found)!=ITEM_EQUIP_ACK_OK) || (mask&~static_cast<uint32>(pc_equippoint(sd,found))))goto done;
            occupied|=mask;target.emplace_back(found,mask);
        }
        if(target.empty())goto done;
        for(int i=0;i<MAX_INVENTORY;++i)if(sd->inventory.u.items_inventory[i].equip)original.emplace_back(i,static_cast<uint32>(sd->inventory.u.items_inventory[i].equip));
        auto order=[&](const auto& a,const auto& b){auto rank=[&](int i){return sd->inventory_data[i]->type==IT_WEAPON?0:sd->inventory_data[i]->type==IT_AMMO?2:1;};return rank(a.first)<rank(b.first);};
        std::stable_sort(target.begin(),target.end(),order);std::stable_sort(original.begin(),original.end(),order);
        bool success=true;
        for(const auto& gear:original)if(sd->inventory.u.items_inventory[gear.first].equip && !pc_unequipitem(sd,gear.first,1)){success=false;break;}
        if(success)for(const auto& gear:target)if(pc_isequip(sd,gear.first)!=ITEM_EQUIP_ACK_OK || !pc_equipitem(sd,gear.first,gear.second)){success=false;break;}
        if(success)for(const auto& gear:target)if(sd->inventory.u.items_inventory[gear.first].equip!=gear.second){success=false;break;}
        if(!success){for(int i=0;i<MAX_INVENTORY;++i)if(sd->inventory.u.items_inventory[i].equip)pc_unequipitem(sd,i,1);for(const auto& gear:original)pc_equipitem(sd,gear.first,gear.second);goto done;}
        sd->state.autoloot=static_cast<uint16>(std::clamp<int64>(pn_preparation::number(*sd,"PNPrepRate",slot),0,10000));
        sd->state.autoloottype=static_cast<uint16>(pn_preparation::number(*sd,"PNPrepType",slot));sd->state.autolooting=false;
        for(int i=0;i<AUTOLOOTITEM_SIZE;++i){const auto id=pn_preparation::number(*sd,"PNPrepLoot",slot*AUTOLOOTITEM_SIZE+i);sd->state.autolootid[i]=id>0 && id<=UINT32_MAX && item_db.find(static_cast<t_itemid>(id))?id:0;if(sd->state.autolootid[i])sd->state.autolooting=true;}
        result=1;
    }
done:script_pushint(st,result);return SCRIPT_CMD_SUCCESS;
}

// Set a target quantity for one ordinary retail supply; 0 removes the entry.
BUILDIN_FUNC(pnprepsupply){
    map_session_data* sd=nullptr;bool ok=false;const int slot=script_getnum(st,2),row=script_getnum(st,3);const int64 id=script_getnum64(st,4),amount=script_getnum64(st,5);
    if(script_rid2sd(sd) && pn_preparation::ready(*sd) && slot>=0 && slot<3 && row>=0 && row<8 && id>0 && id<=UINT32_MAX && pn_preparation::supply(id) && amount>=0 && amount<=1000){
        bool duplicate=false;for(int i=0;i<8;++i)if(i!=row && pn_preparation::number(*sd,"PNPrepSupplyId",slot*8+i)==id && pn_preparation::number(*sd,"PNPrepQuantity",slot*8+i)>0)duplicate=true;
        if(!duplicate)ok=pn_preparation::set(*sd,"PNPrepSupplyId",slot*8+row,amount?id:0) && pn_preparation::set(*sd,"PNPrepQuantity",slot*8+row,amount);
    }script_pushint(st,ok);return SCRIPT_CMD_SUCCESS;
}

// action 0 previews a restock; action 1 rechecks exactly that price/items and
// submits an immutable inventory + wallet transaction. No immediate delivery.
BUILDIN_FUNC(pnpreprestock){
    map_session_data* sd=nullptr;int result=-1;const int slot=script_getnum(st,2),commit=script_getnum(st,3);
    if(!script_rid2sd(sd) || slot<0 || slot>=3 || (commit!=0 && commit!=1) || !pn_preparation::ready(*sd))goto done;
    {
        std::vector<pn_shop::Grant> grants;int64 total=0;int count=0;std::string signature;
        for(int row=0;row<8;++row){const int key=slot*8+row;const auto id=pn_preparation::number(*sd,"PNPrepSupplyId",key),wanted=pn_preparation::number(*sd,"PNPrepQuantity",key);if(!id || !wanted)continue;
            if(id<1 || id>UINT32_MAX || wanted<0 || wanted>1000 || !pn_preparation::supply(id))goto done;
            auto data=item_db.find(static_cast<t_itemid>(id));if(!data || data->value_buy<1 || data->flag.autoequip || !itemdb_isstackable2(data.get()))goto done;
            int64 have=0;for(const auto& it:sd->inventory.u.items_inventory)if(it.nameid==id && it.amount>0)have+=it.amount;
            const int64 needed=std::max<int64>(0,wanted-have);if(!needed)continue;
            const auto cost=static_cast<int64>(data->value_buy)*needed;if(cost<1 || cost>INT64_MAX-total)goto done;total+=cost;
            pn_shop::Grant grant{};grant.nameid=id;grant.amount=needed;grant.price=data->value_buy;grants.push_back(grant);
            signature+=std::to_string(id)+","+std::to_string(needed)+","+std::to_string(data->value_buy)+";";
            pn_preparation::set(*sd,"@PNPrepItem",count,id);pn_preparation::set(*sd,"@PNPrepAmount",count,needed);pn_preparation::set(*sd,"@PNPrepPrice",count,cost);++count;
        }
        if(!commit){pn_preparation::set(*sd,"@PNPrepCount",0,count);pn_preparation::set(*sd,"@PNPrepTotal",0,total);pn_preparation::set(*sd,"@PNPrepSlot",0,slot);pn_preparation::put(*sd,"@PNPrepRestock$",0,signature);result=count;goto done;}
        if(pn_preparation::number(*sd,"@PNPrepSlot")!=slot || pn_preparation::text(*sd,"@PNPrepRestock$",0)!=signature || pn_preparation::number(*sd,"@PNPrepTotal")!=total || grants.empty() || total>sd->status.zeny)goto done;
        auto request=pn_shop_request(*sd,pn_shop::Asset);request->wallet_after-=total;
        result=pn_shop_begin(*sd,request,grants)?1:-1;pn_preparation::put(*sd,"@PNPrepRestock$",0,"");
    }
done:script_pushint(st,result);return SCRIPT_CMD_SUCCESS;
}

// Junk is an explicit character allowlist, never every miscellaneous item.
BUILDIN_FUNC(pnprepjunk){
    map_session_data* sd=nullptr;int result=0;const int row=script_getnum(st,2);const int64 id=script_getnum64(st,3);
    if(script_rid2sd(sd) && pn_preparation::ready(*sd) && row>=0 && row<20 && id>=0 && id<=UINT32_MAX){auto data=item_db.find(static_cast<t_itemid>(id));if(!id || (data && data->type==IT_ETC && data->value_sell>0))result=pn_preparation::set(*sd,"PNJunk",row,id);}
    script_pushint(st,result);return SCRIPT_CMD_SUCCESS;
}

// mode 1 deposits ordinary items, mode 2 sells explicitly listed junk.
BUILDIN_FUNC(pnprepinventory){
    map_session_data* sd=nullptr;int result=-1;const int mode=script_getnum(st,2),commit=script_getnum(st,3);
    if(!script_rid2sd(sd) || (mode!=1 && mode!=2) || (commit!=0 && commit!=1) || !pn_preparation::ready(*sd) || !pc_can_give_items(sd))goto done;
    if(!commit){
        pn_preparation::clear_preview(*sd);int count=0;int64 total=0;
        for(int i=0;i<MAX_INVENTORY;++i)if(pn_preparation::eligible(*sd,i,mode==2) && (mode!=2 || pn_preparation::junk(*sd,sd->inventory.u.items_inventory[i].nameid))){
            const auto& it=sd->inventory.u.items_inventory[i];const auto value=mode==2?pn_preparation::price(*sd,i):0;
            if(value<0 || value>INT64_MAX-total)goto done;total+=value;
            pn_preparation::set(*sd,"@PNPrepIndex",count,i);pn_preparation::set(*sd,"@PNPrepItem",count,it.nameid);pn_preparation::set(*sd,"@PNPrepAmount",count,it.amount);pn_preparation::set(*sd,"@PNPrepPrice",count,value);pn_preparation::put(*sd,"@PNPrepExact$",count,pn_preparation::exact(it));++count;
        }
        pn_preparation::set(*sd,"@PNPrepCount",0,count);pn_preparation::set(*sd,"@PNPrepTotal",0,total);pn_preparation::set(*sd,"@PNPrepAction",0,mode);result=count;
    }else{
        const int count=pn_preparation::number(*sd,"@PNPrepCount");if(pn_preparation::number(*sd,"@PNPrepAction")!=mode || count<1 || count>MAX_INVENTORY)goto done;
        std::vector<int> indices;std::set<int> seen;int64 total=0;
        for(int row=0;row<count;++row){const int i=pn_preparation::number(*sd,"@PNPrepIndex",row);if(i<0 || i>=MAX_INVENTORY || !seen.insert(i).second || pn_preparation::exact(sd->inventory.u.items_inventory[i])!=pn_preparation::text(*sd,"@PNPrepExact$",row) || !pn_preparation::eligible(*sd,i,mode==2) || (mode==2 && !pn_preparation::junk(*sd,sd->inventory.u.items_inventory[i].nameid)))goto done;
            const auto value=mode==2?pn_preparation::price(*sd,i):0;if(value<0 || value>INT64_MAX-total)goto done;total+=value;indices.push_back(i);
        }
        if(total!=pn_preparation::number(*sd,"@PNPrepTotal"))goto done;
        if(mode==2){
            if(total<1 || total>INT64_MAX-sd->status.zeny)goto done;
            auto request=pn_shop_request(*sd,pn_shop::ItemUse);memcpy(request->items,sd->inventory.u.items_inventory,sizeof(request->items));int64 weight=sd->weight;
            for(int i:indices){const auto& it=request->items[i];weight-=static_cast<int64>(sd->inventory_data[i]->weight)*it.amount;request->items[i]={};}
            if(weight<0 || weight>UINT32_MAX)goto done;request->wallet_after+=total;
            result=pn_shop_submit(*sd,request,{},static_cast<uint32>(weight))?count:-1;
        }else{
            auto& storage=sd->storage;
            if(storage.type!=TABLE_STORAGE || storage.id!=sd->status.account_id || storage.stor_id!=0 || storage.lock || !storage_page_available(*sd,0) || storage.max_amount<1 || storage.max_amount>MAX_STORAGE)goto done;
            // Simulate native stack/capacity checks for every item before the
            // first transfer. No UI yield occurs between simulation and commit.
            auto simulated=std::make_unique<s_storage>(storage);
            size_t occupied=0;for(const auto& it:storage.u.items_storage)if(it.nameid)++occupied;
            if(occupied!=storage.amount || storage.amount>storage.max_amount)goto done;
            for(int i:indices){const auto& it=sd->inventory.u.items_inventory[i];auto* data=sd->inventory_data[i];int target=-1;
                if(itemdb_isstackable2(data))for(int j=0;j<simulated->max_amount;++j)if(compare_item(&simulated->u.items_storage[j],const_cast<item*>(&it))){target=j;break;}
                if(target<0){if(simulated->amount>=simulated->max_amount)goto done;for(int j=0;j<simulated->max_amount;++j)if(!simulated->u.items_storage[j].nameid){target=j;simulated->u.items_storage[j]=it;simulated->u.items_storage[j].amount=0;++simulated->amount;break;}}
                if(target<0 || it.amount>MAX_AMOUNT-simulated->u.items_storage[target].amount || (data->stack.storage && it.amount>data->stack.amount-simulated->u.items_storage[target].amount))goto done;
                simulated->u.items_storage[target].amount+=it.amount;
            }
            const bool put=storage.state.put;storage.state.put=1;
            storage_batch_begin(*sd);for(int i:indices)storage_storageadd(sd,&storage,i,sd->inventory.u.items_inventory[i].amount);storage_batch_end(*sd,storage);storage.state.put=put;result=count;
        }
        pn_preparation::clear_preview(*sd);
    }
done:script_pushint(st,result);return SCRIPT_CMD_SUCCESS;
}

// Search all accessible personal pages; owned live pages replace SQL snapshots.
BUILDIN_FUNC(pnprepstorage){
    map_session_data* sd=nullptr;int result=-1;const auto query=pn_planner::lower(script_getstr(st,2));const int page=script_getnum(st,3);
    if(!script_rid2sd(sd) || query.empty() || query.size()>80 || page<0 || page>10000 || !pn_preparation::ready(*sd))goto done;
    {
        pn_planner::Counts wanted,inventory,storage;
        for(const auto& entry:item_db){const auto& data=*entry.second;if(pn_planner::lower(data.ename).find(query)!=std::string::npos || pn_planner::lower(data.name).find(query)!=std::string::npos || std::to_string(entry.first)==query)wanted[entry.first]=0;}
        // Bounded SQL IN lists. Pagination refers to matching item IDs, then
        // only storage holdings on the requested page are displayed.
        int skip=page*80;pn_planner::Counts selected;for(const auto& it:wanted){if(skip){--skip;continue;}if(selected.size()==80)break;selected[it.first]=0;}
        pn_preparation::set(*sd,"@PNPrepMore",0,wanted.size()>static_cast<size_t>(page*80+80));
        if(selected.empty()){result=0;goto done;}if(!pn_planner::holdings(*sd,selected,inventory,storage))goto done;
        result=0;for(const auto& it:storage)if(it.second){pn_preparation::set(*sd,"@PNPrepItem",result,it.first);pn_preparation::set(*sd,"@PNPrepAmount",result,it.second);++result;}
    }
done:script_pushint(st,result);return SCRIPT_CMD_SUCCESS;
}
