// GPL-3.0-or-later. Owner comes only from the attached authenticated player.
#include <nlohmann/json.hpp>
namespace pn_purchase_view {
static bool number(const char* value,uint64& result){
    if(!value || !*value || strlen(value)>20)return false;
    for(const char* p=value;*p;++p)if(*p<'0' || *p>'9')return false;
    char* end=nullptr;errno=0;result=strtoull(value,&end,10);return !errno && end && !*end;
}
static bool reference(const std::string& value,uint64& hi,uint64& lo,uint64& sequence){
    const auto first=value.find('-'),last=value.rfind('-');
    if(first==std::string::npos || first==last || value.size()>62)return false;
    return number(value.substr(0,first).c_str(),hi) && number(value.substr(first+1,last-first-1).c_str(),lo) &&
        number(value.substr(last+1).c_str(),sequence) && sequence && (hi || lo);
}
static const char* kind(int value){switch(value){case 1:return "Market purchase";case 2:return "Equipment/material exchange";case 3:return "Sale purchase";case 4:return "Shop or reward";case 5:return "Pet recovery";case 6:return "Item reward";default:return "Purchase";}}
static std::string item_name(uint32 id){auto data=item_db.find(id);return (data?data->ename:std::string("Item"))+" ["+std::to_string(id)+"]";}
}

BUILDIN_FUNC(pnpurchasehistory){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
    const int page=script_getnum(st,2);pn_planner::number(*sd,"@PNHistoryMore",0,0);
    pn_planner::number(*sd,"@PNHistoryPending",0,sd->shop_commit.pending);
    if(page<0 || page>1000 || !qsmysql_handle || Sql_Query(qsmysql_handle,
        "SELECT nonce_hi,nonce_lo,sequence,kind,outcome,created_at FROM pn_purchase_history WHERE account_id=%u AND char_id=%u ORDER BY created_at DESC,nonce_hi DESC,nonce_lo DESC,sequence DESC LIMIT 11 OFFSET %u",
        sd->status.account_id,sd->status.char_id,page*10)!=SQL_SUCCESS){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
    int count=0,row;bool valid=true;
    while((row=Sql_NextRow(qsmysql_handle))==SQL_SUCCESS){
        if(count==10){pn_planner::number(*sd,"@PNHistoryMore",0,1);break;}
        char* values[6]{};for(int i=0;i<6;++i)if(Sql_GetData(qsmysql_handle,i,&values[i],nullptr)!=SQL_SUCCESS || !values[i])valid=false;
        uint64 hi=0,lo=0,sequence=0,kind=0,outcome=0;
        if(!valid || !pn_purchase_view::number(values[0],hi) || !pn_purchase_view::number(values[1],lo) ||
           !pn_purchase_view::number(values[2],sequence) || !pn_purchase_view::number(values[3],kind) || !pn_purchase_view::number(values[4],outcome)){valid=false;break;}
        const auto ref=std::to_string(hi)+"-"+std::to_string(lo)+"-"+std::to_string(sequence);
        const auto label=std::string(values[5])+" "+pn_purchase_view::kind(kind)+(outcome==pn_shop::Committed?" (committed)":" (rejected)");
        pn_planner::text(*sd,"@PNHistoryRef$",count,ref);pn_planner::text(*sd,"@PNHistoryLabel$",count,label);++count;
    }
    Sql_FreeResult(qsmysql_handle);
    script_pushint(st,valid && (row==SQL_NO_DATA || count==10)?count:-1);return SCRIPT_CMD_SUCCESS;
}

BUILDIN_FUNC(pnpurchasedetail){
    map_session_data* sd=nullptr;if(!script_rid2sd(sd)){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;}
    uint64 hi=0,lo=0,sequence=0;const std::string ref=script_getstr(st,2);
    auto reject=[&](){script_pushint(st,-1);return SCRIPT_CMD_SUCCESS;};
    if(!pn_purchase_view::reference(ref,hi,lo,sequence) || !qsmysql_handle)return reject();
    if(Sql_Query(qsmysql_handle,"SELECT kind,outcome,details,created_at FROM pn_purchase_history WHERE account_id=%u AND char_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64,
       sd->status.account_id,sd->status.char_id,hi,lo,sequence)!=SQL_SUCCESS)return reject();
    char* values[4]{};size_t length=0;bool valid=Sql_NextRow(qsmysql_handle)==SQL_SUCCESS;
    for(int i=0;i<4 && valid;++i)valid=Sql_GetData(qsmysql_handle,i,&values[i],i==2?&length:nullptr)==SQL_SUCCESS && values[i];
    uint64 kind=0,outcome=0;std::string encoded,date;
    if(valid)valid=length<=131072 && pn_purchase_view::number(values[0],kind) && pn_purchase_view::number(values[1],outcome);
    if(valid){encoded.assign(values[2],length);date=values[3];}
    Sql_FreeResult(qsmysql_handle);if(!valid)return reject();
    std::vector<std::string> lines;
    lines.push_back(std::string(pn_purchase_view::kind(kind))+" - "+date);
    lines.push_back("Support reference: "+std::to_string(sd->status.account_id)+"/"+std::to_string(sd->status.char_id)+"/"+ref);
    if(outcome!=pn_shop::Committed)lines.push_back("Rejected: no payment or output committed by this request.");
    else{
        try{
            const auto data=nlohmann::json::parse(encoded);if(data.at("version")!=1 || !data.at("items").is_array() || data.at("items").size()>MAX_INVENTORY*2)return reject();
            lines.push_back("Committed to your character record. Reopening history never delivers again.");
            for(const auto& key:{"zeny","cash","kafra"}){const auto delta=data.at(key).get<int64>();if(delta==INT64_MIN)return reject();
                lines.push_back(std::string(key)+": "+(delta<=0?"spent ":"received ")+std::to_string(delta<=0?-delta:delta));}
            if(data.contains("point_key")){const auto delta=data.at("point_delta").get<int64>();if(delta==INT64_MIN)return reject();lines.push_back(data.at("point_key").get<std::string>()+": "+std::to_string(-delta)+" spent.");}
            lines.push_back("Net item quantities added (+) or removed (-):");
            for(const auto& item:data.at("items")){const uint32 id=item.at(0).get<uint32>();const int64 delta=item.at(1).get<int64>();lines.push_back((delta>0?"+":"")+std::to_string(delta)+" x "+pn_purchase_view::item_name(id));}
            if(data.at("items").empty())lines.push_back("No net item quantity change. Same-item metadata changes are not shown here.");
            for(const auto& pet:data.at("pets"))lines.push_back("Pet reward issued: "+pn_purchase_view::item_name(pet.get<uint32>())+".");
        }catch(const nlohmann::json::exception&){return reject();}
        // Current recovery state is read independently of the old receipt.
        // Both owner columns and the exact receipt identity are mandatory.
        if(Sql_Query(qsmysql_handle,"SELECT claimed,COUNT(*) FROM pn_pet_entitlements WHERE account_id=%u AND char_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64 " GROUP BY claimed",
           sd->status.account_id,sd->status.char_id,hi,lo,sequence)!=SQL_SUCCESS)return reject();
        int row;while((row=Sql_NextRow(qsmysql_handle))==SQL_SUCCESS){char* claimed=nullptr;char* count=nullptr;uint64 state=0,total=0;
            if(Sql_GetData(qsmysql_handle,0,&claimed,nullptr)!=SQL_SUCCESS || Sql_GetData(qsmysql_handle,1,&count,nullptr)!=SQL_SUCCESS || !pn_purchase_view::number(claimed,state) || !pn_purchase_view::number(count,total)){valid=false;break;}
            lines.push_back(std::to_string(total)+(state?" pet reward(s) already recovered.":" pet reward(s) awaiting recovery. Visit the Pet Recovery Desk or use @petrewards."));
        }
        Sql_FreeResult(qsmysql_handle);if(!valid || row!=SQL_NO_DATA)return reject();
    }
    if(lines.size()>1000)return reject();
    for(size_t i=0;i<lines.size();++i)pn_planner::text(*sd,"@PNHistoryLine$",i,lines[i]);
    script_pushint(st,lines.size());return SCRIPT_CMD_SUCCESS;
}
