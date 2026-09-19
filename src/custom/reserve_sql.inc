// Durable Kafra grant: inventory, points and receipt commit in one transaction.
// Included by bank_sql.inc; the internal request never accepts client SQL.
static bool reserve_tosql(const pn_bank_commit& r, s_storage& inventory) {
    if (!r.account_id || !r.char_id || !r.request_id || (!r.nonce_hi && !r.nonce_lo) ||
        r.action != 7 || r.amount < 1 || r.amount > 30000 || r.reserve_before < r.amount ||
        r.reserve_after != r.reserve_before - r.amount || r.reserve_after < 0 ||
        !r.reserve_item[0] || !r.reserve_quantity[0] || r.reserve_quantity[0] > 30000 ||
        ((!r.reserve_item[1]) != (!r.reserve_quantity[1])) || r.reserve_quantity[1] > 30000 ||
        inventory.type != TABLE_INVENTORY) return false;
    const char* tables[]={schema_config.char_db, schema_config.inventory_db,
            schema_config.char_reg_num_table, "pn_reserve_commits"};
    for (const char* table : tables) {
        if (Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS) return false;
        char* engine=nullptr;
        bool good=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&engine,nullptr)==SQL_SUCCESS && engine && strcmpi(engine,"InnoDB")==0;
        Sql_FreeResult(sql_handle);if(!good)return false;
    }
    if(Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS)return false;
    bool success=false;
    do {
        if(Sql_Query(sql_handle,"SELECT char_id,cost,points_before,points_after,item0,quantity0,item1,quantity1 FROM pn_reserve_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND request_id=%" PRIu64 " FOR UPDATE",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id)!=SQL_SUCCESS)break;
        int row=Sql_NextRow(sql_handle);
        if(row==SQL_SUCCESS){
            int64 expected[]={r.char_id,r.amount,r.reserve_before,r.reserve_after,r.reserve_item[0],r.reserve_quantity[0],r.reserve_item[1],r.reserve_quantity[1]};
            success=true;
            for(int i=0;i<8;++i){char* data=nullptr;if(Sql_GetData(sql_handle,i,&data,nullptr)!=SQL_SUCCESS||!data||strtoll(data,nullptr,10)!=expected[i])success=false;}
            Sql_FreeResult(sql_handle);break; // Idempotent ACK; never replay the old inventory snapshot.
        }
        Sql_FreeResult(sql_handle);if(row!=SQL_NO_DATA)break;
        if(Sql_Query(sql_handle,"SELECT account_id FROM `%s` WHERE char_id=%u AND account_id=%u FOR UPDATE",schema_config.char_db,r.char_id,r.account_id)!=SQL_SUCCESS)break;
        bool owner=Sql_NextRow(sql_handle)==SQL_SUCCESS;Sql_FreeResult(sql_handle);if(!owner)break;
        if(char_memitemdata_to_sql(inventory.u.items_inventory,MAX_INVENTORY,r.char_id,TABLE_INVENTORY,inventory.stor_id)!=0)break;
        if(Sql_Query(sql_handle,"REPLACE INTO `%s` (char_id,`key`,`index`,`value`) VALUES (%u,'RESRVPTS',0,%" PRId64 ")",schema_config.char_reg_num_table,r.char_id,r.reserve_after)!=SQL_SUCCESS)break;
        if(Sql_Query(sql_handle,"INSERT INTO pn_reserve_commits (account_id,nonce_hi,nonce_lo,request_id,char_id,cost,points_before,points_after,item0,quantity0,item1,quantity1) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%" PRId64 ",%" PRId64 ",%" PRId64 ",%u,%u,%u,%u)",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id,r.char_id,r.amount,r.reserve_before,r.reserve_after,r.reserve_item[0],r.reserve_quantity[0],r.reserve_item[1],r.reserve_quantity[1])!=SQL_SUCCESS)break;
        success=true;
    }while(false);
    if(Sql_EndTransaction(sql_handle,success)!=SQL_SUCCESS)success=false;
    return success;
}
