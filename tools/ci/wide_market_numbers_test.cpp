#include "../../client-patch/wide_market/numbers.hpp"
#include <cassert>
#include <iostream>
#include <random>
#include "../../src/custom/market_protocol.hpp"
int main() {
    using namespace pn_market_ui;
    int64_t parsed=0,result=0;
    const int64_t amounts[]={2147483648LL,4294967296LL,9007199254740993LL,INT64_MAX};
    for(auto amount:amounts) {
        assert(parse_amount(format_amount(amount),parsed)&&parsed==amount);
        assert(total(amount,1,result)&&result==amount);
    }
    for(auto invalid:{L"",L"-1",L"9,22",L"1e6",L"9223372036854775808",L"1.5",L"1,000,"})assert(!parse_amount(invalid,parsed));
    assert(total(INT64_MAX/2,2,result)&&result==INT64_MAX-1);
    assert(!total(INT64_MAX/2+1,2,result));assert(!total(1,0,result));
    assert(total(9007199254740993LL,3,result)&&result==27021597764222979LL);
    std::mt19937_64 random(20261006);
    for(int i=0;i<200000;++i){
        const int64_t amount=static_cast<int64_t>(random()&INT64_MAX);
        assert(parse_amount(format_amount(amount),parsed)&&parsed==amount);
        const uint32_t quantity=static_cast<uint32_t>(random())|1;
        const int64_t boundary=INT64_MAX/quantity;
        assert(total(boundary,quantity,result)&&result>=0&&result<=INT64_MAX);
        if(quantity>1)assert(!total(boundary+1,quantity,result));
    }
    pn_market::Reply reply;reply.wallet=reply.budget=INT64_MAX;reply.count=pn_market::page_size;
    for(auto& entry:reply.entries){entry.item_id=501;entry.quantity=UINT32_MAX;entry.price=INT64_MAX;}
    assert(pn_market::valid_reply(reply));
    for(auto count:{21u,UINT32_MAX}){reply.count=count;assert(!pn_market::valid_reply(reply));}
    reply.count=pn_market::page_size;reply.entries[19].price=INT64_MIN;assert(!pn_market::valid_reply(reply));
    assert(!parse_amount(L"9223372036854775808",parsed));
    assert(!total(INT64_MIN,UINT32_MAX,result));
    std::cout<<"PASS: 200000 seeded round-trips and multiplication boundaries; oversized catalogs and negative wire prices rejected\n";
    std::cout<<"PASS: exact market input and quantity-times-price boundary checks\n";
}
