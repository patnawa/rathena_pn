#include "../../client-patch/wide_zeny/native_zeny.cpp"
#include <cassert>
#include <thread>
#include <iostream>
volatile LONG fixture_generation=7;
bool bank_current_generation(LONG generation,bool){return generation==InterlockedCompareExchange(&fixture_generation,0,0);}
int wmain(int argc,wchar_t** argv) {
    using namespace native_zeny_detail;Snapshot result;
    native_zeny_reset();assert(!read_snapshot(result));
    native_zeny_update(7,INT64_MAX,true);assert(read_snapshot(result)&&result.wallet==INT64_MAX&&result.generation==7);
    native_zeny_update(7,0,true);assert(read_snapshot(result)&&result.wallet==0);
    native_zeny_update(7,-1,true);assert(!read_snapshot(result));
    native_zeny_update(7,9007199254740993LL,true);assert(read_snapshot(result)&&result.wallet==9007199254740993LL);
    InterlockedExchange(&fixture_generation,8);assert(!read_snapshot(result)&&!result.valid&&result.wallet==0);
    native_zeny_update(8,16709925001LL,true);native_zeny_update(7,123,true);native_zeny_update(7,0,false);
    assert(read_snapshot(result)&&result.wallet==16709925001LL&&result.generation==8);
    native_zeny_update(8,0,false);assert(!read_snapshot(result));
    native_zeny_update(0,INT64_MAX,true);assert(!read_snapshot(result));
    native_zeny_update(8,INT64_MAX,true);native_zeny_reset();assert(!read_snapshot(result));
    // The real DLL is 32-bit: a naked 64-bit field can tear between these values.
    constexpr int64_t a=0x1111111166666666LL,b=0x7777777722222222LL;
    native_zeny_update(8,a,true);
    std::thread writer([](){for(unsigned i=0;i<100000;++i)native_zeny_update(8,i&1?a:b,true);});
    for(unsigned i=0;i<100000;++i){assert(read_snapshot(result));assert(result.wallet==a||result.wallet==b);}
    writer.join();native_zeny_reset();
    assert(!pinned_executable(L"missing-native-zeny-test.exe"));
    wchar_t folder[MAX_PATH],unknown[MAX_PATH];GetTempPathW(MAX_PATH,folder);GetTempFileNameW(folder,L"pnz",0,unknown);
    assert(!pinned_executable(unknown));DeleteFileW(unknown);
    assert(argc==2&&pinned_executable(argv[1]));
    const auto before_text=original_text;const auto before_measure=original_measure;
    native_zeny_install();assert(!read_snapshot(result));
    assert(!InterlockedCompareExchange(&executable_verified,0,0)&&original_text==before_text&&original_measure==before_measure);
    std::cout<<"PASS: exact int64 native cache, atomic concurrent x86 reads, session reset/stale response exclusion and pinned executable hash guard; unknown executable installs no hooks\n";
    return 0;
}
