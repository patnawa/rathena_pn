// ============================================================================
//  PN  /  DEVELOPMENT TOOLS
//  biosphere_crown_transaction_test.cpp
// ----------------------------------------------------------------------------
//  Project contributions: (C) 2026 PN Development Team
//  License for project contributions: GPL-3.0-or-later; see LICENSE.
//  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/biosphere_crown_transaction_test.cpp
//  Existing upstream authors, notices and other rights are retained.
// ============================================================================

// GPL-3.0-or-later. Actual NPC/helper VM and inventory mutation. Explicit
// transport/persistence/player lookup/equip-status boundaries; no world startup.
#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <memory>
#include <sstream>
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
#include "common/random.hpp"
#include "common/timer.hpp"
#include "map/achievement.hpp"
#include "map/battle.hpp"
#include "map/clif.hpp"
#include "map/itemdb.hpp"
#include "map/log.hpp"
#include "map/pc.hpp"
#include "map/script.hpp"

namespace {
constexpr int NPC = 99000003;
map_session_data* attached = nullptr;
unsigned assertions=0, errors=0, cases=0, closes=0, unequips=0, equips=0;
std::map<int64,int64> nums;
std::map<int64,std::string> strings;
std::vector<std::string> messages, menu_text;
std::vector<uint64> windows;
std::function<void()> unequip_hook;
bool fail_unequip=false, fail_equip=false;
void check(bool ok, const char* message) {
    ++assertions;
    if (!ok) { std::fprintf(stderr,"BIOSPHERE TEST FAIL: %s (case %u)\n",message,cases); std::exit(1); }
}
std::string read(const std::string& path) {
    std::ifstream in(path); check(in.good(),"open exact input");
    return {std::istreambuf_iterator<char>(in),std::istreambuf_iterator<char>()};
}
std::string body(const std::string& source, const std::string& name) {
    auto marker=source.find(name); check(marker!=std::string::npos,"source marker exists");
    auto begin=source.find('{',marker); unsigned depth=0;
    bool quoted=false,escaped=false,line=false,block=false;
    for(size_t i=begin;i<source.size();++i) {
        char c=source[i],n=i+1<source.size()?source[i+1]:0;
        if(line){if(c=='\n')line=false;continue;}
        if(block){if(c=='*'&&n=='/'){block=false;++i;}continue;}
        if(quoted){if(escaped)escaped=false;else if(c=='\\')escaped=true;else if(c=='"')quoted=false;continue;}
        if(c=='/'&&n=='/'){line=true;++i;continue;}
        if(c=='/'&&n=='*'){block=true;++i;continue;}
        if(c=='"'){quoted=true;continue;}
        if(c=='{')++depth;
        if(c=='}'&&!--depth)return source.substr(begin,i-begin+1);
    }
    check(false,"balanced exact body");return {};
}
void deny_network() {
    sock_filter rules[]={BPF_STMT(BPF_LD|BPF_W|BPF_ABS,offsetof(seccomp_data,nr)),
      BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_socket,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
      BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_connect,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
      BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_bind,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
      BPF_JUMP(BPF_JMP|BPF_JEQ|BPF_K,__NR_listen,0,1),BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ERRNO|EPERM),
      BPF_STMT(BPF_RET|BPF_K,SECCOMP_RET_ALLOW)};
    sock_fprog p{static_cast<unsigned short>(std::size(rules)),rules};
    check(prctl(PR_SET_NO_NEW_PRIVS,1,0,0,0)==0,"no-new-privileges");
    check(prctl(PR_SET_SECCOMP,SECCOMP_MODE_FILTER,&p)==0,"mandatory network denial");
    check(syscall(SYS_socket,AF_INET,SOCK_STREAM,0)==-1&&errno==EPERM,"socket denied");
}
void setnum(const char* name,int64 value){nums[add_str(name)]=value;}
void put(int index,int id,int amount,bool equipped=false) {
    auto data=item_db.find(id);check(data!=nullptr,"actual item metadata exists");
    auto& it=attached->inventory.u.items_inventory[index];it={};
    it.nameid=id;it.amount=amount;it.identify=1;
    it.unique_id=data->type==IT_ARMOR?UINT64_MAX:0;
    it.equip=equipped?EQP_HEAD_TOP:0;
    attached->inventory_data[index]=data.get();
    if(equipped)attached->equip_index[EQI_HEAD_TOP]=index;
}
void weight() {
    attached->weight=0;
    for(int i=0;i<MAX_INVENTORY;++i)if(attached->inventory_data[i])
        attached->weight+=attached->inventory_data[i]->weight*attached->inventory.u.items_inventory[i].amount;
}
std::unique_ptr<map_session_data> player(int crown=400999) {
    ++cases;nums.clear();strings.clear();messages.clear();menu_text.clear();windows.clear();
    closes=unequips=equips=0;fail_unequip=fail_equip=false;unequip_hook={};
    auto sd=std::make_unique<map_session_data>();attached=sd.get();
    sd->id=sd->status.account_id=99000001;sd->status.char_id=99000002;sd->type=BL_PC;
    const int jobs[]={JOB_DRAGON_KNIGHT,JOB_IMPERIAL_GUARD,JOB_MEISTER,JOB_BIOLO,
        JOB_SHADOW_CROSS,JOB_ABYSS_CHASER,JOB_ARCH_MAGE,JOB_ELEMENTAL_MASTER,
        JOB_CARDINAL,JOB_INQUISITOR,JOB_WINDHAWK,JOB_TROUBADOUR,JOB_SHINKIRO,
        JOB_NIGHT_WATCH,JOB_SKY_EMPEROR,JOB_SOUL_ASCETIC,JOB_HYPER_NOVICE,JOB_SPIRIT_HANDLER};
    sd->status.base_level=250;sd->status.sex=SEX_MALE;
    sd->status.class_=crown>=400529&&crown<=400546?jobs[crown-400529]:JOB_ALITEA;
    sd->class_=pc_jobid2mapid(sd->status.class_);
    sd->status.zeny=7654321;sd->status.inventory_slots=MAX_INVENTORY;
    sd->fd=0;sd->state.ignoretimeout=true;sd->npc_idle_timer=INVALID_TIMER;sd->max_weight=1000000;
    for(auto& index:sd->equip_index)index=-1;
    setnum("ep17_2_main",33);setnum("RepPoints6",2000);setnum("RepPoints9",1500);
    put(0,crown,1,true);
    auto& it=sd->inventory.u.items_inventory[0];
    it.refine=12;it.enchantgrade=3;it.bound=2;it.favorite=1;it.expire_time=2100000000;
    it.card[0]=4365;it.card[1]=312720;it.card[2]=312739;it.card[3]=4700;
    for(int i=0;i<MAX_ITEM_RDM_OPT;++i){it.option[i].id=i+1;it.option[i].value=23+i;it.option[i].param=i;}
    put(1,1001552,50);put(2,1001553,50);put(3,25865,75);put(4,1001556,1000);put(5,1001555,1000);
    weight();return sd;
}
int count(int id) {
    int n=0;for(const auto& it:attached->inventory.u.items_inventory)if(it.nameid==id)n+=it.amount;return n;
}
struct Snapshot {
    decltype(map_session_data::inventory) inventory;
    uint32 weight;int zeny;
    explicit Snapshot():inventory(attached->inventory),weight(attached->weight),zeny(attached->status.zeny){}
    void unchanged()const {
        check(std::memcmp(&inventory,&attached->inventory,sizeof(inventory))==0,"entire inventory unchanged");
        check(weight==attached->weight&&zeny==attached->status.zeny,"weight and zeny unchanged");
    }
};
void walk(script_code* code,const std::vector<int>& choices,
          const std::function<void(int,int)>& hook={}) {
    run_script(code,0,attached->id,NPC);int chosen=0,pause=0;
    while(attached->st) {
        check(++pause<30,"dialogue terminates within bounded suspensions");
        auto* st=attached->st;
        if(hook)hook(pause,chosen);
        if(st->state==RERUNLINE) {
            check(chosen<static_cast<int>(choices.size()),"supplied answer for actual menu");
            attached->npc_menu=choices[chosen++];
        }else if(st->state==CLOSE)st->state=END;
        else if(st->state==STOP&&closes)st->state=RUN;
        else check(st->state==STOP,"known actual dialogue suspension");
        run_script_main(st);
    }
    check(errors==0,"no script errors");
    check(attached->status.zeny==7654321,"no Zeny charge");
}
void seed_roll(int wanted,int bound) {
    // Load a standard mt19937 state with a controlled first output. The actual
    // rand builtin and uniform distribution are unchanged, including endpoints.
    const uint32 raw=(static_cast<uint64>(wanted)*UINT64_C(4294967296)+UINT64_C(2147483648))/bound;
    auto undo_right=[](uint32 y,unsigned shift){uint32 x=y;for(unsigned i=0;i<32/shift+1;++i)x=y^(x>>shift);return x;};
    auto undo_left=[](uint32 y,unsigned shift,uint32 mask){uint32 x=y;for(unsigned i=0;i<32/shift+1;++i)x=y^((x<<shift)&mask);return x;};
    uint32 state=undo_right(raw,18);state=undo_left(state,15,0xefc60000);state=undo_left(state,7,0x9d2c5680);state=undo_right(state,11);
    std::ostringstream text;for(unsigned i=0;i<std::mt19937::state_size;++i)text<<(i==0?state:0)<<' ';text<<0;
    std::istringstream input(text.str());input>>generator;check(!input.fail(),"standard deterministic RNG state loads");
    auto probe=generator;check(std::uniform_int_distribution<int64>(0,bound-1)(probe)==wanted,"actual random distribution reaches exact requested bucket");
}
script_code* compile(const std::string& text,const char* name) {
    auto* result=parse_script(text.c_str(),name,1,0);check(result&&errors==0,"actual source parses");return result;
}
void metadata(const item& before,int slot,int enchant) {
    item expected=before;expected.card[slot]=enchant;
    check(std::memcmp(&expected,&attached->inventory.u.items_inventory[0],sizeof(item))==0,"only selected synthetic field changes");
}
}

extern "C" map_session_data* lookup(int32 id) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* lookup(int32 id){return attached&&attached->id==id?attached:nullptr;}
extern "C" npc_data* npc_lookup(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* npc_lookup(int32){return nullptr;}
extern "C" void mapreg_init() asm("__wrap__Z11mapreg_initv");extern "C" void mapreg_init(){}
extern "C" void mapreg_final() asm("__wrap__Z12mapreg_finalv");extern "C" void mapreg_final(){}
extern "C" int32 dequeue(map_session_data*,bool) asm("__wrap__Z17npc_event_dequeueP16map_session_datab");
extern "C" int32 dequeue(map_session_data*,bool){return 0;}
extern "C" void error(const char*,...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void error(const char* f,...){++errors;va_list a;va_start(a,f);std::vfprintf(stderr,f,a);va_end(a);}
extern "C" bool setreg(map_session_data*,int64,int64) asm("__wrap__Z9pc_setregP16map_session_datall");
extern "C" bool setreg(map_session_data*,int64 key,int64 value){nums[key]=value;return true;}
extern "C" int64 readreg(const map_session_data*,int64) asm("__wrap__Z10pc_readregPK16map_session_datal");
extern "C" int64 readreg(const map_session_data*,int64 key){return nums[key];}
extern "C" int64 registry(const map_session_data*,int64) asm("__wrap__Z15pc_readregistryPK16map_session_datal");
extern "C" int64 registry(const map_session_data*,int64 key){return nums[key];}
extern "C" int64 named_registry(const map_session_data*,const char*) asm("__wrap__Z11pc_readreg2PK16map_session_dataPKc");
extern "C" int64 named_registry(const map_session_data*,const char* name){return nums[add_str(name)];}
extern "C" bool unexpected_mapreg(int64,int64) asm("__wrap__Z13mapreg_setregll");
extern "C" bool unexpected_mapreg(int64 key,int64 value){
    const std::string name=get_str(script_getvarid(key));
    check(name.rfind("$@__SW",0)==0&&name.substr(name.size()-4)=="_VAL","only parser-generated transient switch register");
    nums[key]=value;return true;
}
extern "C" int64 switch_read(int64) asm("__wrap__Z14mapreg_readregl");
extern "C" int64 switch_read(int64 key){return nums[key];}
extern "C" bool setstr(map_session_data*,int64,const char*) asm("__wrap__Z12pc_setregstrP16map_session_datalPKc");
extern "C" bool setstr(map_session_data*,int64 key,const char* value){strings[key]=value;return true;}
extern "C" char* readstr(const map_session_data*,int64) asm("__wrap__Z13pc_readregstrPK16map_session_datal");
extern "C" char* readstr(const map_session_data*,int64 key){return strings[key].data();}
extern "C" void mes(const map_session_data&,uint32,const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void mes(const map_session_data&,uint32,const char* text){messages.emplace_back(text);}
extern "C" void next(const map_session_data&,uint32) asm("__wrap__Z15clif_scriptnextRK16map_session_dataj");
extern "C" void next(const map_session_data&,uint32){}
extern "C" void crown_close(const map_session_data&,uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void crown_close(const map_session_data&,uint32){++closes;}
extern "C" void menu(map_session_data&,uint32,const char*) asm("__wrap__Z15clif_scriptmenuR16map_session_datajPKc");
extern "C" void menu(map_session_data&,uint32,const char* text){menu_text.emplace_back(text);}
extern "C" void open_window(map_session_data&,uint64) asm("__wrap__Z23clif_enchantwindow_openR16map_session_datam");
extern "C" void open_window(map_session_data&,uint64 id){windows.push_back(id);}
extern "C" void display(int,const char*) asm("__wrap__Z19clif_displaymessageiPKc");
extern "C" void display(int,const char* text){messages.emplace_back(text);}
extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*) asm("__wrap__Z11log_pick_pcPK16map_session_data15e_log_pick_typeiPK4item");
extern "C" void crown_log(const map_session_data*,e_log_pick_type,int32,const item*){}
extern "C" void add(const map_session_data*,int32,int32,unsigned char) asm("__wrap__Z12clif_additemPK16map_session_dataiih");
extern "C" void add(const map_session_data*,int32,int32,unsigned char){}
extern "C" void del(const map_session_data&,int32,int32,int16) asm("__wrap__Z12clif_delitemRK16map_session_dataiis");
extern "C" void del(const map_session_data&,int32,int32,int16){}
extern "C" void update(map_session_data&,_sp) asm("__wrap__Z17clif_updatestatusR16map_session_data3_sp");
extern "C" void update(map_session_data&,_sp){}
extern "C" void quest(map_session_data*) asm("__wrap__Z17pc_show_questinfoP16map_session_data");
extern "C" void quest(map_session_data*){}
extern "C" void achievement(map_session_data*,e_achievement_group,uint8,...) asm("__wrap__Z28achievement_update_objectiveP16map_session_data19e_achievement_grouphz");
extern "C" void achievement(map_session_data*,e_achievement_group,uint8,...){}
extern "C" bool tradable(const map_session_data*,int32) asm("__wrap__Z17pc_can_trade_itemPK16map_session_datai");
extern "C" bool tradable(const map_session_data*,int32){return true;}
extern "C" bool unequip(map_session_data*,int32,int32) asm("__wrap__Z14pc_unequipitemP16map_session_dataii");
extern "C" bool unequip(map_session_data* sd,int32 i,int32 flags){
    ++unequips;check(flags==3,"native mutation forces expected unequip flags");
    if(fail_unequip)return false;
    sd->inventory.u.items_inventory[i].equip=0;sd->equip_index[EQI_HEAD_TOP]=-1;
    if(unequip_hook)unequip_hook();return true;
}
extern "C" bool equip(map_session_data*,int16,int32,bool) asm("__wrap__Z12pc_equipitemP16map_session_datasib");
extern "C" bool equip(map_session_data* sd,int16 i,int32 pos,bool){
    ++equips;if(fail_equip)return false;
    sd->inventory.u.items_inventory[i].equip=pos;sd->equip_index[EQI_HEAD_TOP]=i;return true;
}

extern "C" int __wrap_main(int argc,char** argv) {
    check(argc==2,"explicit artifact directory");deny_network();
    static char server[]="biosphere-crown-transaction-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    battle_config.atcommand_disable_npc=0;
    const std::string directory=argv[1];
    auto data=read(directory+"/items.yml");auto items=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto n:items["Body"])check(item_db.parseBodyNode(n)==1,"actual effective item parses");
    data=read(directory+"/reputation.yml");auto reps=ryml::parse_in_arena(ryml::to_csubstr(data));
    for(auto n:reps["Body"])check(reputation_db.parseBodyNode(n)==1,"actual reputation parses");
    for(int id:{132,133}){auto group=std::make_shared<s_item_enchant>();group->id=id;item_enchant_db.put(id,group);}
    const auto after=read(directory+"/after.txt"),before=read(directory+"/before.txt");
    auto* access=compile(body(after,"F_BioDepthQuestAccess"),"actual-access");
    strdb_put(script_get_userfunc_db(),"F_BioDepthQuestAccess",access);
    auto* guard=compile(body(after,"F_BioD2ModifyCrown"),"actual-crown-guard");
    strdb_put(script_get_userfunc_db(),"F_BioD2ModifyCrown",guard);
    auto* npc=compile(body(after,"Abyss Researcher#bio_d2"),"actual-Abyss-Researcher");
    auto* oldguard=compile(body(before,"F_BioD2ModifyCrown"),"pinned-old-crown-helper");
    auto* oldnpc=compile(body(before,"Abyss Researcher#bio_d2"),"pinned-old-Abyss-Researcher");
    for(int option=1;option<=20;++option){
        auto sd=player();Snapshot before;const int oldcount=count(option<=18?400528+option:400999);
        walk(npc,{2,option,1});
        if(option==19)before.unchanged();else{
            const int id=option==20?400999:400528+option;
            check(count(id)==oldcount+1,"all 19 exact craft output identities");
            check(count(1001552)==0&&count(1001553)==0&&count(25865)==0,"exact craft material deductions");
            check(nums[add_str("RepPoints9")]==1500,"reputation not spent");
            auto& result=sd->inventory.u.items_inventory[sd->last_addeditem_index];
            check(result.nameid==id&&result.identify==1&&result.unique_id!=0&&result.refine==0&&result.enchantgrade==0&&result.bound==0,"plain crafted gear and generated UID");
        }
    }
    for(const auto& choices:std::vector<std::vector<int>>{{7},{2,19},{2,20,2},{5,2},{6,2},{255},{2,255},{2,20,255},{5,255},{6,255}}){auto sd=player();Snapshot s;walk(npc,choices);s.unchanged();}
    for(int choice:{2,3,4,5,6})for(int pass=0;pass<2;++pass){
        auto sd=player();const int threshold=choice==2?500:choice==3?750:1500;
        setnum("RepPoints9",threshold-(pass?0:1));Snapshot s;seed_roll(0,choice==6?100000:100);
        walk(npc,choice==2?std::vector<int>{2,20,1}:std::vector<int>{choice,1});
        if(!pass){s.unchanged();check(windows.empty(),"exact threshold minus one refuses");}
        else if(choice==3||choice==4)check(windows.size()==1,"exact interface threshold admits");
        else check(count(choice==2?1001552:choice==5?1001556:1001555)==(choice==2?0:choice==5?990:820),"exact paid threshold admits");
        check(nums[add_str("RepPoints9")]==threshold-(pass?0:1),"exact threshold reputation not consumed");
    }
    for(int kind=0;kind<3;++kind){auto sd=player();if(kind==0)sd->status.base_level=249;else setnum(kind==1?"ep17_2_main":"RepPoints6",kind==1?32:1999);Snapshot s;walk(npc,{2,20,1});s.unchanged();}
    {auto sd=player();sd->status.class_=JOB_NOVICE;sd->class_=MAPID_NOVICE;walk(npc,{2,20,1});check(count(400999)==2,"craft remains an output choice, not an invented job gate");}
    for(int material:{1,2,3}){auto sd=player();--sd->inventory.u.items_inventory[material].amount;weight();Snapshot s;walk(npc,{2,20,1});s.unchanged();}
    {auto sd=player();sd->status.inventory_slots=6;Snapshot s;walk(npc,{2,20,1});s.unchanged();}
    for(int choice:{2,3,4,5,6})for(int kind=0;kind<4;++kind){
        auto sd=player();Snapshot s;bool injected=false;
        const auto hook=[&](int,int chosen){if(!injected&&chosen==(choice==2?2:1)){
            if(kind==0)sd->status.base_level=249;else setnum(kind==1?"ep17_2_main":kind==2?"RepPoints6":"RepPoints9",kind==1?32:kind==2?1999:0);injected=true;}};
        walk(npc,choice==2?std::vector<int>{2,20,1}:std::vector<int>{choice,1},hook);
        check(injected,"access change injected during actual suspension");s.unchanged();check(windows.empty(),"stale access never opens interface");
    }
    for(int choice:{3,4}){auto sd=player();Snapshot s;walk(npc,{choice});s.unchanged();check(windows==std::vector<uint64>{choice==3?133u:132u},"exact existing interface");}
    // Target changes at confirmation, including same-ID replacement and every
    // exposed inventory metadata field, must refuse before any payment.
    for(int choice:{5,6})for(int change=0;change<31;++change){
        auto sd=player();std::unique_ptr<Snapshot> changed;bool injected=false;
        walk(npc,{choice,1},[&](int,int chosen){if(injected||chosen!=1)return;injected=true;
            auto& it=sd->inventory.u.items_inventory[0];
            if(change==0){it.unique_id=123;}else if(change==1){it.nameid=400529;sd->inventory_data[0]=item_db.find(400529).get();}
            else if(change==2){it.equip=0;sd->equip_index[EQI_HEAD_TOP]=-1;}
            else if(change==3){sd->inventory.u.items_inventory[6]=it;sd->inventory_data[6]=sd->inventory_data[0];it={};sd->inventory_data[0]=nullptr;sd->equip_index[EQI_HEAD_TOP]=6;}
            else if(change<8)++it.card[change-4];
            else if(change==8)++it.refine;else if(change==9)++it.enchantgrade;else if(change==10)++it.bound;
            else if(change==11)++it.expire_time;else if(change==12)it.favorite=!it.favorite;
            else if(change==13)it.identify=0;else if(change==14)++it.attribute;else if(change==15)++it.amount;
            else {int n=(change-16)/3,k=(change-16)%3;if(k==0)++it.option[n].id;else if(k==1)++it.option[n].value;else ++it.option[n].param;}
            weight();changed=std::make_unique<Snapshot>();});
        check(injected,"confirmation race injected");changed->unchanged();check(unequips==0,"stale target refused before mutation");
    }
    for(int id:{400547}){auto sd=player(id);Snapshot s;walk(npc,{5});s.unchanged();}
    {auto sd=player();sd->inventory.u.items_inventory[0].unique_id=0;Snapshot s;walk(npc,{5});s.unchanged();}
    for(int choice:{5,6})for(int failure=0;failure<6;++failure){
        auto sd=player();const int material=choice==5?4:5;
        // A refused operation must retain the actual bound material stack,
        // not refund an unbound reconstruction of the same item/count.
        sd->inventory.u.items_inventory[material].bound=2;
        sd->inventory.u.items_inventory[material].amount=choice==5?10:180;weight();
        auto original=sd->inventory.u.items_inventory[0];Snapshot s;
        std::unique_ptr<Snapshot> callback_state;
        if(failure==0)fail_unequip=true;
        if(failure==1)sd->max_weight=sd->weight-1;
        if(failure==2)unequip_hook=[&](){sd->max_weight=0;};
        if(failure==3||failure==4)unequip_hook=[&](){
            auto& current=sd->inventory.u.items_inventory[0];current.unique_id=99;current.card[1]=312726;
            if(failure==4)for(int i=6;i<MAX_INVENTORY;++i)put(i,400547,1);
            weight();callback_state=std::make_unique<Snapshot>();
        };
        if(failure==5)fail_equip=true;
        seed_roll(choice==5?0:11500,choice==5?100:100000);walk(npc,{choice,1});
        if(failure==5){original.equip=0;metadata(original,choice==5?1:3,choice==5?312721:4701);
            check(count(choice==5?1001556:1001555)==0,"successful mutation charges once even if re-equip fails");
        }else if(callback_state)callback_state->unchanged();else s.unchanged();
    }
    // Material reductions during the actual confirmation cause no partial
    // payment, including the old craft's sequential three-material deletion.
    for(int choice:{2,5,6}){
        auto sd=player();std::unique_ptr<Snapshot> changed;bool injected=false;
        walk(npc,choice==2?std::vector<int>{2,20,1}:std::vector<int>{choice,1},[&](int,int selected){
            if(injected||selected!=(choice==2?2:1))return;injected=true;
            sd->inventory.u.items_inventory[choice==2?2:choice==5?4:5].amount=1;weight();changed=std::make_unique<Snapshot>();
        });check(injected,"material race injected");changed->unchanged();
    }
    {auto sd=player();std::unique_ptr<Snapshot> changed;bool injected=false;
        walk(npc,{2,20,1},[&](int,int selected){if(injected||selected!=2)return;injected=true;
            for(int i=6;i<MAX_INVENTORY;++i)put(i,400547,1);weight();changed=std::make_unique<Snapshot>();
        });check(injected,"craft capacity race injected at confirmation");changed->unchanged();}
    const int families[]={312719,312729,312739,312749,312759,312769,312779,312789,314249,314259};
    const int chance[]={0,80,65,50,35,25,20,10,7,5},cost[]={0,5,10,20,35,55,80,110,145,185};
    // Every allowed crown, every jewel family, both exact RNG threshold sides,
    // every starting level, plus level-10 refusal without resource consumption.
    for(int crown=0;crown<19;++crown)for(int base:families)for(int level=1;level<=10;++level)for(int success=0;success<2;++success){
        auto sd=player(crown<18?400529+crown:400999);
        sd->inventory.u.items_inventory[0].card[1]=base+level-1;
        auto original=sd->inventory.u.items_inventory[0];Snapshot s;
        if(level==10){walk(npc,{5});s.unchanged();continue;}
        seed_roll(success?chance[level]-1:chance[level],100);walk(npc,{5,1});
        metadata(original,1,base+level-1+(success?1:level==1?0:-1));
        check(count(1001556)==1000-cost[level]&&count(1001555)==1000,"exact upgrade-only material count");
        check(nums[add_str("RepPoints9")]==1500&&nums[add_str("RepPoints6")]==2000,"upgrade reputation unchanged");
    }
    const int statids[]={4700,4701,4702,4703,4704,4710,4711,4712,4713,4714,4740,4741,4742,4743,4744,4750,4751,4752,4753,4754};
    const int weights[]={11500,8000,3500,1500,500};int cumulative=0;
    for(int i=0;i<20;++i){for(int endpoint:{cumulative,cumulative+weights[i%5]-1})for(int grade=0;grade<=4;++grade){
        auto sd=player();sd->inventory.u.items_inventory[0].enchantgrade=grade;
        auto original=sd->inventory.u.items_inventory[0];seed_roll(endpoint,100000);walk(npc,{6,1});
        metadata(original,3,statids[i]);check(count(1001555)==820&&count(1001556)==1000,"exact stat-only material charge");
    }cumulative+=weights[i%5];}
    check(cumulative==100000,"all native stat endpoints cover exact 100000 buckets");
    // Every bucket executes the unchanged production cumulative-selection loop
    // in the real VM. Only the rand assignment is replaced by an exhaustive
    // loop input; full dialogue/RNG endpoint tests above remain independent.
    auto begin=after.find("\tsetarray .@stat_weight[0]");auto end=after.find("\n\t}\n",begin)+4;
    auto selection=after.substr(begin,end-begin);const std::string random_line="\t.@roll = rand(100000);\n";
    auto random_at=selection.find(random_line);check(random_at!=std::string::npos,"exact original RNG assignment in selection fixture");selection.erase(random_at,random_line.size());
    const std::string stat_declaration=after.substr(after.find("\tsetarray .@stat_id[0]"),after.find(';',after.find("\tsetarray .@stat_id[0]"))-after.find("\tsetarray .@stat_id[0]")+1);
    auto* selection_code=compile("{ freeloop(1); "+stat_declaration+" for(.@roll=0;.@roll<100000;++.@roll){ "+selection+" @bucket[.@roll]=.@new; } end; }","actual-stat-selection-exhaustive");
    {auto sd=player();run_script(selection_code,0,sd->id,0);check(!sd->st&&errors==0,"exhaustive actual-VM selector completes");int total=0;
        for(int i=0;i<20;++i)for(int n=0;n<weights[i%5];++n)check(nums[reference_uid(add_str("@bucket"),total++)]==statids[i],"every exact stat bucket");}
    script_free_code(selection_code);
    // Retain a genuine pre-fix actual-VM failure as a reproducible regression.
    strdb_put(script_get_userfunc_db(),"F_BioD2ModifyCrown",oldguard);
    {auto sd=player(400529);bool swapped=false;seed_roll(0,100);
        walk(oldnpc,{5,1},[&](int,int chosen){if(!swapped&&chosen==1){swapped=true;auto& it=sd->inventory.u.items_inventory[0];it.unique_id=99;it.card[1]=312726;}});
        check(swapped&&sd->inventory.u.items_inventory[0].card[1]==312721&&count(1001556)==990,"pinned old source mutates replacement level8 into level3");
        std::puts("BIOSPHERE_OLD_FAILURE_REPRODUCED: replacement crown modified and charged");}
    strdb_put(script_get_userfunc_db(),"F_BioD2ModifyCrown",guard);script_free_code(oldguard);
    script_free_code(oldnpc);script_free_code(npc);attached=nullptr;
    nums.clear();strings.clear();item_db.clear();reputation_db.clear();item_enchant_db.clear();
    do_final_script();timer_final();db_final();malloc_final();
    std::printf("BIOSPHERE_NATIVE_OK cases=%u assertions=%u\n",cases,assertions);return errors?1:0;
}
