#!/usr/bin/env python3
# ============================================================================
#  PN  /  DEVELOPMENT TOOLS
#  druid_monolith_runtime_test.py
# ----------------------------------------------------------------------------
#  Project contributions: (C) 2026 PN Development Team
#  License for project contributions: GPL-3.0-or-later; see LICENSE.
#  Source: https://github.com/patnawa/rathena_pn/blob/main/tools/ci/druid_monolith_runtime_test.py
#  Existing upstream authors, notices and other rights are retained.
# ============================================================================

"""Emit a native behavioral test of the actual Monolith/Nova/Stomp source.

The native test mocks map/status/packet seams, not the three production function
bodies. Pipe stdout into g++; no server/DB/client is modified. --source-ref can
show the regression against a prior commit using the same test expectations.
"""
import argparse
import subprocess
from pathlib import Path


def function(source, signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    for pos in range(opening + 1, len(source)):
        depth += (source[pos] == '{') - (source[pos] == '}')
        if depth == 0:
            return source[start:pos + 1]
    raise ValueError(f'Unterminated function: {signature}')


PRELUDE = r'''
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <vector>

using int16 = std::int16_t;
using int32 = std::int32_t;
using uint16 = std::uint16_t;
using t_tick = std::int64_t;
constexpr int AT_GLACIER_MONOLITH = 6592;
constexpr int SC_GLACIER_SHEILD = 1;
constexpr int BL_PC = 1;
constexpr int USESKILL_FAIL = 1;

struct block_list {
    int32 id = 100;
    int16 x = 20, y = 20, m = 1;
    block_list* prev = nullptr;
    bool dead = false;
};
struct map_session_data : block_list {};
struct skill_unit : block_list { bool alive = true; int16 range = 7; };
struct s_skill_unit_group {
    int32 skill_id = AT_GLACIER_MONOLITH;
    int32 src_id = 100, map = 1, unit_count = 1;
    skill_unit* unit = nullptr;
};
struct unit_data { std::vector<std::shared_ptr<s_skill_unit_group>> skillunits; };
struct status_change_entry { int32 val1 = 1, val2 = 20, val3 = 20, val4 = 1; };
struct status_change {
    bool active = true;
    status_change_entry cached;
    status_change_entry* getSCE(int) { return active ? &cached : nullptr; }
    bool hasSCE(int) const { return active; }
};

map_session_data player;
skill_unit monolith;
unit_data units;
status_change shield;
bool has_units = true, has_status = true, move_allowed = true;
int nova_hits = 0, stomp_hits = 0, failures = 0, moves = 0, effects = 0;
int damage_x = -1, damage_y = -1;

bool status_isdead(const block_list& src) { return src.dead; }
const unit_data* unit_bl2ud(const block_list*) { return has_units ? &units : nullptr; }
status_change* status_get_sc(block_list*) { return has_status ? &shield : nullptr; }
int distance_xy(int x1, int y1, int x2, int y2) { return std::max(std::abs(x1-x2), std::abs(y1-y2)); }
#define BL_CAST(type, ptr) static_cast<map_session_data*>(ptr)
void clif_skill_fail(map_session_data&, int, int) { ++failures; }
void clif_skill_nodamage(block_list*, block_list&, int, uint16) { ++effects; }
void clif_skill_poseffect(block_list&, int, uint16, int16, int16, t_tick) { ++effects; }
void clif_fixpos(block_list&) {}
bool unit_movepos(block_list* src, int16 x, int16 y, int32 easy, bool checkpath) {
    ++moves;
    if (easy != 2 || !checkpath) std::abort();
    if (!move_allowed) return false;
    src->x = x;
    src->y = y;
    return true;
}

class SkillImplRecursiveDamageSplash {
public:
    void castendPos2(block_list*, int32 x, int32 y, uint16, t_tick, int32&) const {
        ++nova_hits;
        damage_x = x;
        damage_y = y;
    }
    void castendDamageId(block_list*, block_list*, uint16, t_tick, int32&) const { ++stomp_hits; }
};
class SkillGlacialNova : public SkillImplRecursiveDamageSplash {
public:
    int getSkillId() const { return 6593; }
    void castendPos2(block_list*, int32, int32, uint16, t_tick, int32&) const;
};
class SkillGlacialStomp : public SkillImplRecursiveDamageSplash {
public:
    int getSkillId() const { return 6595; }
    void castendNoDamageId(block_list*, block_list*, uint16, t_tick, int32&) const;
};
'''


TESTS = r'''
int checks = 0;
void check(bool passed, const char* label) {
    ++checks;
    if (!passed) {
        std::fprintf(stderr, "FAIL: %s\n", label);
        std::exit(1);
    }
}
void reset() {
    player = {};
    player.prev = &player;
    monolith = {};
    monolith.prev = &monolith;
    auto group = std::make_shared<s_skill_unit_group>();
    group->unit = &monolith;
    units.skillunits = {group};
    shield = {};
    has_units = has_status = move_allowed = true;
    nova_hits = stomp_hits = failures = moves = effects = 0;
    damage_x = damage_y = -1;
}
void nova() {
    int32 flag = 0;
    SkillGlacialNova().castendPos2(&player, 0, 0, 1, 100, flag);
}
void stomp() {
    int32 flag = 0;
    SkillGlacialStomp().castendNoDamageId(&player, &player, 1, 100, flag);
}
void expect_rejected(const char* label) {
    nova();
    check(nova_hits == 0 && effects == 0, label);
    stomp();
    check(stomp_hits == 0 && moves == 0 && failures == 1, label);
}
int main() {
    reset();
    nova();
    check(nova_hits == 1 && damage_x == 20 && damage_y == 20, "live Monolith supports Nova");
    stomp();
    check(stomp_hits == 1 && moves == 1 && failures == 0, "live Monolith supports Stomp");

    reset(); units.skillunits.clear();
    expect_rejected("deleted Monolith must not attack or teleport from lingering status");
    reset(); player.x = 12;
    expect_rejected("distance eight exceeds the live Monolith range seven");
    reset(); player.x = 13; player.y = 13;
    nova(); stomp();
    check(nova_hits == 1 && stomp_hits == 1, "seven-cell diagonal boundary remains valid");

    reset(); shield.cached.val2 = 60; shield.cached.val3 = 60;
    nova(); stomp();
    check(damage_x == 20 && damage_y == 20 && player.x == 20 && player.y == 20,
          "replacement Monolith uses live coordinates, not stale cached coordinates");

    reset(); monolith.alive = false;
    expect_rejected("dead unit rejected");
    reset(); monolith.prev = nullptr;
    expect_rejected("unit removed from map rejected");
    reset(); monolith.m = 2;
    expect_rejected("unit on another map rejected");
    reset(); units.skillunits.front()->map = 2;
    expect_rejected("group on another map rejected");
    reset(); units.skillunits.front()->src_id = 101;
    expect_rejected("another caster's group rejected");
    reset(); units.skillunits.front()->skill_id = 6561;
    expect_rejected("Ice Pillar is not a Monolith");
    reset(); units.skillunits.front()->unit = nullptr;
    expect_rejected("uninitialized group rejected");
    reset(); units.skillunits.front()->unit_count = 0;
    expect_rejected("empty group rejected");
    reset(); units.skillunits = {nullptr};
    expect_rejected("null group rejected");
    reset(); monolith.range = -1;
    expect_rejected("disabled unit rejected");
    reset(); has_units = false;
    expect_rejected("missing caster unit data rejected");
    reset(); player.prev = nullptr;
    expect_rejected("caster removed from map rejected");
    reset(); player.dead = true;
    expect_rejected("dead caster rejected");
    reset(); shield.active = false;
    expect_rejected("required shield absent");
    reset(); has_status = false;
    expect_rejected("missing status container rejected");

    reset(); player.x = 13; move_allowed = false;
    stomp();
    check(moves == 1 && failures == 1 && stomp_hits == 0 && effects == 0,
          "unreachable destination retains path check and emits no success effect");

    reset();
    auto invalid = std::make_shared<s_skill_unit_group>();
    invalid->src_id = 999;
    units.skillunits.insert(units.skillunits.begin(), invalid);
    nova();
    check(nova_hits == 1, "unrelated group does not hide valid Monolith");
    reset(); monolith.range = 0;
    nova();
    check(nova_hits == 1, "zero-range live unit accepts exact same cell");
    reset(); monolith.range = 0; player.x = 19;
    expect_rejected("range is read from unit rather than hardcoded");

    std::printf("%d source-compiled Monolith runtime checks passed\n", checks);
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--source-ref', help='Read production functions from a Git commit instead of working files')
    args = parser.parse_args()
    def read(name):
        relative = 'src/map/skills/druid/' + name + '.cpp'
        if args.source_ref:
            return subprocess.check_output(['git', 'show', f'{args.source_ref}:{relative}'], cwd=args.root, text=True)
        return (args.root / relative).read_text(encoding='utf-8')
    print(PRELUDE)
    monolith = read('glacialmonolith')
    if 'const skill_unit* druid_find_active_monolith(' in monolith:
        print(function(monolith, 'const skill_unit* druid_find_active_monolith('))
    print(function(read('glacialnova'), 'void SkillGlacialNova::castendPos2('))
    print(function(read('glacialstomp'), 'void SkillGlacialStomp::castendNoDamageId('))
    print(TESTS)


if __name__ == '__main__':
    main()
