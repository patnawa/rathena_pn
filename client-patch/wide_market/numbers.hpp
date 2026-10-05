// Exact client-side presentation checks. The server remains authoritative.
#pragma once
#include <cstdint>
#include <string>
#include <limits>

namespace pn_market_ui {
inline bool parse_amount(const std::wstring& text,int64_t& value) {
    value=0;if(text.empty())return false;
    int digits=0;bool grouped=false;
    for(wchar_t c:text) {
        if(c==L',') {
            if(!digits || (grouped?digits!=3:digits>3))return false;
            grouped=true;digits=0;continue;
        }
        if(c<L'0'||c>L'9'||value>(INT64_MAX-(c-L'0'))/10)return false;
        value=value*10+(c-L'0');++digits;
    }
    return !grouped||digits==3;
}
inline std::wstring format_amount(int64_t value) {
    if(value<0)return L"--";
    auto text=std::to_wstring(value);
    for(int i=static_cast<int>(text.size())-3;i>0;i-=3)text.insert(i,L",");
    return text;
}
inline bool total(int64_t price,uint32_t quantity,int64_t& output) {
    output=0;
    if(price<0 || !quantity || price>INT64_MAX/quantity)return false;
    output=price*quantity;return true;
}
}
