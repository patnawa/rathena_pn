// Included after pn_shop_number in shop_sql.inc. Caller owns one InnoDB transaction.
enum class pn_mail_asset_result { Error, Conflict, Ready };
static pn_mail_asset_result pn_mail_asset_take(const pn_shop::Commit& r) {
    if(!r.mail_id)return pn_mail_asset_result::Ready;
    if(!Sql_InTransaction(sql_handle))return pn_mail_asset_result::Error;
    if(Sql_Query(sql_handle,"SELECT dest_id,status,zeny FROM `%s` WHERE id=%u FOR UPDATE",schema_config.mail_db,r.mail_id)!=SQL_SUCCESS)
        return pn_mail_asset_result::Error;
    int row=Sql_NextRow(sql_handle);int64 owner=0,status=0,zeny=0;
    bool valid=row==SQL_SUCCESS && pn_shop_number(0,owner) && pn_shop_number(1,status) && pn_shop_number(2,zeny);
    Sql_FreeResult(sql_handle);
    if(row!=SQL_SUCCESS && row!=SQL_NO_DATA)return pn_mail_asset_result::Error;
    if(row==SQL_NO_DATA)return pn_mail_asset_result::Conflict;
    if(!valid)return pn_mail_asset_result::Error;
    if(owner!=r.char_id || (status<MAIL_NEW || status>MAIL_READ) || (charserv_config.mail_retrieve==0 && status!=MAIL_READ) ||
       (r.mail_zeny && zeny!=r.mail_zeny))return pn_mail_asset_result::Conflict;
    std::string query="SELECT amount,nameid,refine,attribute,identify,unique_id,bound,enchantgrade";
    for(int j=0;j<MAX_SLOTS;++j)query+=",card"+std::to_string(j);
    for(int j=0;j<MAX_ITEM_RDM_OPT;++j)query+=",option_id"+std::to_string(j)+",option_val"+std::to_string(j)+",option_parm"+std::to_string(j);
    query+=" FROM `"+std::string(schema_config.mail_attachment_db)+"` WHERE id="+std::to_string(r.mail_id)+" ORDER BY `index` FOR UPDATE";
    if(Sql_QueryStr(sql_handle,query.c_str())!=SQL_SUCCESS)return pn_mail_asset_result::Error;
    size_t index=0;bool same=true;
    while((row=Sql_NextRow(sql_handle))==SQL_SUCCESS) {
        while(index<MAIL_MAX_ITEM && !r.mail_items[index].nameid)++index;
        if(index>=MAIL_MAX_ITEM){same=false;continue;}
        const auto& it=r.mail_items[index++];
        const int64 fields[]={it.amount,it.nameid,it.refine,it.attribute,it.identify,0,it.bound,it.enchantgrade};
        for(int col=0;col<8;++col) {
            if(col==5){
                char* data=nullptr;char* end=nullptr;errno=0;
                if(Sql_GetData(sql_handle,col,&data,nullptr)!=SQL_SUCCESS || !data || !*data || *data=='-'){valid=false;break;}
                uint64 id=strtoull(data,&end,10);
                if(errno || !end || *end){valid=false;break;}
                if(id!=it.unique_id)same=false;
            }else{int64 number=0;if(!pn_shop_number(col,number)){valid=false;break;}if(number!=fields[col])same=false;}
        }
        for(int j=0;j<MAX_SLOTS && valid;++j){int64 number=0;if(!pn_shop_number(8+j,number))valid=false;else if(number!=it.card[j])same=false;}
        for(int j=0;j<MAX_ITEM_RDM_OPT && valid;++j){
            const int64 options[]={it.option[j].id,it.option[j].value,it.option[j].param};
            for(int k=0;k<3;++k){int64 number=0;if(!pn_shop_number(8+MAX_SLOTS+j*3+k,number)){valid=false;break;}if(number!=options[k])same=false;}
        }
        if(!valid)break;
    }
    Sql_FreeResult(sql_handle);
    if(!valid || row!=SQL_NO_DATA)return pn_mail_asset_result::Error;
    while(index<MAIL_MAX_ITEM && !r.mail_items[index].nameid)++index;
    if(!same || index!=MAIL_MAX_ITEM)return pn_mail_asset_result::Conflict;
    for(const auto& it:r.mail_items)if(it.nameid && it.card[0]==pn_pet::egg_marker) {
        if(it.amount!=1 || !(it.card[1]|it.card[2]) || it.card[1]>UINT16_MAX || it.card[2]>INT16_MAX)return pn_mail_asset_result::Conflict;
        const uint32 pet_id=it.card[1]|(it.card[2]<<16);
        if(Sql_Query(sql_handle,"SELECT egg_id,incubate,account_id,char_id FROM `%s` WHERE pet_id=%u FOR UPDATE",schema_config.pet_db,pet_id)!=SQL_SUCCESS)return pn_mail_asset_result::Error;
        row=Sql_NextRow(sql_handle);int64 egg=0,incubate=0,account=0,character=0;
        valid=row==SQL_SUCCESS && pn_shop_number(0,egg) && pn_shop_number(1,incubate) && pn_shop_number(2,account) && pn_shop_number(3,character);
        Sql_FreeResult(sql_handle);
        if(row!=SQL_SUCCESS && row!=SQL_NO_DATA)return pn_mail_asset_result::Error;
        if(row==SQL_NO_DATA)return pn_mail_asset_result::Conflict;
        if(!valid)return pn_mail_asset_result::Error;
        if(egg!=it.nameid || incubate!=1 || account || character)return pn_mail_asset_result::Conflict;
    }
    if(Sql_Query(sql_handle,"DELETE FROM `%s` WHERE id=%u",schema_config.mail_attachment_db,r.mail_id)!=SQL_SUCCESS)
        return pn_mail_asset_result::Error;
    if(r.mail_zeny && (Sql_Query(sql_handle,"UPDATE `%s` SET zeny=0 WHERE id=%u AND zeny=%" PRId64,schema_config.mail_db,r.mail_id,r.mail_zeny)!=SQL_SUCCESS || Sql_NumRowsAffected(sql_handle)!=1))
        return pn_mail_asset_result::Error;
    return pn_mail_asset_result::Ready;
}
