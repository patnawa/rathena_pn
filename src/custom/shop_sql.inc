// Character-server owner of one immutable NPC purchase and its replay receipt.
// Included by int_storage.cpp. SQL failure is unknown/retry, never a refund.
#include <custom/shop_commit.hpp>
#include <custom/shop_receipt_codec.hpp>
#include <custom/shop_progression.hpp>
#include <common/socket.hpp>
#include <char/int_mail.hpp>
#include <algorithm>
#include <cerrno>
#include <map>
#include <string>
#include <tuple>
#include <vector>

static bool pn_shop_tables(const pn_shop::Commit& r) {
    const char* stock = r.kind==pn_shop::Market ? "market" : r.kind==pn_shop::Barter ? "barter" : "sales";
    std::vector<const char*> tables = {schema_config.char_db,schema_config.inventory_db,
        schema_config.acc_reg_num_table,"pn_shop_commits","pn_purchase_history"};
    if(r.stock_count)tables.push_back(stock);
    if(r.point.scope==pn_shop::CharacterPoint)tables.push_back(schema_config.char_reg_num_table);
    if(r.point.scope==pn_shop::CharacterPoint || r.point.scope==pn_shop::AccountPoint)tables.push_back("pn_point_registry_keys");
    if(r.point.scope==pn_shop::GlobalPoint)tables.push_back("pn_global_point_barriers");
    if(r.mail_id || r.kind==pn_shop::MailSend){tables.push_back(schema_config.mail_db);tables.push_back(schema_config.mail_attachment_db);}
    if(r.pet_count || r.pet_retire_count || r.mail_id || r.kind==pn_shop::MailSend){tables.push_back(schema_config.pet_db);tables.push_back("pn_pet_entitlements");}
    if(r.progression_size)tables.push_back(schema_config.achievement_table);
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

#include <custom/mail_asset_sql.inc>
#include <custom/point_asset_sql.inc>
#include <custom/purchase_history_sql.inc>

static std::vector<uint8> pn_shop_receipt_identity(const pn_shop::Commit& r,const pn_shop_progression::Bundle* progression) {
    const size_t extra=progression?progression->bytes.size():0;
    std::vector<uint8> identity(sizeof(r)+extra);memcpy(identity.data(),&r,sizeof(r));
    if(extra)memcpy(identity.data()+sizeof(r),progression->bytes.data(),extra);
    return identity;
}

static bool pn_shop_receipt(const pn_shop::Commit& r,uint32 outcome,const pn_shop_progression::Bundle* progression,const PnPurchaseCounts* before=nullptr) {
    auto identity=pn_shop_receipt_identity(r,progression);
    if(identity.size()>pn_shop_receipt_codec::limit)return false;
    auto payload = pn_shop_receipt_codec::encode(identity.data(),identity.size());
    SqlStmt stmt{*sql_handle};
    return stmt.Prepare("INSERT INTO pn_shop_commits (account_id,nonce_hi,nonce_lo,sequence,outcome,payload) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,?)",r.account_id,r.nonce_hi,r.nonce_lo,r.sequence,outcome)==SQL_SUCCESS &&
        stmt.BindParam(0,SQLDT_BLOB,payload.data(),payload.size())==SQL_SUCCESS && stmt.Execute()==SQL_SUCCESS &&
        pn_purchase_record(r,outcome,before) && pn_global_point::finish_locked(sql_handle,r);
}

// current_owner is checked by the authenticated inter-server handler. Keeping
// it explicit also permits real-database tests without a running map server.
static uint32 pn_shop_tosql(const pn_shop::Commit& r,bool current_owner,const pn_shop_progression::Bundle* progression=nullptr) {
    const bool progression_valid=(!r.progression_size && !progression) ||
        (progression && progression->bytes.size()==r.progression_size &&
         progression->header.account_id==r.account_id && progression->header.char_id==r.char_id &&
         progression->header.nonce_hi==r.nonce_hi && progression->header.nonce_lo==r.nonce_lo &&
         progression->header.sequence==r.sequence);
    if(!pn_shop::valid(r) || !progression_valid || !pn_shop_tables(r) || Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS)return pn_shop::Retry;
    const auto identity=pn_shop_receipt_identity(r,progression);
    uint32 outcome=pn_shop::Retry;
    bool transaction_complete=false,new_commit=false;
    mail_message mail_notification{};
    do {
        // A retry may be from a departed session. Existing receipts are safe to
        // read, but must never reapply their old inventory or currency snapshot.
        if(Sql_Query(sql_handle,"SELECT outcome,payload FROM pn_shop_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND sequence=%" PRIu64 " FOR UPDATE",r.account_id,r.nonce_hi,r.nonce_lo,r.sequence)!=SQL_SUCCESS)break;
        int row=Sql_NextRow(sql_handle);
        if(row==SQL_SUCCESS) {
            int64 recorded=0;char* payload=nullptr;size_t length=0;
            bool same=pn_shop_number(0,recorded) && (recorded==pn_shop::Committed || recorded==pn_shop::Rejected ||
                recorded==pn_shop::ProgressionStale) &&
                Sql_GetData(sql_handle,1,&payload,&length)==SQL_SUCCESS &&
                pn_shop_receipt_codec::matches(payload,length,identity.data(),identity.size());
            // Global-point expiry can durably reject using the immutable commit
            // before a companion progression frame is available. Rejection has
            // no progression effect, so that exact historical/raw identity is safe.
            if(!same && recorded==pn_shop::Rejected)
                same=pn_shop_receipt_codec::matches(payload,length,&r,sizeof(r));
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

        if(!pn_point_validate_locked(r,reject))break;

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
            if(!pn_shop_receipt(r,pn_shop::Rejected,progression))break;
            outcome=pn_shop::Rejected;transaction_complete=true;break;
        }

        // Everything after this point is an output of the purchase. A newer or
        // divergent achievement baseline must roll all of it back before its
        // terminal stale receipt is recorded.
        if(progression && Sql_Query(sql_handle,"SAVEPOINT pn_shop_outputs")!=SQL_SUCCESS)break;

        // Pet creation/claim belongs to this same payment + inventory + stock
        // transaction. A conflict must undo earlier outputs before recording a
        // durable rejection; SQL failure remains an indeterminate retry.
        if(r.pet_count || r.pet_retire_count || r.mail_id || r.kind==pn_shop::MailSend) {
            if(Sql_Query(sql_handle,"SAVEPOINT pn_pet_outputs")!=SQL_SUCCESS)break;
            bool conflict=false;
            const auto mail_result=pn_mail_asset_take(r);
            if(mail_result==pn_mail_asset_result::Error)break;
            conflict=mail_result==pn_mail_asset_result::Conflict;
            if(!conflict) {
                const auto sent=pn_mail_asset_send(r,mail_notification);
                if(sent==pn_mail_asset_result::Error)break;
                conflict=sent==pn_mail_asset_result::Conflict;
            }
            for(uint16_t i=0;i<r.pet_count && !conflict;++i) {
                const auto& change=r.pets[i];pn_pet::Result result;
                if(change.claim_id)result=pn_pet::claim_locked(r.account_id,r.char_id,change.claim_id,change.output.egg);
                else {
                    pn_pet::Key key{};key.account_id=r.account_id;key.char_id=r.char_id;
                    key.nonce_hi=r.nonce_hi;key.nonce_lo=r.nonce_lo;key.sequence=r.sequence;key.ordinal=i;
                    pn_pet::Entitlement issued{};result=pn_pet::issue_locked(key,change.output,issued);
                }
                if(result==pn_pet::Result::Error){valid=false;break;}
                if(result!=(change.claim_id?pn_pet::Result::ClaimedNow:pn_pet::Result::Issued)){conflict=true;break;}
            }
            std::vector<pn_shop::PetRetirement> retirements(r.retired_pets,r.retired_pets+r.pet_retire_count);
            std::sort(retirements.begin(),retirements.end(),[](const auto& a,const auto& b){return a.pet_id<b.pet_id;});
            if(valid && !conflict)for(const auto& retired:retirements) {
                const auto result=pn_pet::retire_locked(r.char_id,retired.pet_id,retired.egg_id);
                if(result==pn_pet::Result::Error){valid=false;break;}
                if(result!=pn_pet::Result::RetiredNow){conflict=true;break;}
            }
            if(!valid)break;
            if(conflict) {
                if(Sql_Query(sql_handle,"ROLLBACK TO SAVEPOINT pn_pet_outputs")!=SQL_SUCCESS ||
                   !pn_shop_receipt(r,pn_shop::Rejected,progression))break;
                outcome=pn_shop::Rejected;transaction_complete=true;break;
            }
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
        PnPurchaseCounts history_before;
        if(!pn_purchase_before(r,history_before))break;
        if(char_memitemdata_to_sql(r.items,MAX_INVENTORY,r.char_id,TABLE_INVENTORY,0)!=0)break;
        if(Sql_Query(sql_handle,"UPDATE `%s` SET zeny=%" PRId64 ",uniqueitem_counter=%u WHERE char_id=%u AND account_id=%u",schema_config.char_db,r.wallet_after,r.counter_after,r.char_id,r.account_id)!=SQL_SUCCESS)break;
        const int64 after[]={r.cash_after,r.kafra_after};
        for(int i=0;i<2;++i)if(Sql_Query(sql_handle,"REPLACE INTO `%s` (account_id,`key`,`index`,value) VALUES (%u,'%s',0,%" PRId64 ")",schema_config.acc_reg_num_table,r.account_id,currency_keys[i],after[i])!=SQL_SUCCESS){valid=false;break;}
        if(!valid || !pn_point_debit_locked(r))break;
        if(progression) {
            const auto progression_result=mapif_achievement_compare_replace_locked(r.char_id,
                progression->before.data(),static_cast<int32>(progression->before.size()),
                progression->after.data(),static_cast<int32>(progression->after.size()));
            if(progression_result==AchievementCompareReplaceResult::Error)break;
            if(progression_result==AchievementCompareReplaceResult::Conflict) {
                if(Sql_Query(sql_handle,"ROLLBACK TO SAVEPOINT pn_shop_outputs")!=SQL_SUCCESS ||
                   !pn_shop_receipt(r,pn_shop::ProgressionStale,progression))break;
                outcome=pn_shop::ProgressionStale;transaction_complete=true;break;
            }
        }
        if(!pn_shop_receipt(r,pn_shop::Committed,progression,&history_before))break;
        outcome=pn_shop::Committed;transaction_complete=new_commit=true;
    } while(false);
    // A failed COMMIT acknowledgement is indeterminate. An identical retry
    // resolves the receipt; no compensating write or success ACK is sent.
    const int ended=Sql_EndTransaction(sql_handle,transaction_complete);
    // A lost COMMIT reply can leave the cached character older than its DB
    // row. Receipt replay must not copy an old snapshot over newer state;
    // evict instead, so a later authentication reloads authoritative state.
    if((new_commit && ended!=SQL_SUCCESS) || (transaction_complete && outcome==pn_shop::Committed && !new_commit))
        char_get_chardb().erase(static_cast<uint32>(r.char_id));
    if(ended!=SQL_SUCCESS)return pn_shop::Retry;
    if(!transaction_complete)return pn_shop::Retry;
    if(new_commit) {
        if(r.kind==pn_shop::MailSend && mail_notification.id)mapif_Mail_new(&mail_notification);
        auto found=char_get_chardb().find(static_cast<uint32>(r.char_id));
        if(found!=char_get_chardb().end() && found->second) {
            found->second->zeny=r.wallet_after;
            found->second->uniqueitem_counter=r.counter_after;
        }
    }
    return outcome;
}

#include <custom/global_point_char.inc>

using PnShopProgressionKey=std::tuple<uint32,uint32,uint64,uint64,uint64>;
struct PnShopStagedProgression { int32 fd=0;pn_shop_progression::Bundle bundle; };
static std::map<PnShopProgressionKey,PnShopStagedProgression> pn_shop_progressions;
static void pn_shop_progression_disconnect_impl(int32 fd) {
    for(auto it=pn_shop_progressions.begin();it!=pn_shop_progressions.end();) {
        if(it->second.fd==fd)it=pn_shop_progressions.erase(it);else ++it;
    }
}
static PnShopProgressionKey pn_shop_progression_key(uint32 account,uint32 character,uint64 hi,uint64 lo,uint64 sequence) {
    return {account,character,hi,lo,sequence};
}
static bool pn_shop_current_owner(int32 fd,uint32 account,uint32 character) {
    auto online=char_get_onlinedb().find(account);
    if(online==char_get_onlinedb().end() || !online->second || online->second->char_id!=character)return false;
    int32 server=online->second->server;
    return server>=0 && server<MAX_MAP_SERVERS && map_server[server].fd==fd;
}
static void mapif_parse_ShopProgression(int32 fd) {
    const uint16 length=RFIFOW(fd,2);pn_shop_progression::Bundle bundle;
    if(!pn_shop_progression::decode(RFIFOP(fd,0),length,bundle) ||
       !pn_shop_current_owner(fd,bundle.header.account_id,bundle.header.char_id))return;
    const auto key=pn_shop_progression_key(bundle.header.account_id,bundle.header.char_id,
        bundle.header.nonce_hi,bundle.header.nonce_lo,bundle.header.sequence);
    auto found=pn_shop_progressions.find(key);
    if(found!=pn_shop_progressions.end()) {
        if(found->second.fd!=fd || found->second.bundle.bytes!=bundle.bytes)pn_shop_progressions.erase(found);
        return;
    }
    // Only authenticated map servers can reach this parser. Bound abandoned
    // staging if one sends companion frames without their matching commit.
    size_t owned=0;for(const auto& row:pn_shop_progressions)if(row.second.fd==fd)++owned;
    if(owned>=4)for(auto it=pn_shop_progressions.begin();it!=pn_shop_progressions.end();) {
        if(it->second.fd==fd)it=pn_shop_progressions.erase(it);else ++it;
    }
    pn_shop_progressions.emplace(key,PnShopStagedProgression{fd,std::move(bundle)});
}

static void mapif_parse_ShopCommit(int32 fd) {
    if(RFIFOW(fd,2)!=sizeof(pn_shop::Commit))return;
    pn_shop::Commit request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    const bool owner=pn_shop_current_owner(fd,request.account_id,request.char_id);
    const auto key=pn_shop_progression_key(request.account_id,request.char_id,request.nonce_hi,request.nonce_lo,request.sequence);
    auto staged=pn_shop_progressions.find(key);pn_shop_progression::Bundle* progression=nullptr;
    if(staged!=pn_shop_progressions.end() && staged->second.fd==fd)progression=&staged->second.bundle;
    pn_shop::Ack reply;reply.account_id=request.account_id;reply.char_id=request.char_id;
    reply.nonce_hi=request.nonce_hi;reply.nonce_lo=request.nonce_lo;reply.sequence=request.sequence;
    reply.outcome=pn_shop_tosql(request,owner,progression);
    if(reply.outcome!=pn_shop::Retry && staged!=pn_shop_progressions.end())pn_shop_progressions.erase(staged);
    if(reply.outcome==pn_shop::Retry && owner && request.point.scope==pn_shop::GlobalPoint)pn_global_request_barrier(request);
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
