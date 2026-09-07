// Actual current NPC parser/registration/click/unload implementation, not a model.
// World/transport are explicit doubles. First-message termination bounds clicks.
// Inclusion allows initializing only its private registries, without do_init_npc
// loading unrelated content, market SQL, timers, or a live world. No production
// source is modified. The runner excludes the existing npc.o from the link.
#include "../../src/map/npc.cpp"

#include <cerrno>
#include <sstream>
#include <set>
#include <fstream>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace episode_qi {
unsigned assertions = 0, failures = 0, errors = 0, cases = 0;
unsigned gate_vm_calls = 0, messages = 0, closes = 0, option_packets = 0, clears = 0, spawns = 0;
std::vector<std::unique_ptr<map_session_data>> players;
std::unordered_map<int32, block_list*> world;
block_list registered_block_marker;
std::vector<script_code*> gate_codes;
std::vector<std::string> map_names;
unsigned condition_calls=0;
std::string expected_message;


void require(bool value, const char* message) {
    if (!value) { std::fprintf(stderr, "EPISODE_QI_BOUNDARY_FAILURE: %s\n", message); std::exit(3); }
}
void check(bool value, const char* message) {
    ++assertions;
    if (!value) { ++failures; std::fprintf(stdout, "EPISODE_QI_EXPECTATION_FAILURE: %s\n", message); }
}
void deny_network() {
    sock_filter rules[] = {
        BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_socket, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_connect, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_bind, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_listen, 0, 1), BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | EPERM),
        BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
    };
    sock_fprog program{static_cast<unsigned short>(std::size(rules)), rules};
    require(prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) == 0, "no-new-privileges");
    require(prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, &program) == 0, "mandatory network denial");
    check(syscall(SYS_socket, AF_INET, SOCK_STREAM, 0) == -1 && errno == EPERM, "socket denied");
    check(syscall(SYS_connect, 0, nullptr, 0) == -1 && errno == EPERM, "connect denied");
    check(syscall(SYS_bind, 0, nullptr, 0) == -1 && errno == EPERM, "bind denied");
    check(syscall(SYS_listen, 0, 1) == -1 && errno == EPERM, "listen denied");
}
struct Iterator { std::vector<block_list*> values; size_t position = 0; };
}

extern "C" void qi_reg_init() asm("__wrap__Z11mapreg_initv");
extern "C" void qi_reg_init() {}
extern "C" void qi_reg_final() asm("__wrap__Z12mapreg_finalv");
extern "C" void qi_reg_final() {}
extern "C" void qi_error(const char*, ...) asm("__wrap__Z9ShowErrorPKcz");
extern "C" void qi_error(const char* fmt, ...) { ++episode_qi::errors; va_list ap; va_start(ap, fmt); std::vfprintf(stderr, fmt, ap); va_end(ap); }
extern "C" block_list* qi_bl(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* qi_bl(int32 id) { auto p = episode_qi::world.find(id); return p == episode_qi::world.end() ? nullptr : p->second; }
extern "C" bool qi_exists(int32) asm("__wrap__Z15map_blid_existsi");
extern "C" bool qi_exists(int32 id) { return qi_bl(id) != nullptr; }
extern "C" map_session_data* qi_sd(int32) asm("__wrap__Z9map_id2sdi");
extern "C" map_session_data* qi_sd(int32 id) { auto* p = qi_bl(id); return BL_CAST(BL_PC, p); }
extern "C" npc_data* qi_nd(int32) asm("__wrap__Z9map_id2ndi");
extern "C" npc_data* qi_nd(int32 id) { auto* p = qi_bl(id); return BL_CAST(BL_NPC, p); }
extern "C" map_session_data* qi_cid(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* qi_cid(int32 id) { for (auto& p : episode_qi::players) if (p->status.char_id == id) return p.get(); return nullptr; }
extern "C" int16 qi_map(const char*) asm("__wrap__Z17map_mapname2mapidPKc");
extern "C" int16 qi_map(const char* name) { auto p=std::find(episode_qi::map_names.begin(),episode_qi::map_names.end(),name); return p==episode_qi::map_names.end()?-1:p-episode_qi::map_names.begin(); }
extern "C" uint16 qi_index(const char*, const char*) asm("__wrap__Z17mapindex_name2idxPKcS0_");
extern "C" uint16 qi_index(const char* name, const char*) { return qi_map(name) >= 0 ? 1000 + qi_map(name) : 0; }
extern "C" void qi_addid(block_list*) asm("__wrap__Z11map_addiddbP10block_list");
extern "C" void qi_addid(block_list* b) { episode_qi::require(episode_qi::world.emplace(b->id, b).second, "unique synthetic world ID"); }
extern "C" void qi_delid(block_list*) asm("__wrap__Z11map_deliddbP10block_list");
extern "C" void qi_delid(block_list* b) { episode_qi::require(episode_qi::world.erase(b->id) == 1, "remove known synthetic world ID"); }
extern "C" bool qi_addnpc(int16, npc_data*) asm("__wrap__Z10map_addnpcsP8npc_data");
extern "C" bool qi_addnpc(int16 m, npc_data* nd) {
    episode_qi::require(m >= 0 && m < map_num && nd->u.scr.xs == -1 && nd->u.scr.ys == -1, "only selected non-touch static owners enter world double");
    map[m].npc[map[m].npc_num++] = nd; qi_addid(nd); return true;
}
extern "C" int32 qi_addblock(block_list*) asm("__wrap__Z12map_addblockP10block_list");
extern "C" int32 qi_addblock(block_list* b) {
    episode_qi::require(b->type == BL_NPC && b->m >= 0 && b->m < map_num && !b->prev, "known NPC block registration");
    // Native npc_remove_map uses prev != nullptr as registration evidence.
    // Spatial buckets are out of scope; retain their registration contract.
    b->prev = &episode_qi::registered_block_marker; b->next = nullptr; return 0;
}
extern "C" int32 qi_delblock(block_list*) asm("__wrap__Z12map_delblockP10block_list");
extern "C" int32 qi_delblock(block_list* b) {
    episode_qi::require(b->type == BL_NPC && b->m >= 0 && b->m < map_num && b->prev == &episode_qi::registered_block_marker, "known NPC block removal");
    b->prev = b->next = nullptr; return 0;
}
extern "C" s_mapiterator* qi_iter(e_mapitflags, bl_type) asm("__wrap__Z11mapit_alloc12e_mapitflags7bl_type");
extern "C" s_mapiterator* qi_iter(e_mapitflags flags, bl_type types) {
    episode_qi::require(flags == MAPIT_NORMAL, "normal isolated world iterator");
    auto* it = new episode_qi::Iterator;
    for (auto& entry : episode_qi::world) if (entry.second->type & types) it->values.push_back(entry.second);
    return reinterpret_cast<s_mapiterator*>(it);
}
extern "C" void qi_iter_free(s_mapiterator*) asm("__wrap__Z10mapit_freeP13s_mapiterator");
extern "C" void qi_iter_free(s_mapiterator* p) { delete reinterpret_cast<episode_qi::Iterator*>(p); }
extern "C" bool qi_iter_exists(s_mapiterator*) asm("__wrap__Z12mapit_existsP13s_mapiterator");
extern "C" bool qi_iter_exists(s_mapiterator* p) { auto& it = *reinterpret_cast<episode_qi::Iterator*>(p); return it.position < it.values.size(); }
extern "C" block_list* qi_iter_first(s_mapiterator*) asm("__wrap__Z11mapit_firstP13s_mapiterator");
extern "C" block_list* qi_iter_first(s_mapiterator* p) { auto& it = *reinterpret_cast<episode_qi::Iterator*>(p); it.position = 0; return qi_iter_exists(p) ? it.values[0] : nullptr; }
extern "C" block_list* qi_iter_next(s_mapiterator*) asm("__wrap__Z10mapit_nextP13s_mapiterator");
extern "C" block_list* qi_iter_next(s_mapiterator* p) { auto& it = *reinterpret_cast<episode_qi::Iterator*>(p); ++it.position; return qi_iter_exists(p) ? it.values[it.position] : nullptr; }
extern "C" int32 qi_foreach(int32 (*)(block_list*, va_list), int16, int32, ...) asm("__wrap__Z16map_foreachinmapPFiP10block_listP13__va_list_tagEsiz");
extern "C" int32 qi_foreach(int32 (*func)(block_list*, va_list), int16 m, int32 types, ...) {
    episode_qi::require(m >= 0 && m < map_num && types == BL_PC, "cloak cleanup iterates only the attached fixture players");
    int32 result = 0; va_list ap; va_start(ap, types);
    for (auto& p : episode_qi::players) if(p->m==m) { va_list copy; va_copy(copy, ap); result += func(p.get(), copy); va_end(copy); }
    va_end(ap); return result;
}
extern "C" void qi_foreachnpc(int32 (*)(npc_data*, va_list), ...) asm("__wrap__Z14map_foreachnpcPFiP8npc_dataP13__va_list_tagEz");
extern "C" void qi_foreachnpc(int32 (*func)(npc_data*, va_list), ...) {
    std::vector<npc_data*> copy;
    for (auto& e : episode_qi::world) if (e.second->type == BL_NPC) copy.push_back(static_cast<npc_data*>(e.second));
    va_list ap; va_start(ap, func);
    for (auto* nd : copy) { va_list next; va_copy(next, ap); func(nd, next); va_end(next); }
    va_end(ap);
}
extern "C" int32 qi_spawn(const block_list*, bool) asm("__wrap__Z10clif_spawnPK10block_listb");
extern "C" int32 qi_spawn(const block_list* b, bool) { episode_qi::require(b && b->type == BL_NPC, "NPC-only spawn request"); ++episode_qi::spawns; return 0; }
extern "C" void qi_option(const block_list*, const block_list*) asm("__wrap__Z24clif_changeoption_targetPK10block_listS1_");
extern "C" void qi_option(const block_list* b, const block_list* target) {
    episode_qi::require(b && b->type == BL_NPC && (!target || target->type == BL_PC), "NPC option UI recorder");
    ++episode_qi::option_packets;
}
extern "C" void qi_clear(const block_list&, clr_type) asm("__wrap__Z19clif_clearunit_areaRK10block_list8clr_type");
extern "C" void qi_clear(const block_list& b, clr_type) { episode_qi::require(b.type == BL_NPC, "NPC clear UI recorder"); ++episode_qi::clears; }
extern "C" void qi_mes(const map_session_data&, uint32, const char*) asm("__wrap__Z14clif_scriptmesRK16map_session_datajPKc");
extern "C" void qi_mes(const map_session_data& sd, uint32 id, const char* text) {
    episode_qi::require(qi_sd(sd.id) == &sd && qi_nd(id), "first message targets real parsed owner/player");
    episode_qi::require(text == episode_qi::expected_message && sd.st, "exact first message is the explicit bounded-click stop");
    sd.st->state=END; // Test-only first-message boundary; never execute dialogue rewards/travel.
    ++episode_qi::messages;
}
extern "C" void qi_close(const map_session_data&, uint32) asm("__wrap__Z16clif_scriptcloseRK16map_session_dataj");
extern "C" void qi_close(const map_session_data& sd, uint32 id) { episode_qi::require(qi_sd(sd.id) == &sd && qi_nd(id), "known close destination"); ++episode_qi::closes; }
extern "C" void qi_real_run(script_code*, int32, int32, int32) asm("__real__Z10run_scriptP11script_codeiii");
extern "C" void qi_run(script_code*, int32, int32, int32) asm("__wrap__Z10run_scriptP11script_codeiii");
extern "C" void qi_run(script_code* code, int32 pos, int32 rid, int32 oid) {
    if (std::find(episode_qi::gate_codes.begin(), episode_qi::gate_codes.end(), code) != episode_qi::gate_codes.end()) ++episode_qi::gate_vm_calls;
    qi_real_run(code, pos, rid, oid);
}


extern struct eri* num_reg_ers;
extern struct eri* str_reg_ers;
extern "C" bool qi_real_condition(script_code*,map_session_data*) asm("__real__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool qi_condition(script_code*,map_session_data*) asm("__wrap__Z27achievement_check_conditionP11script_codeP16map_session_data");
extern "C" bool qi_condition(script_code* code,map_session_data* sd){
    ++episode_qi::condition_calls;
    auto* st=sd->st;int rid=st?st->rid:0;
    bool result=qi_real_condition(code,sd);
    episode_qi::check(sd->st==st&&(!st||st->rid==rid),"native condition preserves attached caller");
    return result;
}
extern "C" void qi_icon(const map_session_data*,const block_list*,e_questinfo_types,e_questinfo_markcolor) asm("__wrap__Z21clif_quest_show_eventPK16map_session_dataPK10block_list17e_questinfo_types21e_questinfo_markcolor");
extern "C" void qi_icon(const map_session_data* sd,const block_list* nd,e_questinfo_types,e_questinfo_markcolor){
    episode_qi::check(qi_sd(sd->id)==sd&&qi_nd(nd->id),"native marker notification destination; no packet transport");
}
namespace episode_qi {
struct Owner{std::string mapname,name,icon,color,message; int x,y; int32 prior=0;};
std::vector<Owner> owners;
using Reg=std::map<int64,std::pair<int64,bool>>;
int32 reg_collect(DBKey key,DBData* data,va_list ap){
    auto* result=va_arg(ap,Reg*);auto* p=static_cast<script_reg_num*>(db_data2ptr(data));
    require(p&&!p->flag.type,"only numeric loaded fixture variables");(*result)[key.i64]={p->value,p->flag.update!=0};return 0;
}
Reg regs(map_session_data& sd){Reg result;sd.regs.vars->foreach(sd.regs.vars,reg_collect,&result);return result;}
void registration(unsigned expected){
    for(auto& o:owners){auto* nd=npc_name2id(o.name.c_str());require(nd,"owner survives lifecycle");
        check(nd->qi_data.size()==expected,"exact per-owner native condition count");
        int64 icon=0,color=0;require(script_get_constant(o.icon.c_str(),&icon)&&script_get_constant(o.color.c_str(),&color),"actual icon constants");
        for(auto& q:nd->qi_data)check(q->condition&&q->icon==icon&&q->color==color,"compiled original condition and unchanged icon/color");
    }
    for(int m=0;m<map_num;++m){
        unsigned total=std::count_if(owners.begin(),owners.end(),[&](const Owner& o){return qi_map(o.mapname.c_str())==m;});
        check(map[m].npc_num==total,"exact per-map owner count");
        check(map[m].qi_npc.size()==(expected?total:0),"native map owner IDs deduplicate");
        check(std::set<int32>(map[m].qi_npc.begin(),map[m].qi_npc.end()).size()==map[m].qi_npc.size(),"no duplicate QI map owner IDs");
    }
}
void click(Owner& o){
    ++cases;auto* nd=npc_name2id(o.name.c_str());auto& sd=*players[0];sd.m=nd->m;sd.x=nd->x;sd.y=nd->y-1;
    expected_message=o.message;auto before=regs(sd);bool dirty=sd.vars_dirty;auto inventory=sd.inventory;auto zeny=sd.status.zeny;
    unsigned old_messages=messages;
    check(npc_click(&sd,nd)==0,"real native click admitted at source location");
    check(messages==old_messages+1&&!sd.st&&!sd.npc_id,"exact first-message boundary releases native state");
    check(before==regs(sd)&&dirty==sd.vars_dirty,"bounded prefix including real Horuru SyncStep leaves loaded registry unchanged");
    check(!sd.quest_log&&!sd.num_quests&&!sd.avail_quests&&sd.status.zeny==zeny&&std::memcmp(&inventory,&sd.inventory,sizeof(inventory))==0,"bounded prefix has no inventory/Zeny/quest mutations");
}
}
extern "C" int __wrap_main(int argc,char** argv){
    using namespace episode_qi;
    require(argc==6,"helper source, owner source, owner TSV, fixed/old mode, mob-view identities");
    const bool fixed=std::string(argv[4])=="fixed";require(fixed||std::string(argv[4])=="old","explicit phase");
    deny_network();static char server[]="episode-qi-native-test";SERVER_NAME=server;
    malloc_init();db_init();do_init_database();timer_init();do_init_script();battle_set_defaults();
    num_reg_ers=ers_new(sizeof(script_reg_num),"episode-qi:num",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    str_reg_ers=ers_new(sizeof(script_reg_str),"episode-qi:str",(ERSOptions)(ERS_OPT_CLEAN|ERS_OPT_FLEX_CHUNK));
    ev_db=strdb_alloc((DBOptions)(DB_OPT_DUP_KEY|DB_OPT_RELEASE_DATA),EVENT_NAME_LENGTH);
    npcname_db=strdb_alloc(DB_OPT_BASE,NPC_NAME_LENGTH+1);
    npc_path_db=strdb_alloc((DBOptions)(DB_OPT_BASE|DB_OPT_DUP_KEY|DB_OPT_RELEASE_DATA),80);
    for(int i=0;i<MAX_NPC_CLASS;++i)npc_viewdb[i].look[LOOK_BASE]=i;
    for(int i=MAX_NPC_CLASS2_START;i<MAX_NPC_CLASS2_END;++i)npc_viewdb2[i-MAX_NPC_CLASS2_START].look[LOOK_BASE]=i;
    // Same native fake-NPC initialization used by do_init_npc. The actual
    // achievement condition VM attaches this owner, so it cannot be omitted.
    fake_nd=npc_create_npc(-1,0,0);fake_nd->class_=JT_FAKENPC;fake_nd->speed=DEFAULT_NPC_WALK_SPEED;
    std::strcpy(fake_nd->name,"FAKE_NPC");std::memcpy(fake_nd->exname,fake_nd->name,9);
    ++npc_script;fake_nd->type=BL_NPC;fake_nd->subtype=NPCTYPE_SCRIPT;
    strdb_put(npcname_db,fake_nd->exname,fake_nd);fake_nd->u.scr.timerid=INVALID_TIMER;qi_addid(fake_nd);
    // Explicit view-identity world fixture, projected from actual selected DB
    // rows. Native parser defaults initialize the view; no combat stats claim.
    std::ifstream views(argv[5]);require(views.good(),"existing mob-backed NPC view fixture");
    std::string view_text{std::istreambuf_iterator<char>(views),std::istreambuf_iterator<char>()};
    auto view_tree=ryml::parse_in_arena(ryml::to_csubstr(view_text));
    for(auto row:view_tree.rootref().children())require(mob_db.parseBodyNode(row)==1&&!errors,"native existing view identity parsing");
    std::ifstream input(argv[3]);require(input.good(),"owner TSV");
    for(std::string line;std::getline(input,line);){
        std::vector<std::string> f;std::stringstream stream(line);for(std::string s;std::getline(stream,s,'\t');)f.push_back(s);
        require(f.size()==7,"strict owner fixture fields");Owner o{f[0],f[1],f[4],f[5],f[6],std::stoi(f[2]),std::stoi(f[3])};owners.push_back(o);
        if(qi_map(o.mapname.c_str())<0)map_names.push_back(o.mapname);
    }
    require(owners.size()==125&&map_names.size()==27,"entire bounded source inventory");
    map_num=map_names.size();
    for(int m=0;m<map_num;++m){std::strcpy(map[m].name,map_names[m].c_str());map[m].m=m;map[m].index=1000+m;map[m].xs=map[m].ys=1024;}
    battle_config.atcommand_disable_npc=0;battle_config.etc_log=0;battle_config.dynamic_mobs=0;
    auto p=std::make_unique<map_session_data>();p->id=p->status.account_id=99000001;p->status.char_id=99000002;p->type=BL_PC;p->status.base_level=250;
    p->status.zeny=7654321;p->fd=0;p->state.ignoretimeout=true;p->npc_idle_timer=p->npc_timer_id=INVALID_TIMER;p->vars_ok=true;
    p->regs.vars=i64db_alloc(DB_OPT_BASE);pc_set_reg_load(true);
    require(pc_setreg2(p.get(),"FixtureSentinel",77),"real loaded registry seed");pc_set_reg_load(false);
    check(!p->vars_dirty&&!regs(*p).begin()->second.second&&pc_readreg2(p.get(),"EP20_Step")==0,"Horuru empty-story clean loaded premise");
    qi_addid(p.get());players.push_back(std::move(p));
    require(npc_parsesrcfile(argv[1])==1&&!errors,"actual complete helper declarations parse");
    for(int generation=0;generation<2;++generation){
        require(npc_parsesrcfile(argv[2])==1&&!errors,"actual full owner declarations parse");
        for(auto& o:owners){auto* nd=npc_name2id(o.name.c_str());require(nd&&nd->u.scr.script,"native full owner lookup");
            check(nd->id!=o.prior&&nd->m==qi_map(o.mapname.c_str())&&nd->x==o.x&&nd->y==o.y,"fresh identity with original map/coordinates");o.prior=nd->id;}
        registration(0);
        int called=npc_event_doall("OnInit");check(called==(fixed?125:0),"real global OnInit startup enumeration");
        registration(fixed?1:0);
        if(fixed){
            auto& sd=*players[0];auto before=regs(sd);bool dirty=sd.vars_dirty;unsigned prior=condition_calls;
            for(int m=0;m<map_num;++m){sd.m=m;pc_show_questinfo_reinit(&sd);check(sd.qi_display.size()==map[m].qi_npc.size(),"actual native display sizing");pc_show_questinfo(&sd);}
            check(condition_calls==prior+125,"each source condition executes in actual VM on empty-story player");
            check(before==regs(sd)&&dirty==sd.vars_dirty&&!sd.quest_log,"condition execution is read-only for empty-story fixture");
            sd.m=0;pc_show_questinfo_reinit(&sd);sd.qi_display.pop_back();prior=condition_calls;pc_show_questinfo(&sd);
            check(condition_calls==prior,"wrong-sized display really skips native condition loop");pc_show_questinfo_reinit(&sd);
            check(condition_calls==prior,"reinit does not itself evaluate conditions");pc_show_questinfo(&sd);
            check(condition_calls==prior+map[0].qi_npc.size(),"restored display executes exact map owners");
        }
        for(auto& o:owners)click(o);registration(1);
        for(auto& o:owners)click(o);registration(fixed?1:2);
        if(fixed){check(npc_event_doall("OnInit")==125,"deliberate duplicate-startup control runs");registration(2);}
        require(npc_unloadfile(argv[2]),"actual native file unload");
        for(auto& o:owners)check(!npc_name2id(o.name.c_str())&&!qi_bl(o.prior),"unload removes native name and explicit world entry");
        for(int m=0;m<map_num;++m)check(map[m].qi_npc.empty()&&map[m].npc_num==0,"actual unload removes conditions and map memberships");
        check(npc_event_doall("OnInit")==0,"unload removes event registrations");
        check(!errors,"no parser or VM errors");
    }
    auto& sd=*players[0];check(!sd.st&&!sd.regs.arrays,"no suspended state/array residue");
    sd.regs.vars->destroy(sd.regs.vars,script_reg_destroy);sd.regs.vars=nullptr;qi_delid(&sd);players.clear();
    npc_unload(fake_nd,true);fake_nd=nullptr;require(world.empty(),"private world released");
    ev_db->destroy(ev_db,nullptr);npcname_db->destroy(npcname_db,nullptr);npc_path_db->destroy(npc_path_db,nullptr);script_event.clear();
    mob_db.clear();do_final_script();ers_destroy(num_reg_ers);ers_destroy(str_reg_ers);num_reg_ers=str_reg_ers=nullptr;timer_final();db_final();malloc_final();
    std::printf("EPISODE_QI_NATIVE_OK phase=%s owners=125 maps=27 cycles=2 clicks=%u assertions=%u failures=%u errors=%u condition_calls=%u\n",fixed?"fixed":"old",cases,assertions,failures,errors,condition_calls);
    return errors?3:failures?2:0;
}
