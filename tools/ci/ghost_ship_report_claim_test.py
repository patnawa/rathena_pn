#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  ghost_ship_report_claim_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/ghost_ship_report_claim_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Real script-VM proof: a Ghost Ship run supplies one report per character.

Reuses the explicit world/UI boundaries and sanitized build routine of the
Episode 21 VM harness. The actual current finish and daily-report NPC bodies,
array builtins, getcharid and quest mutations execute without rewriting.
Daily-period time is an explicit controlled input, not a clock-function test.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

import yaml
import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'npc/custom/episode21/MysteriousGhostShip.txt'
BASELINE = '51c8195170e9d7018e3b2d10315c2dd8ab3e6c31'
PRE_FIX_HASH = '24bbfe0d1b6351c396ccd3477e7e29c343948afc5efa90f5b6808aa89fe14a4c'
NAMES = ('Maristella#ep21gs_finish', 'Nillem#ep21gs_daily')
QUESTS = (16816,16817,16821,16822,16823)

MAIN = r'''
namespace {
const std::string finish_npc = "Maristella#ep21gs_finish";
void inspect_claims(unsigned expected) {
    finish("@inspect");
    check(stage("'test_claim_count") == expected, "native getarraysize gives exact claim roster length");
}
void new_run() {
    // Same register teardown used by instance_destroy, without maps/NPCs/timers.
    auto old = instances.at(1);
    script_free_vars(old->regs.vars); old->regs.vars = nullptr;
    if (old->regs.arrays) old->regs.arrays->destroy(old->regs.arrays, script_free_array_db);
    old->regs.arrays = nullptr;
    instances.erase(1);
    instances[1] = std::make_shared<s_instance_data>();
    instances.at(1)->state = INSTANCE_BUSY;
    instances.at(1)->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    stage("'stage",22);
}
}
extern "C" int __wrap_main(int argc, char** argv) {
    boundary(argc == 2, "fixture directory required"); fixture_dir = argv[1];
    deny_network(); static char name[] = "ghost-ship-report-test"; SERVER_NAME = name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings = 0; battle_config.atcommand_disable_npc = 0;
    npc.id=NPC; npc.type=BL_NPC; npc.instance_id=1;
    instances[1]=std::make_shared<s_instance_data>();
    instances.at(1)->state=INSTANCE_BUSY; instances.at(1)->regs.vars=i64db_alloc(DB_OPT_RELEASE_DATA);
    for (int i=0;i<2;++i) {
        auto p=std::make_unique<map_session_data>();
        p->id=99000010+i; p->type=BL_PC; p->status.account_id=p->id;
        p->status.char_id=98000010+i; p->state.ignoretimeout=true;
        p->npc_idle_timer=INVALID_TIMER; players.emplace_back(std::move(p));
    }
    for (int id : quest_ids) { auto entry=std::make_shared<s_quest_db>(); entry->id=id; quest_db.put(id,entry); }
    for (const auto& test : source_cases) {
        std::ifstream file(fixture_dir+"/"+test.path);
        boundary(file.good(),"source-extracted actual NPC body opens");
        std::string source{std::istreambuf_iterator<char>(file),std::istreambuf_iterator<char>()};
        codes[test.name]=parse_script(source.c_str(),test.name,1,0);
        boundary(codes[test.name] && !errors,"actual body parses");
    }
    codes["@inspect"]=parse_script("{ 'test_claim_count = getarraysize('gs_reported); end; }","read-only roster inspection fixture",1,0);
    boundary(codes["@inspect"] && !errors,"native roster inspector parses");
    for (int state=0;state<22;++state) {
        reset(); stage("'stage",state); seed_quest(0,16816); seed_quest(1,16821);
        finish(finish_npc,0); finish(finish_npc,1);
        check(q(0,16816)==Q_ACTIVE && q(0,16817)==-1,"story cannot complete before final stage");
        check(q(1,16821)==Q_ACTIVE && q(1,16822)==-1,"daily cannot complete before final stage");
        inspect_claims(0);
    }
    reset(); stage("'stage",22);
    finish(finish_npc); inspect_claims(0);
    check(enabled.count("#EP21_GS_FinalExit"),"unquested visitor can leave without consuming a claim");
    seed_quest(0,16816); finish(finish_npc); inspect_claims(1);
    check(q(0,16816)==-1 && q(0,16817)==Q_ACTIVE,"story completion records native quest transition");
    check(i64db_i64get(instances.at(1)->regs.vars,reference_uid(add_str("'gs_reported"),0))==players[0]->status.char_id,
          "roster stores character ID, not account/session/party ID");
    // Legitimate later daily acceptance is represented by an actual quest_add.
    // The entrance dialogue itself is not executed by this report-only suite.
    seed_quest(0,16821); finish(finish_npc); inspect_claims(1);
    check(q(0,16821)==Q_ACTIVE && q(0,16822)==-1,"story then daily cannot reuse the same cleared run");
    players[0]->id+=100; players[0]->status.account_id+=100;
    finish(finish_npc); inspect_claims(1);
    check(q(0,16821)==Q_ACTIVE && q(0,16822)==-1,"fresh session identity cannot evade stable-character claim");
    players[1]->status.account_id=players[0]->status.account_id;
    seed_quest(1,16821); finish(finish_npc,1); inspect_claims(2);
    check(q(1,16821)==-1 && q(1,16822)==Q_ACTIVE,"another character retains its independent eligible claim");
    check(items.empty() && experience.empty() && reputation.empty(),"finish only records quests, no direct rewards");
    check(stage("'stage")==22 && moves.empty() && events.empty() && spawned==0,"claims do not reset or destroy the party encounter");
    // Destroy just the instance state and recreate the same numeric ID. Character
    // quest state stays intact: no persistent bare instance-ID lock is introduced.
    new_run(); inspect_claims(0); finish(finish_npc); inspect_claims(1);
    check(q(0,16821)==-1 && q(0,16822)==Q_ACTIVE,"new run can complete pending daily even when instance ID is reused");
    capacity=false; finish("Nillem#ep21gs_daily");
    check(q(0,16822)==Q_ACTIVE && items.empty() && experience.empty() && reputation.empty(),"capacity rejection preserves pending report and all rewards");
    inspect_claims(1); capacity=true; finish("Nillem#ep21gs_daily");
    check(q(0,16822)==-1 && q(0,16823)==Q_ACTIVE,"real daily report advances to completion quest");
    check(items[players[0]->id][1001618]==10 && items[players[0]->id][102947]==1 && reputation[players[0]->id]==5,
          "original voucher/antiquity/reputation reward remains exact");
    check(experience[players[0]->id]==std::make_pair<int64,int64>(91298960,59995050),"original experience remains exact");
    const auto before_rewards=std::make_tuple(items,experience,reputation);
    finish("Nillem#ep21gs_daily");
    check(before_rewards==std::make_tuple(items,experience,reputation),"same-day report retry cannot duplicate rewards");
    daily_period+=86400; seed_quest(0,16821); finish(finish_npc); inspect_claims(1);
    check(q(0,16821)==Q_ACTIVE && q(0,16822)==-1,"daily reset does not refresh a cleared-run claim");
    check(before_rewards==std::make_tuple(items,experience,reputation),"reset-boundary repeat finish gives no extra rewards");
    check(enabled.count("#EP21_GS_FinalExit") && !disabled.count(finish_npc),"repeat claimant can exit and cannot hide finish from other members");
    // Concurrent open dialogues for separate characters each record before close2.
    reset(); stage("'stage",22); seed_quest(0,16821); seed_quest(1,16821);
    invoke(finish_npc,0); to_close2(0); invoke(finish_npc,1); to_close2(1);
    check(q(0,16822)==Q_ACTIVE && q(1,16822)==Q_ACTIVE,"both native quest mutations complete before either close acknowledgement");
    acknowledge(1); acknowledge(0); inspect_claims(2);
    check(errors==0,"zero native parser/quest errors");
    reset();
    for (auto& entry : codes) script_free_code(entry.second);
    codes.clear(); players.clear(); attached=nullptr;
    script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear();
    do_final_script(); timer_final(); db_final(); malloc_final();
    std::printf("GHOST_SHIP_REPORT_RESULT cases=%u assertions=%u failures=%u errors=%u\n",cases-1,checks,failures,errors);
    return failures || errors ? 1 : 0;
}
'''


def make_fixtures(build, pre_fix=False):
    current = SCRIPT.read_text(encoding='utf-8')
    source = current
    if pre_fix:
        raw = subprocess.check_output(['git','show',BASELINE+':'+SCRIPT.relative_to(ROOT).as_posix()],cwd=ROOT)
        options = (raw,raw.replace(b'\r\n',b'\n'),raw.replace(b'\r\n',b'\n').replace(b'\n',b'\r\n'))
        if not any(hashlib.sha256(v).hexdigest()==PRE_FIX_HASH for v in options):
            raise AssertionError('Pinned pre-fix source hash mismatch')
        source = raw.decode('utf-8').replace('\r\n','\n')
    config=(ROOT/'npc/scripts_custom.conf').read_text()
    if len(re.findall(r'^npc:\s*npc/custom/episode21/MysteriousGhostShip.txt\s*$',config,re.M))!=1:
        raise AssertionError('Ghost Ship must be enabled exactly once')
    records=yaml.safe_load((ROOT/'db/import/quest_db.yml').read_text())['Body']
    if not set(QUESTS)<={r['Id'] for r in records}: raise AssertionError('Actual quest dependencies missing')
    header=['static Case source_cases[] = {']
    for index,name in enumerate(NAMES):
        found=re.search(r'(?m)^[^\n]*\tscript(?:\(DISABLED\))?\t'+re.escape(name)+r'\t[^\n]*\{',source)
        if not found: raise AssertionError('Missing actual NPC '+name)
        start=found.end()-1; body=source[start:scan_to(source,start,'{','}')+1]
        file='body_'+str(index)+'.script'; (build/file).write_text(body,encoding='utf-8')
        header.append('{'+','.join((json.dumps(name),json.dumps(file),'""','0','false'))+'},')
    header+=['};','static int quest_ids[] = {'+','.join(map(str,QUESTS))+'};']
    (build/'episode_cases.inc').write_text('\n'.join(header),encoding='utf-8')
    print(json.dumps({'pre_fix':pre_fix,'checked_source_sha256':hashlib.sha256(source.encode()).hexdigest(),
                      'actual_npcs':list(NAMES),'instance_array_and_quest_builtins':'native',
                      'daily_period':'controlled boundary; not a time-function test'}),flush=True)
    return hashlib.sha256((build/'episode_cases.inc').read_bytes()).hexdigest()


def run(build, pre_fix, prepare_only=False):
    if prepare_only:
        make_fixtures(build,pre_fix)
        return
    prefix=native.CPP.split('extern "C" int __wrap_main',1)[0]
    needle='bool capacity = true;'
    if prefix.count(needle)!=1: raise AssertionError('Shared fixture capacity declaration changed')
    prefix=prefix.replace(needle,needle+'\nint64 daily_period = 1788667200;')
    needle='else if (function == "EP21_MainComplete")'
    if prefix.count(needle)!=1: raise AssertionError('Shared callfunc boundary changed')
    prefix=prefix.replace(needle,'else if (function == "EP21_DailyKey") script_pushint(st, daily_period);\n        '+needle)
    old_cpp,old_fixtures=native.CPP,native.fixtures
    try:
        native.CPP=prefix+MAIN; native.fixtures=make_fixtures
        native.run(build,False,pre_fix,completion_marker='GHOST_SHIP_REPORT_RESULT ')
    finally:
        native.CPP,native.fixtures=old_cpp,old_fixtures


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build-dir',type=Path)
    parser.add_argument('--pre-fix',action='store_true',help='Execute exact previous source; expect claim-regression failure')
    parser.add_argument('--prepare-only',action='store_true',help='Write source-extracted fixtures for an existing identical native driver')
    args=parser.parse_args()
    if args.build_dir:
        build=args.build_dir.resolve(); build.mkdir(parents=True,exist_ok=True); run(build,args.pre_fix,args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix='ghost-ship-report-') as directory: run(Path(directory),args.pre_fix,args.prepare_only)
