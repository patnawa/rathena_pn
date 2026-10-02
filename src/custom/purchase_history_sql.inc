// Same transaction as the immutable purchase receipt; never a second delivery.
#include <nlohmann/json.hpp>
using PnPurchaseCounts=std::map<uint32,int64>;
static bool pn_purchase_before(const pn_shop::Commit& r,PnPurchaseCounts& counts){
    if(Sql_Query(sql_handle,"SELECT nameid,SUM(amount) FROM `%s` WHERE char_id=%u GROUP BY nameid FOR UPDATE",schema_config.inventory_db,r.char_id)!=SQL_SUCCESS)return false;
    bool valid=true;int row;
    while((row=Sql_NextRow(sql_handle))==SQL_SUCCESS){int64 id=0,amount=0;
        if(!pn_shop_number(0,id) || !pn_shop_number(1,amount) || id<=0 || id>UINT32_MAX || amount<0){valid=false;break;}
        counts[static_cast<uint32>(id)]=amount;
    }
    Sql_FreeResult(sql_handle);return valid && row==SQL_NO_DATA;
}
static bool pn_purchase_record(const pn_shop::Commit& r,uint32 outcome,const PnPurchaseCounts* before){
    nlohmann::json details={{"version",1},{"items",nlohmann::json::array()},{"pets",nlohmann::json::array()},{"shops",nlohmann::json::array()}};
    if(outcome==pn_shop::Committed){
        if(!before)return false;
        PnPurchaseCounts delta;
        for(const auto& it:r.items)if(it.nameid && it.amount>0)delta[static_cast<uint32>(it.nameid)]+=it.amount;
        for(const auto& entry:*before)delta[entry.first]-=entry.second;
        for(const auto& entry:delta)if(entry.second)details["items"].push_back({entry.first,entry.second});
        details["zeny"]=r.wallet_after-r.wallet_before;details["cash"]=r.cash_after-r.cash_before;details["kafra"]=r.kafra_after-r.kafra_before;
        if(r.point.scope){details["point_key"]=r.point.key;details["point_delta"]=r.point.after-r.point.before;}
        for(uint16 i=0;i<r.pet_count;++i)if(!r.pets[i].claim_id)details["pets"].push_back(static_cast<uint32>(r.pets[i].output.egg.nameid));
    }
    for(uint32 i=0;i<r.stock_count;++i)if(r.stocks[i].name[0])details["shops"].push_back(r.stocks[i].name);
    // Some legacy NPC names contain non-UTF-8 bytes. Display normalization must
    // not throw through the purchase transaction or change its wire identity.
    const auto encoded=details.dump(-1,' ',false,nlohmann::json::error_handler_t::replace);if(encoded.size()>131072)return false;
    SqlStmt stmt{*sql_handle};
    return stmt.Prepare("INSERT INTO pn_purchase_history(account_id,char_id,nonce_hi,nonce_lo,sequence,kind,outcome,details) VALUES(%u,%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%u,?)",r.account_id,r.char_id,r.nonce_hi,r.nonce_lo,r.sequence,r.kind,outcome)==SQL_SUCCESS &&
        stmt.BindParam(0,SQLDT_STRING,const_cast<char*>(encoded.data()),encoded.size())==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS;
}
