#include "../../src/custom/zeny_arithmetic.hpp"
#include <cassert>
#include <iostream>
int main() {
    using pn_zeny::limit;
    std::int64_t total = -1;
    assert(pn_zeny::fee_total(0,0,0,total) && total==0);
    assert(pn_zeny::fee_total(99,1,0,total) && total==99);
    assert(pn_zeny::fee_total(100,1,0,total) && total==101);
    assert(pn_zeny::fee_total(2147483648LL,2,5,total) && total==2190433325LL);
    assert(pn_zeny::fee_total(9007199254740993LL,2,5,total) && total==9187343239835817LL);
    assert(pn_zeny::fee_total(limit,0,0,total) && total==limit);
    assert(pn_zeny::fee_total(limit/2,100,1,total) && total==limit);
    assert(!pn_zeny::fee_total(limit/2,100,2,total));
    assert(!pn_zeny::fee_total(limit,1,0,total));
    assert(!pn_zeny::fee_total(limit,0,1,total));
    assert(!pn_zeny::fee_total(-1,0,0,total));
    assert(!pn_zeny::fee_total(1,-1,0,total));
    assert(!pn_zeny::fee_total(1,0,-1,total));
    assert(!pn_zeny::fee_total(limit,limit,0,total));
    assert(pn_zeny::room(0,0,limit));
    assert(pn_zeny::room(limit-10,7,3));
    assert(!pn_zeny::room(limit-10,7,4));
    assert(!pn_zeny::room(limit,1,0));
    assert(!pn_zeny::room(-1,0,0));
    assert(!pn_zeny::room(0,-1,0));
    assert(!pn_zeny::room(0,0,-1));
    assert(!pn_zeny::room(limit,limit,limit));
    std::cout << "PASS: exact 64-bit fees, rounding, signed maximum, overflow rejection and pending-credit reservations\n";
}
