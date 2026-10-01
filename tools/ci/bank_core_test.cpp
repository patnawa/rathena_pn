// Pure production arithmetic, exercised with boundary and randomized balances.
#include <custom/bank_protocol.hpp>
#include <cassert>
#include <iostream>
#include <random>
int main() {
    using namespace pn_bank;
    Reply wire; assert(valid_reply(wire));
    wire.buy[0]=0; assert(!valid_reply(wire)); wire=Reply{};
    wire.bank=-1; assert(!valid_reply(wire)); wire=Reply{};
    wire.wallet=-1; assert(!valid_reply(wire)); wire=Reply{};
    wire.max_deposit=1; assert(!valid_reply(wire)); wire=Reply{};
    wire.max_sell[0]=1; assert(!valid_reply(wire)); wire=Reply{};
    wire.wallet_limit=0; assert(!valid_reply(wire));
    wire=Reply{}; wire.flags=open_panel; assert(valid_reply(wire));
    wire.flags=2; assert(!valid_reply(wire));
    Reply s; s.bank=1500000000; s.wallet=1000000000;
    s.counts[0]=20; s.counts[1]=30000; s.max_buy[0]=20; s.max_buy[1]=30000;
    assert(plan(s,Deposit,1).bank==1500000001);
    assert(plan(s,Withdraw,1).wallet==1000000001);
    for(int64_t value : {int64_t(-1),int64_t(0),INT64_MAX,INT64_MIN})
        for(uint32_t action=Deposit;action<=SellNote;++action) assert(plan(s,action,value).result!=Ok);
    assert(plan(s,Deposit,1000000000).bank==2500000000LL);
    assert(plan(s,Withdraw,1500000000).wallet==2500000000LL);
    for(uint32_t action=BuyDiamond;action<=SellNote;++action)
        for(int64_t amount : {int64_t(1), int64_t(20), int64_t(INT32_MAX)}) {
            auto rejected=plan(s,action,amount);
            assert(rejected.result==Invalid && rejected.bank==s.bank && rejected.wallet==s.wallet && rejected.item_delta==0);
        }
    // Exact integer boundaries, including amounts a floating-point UI would round.
    for(int64_t balance : {int64_t(INT32_MAX), int64_t(1ULL<<53), bank_limit-1}) {
        Reply high; high.bank=balance; high.wallet=1;
        auto p=plan(high,Deposit,1); assert(p.result==Ok && p.bank==balance+1 && p.wallet==0);
        high.bank=p.bank; high.wallet=0;
        p=plan(high,Withdraw,1); assert(p.result==Ok && p.bank==balance && p.wallet==1);
    }
    Reply high; high.bank=bank_limit; high.wallet=1; high.counts[0]=1;
    assert(plan(high,Deposit,1).result==Limit && plan(high,SellDiamond,1).result==Invalid);
    high.wallet=wallet_limit; assert(plan(high,Withdraw,1).result==Limit);
    high.wallet=0; assert(plan(high,Withdraw,wallet_limit).wallet==wallet_limit);
    assert(plan(high,Withdraw,INT64_MIN).result==Invalid);
    std::mt19937_64 random(20260913);
    int successes=0;
    for(int i=0;i<500000;++i) {
        s.bank=static_cast<int64_t>(random() & INT64_MAX); s.wallet=static_cast<int64_t>(random() & INT64_MAX);
        if(i%4==0) s.bank=bank_limit-(random()%1000000000);
        if(i%4==1) s.bank=random()%3000000000ULL;
        s.counts[0]=random()%50; s.counts[1]=random()%30001;
        s.max_buy[0]=random()%50; s.max_buy[1]=random()%30001;
        uint32_t action=1+random()%6;
        int64_t amount=action<=Withdraw ? static_cast<int64_t>(random() & INT64_MAX) : random()%50000;
        auto p=plan(s,action,amount);
        if(p.result!=Ok) { assert(p.bank==s.bank && p.wallet==s.wallet); continue; }
        ++successes;
        assert(p.bank>=0 && p.bank<=bank_limit && p.wallet>=0 && p.wallet<=wallet_limit);
        if(action<=Withdraw) assert(p.bank-s.bank==s.wallet-p.wallet && p.item_delta==0);
        else assert(false); // Exchange operations must never reach mutation.

    }
    std::cout<<"PASS: 500000 randomized bank plans, edge limits, invalid amounts, direct-only exchange rejection; "<<successes<<" accepted plans\n";
}
