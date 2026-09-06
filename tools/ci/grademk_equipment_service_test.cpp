// Actual source-extracted NPC dialogue and real item_enchant builtin.
// Lookup, transient @menu persistence, UI and payment/deletion boundaries only.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
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
#include "common/core.hpp"
#include "common/database.hpp"
#include "common/db.hpp"
#include "common/malloc.hpp"
#include "common/timer.hpp"
#include "map/battle.hpp"
#include "map/itemdb.hpp"
#include "map/log.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"
#include "equipment_fixture.inc"

namespace {
constexpr int32 NPC=99000003;
map_session_data* attached=nullptr;
unsigned assertions=0,failures=0,errors=0,closes=0,nexts=0,menus=0,payments=0,deletions=0;
std::vector<uint64> requests;
std::vector<int64> selections;
std::vector<std::string> messages;
void check(bool ok,const char* message) {
    ++assertions;
    if(!ok) { ++failures; std::fprintf(stderr,"EQUIPMENT FAIL: %s\n",message); }
}
void boundary(bool ok,const char* message) {
    if(!ok) { std::fprintf(stderr,"EQUIPMENT BOUNDARY: %s\n",message); std::exit(2); }
}
void deny_network() {
    sock_filter rules[]={
        BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_connect,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_bind,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_listen,0,1), BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
        BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
    sock_fprog filter{static_cast<unsigned short>(std::size(rules)),rules};
    boundary(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0 && prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&filter)==0,"kernel network denial");
    for(long call : {__NR_socket,__NR_connect,__NR_bind,__NR_listen}) check(syscall(call,0,0,0)==-1 && errno==EPERM,"network operation denied");
}
}
extern "C" map_session_data* lookup(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* lookup(int32 id) { return attached && attached->id==id ? attached : nullptr; }
extern "C" npc_data* npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* npc_lookup(int32) { return nullptr; }
extern "C" void reg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void reg_init() {}
extern "C" void reg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void reg_final() {}
extern "C" int32 dequeue(map_session_data*,bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 dequeue(map_session_data*,bool) { return 0; }
extern "C" void mes(const map_session_data&,uint32,const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void mes(const map_session_data& sd,uint32 npc,const char* text) { check(&sd==attached && npc==NPC,"message recipient"); messages.emplace_back(text); }
extern "C" void next(const map_session_data&,uint32) asm("__wrap__Z15clif_scriptnextRK16map_session_dataj");
extern "C" void next(const map_session_data& sd,uint32 npc) { check(&sd==attached && npc==NPC,"next recipient"); ++nexts; }
extern "C" void close_ui(const map_session_data&,uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void close_ui(const map_session_data& sd,uint32 npc) { check(&sd==attached && npc==NPC,"close recipient"); ++closes; }
extern "C" void menu(map_session_data&,uint32,const char*) asm("__wrap__Z15clif_scriptmenuR16map_session_datajPKc");
extern "C" void menu(map_session_data& sd,uint32 npc,const char* text) {
    check(&sd==attached && npc==NPC,"menu recipient");
    check(std::strcmp(text,"Flush Einbech:Muqaddas:Furious weapons:Furious crowns:Sky Rune crowns:Cancel")==0,"exact five-family menu and Cancel"); ++menus;
}
extern "C" bool set_reg(map_session_data*,int64,int64) asm("__wrap__Z9pc_setregP16map_session_datall");
extern "C" bool set_reg(map_session_data* sd,int64 key,int64 value) {
    check(sd==attached && std::strcmp(get_str(script_getvarid(key)),"@menu")==0,"only native select's transient @menu persistence"); selections.push_back(value); return true;
}
extern "C" void open_ui(map_session_data&,uint64) asm("__wrap__Z23clif_enchantwindow_openR16map_session_datam");
extern "C" void open_ui(map_session_data& sd,uint64 group) {
    check(&sd==attached && closes==1,"actual item_enchant requests UI only after close acknowledgement"); requests.push_back(group);
    // No packet delivery or item_enchant_index activation is simulated.
}
extern "C" char pay(map_session_data*,int32,e_log_pick_type,uint32) asm("__wrap__Z10pc_payzenyP16map_session_datai15e_log_pick_typej");
extern "C" char pay(map_session_data*,int32,e_log_pick_type,uint32) { ++payments; return 1; }
extern "C" char del(map_session_data*,int32,int32,int32,int16,e_log_pick_type) asm("__wrap__Z10pc_delitemP16map_session_dataiiis15e_log_pick_type");
extern "C" char del(map_session_data*,int32,int32,int32,int16,e_log_pick_type) { ++deletions; return 1; }
extern "C" void error_message(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error_message(const char* format,...) { ++errors; va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args); }
extern "C" int __wrap_main(int argc,char** argv) {
    boundary(argc==2,"source-extracted body path supplied"); deny_network();
    static char name[]="grademk-equipment-service-test"; SERVER_NAME=name;
    malloc_init(); db_init(); do_init_database(); timer_init(); do_init_script();
    for(uint64 id : GROUPS) { auto group=std::make_shared<s_item_enchant>(); group->id=id; item_enchant_db.put(id,group); }
    // The real builtin only checks group existence. Recipe parsing/execution is
    // not claimed; Python independently verifies effective targets and identity.
    std::ifstream input(argv[1]); boundary(input.good(),"actual extracted source exists");
    const std::string source{std::istreambuf_iterator<char>(input),std::istreambuf_iterator<char>()};
    auto* code=parse_script(source.c_str(),"grademk_equipment_enchants.txt",1,0); boundary(code && errors==0,"actual complete NPC body parses");
    battle_config.atcommand_disable_npc=0;
    for(int choice : {1,2,3,4,5,6,255}) {
        closes=nexts=menus=payments=deletions=0; requests.clear(); selections.clear(); messages.clear();
        auto player=std::make_unique<map_session_data>(); attached=player.get();
        player->id=99000001; player->type=BL_PC; player->status.account_id=player->id; player->status.char_id=99000002;
        player->fd=0; player->state.ignoretimeout=true; player->npc_idle_timer=INVALID_TIMER; player->status.zeny=7654321;
        for(size_t i=0;i<std::size(SAMPLE_ITEMS);++i) {
            auto& item=player->inventory.u.items_inventory[i]; item.nameid=SAMPLE_ITEMS[i]; item.amount=1; item.identify=1;
            item.refine=10; item.unique_id=UINT64_MAX-i; item.bound=2; item.favorite=1; item.card[3]=4702;
        }
        player->inventory.amount=std::size(SAMPLE_ITEMS); const auto before=player->inventory;
        auto unchanged=[&]() {
            check(std::memcmp(&before,&player->inventory,sizeof(before))==0,"all inventory bytes unchanged");
            check(player->status.zeny==7654321 && payments==0 && deletions==0,"no zeny mutation or payment/deletion attempt");
        };
        run_script(code,0,player->id,NPC);
        boundary(player->st,"real Next suspension"); check(player->st->state==STOP && nexts==1 && messages.size()==3,"all introductory messages precede menu");
        check(messages[1].find("custom workshop")!=std::string::npos,"service is clearly custom"); unchanged();
        run_script_main(player->st); boundary(player->st,"real menu suspension");
        check(player->st->state==RERUNLINE && menus==1 && player->state.menu_or_input && requests.empty(),"native select awaits client choice with no early window");
        player->npc_menu=choice; run_script_main(player->st);
        if(choice==255) {
            check(!player->st && closes==0 && selections.empty(),"Escape ends real select without continuing or opening UI");
        } else {
            boundary(player->st,"close acknowledgement pending");
            check(selections==std::vector<int64>{choice},"native select resolves exact choice");
            check(closes==1 && requests.empty() && player->st->state==(choice==6 ? CLOSE : STOP),"Cancel closes; each Open suspends at close2 before any UI request");
            unchanged(); player->st->state=choice==6 ? END : RUN; run_script_main(player->st);
            check(!player->st,"native dialogue detaches after acknowledgement");
        }
        const bool open=choice>=1 && choice<=5;
        check(requests.size()==(open ? 1u : 0u),"only five Open choices request one enchant window");
        if(open) check(requests[0]==GROUPS[choice-1],"exact existing backend group selected");
        check(!player->state.menu_or_input && player->state.item_enchant_index==0,"menu cleared without faking transport activation");
        unchanged(); check(errors==0,"no native script errors");
        std::printf("EQUIPMENT_SERVICE_CASE_PASS choice=%d\n",choice); attached=nullptr;
    }
    script_free_code(code); item_enchant_db.clear(); do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("EQUIPMENT_SERVICE_NATIVE_RESULT cases=7 assertions=%u failures=%u errors=%u\n",assertions,failures,errors);
    return failures || errors ? 1 : 0;
}
