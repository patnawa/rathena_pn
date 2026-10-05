#ifndef PN_MAIL_STATE_HPP
#define PN_MAIL_STATE_HPP
#include "mail_protocol.hpp"
struct pn_mail_state {
    pn_mail::Request request{};
    uint32_t result=pn_mail::Ok;
    int64_t total=0;
    int64_t wallet_before=0;
    uint32_t fee_percent=0;
    bool pending=false,applying=false;
};
#endif
