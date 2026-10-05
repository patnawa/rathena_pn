#pragma once
#include "../wide_zeny/bank_client.hpp"
#include "mail_protocol.hpp"
constexpr UINT MAIL_RESULT=WM_APP+81;
struct MailResult {pn_mail::Reply state;LONG generation=0;bool connected=false;};
inline bool valid_mail_reply(const pn_mail::Reply& reply) {
    if(reply.magic_value!=pn_mail::magic||reply.protocol!=pn_mail::version||reply.length!=sizeof(reply)||
        reply.result>pn_mail::Failed||reply.wallet<0||reply.amount<0||reply.fee<0||reply.total<0||reply.limit!=INT64_MAX||reply.pending>1)return false;
    if(reply.result==pn_mail::Ok||reply.result==pn_mail::Pending)
        return reply.fee<=INT64_MAX-reply.amount&&reply.total==reply.amount+reply.fee;
    return true;
}
bool mail_submit(HWND panel,pn_mail::Request request);
void mail_open(HWND owner,const pn_bank::Reply& bank);
void mail_forget_session();
