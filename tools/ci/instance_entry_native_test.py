#!/usr/bin/env python3
"""Native instance gates and rewards: cooldown isolation and instance ID reuse.

Production NPC body and instance/register builtins execute unchanged. Clock,
party ownership, menu choice and instance world movement are explicit doubles.
Linux map-server objects and the shared native encounter harness are required.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

import episode21_encounter_flow_test as native
from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
TARGETS = (
    ('npc/custom/instances/OldGlastHeimChallenge.txt', 'Oscar#oghcm_gate'),
    ('npc/custom/instances/FallOfGlastHeim.txt', 'Oscar#fogh_gate'),
    ('npc/custom/instances/FallOfGlastHeim.txt', 'Oscar#fogh_reward'),
    ('npc/custom/instances/AirshipCrash.txt', 'Dr. Strong#acr'),
    ('npc/custom/instances/TombOfRemorse.txt', 'Princess Tiara#tor_gate'),
)
BASELINE = 'bfe472585206fedcfc50e053384417337f3ccf2a'
BASELINE_HASHES = {
    'Oscar#oghcm_gate': 'ab54b9dc3e1ce34b34bb844f776c9f28fced09d52bb58730130d60412b728096',
    'Oscar#fogh_gate': '80fca9b63cd54269a311269b3b464c2063c1e55a834edfc6d0cbe2ae86ca6207',
    'Oscar#fogh_reward': '631750646051fb956c5919710b6c8a6f346bb61c00b6b845f3fdb724257c8b18',
    'Dr. Strong#acr': '487635fe314aed3a449252af97ee1dd4ba1bf0040e81323cea3d0b5066b555ca',
    'Princess Tiara#tor_gate': 'de25fceda89a9054e383cec5a791398a1113482137eef878a79c5bc993d490e4',
}
MAIN = r'''
extern "C" int __wrap_main(int argc, char** argv) {
    boundary(argc == 2, "fixture directory supplied"); fixture_dir = argv[1];
    deny_network(); static char name[] = "instance-entry-test"; SERVER_NAME = name;
    malloc_init(); db_init(); do_init_database(); timer_init(); install_world_doubles(); do_init_script();
    save_settings = 0; battle_config.atcommand_disable_npc = 0;
    npc.id = NPC; npc.type = BL_NPC; npc.instance_id = 1;
    instances[1] = std::make_shared<s_instance_data>();
    instances.at(1)->state = INSTANCE_BUSY;
    instances.at(1)->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    for (int i = 0; i < 2; ++i) {
        auto p = std::make_unique<map_session_data>();
        p->id = 99000010+i; p->type = BL_PC; p->status.account_id = p->id;
        p->status.char_id = 100+i; p->status.party_id = 17; p->status.base_level = 200;
        p->fd = 0; p->state.ignoretimeout = true; p->npc_idle_timer = INVALID_TIMER;
        players.emplace_back(std::move(p));
    }
    for (int id : quest_ids) { auto entry = std::make_shared<s_quest_db>(); entry->id = id; quest_db.put(id,entry); }
    for (const auto& test : source_cases) {
        std::ifstream file(fixture_dir + "/" + test.path);
        const std::string source{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        auto* code = parse_script(source.c_str(), test.name, 1, 0);
        boundary(code != nullptr, "actual gate parses"); codes[test.name] = code;
    }
    boundary(errors == 0, "no parser errors");
    const std::string gate = "Oscar#oghcm_gate";
    auto next = [&](int i)->int64& { return registries[players.at(i)->id]["OGHCM_Next"]; };

    reset(); entries = 0; clock_now = 100000; owned_instance = 1; entry_result = 0;
    next(0) = clock_now+5000;
    finish(gate);
    check(entries == 0, "cooldown character cannot join another party reservation");
    check(next(0) == clock_now+5000, "denial leaves cooldown untouched");

    reset(); entries = 0;
    finish(gate);
    check(entries == 1 && next(0) == clock_now+86400, "first successful admission starts 24h cooldown");
    clock_now += 120;
    finish(gate);
    check(entries == 2 && next(0) == clock_now+86280, "registered re-entry preserves original cooldown deadline");
    next(1) = clock_now+5000;
    finish(gate,1);
    check(entries == 2, "one member's registration cannot unlock another character");

    // Destroy all instance registers and create a fresh logical run with the
    // same numeric ID, as happens after server restart. Keep character state.
    script_free_vars(instances.at(1)->regs.vars);
    instances.at(1)->regs.vars = i64db_alloc(DB_OPT_RELEASE_DATA);
    if (instances.at(1)->regs.arrays) {
        instances.at(1)->regs.arrays->destroy(instances.at(1)->regs.arrays,script_free_array_db);
        instances.at(1)->regs.arrays = nullptr;
    }
    finish(gate);
    check(entries == 2, "reused numeric instance ID cannot bypass cooldown");

    reset(); entries = 0; entry_result = 3;
    finish(gate);
    check(entries == 1 && next(0) == 0, "failed admission does not charge cooldown");
    entry_result = 0; next(0) = clock_now+5000;
    finish(gate);
    check(entries == 1, "failed admission did not register a return exemption");

    reset(); entries = 0; owned_instance = 0;
    finish(gate);
    check(entries == 1 && next(0) == clock_now+86400, "leader creation records only successful admission");

    reset(); entries = 0; owned_instance = 1; active_name = "Fall of Glast Heim (Normal)";
    const std::string fogh_gate = "Oscar#fogh_gate", fogh_reward = "Oscar#fogh_reward";
    auto story = [&](int i) { seed_quest(i,12457,true); seed_quest(i,12458,true); };
    auto& registry = registries[players.at(0)->id];
    story(0); registry["FOGH_RunIID"] = 1; registry["FOGH_DailyNext"] = clock_now+5000;
    finish(fogh_gate);
    check(entries == 0, "persistent old Fall record ID cannot exempt a new instance from cooldown");

    reset(); entries = 0; story(0);
    registries[players.at(0)->id]["FOGH_RunIID"] = 1;
    registries[players.at(0)->id]["FOGH_RewardIID"] = 1;
    finish(fogh_gate);
    check(entries == 1 && q(0,12459) == Q_ACTIVE, "reused ID starts a fresh hunt and admission record");
    check(registries[players.at(0)->id]["FOGH_DailyNext"] == clock_now+86400, "fresh Fall entry charges cooldown despite old persistent ID");
    clock_now += 120; finish(fogh_gate);
    check(entries == 2 && registries[players.at(0)->id]["FOGH_DailyNext"] == clock_now+86280, "Fall re-entry preserves original deadline and hunt");
    stage("'fogh_hunt",12459);
    // Continue the pinned negative run after reporting its missing-hunt
    // failure, so reward regressions are measured independently as well.
    if (q(0,12459)==-1) seed_quest(0,12459);
    boundary(quest_update_status(players.at(0).get(),12459,Q_COMPLETE)==0,"complete real hunt");
    capacity = false; finish(fogh_reward);
    check(items[players.at(0)->id].empty() && q(0,12459)==Q_COMPLETE, "full inventory retains completed hunt and claim eligibility");
    capacity = true; finish(fogh_reward);
    check(items[players.at(0)->id][25739]==3 && items[players.at(0)->id][102550]==1, "old persistent reward ID cannot block legitimate new-run reward");
    if (q(0,12459)==-1) seed_quest(0,12459,true);
    finish(fogh_reward);
    check(items[players.at(0)->id][25739]==3 && items[players.at(0)->id][102550]==1, "same-instance reward replay is blocked even with a reseeded hunt");
    seed_quest(1,12459,true); finish(fogh_reward,1);
    check(items[players.at(1)->id][25739]==3 && items[players.at(1)->id][102550]==1, "reward claim isolation preserves other party members' rewards");

    for (int which = 0; which < 2; ++which) {
        const std::string gate_name = which ? "Princess Tiara#tor_gate" : "Dr. Strong#acr";
        active_name = which ? "Tomb of Remorse" : "Airship Crash";
        const std::string variable = which ? "TombRemorse" : "AirshipCrash";
        for (int scenario = 0; scenario < 4; ++scenario) {
            reset(); entries = 0; owned_instance = 1; entry_result = 0;
            players.at(0)->status.base_level = 250;
            registries[players.at(0)->id]["TombRemorse_Access"] = 1;
            seed_quest(0,5908,true);
            start_on_select = scenario == 0; roster_cid = 101;
            if (scenario == 1 || scenario == 2) {
                registries[players.at(0)->id][variable+"_ActiveInstance"] = 1;
                registries[players.at(0)->id][variable+"_ActiveUntil"] = clock_now+3600;
                stage("'started",1); stage("'member_count",1);
                stage("'member_cid",scenario == 2 ? 100 : 101);
            }
            finish(gate_name);
            check(entries == (scenario >= 2 ? 1 : 0), active_name +
                (scenario == 0 ? " rejects outsider when run starts during menu" :
                 scenario == 1 ? " rejects former entrant absent from starting roster" :
                 scenario == 2 ? " permits actual starting member re-entry" :
                                 " permits eligible entry before run starts"));
        }
    }
    start_on_select = false;

    for (auto& player : players) boundary(!player->st, "all dialogues completed");
    for (auto& entry : codes) script_free_code(entry.second); codes.clear();
    reset(); script_free_vars(instances.at(1)->regs.vars); instances.clear(); quest_db.clear();
    players.clear(); attached = nullptr;
    do_final_script(); timer_final(); db_final();
    std::printf("INSTANCE_ENTRY_RESULT checks=%u failures=%u errors=%u\n",checks,failures,errors);
    malloc_final(); return failures || errors ? 1 : 0;
}
'''


def before_fix(name, body):
    """Reverse this small patch in memory; require exact pinned baseline bytes."""
    if name == 'Oscar#oghcm_gate':
        start = body.index('\t// Only a character already admitted')
        end = body.index('\tif (!.@returning && OGHCM_Next', start)
        body = body[:start] + body[end:]
        body = body.replace('if (!.@returning && OGHCM_Next', 'if (!.@iid && OGHCM_Next')
        body = body.replace('\tif (!.@returning) {\n\t\tsetinstancevar \'ogh_entered[getcharid(0)],1,.@iid;\n\t\tOGHCM_Next = gettimetick(2) + 86400;\n\t}', '\tOGHCM_Next = gettimetick(2) + 86400;')
    elif name == 'Oscar#fogh_gate':
        body = body.replace('\t.@returning = 0;\n', '')
        start = body.index('\t\t// Instance IDs are reused')
        end = body.index('\t\tif (!.@returning && FOGH_DailyNext',start)
        body = body[:start]+body[end:]
        body = body.replace('if (!.@returning &&', 'if (FOGH_RunIID != .@iid &&')
        body = body.replace('if (!.@returning) {', 'if (FOGH_RunIID != .@iid) {')
        body = body.replace("setinstancevar 'fogh_entered[getcharid(0)],1,.@iid;", 'FOGH_RunIID = .@iid;')
    elif name == 'Oscar#fogh_reward':
        body = body.replace("if ('fogh_rewarded[getcharid(0)])", 'if (FOGH_RewardIID == .@iid)')
        body = body.replace("'fogh_rewarded[getcharid(0)] = 1;", 'FOGH_RewardIID = .@iid;')
    else:
        start = body.index('\t// The party can start while this entry menu is open.')
        end = body.index('\t.@result = instance_enter(',start)
        body = body[:start]+body[end:]
    if hashlib.sha256(body.encode()).hexdigest() != BASELINE_HASHES[name]:
        raise AssertionError('Negative fixture no longer matches pinned baseline '+BASELINE+' '+name)
    return body


def fixtures(build, pre_fix=False):
    header = 'static Case source_cases[] = {\n'
    bodies = ''
    for index,(path,name) in enumerate(TARGETS):
        source = (ROOT / path).read_text(encoding='utf-8')
        match = re.search(r'(?m)^.*\tscript\t'+re.escape(name)+r'\t[^\n]*\{', source)
        start = match.end()-1
        body = source[start:scan_to(source,start,'{','}')+1]
        if pre_fix:
            body = before_fix(name,body)
        filename = f'body_{index}.script'
        (build/filename).write_text(body,encoding='utf-8'); bodies += body
        header += '{'+json.dumps(name)+','+json.dumps(filename)+',"",0,false},\n'
        print(json.dumps({'npc':name,'sha256':hashlib.sha256(body.encode()).hexdigest(),'pre_fix':pre_fix}),flush=True)
    header += '};\nstatic int quest_ids[] = {5908,5909,12457,12458,12459,12460,12461,12462,16463,16464};\n'
    (build/'episode_cases.inc').write_text(header)
    return hashlib.sha256((header+bodies).encode()).hexdigest()


def run(build, pre_fix):
    prefix = native.CPP.split('extern "C" int __wrap_main',1)[0]
    prefix = prefix.replace('bool capacity = true;', 'bool capacity = true; int64 clock_now=100000; int owned_instance=1, entries=0, entry_result=0, roster_cid=101; bool start_on_select=false; std::string active_name="Old Glast Heim (Challenge Mode)";')
    prefix = prefix.replace('if (command == "instance_mapname"', '''if (command == "gettimetick") script_pushint(st,clock_now);
    else if (command == "select") {
        if (start_on_select) { stage("'started",1); stage("'member_count",1); stage("'member_cid",roster_cid); }
        script_pushint(st,1);
    }
    else if (command == "is_party_leader") script_pushint(st,1);
    else if (command == "instance_live_info") script_pushstrcopy(st,active_name.c_str());
    else if (command == "checkweight2") script_pushint(st,capacity);
    else if (command == "getitemname") script_pushstrcopy(st,"reward");
    else if (command == "instance_create") { owned_instance=1; script_pushint(st,1); }
    else if (command == "instance_enter") { ++entries; script_pushint(st,entry_result); }
    else if (command == "instance_mapname"''',1)
    prefix = prefix.replace('script_pushint(st, 1);\n    else if (command == "strnpcinfo")', 'script_pushint(st, owned_instance);\n    else if (command == "strnpcinfo")',1)
    prefix = prefix.replace('if (function == "EP21_GhostShipUnlocked")', '''if (function == "OGHCM_MaxLevel") script_pushint(st,10);
        else if (function == "F_AirshipCrashCleanup" || function == "F_TombRemorseCleanup") script_pushint(st,0);
        else if (function == "F_AirshipCrashNextReset") script_pushint(st,clock_now+259200);
        else if (function == "EP21_GhostShipUnlocked")''',1)
    prefix = prefix.replace('"mapannounce", "questinfo", "checkweight", "getitem", "getexp", "callfunc"};', '"mapannounce", "questinfo", "checkweight", "getitem", "getexp", "callfunc", "gettimetick", "select", "is_party_leader", "instance_live_info", "instance_create", "instance_enter", "checkweight2", "getitemname"};',1)
    native.CPP = prefix+MAIN
    native.fixtures = fixtures
    native.run(build,False,pre_fix,completion_marker='INSTANCE_ENTRY_RESULT ')


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build-dir',type=Path)
    p.add_argument('--pre-fix',action='store_true',help='Reconstruct pinned baseline in memory; expected regression failure')
    args = p.parse_args()
    if args.build_dir:
        args.build_dir.mkdir(parents=True,exist_ok=True); run(args.build_dir.resolve(),args.pre_fix)
    else:
        with tempfile.TemporaryDirectory(prefix='instance-entry-') as directory:
            run(Path(directory),args.pre_fix)
