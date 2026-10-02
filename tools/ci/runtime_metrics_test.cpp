#include <common/runtime_metrics.hpp>
#include <cassert>
#include <chrono>
#include <iostream>
int main() {
    pn_metrics::Histogram histogram;
    for(unsigned i=1;i<=100;++i)histogram.observe(i);
    assert(histogram.percentile(50)==50 && histogram.percentile(95)==100 && histogram.percentile(99)==100);
    histogram={};histogram.observe(70000);assert(histogram.percentile(99)==70000);
    pn_metrics::Runtime metrics;
    assert(!metrics.report_due(1000));metrics.shop_started(2000);
    metrics.shop_queue_depth=32;
    metrics.shop_acknowledged(2250);metrics.shop_acknowledged(3000);
    assert(metrics.shop_ack.count==1 && metrics.shop_ack.maximum==250);
    metrics.shop_refresh_failures=2;metrics.shop_retry_attempts=3;
    assert(metrics.pending);assert(metrics.report_due(61000));
    metrics.timer_late.observe(2000);
    const auto pending=metrics.report(61000,1700000000);
    assert(pending.find("\"version\":2")!=std::string::npos);
    assert(pending.find("\"shop_queue_depth\":32")!=std::string::npos);
    assert(pending.find("\"shop_oldest_ms\":59000")!=std::string::npos);
    assert(pending.find("\"timer_late_max_ms\":2000")!=std::string::npos);
    assert(metrics.pending && !metrics.timer_late.count && metrics.shop_started_count==1);
    metrics.shop_finished(62000,true);metrics.shop_finished(63000,true);
    assert(metrics.shop_committed==1 && metrics.shop_complete.maximum==60000 && !metrics.pending);
    metrics.shop_started(64000);metrics.shop_finished(64500,false);
    assert(metrics.shop_rejected==1);
    metrics.shop_queue_depth=0;
    const auto idle=metrics.report(64500,1700000003);
    assert(idle.find("\"shop_pending\":0")!=std::string::npos);
    assert(idle.find("\"shop_queue_depth\":0")!=std::string::npos);
    assert(idle.find("\"shop_oldest_ms\":0")!=std::string::npos);
    const auto start=std::chrono::steady_clock::now();
    for(unsigned i=0;i<1000000;++i)metrics.timer_late.observe(i%1500);
    const auto micros=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
    assert(metrics.timer_late.count==1000000);
    std::cout << "METRICS_PASS million_observations_us=" << micros << '\n' << pending << '\n' << idle << '\n';
}
