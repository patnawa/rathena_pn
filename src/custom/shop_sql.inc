// Character-server owner of one immutable NPC purchase and its replay receipt.
// Included by int_storage.cpp. SQL failure is unknown/retry, never a refund.
#include <custom/shop_commit.hpp>
#include <common/socket.hpp>
#include <algorithm>
#include <cerrno>
#include <string>
#include <vector>

static bool pn_shop_tables(const pn_shop::Commit& r) {
    const char* stock = r.kind==pn_shop::Market ? "market" : r.kind==pn_shop::Barter ? "barter" : "sales";
    const char* tables[] = {schema_config.char_db,schema_config.inventory_db,
        schema_config.acc_reg_num_table,stock,"pn_shop_commits"};
    for(const char* table:tables) {
        if(Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS)return false;
        char* engine=nullptr;
        bool valid=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&engine,nullptr)==SQL_SUCCESS && engine && !strcmpi(engine,"InnoDB");
        Sql_FreeResult(sql_handle);
        if(!valid)return false;
    }
    return true;
}

static bool pn_shop_number(uint32 column,int64& out) {
    char* value=nullptr;
    if(Sql_GetData(sql_handle,column,&value,nullptr)!=SQL_SUCCESS || !value)return false;
    char* end=nullptr;
    errno=0;out=strtoll(value,&end,10);
    return !errno && end!=value && *end=='\0';
}

static bool pn_shop_receipt(const pn_shop::Commit& r,uint32 outcome) {
    SqlStmt stmt{*sql_handle};
    return stmt.Prepare("INSERT INTO pn_shop_commits (account_id,nonce_hi,nonce_lo,sequence,outcome,payload) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,?)",r.account_id,r.nonce_hi,r.nonce_lo,r.sequence,outcome)==SQL_SUCCESS &&
        stmt.BindParam(0,SQLDT_BLOB,const_cast<pn_shop::Commit*>(&r),sizeof(r))==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS;
}

// current_owner is checked by the authenticated inter-server handler. Keeping
// it explicit also permits real-database tests without a running map server.
static uint32 pn_shop_tosql(const pn_shop::Commit& r,bool current_owner) {
    if(!pn_shop::valid(r) || !pn_shop_tables(r) || Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS)return pn_shop::Retry;
    uint32 outcome=pn_shop::Retry;
    bool transaction_complete=false,new_commit=false;
    do {
        // A retry may be from a departed session. Existing receipts are safe to
        // read, but must never reapply their old inventory or currency snapshot.
        if(Sql_Query(sql_handle,"SELECT outcome,payload FROM pn_shop_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64 " FOR UPDATE",r.account_id,r.nonce_hi,r.nonce_lo,r.sequence)!=SQL_SUCCESS)break;
        int row=Sql_NextRow(sql_handle);
        if(row==SQL_SUCCESS) {
            int64 recorded=0;char* payload=nullptr;size_t length=0;
            bool same=pn_shop_number(0,recorded) && (recorded==pn_shop::Committed || recorded==pn_shop::Rejected) &&
                Sql_GetData(sql_handle,1,&payload,&length)==SQL_SUCCESS && payload && length==sizeof(r) && !memcmp(payload,&r,sizeof(r));
            Sql_FreeResult(sql_handle);
            if(same){outcome=static_cast<uint32>(recorded);transaction_complete=true;}
            break;
        }
        Sql_FreeResult(sql_handle);
        if(row!=SQL_NO_DATA)break;

        bool reject=!current_owner;
        // The map service flushes prior state on this same stream before it
        // submits the immutable request. Compare the resulting balances/counter.
        if(Sql_Query(sql_handle,"SELECT zeny,uniqueitem_counter FROM `%s` WHERE char_id=%u AND account_id=%u FOR UPDATE",schema_config.char_db,r.char_id,r.account_id)!=SQL_SUCCESS)break;
        row=Sql_NextRow(sql_handle);int64 wallet=0,counter=0;
        bool valid=true;
        if(row==SQL_SUCCESS)valid=pn_shop_number(0,wallet)&&pn_shop_number(1,counter);
        else if(row==SQL_NO_DATA)reject=true;
        else valid=false;
        Sql_FreeResult(sql_handle);
        if(!valid)break;
        if(row==SQL_SUCCESS && (wallet!=r.wallet_before || counter!=r.counter_before))reject=true;
        const char* currency_keys[]={"#CASHPOINTS","#KAFRAPOINTS"};
        const int64 before[]={r.cash_before,r.kafra_before};
        for(int i=0;i<2;++i) {
            if(Sql_Query(sql_handle,"SELECT value FROM `%s` WHERE account_id=%u AND `key`='%s' AND `index`=0 FOR UPDATE",schema_config.acc_reg_num_table,r.account_id,currency_keys[i])!=SQL_SUCCESS){valid=false;break;}
            row=Sql_NextRow(sql_handle);int64 balance=0;
            if(row==SQL_SUCCESS)valid=pn_shop_number(0,balance);
            else if(row!=SQL_NO_DATA)valid=false;
            Sql_FreeResult(sql_handle);
            if(!valid)break;
            if(balance!=before[i])reject=true;
        }
        if(!valid)break;

        struct StockRow { bool missing=false; std::string escaped,canonical; };
        std::vector<StockRow> stock_rows(r.stock_count);
        std::vector<uint32> order;
        for(uint32 i=0;i<r.stock_count;++i)order.push_back(i);
        std::sort(order.begin(),order.end(),[&](uint32 a,uint32 b){int c=strcmp(r.stocks[a].name,r.stocks[b].name);return c?c<0:r.stocks[a].key<r.stocks[b].key;});
        for(uint32 i:order) {
            const auto& s=r.stocks[i];auto& state=stock_rows[i];
            char escaped[2*pn_shop::stock_name_size+1]{};
            Sql_EscapeStringLen(sql_handle,escaped,s.name,strlen(s.name));state.escaped=escaped;
            if(r.kind==pn_shop::Market)valid=Sql_Query(sql_handle,"SELECT amount,`name`,price,flag FROM market WHERE `name`='%s' AND nameid=%u FOR UPDATE",escaped,s.key)==SQL_SUCCESS;
            else if(r.kind==pn_shop::Barter)valid=Sql_Query(sql_handle,"SELECT amount,`name` FROM barter WHERE `name`='%s' AND `index`=%u FOR UPDATE",escaped,s.key)==SQL_SUCCESS;
            else valid=Sql_Query(sql_handle,"SELECT amount,UNIX_TIMESTAMP(`start`),UNIX_TIMESTAMP(`end`),UNIX_TIMESTAMP() FROM sales WHERE nameid=%u FOR UPDATE",s.key)==SQL_SUCCESS;
            if(!valid)break;
            row=Sql_NextRow(sql_handle);
            if(row==SQL_NO_DATA) {
                state.missing=true;
                // Legacy market/barter catalogs may not yet have a SQL row.
                // The authenticated map declares the initial configured stock.
                if(r.kind==pn_shop::Sale)reject=true;
            } else if(row==SQL_SUCCESS) {
                int64 amount=0;
                valid=pn_shop_number(0,amount);
                if(valid && amount!=s.before)reject=true;
                if(r.kind==pn_shop::Sale) {
                    int64 start=0,end=0,now=0;
                    valid=valid&&pn_shop_number(1,start)&&pn_shop_number(2,end)&&pn_shop_number(3,now);
                    if(valid && (start!=s.sale_start || end!=s.sale_end || now<start || now>end))reject=true;
                } else {
                    char* name=nullptr;
                    valid=valid&&Sql_GetData(sql_handle,1,&name,nullptr)==SQL_SUCCESS&&name;
                    if(valid)state.canonical=name;
                    if(r.kind==pn_shop::Market) {
                        int64 price=0,flag=0;valid=valid&&pn_shop_number(2,price)&&pn_shop_number(3,flag);
                        if(valid && (price!=s.price || flag!=s.flag))reject=true;
                    }
                }
            } else valid=false;
            Sql_FreeResult(sql_handle);
            if(!valid)break;
            // SQL collations may identify differently-cased NPC names as one
            // row. Detect this before any write, even if byte keys differed.
            if(!state.canonical.empty())for(uint32 j=0;j<r.stock_count;++j)
                if(j!=i && !stock_rows[j].canonical.empty() && s.key==r.stocks[j].key && state.canonical==stock_rows[j].canonical)reject=true;
        }
        if(!valid)break;
        if(reject) {
            if(!pn_shop_receipt(r,pn_shop::Rejected))break;
            outcome=pn_shop::Rejected;transaction_complete=true;break;
        }

        for(uint32 i:order) {
            const auto& s=r.stocks[i];const auto& state=stock_rows[i];
            if(r.kind==pn_shop::Market) {
                if(state.missing)valid=Sql_Query(sql_handle,"INSERT INTO market (`name`,nameid,price,amount,flag) VALUES ('%s',%u,%u,%" PRId64 ",%u)",state.escaped.c_str(),s.key,s.price,s.after,static_cast<uint32>(s.flag))==SQL_SUCCESS;
                else valid=Sql_Query(sql_handle,"UPDATE market SET amount=%" PRId64 " WHERE `name`='%s' AND nameid=%u AND amount=%" PRId64,s.after,state.escaped.c_str(),s.key,s.before)==SQL_SUCCESS;
            } else if(r.kind==pn_shop::Barter) {
                if(state.missing)valid=Sql_Query(sql_handle,"INSERT INTO barter (`name`,`index`,amount) VALUES ('%s',%u,%" PRId64 ")",state.escaped.c_str(),s.key,s.after)==SQL_SUCCESS;
                else valid=Sql_Query(sql_handle,"UPDATE barter SET amount=%" PRId64 " WHERE `name`='%s' AND `index`=%u AND amount=%" PRId64,s.after,state.escaped.c_str(),s.key,s.before)==SQL_SUCCESS;
            } else {
                // Keep an exhausted row at zero. Optional later cleanup must
                // not make a failed last-unit DELETE restore usable stock.
                valid=Sql_Query(sql_handle,"UPDATE sales SET amount=%" PRId64 " WHERE nameid=%u AND amount=%" PRId64,s.after,s.key,s.before)==SQL_SUCCESS;
            }
            if(!valid || Sql_NumRowsAffected(sql_handle)!=1){valid=false;break;}
        }
        if(!valid)break;
        if(char_memitemdata_to_sql(r.items,MAX_INVENTORY,r.char_id,TABLE_INVENTORY,0)!=0)break;
        if(Sql_Query(sql_handle,"UPDATE `%s` SET zeny=%" PRId64 ",uniqueitem_counter=%u WHERE char_id=%u AND account_id=%u",schema_config.char_db,r.wallet_after,r.counter_after,r.char_id,r.account_id)!=SQL_SUCCESS)break;
        const int64 after[]={r.cash_after,r.kafra_after};
        for(int i=0;i<2;++i)if(Sql_Query(sql_handle,"REPLACE INTO `%s` (account_id,`key`,`index`,value) VALUES (%u,'%s',0,%" PRId64 ")",schema_config.acc_reg_num_table,r.account_id,currency_keys[i],after[i])!=SQL_SUCCESS){valid=false;break;}
        if(!valid || !pn_shop_receipt(r,pn_shop::Committed))break;
        outcome=pn_shop::Committed;transaction_complete=new_commit=true;
    } while(false);
    // A failed COMMIT acknowledgement is indeterminate. An identical retry
    // resolves the receipt; no compensating write or success ACK is sent.
    const int ended=Sql_EndTransaction(sql_handle,transaction_complete);
    // A lost COMMIT reply can leave the cached character older than its DB
    // row. Receipt replay must not copy an old snapshot over newer state;
    // evict instead, so a later authentication reloads authoritative state.
    if((new_commit && ended!=SQL_SUCCESS) || (transaction_complete && outcome==pn_shop::Committed && !new_commit))
        char_get_chardb().erase(r.char_id);
    if(ended!=SQL_SUCCESS)return pn_shop::Retry;
    if(!transaction_complete)return pn_shop::Retry;
    if(new_commit) {
        auto found=char_get_chardb().find(r.char_id);
        if(found!=char_get_chardb().end() && found->second) {
            found->second->zeny=r.wallet_after;
            found->second->uniqueitem_counter=r.counter_after;
        }
    }
    return outcome;
}

static void mapif_parse_ShopCommit(int32 fd) {
    if(RFIFOW(fd,2)!=sizeof(pn_shop::Commit))return;
    pn_shop::Commit request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    bool owner=false;
    auto online=char_get_onlinedb().find(request.account_id);
    if(online!=char_get_onlinedb().end() && online->second && online->second->char_id==request.char_id) {
        int32 server=online->second->server;
        owner=server>=0 && server<MAX_MAP_SERVERS && map_server[server].fd==fd;
    }
    pn_shop::Ack reply;reply.account_id=request.account_id;reply.char_id=request.char_id;
    reply.nonce_hi=request.nonce_hi;reply.nonce_lo=request.nonce_lo;reply.sequence=request.sequence;
    reply.outcome=pn_shop_tosql(request,owner);
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
