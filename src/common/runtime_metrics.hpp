// Bounded, process-local measurements. No player identities or dynamic labels.
#ifndef PN_RUNTIME_METRICS_HPP
#define PN_RUNTIME_METRICS_HPP
#include <algorithm>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>

namespace pn_metrics {
struct Histogram {
    static constexpr std::array<uint64_t,12> limits{{0,1,5,10,25,50,100,250,500,1000,5000,60000}};
    std::array<uint64_t,13> bins{};
    uint64_t count=0,maximum=0;
    void observe(uint64_t value) {
        const auto index=std::lower_bound(limits.begin(),limits.end(),value)-limits.begin();
        ++bins[index];++count;maximum=std::max(maximum,value);
    }
    uint64_t percentile(unsigned percent) const {
        if(!count)return 0;
        const uint64_t rank=(count/100)*percent+((count%100)*percent+99)/100;
        uint64_t seen=0;
        for(size_t i=0;i<bins.size();++i) {
            seen+=bins[i];
            if(seen>=rank)return i<limits.size()?std::min(limits[i],maximum):maximum;
        }
        return maximum;
    }
};
struct Runtime {
    Histogram timer_late,dispatch,shop_ack,shop_complete;
    uint64_t shop_started_count=0,shop_committed=0,shop_rejected=0;
    uint64_t shop_retry_attempts=0,shop_refresh_failures=0,shop_busy_refusals=0;
    uint32_t shop_queue_depth=0;
    bool pending=false,ack_seen=false;
    int64_t pending_since=0,last_report=0;
    static uint64_t elapsed(int64_t now,int64_t before) {
        return now>=before?static_cast<uint64_t>(now-before):0;
    }
    void shop_started(int64_t now) {
        pending=true;ack_seen=false;pending_since=now;++shop_started_count;
    }
    void shop_acknowledged(int64_t now) {
        if(pending && !ack_seen){shop_ack.observe(elapsed(now,pending_since));ack_seen=true;}
    }
    void shop_finished(int64_t now,bool committed) {
        if(!pending)return;
        shop_complete.observe(elapsed(now,pending_since));
        committed?++shop_committed:++shop_rejected;
        pending=false;ack_seen=false;
    }
    bool report_due(int64_t now) {
        if(!last_report){last_report=now;return false;}
        return elapsed(now,last_report)>=60000;
    }
    std::string report(int64_t now,int64_t utc_seconds) {
        std::ostringstream out;
        out << "{\"version\":2,\"utc\":" << utc_seconds
            << ",\"window_ms\":" << elapsed(now,last_report)
            << ",\"shop_pending\":" << (pending?1:0)
            << ",\"shop_queue_depth\":" << shop_queue_depth
            << ",\"shop_oldest_ms\":" << (pending?elapsed(now,pending_since):0)
            << ",\"shop_started\":" << shop_started_count
            << ",\"shop_committed\":" << shop_committed
            << ",\"shop_rejected\":" << shop_rejected
            << ",\"shop_retry_attempts\":" << shop_retry_attempts
            << ",\"shop_refresh_failures\":" << shop_refresh_failures
            << ",\"shop_busy_refusals\":" << shop_busy_refusals;
        for(auto pair:{std::make_pair("timer_late",&timer_late),std::make_pair("dispatch",&dispatch),
                       std::make_pair("shop_ack",&shop_ack),std::make_pair("shop_complete",&shop_complete)}) {
            out << ",\"" << pair.first << "_count\":" << pair.second->count
                << ",\"" << pair.first << "_p50_ms\":" << pair.second->percentile(50)
                << ",\"" << pair.first << "_p95_ms\":" << pair.second->percentile(95)
                << ",\"" << pair.first << "_p99_ms\":" << pair.second->percentile(99)
                << ",\"" << pair.first << "_max_ms\":" << pair.second->maximum;
            *pair.second={};
        }
        out << '}';last_report=now;return out.str();
    }
};
inline Runtime runtime;
}
#endif
