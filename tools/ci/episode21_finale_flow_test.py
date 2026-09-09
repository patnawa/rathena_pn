#!/usr/bin/env python3
"""Extend the native encounter VM with finale dialogue races and party rewards.

Uses production NPC bodies, native script suspension and quest state; map,
inventory and network effects use the encounter harness's explicit doubles.
The select boundary chooses the first option after the real Next suspension.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import tempfile

import episode21_encounter_flow_test as harness
from episode_party_progression_test import npc_body

ROOT = harness.ROOT
TRANSITIONS = {
    "SecretAltar.txt": [("Lehar#ep21_sa_start", "'sa_stage", 0),
                        ("Jormungandr Shaman#ep21_sa", "'sa_stage", 4),
                        ("Cult Shaman#ep21_sa", "'sa_stage", 6)],
    "SilentSanctuary.txt": [("Aurelie#ep21_ss_start", "'ss_stage", 0),
                            ("Lasgand#ep21_ss", "'ss_stage", 1)],
    "FinalBattle.txt": [("Giant Egg#ep21_fb_story", "'fb_stage", 1)],
}
EXTRA = {
    "SecretAltar.txt": ["Lehar#ep21_sa_inner", "Giant Egg#ep21_sa"],
    "SilentSanctuary.txt": ["Aurelie#ep21_ss_end"],
    "FinalBattle.txt": ["Heine's Tablet#ep21_fb_start", "Nyar#ep21_fb_start",
                        "Tris#ep21_fb_channel", "Tan#ep21_fb_channel",
                        "Nadoyo#ep21_fb_channel", "Heine#ep21_fb_channel"],
}


def fixtures(build, pre_fix=False):
    path = build / "episode_cases.inc"
    entries = []
    for filename in TRANSITIONS:
        source = ROOT / "npc/custom/episode21" / filename
        enabled = (ROOT / 'npc/scripts_custom.conf').read_text()
        assert len(re.findall(r'(?m)^npc:\s*npc/custom/episode21/' + re.escape(filename) + r'\s*$', enabled)) == 1
        for name, var, stage in TRANSITIONS[filename] + [(name, "", 0) for name in EXTRA[filename]]:
            body = "{\n" + npc_body(source, name) + "\n}"
            if pre_fix:
                # Remove only the newly added after-yield guards. Initial
                # production guards and all spawn/reward commands remain.
                body = re.sub(r"(close2;\s*)if \('(?:sa_stage|ss_stage|fb_stage)[^\n]*\) end;\s*", r"\1", body)
                body = re.sub(r"\s*'sa_priests_started = 1;", "", body)
                body = re.sub(r"(close;\s*)if \('fb_stage != 0\) end;", r"\1", body)
            dest = "finale_" + str(len(entries)) + ".script"
            (build / dest).write_text(body)
            entries.append("{" + ",".join((json.dumps(name), json.dumps(dest), json.dumps(var), str(stage), str(bool(var)).lower())) + "},")
    header = "static Case source_cases[] = {\n" + "\n".join(entries) + "\n};\nstatic int quest_ids[] = {18348,18352,18353,18358,18359};\n"
    path.write_text(header)
    # Include body bytes: metadata alone must never allow stale positive or
    # negative fixtures to reuse a previously built executable.
    digest = hashlib.sha256(header.encode())
    for script in sorted(build.glob("*.script")):
        digest.update(script.read_bytes())
    return digest.hexdigest()


ADDITIONAL_CASES = r'''
    // Priest wave holds the same stage until all ten enemies die.
    reset(); stage("'sa_stage",3);
    invoke("Lehar#ep21_sa_inner",0); to_close2(0);
    invoke("Lehar#ep21_sa_inner",1); to_close2(1);
    acknowledge(0); check(spawned == 10, "priest wave has ten enemies");
    acknowledge(1); check(spawned == 10, "same-stage second dialogue cannot duplicate priests");
    reset(); stage("'sa_stage",3);
    invoke("Lehar#ep21_sa_inner",0); to_close2(0);
    stage("'sa_stage",4); acknowledge(0);
    check(spawned == 0, "late priest dialogue cannot restart an ended wave");
    for (const char* name : {"Tris#ep21_fb_channel", "Tan#ep21_fb_channel", "Nadoyo#ep21_fb_channel", "Heine#ep21_fb_channel"}) {
        reset(); stage("'fb_stage",1);
        invoke(name,0); to_close2(0); invoke(name,1); to_close2(1);
        acknowledge(0); auto bits = stage("'fb_channels");
        check(bits > 0, "first channel interaction records its bit");
        stage("'fb_stage",3); enabled.clear(); disabled.clear();
        acknowledge(1);
        check(stage("'fb_channels") == bits && enabled.empty() && disabled.empty(), "stale channel has no effects after combat begins");
    }
    for (const char* name : {"Heine's Tablet#ep21_fb_start", "Nyar#ep21_fb_start"}) {
        reset(); stage("'fb_stage",0);
        seed_quest(0,18348); seed_quest(1,18348);
        invoke(name,0); invoke(name,1);
        boundary(players[0]->st && players[1]->st, "both start dialogues suspended at Next");
        acknowledge(0); boundary(!players[0]->st, "first start completes");
        auto count = events.size(); auto move_count = moves.size();
        stage("'fb_stage",20); acknowledge(1);
        boundary(!players[1]->st, "late start completes");
        check(stage("'fb_stage") == 20 && count == events.size() && move_count == moves.size(), "stale selection cannot restart final battle");
    }
    // Reward NPC remains shared, but only the active character quest advances.
    reset(); stage("'sa_stage",8); seed_quest(0,18352); seed_quest(1,18352);
    capacity = false; finish("Giant Egg#ep21_sa",0);
    check(q(0,18352) == Q_ACTIVE && items[players[0]->id].empty(), "full inventory preserves altar quest");
    capacity = true; finish("Giant Egg#ep21_sa",0); finish("Giant Egg#ep21_sa",1);
    finish("Giant Egg#ep21_sa",0);
    for (int i : {0,1}) check(q(i,18353) == Q_ACTIVE && items[players[i]->id][1001618] == 60 && reputation[players[i]->id] == 50, "altar party reward is sixty vouchers once per member");
    reset(); stage("'ss_stage",3); seed_quest(0,18358); seed_quest(1,18358);
    finish("Aurelie#ep21_ss_end",0); finish("Aurelie#ep21_ss_end",1);
    check(q(0,18359) == Q_ACTIVE && q(1,18359) == Q_ACTIVE, "both sanctuary members receive story credit");
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--pre-fix", action="store_true", help="Remove only new guards in memory; expect regression failure")
    parser.add_argument("--prepare-only", action="store_true")
    parser.add_argument("--reuse-build", action="store_true")
    args = parser.parse_args()
    harness.fixtures = fixtures
    cpp = harness.CPP.replace('stage(test.var, test.stage);\n        invoke', 'stage(test.var, test.stage); if (std::string(test.name) == "Giant Egg#ep21_fb_story") stage("\'fb_channels",15);\n        invoke')
    cpp = cpp.replace('else if (command == "checkweight")', 'else if (command == "select") script_pushint(st, 1);\n    else if (command == "checkweight")')
    cpp = cpp.replace('"mapannounce", "questinfo", "checkweight"', '"select", "mapannounce", "questinfo", "checkweight"')
    start = cpp.index('    // Four Gimli dialogues')
    stop = cpp.index('    check(errors == 0, \"no parser or native quest errors\");', start)
    cpp = cpp[:start] + ADDITIONAL_CASES + '\n' + cpp[stop:]
    harness.CPP = cpp
    if args.build_dir:
        args.build_dir.mkdir(parents=True, exist_ok=True)
        harness.run(args.build_dir.resolve(), args.reuse_build, args.pre_fix, args.prepare_only)
    else:
        with tempfile.TemporaryDirectory(prefix="rathena-ep21-finale-") as temp:
            harness.run(Path(temp), False, args.pre_fix, args.prepare_only)


if __name__ == "__main__":
    main()
