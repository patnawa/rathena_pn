// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  episode21_daily_clock_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/episode21_daily_clock_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// Appended to the native crown fixture's explicit world/transport boundaries.
// Real VM, gettimetick/gettime/gettimestr/atoi and native date conversion.
#include <ctime>

namespace {
bool clock_active=false;
std::vector<time_t> clock_values;
size_t clock_calls=0;
unsigned fixed_cases=0,stable_controls=0,original_defects=0;
void assert_zone(long offset){
    time_t anchor=1704067200; // 2024-01-01 00:00:00 UTC, before every test date.
    auto local=*std::localtime(&anchor);
    check(local.tm_gmtoff==offset&&local.tm_year==124&&local.tm_mon==0&&local.tm_mday==1,
          "named timezone exists with independently known modern UTC offset");
}

time_t civil(int year,int month,int day,int hour=0,int minute=0,int second=0){
    std::tm value{};value.tm_year=year-1900;value.tm_mon=month-1;
    value.tm_mday=day;value.tm_hour=hour;value.tm_min=minute;value.tm_sec=second;value.tm_isdst=-1;
    time_t result=std::mktime(&value);check(result>0,"positive native civil timestamp");return result;
}
time_t reference_key(time_t instant){
    auto value=*std::localtime(&instant);value.tm_hour=4;value.tm_min=value.tm_sec=0;value.tm_isdst=-1;
    time_t reset=std::mktime(&value);
    if(instant<reset){--value.tm_mday;value.tm_isdst=-1;reset=std::mktime(&value);}
    return reset;
}
int64 evaluate(script_code* probe,script_code* helper,time_t instant,int split,bool fixed){
    strdb_put(script_get_userfunc_db(),"EP21_DailyKey",helper);
    clock_values.assign(4,instant);
    if(split>0)for(size_t i=split;i<clock_values.size();++i)++clock_values[i];
    clock_calls=0;setnum("@daily_result",-1);clock_active=true;
    run_script(probe,0,attached->id,NPC);clock_active=false;
    check(!attached->st,"pure helper never suspends");
    check(errors==0,"native time helper has no script errors");
    check(clock_calls==(fixed?1u:4u),"exact native clock read count");
    return nums[add_str("@daily_result")];
}
}

extern "C" time_t __real_time(time_t*);
extern "C" time_t __wrap_time(time_t* result){
    if(!clock_active)return __real_time(result);
    check(clock_calls<clock_values.size(),"no unexpected native clock consumer");
    time_t value=clock_values[clock_calls++];if(result)*result=value;return value;
}

extern "C" int __wrap_main(int argc,char** argv){
    check(argc==2,"explicit isolated artifact directory");deny_network();
    static char name[]="episode21-daily-clock-test";SERVER_NAME=name;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    auto sd=std::make_unique<map_session_data>();attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;
    sd->status.inventory_slots=MAX_INVENTORY;sd->status.zeny=123456;
    const std::string dir=argv[1];
    auto* old=compile(read(dir+"/before.txt"),"genuine-original-EP21-DailyKey");
    auto* fixed=compile(read(dir+"/after.txt"),"snapshot-EP21-DailyKey");
    auto* probe=compile("{ @daily_result = callfunc(\"EP21_DailyKey\"); end; }","daily-return-probe");
    Snapshot unchanged;
    // Every second of a complete day in the observed Docker timezone (UTC)
    // and the user's timezone (Bangkok). The server timezone is not changed.
    for(const char* timezone:{"UTC","Asia/Bangkok"}){
        check(setenv("TZ",timezone,1)==0,"set isolated child timezone");tzset();
        assert_zone(std::strcmp(timezone,"UTC")==0?0:25200);
        time_t start=civil(2026,9,6);
        for(int second=0;second<86400;++second){
            time_t instant=start+second;
            check(evaluate(probe,fixed,instant,1,true)==reference_key(instant),"all UTC and Bangkok seconds have exact civil reset key");++fixed_cases;
        }
    }
    // Calendar, reset and fractional-offset boundaries; no DST semantics change
    // is claimed for the pre-existing fixed-86400 fallback policy.
    for(const char* timezone:{"UTC","Asia/Bangkok","Asia/Kathmandu"}){
        check(setenv("TZ",timezone,1)==0,"set isolated fixed-offset timezone");tzset();
        assert_zone(std::strcmp(timezone,"UTC")==0?0:std::strcmp(timezone,"Asia/Bangkok")==0?25200:20700);
        for(auto date:std::vector<std::vector<int>>{{2026,9,6},{2026,12,31},{2028,2,29}}){
            time_t day=civil(date[0],date[1],date[2]);
            for(int second:{0,1,59,60,3599,3600,14398,14399,14400,14401,45296,86398,86399}){
                time_t instant=day+second,expected=reference_key(instant);
                check(evaluate(probe,old,instant,0,false)==expected,"genuine stable-clock behavior preserved");++stable_controls;
                for(int split=0;split<4;++split){
                    check(evaluate(probe,fixed,instant,split,true)==expected,"snapshot ignores later second/minute/hour/calendar clock changes");++fixed_cases;
                }
            }
        }
        for(int second:{59,3599,14399,45296,86399})for(int split=1;split<4;++split){
            time_t instant=civil(2026,9,6)+second;
            auto observed=evaluate(probe,old,instant,split,false);
            check(observed!=reference_key(instant),"genuine original mixed-clock key reproduced");++original_defects;
        }
    }
    unchanged.unchanged();check(messages.empty()&&menu_text.empty()&&windows.empty(),"pure time helper has no UI effects");
    // The user-function DB owns the last registered helper; free the other once.
    strdb_put(script_get_userfunc_db(),"EP21_DailyKey",fixed);script_free_code(old);script_free_code(probe);
    attached=nullptr;sd.reset();nums.clear();strings.clear();
    do_final_script();timer_final();db_final();malloc_final();
    std::printf("EP21_DAILY_CLOCK_OK fixed_cases=%u stable_controls=%u original_defects=%u assertions=%u\n",fixed_cases,stable_controls,original_defects,assertions);
    return errors?1:0;
}
