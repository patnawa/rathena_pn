#include <custom/bank_sweep.hpp>
#include <vector>
static void bank_sweep_tosql(const pn_bank_sweep& r,pn_bank_sweep_ack& reply) {
    if (!r.account_id || !r.char_id || !r.request_id || !(r.nonce_hi|r.nonce_lo) || r.bank_before<0) {reply.result=pn_bank::SaveFailed;return;}
    if (!bank_tables_transactional()) return;
    if (Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='pn_bank_sweep_commits'")!=SQL_SUCCESS) return;
    char* value=nullptr;bool engine=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&value,nullptr)==SQL_SUCCESS && value && !strcmpi(value,"InnoDB");
    Sql_FreeResult(sql_handle);if (!engine || Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS) return;
    bool success=false,wrote=false;
    struct Wallet {uint32 id;int64 zeny;bool online;};
    std::vector<Wallet> wallets;std::vector<uint32> collected;
    do {
        if (Sql_Query(sql_handle,"SELECT char_id,bank_before,bank_after,collected,skipped FROM pn_bank_sweep_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND request_id=%" PRIu64 " FOR UPDATE",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id)!=SQL_SUCCESS) break;
        int row=Sql_NextRow(sql_handle);
        if (row==SQL_SUCCESS) {
            int64 fields[5]={};bool valid=true;
            for (int i=0;i<5;++i) {valid&=Sql_GetData(sql_handle,i,&value,nullptr)==SQL_SUCCESS && value;if (value) fields[i]=strtoll(value,nullptr,10);}
            success=valid && fields[0]==r.char_id && fields[1]==r.bank_before;
            if (success) {reply.bank_after=fields[2];reply.collected=static_cast<uint32>(fields[3]);reply.skipped=static_cast<uint32>(fields[4]);}
            Sql_FreeResult(sql_handle);break;
        }
        Sql_FreeResult(sql_handle);if (row!=SQL_NO_DATA) break;
        if (Sql_Query(sql_handle,"SELECT char_id,zeny,online FROM `%s` WHERE account_id=%u ORDER BY char_id FOR UPDATE",schema_config.char_db,r.account_id)!=SQL_SUCCESS) break;
        bool owner=false,valid=true;
        while (Sql_NextRow(sql_handle)==SQL_SUCCESS) {
            Wallet wallet{};
            Sql_GetData(sql_handle,0,&value,nullptr);if (!value) {valid=false;break;}wallet.id=static_cast<uint32>(strtoul(value,nullptr,10));
            Sql_GetData(sql_handle,1,&value,nullptr);if (!value) {valid=false;break;}wallet.zeny=strtoll(value,nullptr,10);
            Sql_GetData(sql_handle,2,&value,nullptr);wallet.online=value && strtol(value,nullptr,10)!=0;
            if (wallet.zeny<0) {valid=false;break;}
            if (wallet.id==r.char_id) owner=true;
            wallets.push_back(wallet);
        }
        Sql_FreeResult(sql_handle);if (!valid || !owner) break;
        if (Sql_Query(sql_handle,"SELECT value FROM `%s` WHERE account_id=%u AND `key`='#BANKVAULT' AND `index`=0 FOR UPDATE",schema_config.acc_reg_num_table,r.account_id)!=SQL_SUCCESS) break;
        int64 bank=0;row=Sql_NextRow(sql_handle);
        if (row==SQL_SUCCESS) {Sql_GetData(sql_handle,0,&value,nullptr);if (value) bank=strtoll(value,nullptr,10);else valid=false;}
        else if (row!=SQL_NO_DATA) valid=false;
        Sql_FreeResult(sql_handle);
        if (!valid || bank!=r.bank_before) {reply.result=pn_bank::SaveFailed;break;}
        const auto online=char_get_onlinedb().find(r.account_id);
        for (const auto& wallet:wallets) {
            const bool active=online!=char_get_onlinedb().end() && online->second && online->second->char_id==wallet.id &&
                (online->second->server!=-1 || online->second->fd>=0 || online->second->waiting_disconnect!=INVALID_TIMER);
            if (wallet.id==r.char_id || wallet.online || active) {++reply.skipped;continue;}
            if (!wallet.zeny) continue;
            if (wallet.zeny>pn_bank::bank_limit-bank) {++reply.skipped;continue;}
            bank+=wallet.zeny;collected.push_back(wallet.id);
        }
        bool updated=true;
        for (auto cid:collected)
            if (Sql_Query(sql_handle,"UPDATE `%s` SET zeny=0 WHERE char_id=%u AND account_id=%u AND online=0",schema_config.char_db,cid,r.account_id)!=SQL_SUCCESS) {updated=false;break;}
        if (!updated) break;
        if (Sql_Query(sql_handle,"REPLACE INTO `%s` (account_id,`key`,`index`,value) VALUES (%u,'#BANKVAULT',0,%" PRId64 ")",schema_config.acc_reg_num_table,r.account_id,bank)!=SQL_SUCCESS) break;
        reply.bank_after=bank;reply.collected=static_cast<uint32>(collected.size());
        if (Sql_Query(sql_handle,"INSERT INTO pn_bank_sweep_commits (account_id,nonce_hi,nonce_lo,request_id,char_id,bank_before,bank_after,collected,skipped) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%" PRId64 ",%" PRId64 ",%u,%u)",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id,r.char_id,r.bank_before,bank,reply.collected,reply.skipped)!=SQL_SUCCESS) break;
        success=wrote=true;
    } while(false);
    if (Sql_EndTransaction(sql_handle,success)!=SQL_SUCCESS) success=false;
    if (success) {
        reply.result=pn_bank::Ok;
        if (wrote) for (auto cid:collected) {
            auto found=char_get_chardb().find(cid);if (found!=char_get_chardb().end() && found->second) found->second->zeny=0;
        }
    }
}
static void mapif_parse_BankSweep(int32 fd) {
    if (RFIFOW(fd,2)!=sizeof(pn_bank_sweep)) return;
    pn_bank_sweep request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    pn_bank_sweep_ack reply;reply.account_id=request.account_id;reply.char_id=request.char_id;
    reply.nonce_hi=request.nonce_hi;reply.nonce_lo=request.nonce_lo;reply.request_id=request.request_id;
    bank_sweep_tosql(request,reply);
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
