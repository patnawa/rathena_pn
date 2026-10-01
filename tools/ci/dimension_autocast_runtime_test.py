#!/usr/bin/env python3
"""Compile actual autocast dispatch functions with explicit world boundaries.

Exercises unmodified production skill_onskillusage, skill_get_casttype,
skill_castend_pos2, battle_check_range and Elemental Buster/ground skill methods.
DB fields come from the effective Renewal skill data. Geometry, skill placement,
resource debit, and damage delivery are isolated spies; no server is launched.
Requires Linux g++, Python and PyYAML; runs without prebuilt server objects.
"""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

import yaml

from episode_party_progression_test import scan_to

ROOT = Path(__file__).resolve().parents[2]
SKILL_SOURCE = ROOT / "src/map/skill.cpp"


def extract(path, name):
    source = (SKILL_SOURCE if path == "src/map/skill.cpp" else ROOT / path).read_text(encoding="utf-8")
    match = re.search(r"(?m)^[^\n;]*\b" + re.escape(name) + r"\s*\([^;]*?\)\s*(?:const\s*)?\{", source)
    if not match:
        raise AssertionError("Missing production function " + name)
    brace = source.index("{", match.start())
    return source[match.start():scan_to(source, brace, "{", "}") + 1]


def generate():
    names = ["EM_DIAMOND_STORM", "EM_TERRA_DRIVE", "EM_ELEMENTAL_BUSTER",
             "EM_ELEMENTAL_BUSTER_FIRE", "EM_ELEMENTAL_BUSTER_WATER",
             "EM_ELEMENTAL_BUSTER_WIND", "EM_ELEMENTAL_BUSTER_GROUND",
             "EM_ELEMENTAL_BUSTER_POISON", "ABC_DEFT_STAB", "ABC_ABYSS_DAGGER",
             "SOA_TALISMAN_OF_WHITE_TIGER", "SOA_TALISMAN_OF_BLACK_TORTOISE",
             "NW_SPIRAL_SHOOTING", "NW_WILD_FIRE", "IQ_FIRST_BRAND", "IQ_SECOND_FLAME",
             "IQ_THIRD_FLAME_BOMB", "GC_PHANTOMMENACE", "GC_ROLLINGCUTTER", "SHC_IMPACT_CRATER"]
    rows = {}
    for path in ("db/re/skill_db.yml", "db/import/skill_db.yml"):
        for row in (yaml.safe_load((ROOT / path).read_text(encoding="utf-8")) or {}).get("Body", []):
            rows.setdefault(row["Id"], {}).update(row)
    selected = {r["Name"]: r for r in rows.values() if r.get("Name") in names}
    assert set(selected) == set(names)
    constants = "enum {" + ",".join(f"{name}={selected[name]['Id']}" for name in names) + "};"
    init = []
    for name in names:
        row = selected[name]
        target = row.get("TargetType", "Passive")
        inf = {"Self": "INF_SELF_SKILL", "Ground": "INF_GROUND_SKILL",
               "Support": "INF_SUPPORT_SKILL", "Attack": "INF_ATTACK_SKILL", "Passive": "0"}[target]
        nodamage = str(bool(row.get("DamageFlags", {}).get("NoDamage", False))).lower()
        no_target_self = str(bool(row.get("Flags", {}).get("NoTargetSelf", False))).lower()
        init.append(f"add_skill({name}, {inf}, {nodamage}, {no_target_self});")
    functions = [extract("src/map/skill.cpp", name) for name in
                 ("skill_get_casttype", "skill_onskillusage", "skill_castend_pos2")]
    functions += [extract("src/map/battle.cpp", "battle_check_range")]
    functions += [extract("src/map/pc.cpp", "pc_bonus_autospell_onskill")]
    functions += [extract("src/map/skills/mage/elementalbuster.cpp", "SkillElementalBuster::castendNoDamageId")]
    functions += [extract("src/map/skills/mage/diamondstorm.cpp", "SkillDiamondStorm::castendPos2")]
    functions += [extract("src/map/skills/mage/terradrive.cpp", "SkillTerraDrive::castendPos2")]
    source = (ROOT / "tools/ci/dimension_autocast_runtime_test.cpp").read_text(encoding="utf-8")
    return source.replace("// SKILL_CONSTANTS", constants).replace("// DATABASE_INIT", "\n".join(init)).replace("// PRODUCTION_FUNCTIONS", "\n\n".join(functions))


def run(build, case, mutate):
    source = generate()
    if mutate:
        substitutions = {
            "range-origin": ("!battle_check_range(sd, tbl,", "!battle_check_range(bl, tbl,"),
            "ground-limit": ("tbl->y, skill, skill_lv, BL_PC, false)", "tbl->y, skill_id, skill_lv, BL_PC, false)"),
            "summon-guard": ("if (skill == EM_ELEMENTAL_BUSTER &&", "if (false && skill == EM_ELEMENTAL_BUSTER &&"),
        }
        old, new = substitutions[mutate]
        assert source.count(old) == 1, f"Mutation anchor drift: {mutate}"
        source = source.replace(old, new)
    cpp, exe = build / "dimension_autocast_runtime.cpp", build / "dimension_autocast_runtime"
    cpp.write_text(source, encoding="utf-8")
    subprocess.run(["g++", "-std=c++17", "-O0", "-g", "-Wall", "-Wextra",
                    "-Wno-unused-parameter", "-Wno-unused-but-set-variable", "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                    str(cpp), "-o", str(exe)], check=True)
    result = subprocess.run([str(exe), case], capture_output=True, text=True)
    (build / "result.log").write_text(result.stdout + result.stderr, encoding="utf-8")
    print(result.stdout, end=""); print(result.stderr, end="")
    if result.returncode:
        raise SystemExit(result.returncode)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--case", default="all", choices=("all", "em", "range", "ground", "chain"))
    parser.add_argument("--mutate", choices=("range-origin", "ground-limit", "summon-guard"))
    parser.add_argument("--build-dir", type=Path, help="Retain generated production functions, binary and result.log")
    parser.add_argument("--skill-source", type=Path, default=SKILL_SOURCE, help="Alternate production skill.cpp for baseline comparison")
    args = parser.parse_args()
    SKILL_SOURCE = args.skill_source.resolve()
    if args.build_dir:
        args.build_dir.mkdir(parents=True, exist_ok=True)
        run(args.build_dir.resolve(), args.case, args.mutate)
    else:
        with tempfile.TemporaryDirectory(prefix="dimension-autocast-") as directory:
            run(Path(directory), args.case, args.mutate)
