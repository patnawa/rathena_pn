#!/usr/bin/env python3
"""Native Airship Crash briefing checks with real RID and quest operations."""
import argparse
import hashlib
from pathlib import Path
import re
import tempfile

import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to

SOURCE = native.ROOT / 'npc/custom/instances/AirshipCrash.txt'


def fixtures(build, pre_fix=False):
    source = SOURCE.read_text()
    match = re.search(r'function\s+script\s+F_AirshipCrashBriefed\s*\{', source)
    begin = match.end()-1
    body = source[begin:scan_to(source, begin, '{', '}')+1]
    if pre_fix:
        begin = body.index('\t\t// Script boolean operands')
        end = body.index('\t\tif (checkquest(5911) != 2)',begin)
        body = body[:begin] + '\t\tif (!isloggedin(.@aid[.@i],.@cid[.@i]) || !attachrid(.@aid[.@i]) || checkquest(5911) != 2)' + body[end+len('\t\tif (checkquest(5911) != 2)'):]
    script = '''{
function Briefed;
'member_count = 2;
setarray 'member_aid[0],99000010,99000011;
setarray 'member_cid[0],100,101;
.@ready = Briefed();
'test_ready = .@ready;
'test_caller = getcharid(3);
'test_returned = 1;
end;
function Briefed ''' + body + '\n}'
    (build/'briefing.script').write_text(script)
    (build/'episode_cases.inc').write_text('static Case source_cases[] = {{"briefing","briefing.script","",0,false}};')
    return hashlib.sha256(script.encode()).hexdigest()


MAIN = r'''
extern "C" int __wrap_main(int argc,char** argv) {
    boundary(argc==2,"fixture directory supplied"); fixture_dir=argv[1];
    deny_network(); static char name[]="airship-briefing-test"; SERVER_NAME=name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings=0; battle_config.atcommand_disable_npc=0;
    npc.id=NPC; npc.type=BL_NPC; npc.instance_id=1;
    instances[1]=std::make_shared<s_instance_data>(); instances.at(1)->state=INSTANCE_BUSY;
    instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    for (int i=0;i<2;++i) {
        auto p=std::make_unique<map_session_data>(); p->id=99000010+i; p->type=BL_PC;
        p->status.account_id=p->id; p->status.char_id=100+i;
        p->fd=0; p->state.ignoretimeout=true; p->npc_idle_timer=INVALID_TIMER; players.emplace_back(std::move(p));
    }
    auto quest=std::make_shared<s_quest_db>(); quest->id=5911; quest_db.put(5911,quest);
    std::ifstream f(fixture_dir+"/briefing.script"); const std::string source{std::istreambuf_iterator<char>(f),std::istreambuf_iterator<char>()};
    codes["briefing"]=parse_script(source.c_str(),"briefing",1,0); boundary(codes["briefing"]!=nullptr,"actual helper parses");
    for (int scenario=0;scenario<3;++scenario) {
        offline_id=0; reset(); seed_quest(0,5911,true); seed_quest(1,5911,scenario!=1);
        if (scenario==0) offline_id=players[1]->id;
        finish("briefing");
        check(stage("'test_returned")==1,"offline or unbriefed member cannot abort requesting script");
        check(stage("'test_ready")==int(scenario==2),"only all online and briefed members are ready");
        check(stage("'test_caller")==players[0]->id,"requesting RID restored before returning");
    }
    offline_id=0; check(errors==0,"no native RID or quest diagnostics");
    for (auto& code:codes) script_free_code(code.second); codes.clear(); reset();
    script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear(); players.clear(); attached=nullptr;
    do_final_script(); timer_final(); db_final();
    std::printf("AIRSHIP_BRIEFING_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);
    malloc_final(); return failures||errors ? 1:0;
}
'''


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--prepare-only',action='store_true')
    parser.add_argument('--pre-fix',action='store_true',help='Restore eager combined condition in memory; expected native failure')
    args=parser.parse_args()
    prefix=native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix=prefix.replace('extern "C" map_session_data* ep_lookup(int32) asm', 'int32 offline_id=0;\nextern "C" map_session_data* ep_lookup(int32) asm')
    prefix=prefix.replace('if (p->id == id) return p.get();','if (p->id == id && id != offline_id) return p.get();')
    prefix += r'''
extern "C" block_list* briefing_bl(int32) asm("__wrap__Z9map_id2bli");
extern "C" block_list* briefing_bl(int32 id) { for (auto& p:players) if (p->id==id && id!=offline_id) return p.get(); return id==NPC ? &npc:nullptr; }
'''
    native.WRAPPERS += ('_Z9map_id2bli',)
    native.CPP=prefix+MAIN
    native.fixtures=fixtures
    def run(directory):
        directory.mkdir(parents=True,exist_ok=True)
        native.run(directory.resolve(),False,args.pre_fix,args.prepare_only,completion_marker='AIRSHIP_BRIEFING_RESULT ')
    if args.build_dir:
        run(args.build_dir)
    else:
        with tempfile.TemporaryDirectory(prefix='airship-briefing-') as temp:
            run(Path(temp))


if __name__=='__main__':
    main()
