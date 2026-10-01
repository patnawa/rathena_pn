// One SQL transaction owns both players' item arrays, balances and replay receipt.
#include <custom/pair_commit.hpp>
static bool pn_pair_tables(const pn_pair::Commit& r) {
    const char* tables[]={schema_config.char_db,schema_config.inventory_db,schema_config.acc_reg_num_table,"pn_pair_commits",
        r.kind==pn_pair::Vending?schema_config.cart_db:nullptr,
        r.kind==pn_pair::Vending?"vendings":(r.kind==pn_pair::Buying?"buyingstores":nullptr),
        r.kind==pn_pair::Vending?"vending_items":(r.kind==pn_pair::Buying?"buyingstore_items":nullptr)};
    for(const char* table:tables) {
        if(!table)continue;
        if(Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS)return false;
        char* value=nullptr;
        bool good=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&value,nullptr)==SQL_SUCCESS && value && !strcmpi(value,"InnoDB");
        Sql_FreeResult(sql_handle);if(!good)return false;
    }
    return true;
}
static bool pn_pair_tosql(const pn_pair::Commit& r) {
    if(!r.sequence || !pn_pair::conserved(r) || r.side[0].account_id==r.side[1].account_id || r.side[0].char_id==r.side[1].char_id ||
       r.listing_count>pn_pair::listing_capacity || (r.kind==pn_pair::Trade && (r.vending_id || r.listing_count)) ||
       (r.kind!=pn_pair::Buying && r.buying_budget))return false;
    for(const auto& s:r.side)if(!s.account_id || !s.char_id || !(s.nonce_hi|s.nonce_lo))return false;
    if(r.kind==pn_pair::Vending) {
        if(!r.vending_id)return false;
        for(uint32 i=0;i<r.listing_count;++i)if(!r.listings[i].cart_id || !r.listings[i].amount || r.listings[i].price<0)return false;
    }
    if(r.kind==pn_pair::Buying) {
        if(!r.vending_id || !r.listing_count || r.buying_budget>r.side[1].wallet_after)return false;
        for(uint32 i=0;i<r.listing_count;++i)
            if(!r.listings[i].cart_id || r.listings[i].amount>UINT16_MAX || r.listings[i].price<=0)return false;
    }
    if(!pn_pair_tables(r) || Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS)return false;
    const auto& key=r.side[0];bool success=false,wrote=false;
    do {
        if(Sql_Query(sql_handle,"SELECT payload FROM pn_pair_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64 " FOR UPDATE",key.account_id,key.nonce_hi,key.nonce_lo,r.sequence)!=SQL_SUCCESS)break;
        int row=Sql_NextRow(sql_handle);
        if(row==SQL_SUCCESS) {
            char* data=nullptr;size_t length=0;
            success=Sql_GetData(sql_handle,0,&data,&length)==SQL_SUCCESS && data && length==sizeof(r) && !memcmp(data,&r,sizeof(r));
            Sql_FreeResult(sql_handle);break; // Never reapply an old inventory snapshot.
        }
        Sql_FreeResult(sql_handle);if(row!=SQL_NO_DATA)break;
        const size_t first=r.side[0].char_id<r.side[1].char_id?0:1;
        bool owners=true;
        for(size_t turn=0;turn<2;++turn) {
            const auto& s=r.side[turn?1-first:first];
            if(Sql_Query(sql_handle,"SELECT account_id FROM `%s` WHERE char_id=%u AND account_id=%u FOR UPDATE",schema_config.char_db,s.char_id,s.account_id)!=SQL_SUCCESS){owners=false;break;}
            owners=Sql_NextRow(sql_handle)==SQL_SUCCESS;Sql_FreeResult(sql_handle);if(!owners)break;
        }
        if(!owners)break;
        if(r.kind==pn_pair::Vending) {
            if(Sql_Query(sql_handle,"SELECT id FROM vendings WHERE id=%u AND account_id=%u AND char_id=%u FOR UPDATE",r.vending_id,r.side[1].account_id,r.side[1].char_id)!=SQL_SUCCESS)break;
            bool owner=Sql_NextRow(sql_handle)==SQL_SUCCESS;Sql_FreeResult(sql_handle);if(!owner)break;
        }
        if(r.kind==pn_pair::Buying) {
            if(Sql_Query(sql_handle,"SELECT id FROM buyingstores WHERE id=%u AND account_id=%u AND char_id=%u FOR UPDATE",r.vending_id,r.side[1].account_id,r.side[1].char_id)!=SQL_SUCCESS)break;
            bool owner=Sql_NextRow(sql_handle)==SQL_SUCCESS;Sql_FreeResult(sql_handle);if(!owner)break;
        }
        bool saved=true;
        for(size_t i=0;i<2;++i) {
            const auto& s=r.side[i];const bool cart=i==1 && r.kind==pn_pair::Vending;
            if(char_memitemdata_to_sql(s.items,cart?MAX_CART:MAX_INVENTORY,s.char_id,cart?TABLE_CART:TABLE_INVENTORY,0)!=0 ||
               Sql_Query(sql_handle,"UPDATE `%s` SET zeny=%" PRId64 " WHERE char_id=%u AND account_id=%u",schema_config.char_db,s.wallet_after,s.char_id,s.account_id)!=SQL_SUCCESS){saved=false;break;}
            if(s.bank_after!=s.bank_before && Sql_Query(sql_handle,"REPLACE INTO `%s` (`account_id`,`key`,`index`,`value`) VALUES (%u,'#BANKVAULT',0,%" PRId64 ")",schema_config.acc_reg_num_table,s.account_id,s.bank_after)!=SQL_SUCCESS){saved=false;break;}
        }
        if(!saved)break;
        if(r.kind==pn_pair::Vending) {
            if(Sql_Query(sql_handle,"DELETE FROM vending_items WHERE vending_id=%u",r.vending_id)!=SQL_SUCCESS)break;
            for(uint32 i=0;i<r.listing_count;++i) {
                const auto& item=r.listings[i];
                if(Sql_Query(sql_handle,"INSERT INTO vending_items (vending_id,`index`,cartinventory_id,amount,price) VALUES (%u,%u,%u,%u,%" PRId64 ")",r.vending_id,i,item.cart_id,item.amount,item.price)!=SQL_SUCCESS){saved=false;break;}
            }
            if(!saved)break;
        }
        if(r.kind==pn_pair::Buying) {
            if(Sql_Query(sql_handle,"UPDATE buyingstores SET `limit`=%" PRId64 " WHERE id=%u",r.buying_budget,r.vending_id)!=SQL_SUCCESS ||
               Sql_Query(sql_handle,"DELETE FROM buyingstore_items WHERE buyingstore_id=%u",r.vending_id)!=SQL_SUCCESS)break;
            for(uint32 i=0;i<r.listing_count;++i) {
                const auto& item=r.listings[i];if(!item.amount)continue;
                if(Sql_Query(sql_handle,"INSERT INTO buyingstore_items (buyingstore_id,`index`,item_id,amount,price) VALUES (%u,%u,%u,%u,%" PRId64 ")",r.vending_id,i,item.cart_id,item.amount,item.price)!=SQL_SUCCESS){saved=false;break;}
            }
            if(!saved)break;
        }
        SqlStmt stmt{*sql_handle};
        if(stmt.Prepare("INSERT INTO pn_pair_commits (account_id,nonce_hi,nonce_lo,sequence,payload) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",?)",key.account_id,key.nonce_hi,key.nonce_lo,r.sequence)!=SQL_SUCCESS ||
           stmt.BindParam(0,SQLDT_BLOB,const_cast<pn_pair::Commit*>(&r),sizeof(r))!=SQL_SUCCESS || stmt.Execute()!=SQL_SUCCESS)break;
        success=wrote=true;
    }while(false);
    if(Sql_EndTransaction(sql_handle,success)!=SQL_SUCCESS)success=false;
    if(success && wrote)for(const auto& s:r.side) {
        auto found=char_get_chardb().find(s.char_id);
        if(found!=char_get_chardb().end() && found->second)found->second->zeny=s.wallet_after;
    }
    return success;
}
static void mapif_parse_PairCommit(int32 fd) {
    if(RFIFOW(fd,2)!=sizeof(pn_pair::Commit))return;
    pn_pair::Commit request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    pn_pair::Ack reply;const auto& first=request.side[0];
    reply.account_id=first.account_id;reply.char_id=first.char_id;reply.nonce_hi=first.nonce_hi;reply.nonce_lo=first.nonce_lo;reply.sequence=request.sequence;
    reply.committed=pn_pair_tosql(request)?1:0;
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
