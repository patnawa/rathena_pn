// Zeny-only companion mail: one transaction owns mail, wallet and replay receipt.
#include <custom/mail_commit.hpp>
#include <custom/zeny_arithmetic.hpp>
#include <char/int_mail.hpp>
static std::string pn_mail_payload(const pn_mail::Request& request) {
    static const char digits[]="0123456789ABCDEF";
    const auto* bytes=reinterpret_cast<const unsigned char*>(&request.amount);
    std::string out;out.reserve(1160);
    for (size_t i=0;i<580;++i) {out.push_back(digits[bytes[i]>>4]);out.push_back(digits[bytes[i]&15]);}
    return out;
}
static uint32 pn_mail_tosql(const pn_mail_commit& commit) {
    const auto& r=commit.request;
    std::int64_t total;
    if (!r.account_id || !r.char_id || !r.request_id || !(r.nonce_hi|r.nonce_lo) || r.action!=pn_mail::Send ||
        r.amount<=0 || !r.recipient[0] || !r.title[0] || r.recipient[23] || r.title[39] || r.body[499] ||
        !pn_zeny::fee_total(r.amount,commit.fee_percent,0,total) || total!=r.expected_total || commit.wallet_before<total)
        return pn_mail::Failed;
    const char* tables[]={schema_config.char_db,schema_config.mail_db,"pn_mail_commits"};
    for (const char* table : tables) {
        if (Sql_Query(sql_handle,"SELECT ENGINE FROM information_schema.TABLES WHERE TABLE_SCHEMA=DATABASE() AND TABLE_NAME='%s'",table)!=SQL_SUCCESS) return pn_mail::Pending;
        char* value=nullptr;bool good=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&value,nullptr)==SQL_SUCCESS && value && !strcmpi(value,"InnoDB");
        Sql_FreeResult(sql_handle);if (!good) return pn_mail::Pending;
    }
    if (Sql_BeginTransaction(sql_handle)!=SQL_SUCCESS) return pn_mail::Pending;
    bool success=false,wrote=false;uint32 result=pn_mail::Pending;
    mail_message notification{};
    const auto payload=pn_mail_payload(r);
    do {
        if (Sql_Query(sql_handle,"SELECT char_id,wallet_before,HEX(payload),result FROM pn_mail_commits WHERE account_id=%u AND nonce_hi=%" PRIu64 " AND nonce_lo=%" PRIu64 " AND request_id=%" PRIu64 " FOR UPDATE",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id)!=SQL_SUCCESS) break;
        int row=Sql_NextRow(sql_handle);
        if (row==SQL_SUCCESS) {
            char *cid=nullptr,*wallet=nullptr,*stored=nullptr,*outcome=nullptr;
            success=Sql_GetData(sql_handle,0,&cid,nullptr)==SQL_SUCCESS && cid && strtoul(cid,nullptr,10)==r.char_id &&
                Sql_GetData(sql_handle,1,&wallet,nullptr)==SQL_SUCCESS && wallet && strtoll(wallet,nullptr,10)==commit.wallet_before &&
                Sql_GetData(sql_handle,2,&stored,nullptr)==SQL_SUCCESS && stored && payload==stored &&
                Sql_GetData(sql_handle,3,&outcome,nullptr)==SQL_SUCCESS && outcome;
            if (success) result=static_cast<uint32>(strtoul(outcome,nullptr,10));
            Sql_FreeResult(sql_handle);break;
        }
        Sql_FreeResult(sql_handle);if (row!=SQL_NO_DATA) break;
        char sender[NAME_LENGTH]={};
        if (Sql_Query(sql_handle,"SELECT name FROM `%s` WHERE char_id=%u AND account_id=%u FOR UPDATE",schema_config.char_db,r.char_id,r.account_id)!=SQL_SUCCESS) break;
        char* value=nullptr;bool owner=Sql_NextRow(sql_handle)==SQL_SUCCESS && Sql_GetData(sql_handle,0,&value,nullptr)==SQL_SUCCESS && value;
        if (owner) safestrncpy(sender,value,sizeof(sender));
        Sql_FreeResult(sql_handle);if (!owner) break;
        char escaped[NAME_LENGTH*2+1];Sql_EscapeString(sql_handle,escaped,r.recipient);
        if (Sql_Query(sql_handle,"SELECT char_id,name FROM `%s` WHERE name='%s' LIMIT 1",schema_config.char_db,escaped)!=SQL_SUCCESS) break;
        uint32 recipient=0;char recipient_name[NAME_LENGTH]={};
        if (Sql_NextRow(sql_handle)==SQL_SUCCESS) {
            Sql_GetData(sql_handle,0,&value,nullptr);if (value) recipient=static_cast<uint32>(strtoul(value,nullptr,10));
            Sql_GetData(sql_handle,1,&value,nullptr);if (value) safestrncpy(recipient_name,value,sizeof(recipient_name));
        }
        Sql_FreeResult(sql_handle);
        // Rejections also get a receipt, so retry cannot change its outcome.
        result=recipient ? pn_mail::Ok : pn_mail::Failed;
        if (recipient) {
            SqlStmt stmt{*sql_handle};
            if (stmt.Prepare("INSERT INTO `%s` (send_name,send_id,dest_name,dest_id,title,message,time,status,zeny,type) VALUES (?,%u,?,%u,?,?,%" PRIu64 ",0,%" PRId64 ",0)",schema_config.mail_db,r.char_id,recipient,static_cast<uint64>(time(nullptr)),r.amount)!=SQL_SUCCESS ||
                stmt.BindParam(0,SQLDT_STRING,sender,strlen(sender))!=SQL_SUCCESS ||
                stmt.BindParam(1,SQLDT_STRING,recipient_name,strlen(recipient_name))!=SQL_SUCCESS ||
                stmt.BindParam(2,SQLDT_STRING,const_cast<char*>(r.title),strlen(r.title))!=SQL_SUCCESS ||
                stmt.BindParam(3,SQLDT_STRING,const_cast<char*>(r.body),strlen(r.body))!=SQL_SUCCESS || stmt.Execute()!=SQL_SUCCESS) break;
            notification.id=static_cast<int32>(stmt.LastInsertId());notification.dest_id=recipient;
            safestrncpy(notification.send_name,sender,sizeof(notification.send_name));
            safestrncpy(notification.title,r.title,sizeof(notification.title));
            notification.type=MAIL_INBOX_NORMAL;
            if (Sql_Query(sql_handle,"UPDATE `%s` SET zeny=%" PRId64 " WHERE char_id=%u AND account_id=%u",schema_config.char_db,commit.wallet_before-total,r.char_id,r.account_id)!=SQL_SUCCESS) break;
        }
        if (Sql_Query(sql_handle,"INSERT INTO pn_mail_commits (account_id,nonce_hi,nonce_lo,request_id,char_id,wallet_before,payload,result) VALUES (%u,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%u,%" PRId64 ",UNHEX('%s'),%u)",r.account_id,r.nonce_hi,r.nonce_lo,r.request_id,r.char_id,commit.wallet_before,payload.c_str(),result)!=SQL_SUCCESS) break;
        success=wrote=true;
    } while(false);
    if (Sql_EndTransaction(sql_handle,success)!=SQL_SUCCESS) success=false;
    if (success && wrote && result==pn_mail::Ok) {
        auto found=char_get_chardb().find(r.char_id);
        if (found!=char_get_chardb().end() && found->second) found->second->zeny=commit.wallet_before-total;
        mapif_Mail_new(&notification);
    }
    return success?result:pn_mail::Pending;
}
static void mapif_parse_MailCompanionCommit(int32 fd) {
    if (RFIFOW(fd,2)!=sizeof(pn_mail_commit)) return;
    pn_mail_commit request;memcpy(&request,RFIFOP(fd,0),sizeof(request));
    pn_mail_ack reply;reply.account_id=request.request.account_id;reply.char_id=request.request.char_id;
    reply.nonce_hi=request.request.nonce_hi;reply.nonce_lo=request.request.nonce_lo;reply.request_id=request.request.request_id;
    reply.result=pn_mail_tosql(request);
    WFIFOHEAD(fd,sizeof(reply));memcpy(WFIFOP(fd,0),&reply,sizeof(reply));WFIFOSET(fd,sizeof(reply));
}
