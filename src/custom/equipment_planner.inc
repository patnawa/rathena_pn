// GPL-3.0-or-later. Read-only views of loaded recipes and private holdings.
#include <set>
#include <cctype>

namespace pn_planner {
using Counts = std::map<t_itemid,int64>;
static void number(map_session_data& sd,const char* key,int index,int64 value) {
    pc_setreg(&sd,reference_uid(add_str(key),index),value);
}
static void text(map_session_data& sd,const char* key,int index,const std::string& value) {
    pc_setregstr(&sd,reference_uid(add_str(key),index),value.c_str());
}
static std::string lower(std::string value) {
    for(char& c:value)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return value;
}
static bool identifier(const char* value,size_t capacity) {
    const size_t length=strnlen(value,capacity);
    if(!length || length==capacity)return false;
    for(size_t i=0;i<length;++i)if(!std::isalnum(static_cast<unsigned char>(value[i])) && value[i]!='_')return false;
    return true;
}
static void add(Counts& counts,const item* items,size_t length,const Counts& wanted) {
    for(size_t i=0;i<length;++i)if(items[i].amount>0 && wanted.count(items[i].nameid))counts[items[i].nameid]+=items[i].amount;
}
// One SQL statement provides a consistent view of unloaded pages. Loaded pages
// use their owned live snapshot, including unsaved changes, never both copies.
// No caller-supplied account, character, table or item-expression reaches SQL.
static bool holdings(map_session_data& sd,const Counts& wanted,Counts& inventory,Counts& storage) {
    inventory.clear();storage.clear();
    if(pc_transaction_pending(&sd) || sd.multi_storage.loading || sd.multi_storage.pending ||
       sd.state.storage_flag || storage_db.empty() || wanted.empty() || wanted.size()>100)return false;
    add(inventory,sd.inventory.u.items_inventory,MAX_INVENTORY,wanted);
    std::string ids;
    for(const auto& cost:wanted){if(!cost.first)return false;if(!ids.empty())ids+=",";ids+=std::to_string(cost.first);}
    std::set<std::pair<std::string,uint32>> seen;
    std::vector<std::string> selects;
    for(const auto& entry:storage_db){
        const auto& table=*entry.second;
        if(!pn_storage::managed(entry.first) || !storage_page_available(sd,entry.first))continue;
        if(!identifier(table.table,sizeof(table.table)))return false;
        const bool character_owned=entry.first==pn_storage::character;
        const uint32 owner=character_owned?sd.status.char_id:sd.status.account_id;
        const char* owner_column=character_owned?"char_id":"account_id";
        // Ambiguous aliases are not counted twice, nor silently resolved to a
        // potentially different loaded page.
        if(!seen.emplace(lower(table.table),owner).second)return false;
        const s_storage* live=nullptr;
        for(const s_storage* candidate:{&sd.storage,&sd.premiumStorage})
            if(candidate->type==TABLE_STORAGE && candidate->id==owner && candidate->stor_id==entry.first){
                if(live)return false;live=candidate;
            }
        if(live){add(storage,live->u.items_storage,MAX_STORAGE,wanted);continue;}
        selects.push_back("SELECT nameid,amount FROM `"+std::string(table.table)+"` WHERE "+owner_column+"="+std::to_string(owner)+" AND nameid IN ("+ids+") AND amount>0");
    }
    if(seen.empty())return false;
    if(selects.empty())return true;
    std::string query="SELECT nameid,SUM(amount) FROM (";
    for(size_t i=0;i<selects.size();++i){if(i)query+=" UNION ALL ";query+=selects[i];}
    query+=") AS pn_owned_pages GROUP BY nameid";
    if(!qsmysql_handle || Sql_Query(qsmysql_handle,"%s",query.c_str())!=SQL_SUCCESS)return false;
    bool valid=true;int row;
    while((row=Sql_NextRow(qsmysql_handle))==SQL_SUCCESS){
        char* id=nullptr;char* amount=nullptr;char* end=nullptr;
        if(Sql_GetData(qsmysql_handle,0,&id,nullptr)!=SQL_SUCCESS || !id ||
           Sql_GetData(qsmysql_handle,1,&amount,nullptr)!=SQL_SUCCESS || !amount){valid=false;break;}
        errno=0;const auto nameid=strtoull(id,&end,10);
        if(errno || !end || *end || nameid>UINT32_MAX || !wanted.count(static_cast<t_itemid>(nameid))){valid=false;break;}
        errno=0;const auto count=strtoll(amount,&end,10);
        if(errno || !end || *end || count<0 || count>INT64_MAX-storage[nameid]){valid=false;break;}
        storage[nameid]+=count;
    }
    Sql_FreeResult(qsmysql_handle);
    if(!valid || row!=SQL_NO_DATA){inventory.clear();storage.clear();return false;}
    return true;
}
struct Route { const char* shop;const char* npc; };
static const Route routes[]={
#include "planner_routes.inc"
};
static npc_data* entrance(const s_npc_barter& shop){
    auto* direct=map_id2nd(shop.npcid);
    if(direct && direct->m>=0)return direct;
    for(const auto& route:routes)if(shop.name==route.shop){auto* nd=npc_name2id(route.npc);if(nd && nd->m>=0)return nd;}
    return nullptr;
}
static bool equipment(t_itemid id){auto data=item_db.find(id);return data && (data->type==IT_WEAPON || data->type==IT_ARMOR || data->type==IT_SHADOWGEAR);}
struct RecipeKey {std::string shop;uint32 index;};
static std::vector<RecipeKey> recipes(t_itemid id){
    std::vector<RecipeKey> result;
    for(const auto& shop:barter_db)for(const auto& entry:shop.second->items)
        if(entry.second->nameid==id)result.push_back({shop.first,entry.first});
    for(const auto& reform:item_reform_db)for(const auto& base:reform.second->base_items)
        if(base.second->resultItemId==id)result.push_back({"@reform/"+std::to_string(reform.first),base.first});
    std::sort(result.begin(),result.end(),[](const auto& a,const auto& b){
        // Keep the established barter ordering before additional reform plans.
        const bool ar=a.shop.rfind("@reform/",0)==0,br=b.shop.rfind("@reform/",0)==0;
        if(ar!=br)return !ar;return std::tie(a.shop,a.index)<std::tie(b.shop,b.index);
    });
    return result;
}
static void location(map_session_data& sd,const char* prefix,int index,npc_data* nd){
    text(sd,(std::string(prefix)+"Map$").c_str(),index,nd?map[nd->m].name:"");
    number(sd,(std::string(prefix)+"X").c_str(),index,nd?nd->x:0);
    number(sd,(std::string(prefix)+"Y").c_str(),index,nd?nd->y:0);
}
}

// Up to 15 stable item IDs per page; search accepts item name, Aegis name or ID.
BUILDIN_FUNC(pnplansearch){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    const auto query=pn_planner::lower(script_getstr(st,2));const int page=script_getnum(st,3);
    pn_planner::number(*sd,"@PNPlanMore",0,0);
    if(query.size()>80 || page<0 || page>10000){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    std::set<t_itemid> matches;
    for(const auto& shop:barter_db)for(const auto& entry:shop.second->items){
        const auto id=entry.second->nameid;if(!pn_planner::equipment(id))continue;
        auto data=item_db.find(id);
        if(query.empty() || pn_planner::lower(data->ename).find(query)!=std::string::npos ||
           pn_planner::lower(data->name).find(query)!=std::string::npos || std::to_string(id)==query)matches.insert(id);
    }
    for(const auto& reform:item_reform_db)for(const auto& base:reform.second->base_items){
        const auto id=base.second->resultItemId;if(!pn_planner::equipment(id))continue;
        auto data=item_db.find(id);
        if(query.empty() || pn_planner::lower(data->ename).find(query)!=std::string::npos ||
           pn_planner::lower(data->name).find(query)!=std::string::npos || std::to_string(id)==query)matches.insert(id);
    }
    int count=0,skip=page*15;
    for(auto id:matches){if(skip){--skip;continue;}if(count==15){pn_planner::number(*sd,"@PNPlanMore",0,1);break;}pn_planner::number(*sd,"@PNPlanItem",count++,id);}
    script_pushint(st,count);return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pnplanrecipes){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    const auto id=script_getnum64(st,2);
    if(id<=0 || id>UINT32_MAX){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    const auto recipes=pn_planner::recipes(static_cast<t_itemid>(id));
    int count=0;
    for(const auto& recipe:recipes){if(count==100)break;pn_planner::text(*sd,"@PNRecipeShop$",count,recipe.shop);pn_planner::number(*sd,"@PNRecipeIndex",count,recipe.index);++count;}
    script_pushint(st,count);return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pnplanrecipe){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
    const std::string key=script_getstr(st,2);
    if(key.rfind("@reform/",0)==0){
        const auto suffix=key.substr(8);char* end=nullptr;errno=0;
        const auto catalyst=strtoull(suffix.c_str(),&end,10);const int64 index=script_getnum64(st,3);
        if(suffix.empty() || suffix.find_first_not_of("0123456789")!=std::string::npos || errno || !end || *end || catalyst<1 || catalyst>UINT32_MAX || index<1 || index>UINT32_MAX){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
        const auto reform=item_reform_db.find(static_cast<t_itemid>(catalyst));
        if(!reform || !reform->base_items.count(static_cast<t_itemid>(index)) || !item_db.find(catalyst)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
        const auto base=reform->base_items.at(static_cast<t_itemid>(index));
        auto data=item_db.find(base->item_id);
        if(!data || !item_db.find(base->resultItemId)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
        pn_planner::Counts costs,inventory,storage;costs[base->item_id]=1;
        for(const auto& material:base->materials)costs[material.first]+=material.second;
        const bool known=pn_planner::holdings(*sd,costs,inventory,storage);
        std::string rules=data->ename+": refine +"+std::to_string(base->minimumRefine)+" to +"+std::to_string(base->maximumRefine)+"; random options required: "+std::to_string(base->requiredRandomOptions)+". ";
        rules+=base->cardsAllowed?"Cards allowed. ":"Remove cards first. ";
        rules+="Refine change: "+std::to_string(base->refineChange)+". ";
        if(base->clearSlots)rules+="Cards and enchants are cleared. ";
        if(base->removeEnchantgrade)rules+="Grade is removed. ";
        if(base->randomOptionGroup)rules+="Random options are replaced. ";
        rules+="Open with "+item_db.find(catalyst)->ename+"; its acquisition and consumption depend on the existing reform service.";
        pn_planner::number(*sd,"@PNPlanKnown",0,known);pn_planner::number(*sd,"@PNPlanZeny",0,0);
        pn_planner::number(*sd,"@PNPlanOutput",0,base->resultItemId);pn_planner::number(*sd,"@PNPlanRefine",0,base->refineChange);
        pn_planner::number(*sd,"@PNPlanReform",0,1);pn_planner::number(*sd,"@PNPlanCatalyst",0,catalyst);
        pn_planner::number(*sd,"@PNPlanStock",0,-1);pn_planner::text(*sd,"@PNPlanRules$",0,rules);
        pn_planner::location(*sd,"@PNPlan",0,nullptr);pn_planner::text(*sd,"@PNPlanNpc$",0,"Item reform");
        int count=0;for(const auto& cost:costs){pn_planner::number(*sd,"@PNMaterial",count,cost.first);pn_planner::number(*sd,"@PNRequired",count,cost.second);pn_planner::number(*sd,"@PNInventory",count,known?inventory[cost.first]:-1);pn_planner::number(*sd,"@PNStorage",count,known?storage[cost.first]:-1);++count;}
        script_pushint(st,count);return SCRIPT_CMD_SUCCESS;
    }
    const auto shop=barter_db.find(script_getstr(st,2));const int index=script_getnum(st,3);
    if(!shop || index<0 || index>UINT16_MAX || !shop->items.count(index)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
    const auto recipe=shop->items.at(index);pn_planner::Counts costs,inventory,storage;
    pn_planner::number(*sd,"@PNPlanReform",0,0);pn_planner::number(*sd,"@PNPlanCatalyst",0,0);
    std::string rules;
    for(const auto& entry:recipe->requirements){const auto& cost=*entry.second;costs[cost.nameid]+=cost.amount;
        if(cost.refine>=0){auto data=item_db.find(cost.nameid);if(!data){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
            rules+=data->ename+": refine "+(itemdb_isstackable2(data.get())?std::string("exactly "):std::string("at least "))+std::to_string(cost.refine)+". ";}
    }
    const bool known=costs.empty() || pn_planner::holdings(*sd,costs,inventory,storage);
    pn_planner::number(*sd,"@PNPlanKnown",0,known);
    pn_planner::number(*sd,"@PNPlanZeny",0,recipe->price);
    pn_planner::number(*sd,"@PNPlanOutput",0,recipe->nameid);
    pn_planner::number(*sd,"@PNPlanRefine",0,recipe->refine);
    pn_planner::number(*sd,"@PNPlanStock",0,recipe->stockLimited?static_cast<int64>(recipe->stock):-1);
    pn_planner::text(*sd,"@PNPlanRules$",0,rules);
    auto* nd=pn_planner::entrance(*shop);pn_planner::location(*sd,"@PNPlan",0,nd);
    pn_planner::text(*sd,"@PNPlanNpc$",0,nd?nd->name:shop->name);
    int count=0;
    for(const auto& cost:costs){pn_planner::number(*sd,"@PNMaterial",count,cost.first);pn_planner::number(*sd,"@PNRequired",count,cost.second);pn_planner::number(*sd,"@PNInventory",count,known?inventory[cost.first]:-1);pn_planner::number(*sd,"@PNStorage",count,known?storage[cost.first]:-1);++count;}
    script_pushint(st,count);return SCRIPT_CMD_SUCCESS;
}

// Loaded monster drop metadata is offered only with an actual normal spawn.
// Scripted/instance rewards without a verified destination remain unlisted.
BUILDIN_FUNC(pnplansources){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    const int64 id=script_getnum64(st,2);const int page=script_getnum(st,3);
    struct Source {std::string label,map;int x=0,y=0;};std::map<std::string,Source> sources;
    if(id<=0 || id>UINT32_MAX || page<0 || page>10000){script_pushint(st,0);return SCRIPT_CMD_SUCCESS;}
    for(const auto& shop:barter_db){bool sells=false;for(const auto& entry:shop.second->items)if(entry.second->nameid==id)sells=true;
        if(sells){auto* nd=pn_planner::entrance(*shop.second);if(nd){Source source{"Exchange: "+std::string(nd->name),map[nd->m].name,nd->x,nd->y};sources[source.label+source.map]=source;}}
    }
    for(const auto& entry:mob_db){bool ordinary=false,mvp=false;for(const auto& drop:entry.second->dropitem)if(drop->nameid==id && drop->rate)ordinary=true;
        for(const auto& drop:entry.second->mvpitem)if(drop->nameid==id && drop->rate)mvp=true;
        if(!ordinary && !mvp)continue;
        for(const auto& spawn:mob_get_spawns(entry.first)){const int m=map_mapindex2mapid(spawn.mapindex);if(!spawn.qty || m<0 || map[m].instance_id)continue;
            Source source{entry.second->jname+(ordinary?" drop at ":" MVP reward at ")+map[m].name,map[m].name};sources[source.label]=source;
        }
    }
    int count=0,skip=page*15;pn_planner::number(*sd,"@PNSourceMore",0,0);
    for(const auto& entry:sources){if(skip){--skip;continue;}if(count==15){pn_planner::number(*sd,"@PNSourceMore",0,1);break;}
        const auto& source=entry.second;pn_planner::text(*sd,"@PNSourceLabel$",count,source.label);pn_planner::text(*sd,"@PNSourceMap$",count,source.map);pn_planner::number(*sd,"@PNSourceX",count,source.x);pn_planner::number(*sd,"@PNSourceY",count,source.y);++count;
    }
    script_pushint(st,count);return SCRIPT_CMD_SUCCESS;
}
