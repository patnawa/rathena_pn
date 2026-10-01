// GPL-3.0-or-later. Production progression and condition VM; isolated player and
// packet boundaries. No SQL or game-server startup; kernel denies networking.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <vector>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/socket.h>
#include <unistd.h>
#include <common/core.hpp>
#include <common/database.hpp>
#include <common/db.hpp>
#include <common/ers.hpp>
#include <common/malloc.hpp>
#include <common/timer.hpp>
#include <custom/achievement_protocol.hpp>
#include <map/achievement.hpp>
#include <map/battle.hpp>
#include <map/clif.hpp>
#include <map/npc.hpp>
#include <map/pc.hpp>
#include <map/script.hpp>

namespace {
map_session_data* attached = nullptr;
unsigned assertions = 0, errors = 0, updates = 0, lists = 0, eof_calls = 0;
void check(bool good, const char* message) {
    ++assertions;
    if (!good) { std::fprintf(stderr, "SHOP PROGRESSION FAIL: %s\n", message); std::exit(1); }
}
void deny_network() {
    sock_filter rules[] = {BPF_STMT(BPF_LD|BPF_W|BPF_ABS, offsetof(seccomp_data,nr)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, __NR_socket,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, __NR_connect,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, __NR_bind,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K, __NR_listen,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
    sock_fprog program{static_cast<unsigned short>(std::size(rules)), rules};
    check(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0, "no-new-privileges");
    check(prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&program)==0, "network denial installed");
    check(syscall(SYS_socket,AF_INET,SOCK_STREAM,0)==-1 && errno==EPERM, "network denied");
}
std::string read(const std::string& path) {
    std::ifstream input(path); check(input.good(), "exact fixture opens");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}
const achievement* row(const std::vector<achievement>& rows, int32 id) {
    for (const auto& value: rows) if (value.achievement_id==id) return &value;
    return nullptr;
}
script_reg_num* argument(map_session_data& sd, int i) {
    return static_cast<script_reg_num*>(i64db_get(sd.regs.vars, add_str(("ARG"+std::to_string(i)).c_str())));
}
}

extern "C" map_session_data* lookup(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* lookup(int32 id) { return attached && attached->id==id ? attached : nullptr; }
extern "C" map_session_data* character(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* character(int32 id) { return attached && attached->status.char_id==static_cast<uint32>(id) ? attached : nullptr; }
extern "C" npc_data* npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* npc_lookup(int32) { return nullptr; }
extern "C" void mapreg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void mapreg_init() {}
extern "C" void mapreg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void mapreg_final() {}
extern "C" int32 dequeue(map_session_data*,bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 dequeue(map_session_data*,bool) { return 0; }
extern "C" void error(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error(const char* format,...) {
    ++errors; va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args);
}
extern "C" void update(map_session_data*,const achievement*,int32) asm("__wrap__Z23clif_achievement_updateP16map_session_dataPK11achievementi");
extern "C" void update(map_session_data*,const achievement*,int32) { ++updates; }
extern "C" void list(map_session_data*) asm("__wrap__Z25clif_achievement_list_allP16map_session_data");
extern "C" void list(map_session_data*) { ++lists; }
extern "C" void eof(int32) asm("__wrap__Z7set_eofi");
extern "C" void eof(int32 fd) {
    check(attached && fd==attached->fd, "deferred overflow disconnects the affected player");
    ++eof_calls;
}

extern "C" int __wrap_main(int argc, char** argv) {
    check(argc==2, "explicit artifact directory"); deny_network();
    static char server[]="shop-progression-capture-test"; SERVER_NAME=server;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script(); battle_set_defaults();
    num_reg_ers=ers_new(sizeof(script_reg_num),"capture-test numeric registry",ERS_OPT_CLEAN);
    static npc_data dummy{}; dummy.id=99000003; fake_nd=&dummy;
    battle_config.feature_achievement=1;
    const std::string directory=argv[1];
    auto text=read(directory+"/achievements.yml"); auto document=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto value:document["Body"]) check(achievement_db.parseBodyNode(value)==1, "real achievement metadata/VM parses");
    text=read(directory+"/levels.yml"); document=ryml::parse_in_arena(ryml::to_csubstr(text));
    for(auto value:document["Body"]) check(achievement_level_db.parseBodyNode(value)==1, "real achievement level metadata parses");

    auto mutation_player=std::make_unique<map_session_data>(); attached=mutation_player.get();
    mutation_player->id=mutation_player->status.account_id=99000011;
    mutation_player->status.char_id=99000012; mutation_player->fd=12;
    mutation_player->achievement_data.loaded=true;
    check(achievement_add(mutation_player.get(),990003)!=nullptr && achievement_remove(mutation_player.get(),990003),
          "all-zero transient achievement remains removable");
    check(achievement_update_count(mutation_player.get(),990003,0,5) &&
          mutation_player->achievement_data.count==1 && mutation_player->achievement_data.achievements[0].count[0]==5,
          "explicit count update can add and advance monotonic progress");
    check(!achievement_update_count(mutation_player.get(),990003,0,4) &&
          mutation_player->achievement_data.achievements[0].count[0]==5,
          "explicit count update refuses a durable decrease before mutation");
    check(!achievement_remove(mutation_player.get(),990003) && mutation_player->achievement_data.count==1,
          "persisted achievement removal is explicitly refused");
	mutation_player->achievement_data.opaque_count = static_cast<uint16>(
		pn_achievement_protocol::max_logout_rows(sizeof(achievement)) - mutation_player->achievement_data.count);
	check(achievement_add(mutation_player.get(),990004)==nullptr && mutation_player->achievement_data.count==1,
		  "opaque durable rows reserve logout-frame capacity from new known achievements");
	mutation_player->achievement_data.opaque_count=0;
    mutation_player->achievement_data.loaded=false;
    check(!achievement_update_count(mutation_player.get(),990003,0,6),
          "explicit mutation refuses an unfinished initial load");
    achievement_free(mutation_player.get()); mutation_player.reset(); attached=nullptr;

    auto player=std::make_unique<map_session_data>(); auto& sd=*player; attached=&sd;
    sd.id=sd.status.account_id=99000001; sd.status.char_id=99000002; sd.type=BL_PC;
    sd.status.zeny=7654321; sd.status.str=90; sd.vars_ok=true; sd.regs.vars=i64db_alloc(DB_OPT_BASE);
    sd.state.ignoretimeout=true; sd.npc_idle_timer=INVALID_TIMER;
    CREATE(sd.achievement_data.achievements,achievement,2);
    sd.achievement_data.count=2; sd.achievement_data.incompleteCount=1;
    sd.achievement_data.loaded=true;
    sd.achievement_data.achievements[0].achievement_id=990002;
    sd.achievement_data.achievements[0].count[0]=9;
    sd.achievement_data.achievements[1].achievement_id=240001;
    sd.achievement_data.achievements[1].completed=100;
    sd.achievement_data.achievements[1].score=10;
    auto* empty=achievement_add(&sd,220024);
    check(empty && !empty->completed && !empty->rewarded && empty->score==10,
          "real achievement_add creates the nonpersistable incomplete fixture");
    sd.achievement_data.save=true; achievement_level(&sd,false);
    pc_setglobalreg(&sd,add_str("ARG0"),765); argument(sd,0)->flag.update=0;
    pc_setglobalreg(&sd,add_str("ARG9"),-9223372036854775807LL); argument(sd,9)->flag.update=1;
    sd.vars_dirty=false;
    const auto original=sd.achievement_data;
    const auto first=original.achievements[0], second=original.achievements[1], third=original.achievements[2];
    updates=lists=0;
    std::vector<achievement> before,after;
    check(achievement_prepare_shop(sd,{},before,after) && !row(before,220024) && !row(after,220024) && before.size()==2,
          "zero-counter incomplete row is absent from both persisted snapshots");
    std::vector<achievement> persisted_baseline{third,first};
    for(auto& value:persisted_baseline) value.score=0;
    check(before.size()==persisted_baseline.size() &&
          std::memcmp(before.data(),persisted_baseline.data(),before.size()*sizeof(achievement))==0,
          "canonical map baseline exactly matches SQL's omitted-empty-row form");
    sd.achievement_data.achievements[1].count[MAX_ACHIEVEMENT_OBJECTIVES-1]=1;
    check(achievement_prepare_shop(sd,{},before,after) && row(before,220024) &&
          row(after,220024)->count[MAX_ACHIEVEMENT_OBJECTIVES-1]==1,
          "any nonzero objective including the last counter is persistable");
    sd.achievement_data.achievements[1].count[MAX_ACHIEVEMENT_OBJECTIVES-1]=0;
    check(achievement_prepare_shop(sd,{100,150000},before,after), "native prepare succeeds");
    check(updates==0 && lists==0, "prepare sends no achievement packets");
    check(sd.achievement_data.achievements==original.achievements && sd.achievement_data.count==3 &&
          sd.achievement_data.incompleteCount==2 && sd.achievement_data.save && sd.achievement_data.loaded &&
          sd.achievement_data.total_score==original.total_score && sd.achievement_data.level==original.level,
          "prepare restores original log ownership and scalar state");
    check(std::memcmp(&first,&sd.achievement_data.achievements[0],sizeof(first))==0 &&
          std::memcmp(&second,&sd.achievement_data.achievements[1],sizeof(second))==0 &&
          std::memcmp(&third,&sd.achievement_data.achievements[2],sizeof(third))==0,
          "prepare leaves every original row byte untouched");
    check(argument(sd,0)->value==765 && !argument(sd,0)->flag.update &&
          argument(sd,9)->value==-9223372036854775807LL && argument(sd,9)->flag.update && !sd.vars_dirty,
          "prepare restores all argument values and per-entry/overall dirty flags");
    for(int i=1;i<9;++i) check(argument(sd,i)==nullptr, "absent argument records stay absent");
    check(before.size()==2 && before[0].achievement_id==240001 && before[1].achievement_id==990002,
          "baseline is canonical and sorted independently of live partition");
    check(!row(before,220024) && row(after,220024) && row(after,220024)->completed>0,
          "omitted in-memory empty achievement enters after only when real progression earns it");
    for(int id=220023;id<=220029;++id) check(row(after,id) && row(after,id)->completed>0,
          "all seven real Get_Item thresholds reach prepared progression");
    for(const auto& value:after) check(value.score==0, "canonical durable rows exclude derived map scores");
    check(row(after,990001) && row(after,990001)->completed>0, "real readparam and projected Zeny condition executes");
    check(row(after,240002) && row(after,240003), "real dependent achievement-level recursion is captured");
    check(row(after,990002)->count[0]==9 && !row(after,990002)->completed,
          "unrelated incomplete counter survives capture");
    std::vector<pn_shop::AchievementEvent> success_events{
        {AG_TAMING,{0}},{AG_GET_ZENY,{1000000000}},{AG_GET_ITEM,{100}}};
    check(achievement_prepare_shop_events(sd,success_events,before,after),
          "native mixed success-event prepare succeeds");
    check(row(after,990004) && row(after,990004)->completed,
          "real Taming objective reaches the durable image");
    for(int id=220030;id<=220035;++id) check(row(after,id) && row(after,id)->completed,
          "all six real Get_Zeny thresholds reach the durable image");
    check(row(after,220023) && row(after,220023)->completed,
          "mixed plan preserves its trailing Get_Item callback order");
    check(updates==0 && lists==0,
          "mixed Taming/Get_Zeny/Get_Item capture sends no live packets");
    check(!achievement_prepare_shop_events(sd,{{AG_MAX,{1}}},before,after),
          "invalid planned objective group refuses before capture");
    check(!achievement_prepare_shop_events(sd,{{AG_GET_ZENY,std::vector<int32>(MAX_ACHIEVEMENT_OBJECTIVES+1,1)}},before,after),
          "oversized planned objective arguments refuse before capture");
    sd.status.str=89;
    check(achievement_prepare_shop(sd,{100},before,after) && !row(after,990001),
          "real readparam false condition is respected");
    sd.status.str=90;
    sd.status.zeny=1;
    check(achievement_prepare_shop(sd,{100},before,after) && !row(after,990001),
          "real projected-player parameter false condition is respected");
    sd.status.zeny=7654321;

    sd.shop_commit.pending=true;
    achievement_update_objective(&sd,AG_SPEND_ZENY,1,20);
    achievement_update_objective_values(&sd,AG_BATTLE,{1002,7});
    check(sd.shop_commit.deferred_achievements.size()==2 &&
          sd.shop_commit.deferred_achievements[0].group==AG_SPEND_ZENY &&
          sd.shop_commit.deferred_achievements[0].arguments==std::vector<int32>{20} &&
          sd.shop_commit.deferred_achievements[1].arguments==std::vector<int32>({1002,7}),
          "pending progression records exact group arguments and ordering");
    check(sd.achievement_data.achievements[0].count[0]==9 && updates==0,
          "queued events cannot mutate the prepared baseline");
    check(achievement_prepare_shop(sd,{100},before,after), "owned capture bypasses pending fence");
    check(row(after,220023) && row(after,220023)->completed>0,
          "pending owned capture evaluates the real condition VM");
    check(!row(before,220024) && !row(after,220024),
          "an unearned empty row remains omitted during a real item delivery");
    check(sd.shop_commit.deferred_achievements.size()==2 &&
          sd.shop_commit.deferred_achievements[0].group==AG_SPEND_ZENY &&
          sd.shop_commit.deferred_achievements[0].arguments==std::vector<int32>{20} &&
          sd.shop_commit.deferred_achievements[1].group==AG_BATTLE &&
          sd.shop_commit.deferred_achievements[1].arguments==std::vector<int32>({1002,7}),
          "capture leaves the exact ordinary deferred queue untouched");
    check(sd.shop_commit.pending && !achievement_shop_capture_active(&sd),
          "capture restores transaction fence and clears owned bypass");
    check(achievement_apply_shop(sd,after), "durable snapshot installs");
    check(updates==1 && lists==1 && !sd.achievement_data.save && sd.titles==std::vector<int32>{1023},
          "apply refreshes complete packets titles and durable flag");
    check(sd.achievement_data.incompleteCount==2 && sd.achievement_data.achievements[0].achievement_id==990002 &&
          sd.achievement_data.achievements[1].achievement_id==220024 && !sd.achievement_data.achievements[1].completed,
          "apply rebuilds the partition and preserves an unearned live transient row");
    auto queued=std::move(sd.shop_commit.deferred_achievements); sd.shop_commit={};
    for(const auto& event:queued) achievement_update_objective_values(&sd,static_cast<e_achievement_group>(event.group),event.arguments);
    check(sd.achievement_data.achievements[0].count[0]==29 && sd.achievement_data.save,
          "post-settlement replay preserves exact unrelated progress once");
    sd.achievement_data.reward_pending_id=240001;
    achievement_update_objective(&sd,AG_SPEND_ZENY,1,10);
    check(sd.shop_commit.deferred_achievements.size()==1 && sd.achievement_data.achievements[0].count[0]==29,
          "reward transaction fences objective mutation until its durable response");
    sd.achievement_data.reward_pending_id=0;
    queued=std::move(sd.shop_commit.deferred_achievements);
    for(const auto& event:queued) achievement_update_objective_values(&sd,static_cast<e_achievement_group>(event.group),event.arguments);
    check(sd.achievement_data.achievements[0].count[0]==39,
          "reward response replay advances the fenced objective exactly once");
    sd.shop_commit.pending=true;
    sd.shop_commit.deferred_achievements.assign(pn_shop::max_deferred_achievements,{AG_BATTLE,{1002,7}});
    achievement_update_objective_values(&sd,AG_BATTLE,{1002,8});
    check(eof_calls==1 && sd.shop_commit.deferred_achievements.empty() &&
          !sd.achievement_data.loaded && !sd.achievement_data.save,
          "deferred progression overflow clears untrusted state and disconnects fail-closed");
    sd.shop_commit={}; sd.achievement_data.loaded=true;
    const auto* installed=sd.achievement_data.achievements;
    auto invalid=after; invalid.push_back(after.front());
    check(!achievement_apply_shop(sd,invalid) && sd.achievement_data.achievements==installed,
          "duplicate snapshot refuses without destroying live rows");
    invalid=after; invalid[0].achievement_id=99999999;
    check(!achievement_apply_shop(sd,invalid) && sd.achievement_data.achievements==installed,
          "unknown definition refuses without destroying live rows");
    invalid=after; achievement unearned{}; unearned.achievement_id=220024; unearned.score=10;
    invalid.push_back(unearned);
    check(!achievement_apply_shop(sd,invalid) && sd.achievement_data.achievements==installed,
          "nonpersistable empty bundle row refuses without destroying live rows");
	sd.achievement_data.opaque_count = static_cast<uint16>(
		pn_achievement_protocol::max_logout_rows(sizeof(achievement)) - after.size() + 1);
	check(!achievement_apply_shop(sd,after) && sd.achievement_data.achievements==installed,
		  "durable shop apply cannot overbook capacity reserved by opaque SQL rows");
	sd.achievement_data.opaque_count=0;
    sd.vars_dirty=true;
    check(achievement_prepare_shop(sd,{},before,after) && sd.vars_dirty, "empty capture preserves an already dirty registry");
    check(before.size()==after.size() && std::memcmp(before.data(),after.data(),before.size()*sizeof(achievement))==0,
          "empty delivery produces byte-identical canonical snapshots");
    check(!achievement_prepare_shop(sd,{},before,before), "aliased output snapshots refuse");
    sd.vars_ok=false;
    check(!achievement_prepare_shop(sd,{100},before,after), "unloaded registry refuses capture");
    sd.vars_ok=true;
    sd.achievement_data.loaded=false;
    check(!achievement_prepare_shop(sd,{100},before,after), "initial achievement load must finish before capture");
    sd.achievement_data.loaded=true;
    if(auto* value=argument(sd,0)) script_reg_destroy_single(&sd,add_str("ARG0"),&value->flag);
    sd.vars_dirty=false;
    check(achievement_prepare_shop(sd,{100},before,after) && !argument(sd,0) && !sd.vars_dirty,
          "new temporary ARG0 record is removed completely on capture exit");
    battle_config.feature_achievement=0;
    check(achievement_prepare_shop(sd,{150000},before,after) && before.size()==after.size() &&
          std::memcmp(before.data(),after.data(),before.size()*sizeof(achievement))==0,
          "disabled achievements preserve the identical snapshot");
    battle_config.feature_achievement=1;
    check(achievement_apply_shop(sd,{}), "empty durable log installs while retaining live transient rows");
    check(sd.achievement_data.count==1 && sd.achievement_data.incompleteCount==1 &&
          sd.achievement_data.achievements[0].achievement_id==220024 && sd.titles.empty(),
          "empty durable replacement preserves only the unearned transient row");
    check(achievement_prepare_shop(sd,{},before,after) && before.empty() && after.empty(),
          "preserved transient map row still causes no persisted baseline mismatch");
    check(achievement_prepare_shop(sd,{150000},before,after) && row(after,220024) && row(after,220024)->completed>0,
          "preserved transient row can still earn real progression later");
    check(achievement_apply_shop(sd,after), "earned durable snapshot supersedes its transient row");
    int matching=0;
    for(int i=0;i<sd.achievement_data.count;++i) if(sd.achievement_data.achievements[i].achievement_id==220024) ++matching;
    check(matching==1 && sd.achievement_data.incompleteCount==0,
          "earned durable ID appears once without a duplicate transient entry");
    check(achievement_apply_shop(sd,{}), "empty durable log removes every durable-only live row");
    check(!sd.achievement_data.achievements && !sd.achievement_data.count && !sd.achievement_data.incompleteCount && sd.titles.empty(),
          "empty apply resets all ownership counters and titles");

    for(int i=0;i<MAX_ACHIEVEMENT_OBJECTIVES;++i) if(auto* value=argument(sd,i))
        script_reg_destroy_single(&sd,add_str(("ARG"+std::to_string(i)).c_str()),&value->flag);
    db_destroy(sd.regs.vars); sd.regs.vars=nullptr; attached=nullptr; player.reset();
    do_final_achievement(); ers_destroy(num_reg_ers); num_reg_ers=nullptr;
    do_final_script(); timer_final(); db_final(); malloc_final();
    check(errors==0, "no real script errors");
    std::printf("SHOP_PROGRESSION_CAPTURE_OK assertions=%u\n",assertions);
    return 0;
}
