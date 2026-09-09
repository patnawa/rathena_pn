#!/usr/bin/env python3
"""Native Undying dialogue, participant lock and reward transaction regressions.

Production NPC bodies run in the native script VM with native instance arrays,
character variables, name/ID lookup builtins and dialogue suspension. Clock,
random generator, visible map population, monster effects and inventory delivery
are explicit controlled boundaries. No server, player session or SQL is used.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

import episode21_encounter_flow_test as native
from episode_party_progression_test import npc_body

ROOT = native.ROOT
SOURCE = ROOT / 'npc/custom/episode20/Instances.txt'
EXTRA = r'''
int64 clock_now=100000;
int selection=1, random_value=0;
unsigned random_calls=0, timer_calls=0;
int vision_distance=3;
std::vector<int> cast_delays;
std::vector<int64> armor_divisors;
bool present[4]={true,true,false,false};
std::vector<std::tuple<int,int,int64>> unit_changes;
void array_value(script_state* st, int argument, int index, int64 value) {
    auto* data=script_getdata(st,argument);
    boundary(data_isreference(data),"world array argument is a real reference");
    set_reg_num(st,nullptr,reference_uid(reference_getid(data),reference_getindex(data)+index),get_str(reference_getid(data)),value,reference_getref(data));
}
extern "C" map_session_data* immortal_char(int32) asm("__wrap__Z13map_charid2sdi");
extern "C" map_session_data* immortal_char(int32 id) { for (auto& p:players) if (p->status.char_id==id) return p.get(); return nullptr; }
extern "C" block_list* immortal_bl(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* immortal_bl(int32 id) { for (auto& p:players) if (p->id==id) return p.get(); return id==NPC ? &npc : nullptr; }
extern "C" map_session_data* immortal_nick(const char*,bool) asm("__wrap__Z11map_nick2sdPKcb");
extern "C" map_session_data* immortal_nick(const char* name,bool) { for (auto& p:players) if (!std::strcmp(p->status.name,name)) return p.get(); return nullptr; }
'''
WORLD = r'''
    if (command == "gettimetick") script_pushint(st,clock_now);
    else if (command == "select") script_pushint(st,selection);
    else if (command == "instance_live_info") script_pushstrcopy(st,"The Undying");
    else if (command == "getmapunits") {
        unsigned count=0;
        for (unsigned i=0;i<players.size();++i) if (present[i]) array_value(st,4,count++,players[i]->id);
        script_pushint(st,count);
    } else if (command == "rand") { ++random_calls; script_pushint(st,random_value); }
    else if (command == "checkweight2") script_pushint(st,capacity);
    else if (command == "getunitdata") {
        array_value(st,3,UMOB_MODE,MD_CANMOVE|MD_CANATTACK|MD_AGGRESSIVE);
        array_value(st,3,UMOB_HP,1000000); array_value(st,3,UMOB_MAXHP,1000000);
        array_value(st,3,UMOB_X,80+(script_getnum(st,2)==90000001 ? 0:vision_distance));
        array_value(st,3,UMOB_Y,93);
        script_pushint(st,0);
    } else if (command == "setunitdata") unit_changes.emplace_back(script_getnum(st,2),script_getnum(st,3),script_getnum64(st,4));
    else if (command == "monster") { spawned+=script_getnum(st,7); script_pushint(st,90000000+spawned); }
    else if (command == "unitexists") script_pushint(st,1);
    else if (command == "mobcount") script_pushint(st,0);
    else if (command == "setnpctimer" || command == "initnpctimer" || command == "stopnpctimer") { ++timer_calls; }
    else if (command == "sc_start4") armor_divisors.push_back(script_getnum64(st,5));
    else if (command == "unitskillusepos") cast_delays.push_back(script_getnum(st,7));
    else if (command == "getmapxy") {
        auto* data=script_getdata(st,2);
        set_reg_str(st,nullptr,reference_getuid(data),get_str(reference_getid(data)),"1@twbs",reference_getref(data));
        array_value(st,3,0,80); array_value(st,4,0,80); script_pushint(st,0);
    }
    else if (command == "sc_start" || command == "sc_end" || command == "killmonster" || command == "unitskilluseid" || command == "unittalk") { }
    else if (command == "instance_mapname"'''
MAIN = r'''
extern "C" int __wrap_main(int argc,char** argv) {
    boundary(argc==2,"fixture directory supplied"); fixture_dir=argv[1];
    deny_network(); static char server_name[]="immortal-instance-test"; SERVER_NAME=server_name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings=0; battle_config.atcommand_disable_npc=0;
    npc.id=NPC; npc.type=BL_NPC; npc.instance_id=1;
    instances[1]=std::make_shared<s_instance_data>(); instances.at(1)->state=INSTANCE_BUSY;
    instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    for (int i=0;i<4;++i) {
        auto p=std::make_unique<map_session_data>();
        p->id=99000010+i; p->type=BL_PC; p->status.account_id=p->id;
        p->status.char_id=100+i; p->status.party_id=17; p->status.base_level=230;
        std::snprintf(p->status.name,sizeof(p->status.name),"Entrant%d",i);
        p->battle_status.hp=1000; p->battle_status.max_hp=1000;
        p->fd=0; p->state.ignoretimeout=true; p->npc_idle_timer=INVALID_TIMER;
        players.emplace_back(std::move(p));
    }
    auto quest=std::make_shared<s_quest_db>(); quest->id=18233; quest_db.put(18233,quest);
    for (const auto& test:source_cases) {
        std::ifstream f(fixture_dir+"/"+test.path);
        const std::string source{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
        auto* code=parse_script(source.c_str(),test.name,1,0);
        boundary(code!=nullptr,"production Undying body parses"); codes[test.name]=code;
    }
    boundary(errors==0,"no parser errors");
    const char* start="Lasgand#ep20_undying_start";
    const char* storage="Mysterious Storage#ep20";
    auto setup=[&]() {
        reset(); clock_now=100000; selection=1; random_value=0; random_calls=timer_calls=0;
        unit_changes.clear(); present[0]=present[1]=true; present[2]=present[3]=false;
        armor_divisors.clear(); cast_delays.clear(); vision_distance=3;
        for (auto& p:players) { p->status.party_id=17; registries[p->id]["EP20_Main_Complete"]=1; }
        for (auto& p:players) p->battle_status.hp=1000;
        finish("#EP20_Undying_Control::OnInstanceInit");
    };
    auto win=[&]() { finish("#EP20_Undying_Control::OnBossDead"); boundary(stage("'ep20_undying_stage")==2,"boss death unlocks storage"); };

    setup(); invoke(start,0); to_close2(0); selection=2; invoke(start,1); to_close2(1);
    acknowledge(0); auto first_spawn=spawned;
    check(stage("'ep20_undying_stage")==1 && stage("'ep20_undying_mode")==1,"first confirmed mode wins");
    acknowledge(1);
    check(spawned==first_spawn && stage("'ep20_undying_mode")==1,"late mode confirmation neither respawns boss nor changes mode");

    setup(); finish(start); win(); present[2]=true;
    finish(storage,2);
    check(items[players[2]->id].empty() && random_calls==0,"joining after fight start gives no loot or random rolls");
    finish(storage,0); finish(storage,1);
    check(items[players[0]->id][102567]==1 && items[players[1]->id][102567]==1,"both eligible participants claim their own Normal token");
    auto loot=items; auto exp=experience; auto rep=reputation; auto rolls=random_calls;
    clock_now+=86400; finish(storage,0); finish(storage,1);
    check(items==loot && experience==exp && reputation==rep && random_calls==rolls,"daily reset cannot reopen the same instance reward");

    setup(); registries[players[1]->id]["EP20_UndyingNormalNext"]=clock_now+10;
    finish(start); win(); clock_now+=20; finish(storage,1);
    check(items[players[1]->id].empty() && random_calls==0,"cooldown at fight start remains ineligible after reset");

    setup(); present[2]=present[3]=true; players[2]->status.party_id=99; registries[players[3]->id]["EP20_Main_Complete"]=0;
    finish(start); win(); finish(storage,2); finish(storage,3);
    check(items[players[2]->id].empty() && items[players[3]->id].empty(),"foreign party and unfinished-story characters excluded");

    setup(); selection=2; finish(start); win(); capacity=false; finish(storage,0);
    check(items[players[0]->id].empty() && reputation.empty(),"full inventory grants nothing");
    const auto cached_rolls=random_calls; check(cached_rolls==14,"all fourteen independent chances rolled once");
    capacity=true; random_value=9999; finish(storage,0);
    check(random_calls==cached_rolls,"capacity retry uses cached roll without rerolling");
    check(items[players[0]->id][102568]==1 && items[players[0]->id][1001251]==1,"cached winning High-mode rewards survive retry");
    check(registries[players[0]->id]["EP20_UndyingNormalNext"]==0,"High reward leaves Normal cooldown untouched");
    auto received=items; finish(storage,0); check(items==received,"repeat click cannot duplicate rewards");

    for (int difficulty:{1,2}) {
        setup(); selection=difficulty; finish(start);
        check(spawned==5,"one boss and four visions spawn");
        check(armor_divisors.size()==1 && armor_divisors[0]==1000,"opening protection uses exact thousandfold divisor");
        finish("#EP20_Undying_Control::OnTimer3000");
        check(std::find(unit_changes.begin(),unit_changes.end(),std::make_tuple(90000001,(int)UMOB_DAMAGETAKEN,(int64)(difficulty==2 ? 1:10)))!=unit_changes.end(),"opening timer restores mode-specific damage taken");
    }
    setup(); finish(start); players[1]->battle_status.hp=0;
    finish("#EP20_Undying_Control::OnTimer4000");
    check(stage("'ep20_undying_dead")==1,"fallen participant activates protection");
    players[1]->battle_status.hp=1000;
    finish("#EP20_Undying_Control::OnTimer4000");
    check(stage("'ep20_undying_dead")==0,"revived participant releases protection");
    win(); unit_changes.clear(); finish("#EP20_Undying_Control::OnTimer4000");
    check(unit_changes.empty(),"completed fight cannot resume timed effects");
    for (int distance:{3,8,14}) {
        setup(); finish(start); vision_distance=distance; stage("'ep20_undying_ticks",8);
        finish("#EP20_Undying_Control::OnTimer4000");
        check(cast_delays.size()==4,"all four ready visions cast");
        const int expected=distance<=5 ? 1:distance<=10 ? 3:5;
        check(std::all_of(cast_delays.begin(),cast_delays.end(),[&](int delay){return delay==expected;}),"vision cast delay follows distance band");
        finish("#EP20_Undying_Control::OnTimer4000");
        check(cast_delays.size()==4,"vision cooldown prevents immediate recast");
    }

    check(errors==0,"no native script/quest diagnostics");
    for (auto& entry:codes) script_free_code(entry.second); codes.clear();
    reset(); script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear();
    players.clear(); attached=nullptr; do_final_script(); timer_final(); db_final();
    std::printf("IMMORTAL_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);
    malloc_final(); return failures||errors ? 1:0;
}
'''


def fixtures(build, pre_fix=False):
    source = SOURCE.read_text()
    bodies = []
    for name in ('Lasgand#ep20_undying_start', 'Mysterious Storage#ep20'):
        body = '{\n'+npc_body(SOURCE,name)+'\n}'
        if pre_fix:
            # Sensitivity fixture: disable only the newly introduced defenses
            # in memory. This is not a historical rewrite of the encounter.
            if name.startswith('Lasgand'):
                body, count = re.subn(r'(close2;\s*)if \(instance_live_info\(ILI_NAME\) != "The Undying" \|\| \'ep20_undying_stage != 0\) end;', r'\1', body)
                assert count == 1
                body, count = re.subn(r'(?m)^\s*if \((?:getcharid\(1,\.@name\$\)|!getvar\(EP20_Main_Complete|\(\.@choice == 1 && getvar\(EP20_UndyingNormalNext)[^\n]*continue;', '', body)
                assert count == 3
            else:
                body, count = re.subn(r"if \('ep20_undying_claimed\[\.@member\]\)", 'if (0)', body)
                assert count == 1
                body, count = re.subn(r"if \(!'ep20_undying_rolled\[\.@member\]\)", "if (1)", body)
                assert count == 1
        bodies.append((name, body))
    name = '#EP20_Undying_Control'
    body = npc_body(SOURCE,name)
    for label in ('OnInstanceInit','OnBossDead','OnTimer3000','OnTimer4000'):
        match = re.search(r'(?m)^'+label+r':\s*\n',body)
        boundary = re.search(r'(?m)^\w+:',body[match.end():])
        stop = match.end()+boundary.start() if boundary else len(body)
        bodies.append((name+'::'+label,'{\n'+body[match.end():stop]+'\n}'))
    header=['static Case source_cases[] = {']
    for i,(name,body) in enumerate(bodies):
        filename=f'undying_{i}.script'; (build/filename).write_text(body)
        header.append('{'+','.join((json.dumps(name),json.dumps(filename),'""','0','false'))+'},')
    header.append('};\nstatic int quest_ids[] = {18233};')
    text='\n'.join(header); (build/'episode_cases.inc').write_text(text)
    print(json.dumps({'production_sha256': hashlib.sha256(source.encode()).hexdigest(), 'native_bodies': len(bodies), 'defenses_disabled': pre_fix}), flush=True)
    return hashlib.sha256((text+repr(bodies)).encode()).hexdigest()


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--pre-fix',action='store_true',help='Disable new defenses in memory; expected regression failure')
    args=parser.parse_args()
    prefix='#include <algorithm>\n'+native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix=prefix.replace('int32 world(script_state* st) {',EXTRA+'\nint32 world(script_state* st) {')
    prefix=prefix.replace('    if (command == "instance_mapname"',WORLD,1)
    prefix=prefix.replace('if (function == "EP21_GhostShipUnlocked")','''if (function == "EP20_MainComplete") script_pushint(st,registries[st->rid]["EP20_Main_Complete"]);
        else if (function == "EP20_NextReset") script_pushint(st,clock_now+100);
        else if (function == "EP20_Reward") { reputation[st->rid]+=script_getnum(st,4); experience[st->rid]={script_getnum64(st,5),script_getnum64(st,6)}; script_pushint(st,1); }
        else if (function == "EP21_GhostShipUnlocked")''',1)
    names=['gettimetick','select','instance_live_info','getmapunits','rand','checkweight2','getunitdata','setunitdata','unitexists','mobcount','setnpctimer','initnpctimer','stopnpctimer','sc_start','sc_start4','sc_end','killmonster','unitskilluseid','unitskillusepos','getmapxy','unittalk']
    prefix=prefix.replace('"getexp", "callfunc"};','"getexp", "callfunc",'+','.join(json.dumps(x) for x in names)+'};',1)
    native.WRAPPERS += ('_Z13map_charid2sdi','_Z9map_id2bli','_Z11map_nick2sdPKcb')
    native.CPP=prefix+MAIN; native.fixtures=fixtures
    def run(directory):
        directory.mkdir(parents=True,exist_ok=True)
        native.run(directory.resolve(),False,args.pre_fix,args.prepare_only,completion_marker='IMMORTAL_RESULT ')
    if args.build_dir: run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='immortal-instance-') as temp: run(Path(temp))


if __name__=='__main__': main()
