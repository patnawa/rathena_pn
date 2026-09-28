#include "../../client-patch/wide_market/numbers.hpp"
#include <cassert>
#include <iostream>
int main() {
    using namespace pn_market_ui;
    int64_t parsed=0,result=0;
    for(auto amount:{2147483648LL,4294967296LL,9007199254740993LL,INT64_MAX}) {
        assert(parse_amount(format_amount(amount),parsed)&&parsed==amount);
        assert(total(amount,1,result)&&result==amount);
    }
    for(auto invalid:{L"",L"-1",L"9,22",L"1e6",L"9223372036854775808",L"1.5",L"1,000,"})assert(!parse_amount(invalid,parsed));
    assert(total(INT64_MAX/2,2,result)&&result==INT64_MAX-1);
    assert(!total(INT64_MAX/2+1,2,result));assert(!total(1,0,result));
    assert(total(9007199254740993LL,3,result)&&result==27021597764222979LL);
    std::cout<<"PASS: exact market input and quantity-times-price boundary checks\n";
}
