#include "turbo_core.hpp"
#include <cassert>
#include <cstdio>
using namespace pn_turbo;
int main(){
    Core c;c.bindings[112]={Smart,10};c.bindings[116]={Repeat,75};c.bindings[2]={AltRight,100};
    assert(!c.event(112,true,true,0));assert(c.next(100,true).mode==Off);c.event(112,false,true,100);
    c.enable(true);assert(c.event(112,true,true,101));auto a=c.next(101,true);assert(a.mode==Smart&&a.key==112);
    assert(c.next(109,true).mode==Off);c.completed(a,120);assert(c.next(129,true).mode==Off);assert(c.next(130,true).key==112);
    c.event(112,false,true,131);assert(c.next(200,true).mode==Off);
    // OS autorepeat is consumed; one held key has one schedule, not one per event.
    assert(c.event(116,true,true,200));assert(c.event(116,true,true,201));a=c.next(201,true);assert(a.key==116);
    assert(c.next(250,true).mode==Off);assert(c.next(10000,true).key==116);assert(c.next(10000,true).mode==Off);
    // Focus/modifier/session loss requires a physical release before resuming.
    c.next(11000,false);assert(c.next(12000,true).mode==Off);assert(c.event(116,true,true,12001));assert(c.next(13000,true).mode==Off);
    c.event(116,false,false,13001);assert(c.event(116,true,true,13002));assert(c.next(13002,true).key==116);
    c.enable(false);assert(c.next(14000,true).mode==Off);c.enable(true);assert(c.next(15000,true).mode==Off);
    assert(c.event(116,false,false,15001));assert(c.event(116,true,true,15002));assert(c.next(15002,true).key==116);
    c.event(116,false,true,15003);
    // Keys first pressed outside the game are never adopted on focus return.
    assert(!c.event(112,true,false,16000));assert(c.next(17000,true).mode==Off);c.event(112,false,true,17001);
    assert(c.event(112,true,true,17002));assert(c.event(116,true,true,17002));
    auto first=c.next(17002,true),second=c.next(17002,true);assert(first.key!=second.key&&first.key&&second.key);
    c.event(112,false,true,17003);c.event(116,false,true,17003);
    assert(c.event(2,true,true,18000));assert(c.next(18000,true).mode==AltRight);c.interrupt();assert(c.next(19000,true).mode==Off);assert(c.event(2,false,false,19001));
    puts("PASS: held-only repeat, smart mode, per-key delays, no backlog bursts, toggle, focus/modifier/session suspension, release/rearm, fairness, Alt-right-click");
}
